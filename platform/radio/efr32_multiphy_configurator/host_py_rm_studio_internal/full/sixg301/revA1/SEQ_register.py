
# -*- coding: utf-8 -*-

from . static import Base_RM_Register
from . SEQ_field import *


class RM_Register_SEQ_MMDDENOMINIT_CALC(Base_RM_Register):
    def __init__(self, rmio, label):
        self.__dict__['zz_frozen'] = False
        super(RM_Register_SEQ_MMDDENOMINIT_CALC, self).__init__(rmio, label,
            0xa0000000, 0x000,
            'MMDDENOMINIT_CALC', 'SEQ.MMDDENOMINIT_CALC', 'read-write',
            u"",
            0x00000000, 0x00000000,
            0x00001000, 0x00002000,
            0x00003000)

        self.DENOMINIT0 = RM_Field_SEQ_MMDDENOMINIT_CALC_DENOMINIT0(self)
        self.zz_fdict['DENOMINIT0'] = self.DENOMINIT0
        self.DENOMINIT1 = RM_Field_SEQ_MMDDENOMINIT_CALC_DENOMINIT1(self)
        self.zz_fdict['DENOMINIT1'] = self.DENOMINIT1
        self.__dict__['zz_frozen'] = True


class RM_Register_SEQ_MMDDENOMINIT_CALC_DOUBLED(Base_RM_Register):
    def __init__(self, rmio, label):
        self.__dict__['zz_frozen'] = False
        super(RM_Register_SEQ_MMDDENOMINIT_CALC_DOUBLED, self).__init__(rmio, label,
            0xa0000000, 0x004,
            'MMDDENOMINIT_CALC_DOUBLED', 'SEQ.MMDDENOMINIT_CALC_DOUBLED', 'read-write',
            u"",
            0x00000000, 0x00000000,
            0x00001000, 0x00002000,
            0x00003000)

        self.DENOMINIT0 = RM_Field_SEQ_MMDDENOMINIT_CALC_DOUBLED_DENOMINIT0(self)
        self.zz_fdict['DENOMINIT0'] = self.DENOMINIT0
        self.DENOMINIT1 = RM_Field_SEQ_MMDDENOMINIT_CALC_DOUBLED_DENOMINIT1(self)
        self.zz_fdict['DENOMINIT1'] = self.DENOMINIT1
        self.__dict__['zz_frozen'] = True


class RM_Register_SEQ_MODINDEX_CALC(Base_RM_Register):
    def __init__(self, rmio, label):
        self.__dict__['zz_frozen'] = False
        super(RM_Register_SEQ_MODINDEX_CALC, self).__init__(rmio, label,
            0xa0000000, 0x008,
            'MODINDEX_CALC', 'SEQ.MODINDEX_CALC', 'read-write',
            u"",
            0x00000000, 0x00000000,
            0x00001000, 0x00002000,
            0x00003000)

        self.MODINDEXM = RM_Field_SEQ_MODINDEX_CALC_MODINDEXM(self)
        self.zz_fdict['MODINDEXM'] = self.MODINDEXM
        self.MODINDEXE = RM_Field_SEQ_MODINDEX_CALC_MODINDEXE(self)
        self.zz_fdict['MODINDEXE'] = self.MODINDEXE
        self.FREQGAINE = RM_Field_SEQ_MODINDEX_CALC_FREQGAINE(self)
        self.zz_fdict['FREQGAINE'] = self.FREQGAINE
        self.FREQGAINM = RM_Field_SEQ_MODINDEX_CALC_FREQGAINM(self)
        self.zz_fdict['FREQGAINM'] = self.FREQGAINM
        self.AMMODINDEXM = RM_Field_SEQ_MODINDEX_CALC_AMMODINDEXM(self)
        self.zz_fdict['AMMODINDEXM'] = self.AMMODINDEXM
        self.AMMODINDEXE = RM_Field_SEQ_MODINDEX_CALC_AMMODINDEXE(self)
        self.zz_fdict['AMMODINDEXE'] = self.AMMODINDEXE
        self.__dict__['zz_frozen'] = True


