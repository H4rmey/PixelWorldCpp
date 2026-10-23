#include "world/chunkManager.hpp"
#include "world/helpers/constants.h"
#include "world/helpers/element_type.h"
#include "world/helpers/matter_state.h"
#include "world/helpers/physics.hpp"
#include "world/movement/IMovement.h"
#include "world/movementSystems.hpp"
#include <algorithm>
#include <iostream>
#include <raylib.h>

Color element_to_color(ElementType type)
{
    switch (type)
    {
    case ElementType::Empty:
        return GRAY;
    case ElementType::Sand:
        return YELLOW;
    case ElementType::Water:
        return BLUE;
    case ElementType::Stone:
        return DARKGRAY;
    case ElementType::Gas:
        return GREEN;
    case ElementType::Bedrock:
        return BLACK;
    case ElementType::Custom:
        return PURPLE;
    case ElementType::Scaffolding:
        return BROWN;
    default:
        return WHITE;
    }
}

int main()
{
    const int cellSize = CELL_SIZE;

    const int total_width  = CHUNK_COUNT_X * CHUNK_SIZE_X;
    const int total_height = CHUNK_COUNT_Y * CHUNK_SIZE_Y;

    const int screenW = total_width * cellSize;
    const int screenH = total_height * cellSize;

    InitWindow(screenW, screenH, "World Simulation");
    SetTargetFPS(MAX_FRAMERATE);

    // World + movement systems
    static WorldManager world;
    SandMovement        sand_movement;
    SolidMovement       solid_movement;
    GasMovement         gas_movement;
    LiquidMovement      liquid_movement;
    EmptyMovement       empty_movement;
    ScaffoldingMovement scaffolding_movement;

    IMovement *movement_ptr = &sand_movement;

    // Add these near your brush_size variables
    bool FRAME_CLICK = false;
    bool step_once   = false; // Trigger for a single frame update
                              //
    int         brush_size   = BRUSH_SIZE;
    ElementType brush_type   = ElementType::Sand;
    MatterState matter_state = MatterState::Solid;
    Physics     physics_type = Physics::Empty;

    // ---------------------------
    // Camera setup
    // ---------------------------
    Camera2D camera = {};
    camera.target   = {0.0f, 0.0f};
    camera.offset   = {0.0f, 0.0f};
    camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    const float cam_pan_speed = 900.0f;
    const float zoom_step     = 1.10f;
    const float zoom_min      = 0.10f;
    const float zoom_max      = 12.0f;

    // Target 60 updates per second for the simulation
    const float targetDt    = 1.0f / 60.0f;
    float       accumulator = 0.0f;

    while (!WindowShouldClose())
    {
        // --- Frame Stepping Logic ---
        // Press 'P' to toggle between Real-time and Frame-by-Frame mode
        if (IsKeyPressed(KEY_P))
        {
            FRAME_CLICK = !FRAME_CLICK;
        }

        // Press 'F' to trigger exactly one frame update when in Frame-by-Frame
        // mode
        if (FRAME_CLICK && IsKeyPressed(KEY_F))
        {
            step_once = true;
        }
        const bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        // ---------------------------
        // Zoom (SHIFT + +/-)
        // ---------------------------
        if (shiftDown)
        {
            bool zoomIn  = (IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_EQUAL));
            bool zoomOut = (IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_MINUS));

            if (zoomIn || zoomOut)
            {
                Vector2 mouse_screen = GetMousePosition();
                Vector2 before       = GetScreenToWorld2D(mouse_screen, camera);

                float newZoom = camera.zoom * (zoomIn ? zoom_step : (1.0f / zoom_step));
                newZoom       = std::clamp(newZoom, zoom_min, zoom_max);
                camera.zoom   = newZoom;

                Vector2 after = GetScreenToWorld2D(mouse_screen, camera);
                camera.target.x += (before.x - after.x);
                camera.target.y += (before.y - after.y);
            }
        }
        else
        {
            // Brush size
            if (IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_EQUAL))
            {
                brush_size = std::min(90, brush_size + 1);
            }

            if (IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_MINUS))
            {
                brush_size = std::max(0, brush_size - 1);
            }
        }

        // ---------------------------
        // Pan with SHIFT + LMB drag
        // ---------------------------
        if (shiftDown && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            Vector2 d = GetMouseDelta();
            camera.target.x -= d.x / camera.zoom;
            camera.target.y -= d.y / camera.zoom;
        }

        // ---------------------------
        // Pan with arrow keys
        // ---------------------------
        {
            float dt   = GetFrameTime();
            float step = (cam_pan_speed * dt) / camera.zoom;

            if (IsKeyDown(KEY_RIGHT))
            {
                camera.target.x += step;
            }
            if (IsKeyDown(KEY_LEFT))
            {
                camera.target.x -= step;
            }
            if (IsKeyDown(KEY_DOWN))
            {
                camera.target.y += step;
            }
            if (IsKeyDown(KEY_UP))
            {
                camera.target.y -= step;
            }
        }

        // ---------------------------
        // Element selection
        // ---------------------------
        if (IsKeyPressed(KEY_ONE))
        {
            brush_type   = ElementType::Sand;
            matter_state = MatterState::Solid;
            physics_type = Physics::Sand;
            movement_ptr = &sand_movement;
        }
        if (IsKeyPressed(KEY_TWO))
        {
            brush_type   = ElementType::Water;
            matter_state = MatterState::Liquid;
            physics_type = Physics::Water;
            movement_ptr = &liquid_movement;
        }
        if (IsKeyPressed(KEY_THREE))
        {
            brush_type   = ElementType::Stone;
            matter_state = MatterState::Solid;
            physics_type = Physics::Stone;
            movement_ptr = &sand_movement;
        }
        if (IsKeyPressed(KEY_FOUR))
        {
            brush_type   = ElementType::Gas;
            matter_state = MatterState::Gas;
            physics_type = Physics::Gas;
            movement_ptr = &gas_movement;
        }
        if (IsKeyPressed(KEY_FIVE))
        {
            brush_type   = ElementType::Custom;
            matter_state = MatterState::None;
            physics_type = Physics::Empty;
            movement_ptr = &empty_movement;
        }
        if (IsKeyPressed(KEY_SIX))
        {
            brush_type   = ElementType::Bedrock;
            matter_state = MatterState::Solid;
            physics_type = Physics::Empty;
            movement_ptr = &empty_movement;
        }
        if (IsKeyPressed(KEY_SEVEN))
        {
            brush_type   = ElementType::Empty;
            matter_state = MatterState::None;
            physics_type = Physics::Empty;
            movement_ptr = &empty_movement;
        }

        if (IsKeyPressed(KEY_EIGHT))
        {
            brush_type   = ElementType::Scaffolding;
            matter_state = MatterState::ForeGround;
            physics_type = Physics::Scaffolding;
            movement_ptr = &scaffolding_movement;
        }
        // ---------------------------
        // Mouse input (world space)
        // ---------------------------
        Vector2 mouse_screen = GetMousePosition();
        Vector2 mouse_world  = GetScreenToWorld2D(mouse_screen, camera);

        int gx = static_cast<int>(mouse_world.x) / cellSize;
        int gy = static_cast<int>(mouse_world.y) / cellSize;

        bool isPanning = shiftDown && IsMouseButtonDown(MOUSE_LEFT_BUTTON);

        if (!isPanning &&
            (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonDown(MOUSE_RIGHT_BUTTON)))
        {
            bool erase = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);

            for (int dy = -brush_size; dy <= brush_size; dy++)
            {
                for (int dx = -brush_size; dx <= brush_size; dx++)
                {
                    if (dx * dx + dy * dy <= brush_size * brush_size)
                    {
                        int px = gx + dx;
                        int py = gy + dy;

                        int cx, cy, lx, ly;
                        if (world.get_chunk_for_global_cell(px, py, cx, cy, lx, ly))
                        {
                            ElementType type_to_set = erase ? ElementType::Empty : brush_type;
                            IMovement  *move_ptr    = erase ? &empty_movement : movement_ptr;
                            MatterState matterstate = erase ? MatterState::None : matter_state;
                            Physics     physics_type_to_set = erase ? Physics::Empty : physics_type;

                            world.chunk_at(cx, cy).set_element_local(
                                lx, ly,
                                Element(matterstate, type_to_set, move_ptr, physics_type_to_set));
                        }
                    }
                }
            }
        }

        // 1. Get the time elapsed since the last frame
        float frameTime = GetFrameTime();

        // Safety cap: prevents "spiral of death" if the simulation lags
        if (frameTime > 0.25f)
        {
            frameTime = 0.25f;
        }

        accumulator += frameTime;

        // ---------------------------
        // Input & Camera (Runs every frame for smoothness)
        // ---------------------------
        // ... (Your existing KEY_P, KEY_F, and Camera logic) ...
        // Note: Use 'frameTime' for your camera panning instead of calling
        // GetFrameTime() again
        float camStep = (cam_pan_speed * frameTime) / camera.zoom;

        // ---------------------------
        // Step world (Fixed Timestep)
        // ---------------------------
        if (!FRAME_CLICK)
        {
            // While we have enough time accumulated, run a physics step
            while (accumulator >= targetDt)
            {
                world.step();
                accumulator -= targetDt;
            }
        }
        else if (step_once)
        {
            // Manual frame-by-frame still works
            world.step();
            step_once   = false;
            accumulator = 0; // Reset accumulator to prevent catch-up jumps
        }

        // ---------------------------
        // Rendering
        // ---------------------------
        BeginDrawing();
        ClearBackground(WHITE);

        BeginMode2D(camera);

        // =========================================================
        // Render ONLY active chunks (but camera still sees full world)
        // =========================================================
        for (int cy = 0; cy < CHUNK_COUNT_Y; cy++)
        {
            for (int cx = 0; cx < CHUNK_COUNT_X; cx++)
            {

                Chunk &chunk = world.chunk_at(cx, cy);

                int base_x = cx * CHUNK_SIZE_X;
                int base_y = cy * CHUNK_SIZE_Y;

                // Inactive chunk → draw faint background (cheap)
                // if (chunk.active_index == UINT32_MAX) {
                //    DrawRectangle(base_x * cellSize, base_y * cellSize,
                //                  CHUNK_SIZE_X * cellSize,
                //                  CHUNK_SIZE_Y * cellSize,
                //                  Color{230, 230, 230, 255});
                //    continue;
                //}

                // Active chunk → draw all its cells
                for (int y = 0; y < CHUNK_SIZE_Y; y++)
                {
                    for (int x = 0; x < CHUNK_SIZE_X; x++)
                    {
                        ElementType type = chunk.at(x, y).type;
                        Color       c    = element_to_color(type);

                        DrawRectangle((base_x + x) * cellSize, (base_y + y) * cellSize, cellSize,
                                      cellSize, c);
                    }
                }
            }
        }

        // --- Draw chunk outlines ---
        for (int y = 0; y < CHUNK_COUNT_Y; y++)
        {
            for (int x = 0; x < CHUNK_COUNT_X; x++)
            {
                int posx   = x * CHUNK_SIZE_X * cellSize + 1;
                int posy   = y * CHUNK_SIZE_Y * cellSize + 1;
                int width  = CHUNK_SIZE_X * cellSize - 1;
                int height = CHUNK_SIZE_Y * cellSize - 1;

                Chunk &chunk = world.chunk_at(x, y);
                DrawRectangleLines(posx, posy, width, height,
                                   (chunk.active_index == UINT32_MAX) ? RED : GREEN);
            }
        }

        // --- Draw brush preview ---
        DrawCircleLines(static_cast<int>(mouse_world.x), static_cast<int>(mouse_world.y),
                        brush_size * cellSize, RED);

        EndMode2D();

        DrawFPS(30, 30);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
