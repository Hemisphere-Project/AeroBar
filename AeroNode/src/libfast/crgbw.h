#ifndef __INC_PIXELS_H
#define __INC_PIXELS_H

/*
    _libfast files are freely adapted from FastLED / lib8tion libraries
    Original author and license: FastLED / MIT license 
    Modified by: Thomas BOHL for KXKM / MIT license / 2020
*/

#include "../librmt/esp32_digital_led_lib.h"
#include "math8.h"
#include "hsv2rgb.h"


/// Representation of an RGB pixel (Red, Green, Blue)
struct CRGBW {
	union {
		struct {
            union {
                uint8_t r;
                uint8_t red;
            };
            union {
                uint8_t g;
                uint8_t green;
            };
            union {
                uint8_t b;
                uint8_t blue;
            };
            union {
                uint8_t w;
                uint8_t white;
            };
        };
		uint8_t raw[4];
	};

    /// Array access operator to index into the CRGBW object
	inline uint8_t& operator[] (uint8_t x) __attribute__((always_inline))
    {
        return raw[x];
    }

    /// Array access operator to index into the CRGBW object
    inline const uint8_t& operator[] (uint8_t x) const __attribute__((always_inline))
    {
        return raw[x];
    }

    // default values are UNINITIALIZED
	inline CRGBW() __attribute__((always_inline))
    {
    }

    /// allow construction from R, G, B, W
    inline CRGBW( int ir, int ig, int ib, int iw=0)  __attribute__((always_inline))
        : r((uint8_t)ir), g((uint8_t)ig), b((uint8_t)ib), w((uint8_t)iw)
    {
    }

    /// allow copy construction
	inline CRGBW(const CRGBW& rhs) __attribute__((always_inline))
    {
        r = rhs.r;
        g = rhs.g;
        b = rhs.b;
        w = rhs.w;
    }

    /// allow construction from 32-bit (really 24-bit) 0xRRGGBB color code
	inline CRGBW(uint32_t colorcode) __attribute__((always_inline))
    {
        r = (colorcode >> 16) & 0xFF;
        g = (colorcode >>  8) & 0xFF;
        b = (colorcode >>  0) & 0xFF;
        w = 0;
    }

    /// allow assignment from one RGB struct to another
	inline CRGBW& operator= (const CRGBW& rhs) __attribute__((always_inline))
    {
        r = rhs.r;
        g = rhs.g;
        b = rhs.b;
        w = rhs.w;
        return *this;
    }

    /// allow assignment from R, G, B and W
	inline CRGBW& setRGB (uint8_t nr, uint8_t ng, uint8_t nb, uint8_t nw=0) __attribute__((always_inline))
    {
        r = nr;
        g = ng;
        b = nb;
        w = nw;
        return *this;
    }

    /// allow assignment from just a Hue, saturation and value automatically at max.
	inline CRGBW& setHue (uint8_t hue) __attribute__((always_inline))
    {
        uint8_t rgbw[4];
        hue2rgb_rainbow( hue, rgbw);
        r = rgbw[0];
        g = rgbw[1];
        b = rgbw[2];
        w = rgbw[3];
        return *this;
    }

    /// allow assignment from 32-bit (really 24-bit) 0xRRGGBB color code
	inline CRGBW& setColorCode (uint32_t colorcode) __attribute__((always_inline))
    {
        r = (colorcode >> 16) & 0xFF;
        g = (colorcode >>  8) & 0xFF;
        b = (colorcode >>  0) & 0xFF;
        w = 0;
        return *this;
    }

    // convert CRGBW color into _librmt pixel (applying i*i/255)
    inline void makePixel(pixelColor_t &pixel)
    {
        pixel = pixelFromRGBW(r, g, b, w);
    }

    // convert CRGBW color into _librmt pixel (applying i*i/255)
    inline pixelColor_t getPixel()
    {
        return pixelFromRGBW(r, g, b, w);
    }

    // convert CRGBW color into _librmt pixel (applying i*i/255): implicit cast !
    inline operator pixelColor_t() const 
    { 
        return pixelFromRGBW(r, g, b, w);
    }

