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

#include "tusb_option.h"

#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_AUDIO)

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "audio_device.h"
#include "device/usbd_pvt.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
typedef struct
{
  uint8_t itf_num;      // Interface number of Audio Streaming Interface
  uint8_t itf_data_alt; // Alternate setting of Data Interface. 0 : inactive, 1 : active
  uint8_t ep;           // End Point, may be in or out as audio streaming is unidirectional

  /*------------- From this point, data is not cleared by bus reset -------------*/
  // FIFO
  tu_fifo_t fifo;
  uint8_t fifo_buf[CFG_TUD_AUDIO_BUFSIZE];

#if CFG_FIFO_MUTEX
  osal_mutex_def_t fifo_mutex;
#endif

  // Endpoint Transfer buffer
  CFG_TUSB_MEM_ALIGN uint8_t ep_buf[CFG_TUD_AUDIO_EP_BUFSIZE];

} audiod_interface_t;

#define ITF_MEM_RESET_SIZE   offsetof(audiod_interface_t, fifo)

//--------------------------------------------------------------------+
// INTERNAL OBJECT & FUNCTION DECLARATION
//--------------------------------------------------------------------+
CFG_TUSB_MEM_SECTION static audiod_interface_t _audiod_itf[CFG_TUD_AUDIO];
CFG_TUSB_MEM_SECTION static uint8_t _audio_ctrl_data[16];

static void _prep_out_transaction (uint8_t itf)
{
  audiod_interface_t* p_audio = &_audiod_itf[itf];

  // skip if previous transfer not complete
  if ( usbd_edpt_busy(TUD_OPT_RHPORT, p_audio->ep) ) return;

  // Prepare for incoming data but only allow what we can store in the ring buffer.
  uint16_t max_read = tu_fifo_remaining(&p_audio->fifo);
  if ( max_read >= sizeof(p_audio->ep_buf) )
  {
    usbd_edpt_xfer(TUD_OPT_RHPORT, p_audio->ep, p_audio->ep_buf, sizeof(p_audio->ep_buf));
  }
}

//--------------------------------------------------------------------+
// Audio API
//--------------------------------------------------------------------+
static bool maybe_transmit(audiod_interface_t* audio)
{
  // skip if previous transfer not complete
  TU_VERIFY( !usbd_edpt_busy(TUD_OPT_RHPORT, audio->ep) );

  const uint16_t count = TU_MIN(tu_fifo_count(&audio->fifo), sizeof(audio->ep_buf));
  tu_fifo_read_n(&audio->fifo, audio->ep_buf, count);
  TU_ASSERT( usbd_edpt_xfer(TUD_OPT_RHPORT, audio->ep, audio->ep_buf, count) );

  return true;
}

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+
bool tud_audio_n_open (uint8_t itf)
{
  return tud_ready() && (_audiod_itf[itf].itf_data_alt != 0);
}

uint32_t tud_audio_n_available (uint8_t itf)
{
  return tu_fifo_count(&_audiod_itf[itf].fifo);
}

uint32_t tud_audio_n_read (uint8_t itf, void* buffer, uint32_t bufsize)
{
  uint32_t num_read = tu_fifo_read_n(&_audiod_itf[itf].fifo, buffer, bufsize);
  _prep_out_transaction(itf);
  return num_read;
}

// Clear the interface FIFO
void tud_audio_n_flush (uint8_t itf)
{
  tu_fifo_clear(&_audiod_itf[itf].fifo);
  if (tu_edpt_dir(_audiod_itf[itf].ep) == TUSB_DIR_OUT)
    _prep_out_transaction(itf);
}

// Write data to fifo, return bytes written
uint32_t tud_audio_n_write (uint8_t itf, void *buffer, uint32_t bufsize)
{
  uint16_t ret = tu_fifo_write_n(&_audiod_itf[itf].fifo, buffer, bufsize);
  maybe_transmit(&_audiod_itf[itf]);
  return ret;
}

// Return the number of bytes that can be written
uint32_t tud_cdc_n_write_available (uint8_t itf)
{
  return tu_fifo_remaining(&_audiod_itf[itf].fifo);
}

