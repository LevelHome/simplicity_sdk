from pyradioconfig.parts.panther.calculators.calc_global import CALC_Global_panther
from pycalcmodel.core.variable import ModelVariableFormat

class CALC_Global_lynx(CALC_Global_panther):

    def buildVariables(self, model):

        # Build variables from the Panther calculations
        super().buildVariables(model)

        #Add variables for Lynx
        self._addModelVariable(model, 'preamsch_len', int, ModelVariableFormat.DECIMAL)

        self._addModelVariable(model, 'grpdelay_to_demod', int, ModelVariableFormat.DECIMAL)
        self._addModelVariable(model, 'delay_adc_to_demod_symbols', int, ModelVariableFormat.DECIMAL)
        self._addModelActual(model, 'phscale', float, ModelVariableFormat.DECIMAL)
        self._addModelActual(model, 'syncacqwin', int, ModelVariableFormat.DECIMAL)
        self._addModelActual(model, 'pmacquingwin', int, ModelVariableFormat.DECIMAL)

        self._addModelVariable(model, 'ksi1', int, ModelVariableFormat.DECIMAL)
        self._addModelVariable(model, 'ksi2', int, ModelVariableFormat.DECIMAL)
        self._addModelVariable(model, 'ksi3', int, ModelVariableFormat.DECIMAL)

        # These were extracted from Lynx register model and identified as new (and non-existent in the Panther map)

        # TODO: decide if all of these really belong in the calculator. For instance: enables, interrupt enables, etc.
        # TODO: do we really want the calculator to explicitly re-write these to their reset/default values a la MCUW_RADIO_CFG-734?

        # Must have a corresponding ModelVariableContainer entry in pyradioconfig/parts/common/profiles/lynx_regs.py build_modem_regs_lynx.
        # MCUW_RADIO_CFG-734 Panther: PHY/MAC Design requests more registers added (all MODEM, AGC, etc.) with re-writes of reset values

        # Add each register using a separate method
        # This is best practice moving forward so that we can handle inheritance granularly
        self._add_etsctrl_regs(model)
        self._add_antswctrl_regs(model)
        self._add_antswstart_regs(model)
        self._add_antswend_regs(model)
        self._add_cfgantpatt_regs(model)
        self._add_RAC_ADC_regs(model)

        self._addModelRegister(model, 'AGC.CTRL2.DEBCNTRST', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.CTRL4.PERIODRFPKD', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.CTRL4.RFPKDPRDGEAR', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.CTRL4.RFPKDSYNCSEL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.CTRL4.RFPKDSEL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.CTRL4.FRZPKDEN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.CTRL4.RFPKDCNTEN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.CTRL5.PNUPDISTHD', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.CTRL5.PNUPRELTHD', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.CTRL6.SEQPNUPALLOW', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.CTRL6.SEQRFPKDEN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.GAINSTEPLIM.PNINDEXMAX', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.PNRFATT3.LNAMIXRFATT15', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.PNRFATT3.LNAMIXRFATT16', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.PNRFATT3.LNAMIXRFATT17', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'AGC.SEQIF.RSSIVALID', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.DFLCTRL.DFLBOIOFFSET', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.CTRL.LPMODEDIS', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.CTRL.WAITEOFEN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.TRAILTXDATACTRL.TRAILTXREPLEN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.TRAILTXDATACTRL.TXSUPPLENOVERIDE', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.WCNTCMP3.SUPPLENFIELDLOC', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.BOICTRL.BOIEN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.BOICTRL.BOIFIELDLOC', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.BOICTRL.BOIBITPOS', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.BOICTRL.BOIMATCHVAL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.DSLCTRL.DSLMODE', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.DSLCTRL.DSLBITORDER', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.DSLCTRL.DSLSHIFT', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.DSLCTRL.DSLOFFSET', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.DSLCTRL.DSLBITS', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.DSLCTRL.DSLMINLENGTH', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.DSLCTRL.RXSUPRECEPMODE', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.DSLCTRL.STORESUP', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.DSLCTRL.SUPSHFFACTOR', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'FRC.WCNTCMP4.SUPPLENGTH', int, ModelVariableFormat.HEX)
        # These are removed from RC as they are RAIL-owned
        # self._addModelRegister(model, 'FRC.PKTBUFCTRL.PKTBUFSTARTLOC', int, ModelVariableFormat.HEX)
        # self._addModelRegister(model, 'FRC.PKTBUFCTRL.PKTBUFTHRESHOLD', int, ModelVariableFormat.HEX)
        # self._addModelRegister(model, 'FRC.PKTBUFCTRL.PKTBUFTHRESHOLDEN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.DIGMIXCTRL.DIGMIXFB', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.VTTRACK.SYNCTIMEOUTSEL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.LRFRC.LRCORRMODE', int, ModelVariableFormat.HEX)
        # self._addModelRegister(model, 'MODEM.RAMPCTRL.RAMPRATE0', int, ModelVariableFormat.HEX)
        # self._addModelRegister(model, 'MODEM.RAMPCTRL.RAMPRATE1', int, ModelVariableFormat.HEX)
        # self._addModelRegister(model, 'MODEM.RAMPCTRL.RAMPRATE2', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.REALTIMCFE.MINCOSTTHD', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.REALTIMCFE.RTSCHWIN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.REALTIMCFE.RTSCHMODE', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.REALTIMCFE.TRACKINGWIN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.REALTIMCFE.SYNCACQWIN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.REALTIMCFE.SINEWEN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.REALTIMCFE.VTAFCFRAME', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.REALTIMCFE.RTCFEEN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.TRECPMPATT.PMEXPECTPATT', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.TRECPMDET.PMACQUINGWIN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.TRECPMDET.PMCOSTVALTHD', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.TRECPMDET.PMTIMEOUTSEL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.TRECPMDET.PHSCALE', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.TRECPMDET.PMMINCOSTTHD', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.TRECPMDET.VTPMDETSEL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.TRECPMDET.COSTHYST', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.TRECPMDET.PREAMSCH', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.ETSTIM.ETSTIMVAL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.ETSTIM.ETSCOUNTEREN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.ANTSWCTRL1.TIMEPERIOD', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.COCURRMODE.CONCURRENT', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.ANTDIVCTRL.ADPRETHRESH', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.ANTDIVCTRL.ENADPRETHRESH', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.BLEIQDSAEXT2.DISMAXPEAKTRACKMODE', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.BLEIQDSAEXT2.BBSSDEBOUNCETIM', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.BLEIQDSAEXT2.BBSSDIFFCHVAL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.CLKMULTEN0.CLKMULTENDRVN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.CLKMULTEN0.CLKMULTENREG3', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.CLKMULTEN0.CLKMULTENBYPASS40MHZ', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.CLKMULTEN0.CLKMULTREG3ADJV', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.CLKMULTEN1.CLKMULTDRVAMPSEL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.LNAMIXDEBUG.LNAMIXDISMXR', int, ModelVariableFormat.HEX)
        # Already added as model in Panther, but not an output
        # self._addModelRegister(model, 'RAC.LNAMIXTRIM0.LNAMIXRFPKDBWSEL', int, ModelVariableFormat.HEX)
        # self._addModelRegister(model, 'RAC.LNAMIXTRIM0.LNAMIXRFPKDCALDM', int, ModelVariableFormat.HEX)
        # self._addModelRegister(model, 'RAC.LNAMIXTRIM0.LNAMIXTRIMVREG', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.LNAMIXTRIM1.LNAMIXIBIASADJ', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.LNAMIXTRIM1.LNAMIXLNACAPSEL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.LNAMIXTRIM1.LNAMIXMXRBIAS', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.LNAMIXTRIM1.LNAMIXNCASADJ', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.LNAMIXTRIM1.LNAMIXPCASADJ', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.LNAMIXTRIM1.LNAMIXVOUTADJ', int, ModelVariableFormat.HEX)
        # These are removed from RC as they are RAIL-owned
        # self._addModelRegister(model, 'RAC.LNAMIXCAL.LNAMIXENIRCAL', int, ModelVariableFormat.HEX)
        # self._addModelRegister(model, 'RAC.LNAMIXCAL.LNAMIXIRCALAMP', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.SYTRIM1.SYLODIVRLOADCCLKSEL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.SYTRIM1.SYLODIVSGTESTDIV', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.SYMMDCTRL.SYMMDMODE', int, ModelVariableFormat.HEX)


    def _add_etsctrl_regs(self, model):
        self._addModelRegister(model, 'MODEM.ETSCTRL.ETSLOC', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.ETSCTRL.CAPTRIG', int, ModelVariableFormat.HEX)

    def _add_antswctrl_regs(self, model):
        self._addModelRegister(model, 'MODEM.ANTSWCTRL.ANTDFLTSEL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.ANTSWCTRL.ANTCOUNT', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.ANTSWCTRL.ANTSWTYPE', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.ANTSWCTRL.ANTSWRST', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.ANTSWCTRL.CFGANTPATTEN', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.ANTSWCTRL.ANTSWENABLE', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.ANTSWCTRL.EXTDSTOPPULSECNT', int, ModelVariableFormat.HEX)

    def _add_antswstart_regs(self, model):
        self._addModelRegister(model, 'MODEM.ANTSWSTART.ANTSWSTARTTIM', int, ModelVariableFormat.HEX)

    def _add_antswend_regs(self, model):
        self._addModelRegister(model, 'MODEM.ANTSWEND.ANTSWENDTIM', int, ModelVariableFormat.HEX)

    def _add_cfgantpatt_regs(self, model):
        self._addModelRegister(model, 'MODEM.CFGANTPATT.CFGANTPATTVAL', int, ModelVariableFormat.HEX)

    def _add_SHAPING_regs(self, model):
        self._addModelRegister(model, 'MODEM.SHAPING2.COEFF9', int, ModelVariableFormat.HEX )
        self._addModelRegister(model, 'MODEM.SHAPING2.COEFF10', int, ModelVariableFormat.HEX )
        self._addModelRegister(model, 'MODEM.SHAPING2.COEFF11', int, ModelVariableFormat.HEX )
        self._addModelRegister(model, 'MODEM.SHAPING3.COEFF12', int, ModelVariableFormat.HEX )
        self._addModelRegister(model, 'MODEM.SHAPING3.COEFF13', int, ModelVariableFormat.HEX )
        self._addModelRegister(model, 'MODEM.SHAPING3.COEFF14', int, ModelVariableFormat.HEX )
        self._addModelRegister(model, 'MODEM.SHAPING3.COEFF15', int, ModelVariableFormat.HEX )
        self._addModelRegister(model, 'MODEM.SHAPING4.COEFF16', int, ModelVariableFormat.HEX )
        self._addModelRegister(model, 'MODEM.SHAPING4.COEFF17', int, ModelVariableFormat.HEX )
        self._addModelRegister(model, 'MODEM.SHAPING4.COEFF18', int, ModelVariableFormat.HEX )
        self._addModelRegister(model, 'MODEM.SHAPING4.COEFF19', int, ModelVariableFormat.HEX )
        self._addModelRegister(model, 'MODEM.SHAPING5.COEFF20', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING5.COEFF21', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING5.COEFF22', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING5.COEFF23', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING6.COEFF24', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING6.COEFF25', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING6.COEFF26', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING6.COEFF27', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING7.COEFF28', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING7.COEFF29', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING7.COEFF30', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING7.COEFF31', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING8.COEFF32', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING8.COEFF33', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING8.COEFF34', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING8.COEFF35', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING9.COEFF36', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING9.COEFF37', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING9.COEFF38', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING9.COEFF39', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING10.COEFF40', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING10.COEFF41', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING10.COEFF42', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING10.COEFF43', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING11.COEFF44', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING11.COEFF45', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING11.COEFF46', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'MODEM.SHAPING11.COEFF47', int, ModelVariableFormat.HEX)

    def _add_RAC_CLKMULTEN0_en(self, model):
        super()._add_RAC_CLKMULTEN0_en(model)
        self._addModelRegister(model, 'RAC.CLKMULTEN0.CLKMULTENDRVP', int, ModelVariableFormat.HEX)

    def _add_RAC_ADC_regs(self, model):
        self._addModelRegister(model, 'RAC.IFADCTRIM0.IFADCCLKSEL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.IFADCTRIM0.IFADCENHALFMODE', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.IFADCTRIM0.IFADCLDOSERIESAMPLVL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.IFADCTRIM0.IFADCLDOSHUNTAMPLVL1', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.IFADCTRIM0.IFADCLDOSHUNTAMPLVL2', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.IFADCTRIM0.IFADCLDOSHUNTCURLVL1', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.IFADCTRIM0.IFADCLDOSHUNTCURLVL2', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.IFADCTRIM0.IFADCOTACURRENT', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.IFADCTRIM0.IFADCREFBUFAMPLVL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.IFADCTRIM0.IFADCREFBUFCURLVL', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.IFADCTRIM0.IFADCSIDETONEAMP', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.IFADCTRIM0.IFADCSIDETONEFREQ', int, ModelVariableFormat.HEX)

        self._addModelRegister(model, 'RAC.IFADCTRIM1.IFADCNEGRESVCM', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.IFADCTRIM1.IFADCNEGRESCURRENT', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.IFADCTRIM1.IFADCENNEGRES', int, ModelVariableFormat.HEX)
        self._addModelRegister(model, 'RAC.IFADCTRIM1.IFADCVCMLVL', int, ModelVariableFormat.HEX)

