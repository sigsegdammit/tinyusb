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

/** \ingroup group_class
 *  \defgroup ClassDriver_Audio Audio
 *            Currently only MIDI subclass is supported
 *  @{ */

#ifndef _TUSB_AUDIO_H__
#define _TUSB_AUDIO_H__

#include "common/tusb_common.h"

#ifdef __cplusplus
 extern "C" {
#endif

/// Audio Interface Subclass Codes
typedef enum
{
  AUDIO_SUBCLASS_CONTROL = 0x01  , ///< Audio Control
  AUDIO_SUBCLASS_STREAMING       , ///< Audio Streaming
  AUDIO_SUBCLASS_MIDI_STREAMING  , ///< MIDI Streaming
} audio_subclass_type_t;

/// Audio Protocol Codes
typedef enum
{
  AUDIO_PROTOCOL_V1                   = 0x00, ///< Version 1.0
  AUDIO_PROTOCOL_V2                   = 0x20, ///< Version 2.0
  AUDIO_PROTOCOL_V3                   = 0x30, ///< Version 3.0
} audio_protocol_type_t;

/// Audio Function Category Codes
typedef enum
{
  AUDIO_FUNC_DESKTOP_SPEAKER    = 0x01,
  AUDIO_FUNC_HOME_THEATER       = 0x02,
  AUDIO_FUNC_MICROPHONE         = 0x03,
  AUDIO_FUNC_HEADSET            = 0x04,
  AUDIO_FUNC_TELEPHONE          = 0x05,
  AUDIO_FUNC_CONVERTER          = 0x06,
  AUDIO_FUNC_SOUND_RECODER      = 0x07,
  AUDIO_FUNC_IO_BOX             = 0x08,
  AUDIO_FUNC_MUSICAL_INSTRUMENT = 0x09,
  AUDIO_FUNC_PRO_AUDIO          = 0x0A,
  AUDIO_FUNC_AUDIO_VIDEO        = 0x0B,
  AUDIO_FUNC_CONTROL_PANEL      = 0x0C
} audio_function_t;

/// Audio Class-Specific AC Interface Descriptor Subtypes
typedef enum
{
  AUDIO_CS_INTERFACE_HEADER                = 0x01,
  AUDIO_CS_INTERFACE_INPUT_TERMINAL        = 0x02,
  AUDIO_CS_INTERFACE_OUTPUT_TERMINAL       = 0x03,
  AUDIO_CS_INTERFACE_MIXER_UNIT            = 0x04,
  AUDIO_CS_INTERFACE_SELECTOR_UNIT         = 0x05,
  AUDIO_CS_INTERFACE_FEATURE_UNIT          = 0x06,
  AUDIO_CS_INTERFACE_EFFECT_UNIT           = 0x07,
  AUDIO_CS_INTERFACE_PROCESSING_UNIT       = 0x08,
  AUDIO_CS_INTERFACE_EXTENSION_UNIT        = 0x09,
  AUDIO_CS_INTERFACE_CLOCK_SOURCE          = 0x0A,
  AUDIO_CS_INTERFACE_CLOCK_SELECTOR        = 0x0B,
  AUDIO_CS_INTERFACE_CLOCK_MULTIPLIER      = 0x0C,
  AUDIO_CS_INTERFACE_SAMPLE_RATE_CONVERTER = 0x0D,
} audio_cs_interface_subtype_t;

/// Audio Class USB Terminal Types
typedef enum{
  AUDIO_USB_TERMINAL_STREAMING             = 0x0101,
  AUDIO_USB_TERMINAL_VENDOR_SPECIFIC       = 0x01FF,
  AUDIO_INPUT_TERMINAL_MICROPHONE          = 0x0201,
  AUDIO_INPUT_TERMINAL_DESKTOP_MICROPHONE  = 0x0202,
  AUDIO_INPUT_TERMINAL_PERSONAL_MICROPHONE = 0x0203,
  AUDIO_INPUT_TERMINAL_OMNI_MICROPHONE     = 0x0204,
  AUDIO_INPUT_TERMINAL_ARRAY_MICROPHONE    = 0x0205,
  AUDIO_INPUT_TERMINAL_PROC_ARRAY_MICROPHONE  = 0x0206,
  AUDIO_OUTPUT_TERMINAL_SPEAKER            = 0x0301,
  AUDIO_OUTPUT_TERMINAL_HEADPHONES         = 0x0302,
  AUDIO_OUTPUT_TERMINAL_HEADSUP_DISPLAY    = 0x0303,
  AUDIO_OUTPUT_TERMINAL_DESKTOP_SPEAKER    = 0x0304,
  AUDIO_OUTPUT_TERMINAL_ROOM_SPEAKER       = 0x0305,
  AUDIO_OUTPUT_TERMINAL_COMM_SPEAKER       = 0x0306,
  AUDIO_OUTPUT_TERMINAL_LFE_SPEAKER        = 0x0307,
  //... More to do
} audio_input_terminal_type_t;

typedef enum{
  AUDIO_FORMAT_TYPE_I           = 0x0001,
  AUDIO_FORMAT_TYPE_II          = 0x0002,
  AUDIO_FORMAT_TYPE_III         = 0x0003,
  AUDIO_FORMAT_TYPE_IV          = 0x0004,
  AUDIO_EXT_FORMAT_TYPE_I       = 0x0081,
  AUDIO_EXT_FORMAT_TYPE_II      = 0x0082,
  AUDIO_EXT_FORMAT_TYPE_IIII    = 0x0083,
} audio_format_type_t;

//--------------------------------------------------------------------+
// Class Specific Functional Descriptor (Audio Interface)
//--------------------------------------------------------------------+

/// Audio Class Interface Header Descriptor (Audio Interface)
typedef struct TU_ATTR_PACKED
{
  uint8_t bLength            ; ///< Size of this descrptor in bytes: 9
  uint8_t bDescriptorType    ; ///< CS_INTERFACE descriptor type.
  uint8_t bDescriptorSubType ; ///< HEADER descriptor subtype.
  uint16_t bcdADC            ; ///< Audio Device Class Specification Release Number in Binary-Coded Decimal.
  uint8_t bCategory          ; ///< Constant, indicating the primary use of this audio function, as intended by the manufacturer. See Appendix A.7, “Audio Function Category Codes.”
  uint16_t wTotalLength      ; ///< Total number of bytes returned for the class-specific AudioControl interface descriptor. Includes the combined length of this descriptor header and all Clock Source, Unit and Terminal descriptors.
  uint8_t bmControls         ; ///< Latency Control
}audio_class_interface_header_t;

/// Audio Clock Source Descriptor (Audio Interface)
typedef struct TU_ATTR_PACKED
{
  uint8_t bLength            ; ///< Size of this descrptor in bytes: 8
  uint8_t bDescriptorType    ; ///< CS_INTERFACE descriptor type.
  uint8_t bDescriptorSubType ; ///< CLOCK_SOURCE descriptor subtype.
  uint8_t bCLockID           ; ///< Constant uniquely identifying the Clock Source Entity within the audio function. This value is used in all requests to address this Entity.
  uint8_t bmAttributes       ; ///< Clock Type
  uint8_t bmControls         ; ///< ...
  uint8_t bAssocTerminal     ; ///< Terminal ID of the Terminal that is associated with this Clock Source.
  uint8_t iClockSource       ; ///< Index of a string descriptor, describing the Clock Source Entity.
}audio_clock_source_desc_t;

/// Audio Input Terminal Descriptor (Audio Interface)
typedef struct TU_ATTR_PACKED
{
  uint8_t bLength            ; ///< Size of this descrptor in bytes: 17
  uint8_t bDescriptorType    ; ///< CS_INTERFACE descriptor type.
  uint8_t bDescriptorSubType ; ///< INPUT_TERMINAL descriptor subtype.
  uint8_t bTerminalID        ; ///< Constant uniquely identifying the Terminal within the audio function. This value is used in all requests to address this Terminal.
  uint16_t wTerminalType     ; ///< Constant characterizing the type of Terminal. See USB Audio Terminal Types.
  uint8_t bAssocTerminal     ; ///< ID of the Output Terminal to which this Input Terminal is associated.
  uint8_t bCSourceID         ; ///< ID of the Clock Entity to which this Input Terminal is connected.
  uint8_t bNrChannels        ; ///< Number of logical output channels in the Terminal’s output audio channel cluster.
  uint32_t bmChannelConfig   ; ///< Describes the spatial location of the logical channels.
  uint8_t iChannelNames      ; ///< Index of a string descriptor, describing the name of the first logical channel.
  uint16_t bmControls        ; ///< ...
  uint8_t iTerminal          ; ///< Index of a string descriptor, describing the Input Terminal.
}audio_input_terminal_desc_t;

/// Audio Input Terminal Descriptor (Audio Interface)
typedef struct TU_ATTR_PACKED
{
  uint8_t bLength            ; ///< Size of this descrptor in bytes: 12
  uint8_t bDescriptorType    ; ///< CS_INTERFACE descriptor type.
  uint8_t bDescriptorSubType ; ///< OUTPUT_TERMINAL descriptor subtype.
  uint8_t bTerminalID        ; ///< Constant uniquely identifying the Terminal within the audio function. This value is used in all requests to address this Terminal.
  uint16_t wTerminalType     ; ///< Constant characterizing the type of Terminal. See USB Audio Terminal Types.
  uint8_t bAssocTerminal     ; ///< ID of the Input Terminal to which this Output Terminal is associated.
  uint8_t bCSourceID         ; ///< ID of the Clock Entity to which this Output Terminal is connected.
  uint16_t bmControls        ; ///< ...
  uint8_t iTerminal          ; ///< Index of a string descriptor, describing the Output Terminal.
}audio_output_terminal_desc_t;

/// Audio Channel Cluster Description (Audio Interface)
typedef struct TU_ATTR_PACKED
{
  uint8_t bNrChannels        ; ///< Number of logical output chanels in the Terminal's outptu audio cluster
  uint32_t bmChannelConfig   ; ///< Describes the spatial location of the logical channels
  uint8_t iChannelNames      ; ///< Index of a string descroptor, describing the name of the first logical channel
}audio_desc_channel_cluster_t;

/// Interface Descriptor (Audio Stream)
typedef struct TU_ATTR_PACKED
{
  uint8_t bLength            ; ///< Size of this descrptor in bytes: 16
  uint8_t bDescriptorType    ; ///< CS_INTERFACE descriptor type.
  uint8_t bDescriptorSubtype ; ///< AS_GENERAL descriptor subtype.
  uint8_t bTerminalLink      ; ///< The Terminal ID of the Terminal to which this interface is connected.
  uint8_t bmControls         ; ///< ...
  uint8_t bFormatType        ; ///< Constant identifying the Format Type the AudioStreaming interface is using.
  uint32_t bmFormats         ; ///< The Audio Data Format(s) that can be used to communicate with this interface. See the USB Audio Data Formats document for further details.
  uint8_t bNrChannels        ; ///< Number of physical channels in the AS Interface audio channel cluster.
  uint32_t bmChannelConfig   ; ///< Describes the spatial location of the physical channels.
  uint8_t iChannelNames      ; ///< Index of a string descriptor, describing the name of the first physical channel.
}audio_stream_interface_desc_t;

/** @} */

#ifdef __cplusplus
 }
#endif

#endif

/** @} */
