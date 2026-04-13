//
//  main.cpp
//  png2ilbm
//
//  Created by Fredrik on 2024-03-11.
//

#include <iostream>
#include <expected>
#include <png.h>

#include "arguments.hpp"

#include "core/geometry.hpp"
#include "media/image.hpp"
#include "media/canvas.hpp"
#include "media/tileset.hpp"


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
static bool has_tileset = false;
static size_s tileset_tile_size = {0, 0};
static std::vector<rect_s> tileset_rects;

using int_pair = std::pair<int, int>;
using group_pair = std::pair<int_pair, int_pair>;

// Parse "{x,y}" from str at pos, advancing pos past '}'.
static std::expected<int_pair, std::string> try_parse_pair(const std::string& str, size_t& pos) {
    if (pos >= str.size() || str[pos] != '{')
        return std::unexpected("Expected '{' at position " + std::to_string(pos));
    pos++;
    int a = atoi(str.c_str() + pos);
    auto comma = str.find(',', pos);
    if (comma == std::string::npos)
        return std::unexpected("Expected ',' after first value at position " + std::to_string(pos));
    pos = comma + 1;
    int b = atoi(str.c_str() + pos);
    auto close = str.find('}', pos);
    if (close == std::string::npos)
        return std::unexpected("Expected '}' after second value at position " + std::to_string(pos));
    pos = close + 1;
    return int_pair{a, b};
}

// Parse "{{x,y},{u,v}}" from str at pos.
static std::expected<group_pair, std::string> try_parse_group(const std::string& str, size_t& pos) {
    if (pos >= str.size() || str[pos] != '{')
        return std::unexpected("Expected '{' at position " + std::to_string(pos));
    pos++;
    auto first = try_parse_pair(str, pos);
    if (!first) return std::unexpected(first.error());
    if (pos < str.size() && str[pos] == ',') pos++;
    auto second = try_parse_pair(str, pos);
    if (!second) return std::unexpected(second.error());
    if (pos >= str.size() || str[pos] != '}')
        return std::unexpected("Expected '}' closing group at position " + std::to_string(pos));
    pos++;
    return group_pair{*first, *second};
}

// Parse "{{x,y},{u,v}},{{x,y},{u,v}},..." from str.
static std::expected<std::vector<group_pair>, std::string> try_parse_group_list(const std::string& str) {
    std::vector<group_pair> groups;
    size_t pos = 0;
    while (pos < str.size()) {
        if (str[pos] == ',') pos++;
        auto group = try_parse_group(str, pos);
        if (!group) return std::unexpected(group.error());
        groups.push_back(*group);
    }
    if (groups.empty())
        return std::unexpected("Empty group list");
    return groups;
}

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
    {"-t {W,H}",         {"Add uniform tileset definition.", [] (arguments_t& args) {
        if (args.empty()) { printf("Missing size for -t.\n"); exit(-1); }
        size_t pos = 0;
        std::string s = args.front(); args.pop_front();
        auto pair = try_parse_pair(s, pos);
        if (!pair) { printf("-t: %s\n", pair.error().c_str()); exit(-1); }
        if (pair->first <= 0 || pair->second <= 0) {
            printf("-t: Width and height must be positive.\n");
            exit(-1);
        }
        has_tileset = true;
        tileset_tile_size = {(int16_t)pair->first, (int16_t)pair->second};
    }}},
    {"-ts {{X,Y},{W,H}},...", {"Add variable tileset rect definitions.", [] (arguments_t& args) {
        if (args.empty()) { printf("Missing rects for -ts.\n"); exit(-1); }
        std::string s = args.front(); args.pop_front();
        auto groups = try_parse_group_list(s);
        if (!groups) { printf("-ts: %s\n", groups.error().c_str()); exit(-1); }
        has_tileset = true;
        tileset_tile_size = {0, 0};
        for (auto& [origin, size] : *groups) {
            tileset_rects.push_back({(int16_t)origin.first, (int16_t)origin.second,
                                     (int16_t)size.first, (int16_t)size.second});
        }
    }}},
/*   {"-g x,y",      {"Add grab point.", [] (arguments_t& args) {
        auto split = split_string(args.front(), ',');
        grab_point = {(int16_t)atoi(split[0].c_str()), (int16_t)atoi(split[1].c_str())};
    }}},*/
};

static void handle_help(arguments_t& args) {
    do_print_help("png2ilbm - A utility for converting png images to iff ilbm.\nusage: png2ilbm [options] image.png [image.iff]", arg_handlers);
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

    auto tshd_writer = [](iffstream_c& stream) -> bool {
        if (!has_tileset) return true;
        iff_chunk_s chunk;
        detail::tileset_header_s tshd{};
        tshd.tile_size = tileset_tile_size;
        stream.begin(detail::cc4::TSHD, chunk);
        stream.write(&tshd);
        for (auto& r : tileset_rects) {
            stream.write((uint16_t*)&r, sizeof(rect_s) / sizeof(uint16_t));
        }
        stream.end(chunk);
        return true;
    };

    if (has_masked_idx) {
        cgimage.save(ilbm_file.c_str(), compression, false, masked_idx, tshd_writer);
    } else {
        cgimage.save(ilbm_file.c_str(), compression, save_masked, image_c::MASKED_CIDX, tshd_writer);
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
