#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <string>

#include "FileDialog.h"
#include <cstring>
#include <format>

#include "Rectangle.h"
#include "Circle.h"
#include "Line.h"
#include "SceneSerializer.h"
#include "RasterImage.h"
#include "PpmLoader.h"

enum class EditorMode {
    SelectAndMove,
    DrawRectangle,
    DrawCircle,
    DrawLine
};

int main() {
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(1200, 800), "Edytor Graficzny - Wektory i Formaty PPM", sf::Style::Default, settings);
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        return -1;
    }

    // --- STAN DLA EDYTORA WEKTOROWEGO ---
    std::vector<std::unique_ptr<Shape>> shapes;
    EditorMode currentMode = EditorMode::SelectAndMove;
    Shape* selectedShape = nullptr;
    bool isDraggingShape = false;
    int activeControlPoint = -1;
    sf::Vector2f lastMousePos;

    bool isDrawing = false;
    sf::Vector2f startPoint;
    std::unique_ptr<Shape> previewShape = nullptr;

    float rectInput[4] = { 400.f, 200.f, 150.f, 100.f };
    float circleInput[3] = { 600.f, 250.f, 60.f };
    float lineInput[4] = { 400.f, 450.f, 700.f, 500.f };
    char filenameBuffer[128] = "scena.txt";
    std::string vectorStatusMessage = "";

    // --- STAN DLA PRZEGLĄDARKI PPM ---
    RasterImage currentRaster;
    sf::Texture rasterTexture;
    sf::Sprite rasterSprite;
    bool hasRasterLoaded = false;
    char ppmPathBuffer[256] = "test_p3.ppm";
    std::string ppmStatusMessage = "";
    bool ppmLoadError = false;

    // Pozycja wyświetlania obrazu rastrowego
    //rasterSprite.setPosition(380.f, 40.f);

    // --- STAN ZOOM I PRZESUWANIA (KAMERA) ---
    sf::View defaultView = window.getDefaultView();
    sf::View imageView = window.getDefaultView();
    float zoomLevel = 1.0f; // 1.0 = 100%, 0.1 = 1000% zoom
    bool isPanning = false;
    sf::Vector2i panStartPixelPos;

    int activeTab = 0; // 0 = PPM, 1 = Wektory

    sf::Clock deltaClock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // rozmair okna
            if (event.type == sf::Event::Resized) {
                defaultView.setSize(static_cast<float>(event.size.width), static_cast<float>(event.size.height));
                defaultView.setCenter(event.size.width / 2.f, event.size.height / 2.f);
                imageView.setSize(static_cast<float>(event.size.width) * zoomLevel, static_cast<float>(event.size.height) * zoomLevel);
            }

            // Ignorujemy zdarzenia dla płótna, jeśli mysz klika w kontrolki GUI
            if (ImGui::GetIO().WantCaptureMouse) {
                continue;
            }

            // ==================== ZDARZENIA DLA PRZEGLĄDARKI PPM ====================
            if (activeTab == 0 && hasRasterLoaded) {
                // 1. Zoom kółkiem myszy
                if (event.type == sf::Event::MouseWheelScrolled && event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                    float delta = event.mouseWheelScroll.delta;
                    sf::Vector2i mousePixel(event.mouseWheelScroll.x, event.mouseWheelScroll.y);
                    sf::Vector2f beforeZoomWorld = window.mapPixelToCoords(mousePixel, imageView);

                    float zoomFactor = (delta > 0) ? 0.85f : 1.15f;
                    zoomLevel *= zoomFactor;
                    zoomLevel = std::clamp(zoomLevel, 0.005f, 10.0f); // Ograniczenie min i max zoomu

                    imageView.setSize(window.getSize().x * zoomLevel, window.getSize().y * zoomLevel);

                    // Skupienie zoomu wokół punktu kursora myszy
                    sf::Vector2f afterZoomWorld = window.mapPixelToCoords(mousePixel, imageView);
                    imageView.move(beforeZoomWorld - afterZoomWorld);
                }

                // 2. Rozpoczęcie przesuwania (Prawy przycisk myszy lub Środkowy)
                if (event.type == sf::Event::MouseButtonPressed && 
                   (event.mouseButton.button == sf::Mouse::Right || event.mouseButton.button == sf::Mouse::Middle)) {
                    isPanning = true;
                    panStartPixelPos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
                }

                // 3. Koniec przesuwania
                if (event.type == sf::Event::MouseButtonReleased && 
                   (event.mouseButton.button == sf::Mouse::Right || event.mouseButton.button == sf::Mouse::Middle)) {
                    isPanning = false;
                }

                // 4. Ruch myszą przy przesuwaniu (Pan)
                if (event.type == sf::Event::MouseMoved && isPanning) {
                    sf::Vector2i currentPixelPos(event.mouseMove.x, event.mouseMove.y);
                    sf::Vector2f delta = window.mapPixelToCoords(panStartPixelPos, imageView) - 
                                         window.mapPixelToCoords(currentPixelPos, imageView);
                    imageView.move(delta);
                    panStartPixelPos = currentPixelPos;
                }
            }
            // Obsługa myszy dla wektorów
            if (activeTab == 1) {
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos = window.mapPixelToCoords(
                    sf::Vector2i(event.mouseButton.x, event.mouseButton.y)
                );

                if (currentMode == EditorMode::SelectAndMove) {
                    activeControlPoint = -1;
                    isDraggingShape = false;

                    if (selectedShape != nullptr) {
                        int hitPt = selectedShape->GetHitControlPointIndex(mousePos.x, mousePos.y);
                        if (hitPt != -1) {
                            activeControlPoint = hitPt;
                        }
                    }

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

            if (event.type == sf::Event::MouseMoved) {
                sf::Vector2f currentMousePos = window.mapPixelToCoords(
                    sf::Vector2i(event.mouseMove.x, event.mouseMove.y)
                );

                if (currentMode == EditorMode::SelectAndMove) {
                    if (selectedShape != nullptr && activeControlPoint != -1) {
                        selectedShape->MoveControlPoint(activeControlPoint, currentMousePos);
                    } else if (selectedShape != nullptr && isDraggingShape) {
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
    }
    

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::SetNextWindowPos(ImVec2(10.f, 10.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(350.f, 720.f), ImGuiCond_FirstUseEver);

        ImGui::Begin("Sterowanie Projektem");

        if (ImGui::BeginTabBar("MainTabs")) {
            // ==================== ZAKŁADKA 1: FORMATY RASTROWE (PPM) ====================
            if (ImGui::BeginTabItem("Formaty PPM")) {
                activeTab = 0;
                ImGui::Text("Ladowanie plikow PPM (P3 i P6):");
            // Pole tekstowe zajmuje dostępną szerokość pomniejszoną o przycisk
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 110.f);
                ImGui::InputText("##ppmPath", ppmPathBuffer, sizeof(ppmPathBuffer));

                ImGui::SameLine();
                if (ImGui::Button("Przegladaj...")) {
                    std::string selected = FileDialog::OpenFile("Pliki PPM (*.ppm)\0*.ppm\0Wszystkie pliki (*.*)\0*.*\0");
                    if (!selected.empty()) {
                        std::strncpy(ppmPathBuffer, selected.c_str(), sizeof(ppmPathBuffer) - 1);
                        ppmPathBuffer[sizeof(ppmPathBuffer) - 1] = '\0';
                    }
                }

                if (ImGui::Button("Wczytaj plik PPM",ImVec2(-1, 0))) {
                    std::string err = PpmLoader::Load(ppmPathBuffer, currentRaster);
                    if (err.empty() && currentRaster.isValid()) {
                        ppmLoadError = false;
                        ppmStatusMessage = "Zaladowano: " + std::to_string(currentRaster.width) + "x" +
                                           std::to_string(currentRaster.height) + ", MaxVal: " +
                                           std::to_string(currentRaster.maxColorValue);

                        // Tworzenie i aktualizacja tekstury w pamięci karty graficznej
                       if (rasterTexture.create(currentRaster.width, currentRaster.height)) {
                            rasterTexture.update(currentRaster.pixels.data());
                            rasterSprite.setTexture(rasterTexture, true);
                            rasterSprite.setPosition(0.f, 0.f);
                            hasRasterLoaded = true;

                            // Reset kamery i wyśrodkowanie na załadowanym obrazku
                            zoomLevel = 1.0f;
                            imageView.setSize(window.getSize().x, window.getSize().y);
                            imageView.setCenter(currentRaster.width / 2.f, currentRaster.height / 2.f);
                        }
                    } else {
                        ppmLoadError = true;
                        ppmStatusMessage = err.empty() ? "Blad: Nieznany blad wczytywania." : err;
                        hasRasterLoaded = false;
                    }
                }

                ImGui::Separator();

                // Komunikaty statusu / błędu
                if (!ppmStatusMessage.empty()) {
                    if (ppmLoadError) {
                        ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), "%s", ppmStatusMessage.c_str());
                    } else {
                        ImGui::TextColored(ImVec4(0.3f, 1.f, 0.4f, 1.f), "%s", ppmStatusMessage.c_str());
                    }
                }

                if (hasRasterLoaded) {
                    ImGui::Spacing();
                    ImGui::Text("Szczegoly obrazu:");
                    ImGui::BulletText("Szerokosc: %d px", currentRaster.width);
                    ImGui::BulletText("Wysokosc: %d px", currentRaster.height);
                    ImGui::BulletText("Zrodlowy MaxVal: %d", currentRaster.maxColorValue);
                    ImGui::BulletText("Skalowanie liniowe do 8-bit: Aktywne");
                    
                    if (ImGui::Button("Reset widoku kamery")) {
                        zoomLevel = 1.0f;
                        imageView.setSize(window.getSize().x, window.getSize().y);
                        imageView.setCenter(currentRaster.width / 2.f, currentRaster.height / 2.f);
                    }
                }

                ImGui::EndTabItem();
            }

            // ==================== ZAKŁADKA 2: EDYTOR WEKTOROWY ====================
            if (ImGui::BeginTabItem("Edytor Wektorowy")) {
                activeTab = 1;


                ImGui::Text("Narzedzia myszy:");
                int modeInt = static_cast<int>(currentMode);
                ImGui::RadioButton("Zaznacz / Przesun / Skaluj", &modeInt, 0);
                ImGui::RadioButton("Rysuj Prostokat", &modeInt, 1);
                ImGui::RadioButton("Rysuj Okrag", &modeInt, 2);
                ImGui::RadioButton("Rysuj Linie", &modeInt, 3);
                currentMode = static_cast<EditorMode>(modeInt);

                ImGui::Separator();

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
                    ImGui::TextDisabled("Brak zaznaczenia");
                }

                ImGui::Separator();

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

                ImGui::InputText("Plik sceny", filenameBuffer, sizeof(filenameBuffer));
                if (ImGui::Button("Zapisz scene")) {
                    SceneSerializer::SaveToFile(filenameBuffer, shapes);
                }
                ImGui::SameLine();
                if (ImGui::Button("Wczytaj scene")) {
                    selectedShape = nullptr;
                    SceneSerializer::LoadFromFile(filenameBuffer, shapes);
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();

        // --- RENDEROWANIE ---
        window.clear(sf::Color(30, 30, 30));

        // Jeśli załadowano obraz rastrowy, rysujemy go
        if (activeTab == 0 && hasRasterLoaded) {
            // Ustawiamy kamerę dla obrazu rastrowego
            window.setView(imageView);
            window.draw(rasterSprite);

            // Wyznaczamy widoczny obszar świata (współrzędne pikseli obrazka)
            sf::Vector2f topLeft = window.mapPixelToCoords(sf::Vector2i(0, 0), imageView);
            sf::Vector2f bottomRight = window.mapPixelToCoords(sf::Vector2i(window.getSize().x, window.getSize().y), imageView);

            int startX = std::max(0, static_cast<int>(std::floor(topLeft.x)));
            int endX = std::min(currentRaster.width, static_cast<int>(std::ceil(bottomRight.x)));
            int startY = std::max(0, static_cast<int>(std::floor(topLeft.y)));
            int endY = std::min(currentRaster.height, static_cast<int>(std::ceil(bottomRight.y)));

            // Obliczamy rozmiar 1 piksela obrazka w pikselach ekranu
            sf::Vector2i screenP0 = window.mapCoordsToPixel(sf::Vector2f(0.f, 0.f), imageView);
            sf::Vector2i screenP1 = window.mapCoordsToPixel(sf::Vector2f(1.f, 1.f), imageView);
            float pixelScreenSize = static_cast<float>(screenP1.x - screenP0.x);

            // Jeśli piksel jest odpowiednio duży na ekranie (np. >= 45 px) - rysujemy siatkę i wartości RGB
            if (pixelScreenSize >= 45.f) {
                ImDrawList* drawList = ImGui::GetForegroundDrawList();

                for (int y = startY; y < endY; ++y) {
                    for (int x = startX; x < endX; ++x) {
                        sf::Vector2i pTopLeft = window.mapCoordsToPixel(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)), imageView);
                        sf::Vector2i pBottomRight = window.mapCoordsToPixel(sf::Vector2f(static_cast<float>(x + 1), static_cast<float>(y + 1)), imageView);

                        // Siatka wokół piksela (delikatna czarna/ciemnoszara ramka)
                        drawList->AddRect(
                            ImVec2(static_cast<float>(pTopLeft.x), static_cast<float>(pTopLeft.y)),
                            ImVec2(static_cast<float>(pBottomRight.x), static_cast<float>(pBottomRight.y)),
                            IM_COL32(0, 0, 0, 100)
                        );

                        // Pobranie koloru piksela
                        sf::Color c = currentRaster.getPixel(x, y);

                        // Dobór koloru tekstu (biały na ciemnym tle, czarny na jasnym tle)
                        float luminance = 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
                        ImU32 textColor = (luminance > 140.f) ? IM_COL32(0, 0, 0, 255) : IM_COL32(255, 255, 255, 255);

                        std::string rStr = std::format("R:{}", c.r);
                        std::string gStr = std::format("G:{}", c.g);
                        std::string bStr = std::format("B:{}", c.b);

                        float textStartX = static_cast<float>(pTopLeft.x) + 4.f;
                        float textStartY = static_cast<float>(pTopLeft.y) + 4.f;

                        // Wypisanie R, G, B w 3 linijkach
                        drawList->AddText(ImVec2(textStartX, textStartY), textColor, rStr.c_str());
                        drawList->AddText(ImVec2(textStartX, textStartY + 14.f), textColor, gStr.c_str());
                        drawList->AddText(ImVec2(textStartX, textStartY + 28.f), textColor, bStr.c_str());
                    }
                }
            }

            // Przywracamy domyślny widok okna dla GUI i wektorów
            window.setView(defaultView);
        } else if (activeTab == 1) {
        // Rysowanie figur wektorowych
        for (const auto& shape : shapes) {
            shape->Draw(window);
        }
        if (previewShape) {
            previewShape->Draw(window);
        }
        if (selectedShape != nullptr) {
            selectedShape->DrawControlPoints(window);
        }
    }
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}