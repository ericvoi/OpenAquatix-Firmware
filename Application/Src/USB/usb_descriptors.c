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
enum {
  ITF_NUM_CDC = 0,
  ITF_NUM_CDC_DATA,
  ITF_NUM_TOTAL
};

// CDC uses 2 interfaces: control + data
// Endpoints: 1 notify (interrupt IN), 1 data OUT, 1 data IN
#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

#define EPNUM_CDC_NOTIFY    0x81    // EP1 IN  (interrupt, for line state)
#define EPNUM_CDC_OUT       0x02    // EP2 OUT (bulk, host->device)
#define EPNUM_CDC_IN        0x82    // EP2 IN  (bulk, device->host)

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

  // CDC descriptor (expands to control + data interface)
  TUD_CDC_DESCRIPTOR(
    ITF_NUM_CDC,        // interface number
    4,                  // string index
    EPNUM_CDC_NOTIFY,   // notify endpoint
    8,                  // notify EP size
    EPNUM_CDC_OUT,      // data out endpoint
    EPNUM_CDC_IN,       // data in endpoint
    512                 // data EP size
  )
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
  "123456",                        // 3: Serial Number
  "HMI",                           // 4: CDC Interface
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
