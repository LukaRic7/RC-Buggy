from PyQt5.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget, QLabel
import serial, struct, os, time, sys, threading
from PyQt5.QtCore import QTimer
from collections import deque
import pyqtgraph as pg

# pip install PyQt5 pyqtgraph pyserial

DIR_PATH = os.path.dirname(__file__)
LOG_DIR  = os.path.join(DIR_PATH, '../logs')

PACKET_SIZE_BYTES = 25
BAUD_RATE         = 115_200
SERIAL_PORT       = 'COM3'
UPDATE_FREQ_HZ    = 50
HISTORY_SECONDS   = 60
HISTORY_SIZE      = UPDATE_FREQ_HZ * HISTORY_SECONDS

KEYS = [
    'rpm', 'kmh', 'temperature', 'corner', 'acceleration', 'pitch',
    'roll', 'wattage', 'batteryPct', 'frontVib', 'backVib'
]

# Shared memory between background thread and GUI
data_buffers = { key: deque(maxlen=HISTORY_SIZE) for key in KEYS }
time_buffer  = deque(maxlen=HISTORY_SIZE)

# Shared state for the UI label
file_info   = { 'name': 'Initializing...', 'size_kb': 0.0, 'lines': 0 }
app_running = True # Flag to stop the thread gracefully

def parse_packet(tuple_data:tuple) -> dict:
    """
    **Parses a packet into a dictionary and scales their values appropriately.**
    
    *Parameters*:
    - `tuple_data` (tuple): The extracted tuple containing all the telemetry data.
    
    *Returns*:
    - (dict): The parsed data.
    """

    keys   = ['header'] + KEYS + ['checksum']
    scales = [1, 1, 100, 10, 100, 100, 10, 10, 10, 10, 1, 1, 1]

    data = {}
    for value, key, scale in zip(tuple_data, keys, scales):
        data[key] = value / scale
    
    return data

def data_acquisition_thread():
    """
    **Runs in the background, generating data, writing to CSV, and updating buffers.**
    """
    
    if not os.path.exists(LOG_DIR):
        os.makedirs(LOG_DIR, exist_ok=True)

    filename          = f'{int(time.time())}.csv'
    log_file_path     = os.path.join(LOG_DIR, filename)
    file_info["name"] = filename

    with open(log_file_path, 'w') as file:
        file.write(','.join(KEYS) + '\n')
        
        ser           = serial.Serial(SERIAL_PORT, BAUD_RATE)
        start_time    = time.time()
        lines_written = 0

        while app_running:
            byte_data  = ser.read(PACKET_SIZE_BYTES)
            tuple_data = struct.unpack('<BHHhhhhhHHHH', byte_data)
            packet     = parse_packet(tuple_data)

            # Write scaled values to CSV based on the KEYS list
            csv_row = ','.join(str(packet[k]) for k in KEYS)
            file.write(csv_row + '\n')
            lines_written += 1

            # Update shared memory for the GUI
            current_time = time.time() - start_time
            time_buffer.append(current_time)
            for k in KEYS:
                data_buffers[k].append(packet[k])

            # Update file metadata
            file_info['lines'] = lines_written
            # Flush occasionally to get accurate file size reading during runtime
            if lines_written % 50 == 0: 
                file.flush()
                file_info["size_kb"] = os.path.getsize(log_file_path) / 1024
            
            time.sleep(1 / UPDATE_FREQ_HZ)

class RealTimeTelemetryGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Real-Time Telemetry Dashboard")
        self.resize(1200, 800)

        # Main Widget & Layout
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        layout = QVBoxLayout(main_widget)

        # File Info Label
        self.info_label = QLabel("Loading file data...")
        self.info_label.setStyleSheet("font-size: 16px; font-weight: bold; padding: 5px;")
        layout.addWidget(self.info_label)

        # Graphics Layout (The Grid)
        self.graph_widget = pg.GraphicsLayoutWidget()
        layout.addWidget(self.graph_widget)

        self.plots = {}
        self.curves = {}

        # Construct a 3x4 grid for the 11 graphs
        col_count = 0
        for key in KEYS:
            plot = self.graph_widget.addPlot(title=key.upper())
            plot.showGrid(x=True, y=True)
            plot.setLabel('bottom', 'Time (s)')
            
            # Create a line curve for this plot
            curve = plot.plot(pen=pg.mkPen(color='cyan', width=1.5))
            self.plots[key] = plot
            self.curves[key] = curve

            col_count += 1
            if col_count == 4:
                col_count = 0
                self.graph_widget.nextRow()

        # UI Update Timer (Running at 25Hz to save CPU)
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_dashboard)
        self.timer.start(40) 

    def update_dashboard(self):
        """
        **Pulls from the shared deques and updates the UI.**
        """

        # Update the text label
        self.info_label.setText(
            f"File: {file_info['name']}  |  "
            f"Size: {file_info['size_kb']:.2f} KB  |  "
            f"Lines (excl. header): {file_info['lines']}"
        )

        # Update the graphs
        if not time_buffer:
            return
            
        t_data = list(time_buffer)
        for key in KEYS:
            self.curves[key].setData(t_data, list(data_buffers[key]))

    def closeEvent(self, event):
        """
        **Ensures the background thread stops when you close the window.**
        """

        global app_running
        app_running = False
        event.accept()

if __name__ == '__main__':
    # Start the GUI Application
    app = QApplication(sys.argv)
    
    # Start the background data thread
    data_thread = threading.Thread(target=data_acquisition_thread, daemon=True)
    data_thread.start()

    # Show Window and execute app loop
    window = RealTimeTelemetryGUI()
    window.show()
    sys.exit(app.exec_())