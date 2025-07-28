import requests
import time
from enum import Enum
import sys


global_gdb_url = "http://127.0.0.1:5000"

if len(sys.argv) > 1 and sys.argv[1] == "true":
    global_toogle_setting = True
else:
    global_toogle_setting = False

cmds = [
    [
        "e0040001ff058000002c8000003c80000000000000000000000002f9017282a4ec81eb8502540be40085174876e80082abe194111111125421ca6dc452d289314280a0f8842a6580b9014407ed2379000000000000000000000000f313b370d28760b98a2e935e56be92feb2c4ec04000000000000000000000000eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee0000000000000000000000002f25deb3848c207fc8e0c34035b3ba7fc157602b000000000000000000000000f313b370d28760b98a2e935e56be92feb2c4ec04000000000000000000000000dad77910dbdfde764fc21fcd4e74d71bbaca6d8d000000000000000000000000000000000000000000",
        "9000",
    ],
    [
        "e00480018c000000030d98d59a9600000000000000000000000000000000000000000000000000000000000028a8527d000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001200000000000000000000000000000000000000000000000000000000000000000c0",
        "9000",
    ],
    [
        "b0060b00a101010102010211040000000212010013020002140101160400000000200863616c6c646174613002000831010b32012133210381c0821e2a14ac2546fb0b9852f37ca2789d7d76483d79217fb36f51dce1e7b434010135010215463044022077ff9625006cb8a4ad41a4b04ff2112e92a732bd263cce9b97d8e7d2536d04300220445b8ee3616fb907aa5e68359275e94d0a099c3e32a4fc8b3669c34083671f2f",
        "",
    ],
    [
        "e0260100d500d30001010102a4ec0214111111125421ca6dc452d289314280a0f8842a65030407ed237904201160dfb035ef7b9b2970b01a248fa15e5eb3731cab8af0316d5b609855400161050473776170060531696e6368070d31696e6368204e6574776f726b080831696e63682e696f09154167677265676174696f6e20526f757465722056360a0465c9d00081ff483046022100c65a5ca71cdde881a41a8d9ae2ba3e41ebb89b83817bbb47662051eb50f904270221008d9c97f470abbbc7308e0f5fb813dbb2503c766dcb1e6ac0927ccd46d712f4a0",
        "9000",
    ],
    [
        "b0060800a501010102010211040000000212010013020002140101160400000000200b45524332305f546f6b656e300200063101083201213321024cca8fad496aa5040a00a7eb2f5cc3b85376d88ba147a7d7054a99c64056188734010135010215473045022100e3b956f93fbff0d41908483888f0f75d4714662a692f7a38dc6c41a13294f9370220471991becb3ca4f43413cadc8ff738a8cc03568bfa832b4dcfe8c469080984e5",
        "9000",
    ],
    [
        "e00a00006904555344432f25deb3848c207fc8e0c34035b3ba7fc157602b000000060000a4ec3046022100c986d7d103dcf0e433e0835ad88edbdeb65d0e1488ec68d0d4ebf6f10a658c3e0221009f5f28e7944d803e00db65aae14bcf1dea40e2fb7b1b65526b0caeea03309b25",
        "9000",
    ],
    [
        "e02801002b002900010101084578656375746f7202010003170001010112000101010105030a00010101020000040103",
        "9000",
    ],
    [
        "e02801005c005a000101010453656e64020102034c0001010119000101010101020120030e00010101020001010200040401030216000101010105030e00010101020001010200000401030314eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
        "9000",
    ],
    [
        "e02801005f005d000101010752656365697665020102034c0001010119000101010101020120030e00010101020001010200050401030216000101010105030e00010101020001010200010401030314eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
        "9000",
    ],
    [
        "e004000200",
        "012e3030343430303100303030303030303030300002034c0001010119000101010101020120030e00010101020001010200050401030216000101010105030e009000",
    ],
]


class Button(Enum):
    LEFT = "left"
    RIGHT = "right"
    BOTH = "both"


def press_button(button: Button):
    try:
        json = {"action": "press-and-release"}
        response = requests.post(f"{global_gdb_url}/button/" + button.value, json=json)
    except Exception as e:
        print(f"error pressing {button}: {e}")
        return

    print(f"pressed {button}")
    print(response)


def enable_setting_contract_data(sleep_time: float = 0):
    buttons = [
        # GET in settings
        Button.RIGHT,
        Button.BOTH,
        # TOGGLE THE FIRST SETTING
        Button.BOTH,
        # EXIT SETTINGS
        Button.RIGHT,
        Button.RIGHT,
        Button.RIGHT,
        Button.RIGHT,
        Button.BOTH,
        # GO BACK TO 1ST SCREEN
        Button.LEFT,
    ]
    for button in buttons:
        press_button(button)
        time.sleep(sleep_time)


def send_apdu_to_gdb(apdu: str, expectedResponse: str = ""):
    print(f"=> {apdu}")
    response = requests.post(f"{global_gdb_url}/apdu", json={"data": apdu})
    data = response.json()["data"]
    print(f"<= {data}")
    if (expectedResponse != "") and expectedResponse not in data:
        print(f"Response mismatch: expected {expectedResponse}  vs {data}")
        return False
    return True


def main():
    if global_toogle_setting:
        enable_setting_contract_data()

    for i in range(len(cmds)):
        if not send_apdu_to_gdb(cmds[i][0], cmds[i][1]):
            break
        # temp = input()
        # if temp == "exit":
        #     print("exiting")
        #     break


main()
