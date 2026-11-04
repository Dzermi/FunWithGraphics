#pragma once
#include "Shape.h"
#include <imgui.h>
#include <cmath>
#include <algorithm>
#include <format>

class Circle : public Shape {
private:
    sf::CircleShape sfmlCircle;

public:
    Circle(float x, float y, float radius) {
        sfmlCircle.setPosition(x, y);
        sfmlCircle.setRadius(radius);
        sfmlCircle.setFillColor(sf::Color(52, 152, 219));
        sfmlCircle.setOutlineColor(sf::Color::White);
        sfmlCircle.setOutlineThickness(2.f);
    }

    sf::Vector2f GetCenter() const {
        float r = sfmlCircle.getRadius();
        return sfmlCircle.getPosition() + sf::Vector2f(r, r);
    }

    void SetCenterAndRadius(const sf::Vector2f& center, float radius) {
        radius = std::max(5.f, radius);
        sfmlCircle.setRadius(radius);
        sfmlCircle.setPosition(center.x - radius, center.y - radius);
    }

    void Draw(sf::RenderWindow& window) const override {
        window.draw(sfmlCircle);
    }

    bool Contains(float x, float y) const override {
        float r = sfmlCircle.getRadius();
        sf::Vector2f center = GetCenter();
        float dx = x - center.x;
        float dy = y - center.y;
        return (dx * dx + dy * dy) <= (r * r);
    }

    void Move(float deltaX, float deltaY) override {
        sfmlCircle.move(deltaX, deltaY);
    }

    std::vector<sf::Vector2f> GetControlPoints() const override {
        sf::Vector2f center = GetCenter();
        float r = sfmlCircle.getRadius();
        // Punkt na prawej krawędzi okręgu do rozciągania promienia
        return { { center.x + r, center.y } };
    }

    void MoveControlPoint(int index, const sf::Vector2f& newPos) override {
        if (index == 0) {
            sf::Vector2f center = GetCenter();
            float dx = newPos.x - center.x;
            float dy = newPos.y - center.y;
            float newRadius = std::sqrt(dx * dx + dy * dy);
            SetCenterAndRadius(center, newRadius);
        }
    }

    void RenderInspectorUI() override {
        ImGui::Text("Typ: Okrag");
        sf::Vector2f center = GetCenter();
        float r = sfmlCircle.getRadius();

        float centerArr[2] = { center.x, center.y };
        if (ImGui::InputFloat2("Srodek (X, Y)", centerArr)) {
            SetCenterAndRadius({ centerArr[0], centerArr[1] }, r);
        }
        if (ImGui::InputFloat("Promien (R)", &r)) {
            SetCenterAndRadius(center, r);
        }
    }

    std::string Serialize() const override {
        sf::Vector2f pos = sfmlCircle.getPosition();
        return std::format("CIRCLE {} {} {}", pos.x, pos.y, sfmlCircle.getRadius());
    }
};