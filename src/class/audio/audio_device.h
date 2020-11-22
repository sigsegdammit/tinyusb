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
 * This file is part of the TinyUSB stack.
 */

#ifndef _TUSB_AUDIO_DEVICE_H_
#define _TUSB_AUDIO_DEVICE_H_

#include "common/tusb_common.h"
#include "device/usbd.h"

#include "audio.h"

//--------------------------------------------------------------------+
// Class Driver Configuration
//--------------------------------------------------------------------+
#ifndef CFG_TUD_AUDIO_EP_BUFSIZE
  #define CFG_TUD_AUDIO_EP_BUFSIZE     192
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup Audio
 *  @{
 *  \defgroup   Audio Device
 *  @{ */

//--------------------------------------------------------------------+
// Application API (Multiple Ports)
// CFG_TUD_AUDIO > 1
//--------------------------------------------------------------------+
// Return true if an audio channel has been opened
bool     tud_audio_n_open           (uint8_t itf);

// Get the number of bytes available for reading
uint32_t tud_audio_n_available       (uint8_t itf);

// Read received bytes
uint32_t tud_audio_n_read            (uint8_t itf, void* buffer, uint32_t bufsize);

// Clear the interface FIFO
void     tud_audio_n_flush           (uint8_t itf);

// Write data to fifo, return bytes written
uint32_t tud_audio_n_write           (uint8_t itf, void *buffer, uint32_t bufsize);

// Return the number of bytes that can be written
uint32_t tud_cdc_n_write_available   (uint8_t itf);

//--------------------------------------------------------------------+
// Application Callback API (weak is optional)
//--------------------------------------------------------------------+
TU_ATTR_WEAK void tud_audio_channel_open(uint8_t itf);
TU_ATTR_WEAK void tud_audio_channel_close(uint8_t itf);

TU_ATTR_WEAK bool tud_audio_ctrl_get_mute(uint8_t term_id, uint8_t chan_id, uint8_t *val, uint8_t *len );
TU_ATTR_WEAK void tud_audio_ctrl_set_mute(uint8_t term_id, uint8_t chan_id, const uint8_t *val, uint8_t len );

TU_ATTR_WEAK bool tud_audio_ctrl_get_volume(uint8_t term_id, uint8_t chan_id, int16_t *val, uint8_t *len );
TU_ATTR_WEAK void tud_audio_ctrl_set_volume(uint8_t term_id, uint8_t chan_id, const int16_t *val, uint8_t len );

TU_ATTR_WEAK bool tud_audio_ep_get_srate(uint8_t ep, uint32_t *val );
TU_ATTR_WEAK void tud_audio_ep_set_srate(uint8_t ep, const uint32_t val );

//--------------------------------------------------------------------+
// Inline Functions
//--------------------------------------------------------------------+

/** @} */
/** @} */

//--------------------------------------------------------------------+
// INTERNAL USBD-CLASS DRIVER API
//--------------------------------------------------------------------+
void     audiod_init             (void);
void     audiod_reset            (uint8_t rhport);
uint16_t audiod_open             (uint8_t rhport, tusb_desc_interface_t const * itf_desc, uint16_t max_len);
bool     audiod_control_request  (uint8_t rhport, tusb_control_request_t const * request);
bool     audiod_control_complete (uint8_t rhport, tusb_control_request_t const * request);
bool     audiod_xfer_cb          (uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_AUDIO_DEVICE_H_ */
