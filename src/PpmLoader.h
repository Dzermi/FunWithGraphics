#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <cctype>
#include <cstdint>
#include "RasterImage.h"

class PpmLoader {
public:
    // Zwraca pusty string przy sukcesie lub treść błędu
    static std::string Load(const std::string& filepath, RasterImage& outImage) {
        outImage.clear();

        // 1. Blokowy odczyt całego pliku do wektora pamięci RAM
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return "Blad: Nie mozna otworzyc pliku '" + filepath + "'.";
        }

        std::streamsize fileSize = file.tellg();
        if (fileSize <= 0) {
            return "Blad: Plik jest pusty.";
        }
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
        if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
            return "Blad: Nie udalo sie wczytac danych z pliku.";
        }

        size_t cursor = 0;

        // Pomocnicza lambda: pomijanie białych znaków i komentarzy (# ...)
        auto skipWhitespaceAndComments = [&]() {
            while (cursor < buffer.size()) {
                if (std::isspace(buffer[cursor])) {
                    cursor++;
                } else if (buffer[cursor] == '#') {
                    // Przewijamy aż do znaku końca linii
                    while (cursor < buffer.size() && buffer[cursor] != '\n' && buffer[cursor] != '\r') {
                        cursor++;
                    }
                } else {
                    break;
                }
            }
        };

        // Pomocnicza lambda: czytanie kolejnego tokena tekstowego (słowa/liczby)
        auto readNextToken = [&]() -> std::string {
            skipWhitespaceAndComments();
            std::string token;
            while (cursor < buffer.size() && !std::isspace(buffer[cursor]) && buffer[cursor] != '#') {
                token += static_cast<char>(buffer[cursor]);
                cursor++;
            }
            return token;
        };

        // 2. Parsowanie nagłówka PPM
        std::string magic = readNextToken();
        if (magic != "P3" && magic != "P6") {
            return "Blad: Nieobslugiwany format pliku (" + magic + "). Oczekiwano PPM P3 lub P6.";
        }

        std::string widthStr = readNextToken();
        std::string heightStr = readNextToken();
        std::string maxValStr = readNextToken();

        if (widthStr.empty() || heightStr.empty() || maxValStr.empty()) {
            return "Blad: Uszkodzony lub niepelny naglowek pliku PPM.";
        }

        int width = 0, height = 0, maxVal = 0;
        try {
            width = std::stoi(widthStr);
            height = std::stoi(heightStr);
            maxVal = std::stoi(maxValStr);
        } catch (...) {
            return "Blad: Wartosci wymiarow lub MaxVal nie sa poprawnymi liczbami calkowitymi.";
        }

        if (width <= 0 || height <= 0) {
            return "Blad: Nieprawidlowe wymiary obrazu: " + std::to_string(width) + "x" + std::to_string(height);
        }
        if (maxVal <= 0) {
            return "Blad: Nieprawidlowa wartosc MaxVal: " + std::to_string(maxVal);
        }

        outImage.width = width;
        outImage.height = height;
        outImage.maxColorValue = maxVal;
        size_t totalPixels = static_cast<size_t>(width * height);
        outImage.pixels.resize(totalPixels * 4); // RGBA

        // 3. Parsowanie zawartości w zależności od typu
        if (magic == "P3") {
            // PPM P3 (tekstowy ASCII)
            for (size_t i = 0; i < totalPixels; ++i) {
                std::string rStr = readNextToken();
                std::string gStr = readNextToken();
                std::string bStr = readNextToken();

                if (rStr.empty() || gStr.empty() || bStr.empty()) {
                    return "Blad P3: Plik zakonczyl sie przedwczesnie (brak pikseli na pozycji " + std::to_string(i) + ").";
                }

                int r = std::stoi(rStr);
                int g = std::stoi(gStr);
                int b = std::stoi(bStr);

                // Skalowanie liniowe do zakresu [0, 255]
                outImage.pixels[i * 4 + 0] = static_cast<uint8_t>(std::clamp((r * 255 + (maxVal / 2)) / maxVal, 0, 255));
                outImage.pixels[i * 4 + 1] = static_cast<uint8_t>(std::clamp((g * 255 + (maxVal / 2)) / maxVal, 0, 255));
                outImage.pixels[i * 4 + 2] = static_cast<uint8_t>(std::clamp((b * 255 + (maxVal / 2)) / maxVal, 0, 255));
                outImage.pixels[i * 4 + 3] = 255; // Alfa (pełna nieprzezroczystość)
            }
        } 
        else if (magic == "P6") {
            // PPM P6 (binarny)
            // Po nagłówku musi nastąpić dokładnie jeden pojedynczy biały znak separatora
            if (cursor < buffer.size() && std::isspace(buffer[cursor])) {
                cursor++;
            }

            if (maxVal <= 255) {
                // 1 bajt na kanał (RGB = 3 bajty na piksel)
                size_t neededBytes = totalPixels * 3;
                if (buffer.size() - cursor < neededBytes) {
                    return "Blad P6: Zbyt malo danych binarnych w pliku w stosunku do deklarowanych wymiarow.";
                }

                const uint8_t* rawData = &buffer[cursor];
                for (size_t i = 0; i < totalPixels; ++i) {
                    int r = rawData[i * 3 + 0];
                    int g = rawData[i * 3 + 1];
                    int b = rawData[i * 3 + 2];

                    outImage.pixels[i * 4 + 0] = static_cast<uint8_t>((r * 255 + (maxVal / 2)) / maxVal);
                    outImage.pixels[i * 4 + 1] = static_cast<uint8_t>((g * 255 + (maxVal / 2)) / maxVal);
                    outImage.pixels[i * 4 + 2] = static_cast<uint8_t>((b * 255 + (maxVal / 2)) / maxVal);
                    outImage.pixels[i * 4 + 3] = 255;
                }
            } else {
                // MaxVal > 255: 2 bajty na kanał (Big-Endian)
                size_t neededBytes = totalPixels * 6;
                if (buffer.size() - cursor < neededBytes) {
                    return "Blad P6 (16-bit): Zbyt malo danych binarnych w pliku.";
                }

                const uint8_t* rawData = &buffer[cursor];
                for (size_t i = 0; i < totalPixels; ++i) {
                    size_t offset = i * 6;
                    // Składanie 16 bitów w formacie Big-Endian (starszy bajt najpierw)
                    int r = (rawData[offset + 0] << 8) | rawData[offset + 1];
                    int g = (rawData[offset + 2] << 8) | rawData[offset + 3];
                    int b = (rawData[offset + 4] << 8) | rawData[offset + 5];

                    outImage.pixels[i * 4 + 0] = static_cast<uint8_t>(std::clamp((r * 255 + (maxVal / 2)) / maxVal, 0, 255));
                    outImage.pixels[i * 4 + 1] = static_cast<uint8_t>(std::clamp((g * 255 + (maxVal / 2)) / maxVal, 0, 255));
                    outImage.pixels[i * 4 + 2] = static_cast<uint8_t>(std::clamp((b * 255 + (maxVal / 2)) / maxVal, 0, 255));
                    outImage.pixels[i * 4 + 3] = 255;
                }
            }
        }

        return ""; // Pusty string oznacza sukces
    }
};