    /// add one RGB to another, saturating at 0xFF for each channel
    inline CRGBW& operator+= (const CRGBW& rhs )
    {
        r = qadd8( r, rhs.r);
        g = qadd8( g, rhs.g);
        b = qadd8( b, rhs.b);
        w = qadd8( w, rhs.w);
        return *this;
    }

    /// add a contstant to each channel, saturating at 0xFF
    /// this is NOT an operator+= overload because the compiler
    /// can't usefully decide when it's being passed a 32-bit
    /// constant (e.g. CRGB::Red) and an 8-bit one (CRGB::Blue)
    inline CRGBW& addToRGB (uint8_t d )
    {
        r = qadd8( r, d);
        g = qadd8( g, d);
        b = qadd8( b, d);
        w = qadd8( w, d);
        return *this;
    }

    /// subtract one RGB from another, saturating at 0x00 for each channel
    inline CRGBW& operator-= (const CRGBW& rhs )
    {
        r = qsub8( r, rhs.r);
        g = qsub8( g, rhs.g);
        b = qsub8( b, rhs.b);
        w = qsub8( w, rhs.b);
        return *this;
    }

    /// subtract a constant from each channel, saturating at 0x00
    /// this is NOT an operator+= overload because the compiler
    /// can't usefully decide when it's being passed a 32-bit
    /// constant (e.g. CRGB::Red) and an 8-bit one (CRGB::Blue)
    inline CRGBW& subtractFromRGB(uint8_t d )
    {
        r = qsub8( r, d);
        g = qsub8( g, d);
        b = qsub8( b, d);
        w = qsub8( w, d);
        return *this;
    }

    /// subtract a constant of '1' from each channel, saturating at 0x00
    inline CRGBW& operator-- ()  __attribute__((always_inline))
    {
        subtractFromRGB(1);
        return *this;
    }

    /// subtract a constant of '1' from each channel, saturating at 0x00
    inline CRGBW operator-- (int )  __attribute__((always_inline))
    {
        CRGBW retval(*this);
        --(*this);
        return retval;
    }

    /// add a constant of '1' from each channel, saturating at 0xFF
    inline CRGBW& operator++ ()  __attribute__((always_inline))
    {
        addToRGB(1);
        return *this;
    }

    /// add a constant of '1' from each channel, saturating at 0xFF
    inline CRGBW operator++ (int )  __attribute__((always_inline))
    {
        CRGBW retval(*this);
        ++(*this);
        return retval;
    }

    /// divide each of the channels by a constant
    inline CRGBW& operator/= (uint8_t d )
    {
        r /= d;
        g /= d;
        b /= d;
        w /= d;
        return *this;
    }

    /// right shift each of the channels by a constant
    inline CRGBW& operator>>= (uint8_t d)
    {
      r >>= d;
      g >>= d;
      b >>= d;
      w >>= d;
      return *this;
    }

    /// multiply each of the channels by a constant,
    /// saturating each channel at 0xFF
    inline CRGBW& operator*= (uint8_t d )
    {
        r = qmul8( r, d);
        g = qmul8( g, d);
        b = qmul8( b, d);
        w = qmul8( w, d);
        return *this;
    }

    /// scale down a RGB to N 256ths of it's current brightness, using
    /// 'video' dimming rules, which means that unless the scale factor is ZERO
    /// each channel is guaranteed NOT to dim down to zero.  If it's already
    /// nonzero, it'll stay nonzero, even if that means the hue shifts a little
    /// at low brightness levels.
    inline CRGBW& nscale8_video (uint8_t scaledown )
    {
        nscale8x4_video( r, g, b, w, scaledown);
        return *this;
    }

    /// %= is a synonym for nscale8_video.  Think of it is scaling down
    /// by "a percentage"
    inline CRGBW& operator%= (uint8_t scaledown )
    {
        nscale8x4_video( r, g, b, w, scaledown);
        return *this;
    }

