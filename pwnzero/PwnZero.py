import serial
from enum import Enum
import pwnagotchi.plugins as plugins
import time

class PwnZeroParam(Enum):
    FACE = 4
    NAME = 5
    CHANNEL = 6
    APS = 7
    UPTIME = 8
    FRIEND = 9
    MODE = 10
    HANDSHAKES = 11
    MESSAGE = 12

class PwnMode(Enum):
    MANU = 4
    AUTO = 5
    AI = 6

class PwnFace(Enum):
    NO_FACE = 4
    DEFAULT_FACE = 5
    LOOK_R = 6
    LOOK_L = 7
    LOOK_R_HAPPY = 8
    LOOK_L_HAPPY = 9
    SLEEP = 10
    SLEEP2 = 11
    AWAKE = 12
    BORED = 13
    INTENSE = 14
    COOL = 15
    HAPPY = 16
    GRATEFUL = 17
    EXCITED = 18
    MOTIVATED = 19
    DEMOTIVATED = 20
    SMART = 21
    LONELY = 22
    SAD = 23
    ANGRY = 24
    FRIEND = 25
    BROKEN = 26
    DEBUG = 27
    UPLOAD = 28
    UPLOAD1 = 29
    UPLOAD2 = 30

class PwnZero(plugins.Plugin):
    __author__ = "github.com/Matt-London"
    __version__ = "1.4.2"
    __license__ = "MIT"
    __description__ = "Plugin to display the Pwnagotchi on the Flipper Zero"

    PROTOCOL_START = 0x02
    PROTOCOL_END = 0x03

    def __init__(self, port="/dev/serial0", baud=115200):
        self._port = port
        self._baud = baud
        self._last_sent_time = 0
        self._update_interval = 2.0  # Increase throttle to 2 seconds
        self._last_sent_data = {}
        self._serialConn = None
        self._connect_serial()

    def _connect_serial(self):
        try:
            self._serialConn = serial.Serial(self._port, self._baud, timeout=1)
            print(f"Connected to {self._port} at {self._baud} baud.")
        except (FileNotFoundError, serial.SerialException) as e:
            print(f"Error connecting to {self._port}: {e}")
            self._serialConn = None

    def _is_connected(self):
        return self._serialConn and self._serialConn.is_open

    def close(self):
        if self._is_connected():
            self._serialConn.close()
            print("Serial connection closed.")

    def _is_byte(self, i: int) -> bool:
        return 0 <= i < 256

    def _str_to_bytes(self, s: str):
        return [ord(c) for c in s]

    def _send_data(self, param: int, args) -> bool:
        if not self._is_connected():
            print("Serial connection not established. Attempting to reconnect...")
            self._connect_serial()
            if not self._is_connected():
                print("Reconnection failed.")
                return False

        try:
            if not self._is_byte(param):
                print(f"Error: Invalid param byte: {param}")
                return False
            for i in args:
                if not self._is_byte(i):
                    print(f"Error: Invalid argument byte: {i}")
                    return False

            data = [self.PROTOCOL_START, param] + args + [self.PROTOCOL_END]
            bytes_written = self._serialConn.write(data)
            if bytes_written != len(data):
                print(f"Error: Only {bytes_written} of {len(data)} bytes written.")
                return False
            print(f"Data sent: {data}")
            return True
        except serial.SerialException as e:
            print(f"SerialException: {e}")
            self._serialConn.close()
            self._serialConn = None
            return False
        except Exception as e:
            print(f"Error sending data: {e}")
            return False

    def _has_state_changed(self, key, value):
        if key not in self._last_sent_data or self._last_sent_data[key] != value:
            self._last_sent_data[key] = value
            return True
        return False

    def _should_send_update(self):
        current_time = time.time()
        if current_time - self._last_sent_time >= self._update_interval:
            self._last_sent_time = current_time
            return True
        return False

    def set_face(self, face: PwnFace) -> bool:
        return self._send_data(PwnZeroParam.FACE.value, [face.value])

    def set_name(self, name: str) -> bool:
        if self._has_state_changed("name", name):
            print(f"Updating name to: {name}")
            return self._send_data(PwnZeroParam.NAME.value, self._str_to_bytes(name))
        print("Name has not changed; skipping update.")
        return True

    def set_channel(self, channelInfo: str) -> bool:
        return self._send_data(PwnZeroParam.CHANNEL.value, self._str_to_bytes(channelInfo))

    def set_aps(self, apsInfo: str) -> bool:
        return self._send_data(PwnZeroParam.APS.value, self._str_to_bytes(apsInfo))

    def set_uptime(self, uptimeInfo: str) -> bool:
        return self._send_data(PwnZeroParam.UPTIME.value, self._str_to_bytes(uptimeInfo))

    def set_mode(self, mode: PwnMode) -> bool:
        return self._send_data(PwnZeroParam.MODE.value, [mode.value])

    def set_handshakes(self, handshakesInfo: str) -> bool:
        return self._send_data(PwnZeroParam.HANDSHAKES.value, self._str_to_bytes(handshakesInfo))

    def set_message(self, message: str) -> bool:
        return self._send_data(PwnZeroParam.MESSAGE.value, self._str_to_bytes(message))

    def on_ui_update(self, ui):
        if not self._should_send_update():
            return

        updates = {}
        for key in ["mode", "status", "channel", "uptime", "aps", "name", "shakes", "face"]:
            value = ui.get(key)
            if value and self._has_state_changed(key, value):
                updates[key] = value

        for key, value in updates.items():
            if key == "mode":
                modeEnum = getattr(PwnMode, value.upper(), PwnMode.MANU)
                self.set_mode(modeEnum)
            elif key == "status":
                self.set_message(value)
            elif key == "channel":
                self.set_channel(value)
            elif key == "uptime":
                self.set_uptime(value)
            elif key == "aps":
                self.set_aps(value)
            elif key == "name":
                self.set_name(value.replace(">", ""))
            elif key == "face":
                faceEnum = getattr(PwnFace, value.upper(), None)
                if faceEnum:
                    self.set_face(faceEnum)
            elif key == "shakes":
                self.set_handshakes(value)
