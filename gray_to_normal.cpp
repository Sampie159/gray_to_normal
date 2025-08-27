#include <print>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <unistd.h>
#include <vector>
#include <filesystem>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

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
    u64 slash_pos = 0;

    for (s32 i = file_name.length() - 1; i >= 0; i--) {
        if (file_name.at(i) == '.') {
            dot_pos = i;
        }

        if (file_name.at(i) == '/') {
            slash_pos = i + 1;
        }

        if (dot_pos != MAX_U64 && slash_pos != 0) break;
    }

    if (dot_pos == MAX_U64) {
        std::println("File extension doesn't exist!");
        exit(EXIT_FAILURE);
    }

    return std::string{file_name, slash_pos, dot_pos} + "_normals.png";
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

static constexpr const char* help_string = R"(gtn - (G)rayscale (T)o (N)ormal Map.

Usage: gtn <file_name> ... [options]

Options:
        -s <strength>       Sets the strength|scale.
        -d <output_dir>     Sets the output directory.
        -j <jobs>           How many threads you want to use. (Mutually Exclusive with -J).
        -J <output_file>    Join all input files into a single output. (Mutually Exclusive with -j).
        -t                  Enables multithreading. Same as -j $(nproc).
        -h                  Displays this help menu.
)";

using VoidFunc = std::function<void()>;

struct ThreadPool {
    std::vector<std::thread> threads;
    std::vector<VoidFunc> tasks;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> done = false;

    ThreadPool(u64 thread_count);

    void add(VoidFunc task);
    void join();
};

ThreadPool::ThreadPool(u64 thread_count) {
    for (u64 i = 0; i < thread_count; i++) {
        threads.emplace_back([this]{
            while (true) {
                VoidFunc task;
                {
                    std::unique_lock lock{mutex};
                    cv.wait(lock, [this] {
                        return done || !tasks.empty();
                    });
                    if (done && tasks.empty()) return;

                    task = std::move(tasks.back());
                    tasks.pop_back();
                }

                task();
            }
        });
    }
}

void ThreadPool::add(VoidFunc task) {
    {
        std::unique_lock lock{mutex};
        tasks.emplace_back(std::move(task));
    }

    cv.notify_one();
}

void ThreadPool::join() {
    {
        std::unique_lock lock{mutex};
        done = true;
    }

    cv.notify_all();
    for (auto& t : threads) t.join();
}

static void run_multithreaded(u64 jobs, const std::vector<std::string>& files, const std::string& out_path, f32 scale) {
    ThreadPool tp{jobs};

    for (const auto& file_name : files) {
        tp.add([&]{
           s32 width, height, channels;
           u8* data = stbi_load(file_name.c_str(), &width, &height, &channels, 1);
           if (!data) {
               std::println("File not found!");
               exit(EXIT_FAILURE);
           }

           std::string file_as_png = get_file_name_as_png(file_name);
           u8* normal_map = generate_normal_map(data, width, height, scale);

           std::string full_path = out_path + file_as_png;
           stbi_write_png(full_path.c_str(), width, height, 3, normal_map, width * 3);
           stbi_image_free(data);
           free(normal_map);
       });
    }

    tp.join();
}

static void run_singlethreaded(const std::vector<std::string>& files, const std::string& out_path, f32 scale) {
    for (const auto& file_name : files) {
       s32 width, height, channels;
       u8* data = stbi_load(file_name.c_str(), &width, &height, &channels, 1);
       if (!data) {
           std::println("File not found!");
           exit(EXIT_FAILURE);
       }

       std::string file_as_png = get_file_name_as_png(file_name);
       u8* normal_map = generate_normal_map(data, width, height, scale);

       std::string full_path = out_path + file_as_png;
       stbi_write_png(full_path.c_str(), width, height, 3, normal_map, width * 3);
       stbi_image_free(data);
       free(normal_map);
   }
}

static void single_output(const std::vector<std::string>& files, const std::string& out_path, f32 scale) {
    bool first_time = true;
    u8* real_data = nullptr;
    s32 width, height, channels;

    for (const auto& file_name : files) {
       u8* data = stbi_load(file_name.c_str(), &width, &height, &channels, 1);
       if (!data) {
           std::println("File not found!");
           exit(EXIT_FAILURE);
       }

       u64 size = width * height;
       if (first_time) {
           real_data = (u8*)calloc(size, 1);
           first_time = false;
       }

       for (u64 i = 0; i < size; i++) {
           real_data[i] += data[i] / files.size();
       }

       stbi_image_free(data);
   }

    u8* normal_map = generate_normal_map(real_data, width, height, scale);
    stbi_write_png(out_path.c_str(), width, height, 3, normal_map, width * 3);

    free(real_data);
    free(normal_map);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::println("Please input a filename");
        exit(EXIT_FAILURE);
    }

    f32 scale = 20;
    std::string out_path = "./";
    u32 jobs = 1;
    bool join_output = false;
    std::string single_file_name;

    s32 c;
    while ((c = getopt(argc, argv, "hts:d:j:J:")) != -1) {
        switch (c) {
        case 's':
            scale = std::stof(optarg);
            break;
        case 'd':
            out_path = optarg;
            break;
        case 'j':
            jobs = std::stoi(optarg);
            break;
        case 't':
            jobs = std::thread::hardware_concurrency();
            break;
        case 'J':
            join_output = true;
            single_file_name = optarg;
            break;
        case 'h':
            std::println("{}", help_string);
            return 0;
        default:
            exit(EXIT_FAILURE);
        }
    }

    if (out_path.at(out_path.length() - 1) != '/') {
        out_path += '/';
    }

    if (!std::filesystem::exists(out_path)) {
        std::filesystem::create_directory(out_path);
    }

    std::vector<std::string> files;
    for (s32 i = optind; i < argc; i++) {
        files.emplace_back(argv[i]);
    }

    if (files.empty()) {
        std::println("No input files specified!");
        exit(EXIT_FAILURE);
    }

    if (join_output) {
        out_path += single_file_name;
        single_output(files, out_path, scale);
    } else if (jobs > 1) {
        run_multithreaded(jobs, files, out_path, scale);
    } else {
        run_singlethreaded(files, out_path, scale);
    }

    std::println("Successfully created normal maps!");
    return 0;
}
