#!/usr/bin/python

import os
import re
import argparse
import yaml

from rail_scripts.rail_model import RAILModel
from rail_scripts.rail_model_types import *
from rail_scripts import rail_model_types

from host_py_rm_studio_internal import RM_Factory
from pyradioconfig.calculator_model_framework.CalcManager import CalcManager
from pycalcmodel import *

EXCLUDE_BLOCK_LIST = [
]

WRITE_ONLY_REGISTERS = [
]

class RAILAdapter(object):

  _MAX_NUM_REG_BASES = 8
  _MAX_NUM_REG_BASES_SOL = 16

  _REG_BASES = (
    0x40080000,
    0x21000000,
  )
  _REG_BASES_EFR32XG2x = (
    0xA8000000,
    0xA8010000,
    0xA8020000,
    0xB0000000,
  )
  _REG_BASES_EFR32XG25 = (
    0x400C0000,
    0xA8000000,
    0xA8010000,
    0xA8020000,
    0xA8030000,
    0xA8040000,
    0xB0000000,
    0xB5000000,
    0xA6040000	
  )
  _REG_BASES_SIXG301 = (
    0xA0200000,
    0xA0210000,
    0xA0220000,
    0xB0000000,
  )

  import os
  current_dir =  os.path.dirname(os.path.abspath(__file__))

  _RADIO_ACTION_WRITE_TMPL = """  0x{0:08X}UL, 0x{1:08X}UL,\n"""

  # ------------------------------------------------------------------------
  def __new__(cls, **kwargs):
    if cls is not RAILAdapter:
      return super(RAILAdapter, cls).__new__(cls)
    if "mphyConfig" in kwargs:
      import rail_scripts.rail_adapter_multi_phy
      return rail_scripts.rail_adapter_multi_phy.RAILAdapter_MultiPhy(**kwargs)
    else:
      raise NotImplementedError("Single phy adapter no longer supported!!!")

  @property
  def modelPopulated(self):
    return self._railModelPopulated

  def pte_script_flag(self):
    return self.pte_script

  def generateRailModelContext(self):
    modelContext = {}
    for element in self.railModel._elements:
      if type(getattr(self.railModel, element)).__name__ in rail_model_types.supportedClassType:
        getattr(self.railModel, element).resolve(modelContext)
      else:
        modelContext[element] = getattr(self.railModel, element)
    return modelContext

  # -------- Internal ---------------------------------------------------------
  # // TODO: Rename if we ever support anything other than writes
  def _encodeWriteAddress(self, reg_address, write_length=1, radio_action=0):

    if self.pte_script is False:
      reg_base = self._regBases[int(reg_address) & 0xFFFF0000]
      reg_offset = int(reg_address) & 0x0000FFFF

      encodedAddress = (radio_action << 28) \
                       | (reg_base << 24) \
                       | (write_length << 16) \
                       | reg_offset
    else:
      # Don't encrypt register address for PTE output
      # only update address value based on opearation(Write/SET/CLR)
      encodedAddress =  reg_address + radio_action 

    return encodedAddress

  def _getRegAddress(self, block, register):
    # Use the RM_Device object to get the absolute register address
    baseAddrString = "self.rm.{0}.{1}.baseAddress".format(block, register)
    regOffsetString = "self.rm.{0}.{1}.addressOffset".format(block, register)
    regAddr = eval(baseAddrString) + eval(regOffsetString)
    return regAddr

  def _regOutput(self, block, register, value, name=None):
    # Use the RM_Device object to get register address value pairs
    regAddr = self._getRegAddress(block, register)
    if (name is None):
      return (regAddr, value)
    else:
      return (regAddr, value, name)

  def _getRegNameFromFieldName(self, fieldname):
    (block, reg, field) = fieldname.split('.')
    return block + '.' + reg

def getParserArgs():
  """
  Build argparse parser and return arguments from sys.argv.
  """
  parser = argparse.ArgumentParser(description="Parse radio configurator output into header file.")
  parser.add_argument('-o', '--output_dir', type=str, default=None, help="The output dir.")
  parser.add_argument('-p', '--phy_name', type=str, default="PHY_RAIL", help="Build configuration for a specific PHY")
  args = parser.parse_args()
  return args

def main():

  args = getParserArgs()
  basePath = os.path.abspath(os.getcwd())

  # Check output information
  outDir = os.path.abspath(args.output_dir)

  radio_configurator = CalcManager("dumbo", "A0")

  # phy_name = args.phy_name
  phy_name = "PHY_Internal_915M_OOK_100kbps"
  adapter='rail_api_2.x'

  print("phy_name is: {}".format(phy_name))
  print("adapter is: {}".format(adapter))

  if phy_name not in radio_configurator.getPhyNames():
    print ("Phy Name {} is not a valid phy. Using PHY_RAIL.".format(phy_name))
    phy_name = "PHY_RAIL"

  instanceDict = {}
  instanceDict["PHY_generated"] = radio_configurator.calculate_phy(phy_name)
  railAdapter = RAILAdapter(dictionary=instanceDict, adapter_name=adapter)
  railAdapter.populateModel()

  outPath = os.path.join(outDir, 'rail_model_out.yml')
  outputDir = os.path.dirname(outPath)

  try:
    os.makedirs(outputDir)
  except OSError:
    if not os.path.isdir(outputDir):
      raise

  railModelContext = railAdapter.generateRailModelContext()
  rail_model_out = yaml.dump(railModelContext)

  with open(outPath, 'w') as fc:
    print ("Creating '{}'...".format(outPath))
    fc.write(rail_model_out)
    fc.close()

  # Script Finished
  return 0

# Call main if necessary
if __name__ == '__main__':
  main()
