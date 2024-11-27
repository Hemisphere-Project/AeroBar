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


#ifndef ESP32_DIGITAL_LED_LIB_H
#define ESP32_DIGITAL_LED_LIB_H

#include "libfast/pixel.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define DEBUG_ESP32_DIGITAL_LED_LIB 0

  typedef struct
  {
    int rmtChannel;
    int gpioNum;
    int ledType;
    int brightLimit;
    int numPixels;
    pixelColor_t *pixels;
    void *_stateVars;
  } strand_t;

  enum led_order
  {
    S_RGB,
    S_RBG,
    S_GRB,
    S_GBR,
    S_BRG,
    S_BGR
  };

  typedef struct
  {
    int bytesPerPixel;
    led_order ledOrder;
    uint32_t T0H;
    uint32_t T1H;
    uint32_t T0L;
    uint32_t T1L;
    uint32_t TRS;
  } ledParams_t;

  enum led_types
  {
    LED_WS2812_V1,
    LED_WS2812B_V1,
    LED_WS2812B_V2,
    LED_WS2812B_V3,
    LED_WS2813_V1,
    LED_WS2813_V2,
    LED_WS2813_V3,
    LED_WS2813_V4,
    LED_WS2815_V1,
    LED_SK6812_V1,
    LED_SK6812W_V1,
    LED_SK6812W_V3,
    LED_TM1934,
  };

  const ledParams_t ledParamsAll[] = {
      // Still must match order of `led_types`
      [LED_WS2812_V1]  = {.bytesPerPixel = 3, .ledOrder = S_GRB, .T0H = 350, .T1H = 700, .T0L = 800, .T1L = 600, .TRS = 50000},
      [LED_WS2812B_V1] = {.bytesPerPixel = 3, .ledOrder = S_GRB, .T0H = 350, .T1H = 900, .T0L = 900, .T1L = 350, .TRS = 50000}, // Older datasheet
      [LED_WS2812B_V2] = {.bytesPerPixel = 3, .ledOrder = S_GRB, .T0H = 400, .T1H = 850, .T0L = 850, .T1L = 400, .TRS = 50000}, // 2016 datasheet
      [LED_WS2812B_V3] = {.bytesPerPixel = 3, .ledOrder = S_GRB, .T0H = 450, .T1H = 850, .T0L = 850, .T1L = 450, .TRS = 50000}, // cplcpu test
      [LED_WS2813_V1]  = {.bytesPerPixel = 3, .ledOrder = S_GRB, .T0H = 350, .T1H = 800, .T0L = 350, .T1L = 350, .TRS = 300000}, // Older datasheet
      [LED_WS2813_V2]  = {.bytesPerPixel = 3, .ledOrder = S_GRB, .T0H = 270, .T1H = 800, .T0L = 800, .T1L = 270, .TRS = 300000}, // 2016 datasheet
      [LED_WS2813_V3]  = {.bytesPerPixel = 3, .ledOrder = S_GRB, .T0H = 270, .T1H = 630, .T0L = 630, .T1L = 270, .TRS = 300000}, // 2017-05 WS datasheet
      [LED_WS2813_V4]  = {.bytesPerPixel = 3, .ledOrder = S_GRB, .T0H = 220, .T1H = 580, .T0L = 580, .T1L = 220, .TRS = 300000}, // 2018-12 WS datasheet
      [LED_WS2815_V1]  = {.bytesPerPixel = 3, .ledOrder = S_RGB, .T0H = 240, .T1H = 750, .T0L = 750, .T1L = 240, .TRS = 300100}, // 
      [LED_SK6812_V1]  = {.bytesPerPixel = 3, .ledOrder = S_GRB, .T0H = 300, .T1H = 600, .T0L = 900, .T1L = 600, .TRS = 80000},  // R V B
      [LED_SK6812W_V1] = {.bytesPerPixel = 4, .ledOrder = S_GRB, .T0H = 300, .T1H = 600, .T0L = 900, .T1L = 600, .TRS = 80000}, // R V B W
      [LED_SK6812W_V3] = {.bytesPerPixel = 4, .ledOrder = S_GRB, .T0H = 350, .T1H = 700, .T0L = 800, .T1L = 600, .TRS = 50000}, // R V B W
      [LED_TM1934]     = {.bytesPerPixel = 3, .ledOrder = S_GRB, .T0H = 560, .T1H = 480, .T0L = 280, .T1L = 640, .TRS = 48000}, // W W W
};

  
  extern int digitalLeds_init();
  extern strand_t* digitalLeds_addStrand(strand_t strands);
  extern int digitalLeds_updatePixels(strand_t *strand);
  extern void digitalLeds_resetPixels(strand_t *pStrand);
  
  

#ifdef __cplusplus
}
#endif

#endif /* ESP32_DIGITAL_LED_LIB_H */
