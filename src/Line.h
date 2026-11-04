#pragma once
#include "Shape.h"
#include <imgui.h>
#include <cmath>
#include <algorithm>
#include <format>

class Line : public Shape {
private:
    sf::Vector2f p1;
    sf::Vector2f p2;
    float thickness;
    sf::Color color;

public:
    Line(float x1, float y1, float x2, float y2, float thickness = 4.f)
        : p1(x1, y1), p2(x2, y2), thickness(thickness), color(sf::Color(231, 76, 60)) {}

    void Draw(sf::RenderWindow& window) const override {
        sf::Vector2f direction = p2 - p1;
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length == 0.f) return;

        sf::RectangleShape lineShape(sf::Vector2f(length, thickness));
        lineShape.setOrigin(0.f, thickness / 2.f); // Wyśrodkowanie względem punktów końcowych
        lineShape.setPosition(p1);
        lineShape.setFillColor(color);

        float angle = std::atan2(direction.y, direction.x) * 180.f / 3.14159265f;
        lineShape.setRotation(angle);

        window.draw(lineShape);
    }

    bool Contains(float x, float y) const override {
        sf::Vector2f p(x, y);
        sf::Vector2f ab = p2 - p1;
        float abSquared = ab.x * ab.x + ab.y * ab.y;
        if (abSquared == 0.f) return false;

        float t = ((p.x - p1.x) * ab.x + (p.y - p1.y) * ab.y) / abSquared;
        t = std::clamp(t, 0.f, 1.f);

        sf::Vector2f projection = p1 + t * ab;
        float dx = p.x - projection.x;
        float dy = p.y - projection.y;
        float hitThreshold = (thickness / 2.f) + 6.f;
        return (dx * dx + dy * dy) <= (hitThreshold * hitThreshold);
    }

    void Move(float deltaX, float deltaY) override {
        sf::Vector2f delta(deltaX, deltaY);
        p1 += delta;
        p2 += delta;
    }

    std::vector<sf::Vector2f> GetControlPoints() const override {
        return { p1, p2 };
    }

    void MoveControlPoint(int index, const sf::Vector2f& newPos) override {
        if (index == 0) p1 = newPos;
        else if (index == 1) p2 = newPos;
    }

    void RenderInspectorUI() override {
        ImGui::Text("Typ: Linia");
        float pt1[2] = { p1.x, p1.y };
        float pt2[2] = { p2.x, p2.y };

        if (ImGui::InputFloat2("Punkt 1 (X1, Y1)", pt1)) {
            p1 = { pt1[0], pt1[1] };
        }
        if (ImGui::InputFloat2("Punkt 2 (X2, Y2)", pt2)) {
            p2 = { pt2[0], pt2[1] };
        }
    }

    std::string Serialize() const override {
        return std::format("LINE {} {} {} {}", p1.x, p1.y, p2.x, p2.y);
    }
};