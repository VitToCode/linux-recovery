/*
 *  Copyright (C) 2016, Zhang YanMing <jamincheung@126.com>
 *
 *  Linux recovery updater
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <string.h>

#include <utils/log.h>
#include <utils/common.h>
#include <utils/assert.h>
#include <graphics/gr_drawer.h>
#include <graphics/font_10x18.h>
#include <fb/fb_manager.h>

#define LOG_TAG "gr_drawer"

struct gr_font {
    uint32_t cwidth;
    uint32_t cheight;
    struct gr_surface* texture;
};

static struct gr_font* gr_font;
static struct fb_manager* fb_manager;
static uint32_t fb_width;
static uint32_t fb_height;
static uint32_t bits_per_pixel;
static uint32_t bytes_per_pixel;

static uint8_t gr_current_r = 255;
static uint8_t gr_current_g = 255;
static uint8_t gr_current_b = 255;
static uint8_t gr_current_a = 255;

static int outside(uint32_t pos_x, uint32_t pos_y) {
    if ((pos_y >= fb_height) || (pos_x >= fb_width))
        return 1;

    return 0;
}

static uint32_t  make_pixel(uint8_t red, uint8_t green, uint8_t blue,
        uint8_t alpha) {
    uint32_t redbit_len = fb_manager->get_redbit_length(fb_manager);
    uint32_t redbit_off = fb_manager->get_redbit_offset(fb_manager);

    uint32_t greenbit_len = fb_manager->get_greenbit_length(fb_manager);
    uint32_t greenbit_off = fb_manager->get_greenbit_offset(fb_manager);

    uint32_t bluebit_len = fb_manager->get_bluebit_length(fb_manager);
    uint32_t bluebit_off = fb_manager->get_bluebit_offset(fb_manager);

    uint32_t alphabit_len = fb_manager->get_alphabit_length(fb_manager);
    uint32_t alphabit_off = fb_manager->get_alphabit_offset(fb_manager);

    uint32_t pixel = (uint32_t)(((red >> (8 - redbit_len)) << redbit_off)
            | ((green >> (8 - greenbit_len)) << greenbit_off)
            | ((blue >> (8 - bluebit_len)) << bluebit_off)
            | ((alpha >> (8 - alphabit_len)) << alphabit_off));

    return pixel;
}

static int draw_png(struct gr_drawer* this, struct gr_surface* surface,
            uint32_t pos_x, uint32_t pos_y) {
    if (outside(pos_x, pos_y)) {
        LOGE("Position out bound of screen\n");
        return -1;
    }

    uint8_t *buf = (uint8_t *) fb_manager->fbmem;

    for (int i = 0; i < surface->height; i++) {
        if ((i >= fb_height) || (i + pos_y > fb_height))
            break;

        for (int j = 0; j < surface->width; j++) {
            if ((j >= fb_width) || (j + pos_x > fb_width))
                break;

            uint8_t red = surface->raw_data[4 * (j + surface->width * i)];
            uint8_t green = surface->raw_data[4 * (j + surface->width * i) + 1];
            uint8_t blue = surface->raw_data[4 * (j + surface->width * i) + 2];
            uint8_t alpha = surface->raw_data[4 * (j + surface->width * i) + 3];

            uint32_t pos = (i + pos_y ) * fb_width + j + pos_x;
            uint32_t pixel = make_pixel(red, green, blue, alpha);

            for (int x = 0; x < bytes_per_pixel; x++)
                buf[bytes_per_pixel * pos + x] = pixel >> (bits_per_pixel -
                        (bytes_per_pixel -x) * 8);

        }
    }

    fb_manager->display(fb_manager);

    return 0;
}

static int blank(struct gr_drawer* this, uint8_t blank) {
    return fb_manager->blank(fb_manager, blank);
}

static void set_color(struct gr_drawer* this, uint8_t red,
        uint8_t green, uint8_t blue, uint8_t alpha) {

    gr_current_r = red;
    gr_current_g = green;
    gr_current_b = blue;
    gr_current_a = alpha;
}

static void fill_screen(struct gr_drawer* this) {
    uint8_t *buf = (uint8_t *) fb_manager->fbmem;

    uint32_t pixel = make_pixel(gr_current_r, gr_current_g, gr_current_b, 0);

    for (int i = 0; i < fb_height; i++) {
        for (int j = 0; j < fb_width; j++) {
            uint32_t pos = i * fb_width + j;

            for (int x = 0; x < bytes_per_pixel; x++)
                buf[bytes_per_pixel * pos + x] = pixel >> (bits_per_pixel -
                        (bytes_per_pixel -x) * 8);
        }
    }

    fb_manager->display(fb_manager);
}

static void text_blend(uint8_t* src_p, int src_row_bytes, uint8_t* dst_p,
        int dst_row_bytes, int width, int height) {

    for (int j = 0; j < height; ++j) {
        uint8_t* sx = src_p;
        uint8_t* px = dst_p;

        for (int i = 0; i < width; ++i) {
            uint8_t a = *sx++;

            if (gr_current_a < 255)
                a = ((int)a * gr_current_a) / 255;

            if (a == 255) {
                *px++ = gr_current_r;
                *px++ = gr_current_g;
                *px++ = gr_current_b;
                px++;

            } else if (a > 0) {
                *px = (*px * (255-a) + gr_current_r * a) / 255;
                ++px;
                *px = (*px * (255-a) + gr_current_g * a) / 255;
                ++px;
                *px = (*px * (255-a) + gr_current_b * a) / 255;
                ++px;
                ++px;

            } else {
                px += 4;
            }
        }

        src_p += src_row_bytes;
        dst_p += dst_row_bytes;
    }
}

static int draw_text(struct gr_drawer* this, uint32_t pos_x, uint32_t pos_y,
        const char* text, uint8_t bold) {
    assert_die_if(text == NULL, "text is NULL\n");

    uint8_t *buf = (uint8_t *) fb_manager->fbmem;

    struct gr_font *font = gr_font;
    uint32_t off;

    bold = bold && (font->texture->height != font->cheight);
    while((off = *text++)) {
        off -= 32;
        if (outside(pos_x, pos_y) ||
                outside(pos_x + font->cwidth-1, pos_y + font->cheight - 1))
            return -1;

        if (off < 96) {

            uint8_t* src_p = font->texture->raw_data + (off * font->cwidth) +
                (bold ? font->cheight * font->texture->row_bytes : 0);

            uint8_t* dst_p = buf + pos_y * fb_manager->get_row_bytes(fb_manager)
                    + pos_x * bytes_per_pixel;

            text_blend(src_p, font->texture->row_bytes,
                       dst_p, fb_manager->get_row_bytes(fb_manager),
                       font->cwidth, font->cheight);

        }

        pos_x += font->cwidth;
    }

    fb_manager->display(fb_manager);

    return 0;
}

static int init(struct gr_drawer* this) {
    fb_manager = _new(struct fb_manager, fb_manager);
    fb_manager->init(fb_manager);

    fb_width = fb_manager->get_screen_width(fb_manager);
    fb_height = fb_manager->get_screen_height(fb_manager);
    bits_per_pixel = fb_manager->get_bits_per_pixel(fb_manager);
    bytes_per_pixel = bits_per_pixel / 8;


    gr_font = calloc(1, sizeof(struct gr_font));

    gr_font->texture = calloc(1, sizeof(*gr_font->texture));
    gr_font->texture->width = font.width;
    gr_font->texture->height = font.height;
    gr_font->texture->row_bytes = font.width;
    gr_font->texture->pixel_bytes = 1;

    uint8_t* bits = malloc(font.width * font.height);
    gr_font->texture->raw_data = (void *) bits;

    uint8_t data;
    uint8_t* in = font.rundata;
    while ((data = *in++)) {
        memset(bits, (data & 0x80) ? 0xff : 0, data & 0x7f);
        bits += (data & 0x7f);
    }

    gr_font->cwidth = font.cwidth;
    gr_font->cheight = font.cheight;

    return 0;
}

static int deinit(struct gr_drawer* this) {
    fb_manager->deinit(fb_manager);
    _delete(fb_manager);

    free(gr_font->texture);
    free(gr_font);

    fb_manager = NULL;
    gr_font = NULL;

    return 0;
}

void construct_gr_drawer(struct gr_drawer* this) {
    this->draw_png = draw_png;
    this->draw_text = draw_text;
    this->blank = blank;
    this->fill_screen = fill_screen;
    this->init = init;
    this->deinit = deinit;
    this->set_color = set_color;
}

void destruct_gr_drawer(struct gr_drawer* this) {
    this->draw_png = NULL;
    this->draw_text = NULL;
    this->blank = NULL;
    this->fill_screen = NULL;
    this->init = NULL;
    this->deinit = NULL;
    this->set_color = NULL;
}