//--------------------------------------------------------------------+
// USBD Driver API
//--------------------------------------------------------------------+
void audiod_init(void)
{
  TU_LOG1( "audiod_init\n" );

  tu_memclr(_audiod_itf, sizeof(_audiod_itf));

  for(uint8_t i=0; i<CFG_TUD_AUDIO; i++)
  {
    audiod_interface_t* p_audio = &_audiod_itf[i];

    // config fifo
    tu_fifo_config(&p_audio->fifo, p_audio->fifo_buf, TU_ARRAY_SIZE(p_audio->fifo_buf), 1, false);

#if CFG_FIFO_MUTEX
    tu_fifo_config_mutex(&p_audio->fifo, osal_mutex_create(&p_audio->fifo_mutex));
#endif
  }
}

void audiod_reset(uint8_t rhport)
{
  TU_LOG1( "audiod_reset\n" );

  (void) rhport;

  for(uint8_t i=0; i<CFG_TUD_AUDIO; i++)
  {
    tu_memclr(&_audiod_itf[i], ITF_MEM_RESET_SIZE);
    tu_fifo_clear(&_audiod_itf[i].fifo);
  }
}

uint16_t audiod_open(uint8_t rhport, tusb_desc_interface_t const * desc_itf, uint16_t max_len)
{
  TU_LOG1( "audiod_open\n" );

  // 1st interface is Audio Control 
  TU_VERIFY(TUSB_CLASS_AUDIO       == desc_itf->bInterfaceClass    &&
            AUDIO_SUBCLASS_CONTROL == desc_itf->bInterfaceSubClass &&
            AUDIO_PROTOCOL_V1      == desc_itf->bInterfaceProtocol, 0);

  uint16_t drv_len = tu_desc_len(desc_itf);
  uint8_t const * p_desc = tu_desc_next(desc_itf);

  // Skip Class Specific descriptors
  while ( TUSB_DESC_CS_INTERFACE == tu_desc_type(p_desc) && drv_len < max_len )
  {
    drv_len += tu_desc_len(p_desc);
    p_desc   = tu_desc_next(p_desc);
  }

  // N number of streaming interfaces
  while ( drv_len < max_len )
  {
    TU_VERIFY(TUSB_DESC_INTERFACE == tu_desc_type(p_desc), 0);
    tusb_desc_interface_t const * desc_audio = (tusb_desc_interface_t const *) p_desc;

    TU_VERIFY(TUSB_CLASS_AUDIO         == desc_audio->bInterfaceClass    &&
              AUDIO_SUBCLASS_STREAMING == desc_audio->bInterfaceSubClass &&
              AUDIO_PROTOCOL_V1        == desc_audio->bInterfaceProtocol, 0);

    // Find available interface
    audiod_interface_t * p_audio = NULL;
    for(uint8_t i=0; i<CFG_TUD_AUDIO; i++)
    {
      if ( _audiod_itf[i].ep == 0 )
      {
        p_audio = &_audiod_itf[i];
        break;
      }
    }
    TU_VERIFY(p_audio, 0);

    p_audio->itf_num = desc_audio->bInterfaceNumber;

    // next descriptor
    drv_len += tu_desc_len(p_desc);
    p_desc   = tu_desc_next(p_desc);

    // Skip to EP descriptor
    while ( TUSB_DESC_ENDPOINT != tu_desc_type(p_desc) && drv_len < max_len )
    {
      drv_len += tu_desc_len(p_desc);
      p_desc   = tu_desc_next(p_desc);
    }
    TU_VERIFY(drv_len<max_len, 0);
  
    // Open EP
    TU_ASSERT(usbd_edpt_open(rhport, (tusb_desc_endpoint_t const *) p_desc), 0);
    p_audio->ep = ((tusb_desc_endpoint_t const *) p_desc)->bEndpointAddress;

    // Prepare for incoming data
    if ( !usbd_edpt_xfer(rhport, p_audio->ep, p_audio->ep_buf, sizeof(p_audio->ep_buf)) )
    {
      TU_LOG1_FAILED();
      TU_BREAKPOINT();
    }

    // Move past EP descritpor
    drv_len += tu_desc_len(p_desc);
    p_desc   = tu_desc_next(p_desc);

    // Skip CS_ENDPOINT
    drv_len += tu_desc_len(p_desc);
    p_desc   = tu_desc_next(p_desc);
  }

  return drv_len;
}

