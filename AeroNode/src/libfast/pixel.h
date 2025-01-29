
#ifndef PIXEL_H
#define PIXEL_H

#ifdef __cplusplus
extern "C"
{
#endif

#define LED_CLAMP 256

// MIN macro
#define MIN(a,b) ((a)<(b)?(a):(b))

#include <stdint.h>

typedef union { 
    struct __attribute__((packed))
    {
    uint8_t r, g, b, w;
    };
    uint32_t num;
} pixelColor_t;

inline pixelColor_t clamp(pixelColor_t v)
{
    if (v.r > LED_CLAMP || v.g > LED_CLAMP || v.b > LED_CLAMP || v.w > LED_CLAMP) {
        float ratio = 1.0;
        if (v.r > LED_CLAMP) ratio = (float)LED_CLAMP / v.r;
        if (v.g > LED_CLAMP) ratio = MIN(ratio, (float)LED_CLAMP / v.g);
        if (v.b > LED_CLAMP) ratio = MIN(ratio, (float)LED_CLAMP / v.b);
        if (v.w > LED_CLAMP) ratio = MIN(ratio, (float)LED_CLAMP / v.w);
        v.r *= ratio;
        v.g *= ratio;
        v.b *= ratio;
        v.w *= ratio;
    }
    return v;
}

inline pixelColor_t pixelFromRGB(uint8_t r, uint8_t g, uint8_t b)
{
    pixelColor_t v;
    v.r = r;
    v.g = g;
    v.b = b;
    v.w = 0;
    return clamp(v);
}

inline pixelColor_t pixelFromRGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    pixelColor_t v;
    v.r = r;
    v.g = g;
    v.b = b;
    v.w = w;
    return clamp(v);
}

inline pixelColor_t pixelFromRGBtoW(uint8_t r, uint8_t g, uint8_t b)
{   
    uint8_t w = MIN(r, MIN(g, b));

    pixelColor_t v;
    v.r = r-w;
    v.g = g-w;
    v.b = b-w;
    v.w = w;
    return clamp(v);
}

#ifdef __cplusplus
}
#endif

#endif