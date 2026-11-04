#pragma once
#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include "Shape.h"
#include "Rectangle.h"
#include "Circle.h"
#include "Line.h"

class SceneSerializer {
public:
    // Zapisuje wszystkie obiekty z wektora do pliku tekstowego
    static bool SaveToFile(const std::string& filename, const std::vector<std::unique_ptr<Shape>>& shapes) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        for (const auto& shape : shapes) {
            file << shape->Serialize() << "\n";
        }

        return true;
    }

    // Wczytuje obiekty z pliku tekstowego i odtwarza wektor kształtów
    static bool LoadFromFile(const std::string& filename, std::vector<std::unique_ptr<Shape>>& shapes) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::vector<std::unique_ptr<Shape>> loadedShapes;
        std::string line;

        while (std::getline(file, line)) {
            if (line.empty()) continue;

            std::istringstream iss(line);
            std::string type;
            iss >> type; // Wczytujemy pierwsze słowo określające typ figury

            if (type == "RECTANGLE") {
                float x, y, w, h;
                if (iss >> x >> y >> w >> h) {
                    loadedShapes.push_back(std::make_unique<Rectangle>(x, y, w, h));
                }
            } else if (type == "CIRCLE") {
                float x, y, r;
                if (iss >> x >> y >> r) {
                    loadedShapes.push_back(std::make_unique<Circle>(x, y, r));
                }
            } else if (type == "LINE") {
                float x1, y1, x2, y2;
                if (iss >> x1 >> y1 >> x2 >> y2) {
                    loadedShapes.push_back(std::make_unique<Line>(x1, y1, x2, y2));
                }
            }
        }

        // Podmieniamy aktualną listę figur na nowo wczytaną
        shapes = std::move(loadedShapes);
        return true;
    }
};