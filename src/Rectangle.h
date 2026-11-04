#pragma once
#include "Shape.h"
#include <imgui.h>
#include <cmath>
#include <algorithm>
#include <format>

class Rectangle : public Shape {
private:
    sf::RectangleShape sfmlShape;

public:
    Rectangle(float x, float y, float width, float height) {
        sfmlShape.setPosition(x, y);
        sfmlShape.setSize(sf::Vector2f(width, height));
        sfmlShape.setFillColor(sf::Color(46, 204, 113));
        sfmlShape.setOutlineColor(sf::Color::White);
        sfmlShape.setOutlineThickness(2.f);
    }

    void Draw(sf::RenderWindow& window) const override {
        window.draw(sfmlShape);
    }

    bool Contains(float x, float y) const override {
        return sfmlShape.getGlobalBounds().contains(x, y);
    }

    void Move(float deltaX, float deltaY) override {
        sfmlShape.move(deltaX, deltaY);
    }

    std::vector<sf::Vector2f> GetControlPoints() const override {
        sf::Vector2f pos = sfmlShape.getPosition();
        sf::Vector2f size = sfmlShape.getSize();
        return {
            pos,                                      // 0: Lewy górny
            { pos.x + size.x, pos.y },                // 1: Prawy górny
            { pos.x + size.x, pos.y + size.y },       // 2: Prawy dolny
            { pos.x, pos.y + size.y }                 // 3: Lewy dolny
        };
    }

    void MoveControlPoint(int index, const sf::Vector2f& newPos) override {
        sf::Vector2f pos = sfmlShape.getPosition();
        sf::Vector2f size = sfmlShape.getSize();
        
        // Wyznaczamy przeciwległy narożnik, który zostaje nieruchomy (kotwica)
        sf::Vector2f anchor;
        if (index == 0) anchor = { pos.x + size.x, pos.y + size.y };
        else if (index == 1) anchor = { pos.x, pos.y + size.y };
        else if (index == 2) anchor = pos;
        else if (index == 3) anchor = { pos.x + size.x, pos.y };
        else return;

        float left = std::min(anchor.x, newPos.x);
        float top = std::min(anchor.y, newPos.y);
        float width = std::max(5.f, std::abs(newPos.x - anchor.x));
        float height = std::max(5.f, std::abs(newPos.y - anchor.y));

        sfmlShape.setPosition(left, top);
        sfmlShape.setSize({ width, height });
    }

    void RenderInspectorUI() override {
        ImGui::Text("Typ: Prostokat");
        sf::Vector2f pos = sfmlShape.getPosition();
        sf::Vector2f size = sfmlShape.getSize();

        float posArr[2] = { pos.x, pos.y };
        float sizeArr[2] = { size.x, size.y };

        if (ImGui::InputFloat2("Pozycja (X, Y)", posArr)) {
            sfmlShape.setPosition(posArr[0], posArr[1]);
        }
        if (ImGui::InputFloat2("Rozmiar (W, H)", sizeArr)) {
            sfmlShape.setSize({ std::max(1.f, sizeArr[0]), std::max(1.f, sizeArr[1]) });
        }
    }

    std::string Serialize() const override {
        sf::Vector2f pos = sfmlShape.getPosition();
        sf::Vector2f size = sfmlShape.getSize();
        return std::format("RECTANGLE {} {} {} {}", pos.x, pos.y, size.x, size.y);
    }
};