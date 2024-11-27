/*
    _librmt files are freely adapted from Chris Osborn / Martin Falatic libraries
    Original author and license: Chris Osborn & Martin F. Falatic / MIT license 
    Modified by: Thomas BOHL for KXKM / MIT license / 2020
*/

/*
 * Library for driving digital RGB(W) LEDs using the ESP32's RMT peripheral
 *
 * Modifications Copyright (c) 2017 Martin F. Falatic
 *
 * Based on public domain code created 19 Nov 2016 by Chris Osborn <fozztexx@fozztexx.com>
 * http://insentricity.com
 *
 */
/*
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
 */


#include "esp32_digital_led_lib.h"
#include <soc/dport_reg.h> // ESP-IDF 4

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ARDUINO)
  #include "esp32-hal.h"
  #include "esp_intr.h"
  #include "driver/gpio.h"
  #include "driver/rmt.h"
  #include "driver/periph_ctrl.h"
  #include "freertos/semphr.h"
  #include "soc/rmt_struct.h"
#elif defined(ESP_PLATFORM)
  #include <esp_intr.h>
  #include <driver/gpio.h>
  #include <driver/rmt.h>
  #include <freertos/FreeRTOS.h>
  #include <freertos/semphr.h>
  #include <soc/dport_reg.h>
  #include <soc/gpio_sig_map.h>
  #include <soc/rmt_struct.h>
  #include <stdio.h>
  #include <string.h>  // memset, memcpy, etc. live here!
#endif

#ifdef __cplusplus
}
#endif

#if DEBUG_ESP32_DIGITAL_LED_LIB
extern char * digitalLeds_debugBuffer;
extern int digitalLeds_debugBufferSz;
#endif

static DRAM_ATTR const uint16_t MAX_PULSES = 32;  // A channel has a 64 "pulse" buffer - we use half per pass
static DRAM_ATTR const uint16_t DIVIDER    =  4;  // 8 still seems to work, but timings become marginal
static DRAM_ATTR const double   RMT_DURATION_NS = 12.5;  // Minimum time of a single RMT duration based on clock ns

// LUT for mapping bits in RMT.int_<op>.ch<n>_tx_thr_event
static DRAM_ATTR const uint32_t tx_thr_event_offsets [] = {
  static_cast<uint32_t>(1) << (24 + 0),
  static_cast<uint32_t>(1) << (24 + 1),
  static_cast<uint32_t>(1) << (24 + 2),
  static_cast<uint32_t>(1) << (24 + 3),
  static_cast<uint32_t>(1) << (24 + 4),
  static_cast<uint32_t>(1) << (24 + 5),
  static_cast<uint32_t>(1) << (24 + 6),
  static_cast<uint32_t>(1) << (24 + 7),
};

// LUT for mapping bits in RMT.int_<op>.ch<n>_tx_end
static DRAM_ATTR const uint32_t tx_end_offsets [] = {
  static_cast<uint32_t>(1) << (0 + 0) * 3,
  static_cast<uint32_t>(1) << (0 + 1) * 3,
  static_cast<uint32_t>(1) << (0 + 2) * 3,
  static_cast<uint32_t>(1) << (0 + 3) * 3,
  static_cast<uint32_t>(1) << (0 + 4) * 3,
  static_cast<uint32_t>(1) << (0 + 5) * 3,
  static_cast<uint32_t>(1) << (0 + 6) * 3,
  static_cast<uint32_t>(1) << (0 + 7) * 3,
};