// Invoked when class request DATA stage is finished.
// return false to stall control endpoint (e.g Host send non-sense DATA)
bool audiod_control_complete(uint8_t rhport, tusb_control_request_t const * request)
{
  (void) rhport;

  // Handle class request only
  TU_VERIFY(request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS);

  if( request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE )
  {
    const uint8_t term_id = request->wIndex >> 8;
    const uint8_t ctrl_id = request->wValue >> 8;
    const uint8_t chan_id = request->wIndex & 0xff;

    //TU_LOG1( "audiod_control_complete: 0x%02x 0x%02x 0x%02x 0x%02x\n", request->bRequest, term_id, ctrl_id, chan_id );

    if ( request->bRequest == AUDIO_REQUEST_SET_CURRENT_VALUE )
    {
      if ( ctrl_id == AUDIO_FEATURE_UNIT_CTRL_MUTE )
      {
        if ( tud_audio_ctrl_set_mute )
          tud_audio_ctrl_set_mute(term_id, chan_id, _audio_ctrl_data, request->wLength );
      }
      else if ( ctrl_id == AUDIO_FEATURE_UNIT_CTRL_VOLUME )
      {
        if ( tud_audio_ctrl_set_volume )
          tud_audio_ctrl_set_volume(term_id, chan_id, (int16_t *)_audio_ctrl_data, request->wLength );
      }
    }
  }

  return true;
}

