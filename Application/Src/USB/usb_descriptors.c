/*
 * usb_descriptors.c
 *
 *  Created on: Mar 30, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#include "tusb.h"
#include "tusb_config.h"

//--------------------------------------------------------------------
// Device Descriptor
//--------------------------------------------------------------------
tusb_desc_device_t const desc_device = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0200,           // USB 2.0

  // Use class/subclass/protocol at device level for CDC
  .bDeviceClass       = TUSB_CLASS_MISC,
  .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol    = MISC_PROTOCOL_IAD,

  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = 0xCafe,           // Change to your VID
  .idProduct          = 0x4001,           // Change to your PID
  .bcdDevice          = 0x0100,

  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,

  .bNumConfigurations = 0x01
};

// Called by TinyUSB to get device descriptor
uint8_t const* tud_descriptor_device_cb(void) {
  return (uint8_t const*)&desc_device;
}

//--------------------------------------------------------------------
// Configuration Descriptor
//--------------------------------------------------------------------

#define TUD_VENDOR_BULK_DESC_LEN  (9 + 7 + 7)  // 23 bytes

#define TUD_VENDOR_BULK_DESCRIPTOR(_itfnum, _stridx, _epout, _epin, _epsize)  \
  /* Interface descriptor */                                                  \
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 2,                                      \
  TUSB_CLASS_VENDOR_SPECIFIC, 0x00, 0x00, _stridx,                            \
  /* Endpoint OUT */                                                          \
  7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_BULK, U16_TO_U8S_LE(_epsize), 0,   \
  /* Endpoint IN */                                                           \
  7, TUSB_DESC_ENDPOINT, _epin,  TUSB_XFER_BULK, U16_TO_U8S_LE(_epsize), 0

// CDC uses 2 interfaces: control + data
// Endpoints: 1 notify (interrupt IN), 1 data OUT, 1 data IN
#define CONFIG_TOTAL_LEN    ( TUD_CONFIG_DESC_LEN      \
                            + TUD_CDC_DESC_LEN         \
                            + TUD_VENDOR_BULK_DESC_LEN \
                            + TUD_VENDOR_BULK_DESC_LEN )

#define EPNUM_CDC_NOTIFY      0x81    // EP1 IN  (interrupt, for line state)
#define EPNUM_CDC_OUT         0x02    // EP2 OUT (bulk, host->device)
#define EPNUM_CDC_IN          0x82    // EP2 IN  (bulk, device->host)

#define EPNUM_HIL_STREAM_OUT  0x03    // EP3 OUT (bulk, host->DAC)
#define EPNUM_HIL_STREAM_IN   0x83    // EP3 IN  (bulk, ADC->host)

#define EPNUM_HIL_CONTROL_OUT 0x04    // EP4 OUT (bulk, commands for HIL)
#define EPNUM_HIL_CONTROL_IN  0x84    // EP4 IN  (bulk, HIL command responses)

uint8_t const desc_configuration[] = {
  // Config descriptor
  TUD_CONFIG_DESCRIPTOR(
    1,                  // config number
    ITF_NUM_TOTAL,      // interface count
    0,                  // string index
    CONFIG_TOTAL_LEN,   // total length
    0x00,               // attributes
    250                 // power in mA
  ),

  // CDC interface 0+1, EP1 notify, EP2 data
  TUD_CDC_DESCRIPTOR(
    ITF_NUM_CDC,        // interface number
    4,                  // string index
    EPNUM_CDC_NOTIFY,   // notify endpoint
    8,                  // notify EP size
    EPNUM_CDC_OUT,      // data out endpoint
    EPNUM_CDC_IN,       // data in endpoint
    512                 // data EP size
  ),

  // HIL stream: interface 2, EP3 OUT + EP3 IN
  TUD_VENDOR_BULK_DESCRIPTOR(
    ITF_NUM_HIL_STREAM,
    5,                    // string index
    EPNUM_HIL_STREAM_OUT,
    EPNUM_HIL_STREAM_IN,
    512                   // HS bulk max packet
  ),

  // HIL control: interface 3, EP4 OUT + EP4 IN
  TUD_VENDOR_BULK_DESCRIPTOR(
    ITF_NUM_HIL_CONTROL,
    6,                    // string index
    EPNUM_HIL_CONTROL_OUT,
    EPNUM_HIL_CONTROL_IN,
    512                   // HS bulk max packet
  ),
};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return desc_configuration;
}

//--------------------------------------------------------------------
// String Descriptors
//--------------------------------------------------------------------
char const* string_desc_arr[] = {
  (const char[]){ 0x09, 0x04 },    // 0: Language (English)
  "OpenAquatix",                   // 1: Manufacturer
  "OpenAquatix Modem",             // 2: Product
  "OA-2-2",                        // 3: Serial Number
  "HMI",                           // 4: CDC Interface
  "HIL Stream",                    // 5: HIL Stream Interface
  "HIL Control",                   // 6: HIL Stream Control
};

static uint16_t desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  uint8_t chr_count;

  if (index == 0) {
    memcpy(&desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
      return NULL;
    }
    const char* str = string_desc_arr[index];
    chr_count = (uint8_t)strlen(str);
    if (chr_count > 31) chr_count = 31;

    // Convert ASCII to UTF-16
    for (uint8_t i = 0; i < chr_count; i++) {
      desc_str[1 + i] = str[i];
    }
  }

  // Header: length + type
  desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return desc_str;
}
