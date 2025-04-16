#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#include <string>
#include "external/stb_image.h"

struct image_data {
    const char * location = nullptr;
    int width;
    int height;
    int channels;
    unsigned char* data = nullptr;
    std::vector<uint8_t> pixels;
    std::string name = std::string("");
};
struct compressed_image_data {
    int width;
    int height;
    std::vector<uint8_t> data;
};

image_data load_image(const char * str);
compressed_image_data load_compressed_image(const std::string& filename);
std::unordered_map<uint32_t,int> color_data(image_data image);
std::vector<std::pair<uint32_t,int>> sorted_color_data(std::unordered_map<uint32_t,int> color_data);
double distribution_sharpness(std::vector<std::pair<uint32_t,int>>  sorted_colors,image_data image);
float beta_function(std::vector<std::pair<uint32_t,int>>  sorted_colors, image_data image);
std::vector<uint32_t> generate_hex_palette(std::vector<std::pair<uint32_t,int>> sorted_colors, float beta);
std::array<float, 3> hex_to_rgb(uint32_t hex);
void save_rgb_palette_as_ppm(const std::string& filename,const std::vector<uint32_t>& hex_palette,int grid_width = 16, int block_size =20);
void save_ppm(const std::string& filename,const std::vector<uint8_t>& rgb_data,int width, int height);
uint8_t find_nearest_palette_index(float r, float g, float b,const std::vector<std::array<float, 3>>& palette);
std::vector<uint8_t> remap_image_to_palette(const image_data image,const std::vector<uint32_t>& hex_palette);
std::vector<uint8_t> reconstruct_rgb_image(const std::vector<uint8_t>& indexed_data,const std::vector<std::array<float, 3>>& palette);
std::vector<std::array<float,3>> hex_rgb_palette(std::vector<uint32_t> hex_palette);
std::string get_filename(const char* path);
void save_luna(image_data image);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: compressor <image_path>\n";
        return 1;
    }

    const char* image_path = argv[1];
    image_data image = load_image(image_path);
    stbi_image_free(image.data);

    save_luna(image);

    return 0;
}

std::vector<uint8_t> reconstruct_rgb_image(const std::vector<uint8_t>& indexed_data,const std::vector<std::array<float, 3>>& palette)
{
    std::vector<uint8_t> rgb_data;
    rgb_data.reserve(indexed_data.size() * 3);

    for (uint8_t index : indexed_data) {
        const auto& color = palette[index];
        rgb_data.push_back(static_cast<uint8_t>(color[0] * 255.0f));
        rgb_data.push_back(static_cast<uint8_t>(color[1] * 255.0f));
        rgb_data.push_back(static_cast<uint8_t>(color[2] * 255.0f));
    }

    return rgb_data;
}

std::vector<std::array<float, 3>> hex_rgb_palette(std::vector<uint32_t> hex_palette) {
    std::vector<std::array<float, 3>> palette;
    for (int i = 0; i < 256; ++i) {
        auto rgb = hex_to_rgb(hex_palette[i]);
        palette.push_back({
            rgb[0] / 255.0f,
            rgb[1] / 255.0f,
            rgb[2] / 255.0f
        });
    }
    return palette;
}

std::vector<uint8_t> remap_image_to_palette(const image_data image,const std::vector<uint32_t>& hex_palette)
{

    int width = image.width, height = image.height;



    // Convert hex palette to normalized float RGB
    std::vector<std::array<float, 3>> palette;
    palette = hex_rgb_palette(hex_palette);

    // Prepare the output
    std::vector<uint8_t> indexed_data;
    indexed_data.reserve(width * height);  // 1 byte per pixel
    int total_0s = 0;
    // Map each pixel to nearest palette entry
    for (int i = 0; i < width * height; ++i) {
        float r = image.pixels[i * 3 + 0] / 255.0f;
        float g = image.pixels[i * 3 + 1] / 255.0f;
        float b = image.pixels[i * 3 + 2] / 255.0f;

        uint8_t index = find_nearest_palette_index(r, g, b, palette);
        indexed_data.push_back(index);

    }
    return indexed_data;
}

