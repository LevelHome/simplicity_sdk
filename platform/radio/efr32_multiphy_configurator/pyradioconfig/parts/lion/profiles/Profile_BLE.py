from pyradioconfig.parts.common.profiles.lion_regs import build_modem_regs_lion
from pyradioconfig.parts.leopard.profiles.Profile_BLE import ProfileBLELeopard


class ProfileBLELion(ProfileBLELeopard):

    def __init__(self):
        super().__init__()
        self._profileName = "BLE"
        self._readable_name = "Bluetooth Low Energy Profile"
        self._category = ""
        self._description = "Profile used for BLE phys"
        self._default = False
        self._activation_logic = ""
        self._family = "lion"

    def build_register_profile_outputs(self, model, profile):
        build_modem_regs_lion(model, profile)