// Gamma correction table (RED calibrated)
// const uint8_t gamma8_table[] = {
//    0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
//    2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
//    3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,  4,
//    5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  7,  7,  7,  7,
//    7,  8,  8,  8,  8,  9,  9,  9,  9, 10, 10, 10, 11, 11, 11, 12,
//   12, 12, 13, 13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17, 18, 18,
//   18, 19, 19, 20, 20, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 27,
//   27, 28, 29, 29, 30, 31, 31, 32, 33, 33, 34, 35, 36, 36, 37, 38,
//   39, 40, 40, 41, 42, 43, 44, 45, 46, 47, 47, 48, 49, 50, 51, 52,
//   53, 54, 55, 56, 57, 58, 59, 61, 62, 63, 64, 65, 66, 67, 68, 70,
//   71, 72, 73, 75, 76, 77, 78, 80, 81, 82, 84, 85, 86, 88, 89, 91,
//   92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,115,
//  117,119,120,122,124,125,127,129,131,133,134,136,138,140,142,144,
//  146,148,150,152,154,156,158,160,162,164,166,168,170,172,175,177,
//  178,180,182,185,187,189,192,194,196,199,201,203,206,208,210,211,
//  212,214,217,220,223,226,229,232,234,237,240,243,246,249,252,255 };

// Gamma 2.2
const uint8_t gamma8_table[] = {
   0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,
   3,  3,  4,  4,  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,  6,  7,
   7,  7,  8,  8,  8,  9,  9,  9, 10, 10, 10, 11, 11, 11, 12, 12,
  13, 13, 14, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20,
  20, 21, 22, 22, 23, 23, 24, 24, 25, 26, 26, 27, 28, 28, 29, 30,
  30, 31, 32, 32, 33, 34, 34, 35, 36, 37, 37, 38, 39, 40, 41, 41,
  42, 43, 44, 45, 46, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
  56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68, 69, 70, 71, 72,
  73, 74, 75, 76, 78, 79, 80, 81, 82, 83, 85, 86, 87, 88, 89, 91,
  92, 93, 94, 96, 97, 98,100,101,102,104,105,106,108,109,110,112,
 113,115,116,118,119,120,122,123,125,126,128,129,131,132,134,136,
 137,139,140,142,143,145,147,148,150,152,153,155,157,158,160,162,
 163,165,167,169,170,172,174,176,177,179,181,183,185,187,188,190,
 192,194,196,198,200,202,204,206,208,210,212,214,216,218,220,222,
 224,226,228,230,232,234,236,238,240,242,245,247,249,251,253,255 };

typedef union {
  struct {
    uint32_t duration0:15;
    uint32_t level0:1;
    uint32_t duration1:15;
    uint32_t level1:1;
  };
  uint32_t val;
} rmtPulsePair;

typedef struct {
  uint8_t * buf_data;
  uint16_t buf_pos, buf_len, buf_half, buf_isDirty;
  xSemaphoreHandle sem;
  rmtPulsePair pulsePairMap[2];
} digitalLeds_stateData;

static strand_t localStrands[8];
static int localStrandCnt = 0;

static intr_handle_t rmt_intr_handle = nullptr;

// Forward declarations of local functions
static void copyToRmtBlock_half(strand_t * pStrand);
static void handleInterrupt(void *arg);

uint8_t gamma8(uint8_t value)
{
  return (int)value*(int)value/255;
  //return gamma8_table[value];
}

int digitalLeds_init() 
{
  DPORT_SET_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, DPORT_RMT_CLK_EN);
  DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, DPORT_RMT_RST);

  RMT.apb_conf.fifo_mask = 1;       // Enable memory access, instead of FIFO mode
  RMT.apb_conf.mem_tx_wrap_en = 1;  // Wrap around when hitting end of buffer
  
  esp_intr_alloc(ETS_RMT_INTR_SOURCE, 0, handleInterrupt, nullptr, &rmt_intr_handle);

  return 0;
}

