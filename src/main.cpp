#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <iostream>

#include "Rectangle.h"
#include "Circle.h"
#include "Line.h"
#include "SceneSerializer.h"

enum class EditorMode {
    SelectAndMove,
    DrawRectangle,
    DrawCircle,
    DrawLine
};

int main() {
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(1100, 750), "Edytor Wektorowy - ImGui", sf::Style::Default, settings);
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        return -1;
    }

    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Rectangle>(400.f, 200.f, 160.f, 110.f));

    EditorMode currentMode = EditorMode::SelectAndMove;

    // Stan zaznaczenia i interakcji
    Shape* selectedShape = nullptr;
    bool isDraggingShape = false;
    int activeControlPoint = -1; // -1 oznacza brak złapanego uchwytu
    sf::Vector2f lastMousePos;

    // Rysowanie myszą
    bool isDrawing = false;
    sf::Vector2f startPoint;
    std::unique_ptr<Shape> previewShape = nullptr;

    // Pola formularza dodawania nowego obiektu
    float rectInput[4] = { 200.f, 200.f, 120.f, 80.f };
    float circleInput[3] = { 450.f, 200.f, 50.f };
    float lineInput[4] = { 200.f, 400.f, 450.f, 450.f };

    char filenameBuffer[128] = "scena.txt";
    std::string statusMessage = "";
    
    sf::Clock deltaClock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (ImGui::GetIO().WantCaptureMouse) {
                continue;
            }

            // 1. WCIŚNIĘCIE LEWEGO PRZYCISKU MYSZY
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos = window.mapPixelToCoords(
                    sf::Vector2i(event.mouseButton.x, event.mouseButton.y)
                );

                if (currentMode == EditorMode::SelectAndMove) {
                    activeControlPoint = -1;
                    isDraggingShape = false;

                    // Najpierw sprawdzamy, czy kliknięto w uchwyt (punkt charakterystyczny) już zaznaczonej figury
                    if (selectedShape != nullptr) {
                        int hitPt = selectedShape->GetHitControlPointIndex(mousePos.x, mousePos.y);
                        if (hitPt != -1) {
                            activeControlPoint = hitPt;
                        }
                    }

                    // Jeśli nie kliknięto w uchwyt, sprawdzamy, czy trafiono w jakąś figurę
                    if (activeControlPoint == -1) {
                        selectedShape = nullptr;
                        for (auto it = shapes.rbegin(); it != shapes.rend(); ++it) {
                            if ((*it)->Contains(mousePos.x, mousePos.y)) {
                                selectedShape = it->get();
                                isDraggingShape = true;
                                lastMousePos = mousePos;
                                break;
                            }
                        }
                    }
                } else {
                    selectedShape = nullptr;
                    isDrawing = true;
                    startPoint = mousePos;
                }
            }

            // 2. RUCH MYSZY
            if (event.type == sf::Event::MouseMoved) {
                sf::Vector2f currentMousePos = window.mapPixelToCoords(
                    sf::Vector2i(event.mouseMove.x, event.mouseMove.y)
                );

                if (currentMode == EditorMode::SelectAndMove) {
                    if (selectedShape != nullptr && activeControlPoint != -1) {
                        // Zmiana rozmiaru / kształtu za pomocą uchwytu
                        selectedShape->MoveControlPoint(activeControlPoint, currentMousePos);
                    } else if (selectedShape != nullptr && isDraggingShape) {
                        // Przesuwanie całego obiektu
                        float deltaX = currentMousePos.x - lastMousePos.x;
                        float deltaY = currentMousePos.y - lastMousePos.y;
                        selectedShape->Move(deltaX, deltaY);
                        lastMousePos = currentMousePos;
                    }
                } else if (isDrawing) {
                    if (currentMode == EditorMode::DrawRectangle) {
                        float left = std::min(startPoint.x, currentMousePos.x);
                        float top = std::min(startPoint.y, currentMousePos.y);
                        float width = std::abs(currentMousePos.x - startPoint.x);
                        float height = std::abs(currentMousePos.y - startPoint.y);
                        previewShape = std::make_unique<Rectangle>(left, top, width, height);
                    } else if (currentMode == EditorMode::DrawCircle) {
                        float dx = currentMousePos.x - startPoint.x;
                        float dy = currentMousePos.y - startPoint.y;
                        float radius = std::sqrt(dx * dx + dy * dy);
                        previewShape = std::make_unique<Circle>(startPoint.x - radius, startPoint.y - radius, radius);
                    } else if (currentMode == EditorMode::DrawLine) {
                        previewShape = std::make_unique<Line>(startPoint.x, startPoint.y, currentMousePos.x, currentMousePos.y);
                    }
                }
            }

            // 3. ZWOLNIENIE PRZYCISKU MYSZY
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                if (currentMode == EditorMode::SelectAndMove) {
                    isDraggingShape = false;
                    activeControlPoint = -1;
                } else if (isDrawing) {
                    if (previewShape) {
                        selectedShape = previewShape.get();
                        shapes.push_back(std::move(previewShape));
                        previewShape = nullptr;
                    }
                    isDrawing = false;
                }
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::SetNextWindowPos(ImVec2(10.f, 10.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(330.f, 580.f), ImGuiCond_FirstUseEver);

        ImGui::Begin("Panel Sterowania");

        ImGui::Text("Narzedzia myszy:");
        int modeInt = static_cast<int>(currentMode);
        ImGui::RadioButton("Zaznacz / Przesun / Skaluj", &modeInt, 0);
        ImGui::RadioButton("Rysuj Prostokat", &modeInt, 1);
        ImGui::RadioButton("Rysuj Okrag", &modeInt, 2);
        ImGui::RadioButton("Rysuj Linie", &modeInt, 3);
        currentMode = static_cast<EditorMode>(modeInt);

        ImGui::Separator();

        // --- EDYCJA ZAZNACZONEGO OBIEKTU ---
        ImGui::Text("Edycja zaznaczonego obiektu:");
        if (selectedShape != nullptr) {
            selectedShape->RenderInspectorUI();

            if (ImGui::Button("Usun zaznaczony obiekt")) {
                shapes.erase(
                    std::remove_if(shapes.begin(), shapes.end(),
                        [selectedShape](const std::unique_ptr<Shape>& s) { return s.get() == selectedShape; }),
                    shapes.end()
                );
                selectedShape = nullptr;
            }
        } else {
            ImGui::TextDisabled("Brak zaznaczenia (kliknij na figure)");
        }

        ImGui::Separator();

        // --- DODAWANIE NOWYCH OBIEKTÓW Z PÓL TEKSTOWYCH ---
        if (ImGui::CollapsingHeader("Dodaj Prostokat")) {
            ImGui::InputFloat2("X, Y##rect", &rectInput[0]);
            ImGui::InputFloat2("W, H##rect", &rectInput[2]);
            if (ImGui::Button("Wstaw Prostokat")) {
                shapes.push_back(std::make_unique<Rectangle>(rectInput[0], rectInput[1], rectInput[2], rectInput[3]));
                selectedShape = shapes.back().get();
            }
        }

        if (ImGui::CollapsingHeader("Dodaj Okrag")) {
            ImGui::InputFloat2("X, Y##circ", &circleInput[0]);
            ImGui::InputFloat("Promien##circ", &circleInput[2]);
            if (ImGui::Button("Wstaw Okrag")) {
                shapes.push_back(std::make_unique<Circle>(circleInput[0], circleInput[1], circleInput[2]));
                selectedShape = shapes.back().get();
            }
        }

        if (ImGui::CollapsingHeader("Dodaj Linie")) {
            ImGui::InputFloat2("X1, Y1##line", &lineInput[0]);
            ImGui::InputFloat2("X2, Y2##line", &lineInput[2]);
            if (ImGui::Button("Wstaw Linie")) {
                shapes.push_back(std::make_unique<Line>(lineInput[0], lineInput[1], lineInput[2], lineInput[3]));
                selectedShape = shapes.back().get();
            }
        }

       ImGui::Separator();

        // --- SERIALIZACJA (ZAPIS I ODCZYT) ---
        ImGui::Text("Zapis / Odczyt sceny:");
        ImGui::InputText("Plik", filenameBuffer, sizeof(filenameBuffer));

        if (ImGui::Button("Zapisz do pliku")) {
            if (SceneSerializer::SaveToFile(filenameBuffer, shapes)) {
                statusMessage = "Zapisano pomyslnie do: " + std::string(filenameBuffer);
            } else {
                statusMessage = "Blad zapisu pliku!";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Wczytaj z pliku")) {
            selectedShape = nullptr; // Resetujemy wskaźnik przed podmianą wektora!
            isDraggingShape = false;
            activeControlPoint = -1;

            if (SceneSerializer::LoadFromFile(filenameBuffer, shapes)) {
                statusMessage = "Wczytano obiektow: " + std::to_string(shapes.size());
            } else {
                statusMessage = "Blad odczytu pliku!";
            }
        }

        if (ImGui::Button("Wyczysc cala plansze")) {
            selectedShape = nullptr;
            shapes.clear();
            statusMessage = "Plansza wyczyszczona.";
        }

        if (!statusMessage.empty()) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.f), "%s", statusMessage.c_str());
        }

        ImGui::End();

        // --- RENDEROWANIE ---
        window.clear(sf::Color(30, 30, 30));

        for (const auto& shape : shapes) {
            shape->Draw(window);
        }

        if (previewShape) {
            previewShape->Draw(window);
        }

        // Rysujemy uchwyty na wierzchu zaznaczonej figury
        if (selectedShape != nullptr) {
            selectedShape->DrawControlPoints(window);
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}