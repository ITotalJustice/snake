#pragma once

#include <stdint.h>

typedef struct
{
    uint8_t r; uint8_t g; uint8_t b; uint8_t a;
} colour_t;

typedef struct
{
    float r; float g; float b; float a;
} colourf_t;

typedef struct
{
    uint32_t x; uint32_t y; uint32_t w; uint32_t h;
} rect_t;

typedef struct
{
    float x; float y; float w; float h;
} rectf_t;

const rect_t map_rect(const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h);
const colour_t map_rgba(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a);
const colour_t map_rgb(const uint8_t r, const uint8_t g, const uint8_t b);

const rectf_t map_rectf(const float x, const float y, const float w, const float h);
const colourf_t map_rgbaf(const float r, const float g, const float b, const float a);
const colourf_t map_rgbf(const float r, const float g, const float b);