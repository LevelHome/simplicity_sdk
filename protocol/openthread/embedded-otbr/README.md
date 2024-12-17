---
# 917 EMBEDDED OTBR

This project is a proof-of-concept of a low cost OTBR with a MCU as a host instead of using a high-end application processor. It builds an OpenThread Border Router (OTBR) on SoC SiWx917 with MG24 RCP.
---

## Dependencies

### Note: See [requirements.txt](requirements.txt) for package dependencies

### Install OpenThread, Simplicity SDK, and Wiseconnect SDK:

```bash
git clone git@github.com:openthread/openthread.git <openthread-dir>
checkout @ 67717618b

git clone git@github.com:SiliconLabs/simplicity_sdk.git <sisdk-dir>
checkout @ 36e12f019 (tag: v2024.6.2)

git clone git@github.com:SiliconLabs/wiseconnect.git <wiseconnect-dir>
checkout @ b5d6422f3 (tag: v3.3.2)

Make sure you have the following environment variables:
export SISDK_PATH=<sisdk-dir>
export PATH=$PATH:<path to ARM gcc-12.2.1/bin containing arm-none-eabi-gcc>
export COMMANDER=<path to commander binary>

Then, under embedded-otbr directory:
ln -s <openthread-dir> openthread
ln -s <sisdk-dir> simplicity_sdk
ln -s <wiseconnect-dir> wiseconnect
```

### The following are patches required for functional LWIP code (not yet upstream):

### Patch OpenThread

```bash
cd <openthread-dir>
git apply <embedded-otbr>/patch/openthread.patch
```

### Patch Simplicity SDK

```bash
cd <sisdk-dir>
git apply <embedded-otbr>/patch/sisdk.patch
```

### Patch WiseConnect

```bash
cd <wiseconnect-dir>
git apply <embedded-otbr>/patch/wiseconnect.patch
```

---

## Build RCP

Using Simplicity SDK @`<sisdk-dir>`, build radio RCP board for eg brd4187c with RCP UART only

You can also use prebuilt 4187c image @`<embedded-otbr>/artifacts/prebuilt/rcp/`

---

## Build MCU Host

### Note: Be sure to have a recent version of slc installed when performing this buil

Currently, MCU Host 917 support is included only for the BRD4338A and BRD4342A boards.

Option 1: Simple OpenThread CLI FTD application on MCU Host

```bash
./script/build brd4338a --mcu-host
```

Option 2: OpenThread Border Router application on MCU Host

```bash
./script/build brd4338a --otbr-mcu-host
```

To re-generate SLC components, use pristine build (example):

```bash
./script/build -p brd4338a --mcu-host
```

---

## Hardware Setup

### Prerequisites

- SiWx917 SoC BRD4338A
- WSTK BRD4002A or Adapter Board BRD8045A (optional, recommended)
- EFR32xG24 BRD4187c

### Option 1. Use BRD4002A only

If using BRD4002A only, you will use the following combination:

```bash
Host: BRD4338A + BRD4002A

RCP: BRD4187C + BRD4002A

| BRD4002A (Host) | BRD4002A (RCP) |
| --------------- | -------------- |
| P33             | EXP 13         |
| P35             | EXP 15         |
```

> ![Figure: BRD4002A-only setup](artifacts/images/brd4002a-only.png)

### Option 2. Setup with BRD8045A

If using BRD8045A adapter:

> ![Figure: Hardware Setup](artifacts/images/brd8045a-brd4002a.png)

#### 2a. Enable UART Log on BRD8045A

If using BRD8045A adapter, move two resistors on BRD8045A board to enable SoC UART Log for SiWx917

> ![Figure: Enable UART Log on BRD8045A](artifacts/images/brd8045a-enable-log.png)

---

## Demo

### OpenThread CLI FTD

1. Build MCU Host image or use prebuilt binary **ot-mcu-cli-ftd.rps** in folder **artifacts/prebuilt/host**

2. Flash RCP image using prebuilt binary **ot-rcp-brd4187c.s37** in folder **artifacts/prebuilt/rcp**

3. Flash Host image

4. Use Putty or other Serial software to display log and use CLI.

5. Form a Thread network:

   Generate, view, and commit a new Active Operational Dataset:

   ```bash
   > dataset init new
   Done
   > dataset
   Active Timestamp: 1
   Channel: 13
   Channel Mask: 0x07fff800
   Ext PAN ID: d63e8e3e495ebbc3
   Mesh Local Prefix: fd3d:b50b:f96d:722d::/64
   Network Key: dfd34f0f05cad978ec4e32b0413038ff
   Network Name: OpenThread-8f28
   PAN ID: 0x8f28
   PSKc: c23a76e98f1a6483639b1ac1271e2e27
   Security Policy: 0, onrc
   Done
   > dataset commit active
   Done
   ```

   Bring up the IPv6 interface:

   ```bash
   > ifconfig up
   Done
   ```

   Start Thread protocol operation:

   ```bash
   > thread start
   Done
   ```

   Wait a few seconds and verify that the device has become a Thread Leader:

   ```bash
   > state
   leader
   Done
   ```

   View IPv6 addresses assigned to Node 1's Thread interface:

   ```bash
   > ipaddr
   fd3d:b50b:f96d:722d:0:ff:fe00:fc00
   fd3d:b50b:f96d:722d:0:ff:fe00:c00
   fd3d:b50b:f96d:722d:7a73:bff6:9093:9117
   fe80:0:0:0:6c41:9001:f3d6:4148
   Done
   ```

6. Start a child node (ot-cli-ftd or ot-cli-mtd)

   Configure the Thread Network Key from Node 1's Active Operational Dataset:

   ```bash
   > dataset networkkey dfd34f0f05cad978ec4e32b0413038ff
   Done
   > dataset commit active
   Done
   ```

   Bring up the IPv6 interface:

   ```bash
   > ifconfig up
   Done
   ```

   Start Thread protocol operation:

   ```bash
   > thread start
   Done
   ```

   Wait a few seconds and verify that the device has become a Thread Child or Router:

   ```bash
   > state
   child
   Done
   ```

7. Ping Node 1 from Node 2

   ```bash
   > ping fd3d:b50b:f96d:722d:7a73:bff6:9093:9117
   16 bytes from fd3d:b50b:f96d:722d:558:f56b:d688:799: icmp_seq=1 hlim=64 time=24ms
   ```

### OpenThread Border Router

1. Build MCU Host image or use prebuilt binary **otbr-mcu-cli-ftd.rps** in folder **artifacts/prebuilt/host**

2. Flash RCP image using prebuilt binary **ot-rcp-brd4187c.s37** in folder **artifacts/prebuilt/rcp**

3. Flash Host image

4. Use Putty or other Serial software to display log and use CLI.

5. Create an Wi-Fi Access Point for OTBR to connect. Default Access Point SSID is **SILABS_OTBR** and Password is **123456789**. You can change SSID and Password for your build in `src/mcu/mcu_netif.cpp`.

6. Wait for device to connect. If it connects to AP successfully, log display should look like below:

> ![Figure: Connect WiFi successfully](artifacts/images/wifi-connect-log.png)

7. mDNS Discovery. Use avahi-browse to scan **\_meshcop.\_udp** service.

> ![Figure: mDNS Discovery](artifacts/images/mdns-discovery.png)

---
