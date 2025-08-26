#include <cstdlib>
#include <print>
#include <cstdint>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

static constexpr u64 MAX_U64 = 0xFFFFFFFFFFFFFFFF;

static std::string get_file_name_as_png(std::string_view file_name) {
    u64 dot_pos = MAX_U64;

    for (s32 i = file_name.length() - 1; i >= 0; i--) {
        if (file_name.at(i) == '.') {
            dot_pos = i;
            break;
        }
    }

    if (dot_pos == MAX_U64) {
        std::println("File extension doesn't exist!");
        exit(EXIT_FAILURE);
    }

    return std::string{file_name, 0, dot_pos} + "_normals.png";
}

static inline f32 h(s32 x, s32 y, s32 width, s32 height, const u8* heightmap) {
    x = std::clamp(x, 0, width - 1);
    y = std::clamp(y, 0, height - 1);
    return heightmap[y * width + x] / 255.f;
}

static u8* generate_normal_map(const u8* heightmap, s32 width, s32 height, f32 scale) {
    u8* data = (u8*)malloc(width * height * 3);

    for (s32 y = 0; y < height; y++) {
        for (s32 x = 0; x < width; x++) {
            f32 height_left = h(x - 1, y, width, height, heightmap);
            f32 height_right = h(x + 1, y, width, height, heightmap);
            f32 height_up = h(x, y - 1, width, height, heightmap);
            f32 height_down = h(x, y + 1, width, height, heightmap);

            f32 dx = (height_right - height_left) * scale;
            f32 dy = (height_down - height_up) * scale;

            f32 nx = -dx;
            f32 ny = -dy;
            f32 nz = 1;

            f32 len = 1.f / std::sqrt(nx * nx + ny * ny + nz * nz);
            nx *= len;
            ny *= len;
            nz *= len;

            u8 r = std::clamp(nx * .5 + .5, 0., 1.) * 255;
            u8 g = std::clamp(ny * .5 + .5, 0., 1.) * 255;
            u8 b = std::clamp(nz * .5 + .5, 0., 1.) * 255;

            s32 idx = (y * width + x) * 3;
            data[idx + 0] = r;
            data[idx + 1] = g;
            data[idx + 2] = b;
        }
    }

    return data;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::println("Please input a filename");
        exit(EXIT_FAILURE);
    }

    std::string file_name = argv[1];
    f32 scale = argv[2] ? std::stof(argv[2]) : 20.f;

    s32 width, height, channels;
    u8* data = stbi_load(file_name.c_str(), &width, &height, &channels, 1);
    if (!data) {
        std::println("File not found!");
        exit(EXIT_FAILURE);
    }

    std::string file_as_png = get_file_name_as_png(file_name);
    u8* normal_map = generate_normal_map(data, width, height, scale);

    stbi_write_png(file_as_png.c_str(), width, height, 3, normal_map, width * 3);

    std::println("Successfully created normal map!");
    return 0;
}