// Handle class control request
// return false to stall control endpoint (e.g unsupported request)
bool audiod_control_request(uint8_t rhport, tusb_control_request_t const * request)
{
  if ( request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD )
  {
    uint8_t const req_itfnum = (uint8_t) request->wIndex;

    // Find the interface this request is referring too
    audiod_interface_t * p_audio = NULL;
    for(uint8_t i=0; i<CFG_TUD_AUDIO; i++)
    {
      if ( _audiod_itf[i].itf_num == req_itfnum )
      {
        p_audio = &_audiod_itf[i];
        break;
      }
    }
    TU_VERIFY(p_audio);

    if ( request->bRequest == TUSB_REQ_GET_INTERFACE )
      return tud_control_xfer(rhport, request, &p_audio->itf_data_alt, 1);
    else if ( request->bRequest == TUSB_REQ_SET_INTERFACE )
    {
      uint8_t const req_alt = (uint8_t) request->wValue;
      TU_VERIFY(req_alt<2);

      p_audio->itf_data_alt = req_alt;

      // If the alt is 0, we are no longer rx/tx, so clear fifo
      if ( 0 == req_alt )
      {
        tu_fifo_clear(&p_audio->fifo);
        tud_audio_channel_close(req_itfnum);
      }
      else
        tud_audio_channel_open(req_itfnum);

      return tud_control_status(rhport, request);
    }

    // Unsupported request
    TU_LOG1( "Unhandled ctrl req: 0x%04x\n", request->bRequest );
    return false;
  }

  // Handle class request only
  TU_VERIFY(request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS);

  if( request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE )
  {
    const uint8_t term_id = request->wIndex >> 8;
    const uint8_t ctrl_id = request->wValue >> 8;
    const uint8_t chan_id = request->wIndex & 0xff;
    uint8_t len = sizeof(_audio_ctrl_data);

    //TU_LOG1( "CTRL: 0x%02x 0x%02x 0x%02x 0x%02x\n", request->bRequest, term_id, ctrl_id, chan_id );

    /* Notes: Volume min/max is signed fixed point 16.16, so max should be 0, min 0x8001 ( -127.9961 dB )
    *        Mute only supports CUR, no MIN/MAX
    *        EP control requests are sample rate, and support CUR, MIN, MAX
    */

    switch ( request->bRequest )
    {
      case AUDIO_REQUEST_GET_CURRENT_VALUE:
        if ( ctrl_id == AUDIO_FEATURE_UNIT_CTRL_MUTE )
        {
          if ( tud_audio_ctrl_get_mute && tud_audio_ctrl_get_mute(term_id, chan_id, _audio_ctrl_data, &len ) )
            return tud_control_xfer(rhport, request, _audio_ctrl_data, len);
        }
        else if ( ctrl_id == AUDIO_FEATURE_UNIT_CTRL_VOLUME )
        {
          if ( tud_audio_ctrl_get_volume && tud_audio_ctrl_get_volume(term_id, chan_id, (int16_t *)_audio_ctrl_data, &len ) )
            return tud_control_xfer(rhport, request, _audio_ctrl_data, len);
        }
        break;

      case AUDIO_REQUEST_SET_CURRENT_VALUE:
        if ( ctrl_id == AUDIO_FEATURE_UNIT_CTRL_MUTE )
        {
          if ( tud_audio_ctrl_set_mute )
            return tud_control_xfer(rhport, request, _audio_ctrl_data, 1); // Get value
        }
        else if ( ctrl_id == AUDIO_FEATURE_UNIT_CTRL_VOLUME )
        {
          if ( tud_audio_ctrl_set_volume )
            return tud_control_xfer(rhport, request, _audio_ctrl_data, chan_id == 0xff ? 4 : 2); // Get value
        }
        break;

      case AUDIO_REQUEST_GET_MINIMUM_VALUE:
        if ( ctrl_id == AUDIO_FEATURE_UNIT_CTRL_VOLUME )
        {
          *(uint16_t *)_audio_ctrl_data = 0;
          return tud_control_xfer(rhport, request, _audio_ctrl_data, 2);
        }
        break;

      case AUDIO_REQUEST_GET_MAXIMUM_VALUE:
        if ( ctrl_id == AUDIO_FEATURE_UNIT_CTRL_VOLUME )
        {
          *(uint16_t *)_audio_ctrl_data = 127;
          return tud_control_xfer(rhport, request, _audio_ctrl_data, 2);
        }
        break;

      case AUDIO_REQUEST_GET_RESOLUTION_VALUE:
        if ( ctrl_id == AUDIO_FEATURE_UNIT_CTRL_VOLUME )
        {
          *(uint16_t *)_audio_ctrl_data = 1;
          return tud_control_xfer(rhport, request, _audio_ctrl_data, 2);
        }
        break;
    }

  }
  else if( request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_ENDPOINT )
  {
    TU_LOG1( "EP: 0x%02x 0x%04x 0x%04x\n", request->bRequest, request->wValue, request->wIndex );

    const uint8_t ctrl_id = request->wValue >> 8;
    const uint8_t ep = request->wIndex & 0xff;

    TU_VERIFY(ctrl_id == AUDIO_EP_CTRL_SAMPLING_FREQ);

    if ( request->bRequest == AUDIO_REQUEST_GET_CURRENT_VALUE )
    {
      if( tud_audio_ep_get_srate && tud_audio_ep_get_srate(ep, (uint32_t *)_audio_ctrl_data ) )
        return tud_control_xfer(rhport, request, _audio_ctrl_data, 3);
    }
    if ( request->bRequest == AUDIO_REQUEST_GET_CURRENT_VALUE )
    {
      if( tud_audio_ep_set_srate )
        return tud_control_xfer(rhport, request, _audio_ctrl_data, 3);
    }
  }

  // Return false for unsupported control request
  return false;
}

bool audiod_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
  (void) result;

  //TU_LOG1( "audiod_xfer_cb\n" );

  uint8_t itf = 0;
  audiod_interface_t* p_audio = _audiod_itf;

  for ( ; ; itf++, p_audio++)
  {
    if (itf >= TU_ARRAY_SIZE(_audiod_itf)) return false;

    if ( ep_addr == p_audio->ep ) break;
  }

  // receive new data
  if (tu_edpt_dir(ep_addr) == TUSB_DIR_OUT)
  {
    const uint32_t count = tu_fifo_write_n(&p_audio->fifo, p_audio->ep_buf, xferred_bytes);
    if( count < xferred_bytes )
      TU_LOG1( "Overflow: %u / %u\n", count, xferred_bytes );

    // prepare for next
    TU_ASSERT( usbd_edpt_xfer(rhport, p_audio->ep, p_audio->ep_buf, sizeof(p_audio->ep_buf)), false );
  } else {
    maybe_transmit(p_audio);
  }

  return true;
}

#endif