    /// %= is a synonym for nscale8_video.  Think of it is scaling down
    /// by "a percentage"
    inline CRGBW& operator%= (CRGBW scalecolor )
    {
        r = scale8_video(r, scalecolor.r);
        g = scale8_video(g, scalecolor.g);
        b = scale8_video(b, scalecolor.b);
        w = scale8_video(w, scalecolor.w);
        return *this;
    }

    /// fadeLightBy is a synonym for nscale8_video( ..., 255-fadefactor)
    inline CRGBW& fadeLightBy (uint8_t fadefactor )
    {
        nscale8x4_video( r, g, b, w, 255 - fadefactor);
        return *this;
    }

    /// scale down a RGB to N 256ths of it's current brightness, using
    /// 'plain math' dimming rules, which means that if the low light levels
    /// may dim all the way to 100% black.
    inline CRGBW& nscale8 (uint8_t scaledown )
    {
        nscale8x4( r, g, b, w, scaledown);
        return *this;
    }

    /// scale down a RGB to N 256ths of it's current brightness, using
    /// 'plain math' dimming rules, which means that if the low light levels
    /// may dim all the way to 100% black.
    inline CRGBW& nscale8 (const CRGBW & scaledown )
    {
        r = ::scale8(r, scaledown.r);
        g = ::scale8(g, scaledown.g);
        b = ::scale8(b, scaledown.b);
        w = ::scale8(w, scaledown.w);
        return *this;
    }

    /// return a CRGBW object that is a scaled down version of this object
    inline CRGBW scale8 (const CRGBW & scaledown ) const
    {
        CRGBW out;
        out.r = ::scale8(r, scaledown.r);
        out.g = ::scale8(g, scaledown.g);
        out.b = ::scale8(b, scaledown.b);
        out.w = ::scale8(w, scaledown.w);
        return out;
    }

    /// fadeToBlackBy is a synonym for nscale8( ..., 255-fadefactor)
    inline CRGBW& fadeToBlackBy (uint8_t fadefactor )
    {
        nscale8x4( r, g, b, w, 255 - fadefactor);
        return *this;
    }

    /// "or" operator brings each channel up to the higher of the two values
    inline CRGBW& operator|= (const CRGBW& rhs )
    {
        if( rhs.r > r) r = rhs.r;
        if( rhs.g > g) g = rhs.g;
        if( rhs.b > b) b = rhs.b;
        if( rhs.w > w) w = rhs.w;
        return *this;
    }

    /// "or" operator brings each channel up to the higher of the two values
    inline CRGBW& operator|= (uint8_t d )
    {
        if( d > r) r = d;
        if( d > g) g = d;
        if( d > b) b = d;
        if( d > w) w = d;
        return *this;
    }

    /// "and" operator brings each channel down to the lower of the two values
    inline CRGBW& operator&= (const CRGBW& rhs )
    {
        if( rhs.r < r) r = rhs.r;
        if( rhs.g < g) g = rhs.g;
        if( rhs.b < b) b = rhs.b;
        if( rhs.w < w) w = rhs.w;
        return *this;
    }

    /// "and" operator brings each channel down to the lower of the two values
    inline CRGBW& operator&= (uint8_t d )
    {
        if( d < r) r = d;
        if( d < g) g = d;
        if( d < b) b = d;
        if( d < w) w = d;
        return *this;
    }

    /// this allows testing a CRGBW for zero-ness
    inline operator bool() const __attribute__((always_inline))
    {
        return r || g || b || w;
    }

    /// invert each channel
    inline CRGBW operator- ()
    {
        CRGBW retval;
        retval.r = 255 - r;
        retval.g = 255 - g;
        retval.b = 255 - b;
        retval.w = 255 - w;
        return retval;
    }


    /// maximize the brightness of this CRGBW object
    inline void maximizeBrightness( uint8_t limit = 255 )  {
        uint8_t max = red;
        if( green > max) max = green;
        if( blue > max) max = blue;
        if( white > max) max = white;

        // stop div/0 when color is black
        if(max > 0) {
            uint16_t factor = ((uint16_t)(limit) * 256) / max;
            red =   (red   * factor) / 256;
            green = (green * factor) / 256;
            blue =  (blue  * factor) / 256;
            white =  (white  * factor) / 256;
        }
    }

