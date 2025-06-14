#include <iostream>
#include <optional>
#include <vector>
#include <png.h>

std::optional<std::pair<std::pair<std::size_t,std::size_t>, std::vector<float>>>
openPngFile(const std::string& filePath) {
    std::vector<float> pixelData;

    // Open the PNG file
    FILE* file = fopen(filePath.c_str(), "rb");
    if (!file) {
        std::cerr << "Error opening PNG file: " << filePath << std::endl;
        return {};
    }

    // Create PNG read struct
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        std::cerr << "Error creating PNG read struct" << std::endl;
        fclose(file);
        return {};
    }

    // Create PNG info struct
    png_infop info = png_create_info_struct(png);
    if (!info) {
        std::cerr << "Error creating PNG info struct" << std::endl;
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(file);
        return {};
    }

    // Set up error handling
    if (setjmp(png_jmpbuf(png))) {
        std::cerr << "Error during PNG read" << std::endl;
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(file);
        return {};
    }

    // Initialize PNG IO
    png_init_io(png, file);

    // Read PNG header
    png_read_info(png, info);

    // Get image attributes
    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    png_byte colorType = png_get_color_type(png, info);
    png_byte bitDepth = png_get_bit_depth(png, info);

    // Ensure the PNG file is in a supported format
    if (colorType != PNG_COLOR_TYPE_RGB_ALPHA || bitDepth != 8) {
        std::cerr << "Unsupported PNG format" << std::endl;
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(file);
        return {};
    }

    // Allocate memory for image data
    png_bytep* rowPointers = new png_bytep[height];
    for (int y = 0; y < height; y++) {
        rowPointers[y] = new png_byte[png_get_rowbytes(png, info)];
    }

    // Read PNG image data
    png_read_image(png, rowPointers);

    // Convert pixel data to vector of floats
    for (int y = 0; y < height; y++) {
        png_bytep row = rowPointers[y];
        for (int x = 0; x < width; x++) {
            png_bytep pixel = &(row[x * 4]);
            float r = pixel[0] / 255.0f;
            float g = pixel[1] / 255.0f;
            float b = pixel[2] / 255.0f;
            float a = pixel[3] / 255.0f;
            pixelData.push_back(r);
            pixelData.push_back(g);
            pixelData.push_back(b);
            pixelData.push_back(a);
        }
    }

    // Clean up
    for (int y = 0; y < height; y++) {
        delete[] rowPointers[y];
    }
    delete[] rowPointers;
    png_destroy_read_struct(&png, &info, nullptr);
    fclose(file);

    return std::make_pair(std::make_pair(width, height), std::move(pixelData));
}