uint8_t find_nearest_palette_index(float r, float g, float b,const std::vector<std::array<float, 3>>& palette) {
    float min_dist = std::numeric_limits<float>::max();
    uint8_t best_index = 0;

    for (size_t i = 0; i < palette.size(); ++i) {
        float dr = r - palette[i][0];
        float dg = g - palette[i][1];
        float db = b - palette[i][2];

        float dist = dr * dr + dg * dg + db * db;
        if (dist < min_dist) {
            min_dist = dist;
            best_index = i;
        }
    }
    return best_index;
}

std::string get_filename(const char* filename) {
    std::string path(filename);
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

image_data load_image(const char* location) {
    image_data image = {location};
    image.name = get_filename(location);

    unsigned char* data = stbi_load( image.location, &image.width, &image.height, &image.channels, 3);
    image.data = data;


    int pixel_count = image.width * image.height;
    int total_bytes = pixel_count * image.channels;
    image.pixels = std::vector<uint8_t>(image.data,image.data + total_bytes);

    if (!data) {
        std::cerr << "Failed to load image: " << stbi_failure_reason() << "\n";

    }
    return image;
}
compressed_image_data load_compressed_image(const std::string& filename) {
    compressed_image_data img;

    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return img;
    }

    // Read width and height (each is 4 bytes)
    in.read(reinterpret_cast<char*>(&img.width), sizeof(int));
    in.read(reinterpret_cast<char*>(&img.height), sizeof(int));

    // Read data size (optional but recommended)
    uint32_t data_size = 0;
    in.read(reinterpret_cast<char*>(&data_size), sizeof(uint32_t));

    // Resize and read the image data
    img.data.resize(data_size);
    in.read(reinterpret_cast<char*>(img.data.data()), data_size);

    in.close();
    return img;
}
std::unordered_map<uint32_t,int> color_data(image_data image) {
    std::unordered_map<uint32_t, int> color_frequency;

    int pixel_count = image.width * image.height;

    for (int i = 0; i < pixel_count; ++i) {
        int offset = i * 3;
        uint8_t r = image.data[offset + 0];
        uint8_t g = image.data[offset + 1];
        uint8_t b = image.data[offset + 2];

        // Pack into a 24-bit integer: 0xRRGGBB
        uint32_t color = (r << 16) | (g << 8) | b;

        color_frequency[color]++;
    }
    return color_frequency;
}

std::vector<std::pair<uint32_t,int>> sorted_color_data(std::unordered_map<uint32_t,int> color_data) {
    std::vector<std::pair<uint32_t, int>> sorted_colors(
    color_data.begin(), color_data.end()
);

    std::sort(sorted_colors.begin(), sorted_colors.end(),
    [](const auto& a, const auto& b) {
        return a.second > b.second;  // sort by frequency
    });
    return sorted_colors;
}

double distribution_sharpness(std::vector<std::pair<uint32_t,int>>  sorted_colors, image_data image) {
    int total_colors = sorted_colors.size();
    int total_pixels = image.width * image.height;
    int pixel_count_t256 = 0;
    for (int i = 0; i < 256; ++i) {
        pixel_count_t256 += sorted_colors[i].second;
    }

    std::cout << "# of pixels in the top 256 colors: " << pixel_count_t256 << "\n";

    double pixel_ratio = static_cast<double>(pixel_count_t256) / total_pixels;
    double color_ratio = 256.0 / total_colors;
    double sharpness = pixel_ratio / color_ratio;

    std::cout << "Sharpness: " << sharpness << "\n";
    return sharpness;
}
float beta_function(std::vector<std::pair<uint32_t,int>>  sorted_colors, image_data image) {
    int total_colors = sorted_colors.size();
    float sharpness = distribution_sharpness(sorted_colors, image);
    float beta = std::ranges::clamp(0.5f + std::log10(sharpness), 0.5f, 3.5f);
    std::cout << "Beta: " << beta << "\n";
    return beta;
}