    /// return a new CRGBW object after performing a linear interpolation between this object and the passed in object
    inline CRGBW lerp8( const CRGBW& other, fract8 frac) const
    {
      CRGBW ret;

      ret.r = lerp8by8(r,other.r,frac);
      ret.g = lerp8by8(g,other.g,frac);
      ret.b = lerp8by8(b,other.b,frac);
      ret.w = lerp8by8(w,other.w,frac);

      return ret;
    }

    /// return a new CRGBW object after performing a linear interpolation between this object and the passed in object
    inline CRGBW lerp16( const CRGBW& other, fract16 frac) const
    {
      CRGBW ret;

      ret.r = lerp16by16(r<<8,other.r<<8,frac)>>8;
      ret.g = lerp16by16(g<<8,other.g<<8,frac)>>8;
      ret.b = lerp16by16(b<<8,other.b<<8,frac)>>8;
      ret.w = lerp16by16(w<<8,other.w<<8,frac)>>8;

      return ret;
    }

    /// Predefined RGB colors
    typedef enum {
        AliceBlue=0xF0F8FF,
        Amethyst=0x9966CC,
        AntiqueWhite=0xFAEBD7,
        Aqua=0x00FFFF,
        Aquamarine=0x7FFFD4,
        Azure=0xF0FFFF,
        Beige=0xF5F5DC,
        Bisque=0xFFE4C4,
        Black=0x000000,
        BlanchedAlmond=0xFFEBCD,
        Blue=0x0000FF,
        BlueViolet=0x8A2BE2,
        Brown=0xA52A2A,
        BurlyWood=0xDEB887,
        CadetBlue=0x5F9EA0,
        Chartreuse=0xB2F000,
        Chocolate=0xD2691E,
        Coral=0xFF7F50,
        CornflowerBlue=0x6495ED,
        Cornsilk=0xFFF8DC,
        Crimson=0xDC143C,
        Cyan=0x00FFFF,
        DarkBlue=0x00008B,
        DarkCyan=0x008B8B,
        DarkGoldenrod=0xB8860B,
        DarkGray=0xA9A9A9,
        DarkGrey=0xA9A9A9,
        DarkGreen=0x006400,
        DarkKhaki=0xBDB76B,
        DarkMagenta=0x8B008B,
        DarkOliveGreen=0x556B2F,
        DarkOrange=0xFF8C00,
        DarkOrchid=0x9932CC,
        DarkRed=0x8B0000,
        DarkSalmon=0xE9967A,
        DarkSeaGreen=0x8FBC8F,
        DarkSlateBlue=0x483D8B,
        DarkSlateGray=0x2F4F4F,
        DarkSlateGrey=0x2F4F4F,
        DarkTurquoise=0x00CED1,
        DarkViolet=0x9400D3,
        DeepPink=0xFF1493,
        DeepSkyBlue=0x00BFFF,
        DimGray=0x696969,
        DimGrey=0x696969,
        DodgerBlue=0x1E90FF,
        FireBrick=0xB22222,
        FloralWhite=0xFFFAF0,
        ForestGreen=0x228B22,
        Fuchsia=0xFF00FF,
        Gainsboro=0xDCDCDC,
        GhostWhite=0xF8F8FF,
        Gold=0xFFC900,
        Goldenrod=0xDAA520,
        Gray=0x808080,
        Grey=0x808080,
        Green=0x00F000,
        GreenYellow=0xADFF2F,
        Honeydew=0xF0FFF0,
        HotPink=0xFF69B4,
        IndianRed=0xCD5C5C,
        Indigo=0x4B0082,
        Ivory=0xFFFFF0,
        Khaki=0xF0E68C,
        Lavender=0xE6E6FA,
        LavenderBlush=0xFFF0F5,
        LawnGreen=0x7CFC00,
        LemonChiffon=0xFFFACD,
        LightBlue=0xADD8E6,
        LightCoral=0xF08080,
        LightCyan=0xE0FFFF,
        LightGoldenrodYellow=0xFAFAD2,
        LightGreen=0x90EE90,
        LightGrey=0xD3D3D3,
        LightPink=0xFFB6C1,
        LightSalmon=0xFFA07A,
        LightSeaGreen=0x20B2AA,
        LightSkyBlue=0x87CEFA,
        LightSlateGray=0x778899,
        LightSlateGrey=0x778899,
        LightSteelBlue=0xB0C4DE,
        LightYellow=0xFFFFE0,
        Lime=0x00FF00,
        LimeGreen=0x32CD32,
        Linen=0xFAF0E6,
        Magenta=0xFF00FF,
        Maroon=0x800000,
        MediumAquamarine=0x66CDAA,
        MediumBlue=0x0000CD,
        MediumOrchid=0xBA55D3,
        MediumPurple=0x9370DB,
        MediumSeaGreen=0x3CB371,
        MediumSlateBlue=0x7B68EE,
        MediumSpringGreen=0x00FA9A,
        MediumTurquoise=0x48D1CC,
        MediumVioletRed=0xC71585,
        MidnightBlue=0x191970,
        MintCream=0xF5FFFA,
        MistyRose=0xFFE4E1,
        Moccasin=0xFFE4B5,
        NavajoWhite=0xFFDEAD,
        Navy=0x000080,
        OldLace=0xFDF5E6,
        Olive=0x808000,
        OliveDrab=0x6B8E23,
        Orange=0xFFA500,
        OrangeRed=0xFF4500,
        Orchid=0xDA70D6,
        PaleGoldenrod=0xEEE8AA,
        PaleGreen=0x98FB98,
        PaleTurquoise=0xAFEEEE,
        PaleVioletRed=0xDB7093,
        PapayaWhip=0xFFEFD5,
        PeachPuff=0xFFDAB9,
        Peru=0xCD853F,
        Pink=0xFFC0CB,
        Plaid=0xCC5533,
        Plum=0xDDA0DD,
        PowderBlue=0xB0E0E6,
        Purple=0x800080,
        Red=0xFF0000,
        RosyBrown=0xBC8F8F,
        RoyalBlue=0x4169E1,
        SaddleBrown=0x8B4513,
        Salmon=0xFA8072,
        SandyBrown=0xF4A460,
        SeaGreen=0x2E8B57,
        Seashell=0xFFF5EE,
        Sienna=0xA0522D,
        Silver=0xC0C0C0,
        SkyBlue=0x87CEEB,
        SlateBlue=0x6A5ACD,
        SlateGray=0x708090,
        SlateGrey=0x708090,
        Snow=0xFFFAFA,
        SpringGreen=0x00FF7F,
        SteelBlue=0x4682B4,
        Tan=0xD2B48C,
        Teal=0x008080,
        Thistle=0xD8BFD8,
        Tomato=0xFF6347,
        Turquoise=0x40E0D0,
        Violet=0xEE82EE,
        Wheat=0xF5DEB3,
        White=0xFFFFFF,
        WhiteSmoke=0xF5F5F5,
        Yellow=0xFFFF00,
        YellowGreen=0x9ACD32,

        // LED RGB color that roughly approximates
        // the color of incandescent fairy lights,
        // assuming that you're using FastLED
        // color correction on your LEDs (recommended).
        FairyLight=0xFFE42D,
        // If you are using no color correction, use this
        FairyLightNCC=0xFF9D2A

    } HTMLColorCode;
};


