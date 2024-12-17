from pyradioconfig.parts.common.phys.phy_common import PHY_COMMON_FRAME_154
from pyradioconfig.calculator_model_framework.interfaces.iphy import IPhy
from py_2_and_3_compatibility import *


class PhysRAILBaseStandardIEEE802154Lynx(IPhy):
    # For now, inherit nothing from Panther
    # In the future, Lynx could inherit PHYs from Panther if the Panther definitions have logic to skip overrides for registers not defined in Lynx.
    #
    # Revisit the decision when we have a better feel for how different Lynx and Panther PHYs may end up.


    #FIXME: there seem to be lots of extra overrides in these PHYs. Wentao needs to go over these and remove them.

    ################################################################################################
    # "base" definitions - allows usage in multiple PHYs

    def IEEE802154_2p4GHz_base(self, phy, model):

        # No AGC overrides needed as this PHY does not inherit (Lynx defaults to fast loop settings)

        PHY_COMMON_FRAME_154(phy, model)
        # Override min length for 802.15.4E Seq# Suppression
        phy.profile_inputs.var_length_minlength.value = 4

        phy.profile_inputs.bandwidth_hz.value = 2524800
        phy.profile_inputs.base_frequency_hz.value = long(2405000000)
        phy.profile_inputs.baudrate_tol_ppm.value = 4000
        phy.profile_inputs.bitrate.value = 250000
        phy.profile_inputs.channel_spacing_hz.value = 5000000
        phy.profile_inputs.deviation.value = 500000
        phy.profile_inputs.diff_encoding_mode.value = model.vars.diff_encoding_mode.var_enum.DISABLED
        phy.profile_inputs.dsss_chipping_code.value = long(0x744AC39B)
        phy.profile_inputs.dsss_len.value = 32
        phy.profile_inputs.dsss_spreading_factor.value = 8
        phy.profile_inputs.frame_bitendian.value = model.vars.frame_bitendian.var_enum.LSB_FIRST
        phy.profile_inputs.frame_length_type.value = model.vars.frame_length_type.var_enum.VARIABLE_LENGTH
        phy.profile_inputs.frequency_comp_mode.value = model.vars.frequency_comp_mode.var_enum.INTERNAL_ALWAYS_ON
        phy.profile_inputs.fsk_symbol_map.value = model.vars.fsk_symbol_map.var_enum.MAP0
        phy.profile_inputs.if_frequency_hz.value = 1370000
        phy.profile_inputs.modulation_type.value = model.vars.modulation_type.var_enum.OQPSK
        phy.profile_inputs.pll_bandwidth_tx.value = model.vars.pll_bandwidth_tx.var_enum.BW_1500KHz
        phy.profile_inputs.pll_bandwidth_rx.value = model.vars.pll_bandwidth_rx.var_enum.BW_250KHz
        phy.profile_inputs.preamble_length.value = 32
        phy.profile_inputs.preamble_pattern.value = 0
        phy.profile_inputs.preamble_pattern_len.value = 4
        phy.profile_inputs.rx_xtal_error_ppm.value = 0
        phy.profile_inputs.shaping_filter.value = model.vars.shaping_filter.var_enum.Custom_OQPSK
        phy.profile_inputs.shaping_filter_param.value = 0.0
        phy.profile_inputs.symbol_encoding.value = model.vars.symbol_encoding.var_enum.DSSS
        phy.profile_inputs.syncword_0.value = long(0xe5)
        phy.profile_inputs.syncword_1.value = long(0x0)
        phy.profile_inputs.syncword_length.value = 8
        phy.profile_inputs.timing_detection_threshold.value = 65
        phy.profile_inputs.timing_resync_period.value = 2
        phy.profile_inputs.timing_sample_threshold.value = 0
        phy.profile_inputs.tx_xtal_error_ppm.value = 0
        phy.profile_inputs.xtal_frequency_hz.value = 38400000

        phy.profile_outputs.FRC_AUTOCG_AUTOCGEN.override = 7
        phy.profile_outputs.MODEM_CGCLKSTOP_FORCEOFF.override = 0x1003  # 0, 1, 12
        phy.profile_outputs.MODEM_TIMING_TIMTHRESH.override = 75
        # phy.profile_outputs.MODEM_SRCCHF_SRCENABLE1.override = 1 # Calc SRC

        if model.part_family.lower() == "lynx":
            phy.profile_outputs.RAC_SYNTHCTRL_MMDPOWERBALANCEDISABLE.override = 1

        if model.part_family.lower() not in ["rainier"]:
            phy.profile_outputs.SYNTH_LPFCTRL1CAL_OP1BWCAL.override = 11
            phy.profile_outputs.SYNTH_LPFCTRL1CAL_OP1COMPCAL.override = 14
            phy.profile_outputs.SYNTH_LPFCTRL1CAL_RFBVALCAL.override = 0
            phy.profile_outputs.SYNTH_LPFCTRL1CAL_RPVALCAL.override = 0
            phy.profile_outputs.SYNTH_LPFCTRL1CAL_RZVALCAL.override = 9

        phy.profile_outputs.AGC_CTRL1_RSSIPERIOD.override = 8

        phy.profile_outputs.rx_sync_delay_ns.override = 6125
        phy.profile_outputs.rx_eof_delay_ns.override = 6125

    ################################################################################################
    # "PHY" definitions - official PHY created here, simulation PHYs in Phys_sim_tests.py

    def PHY_IEEE802154_2p4GHz(self, model,phy_name=None):
        phy = self._makePhy(model, model.profiles.Base, readable_name='Legacy IEEE 802.15.4 2p4GHz PHY from Jumbo',phy_name=phy_name)
        self.IEEE802154_2p4GHz_base(phy, model)

        phy.profile_outputs.AGC_CTRL2_DISRFPKD.override = 1
        phy.profile_outputs.AGC_CTRL4_RFPKDCNTEN.override = 0

        return phy