std::vector<uint32_t> generate_hex_palette(std::vector<std::pair<uint32_t,int>> sorted_colors, float beta) {
    std::vector<uint32_t> palette;
    int total_colors = sorted_colors.size();
    for (int i = 0; i < 256; ++i) {
        float t = static_cast<float>(i) / 255.0f;
        int idx = static_cast<int>(std::pow(t, beta) * (total_colors - 1));

        palette.push_back(sorted_colors[idx].first);  // take RGB key from (color, freq) pair
    }
    return palette;
}

void save_rgb_palette_as_ppm(const std::string& filename,const std::vector<uint32_t>& hex_palette,int grid_width, int block_size) {
    int grid_height = (hex_palette.size() + grid_width - 1) / grid_width;
    int width = grid_width * block_size;
    int height = grid_height * block_size;

    std::ofstream out(filename);
    out << "P3\n" << width << " " << height << "\n255\n";

    std::vector<std::array<float,3>> rgb_palette;
    // convert colors to rgb
    for (int i = 0; i<256; i++) {
        rgb_palette.push_back(hex_to_rgb(hex_palette[i]));
    }

    for (int y = 0; y < height; ++y) {
        int row = y / block_size;
        for (int x = 0; x < width; ++x) {
            int col = x / block_size;
            int index = row * grid_width + col;

            std::array<float, 3> rgb = {0, 0, 0};  // default to black
            if (index < rgb_palette.size()) {
                rgb = rgb_palette[index];
            }

            out << (int)rgb[0] << " "
                << (int)rgb[1] << " "
                << (int)rgb[2] << " ";
        }
        out << "\n";
    }
}

std::array<float, 3> hex_to_rgb(uint32_t hex) {
    float r = (hex >> 16) & 0xFF;
    float g = (hex >> 8) & 0xFF;
    float b = hex & 0xFF;
    return {r, g, b};
}

void save_luna(image_data image)
{
    const int width = image.width;
    const int height = image.height;
    const std::string& filename = image.name;

    std::unordered_map<uint32_t,int> colors = color_data(image);
    std::vector<std::pair<uint32_t,int>> sorted_colors = sorted_color_data(colors);
    std::vector<uint32_t> palette = generate_hex_palette(sorted_colors, beta_function(sorted_colors, image));
    std::vector<std::array<float,3>> rgb_palette = hex_rgb_palette(palette);
    std::vector<uint8_t> indexed_data = remap_image_to_palette(image,palette);

    size_t dot = filename.find_last_of('.');
    std::string base = (dot == std::string::npos) ? filename : filename.substr(0, dot);

    std::string luna_filename = "/Users/nickleinberger/Downloads/" + base + ".luna";
    std::ofstream out(luna_filename, std::ios::binary);
    std::cout << "Writing " << luna_filename << "\n";
    // Header: width + height
    uint32_t w = width, h = height;
    out.write(reinterpret_cast<char*>(&w), sizeof(uint32_t));
    out.write(reinterpret_cast<char*>(&h), sizeof(uint32_t));

    // Palette (256 colors × 3 channels)
    for (const auto& color : rgb_palette) {
        uint8_t r = static_cast<uint8_t>(color[0] * 255.0f);
        uint8_t g = static_cast<uint8_t>(color[1] * 255.0f);
        uint8_t b = static_cast<uint8_t>(color[2] * 255.0f);
        out.write(reinterpret_cast<char*>(&r), 1);
        out.write(reinterpret_cast<char*>(&g), 1);
        out.write(reinterpret_cast<char*>(&b), 1);
    }

    // Pixel data
    out.write(reinterpret_cast<const char*>(indexed_data.data()), indexed_data.size());
    out.close();
}