inline __attribute__((always_inline)) bool operator== (const CRGBW& lhs, const CRGBW& rhs)
{
    return (lhs.r == rhs.r) && (lhs.g == rhs.g) && (lhs.b == rhs.b) && (lhs.w == rhs.w);
}

inline __attribute__((always_inline)) bool operator!= (const CRGBW& lhs, const CRGBW& rhs)
{
    return !(lhs == rhs);
}

inline __attribute__((always_inline)) bool operator< (const CRGBW& lhs, const CRGBW& rhs)
{
    uint16_t sl, sr;
    sl = lhs.r + lhs.g + lhs.b + lhs.w;
    sr = rhs.r + rhs.g + rhs.b + rhs.w;
    return sl < sr;
}

inline __attribute__((always_inline)) bool operator> (const CRGBW& lhs, const CRGBW& rhs)
{
    uint16_t sl, sr;
    sl = lhs.r + lhs.g + lhs.b + lhs.w;
    sr = rhs.r + rhs.g + rhs.b + rhs.w;
    return sl > sr;
}

inline __attribute__((always_inline)) bool operator>= (const CRGBW& lhs, const CRGBW& rhs)
{
    uint16_t sl, sr;
    sl = lhs.r + lhs.g + lhs.b + lhs.w;
    sr = rhs.r + rhs.g + rhs.b + rhs.w;
    return sl >= sr;
}

