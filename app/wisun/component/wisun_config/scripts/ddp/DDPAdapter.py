#!/usr/bin/env python3
# vim: set sw=2 expandtab:

import dep
import json
import base64
import sys
import argparse
import logging
from ddp.commander import *
import SigningServer
import wisun.common
import wisun.command
import wisun.response
import ddp.command
import ddp.response
from ddp.rtt import SerialWire
import os

################################################## HELPER FUNCTIONS ####################################################################################################

def GetMacAddress(sw):
  logger.info("Retrieving device serial number")
  sn = sw.get_mac_address()
  logger.info("Device serial number: %s", sn)
  return sn

def OpenSWConnection(soc_,jlink_ser,jlink_host):
  try:
    soc = wisun.common.socs[soc_]
  except KeyError:
    logger.info(f'{parser.prog}: error: {args.soc} is not a supported SoC type', file=sys.stderr)
    exit(1)

  logger.info("Opening SerialWire connection to the device")
  jlink_xml = os.path.join(os.path.dirname(__file__), "jlink/JLinkDevices.xml")
  sw = SerialWire(soc['device'], jlink_ser,jlink_host, jlink_xml)
  sw.connect()
  sw.reset_and_halt()
  logger.info("Connection opened")
  return (sw, soc)

def CloseSWConnection(sw):
  sw.rtt_stop()
  sw.reset()
  sw.close()

######################################################################################################################################################

################################################## CALLABLE APIS #####################################################################

def _initNVM(args):
  try:
    sw, soc = OpenSWConnection(args.soc,args.jlink_ser,args.jlink_host)
    FlashProvApp(sw,soc,args.prov_img)
    InitNVM(sw,soc)
    CloseSWConnection(sw)
  except Exception as e:
    return False, str(e)
  return True, None

def _genKey(args):
  try:
    sw, soc = OpenSWConnection(args.soc,args.jlink_ser,args.jlink_host)
    FlashProvApp(sw,soc,args.prov_img)
    key = GenerateDeviceKey(sw)
    CloseSWConnection(sw)
  except Exception as e:
    return False, str(e)
  return True , key

def _genCSR(args):
  try:
    sw, soc = OpenSWConnection(args.soc,args.jlink_ser,args.jlink_host)
    FlashProvApp(sw,soc,args.prov_img)
    InitNVM(sw,soc)
    csr = GenWisunCSR(sw)
    CloseSWConnection(sw)
  except Exception as e:
    return False, str(e)
  return True, csr

def _genWisunCerts(args):
  try:
    sw, soc = OpenSWConnection(args.soc,args.jlink_ser,args.jlink_host)
    sn = GetMacAddress(sw)
    newcsr = base64.b64decode(args.data)
    logger.info(newcsr)
    device, batch, root = GenWisunDeviceCerts(sn, newcsr, args.config)
  except Exception as e:
    return False, str(e)
  return True, [device, batch, root]

def _writeDeviceCerts(args):
  try:
    sw, soc = OpenSWConnection(args.soc,args.jlink_ser,args.jlink_host)
    FlashProvApp(sw,soc,args.prov_img)
    InitNVM(sw,soc)
    WriteWisunCertsToNVM(sw, base64.b64decode(args.data))
  except Exception as e:
    return False, str(e)
  return True, None

def _writeBatchCerts(args):
  try:
    sw, soc = OpenSWConnection(args.soc,args.jlink_ser,args.jlink_host)
    FlashProvApp(sw,soc,args.prov_img)
    InitNVM(sw,soc)
    WriteBarchToNVM(sw, base64.b64decode(args.data))
  except Exception as e:
    return False, str(e)
  return True, None

def _writeRootCerts(args):
  try:
    sw, soc = OpenSWConnection(args.soc,args.jlink_ser,args.jlink_host)
    FlashProvApp(sw,soc,args.data)
    InitNVM(sw,soc)
    WriteRootNVM(sw, base64.b64decode(args.data))
  except Exception as e:
    return False, str(e)
  return True, None
  
#########################################################################################################################################


################################################## CORE PROCEDURES #####################################################################

def FlashProvApp(sw, soc, prov_img):
  # Inject and run provisioning application
  with open(prov_img, 'rb') as f:
    provisioning_app = f.read()
  logger.info("Injecting provisioning application")
  ram_addr = soc['ramstartaddress']
  sw.run_application(ram_addr, provisioning_app)
  sw.rtt_start()
  logger.info("Provisioning application running")

def InitNVM(sw,soc):
  # Initialize NVM
  logger.info("Initializing NVM")
  tx = ddp.command.InitializeNvm(soc['nvm3inststartaddress'], soc['nvm3instsize'])
  sw.rtt_send(tx)
  rx = sw.rtt_receive()
  resp = ddp.response.InitializeNvm(rx)
  if resp.status != 0: 
    logger.warning(f"Failure during NVM initialization ({resp.status})")
  logger.info("NVM initialized")


def GenerateDeviceKey(sw:SerialWire):
  logger.info("Generating Wi-SUN key pair on the device")
  tx = wisun.command.GenerateKeyPair(0x100)
  sw.rtt_send(tx)
  rx = sw.rtt_receive()
  resp = wisun.response.GenerateKeyPair(rx)
  assert resp.status in (0,19), f"Failure during Wi-SUN key pair generation ({resp.status})"
  if resp.status == 19:
    logger.warning("Wi-SUN key pair already exists")
  else:
    logger.info("Wi-SUN key pair generated")
    logger.warning("Wi-SUN key pair generated")
  return resp.key

