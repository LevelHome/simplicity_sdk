from pyradioconfig.parts.common.profiles.ocelot_regs import build_modem_regs_ocelot
from pyradioconfig.parts.lynx.profiles.Profile_BLE import ProfileBLELynx
from pycalcmodel.core.output import ModelOutput, ModelOutputType
from pyradioconfig.parts.common.profiles.profile_common import buildCrcOutputs, buildFecOutputs, buildFrameOutputs, \
    buildWhiteOutputs
from pyradioconfig.parts.ocelot.profiles.sw_profile_outputs_common import sw_profile_outputs_common_ocelot


class ProfileBLEOcelot(ProfileBLELynx):

    def __init__(self):
        super().__init__()
        self._profileName = "BLE"
        self._readable_name = "Bluetooth Low Energy Profile"
        self._category = ""
        self._description = "Profile used for BLE phys"
        self._default = False
        self._activation_logic = ""
        self._family = "ocelot"
        self._sw_profile_outputs_common = sw_profile_outputs_common_ocelot()

    def buildProfileModel(self, model):
        # Build profile object and append it to the model
        profile = self._makeProfile(model)

        # Build inputs
        self.build_required_profile_inputs(model, profile)
        self.build_optional_profile_inputs(model, profile)
        self.build_advanced_profile_inputs(model, profile)
        self.build_hidden_profile_inputs(model, profile)
        self.build_metadata_profile_inputs(model, profile)

        # Build outputs
        buildFrameOutputs(model, profile)
        buildCrcOutputs(model, profile)
        buildWhiteOutputs(model, profile)
        buildFecOutputs(model, profile)
        self.build_register_profile_outputs(model, profile)
        self.build_variable_profile_outputs(model, profile)
        self.build_info_profile_outputs(model, profile)

        self._sw_profile_outputs_common.buildStudioLogOutput(model, profile)

    def build_register_profile_outputs(self, model, profile):
        build_modem_regs_ocelot(model, profile)

    def build_variable_profile_outputs(self, model, profile):
        # RAIL Outputs
        self._sw_profile_outputs_common.build_rail_outputs(model, profile)

        # IRCal outputs
        self._sw_profile_outputs_common.build_ircal_outputs(model, profile)

    def build_info_profile_outputs(self, model, profile):
        # Informational output
        self._sw_profile_outputs_common.build_info_outputs(model, profile)
        profile.outputs.append(ModelOutput(model.vars.base_frequency_hz, '', ModelOutputType.INFO, readable_name="Base Channel Frequency"))
        profile.outputs.append(ModelOutput(model.vars.channel_spacing_hz, '', ModelOutputType.INFO, readable_name="Channel Spacing"))
        profile.outputs.append(ModelOutput(model.vars.agc_scheme, '', ModelOutputType.INFO, readable_name="AGC backoff scheme"))
        profile.outputs.append(ModelOutput(model.vars.bandwidth_hz, '', ModelOutputType.INFO, readable_name="Acquisition Channel Bandwidth"))
        profile.outputs.append(ModelOutput(model.vars.etsi_cat1_compatible, '', ModelOutputType.INFO, readable_name="ETSI Category 1 Compatibility"))
        profile.outputs.append(ModelOutput(model.vars.firstframe_bitsperword, '', ModelOutputType.INFO, readable_name="Length of the First Word"))
        profile.outputs.append(ModelOutput(model.vars.if_frequency_hz, '', ModelOutputType.INFO, readable_name="IF Frequency"))
        profile.outputs.append(ModelOutput(model.vars.lo_injection_side, '', ModelOutputType.INFO, readable_name="Injection side"))
        profile.outputs.append(ModelOutput(model.vars.pll_bandwidth_rx, '', ModelOutputType.INFO, readable_name="PLL Bandwidth in RX mode"))
        profile.outputs.append(ModelOutput(model.vars.pll_bandwidth_tx, '', ModelOutputType.INFO, readable_name="PLL Bandwidth in TX mode"))
        profile.outputs.append(ModelOutput(model.vars.src_disable, '', ModelOutputType.INFO, readable_name="SRC Operation"))
        profile.outputs.append(ModelOutput(model.vars.var_length_loc, '', ModelOutputType.INFO, readable_name="Byte position of dynamic length byte"))
        profile.outputs.append(ModelOutput(model.vars.fast_detect_enable, '', ModelOutputType.INFO, readable_name="Fast preamble detect"))
        profile.outputs.append(ModelOutput(model.vars.fec_tx_enable, '', ModelOutputType.INFO, readable_name="Enable FEC"))
        profile.outputs.append(ModelOutput(model.vars.preamble_detection_length, '', ModelOutputType.INFO, readable_name="Preamble Detection Length"))
        profile.outputs.append(ModelOutput(model.vars.skip2ant, '', ModelOutputType.INFO, readable_name="Skip 2nd antenna check with phase demod antenna diversity"))

    def _build_feature_settings(self, model):
        ble_feature = model.profile.inputs.ble_feature.var_value

        if ble_feature == model.vars.ble_feature.var_enum.AOX_1M:
            self._copy_model_variables_from_phy(model, 'PHY_Bluetooth_1M_AOX')
        elif ble_feature == model.vars.ble_feature.var_enum.AOX_2M:
            self._copy_model_variables_from_phy(model, 'PHY_Bluetooth_2M_AOX')
        else:
            super()._build_feature_settings(model)

    def _build_delay_settings(self, model):
        ble_feature = model.profile.inputs.ble_feature.var_value

        if ble_feature == model.vars.ble_feature.var_enum.LE_1M:
            model.vars.rx_sync_delay_ns.value_forced = 50000
            model.vars.rx_eof_delay_ns.value_forced = 10010
            model.vars.tx_sync_delay_ns.value_forced = 0
            model.vars.tx_eof_delay_ns.value_forced = 0
        elif ble_feature == model.vars.ble_feature.var_enum.LE_2M:
            model.vars.rx_sync_delay_ns.value_forced = 14500
            model.vars.rx_eof_delay_ns.value_forced = 4300
            model.vars.tx_sync_delay_ns.value_forced = 0
            model.vars.tx_eof_delay_ns.value_forced = 0
        elif ble_feature == model.vars.ble_feature.var_enum.AOX_1M:
            model.vars.rx_sync_delay_ns.value_forced = 50000
            model.vars.rx_eof_delay_ns.value_forced = 10010
            model.vars.tx_sync_delay_ns.value_forced = 0
            model.vars.tx_eof_delay_ns.value_forced = 0
        elif ble_feature == model.vars.ble_feature.var_enum.AOX_2M:
            model.vars.rx_sync_delay_ns.value_forced = 14500
            model.vars.rx_eof_delay_ns.value_forced = 4300
            model.vars.tx_sync_delay_ns.value_forced = 0
            model.vars.tx_eof_delay_ns.value_forced = 0