inline __attribute__((always_inline)) bool operator<= (const CRGBW& lhs, const CRGBW& rhs)
{
    uint16_t sl, sr;
    sl = lhs.r + lhs.g + lhs.b + lhs.w;
    sr = rhs.r + rhs.g + rhs.b + rhs.w;
    return sl <= sr;
}


__attribute__((always_inline))
inline CRGBW operator+( const CRGBW& p1, const CRGBW& p2)
{
    return CRGBW( qadd8( p1.r, p2.r),
                 qadd8( p1.g, p2.g),
                 qadd8( p1.b, p2.b),
                 qadd8( p1.w, p2.w));
}

__attribute__((always_inline))
inline CRGBW operator-( const CRGBW& p1, const CRGBW& p2)
{
    return CRGBW( qsub8( p1.r, p2.r),
                 qsub8( p1.g, p2.g),
                 qsub8( p1.b, p2.b),
                 qsub8( p1.w, p2.w));
}

__attribute__((always_inline))
inline CRGBW operator*( const CRGBW& p1, uint8_t d)
{
    return CRGBW( qmul8( p1.r, d),
                 qmul8( p1.g, d),
                 qmul8( p1.b, d),
                 qmul8( p1.w, d));
}

__attribute__((always_inline))
inline CRGBW operator/( const CRGBW& p1, uint8_t d)
{
    return CRGBW( p1.r/d, p1.g/d, p1.b/d, p1.w/d);
}


__attribute__((always_inline))
inline CRGBW operator&( const CRGBW& p1, const CRGBW& p2)
{
    return CRGBW( p1.r < p2.r ? p1.r : p2.r,
                 p1.g < p2.g ? p1.g : p2.g,
                 p1.b < p2.b ? p1.b : p2.b,
                 p1.w < p2.w ? p1.w : p2.w);
}

__attribute__((always_inline))
inline CRGBW operator|( const CRGBW& p1, const CRGBW& p2)
{
    return CRGBW( p1.r > p2.r ? p1.r : p2.r,
                 p1.g > p2.g ? p1.g : p2.g,
                 p1.b > p2.b ? p1.b : p2.b,
                 p1.w > p2.w ? p1.w : p2.w);
}

__attribute__((always_inline))
inline CRGBW operator%( const CRGBW& p1, uint8_t d)
{
    CRGBW retval( p1);
    retval.nscale8_video( d);
    return retval;
}

__attribute__((always_inline))
inline CRGBW operator%( const CRGBW& p1, CRGBW p2)
{
    CRGBW retval(p1);
    retval.r = scale8_video(retval.r, p2.r);
    retval.g = scale8_video(retval.g, p2.g);
    retval.b = scale8_video(retval.b, p2.b);
    retval.w = scale8_video(retval.w, p2.w);
    return retval;
}



/// RGB orderings, used when instantiating controllers to determine what
/// order the controller should send RGB data out in, RGB being the default
/// ordering.
enum EOrder {
	RGB=0012,
	RBG=0021,
	GRB=0102,
	GBR=0120,
	BRG=0201,
	BGR=0210
};



#endif