def GenWisunCSR(sw):
  # Generate Wi-SUN CSR
  logger.info("Generating Wi-SUN CSR on the device")
  tx = wisun.command.GenerateCsr(0x100)
  sw.rtt_send(tx)
  rx = sw.rtt_receive()
  resp = wisun.response.GenerateCsr(rx)
  assert resp.status == 0, f"Failure during Wi-SUN CSR generation ({resp.status})"
  logger.info("Wi-SUN CSR generated")
  return base64.b64encode(resp.csr).decode("utf-8")

def GenWisunDeviceCerts(sn: str, csr: bytes, config: str):
  # Generate Wi-SUN device certificate
  logger.info("Generating Wi-SUN device certificate")
  device, batch, root = SigningServer.GetCerts(csr, sn, config)
  logger.info("Wi-SUN device certificate generated")
  return base64.b64encode(device).decode('utf-8'), base64.b64encode(batch).decode('utf-8'), base64.b64encode(root).decode('utf-8')

def WriteWisunCertsToNVM(sw: SerialWire, device:bytes):
    # Write Wi-SUN device certificate into NVM
    logger.info("Saving Wi-SUN device certificate into NVM")
    tx = ddp.command.WriteNvm(0x100, device)
    sw.rtt_send(tx)
    rx = sw.rtt_receive()
    resp = ddp.response.WriteNvm(rx)
    assert resp.status == 0, f"Failure saving Wi-SUN device certificate into NVM ({resp.status})"
    logger.info("Wi-SUN device certificate saved")

def WriteBarchToNVM(sw: SerialWire, batch:bytes):
    # Write Wi-SUN batch certificate into NVM
    logger.info("Saving Wi-SUN batch certificate into NVM")
    tx = ddp.command.WriteNvm(0x101, batch)
    sw.rtt_send(tx)
    rx = sw.rtt_receive()
    resp = ddp.response.WriteNvm(rx)
    assert resp.status == 0, f"Failure saving Wi-SUN batch certificate into NVM ({resp.status})"
    logger.info("Wi-SUN batch certificate saved")

def WriteRootNVM(sw: SerialWire, root:bytes):
    # Write Wi-SUN root certificate into NVM
    logger.info("Saving Wi-SUN root certificate into NVM")
    tx = ddp.command.WriteNvm(0x102, root)
    sw.rtt_send(tx)
    rx = sw.rtt_receive()
    resp = ddp.response.WriteNvm(rx)
    assert resp.status == 0, f"Failure saving Wi-SUN root certificate into NVM ({resp.status})"
    logger.info("Wi-SUN root certificate saved")

################################################################################################################################################################

if __name__ == '__main__':
  logger = logging.getLogger('provision')
  logger.setLevel(logging.DEBUG)
  ch = logging.StreamHandler()
  ch.setFormatter(logging.Formatter('%(asctime)s %(levelname)s %(message)s'))
  logger.addHandler(ch)


  parser = argparse.ArgumentParser(description='Script for performing Wi-SUN provisioning.')
  parser.add_argument('--soc', action='store', required=True, help='SoC type')
  parser.add_argument('--jlink_host', action='store', default=None, help='Host name or IP address of J-Link adapter')
  parser.add_argument('--jlink_ser', action='store', default=None, help='Serial number of J-Link adapter')
  subparsers = parser.add_subparsers(help='sub-command help')

  initNvm = subparsers.add_parser('init_nvm',  help='TODO')
  initNvm.add_argument('--prov_img', action='store', required=True, help='Input file for provisiong application')
  initNvm.set_defaults(func=_initNVM)

  genDevKey = subparsers.add_parser('gen_device_key', help='TODO')
  genDevKey.add_argument('--prov_img', action='store', required=True, help='Input file for provisiong application')
  genDevKey.set_defaults(func=_genKey)

  genCSRKey = subparsers.add_parser('gen_csr_key',  help='TODO')
  genCSRKey.add_argument('--prov_img', action='store', required=True, help='Input file for provisiong application')
  genCSRKey.set_defaults(func=_genCSR)

  genWisunCert = subparsers.add_parser('gen_wisun_certs', help='TODO')
  genWisunCert.add_argument('--data', action='store', required=True, help='TODO')
  genWisunCert.add_argument('--config', action='store', default='openssl.conf', help='OpenSSL configuration file (default: openssl.conf)')
  genWisunCert.set_defaults(func=_genWisunCerts)

  writeDeviceCerts = subparsers.add_parser('write_wisun_certs', help='TODO')
  writeDeviceCerts.add_argument('--data', action='store', required=True, help='TODO')
  writeDeviceCerts.add_argument('--prov_img', action='store', required=True, help='Input file for provisiong application')
  writeDeviceCerts.set_defaults(func=_writeDeviceCerts)

  writeBatch = subparsers.add_parser('write_batch_certs', help='TODO')
  writeBatch.add_argument('--data', action='store', required=True, help='TODO')
  writeBatch.add_argument('--prov_img', action='store', required=True, help='Input file for provisiong application')
  writeBatch.set_defaults(func=_writeBatchCerts)

  writeRoot = subparsers.add_parser('write_root_certs', help='TODO')
  writeRoot.add_argument('--data', action='store', required=True, help='TODO')
  writeRoot.add_argument('--prov_img', action='store', required=True, help='Input file for provisiong application')
  writeRoot.set_defaults(func=_writeRootCerts)

  args = parser.parse_args()
  result, data = args.func(args)
  
  if data:
      if type(data) is list:
        print(json.dumps({"result": result, "data": {"wisun": data[0], "batch": data[1], "root": data[2]}}))
      else:
        print(json.dumps({"result":result, "data": data}))
  else:
      print(json.dumps({"result":result}))
