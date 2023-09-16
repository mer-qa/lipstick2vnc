#include <stdio.h>
#include "../src/pointer_finger.h"
#include "../src/pointer_finger_touch.h"
#include "../src/empty_mouse.h"
#include "./libattopng/libattopng.c"

#define RGBA(r, g, b, a) ((r) | ((g) << 8) | ((b) << 16) | ((a) << 24))

void write_png_rgba(int width, int height, unsigned char* px, const char* fname);

int main(int argc, char *argv[])
{
    write_png_rgba(pointer_finger.width, pointer_finger.height,
      pointer_finger.pixel_data, "cursor_pointer.png");

    write_png_rgba(pointer_finger_touch.width, pointer_finger_touch.height,
      pointer_finger_touch.pixel_data, "cursor_pointer_touch.png");

    write_png_rgba(empty_mouse.width, empty_mouse.height,
      empty_mouse.pixel_data, "cursor_empty.png");

    return 0;
}

void write_png_rgba(int width, int height, unsigned char* px, const char* fname)
{
    int bpp = 4;
    libattopng_t* png = libattopng_new(width, height, PNG_RGBA);

    for(int x=0; x<width; x++) {
        for(int y=0; y<height; y++) {
            int r = px[x*bpp + y*width*bpp + 0];
            int g = px[x*bpp + y*width*bpp + 1];
            int b = px[x*bpp + y*width*bpp + 2];
            int a = px[x*bpp + y*width*bpp + 3];
            libattopng_set_pixel(png, x, y, RGBA(r, g, b, a));
        }
    }

    libattopng_save(png, fname);
    libattopng_destroy(png);
}
