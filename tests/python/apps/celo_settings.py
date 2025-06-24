from enum import Enum, auto
from typing import Union
from ledgered.devices import Device, DeviceType

from ragger.navigator import Navigator, NavInsID, NavIns


class SettingID(Enum):
    CONTRACT_DATA = auto()
    DEBUG_DATA = auto()
    VERBOSE_EIP712 = auto()


# Settings Positions per device. Returns the tuple (page, x, y)
SETTINGS_POSITIONS = {
    DeviceType.STAX: {
        SettingID.CONTRACT_DATA: (0, 350, 130),
        SettingID.DEBUG_DATA: (0, 350, 335),
        SettingID.VERBOSE_EIP712: (1, 350, 130),
    },
    DeviceType.FLEX: {
        SettingID.CONTRACT_DATA: (0, 420, 130),
        SettingID.DEBUG_DATA: (0, 420, 350),
        SettingID.VERBOSE_EIP712: (1, 420, 130),
    },
}


# The order of the settings is important, as it is used to navigate
def get_device_settings(device: Device) -> list[SettingID]:
    """Get the list of settings available on the device"""
    if device.is_nano:
        return [
            SettingID.CONTRACT_DATA,
            SettingID.DEBUG_DATA,
            SettingID.VERBOSE_EIP712,
        ]
    return [
        SettingID.CONTRACT_DATA,
        SettingID.DEBUG_DATA,
        SettingID.VERBOSE_EIP712,
    ]


def get_settings_moves(
    device: Device, to_toggle: list[SettingID]
) -> list[Union[NavIns, NavInsID]]:
    """Get the navigation instructions to toggle the settings"""
    moves: list[Union[NavIns, NavInsID]] = []
    settings = get_device_settings(device)
    # Assume the app is on the 1st page of Settings
    if device.is_nano:
        moves += [NavInsID.RIGHT_CLICK, NavInsID.BOTH_CLICK]
        for setting in settings:
            if setting in to_toggle:
                moves += [NavInsID.BOTH_CLICK]
            moves += [NavInsID.RIGHT_CLICK]
        moves += [NavInsID.BOTH_CLICK]  # Back
    else:
        current_page = 0
        moves += [NavInsID.USE_CASE_HOME_SETTINGS]
        for setting in settings:
            if setting in to_toggle:
                page, x, y = SETTINGS_POSITIONS[device.type][setting]
                moves += [NavInsID.USE_CASE_SETTINGS_NEXT] * (page - current_page)
                moves += [NavIns(NavInsID.TOUCH, (x, y))]
                if setting == SettingID.WEB3_CHECK:
                    # Assume Opt-In is not done, Add a confirmation step
                    moves += [NavInsID.USE_CASE_CHOICE_CONFIRM]
                    # Dismiss the notification
                    moves += [NavInsID.TAPPABLE_CENTER_TAP]
                current_page = page
        moves += [NavInsID.USE_CASE_SETTINGS_MULTI_PAGE_EXIT]
    return moves


def settings_toggle(device: Device, navigator: Navigator, to_toggle: list[SettingID]):
    """Toggle the settings"""
    moves = get_settings_moves(device, to_toggle)
    navigator.navigate(moves, screen_change_before_first_instruction=False)
