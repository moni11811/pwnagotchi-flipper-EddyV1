import serial
from enum import Enum

import pwnagotchi.plugins as plugins
import pwnagotchi.ui.faces as faces


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
    __version__ = "1.1.0"
    __license__ = "MIT"
    __description__ = "Plugin to display the Pwnagotchi on the Flipper Zero"

    PROTOCOL_START = 0x02
    PROTOCOL_END = 0x03

    def __init__(self, port: str = "/dev/serial0", baud: int = 115200):
        self._port = port
        self._baud = baud

        try:
            self._serialConn = serial.Serial(port, baud, timeout=1)
        except Exception as e:
            raise Exception(
                f"Cannot bind to port ({port}) with baud ({baud}): {e}")

    def close(self):
        self._serialConn.close()

    def _is_byte(self, i: int) -> bool:
        return 0 <= i < 256

    def _str_to_bytes(self, s: str):
        return [ord(c) for c in s]

    def _send_data(self, param: int, args) -> bool:
        if not self._is_byte(param) or any(not self._is_byte(i) for i in args):
            return False

        data = [self.PROTOCOL_START, param] + args + [self.PROTOCOL_END]
        return self._serialConn.write(data) == len(data)

    def _read_data(self):
        try:
            while self._serialConn.in_waiting:
                data = self._serialConn.read_until(bytes([self.PROTOCOL_END]))
                self._process_input(data)
        except Exception as e:
            print(f"Error reading data: {e}")

    def _process_input(self, data):
        if len(data) < 3 or data[0] != self.PROTOCOL_START or data[-1] != self.PROTOCOL_END:
            print("Invalid input data received")
            return

        param = data[1]
        args = data[2:-1]

        if param == PwnZeroParam.MODE.value:
            if args[0] == PwnMode.MANU.value:
                print("Switching to Manual mode")
                # Add logic to switch to manual mode
            elif args[0] == PwnMode.AUTO.value:
                print("Switching to Auto mode")
                # Add logic to switch to auto mode
            elif args[0] == PwnMode.AI.value:
                print("Switching to AI mode")
                # Add logic to switch to AI mode

    def set_face(self, face: PwnFace) -> bool:
        return self._send_data(PwnZeroParam.FACE.value, [face.value])

    def set_name(self, name: str) -> bool:
        return self._send_data(PwnZeroParam.NAME.value, self._str_to_bytes(name))

    def set_channel(self, channelInfo: str) -> bool:
        return self._send_data(PwnZeroParam.CHANNEL.value, self._str_to_bytes(channelInfo))

    def set_aps(self, apsInfo: str) -> bool:
        return self._send_data(PwnZeroParam.APS.value, self._str_to_bytes(apsInfo))

    def set_uptime(self, uptimeInfo: str) -> bool:
        return self._send_data(PwnZeroParam.UPTIME.value, self._str_to_bytes(uptimeInfo))

    def set_friend(self) -> bool:
        return False

    def set_mode(self, mode: PwnMode) -> bool:
        return self._send_data(PwnZeroParam.MODE.value, [mode.value])

    def set_handshakes(self, handshakesInfo: str) -> bool:
        return self._send_data(PwnZeroParam.HANDSHAKES.value, self._str_to_bytes(handshakesInfo))

    def set_message(self, message: str) -> bool:
        return self._send_data(PwnZeroParam.MESSAGE.value, self._str_to_bytes(message))

    def on_ui_setup(self, ui):
        pass

    def on_ui_update(self, ui):
        self.set_message(ui.get('status'))

        modeEnum = None
        if ui.get('mode') == 'AI':
            modeEnum = PwnMode.AI
        elif ui.get('mode') == 'MANU':
            modeEnum = PwnMode.MANU
        elif ui.get('mode') == 'AUTO':
            modeEnum = PwnMode.AUTO
        self.set_mode(modeEnum)

        self.set_channel(ui.get('channel'))
        self.set_uptime(ui.get('uptime'))
        self.set_aps(ui.get('aps'))
        self.set_name(ui.get('name').replace(">", ""))

        face = ui.get('face')
        faceEnum = getattr(PwnFace, face.upper(), None)
        if faceEnum:
            self.set_face(faceEnum)

        self.set_handshakes(ui.get('shakes'))

    def run(self):
        try:
            while True:
                self._read_data()
        except KeyboardInterrupt:
            print("Stopping plugin...")