strand_t* digitalLeds_addStrand(strand_t strands)
{
  if (localStrandCnt >= 8) return nullptr;

  localStrandCnt += 1;
  localStrands[localStrandCnt-1] = strands;

  strand_t * pStrand = &localStrands[localStrandCnt-1];
  ledParams_t ledParams = ledParamsAll[pStrand->ledType];

  pStrand->pixels = static_cast<pixelColor_t*>(malloc(pStrand->numPixels * sizeof(pixelColor_t)));
  if (pStrand->pixels == nullptr) {
    return nullptr;
  }
  memset(pStrand->pixels, 0, pStrand->numPixels * sizeof(pixelColor_t));

  pStrand->_stateVars = static_cast<digitalLeds_stateData*>(malloc(sizeof(digitalLeds_stateData)));
  if (pStrand->_stateVars == nullptr) {
    return nullptr;
  }
  digitalLeds_stateData * pState = static_cast<digitalLeds_stateData*>(pStrand->_stateVars);

  pState->buf_len = (pStrand->numPixels * ledParams.bytesPerPixel);
  pState->buf_data = static_cast<uint8_t*>(malloc(pState->buf_len));
  if (pState->buf_data == nullptr) {
    return nullptr;
  }

  #if defined(ARDUINO) && ARDUINO >= 100
    pinMode(pStrand->gpioNum, OUTPUT);
    digitalWrite(pStrand->gpioNum, LOW);
  #elif defined(ESP_PLATFORM)
    gpio_num_t gpioNumNative = static_cast<gpio_num_t>(pStrand->gpioNum);
    gpio_mode_t gpioModeNative = static_cast<gpio_mode_t>(OUTPUT);
    gpio_pad_select_gpio(gpioNumNative);
    gpio_set_direction(gpioNumNative, gpioModeNative);
    gpio_set_level(gpioNumNative, LOW);
  #endif

  rmt_set_pin(
    static_cast<rmt_channel_t>(pStrand->rmtChannel),
    RMT_MODE_TX,
    static_cast<gpio_num_t>(pStrand->gpioNum));

  RMT.conf_ch[pStrand->rmtChannel].conf0.div_cnt = DIVIDER;
  RMT.conf_ch[pStrand->rmtChannel].conf0.mem_size = 1;
  RMT.conf_ch[pStrand->rmtChannel].conf0.carrier_en = 0;
  RMT.conf_ch[pStrand->rmtChannel].conf0.carrier_out_lv = 1;
  RMT.conf_ch[pStrand->rmtChannel].conf0.mem_pd = 0;

  RMT.conf_ch[pStrand->rmtChannel].conf1.rx_en = 0;
  RMT.conf_ch[pStrand->rmtChannel].conf1.mem_owner = 0;
  RMT.conf_ch[pStrand->rmtChannel].conf1.tx_conti_mode = 0;  //loop back mode
  RMT.conf_ch[pStrand->rmtChannel].conf1.ref_always_on = 1;  // use apb clock: 80M
  RMT.conf_ch[pStrand->rmtChannel].conf1.idle_out_en = 1;
  RMT.conf_ch[pStrand->rmtChannel].conf1.idle_out_lv = 0;

  RMT.tx_lim_ch[pStrand->rmtChannel].limit = MAX_PULSES;

  // RMT config for transmitting a '0' bit val to this LED strand
  pState->pulsePairMap[0].level0 = 1;
  pState->pulsePairMap[0].level1 = 0;
  pState->pulsePairMap[0].duration0 = ledParams.T0H / (RMT_DURATION_NS * DIVIDER);
  pState->pulsePairMap[0].duration1 = ledParams.T0L / (RMT_DURATION_NS * DIVIDER);

  // RMT config for transmitting a '0' bit val to this LED strand
  pState->pulsePairMap[1].level0 = 1;
  pState->pulsePairMap[1].level1 = 0;
  pState->pulsePairMap[1].duration0 = ledParams.T1H / (RMT_DURATION_NS * DIVIDER);
  pState->pulsePairMap[1].duration1 = ledParams.T1L / (RMT_DURATION_NS * DIVIDER);

  RMT.int_ena.val |= tx_thr_event_offsets[pStrand->rmtChannel];  // RMT.int_ena.ch<n>_tx_thr_event = 1;
  RMT.int_ena.val |= tx_end_offsets[pStrand->rmtChannel];  // RMT.int_ena.ch<n>_tx_end = 1;

  digitalLeds_updatePixels(pStrand);

  return pStrand;
}


