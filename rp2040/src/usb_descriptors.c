// SPDX-License-Identifier: MIT
#include <string.h>

#include "pico/unique_id.h"
#include "tusb.h"

#define USB_VID 0xCafe
#define USB_PID 0x4037
#define USB_BCD 0x0200

static tusb_desc_device_t const device_descriptor = {
  .bLength = sizeof(tusb_desc_device_t),
  .bDescriptorType = TUSB_DESC_DEVICE,
  .bcdUSB = USB_BCD,
  .bDeviceClass = TUSB_CLASS_UNSPECIFIED,
  .bDeviceSubClass = 0,
  .bDeviceProtocol = 0,
  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor = USB_VID,
  .idProduct = USB_PID,
  .bcdDevice = 0x0101,
  .iManufacturer = 1,
  .iProduct = 2,
  .iSerialNumber = 3,
  .bNumConfigurations = 1,
};

uint8_t const *tud_descriptor_device_cb(void) {
  return (uint8_t const *)&device_descriptor;
}

enum { ITF_NUM_MTP, ITF_NUM_TOTAL };
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MTP_DESC_LEN)
#define EPNUM_MTP_EVENT 0x81
#define EPNUM_MTP_OUT 0x02
#define EPNUM_MTP_IN 0x82

static uint8_t const configuration_descriptor[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0, 100),
  TUD_MTP_DESCRIPTOR(ITF_NUM_MTP, 4, EPNUM_MTP_EVENT, 64, 1,
                     EPNUM_MTP_OUT, EPNUM_MTP_IN, 64),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return configuration_descriptor;
}

enum { STR_LANGID, STR_MANUFACTURER, STR_PRODUCT, STR_SERIAL, STR_MTP };
static char const *const strings[] = {
  (const char[]){0x09, 0x04},
  "p2r3 / RP2040 port",
  "USB of Babel",
  NULL,
  "USB of Babel MTP",
};
static uint16_t string_descriptor[33];

static size_t serial_utf16(uint16_t *out, size_t capacity) {
  pico_unique_board_id_t id;
  pico_get_unique_board_id(&id);
  static char const hex[] = "0123456789ABCDEF";
  size_t count = PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2u;
  if (count > capacity) count = capacity;
  for (size_t i = 0; i < count; ++i) {
    uint8_t byte = id.id[i / 2u];
    out[i] = (uint16_t)hex[(i & 1u) ? (byte & 0x0fu) : (byte >> 4)];
  }
  return count;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  size_t count;
  if (index == STR_LANGID) {
    memcpy(&string_descriptor[1], strings[0], 2);
    count = 1;
  } else if (index == STR_SERIAL) {
    count = serial_utf16(&string_descriptor[1], 32);
  } else {
    if (index >= TU_ARRAY_SIZE(strings) || strings[index] == NULL) return NULL;
    count = strlen(strings[index]);
    if (count > 32) count = 32;
    for (size_t i = 0; i < count; ++i) string_descriptor[1 + i] = strings[index][i];
  }
  string_descriptor[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2u * count + 2u));
  return string_descriptor;
}
