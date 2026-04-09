//
//  main.cpp
//  png2ilbm
//
//  Created by Fredrik on 2024-03-11.
//

#include <iostream>
#include <png.h>

#include "arguments.hpp"

#include "core/geometry.hpp"
#include "media/image.hpp"
#include "media/canvas.hpp"


using namespace toybox;

static void handle_help(arguments_t& args);
static void handle_palette(arguments_t& args);
static void handle_masked(arguments_t& args);
static void handle_compressed(arguments_t& args);

static bool save_palette = true;
static bool save_masked = false;
static bool has_masked_idx = false;
static uint8_t masked_idx = 0;
static image_c::compression_type_e compression = image_c::compression_type_e::none;
//static point_s grab_point = {0,0};

const arg_handlers_t arg_handlers {
    {"-h",          {"Show this help and exit.", &handle_help}},
    {"-np",         {"Do not save palette.", [] (arguments_t&) { save_palette = false; }}},
    {"-m",          {"Save with mask (from PNG alpha).", [] (arguments_t&) { save_masked = true; }}},
    {"-mi index",   {"Save with transparent color index.", [] (arguments_t& args) {
        int idx = args.take_int_front(-1);
        if (idx < 0 || idx > 15) {
            printf("Missing or invalid index for -mi.\n");
            exit(-1);
        }
        masked_idx = idx;
        has_masked_idx = true;
    }}},
    {"-c [type]",        {"Save compressed (default RLE).", [] (arguments_t& args) {
        compression = (image_c::compression_type_e)args.take_int_front(1);
    }}},
/*   {"-g x,y",      {"Add grab point.", [] (arguments_t& args) {
        auto split = split_string(args.front(), ',');
        grab_point = {(int16_t)atoi(split[0].c_str()), (int16_t)atoi(split[1].c_str())};
    }}},*/
};

static void handle_help(arguments_t& args) {
    do_print_help("png2ilbm - A utility for converting png images to iff ilbm.\nusage: png2ilbm [options] image.png image.iff", arg_handlers);
    exit(0);
}

static int convert_png_to_ilbm(const std::string &png_file, const std::string &ilbm_file) {
    int16_t width, height;
    png_byte color_type;
    png_byte bit_depth;
    png_bytep *row_pointers = NULL;

    FILE *fp = fopen(png_file.c_str(), "rb");

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png) abort();

    png_infop info = png_create_info_struct(png);
    if (!info) {
        printf("Could not open open png file.");
        exit(-1);
    }
    png_init_io(png, fp);
    png_read_info(png, info);

    width      = png_get_image_width(png, info);
    height     = png_get_image_height(png, info);
    color_type = png_get_color_type(png, info);
    if (color_type != PNG_COLOR_TYPE_PALETTE) {
        printf("Only indexed images supported.\n");
        exit(-1);
    }
    bit_depth  = png_get_bit_depth(png, info);
    if (bit_depth < 8) {
        png_set_packing(png);
    }
    size_t row_bytes = png_get_rowbytes(png,info);
    if (row_bytes != width) {
        printf("Unexpected row bytes.\n");
        row_bytes = width;
    }
    
    int num_palette;
    png_colorp palette;
    if (png_get_PLTE(png, info, &palette, &num_palette) == 0) {
        printf("No palette.\n");
        exit(-1);
    }
    
    palette_c* cgpalette = nullptr;
    if (num_palette > 0 && save_palette) {
        cgpalette = new palette_c((uint8_t*)palette);
    }

    // Build transparency lookup from PNG tRNS chunk for -m mode
    bool is_transparent[256] = {};
    if (save_masked) {
        png_bytep trans_alpha = nullptr;
        int num_trans = 0;
        png_get_tRNS(png, info, &trans_alpha, &num_trans, nullptr);
        for (int i = 0; i < num_trans; i++) {
            is_transparent[i] = trans_alpha[i] < 128;
        }
    }
    image_c cgimage((size_s){width, height}, save_masked, cgpalette);

    png_bytep row = (png_byte*)malloc(row_bytes);
    point_s at;
    bool has_warned = false;
    for(at.y = 0; at.y < height; at.y++) {
        png_read_row(png, row, nullptr);
        for (at.x = 0; at.x < width; at.x++) {
            uint8_t c = row[at.x];
            if (save_masked && is_transparent[c]) {
                cgimage.put_pixel(image_c::MASKED_CIDX, at);
            } else if (c > 15) {
                if (!has_warned) {
                    printf("WARNING: Color index > 15 found and ignored.\n");
                    has_warned = true;
                }
            } else {
                cgimage.put_pixel(c, at);
            }
        }
    }

    // cgimage.set_offset(grab_point);

    if (has_masked_idx) {
        cgimage.save(ilbm_file.c_str(), compression, false, masked_idx);
    } else {
        cgimage.save(ilbm_file.c_str(), compression, save_masked);
    }
    
    return 0;
}

int main(int argc, const char* argv[]) {
    arguments_t args(&argv[1], &argv[argc]);
    if (args.empty()) {
        handle_help(args);
    } else {
        do_handle_args(args, arg_handlers);
        if (args.size() < 1) {
            printf("No input png file.\n");
            exit(-1);
        }
        auto png_file = args.front(); args.pop_front();
        std::string ilbm_file;
        if (args.size() > 0) {
            ilbm_file = args.front();
            args.pop_front();
        } else {
            // Generate output filename by replacing extension with .iff
            ilbm_file = png_file;
            size_t dot_pos = ilbm_file.find_last_of('.');
            if (dot_pos != std::string::npos) {
                ilbm_file = ilbm_file.substr(0, dot_pos);
            }
            ilbm_file += ".iff";
        }
        if (args.size() > 0) {
            printf("%zu extra unknown arguments.\n", args.size());
            exit(-1);
        }
        return convert_png_to_ilbm(png_file, ilbm_file);
    }
    return 0;
}