class RM_Register_SEQ_MODINDEX_CALC_MODINDEXE_DOUBLED(Base_RM_Register):
    def __init__(self, rmio, label):
        self.__dict__['zz_frozen'] = False
        super(RM_Register_SEQ_MODINDEX_CALC_MODINDEXE_DOUBLED, self).__init__(rmio, label,
            0xa0000000, 0x00C,
            'MODINDEX_CALC_MODINDEXE_DOUBLED', 'SEQ.MODINDEX_CALC_MODINDEXE_DOUBLED', 'read-write',
            u"",
            0x00000000, 0x00000000,
            0x00001000, 0x00002000,
            0x00003000)

        self.MODINDEXM = RM_Field_SEQ_MODINDEX_CALC_MODINDEXE_DOUBLED_MODINDEXM(self)
        self.zz_fdict['MODINDEXM'] = self.MODINDEXM
        self.MODINDEXE = RM_Field_SEQ_MODINDEX_CALC_MODINDEXE_DOUBLED_MODINDEXE(self)
        self.zz_fdict['MODINDEXE'] = self.MODINDEXE
        self.FREQGAINE = RM_Field_SEQ_MODINDEX_CALC_MODINDEXE_DOUBLED_FREQGAINE(self)
        self.zz_fdict['FREQGAINE'] = self.FREQGAINE
        self.FREQGAINM = RM_Field_SEQ_MODINDEX_CALC_MODINDEXE_DOUBLED_FREQGAINM(self)
        self.zz_fdict['FREQGAINM'] = self.FREQGAINM
        self.AMMODINDEXM = RM_Field_SEQ_MODINDEX_CALC_MODINDEXE_DOUBLED_AMMODINDEXM(self)
        self.zz_fdict['AMMODINDEXM'] = self.AMMODINDEXM
        self.AMMODINDEXE = RM_Field_SEQ_MODINDEX_CALC_MODINDEXE_DOUBLED_AMMODINDEXE(self)
        self.zz_fdict['AMMODINDEXE'] = self.AMMODINDEXE
        self.__dict__['zz_frozen'] = True


class RM_Register_SEQ_SPARE4(Base_RM_Register):
    def __init__(self, rmio, label):
        self.__dict__['zz_frozen'] = False
        super(RM_Register_SEQ_SPARE4, self).__init__(rmio, label,
            0xa0000000, 0x010,
            'SPARE4', 'SEQ.SPARE4', 'read-write',
            u"",
            0x00000000, 0x00000000,
            0x00001000, 0x00002000,
            0x00003000)

        self.SPARE4 = RM_Field_SEQ_SPARE4_SPARE4(self)
        self.zz_fdict['SPARE4'] = self.SPARE4
        self.__dict__['zz_frozen'] = True


