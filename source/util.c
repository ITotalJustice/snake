#include <stdint.h>

#include "util.h"

const rect_t map_rect(const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h)
{
    const rect_t rect = { .x = x, .y = y, .w = w, .h = h };
    return rect;
}

const colour_t map_rgba(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a)
{
    const colour_t colour = { .r = r, .g = g, .b = b, .a = a };
    return colour;
}

const colour_t map_rgb(const uint8_t r, const uint8_t g, const uint8_t b)
{
    return map_rgba(r,g,b,255);
}


const rectf_t map_rectf(const float x, const float y, const float w, const float h)
{
    const rectf_t rect = { .x = x, .y = y, .w = w, .h = h };
    return rect;
}

const colourf_t map_rgbaf(const float r, const float g, const float b, const float a)
{
    const colourf_t colour = { .r = r, .g = g, .b = b, .a = a };
    return colour;
}

const colourf_t map_rgbf(const float r, const float g, const float b)
{
    return map_rgbaf(r,g,b,255);
}