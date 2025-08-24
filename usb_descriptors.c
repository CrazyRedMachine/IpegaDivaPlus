/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "usb_descriptors.h"

#include "tusb.h"

#define VID 0x0F0D
#define PID 0x00FB // HORI DIVA

/* A combination of interfaces must have a unique product id, since PC will save
 * device driver after the first plug. Same VID/PID with different interface e.g
 * MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]         HID | MSC | CDC          [LSB]
 */

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const* string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: is supported language is English (0x0409)
    "CrazyRedMachine",          // 1: Manufacturer
    "Ipega Diva Deluxe",        // 2: Product
    "MNDVA",                    // 3: Serial
    "Ipega Diva Deluxe (KB)",   // 4: Product (KB mode)
    "MNDVAKB",                  // 5: Serial (KB mode)
};

static uint16_t _desc_str[64];
uint8_t string_desc_size = sizeof(string_desc_arr)/sizeof(string_desc_arr[0]);

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long
// enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;

    uint8_t chr_count;
    uint8_t string_arr_size = string_desc_size;
    char const** string_arr = string_desc_arr;

    if (index == 0) {
    memcpy(&_desc_str[1], string_arr[0], 2);
    chr_count = 1;
    } else {
        // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

        if (!(index < string_arr_size))
            return NULL;

        const char* str = string_arr[index];

        // Cap at max char
        chr_count = strlen(str);

        if (chr_count > 63) chr_count = 63;

        // Convert ASCII string into UTF-16
        for (uint8_t i = 0; i < chr_count; i++) {
        _desc_str[1 + i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}

uint8_t const desc_hid_report_joy[] = {
  HID_USAGE_PAGE(1), /* Generic Desktop */
  HID_USAGE(5), /* Joystick */
  HID_COLLECTION(1), /* Application */
    // Buttons (2 bytes)
    HID_LOGICAL_MIN(0),
    HID_LOGICAL_MAX(1),
    HID_PHYSICAL_MIN(0),
    HID_PHYSICAL_MAX(1),
    // The Switch will allow us to expand the original HORI descriptors to a full 16 buttons.
    // The Switch will make use of 14 of those buttons.
    HID_REPORT_SIZE(1),
    HID_REPORT_COUNT(16),
    HID_USAGE_PAGE(9),
    HID_USAGE_MIN(1),
    HID_USAGE_MAX(16),
    HID_INPUT(2),
    // HAT Switch (1 nibble)
    HID_USAGE_PAGE(1),
    HID_LOGICAL_MAX(7),
    HID_PHYSICAL_MAX_N(315,2),
    HID_REPORT_SIZE(4),
    HID_REPORT_COUNT(1),
    HID_UNIT(20),
    HID_USAGE(57),
    HID_INPUT(66),
    // There's an additional nibble here that's utilized as part of the Switch Pro Controller.
    // I believe this -might- be separate U/D/L/R bits on the Switch Pro Controller, as they're utilized as four button descriptors on the Switch Pro Controller.
    HID_UNIT(0),
    HID_REPORT_COUNT(1),
    HID_INPUT(1),
    // Joystick (4 bytes)
    HID_LOGICAL_MAX_N(255,2),
    HID_PHYSICAL_MAX_N(255,2),
    HID_USAGE(48),
    HID_USAGE(49),
    HID_USAGE(50),
    HID_USAGE(53),
    HID_REPORT_SIZE(8),
    HID_REPORT_COUNT(4),
    HID_INPUT(2),
    // ??? Vendor Specific (1 byte)
    // This byte requires additional investigation.
    HID_USAGE_PAGE_N(65280,2),
    HID_USAGE(32),
    HID_REPORT_COUNT(1),
    HID_INPUT(2),
    // Output (8 bytes)
    // Original observation of this suggests it to be a mirror of the inputs that we sent.
    // The Switch requires us to have these descriptors available.
    HID_USAGE_N(9761,2),
    HID_REPORT_COUNT(8),
    HID_OUTPUT(2),
  HID_COLLECTION_END,
};

uint8_t const desc_hid_report_kb[] = {
  HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP), 
  HID_USAGE(HID_USAGE_PAGE_KEYBOARD),
  HID_COLLECTION(HID_COLLECTION_APPLICATION),
    HID_REPORT_SIZE(1), 
    HID_REPORT_COUNT(8),
    HID_USAGE_PAGE(HID_USAGE_PAGE_KEYBOARD), 
    HID_USAGE_MIN(224),
    HID_USAGE_MAX(231), 
    HID_LOGICAL_MIN(0), 
    HID_LOGICAL_MAX(1),
    HID_INPUT(HID_VARIABLE), 
    HID_REPORT_SIZE(1), 
    HID_REPORT_COUNT(31 * 8),
    HID_LOGICAL_MIN(0), 
    HID_LOGICAL_MAX(1),
    HID_USAGE_PAGE(HID_USAGE_PAGE_KEYBOARD), 
    HID_USAGE_MIN(0),
    HID_USAGE_MAX(31 * 8 - 1), 
    HID_INPUT(HID_VARIABLE), 
  HID_COLLECTION_END,
};

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

#define EPNUM_HID 0x81
enum { ITF_NUM_HID, ITF_NUM_TOTAL };

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

uint8_t desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute,
    // power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),

    // Interface number, string index, protocol, report descriptor len, EP In
    // address, size & polling interval
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 2, HID_ITF_PROTOCOL_NONE,
                       sizeof(desc_hid_report_joy), EPNUM_HID, // tud_descriptor_device_cb will update the placeholder value
                       CFG_TUD_HID_EP_BUFSIZE, 1),
};

uint8_t const desc_configuration_kb[] = {
    // Config number, interface count, string index, total length, attribute,
    // power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),

    // Interface number, string index, protocol, report descriptor len, EP In
    // address, size & polling interval
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 4, HID_ITF_PROTOCOL_NONE,
                       sizeof(desc_hid_report_kb), EPNUM_HID,
                       CFG_TUD_HID_EP_BUFSIZE, 1)};

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = VID,
    .idProduct = PID,
    .bcdDevice = 0x0100,

    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01};

tusb_desc_device_t desc_device_kb = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = 0xCAFE,
    .idProduct = PID,
    .bcdDevice = 0x0100,

    .iManufacturer = 0x01,
    .iProduct = 0x04,
    .iSerialNumber = 0x05,

    .bNumConfigurations = 0x01};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const* tud_descriptor_device_cb(void) {    
    return (uint8_t const*)(g_kb_mode ? &desc_device_kb : &desc_device);
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;  // for multiple configurations    

    return (uint8_t const*)(g_kb_mode ? &desc_configuration_kb : &desc_configuration);
}

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_hid_descriptor_report_cb(uint8_t itf) {
    if (itf == ITF_NUM_HID) 
        return (g_kb_mode ? desc_hid_report_kb : desc_hid_report_joy);

    return NULL;
}
