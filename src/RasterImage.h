#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <SFML/Graphics.hpp>

struct RasterImage {
    int width = 0;
    int height = 0;
    int maxColorValue = 255;
    // Format SFML oczekuje RGBA (4 bajty na piksel: R, G, B, A)
    std::vector<uint8_t> pixels; 

    bool isValid() const {
        return width > 0 && height > 0 && pixels.size() == static_cast<size_t>(width * height * 4);
    }

    void clear() {
        width = 0;
        height = 0;
        maxColorValue = 255;
        pixels.clear();
    }

    // Pobranie koloru piksela (dla powiększenia i wyświetlania RGB)
    sf::Color getPixel(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) return sf::Color::Black;
        size_t idx = static_cast<size_t>((y * width + x) * 4);
        return sf::Color(pixels[idx], pixels[idx + 1], pixels[idx + 2], pixels[idx + 3]);
    }
};