int IRAM_ATTR digitalLeds_updatePixels(strand_t * pStrand)
{
  digitalLeds_stateData * pState = static_cast<digitalLeds_stateData*>(pStrand->_stateVars);
  ledParams_t ledParams = ledParamsAll[pStrand->ledType];

  int size = ledParams.bytesPerPixel;

  if (size == 3 || size == 4) {
    for (uint16_t i = 0; i < pStrand->numPixels; i++) {

      uint8_t a, b, c = 0;

      if (ledParams.ledOrder == S_GRB) 
      {
        a = pStrand->pixels[i].g;
        b = pStrand->pixels[i].r;
        c = pStrand->pixels[i].b;
      }
      else if (ledParams.ledOrder == S_GBR) 
      {
        a = pStrand->pixels[i].g;
        b = pStrand->pixels[i].b;
        c = pStrand->pixels[i].r;
      }
      else if (ledParams.ledOrder == S_RGB) 
      {
        a = pStrand->pixels[i].r;
        b = pStrand->pixels[i].g;
        c = pStrand->pixels[i].b;
      }
      else if (ledParams.ledOrder == S_RBG) 
      {
        a = pStrand->pixels[i].r;
        b = pStrand->pixels[i].b;
        c = pStrand->pixels[i].g;
      }
      else if (ledParams.ledOrder == S_BRG) 
      {
        a = pStrand->pixels[i].b;
        b = pStrand->pixels[i].r;
        c = pStrand->pixels[i].g;
      }
      else if (ledParams.ledOrder == S_BGR) 
      {
        a = pStrand->pixels[i].b;
        b = pStrand->pixels[i].g;
        c = pStrand->pixels[i].r;
      }

      pState->buf_data[0 + i * size] = gamma8(a);
      pState->buf_data[1 + i * size] = gamma8(b);
      pState->buf_data[2 + i * size] = gamma8(c);

      if (size == 4) 
        pState->buf_data[3 + i * size] = gamma8(pStrand->pixels[i].w);
      
    }
  }
  else {
    return -1;
  }

  pState->buf_pos = 0;
  pState->buf_half = 0;

  copyToRmtBlock_half(pStrand);

  if (pState->buf_pos < pState->buf_len) {
    // Fill the other half of the buffer block
    #if DEBUG_ESP32_DIGITAL_LED_LIB
      snprintf(digitalLeds_debugBuffer, digitalLeds_debugBufferSz,
               "%s# ", digitalLeds_debugBuffer);
    #endif
    copyToRmtBlock_half(pStrand);
  }

  pState->sem = xSemaphoreCreateBinary();

  RMT.conf_ch[pStrand->rmtChannel].conf1.mem_rd_rst = 1;
  RMT.conf_ch[pStrand->rmtChannel].conf1.tx_start = 1;

  xSemaphoreTake(pState->sem, portMAX_DELAY);
  vSemaphoreDelete(pState->sem);
  pState->sem = nullptr;

  return 0;
}

