/*
* *** Honeywell (Ademco) Door/Window Sensors (345.0Mhz) ***
*
* Tested with the Honeywell 5811 Wireless Door/Window transmitters
*
* 64 bit packets, repeated multiple times per open/close event
*
* Protocol whitepaper: "DEFCON 22: Home Insecurity" by Logan Lamb
*
* PP PP C IIIII EE SS SS
* P: 16bit Preamble and sync bit (always ff fe)
* C: 4bit Channel
* I: 20bit Device serial number / or counter value
* E: 8bit Event, where 0x80 = Open/Close, 0x04 = Heartbeat / or id
* S: 16bit CRC
*
*/

#include "rtl_433.h"
#include "pulse_demod.h"
#include "util.h"

static int honeywell_callback(bitbuffer_t *bitbuffer) {
  char time_str[LOCAL_TIME_BUFLEN];
  const uint8_t *bb;
  int channel;
  int device_id;
  int event;
  int state;
  int heartbeat;
  uint16_t crc_calculated;
  uint16_t crc;

  if(bitbuffer->num_rows != 1 || bitbuffer->bits_per_row[0] != 64)
    return 0; // Unrecognized data

  for(uint16_t i=0; i < 8; i++)
    bitbuffer->bb[0][i] = ~bitbuffer->bb[0][i];

  bb = bitbuffer->bb[0];

  crc_calculated = crc16_ccitt(bb, 6, 0x8005, 0xfffe);
  crc = (((uint16_t) bb[6]) << 8) + ((uint16_t) bb[7]);
  if(crc != crc_calculated)
    return 0; // Not a valid packet

  channel = bb[2] >> 4;
  device_id = ((bb[2] & 0xf) << 16) | (bb[3] << 8)| bb[4];
  event = bb[5];
  state = (event & 0x80) >> 7;
  heartbeat = (event & 0x04) >> 2;

  local_time_str(0, time_str);

  data_t *data = data_make(
                   "time",     "", DATA_STRING, time_str,
                   "model", "", DATA_STRING, "Honeywell Door/Window Sensor",
                   "id",       "", DATA_FORMAT, "%05x", DATA_INT, device_id,
                   "channel","", DATA_INT, channel,
                   "event","", DATA_FORMAT, "%02x", DATA_INT, event,
                   "state",    "", DATA_STRING, state ? "open" : "closed",
                   "heartbeat" , "", DATA_STRING, heartbeat ? "yes" : "no",
                          NULL);

  data_acquired_handler(data);
  return 1;
}

static char *output_fields[] = {
        "time",
        "model",
        "id",
        "channel",
        "event",
        "state",
        "heartbeat",
        NULL
};

r_device honeywell = {
        .name                   = "Honeywell Door/Window Sensor",
        .modulation             = OOK_PULSE_MANCHESTER_ZEROBIT,
        .short_limit    =   39 * 4,
        .long_limit             = 0,
        .reset_limit    = 73 * 4,
        .json_callback  = &honeywell_callback,
        .disabled               = 0,
        .demod_arg              = 0,
        .fields                 = output_fields,
};
