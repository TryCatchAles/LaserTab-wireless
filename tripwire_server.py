import serial
import time
import threading
from flask import Flask, render_template_string, jsonify

# --- CONFIGURATION ---
SERIAL_PORT = 'COM4'  # <--- CHECK THIS
BAUD_RATE = 115200

# Global State
current_status = "UNKNOWN"

# Setup Serial
try:
    esp = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)
except:
    print("ERROR: Connect your ESP32!")
    esp = None

app = Flask(__name__)

# --- BACKGROUND THREAD: LISTEN TO ESP32 ---
def read_serial():
    global current_status
    while True:
        if esp and esp.in_waiting > 0:
            try:
                line = esp.readline().decode('utf-8').strip()
                # Parse the messages from ESP B
                if "STATUS:ALARM" in line:
                    current_status = "ALARM TRIGGERED!"
                elif "STATUS:ARMED" in line:
                    current_status = "SYSTEM ARMED"
                elif "STATUS:DISARMED" in line:
                    current_status = "SYSTEM DISARMED"
            except:
                pass
        time.sleep(0.1)

# Start the listener thread
thread = threading.Thread(target=read_serial)
thread.daemon = True
thread.start()

# --- WEB GUI ---
HTML_PAGE = """
<!DOCTYPE html>
<html>
<head>
    <title>Tripwire Control</title>
    <style>
        body { background: #121212; color: white; font-family: sans-serif; text-align: center; padding-top: 50px; }
        .status-box { font-size: 30px; margin-bottom: 30px; font-weight: bold; padding: 20px; border-radius: 10px; display: inline-block;}
        .btn { width: 80%; max-width: 300px; padding: 20px; font-size: 20px; margin: 10px; border: none; border-radius: 8px; cursor: pointer; }
        .arm { background: #2ecc71; color: white; }
        .disarm { background: #e74c3c; color: white; }
        footer { margin-top: 60px; text-align: center; border-top: 2px solid #555; padding-top: 30px; color: #aaa; font-size: 1rem; }
        .dev-names { font-weight: 700; color: white; font-size: 1.1rem; margin: 15px 0; }
    </style>
    <script>
        // Poll the server every 1 second to update status
        setInterval(function() {
            fetch('/get_status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('status-text').innerText = data.status;
                    
                    // Change color based on status
                    let box = document.getElementById('status-box');
                    if(data.status.includes("ALARM")) box.style.background = "#e74c3c";
                    else if(data.status.includes("ARMED")) box.style.background = "#2ecc71";
                    else box.style.background = "#555";
                });
        }, 1000);

        function sendCommand(cmd) {
            fetch('/' + cmd, { method: 'POST' });
        }
    </script>
</head>
<body>
    <h1>LASER SECURITY HUB</h1>
    
    <div id="status-box" class="status-box">
        STATUS: <span id="status-text">WAITING...</span>
    </div>
    
    <br>
    <button class="btn arm" onclick="sendCommand('arm')">ARM SYSTEM</button>
    <br>
    <button class="btn disarm" onclick="sendCommand('disarm')">DISARM SYSTEM</button>
    
    <footer>
        <p>PROJECT DEVELOPED BY</p>
        <p class="dev-names">
            Mohamed-Amine Moustakbal &bull; Ales Laiche &bull; Hyunseo Jeong &bull; Sitan Zhou
        </p>
        <p>&copy; 2026 Sentry Pro Security Solutions</p>
    </footer>
</body>
</html>
"""

@app.route('/')
def index():
    return render_template_string(HTML_PAGE)

@app.route('/get_status')
def get_status():
    return jsonify(status=current_status)

@app.route('/arm', methods=['POST'])
def arm():
    if esp: esp.write(b'1')
    return "OK"

@app.route('/disarm', methods=['POST'])
def disarm():
    if esp: esp.write(b'0')
    return "OK"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)