class RM_Register_SEQ_MODEMINFO(Base_RM_Register):
    def __init__(self, rmio, label):
        self.__dict__['zz_frozen'] = False
        super(RM_Register_SEQ_MODEMINFO, self).__init__(rmio, label,
            0xa0000000, 0x014,
            'MODEMINFO', 'SEQ.MODEMINFO', 'read-write',
            u"",
            0x00000000, 0x00000000,
            0x00001000, 0x00002000,
            0x00003000)

        self.LEGACY_EN = RM_Field_SEQ_MODEMINFO_LEGACY_EN(self)
        self.zz_fdict['LEGACY_EN'] = self.LEGACY_EN
        self.SPARE0 = RM_Field_SEQ_MODEMINFO_SPARE0(self)
        self.zz_fdict['SPARE0'] = self.SPARE0
        self.TRECS_EN = RM_Field_SEQ_MODEMINFO_TRECS_EN(self)
        self.zz_fdict['TRECS_EN'] = self.TRECS_EN
        self.BCR_EN = RM_Field_SEQ_MODEMINFO_BCR_EN(self)
        self.zz_fdict['BCR_EN'] = self.BCR_EN
        self.COHERENT_EN = RM_Field_SEQ_MODEMINFO_COHERENT_EN(self)
        self.zz_fdict['COHERENT_EN'] = self.COHERENT_EN
        self.LONGRANGE_EN = RM_Field_SEQ_MODEMINFO_LONGRANGE_EN(self)
        self.zz_fdict['LONGRANGE_EN'] = self.LONGRANGE_EN
        self.ENHDSSS_EN = RM_Field_SEQ_MODEMINFO_ENHDSSS_EN(self)
        self.zz_fdict['ENHDSSS_EN'] = self.ENHDSSS_EN
        self.SPARE1 = RM_Field_SEQ_MODEMINFO_SPARE1(self)
        self.zz_fdict['SPARE1'] = self.SPARE1
        self.SOFTMODEM_EN = RM_Field_SEQ_MODEMINFO_SOFTMODEM_EN(self)
        self.zz_fdict['SOFTMODEM_EN'] = self.SOFTMODEM_EN
        self.BTC_EN = RM_Field_SEQ_MODEMINFO_BTC_EN(self)
        self.zz_fdict['BTC_EN'] = self.BTC_EN
        self.SPARE2 = RM_Field_SEQ_MODEMINFO_SPARE2(self)
        self.zz_fdict['SPARE2'] = self.SPARE2
        self.__dict__['zz_frozen'] = True


class RM_Register_SEQ_MISC(Base_RM_Register):
    def __init__(self, rmio, label):
        self.__dict__['zz_frozen'] = False
        super(RM_Register_SEQ_MISC, self).__init__(rmio, label,
            0xa0000000, 0x018,
            'MISC', 'SEQ.MISC', 'read-write',
            u"",
            0x00000000, 0x00000000,
            0x00001000, 0x00002000,
            0x00003000)

        self.RESERVED10 = RM_Field_SEQ_MISC_RESERVED10(self)
        self.zz_fdict['RESERVED10'] = self.RESERVED10
        self.IFPKDOFF_AFTER_FRAME_EN = RM_Field_SEQ_MISC_IFPKDOFF_AFTER_FRAME_EN(self)
        self.zz_fdict['IFPKDOFF_AFTER_FRAME_EN'] = self.IFPKDOFF_AFTER_FRAME_EN
        self.RFPKDOFF_AFTER_FRAME_EN = RM_Field_SEQ_MISC_RFPKDOFF_AFTER_FRAME_EN(self)
        self.zz_fdict['RFPKDOFF_AFTER_FRAME_EN'] = self.RFPKDOFF_AFTER_FRAME_EN
        self.RESERVED2 = RM_Field_SEQ_MISC_RESERVED2(self)
        self.zz_fdict['RESERVED2'] = self.RESERVED2
        self.RESERVED1 = RM_Field_SEQ_MISC_RESERVED1(self)
        self.zz_fdict['RESERVED1'] = self.RESERVED1
        self.RESERVED0 = RM_Field_SEQ_MISC_RESERVED0(self)
        self.zz_fdict['RESERVED0'] = self.RESERVED0
        self.__dict__['zz_frozen'] = True


class RM_Register_SEQ_PHYINFO(Base_RM_Register):
    def __init__(self, rmio, label):
        self.__dict__['zz_frozen'] = False
        super(RM_Register_SEQ_PHYINFO, self).__init__(rmio, label,
            0xa0000000, 0x01C,
            'PHYINFO', 'SEQ.PHYINFO', 'read-write',
            u"",
            0x00000000, 0x00000000,
            0x00001000, 0x00002000,
            0x00003000)

        self.ADDRESS = RM_Field_SEQ_PHYINFO_ADDRESS(self)
        self.zz_fdict['ADDRESS'] = self.ADDRESS
        self.__dict__['zz_frozen'] = True


