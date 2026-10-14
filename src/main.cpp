// .cpp
// 2D Binary Space Partitioning Visualizer
// by Kyle Furey

#include <iostream>
#include <string>
#include <exception>
#include <cstdlib>
#include <cmath>
#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>
#include <bsp.hpp>

using namespace std;
using namespace nlohmann;
using namespace geo;

// Classic view or mouse over view
constexpr bool CLASSIC_VIEW = true;

int main(int argc, char *argv[]) {
    // Initialize
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        return -1;
    }

    // File IO
    vector<line> lines;
    while (true) {
        cout << "Enter map JSON path: ";
        string input("quit");
        cin >> input;
        if (input == "quit") {
            cout << "Quitting...\n";
            SDL_Quit();
            return 0;
        }
        size_t size;
        char *file = nullptr;
        file = static_cast<char *>(SDL_LoadFile(input.data(), &size));
        if (file != nullptr) {
            try {
                json json = json::parse(file, nullptr, true, true);
                SDL_free(file);
                file = nullptr;
                cout << "Loading BSP...\n";
                for (const auto &l: json) {
                    lines.push_back(
                        line{
                            v2{
                                l["start"]["x"],
                                l["start"]["y"]
                            },
                            v2{
                                l["end"]["x"],
                                l["end"]["y"]
                            }
                        }
                    );
                }
                break;
            } catch (const exception &) {
                cerr << "Bad JSON file!\n";
                SDL_free(file);
                file = nullptr;
            }
        } else {
            cerr << "File not found!\n";
        }
        SDL_Delay(100);
    }

    // BSP generation
    unique_ptr<bsp> result = nullptr;
    for (size_t partition = 0; partition < lines.size(); ++partition) {
        if (result == nullptr) {
            result = bsp::generate(lines, partition);
        } else {
            unique_ptr<bsp> candidate = bsp::generate(lines, partition);
            result = move(candidate->size() < result->size() ? candidate : result);
        }
    }
    size_t size = result->size();

    // Window rendering
    SDL_Window *window = SDL_CreateWindow(
        "Binary Space Partitioning",
        1000,
        1000,
        SDL_WINDOW_ALWAYS_ON_TOP
    );
    if (!CLASSIC_VIEW) {
        SDL_HideCursor();
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    auto redraw = [=, &result](int state, v2 mouse) {
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw map in red
        if (state == 0 || state == 1) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            for (auto l: lines) {
                SDL_RenderLine(renderer, l.start.x, l.start.y, l.end.x, l.end.y);
            }
        }

        // Draw each wall
        size_t remaining = size;
        vector<line> partitions;
        auto draw = [&](auto &&self, const bsp *node) -> void {
            if (node == nullptr) {
                return;
            }
            float sdist = node->partition.sdist(mouse);
            const bsp *positive = sdist > 0.0f ? node->positive.get() : node->negative.get();
            const bsp *negative = sdist > 0.0f ? node->negative.get() : node->positive.get();
            self(self, positive);
            if (!CLASSIC_VIEW || state == 2) {
                if (CLASSIC_VIEW) {
                    // Random color per wall
                    float red = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                    float green = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                    float blue = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                    float len = sqrt(red * red + green * green + blue * blue);
                    red /= len;
                    green /= len;
                    blue /= len;
                    SDL_SetRenderDrawColorFloat(renderer, red, green, blue, 1.0f);
                } else {
                    // White with alpha based on mouse position
                    float alpha = static_cast<float>(remaining) / static_cast<float>(size);
                    SDL_SetRenderDrawColorFloat(renderer, 1.0f, alpha, alpha, 1.0f);
                }
                auto line = node->partition;
                SDL_RenderLine(renderer, line.start.x, line.start.y, line.end.x, line.end.y);
                for (auto line: node->collinear) {
                    SDL_RenderLine(renderer, line.start.x, line.start.y, line.end.x, line.end.y);
                }
            }
            --remaining;
            self(self, negative);
            if (positive == nullptr && negative == nullptr) {
                partitions.push_back(node->partition);
            }
        };
        draw(draw, result.get());

        // Draw each partition line
        if (CLASSIC_VIEW && state == 1) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            for (auto line: partitions) {
                v2 end = ((line.end - line.start) * 1000.0f) + line.start;
                SDL_RenderLine(renderer, line.start.x, line.start.y, end.x, end.y);
            }
        }

        // Draw mouse
        if (!CLASSIC_VIEW) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            auto horz = line{
                (mouse - v2{10.0f, 0.0f}),
                (mouse + v2{10.0f, 0.0f})
            };
            auto vert = line{
                (mouse - v2{0.0f, 10.0f}),
                (mouse + v2{0.0f, 10.0f})
            };
            SDL_RenderLine(renderer, horz.start.x, horz.start.y, horz.end.x, horz.end.y);
            SDL_RenderLine(renderer, vert.start.x, vert.start.y, vert.end.x, vert.end.y);
        }

        // Update screen
        SDL_RenderPresent(renderer);
    };

    // Window closing
    bool run = true;
    SDL_Event event;
    v2 mouse = {500.0f, 500.0f};
    int state = 0;
    while (run) {
        if (CLASSIC_VIEW) {
            redraw(state++ % 3, mouse);
            SDL_Delay(1000);
        }
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                run = false;
            } else if (!CLASSIC_VIEW && event.type == SDL_EVENT_MOUSE_MOTION) {
                mouse.x = event.motion.x;
                mouse.y = event.motion.y;
                redraw(0, mouse);
            }
        }
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
