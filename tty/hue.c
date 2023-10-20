#include <include/types.h>
#include <tty/include/hue.h>
#include <include/math.h>

//convert from HSV (hue, saturation, value) to RGB
void hsv_to_rgb(uint16_t h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b) {
    //h %= 360; //make sure hsv is in the 0-359 range
    if (h > 359) {h %= 360;}
    if (s > 1) {s = 1;}
    if (v > 1) {v = 1;}

    float c = v * s;
    float x = c * (1 - abs(fmod(((float) h / 60), 2) - 1));
    float m = v - c;
    float _r, _g, _b;
    
    if (h >= 0 && h < 60) {
        _r = c;
        _g = x;
        _b = 0;
    } else if (h >= 60 && h < 120) {
        _r = x;
        _g = c;
        _b = 0;
    } else if (h >= 120 && h < 180) {
        _r = 0;
        _g = c;
        _b = x;
    } else if (h >= 180 && h < 240) {
        _r = 0;
        _g = x;
        _b = c;
    } else if (h >= 240 && h < 300) {
        _r = x;
        _g = 0;
        _b = c;
    } else {
        _r = c;
        _g = 0;
        _b = x;
    }

    *r = (uint8_t)((_r + m) * 255.0);
    *g = (uint8_t)((_g + m) * 255.0);
    *b = (uint8_t)((_b + m) * 255.0);
}