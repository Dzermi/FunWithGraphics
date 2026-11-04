#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// Forward declaration dla przestrzeni nazw SFML (dodamy później)
namespace sf { class RenderWindow; }

class Shape {
public:
    virtual ~Shape() = default;

    // Rysowanie figury na ekranie
    virtual void Draw(sf::RenderWindow& window) const = 0;

    // Hit-testing: sprawdzanie czy kliknięcie myszy (x,y) trafia w obiekt
    virtual bool Contains(float x, float y) const = 0;

    // Przesuwanie całego obiektu o wektor przesunięcia myszy
    virtual void Move(float deltaX, float deltaY) = 0;

    // Serializacja do stringa (np. "RECTANGLE 10 10 100 50")
    virtual std::string Serialize() const = 0;
    // Zwraca listę punktów charakterystycznych (uchwytów) danej figury
    virtual std::vector<sf::Vector2f> GetControlPoints() const = 0;

    // Zmienia kształt/rozmiar figury, gdy użytkownik przeciąga uchwyt o podanym indeksie
    virtual void MoveControlPoint(int index, const sf::Vector2f& newPos) = 0;

    // Wyświetla pola tekstowe ImGui dedykowane dla tej konkretnej figury
    virtual void RenderInspectorUI() = 0;

    // Pomocnicza metoda rysująca żółte kwadraciki uchwytów dla zaznaczonego obiektu
    void DrawControlPoints(sf::RenderWindow& window) const {
        for (const auto& pt : GetControlPoints()) {
            sf::RectangleShape handle(sf::Vector2f(10.f, 10.f));
            handle.setOrigin(5.f, 5.f); // Środek kwadracika dokładnie w punkcie
            handle.setPosition(pt);
            handle.setFillColor(sf::Color::Yellow);
            handle.setOutlineColor(sf::Color::Black);
            handle.setOutlineThickness(1.5f);
            window.draw(handle);
        }
    }

    // Sprawdza, czy kliknięcie myszą trafiło w któryś z uchwytów (zwraca indeks lub -1)
    int GetHitControlPointIndex(float x, float y) const {
        auto points = GetControlPoints();
        for (size_t i = 0; i < points.size(); ++i) {
            float dx = x - points[i].x;
            float dy = y - points[i].y;
            // Tolerancja trafienia w uchwyt: promień 8 pikseli
            if (dx * dx + dy * dy <= 64.f) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
};