static IRAM_ATTR void copyToRmtBlock_half(strand_t * pStrand)
{
  // This fills half an RMT block
  // When wraparound is happening, we want to keep the inactive half of the RMT block filled

  digitalLeds_stateData * pState = static_cast<digitalLeds_stateData*>(pStrand->_stateVars);
  ledParams_t ledParams = ledParamsAll[pStrand->ledType];

  uint16_t i, j, offset, len, byteval;

  offset = pState->buf_half * MAX_PULSES;
  pState->buf_half = !pState->buf_half;

  len = pState->buf_len - pState->buf_pos;
  if (len > (MAX_PULSES / 8))
    len = (MAX_PULSES / 8);

  if (!len) {
    if (!pState->buf_isDirty) {
      return;
    }
    // Clear the channel's data block and return
    for (i = 0; i < MAX_PULSES; i++) {
      RMTMEM.chan[pStrand->rmtChannel].data32[i + offset].val = 0;
    }
    pState->buf_isDirty = 0;
    return;
  }
  pState->buf_isDirty = 1;

  for (i = 0; i < len; i++) {
    byteval = pState->buf_data[i + pState->buf_pos];

    #if DEBUG_ESP32_DIGITAL_LED_LIB
      snprintf(digitalLeds_debugBuffer, digitalLeds_debugBufferSz,
               "%s%d(", digitalLeds_debugBuffer, byteval);
    #endif

    // Shift bits out, MSB first, setting RMTMEM.chan[n].data32[x] to
    // the rmtPulsePair value corresponding to the buffered bit value
    for (j = 0; j < 8; j++, byteval <<= 1) {
      int bitval = (byteval >> 7) & 0x01;
      int data32_idx = i * 8 + offset + j;
      RMTMEM.chan[pStrand->rmtChannel].data32[data32_idx].val = pState->pulsePairMap[bitval].val;
      #if DEBUG_ESP32_DIGITAL_LED_LIB
        snprintf(digitalLeds_debugBuffer, digitalLeds_debugBufferSz,
                 "%s%d", digitalLeds_debugBuffer, bitval);
      #endif
    }
    #if DEBUG_ESP32_DIGITAL_LED_LIB
      snprintf(digitalLeds_debugBuffer, digitalLeds_debugBufferSz,
               "%s) ", digitalLeds_debugBuffer);
    #endif

    // Handle the reset bit by stretching duration1 for the final bit in the stream
    if (i + pState->buf_pos == pState->buf_len - 1) {
      RMTMEM.chan[pStrand->rmtChannel].data32[i * 8 + offset + 7].duration1 =
        ledParams.TRS / (RMT_DURATION_NS * DIVIDER);
      #if DEBUG_ESP32_DIGITAL_LED_LIB
        snprintf(digitalLeds_debugBuffer, digitalLeds_debugBufferSz,
                 "%sRESET ", digitalLeds_debugBuffer);
      #endif
    }
  }

  // Clear the remainder of the channel's data not set above
  for (i *= 8; i < MAX_PULSES; i++) {
    RMTMEM.chan[pStrand->rmtChannel].data32[i + offset].val = 0;
  }

  pState->buf_pos += len;

  #if DEBUG_ESP32_DIGITAL_LED_LIB
    snprintf(digitalLeds_debugBuffer, digitalLeds_debugBufferSz,
             "%s ", digitalLeds_debugBuffer);
  #endif

  return;
}

static IRAM_ATTR void handleInterrupt(void *arg)
{
  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

  #if DEBUG_ESP32_DIGITAL_LED_LIB
    snprintf(digitalLeds_debugBuffer, digitalLeds_debugBufferSz,
             "%sRMT.int_st.val = %08x\n", digitalLeds_debugBuffer, RMT.int_st.val);
  #endif

  for (int i = 0; i < localStrandCnt; i++) {
    strand_t * pStrand = &localStrands[i];
    digitalLeds_stateData * pState = static_cast<digitalLeds_stateData*>(pStrand->_stateVars);

    if (RMT.int_st.val & tx_thr_event_offsets[pStrand->rmtChannel])
    {  // tests RMT.int_st.ch<n>_tx_thr_event
      copyToRmtBlock_half(pStrand);
      RMT.int_clr.val |= tx_thr_event_offsets[pStrand->rmtChannel];  // set RMT.int_clr.ch<n>_tx_thr_event
    }
    else if (RMT.int_st.val & tx_end_offsets[pStrand->rmtChannel] && pState->sem)
    {  // tests RMT.int_st.ch<n>_tx_end and semaphore
      xSemaphoreGiveFromISR(pState->sem, &xHigherPriorityTaskWoken);
      RMT.int_clr.val |= tx_end_offsets[pStrand->rmtChannel];  // set RMT.int_clr.ch<n>_tx_end
      if (xHigherPriorityTaskWoken == pdTRUE)
      {
          portYIELD_FROM_ISR();
      }
    }
  }

  return;
}
