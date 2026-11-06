#pragma once
#include <string>

class FileDialog {
public:
    static std::string OpenFile(const char* filter = "Pliki PPM (*.ppm)\0*.ppm\0Pliki JPEG (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0Wszystkie pliki (*.*)\0*.*\0");
    static std::string SaveFile(const char* filter = "Plik JPEG (*.jpg)\0*.jpg\0Wszystkie pliki (*.*)\0*.*\0");
};