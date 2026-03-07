#include "movement/IMovement.h"
#include "raylib.h"
#include "world_manager.hpp"
#include "helpers/constants.h"
#include "helpers/element_type.h"
#include "movement_systems.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>

Color element_to_color(ElementType type) {
    switch (type) {
        case ElementType::Empty:  	return GRAY;
        case ElementType::Sand:   	return YELLOW;
        case ElementType::Water:  	return BLUE;
        case ElementType::Stone:  	return GRAY;
        case ElementType::Gas:    	return GREEN;
        case ElementType::Bedrock:	return LIGHTGRAY;
        case ElementType::Custom: 	return PURPLE;
        default: return WHITE;
    }
}
int main() {
    const int cellSize = CELL_SIZE;

    const int total_width  = CHUNK_COUNT_X * CHUNK_SIZE_X;
    const int total_height = CHUNK_COUNT_Y * CHUNK_SIZE_Y;

    const int screenW = total_width  * cellSize;
    const int screenH = total_height * cellSize;

    InitWindow(screenW, screenH, "World Simulation");
    //SetTargetFPS(60);

    static WorldManager world;
    IMovement *movement_ptr;
    SandMovement  sand_movement;
    SolidMovement solid_movement;
    GasMovement   gas_movement;
    LiquidMovement liquid_movement;
    PowderMovement powder_movement;
    EmptyMovement  empty_movement;
    movement_ptr = &sand_movement;

    int brush_size = BRUSH_SIZE; // initial brush radius
    ElementType brush_type = ElementType::Sand;

    // ---------------------------
    // Camera setup
    // ---------------------------
    Camera2D camera = {};
    camera.target = { 0.0f, 0.0f };                 // world-space target at (0,0) so old behavior matches at zoom=1
    camera.offset = { 0.0f, 0.0f };                 // screen-space offset (top-left anchored, same as your old drawing)
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // Camera tuning
    const float cam_pan_speed = 900.0f;             // pixels/sec in world units at zoom=1 (arrow keys)
    const float zoom_step = 1.10f;                  // 10% per key press
    const float zoom_min  = 0.10f;
    const float zoom_max  = 12.0f;

    while (!WindowShouldClose()) {

        const bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        // ---------------------------
        // Zoom (SHIFT + +/-)
        // ---------------------------
        if (shiftDown) {
            bool zoomIn  = (IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_EQUAL));   // '=' becomes '+' with shift on many layouts
            bool zoomOut = (IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_MINUS));

            if (zoomIn || zoomOut) {
                Vector2 mouse_screen = GetMousePosition();
                Vector2 mouse_world_before = GetScreenToWorld2D(mouse_screen, camera);

                float newZoom = camera.zoom * (zoomIn ? zoom_step : (1.0f / zoom_step));
                newZoom = std::clamp(newZoom, zoom_min, zoom_max);
                camera.zoom = newZoom;

                // Keep the point under the mouse stable (zoom around cursor)
                Vector2 mouse_world_after = GetScreenToWorld2D(mouse_screen, camera);
                camera.target.x += (mouse_world_before.x - mouse_world_after.x);
                camera.target.y += (mouse_world_before.y - mouse_world_after.y);
            }
        } else {
            // ---------------------------
            // Brush size (+/-) (only when NOT holding SHIFT)
            // ---------------------------
            if (IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_EQUAL))
                brush_size = std::min(90, brush_size + 1);

            if (IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_MINUS))
                brush_size = std::max(0, brush_size - 1);
        }

        // ---------------------------
        // Pan with SHIFT + LMB drag ("grab" style)
        // ---------------------------
        if (shiftDown && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Vector2 d = GetMouseDelta(); // screen-space delta since last frame
            // grab style: dragging right moves world right -> camera target moves left
            camera.target.x -= d.x / camera.zoom;
            camera.target.y -= d.y / camera.zoom;
        }

        // ---------------------------
        // Pan with arrow keys
        // ---------------------------
        {
            float dt = GetFrameTime();
            float step = (cam_pan_speed * dt) / camera.zoom; // keep speed consistent at different zooms

            if (IsKeyDown(KEY_RIGHT)) camera.target.x += step;
            if (IsKeyDown(KEY_LEFT))  camera.target.x -= step;
            if (IsKeyDown(KEY_DOWN))  camera.target.y += step;
            if (IsKeyDown(KEY_UP))    camera.target.y -= step;
        }

        // ---------------------------
        // Element selection
        // ---------------------------
        if (IsKeyPressed(KEY_ONE))   { brush_type = ElementType::Sand;    movement_ptr = &sand_movement; }
        if (IsKeyPressed(KEY_TWO))   { brush_type = ElementType::Water;   movement_ptr = &liquid_movement; }
        if (IsKeyPressed(KEY_THREE)) { brush_type = ElementType::Stone;   movement_ptr = &solid_movement; }
        if (IsKeyPressed(KEY_FOUR))  { brush_type = ElementType::Gas;     movement_ptr = &gas_movement; }
        if (IsKeyPressed(KEY_FIVE))  { brush_type = ElementType::Custom;  movement_ptr = &empty_movement; }
        if (IsKeyPressed(KEY_SIX))   { brush_type = ElementType::Bedrock; movement_ptr = &empty_movement; }
        if (IsKeyPressed(KEY_SEVEN)) { brush_type = ElementType::Empty;   movement_ptr = &empty_movement; }

        // ---------------------------
        // Mouse input: paint/erase (MUST use world mouse position now)
        // ---------------------------
        Vector2 mouse_screen = GetMousePosition();
        Vector2 mouse_world  = GetScreenToWorld2D(mouse_screen, camera);

        int gx = static_cast<int>(mouse_world.x) / cellSize;
        int gy = static_cast<int>(mouse_world.y) / cellSize;

        // optional: prevent painting while you are panning with SHIFT+LMB
        const bool isPanning = shiftDown && IsMouseButtonDown(MOUSE_LEFT_BUTTON);

        if (!isPanning && (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonDown(MOUSE_RIGHT_BUTTON))) {
            bool erase = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);

            for (int dy = -brush_size; dy <= brush_size; dy++) {
                for (int dx = -brush_size; dx <= brush_size; dx++) {
                    if (dx*dx + dy*dy <= brush_size*brush_size) { // circle
                        int px = gx + dx;
                        int py = gy + dy;

                        int cx, cy, lx, ly;
                        if (world.get_chunk_for_global_cell(px, py, cx, cy, lx, ly)) {
                            ElementType type_to_set = erase ? ElementType::Empty : brush_type;

                            IMovement* move_ptr = erase ? &empty_movement : movement_ptr;
                            world.chunk_at(cx, cy).set_element_local(lx, ly, Element(type_to_set, move_ptr));
                        }
                    }
                }
            }
        }

        world.step();

        BeginDrawing();
        ClearBackground(WHITE);

        // =========================================================
        // World rendering under camera
        // =========================================================
        BeginMode2D(camera);

        // --- Draw all elements ---
        for (int gy2 = 0; gy2 < total_height; ++gy2) {
            for (int gx2 = 0; gx2 < total_width; ++gx2) {
                int cx, cy, lx, ly;
                if (world.get_chunk_for_global_cell(gx2, gy2, cx, cy, lx, ly)) {
                    Chunk& chunk = world.chunk_at(cx, cy);
                    ElementType type = chunk.at(lx, ly).type;
                    DrawRectangle(gx2 * cellSize, gy2 * cellSize, cellSize, cellSize, element_to_color(type));
                }
            }
        }

        // --- Draw chunk outlines ---
        for (int y = 0; y < CHUNK_COUNT_Y; y++) {
            for (int x = 0; x < CHUNK_COUNT_X; x++) {
                int posx = x * CHUNK_SIZE_X * cellSize + 1;
                int posy = y * CHUNK_SIZE_Y * cellSize + 1;
                int width  = CHUNK_SIZE_X * cellSize - 1;
                int height = CHUNK_SIZE_Y * cellSize - 1;

                Chunk& chunk = world.chunk_at(x, y);
                DrawRectangleLines(posx, posy, width, height, (chunk.active_index == UINT32_MAX) ? RED : GREEN);
            }
        }

        // --- Draw chunk dirty rect outlines ---
        for (int y = 0; y < CHUNK_COUNT_Y; y++) {
            for (int x = 0; x < CHUNK_COUNT_X; x++) {
                int posx = x * CHUNK_SIZE_X * cellSize;
                int posy = y * CHUNK_SIZE_Y * cellSize;

                Chunk& chunk = world.chunk_at(x, y);
                Rect r = chunk.get_dirty_rect();

                if (r.max_x == -1 || r.max_y == -1) continue;

                int width  = (r.max_x - r.min_x + 1) * cellSize;
                int height = (r.max_y - r.min_y + 1) * cellSize;

                DrawRectangleLines(
                    posx + cellSize * r.min_x,
                    posy + cellSize * r.min_y,
                    width, height, PURPLE
                );
            }
        }

        // --- Draw brush preview in WORLD space ---
        DrawCircleLines(
            static_cast<int>(mouse_world.x),
            static_cast<int>(mouse_world.y),
            brush_size * cellSize,
            RED
        );

		EndMode2D();

        // =========================================================
        // UI overlay (screen space)
        // =========================================================
        DrawFPS(30, 30);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

//int main() {
//    const int cellSize = CELL_SIZE;
//
//    const int total_width  = CHUNK_COUNT_X * CHUNK_SIZE_X;
//    const int total_height = CHUNK_COUNT_Y * CHUNK_SIZE_Y;
//
//    InitWindow(total_width * cellSize, total_height * cellSize, "World Simulation");
//    //SetTargetFPS(60);
//
//    static WorldManager world;
//    IMovement *movement_ptr;
//    SandMovement sand_movement;
//	SolidMovement solid_movement;
//	GasMovement   gas_movement;
//	LiquidMovement liquid_movement;
//	PowderMovement powder_movement;
//	EmptyMovement empty_movement;
//	movement_ptr = &sand_movement;
//
//    int brush_size = BRUSH_SIZE; // initial brush radius
//    ElementType brush_type = ElementType::Sand;
//
//
//    while (!WindowShouldClose()) {
//
//		// --- Input: Adjust brush size ---
//		if (IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_EQUAL)) brush_size = std::min(90, brush_size + 1);
//
//		// Check multiple minus variants for different layouts
//		if (IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_MINUS))
//			brush_size = std::max(0, brush_size - 1);
//
//        // --- Input: Change element type with number keys ---
//		if (IsKeyPressed(KEY_ONE)) { brush_type = ElementType::Sand; movement_ptr = &sand_movement; }
//		if (IsKeyPressed(KEY_TWO)) { brush_type = ElementType::Water; movement_ptr = &liquid_movement; }
//		if (IsKeyPressed(KEY_THREE)) { brush_type = ElementType::Stone; movement_ptr = &solid_movement; }
//		if (IsKeyPressed(KEY_FOUR)) { brush_type = ElementType::Gas; movement_ptr = &gas_movement; }
//		if (IsKeyPressed(KEY_FIVE)) { brush_type = ElementType::Custom; movement_ptr = &empty_movement; }
//		if (IsKeyPressed(KEY_SIX)) { brush_type = ElementType::Bedrock; movement_ptr = &empty_movement; }
//		if (IsKeyPressed(KEY_SEVEN)) { brush_type = ElementType::Empty; movement_ptr = &empty_movement; }
//        // optionally 8/9 can be used for other custom types
//
//        // --- Mouse input: Place / remove elements ---
//        Vector2 mouse_pos = GetMousePosition();
//        int gx = static_cast<int>(mouse_pos.x) / cellSize;
//        int gy = static_cast<int>(mouse_pos.y) / cellSize;
//
//        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
//            bool erase = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
//            for (int dy = -brush_size; dy <= brush_size; dy++) {
//                for (int dx = -brush_size; dx <= brush_size; dx++) {
//                    if (dx*dx + dy*dy <= brush_size*brush_size) { // circle check
//                        int px = gx + dx;
//                        int py = gy + dy;
//
//                        int cx, cy, lx, ly;
//                        if (world.get_chunk_for_global_cell(px, py, cx, cy, lx, ly)) {
//                            ElementType type_to_set = erase ? ElementType::Empty : brush_type;
//
//                            IMovement* move_ptr;
//							if (erase)
//								move_ptr = &empty_movement;
//							else
//								move_ptr = movement_ptr;
//
//                            world.chunk_at(cx, cy).set_element_local(lx, ly, Element(type_to_set, move_ptr));
//                        }
//                    }
//                }
//            }
//        }
//
//        world.step();
//
//        BeginDrawing();
//        ClearBackground(WHITE);
//
//        // --- Draw all elements ---
//        for (int gy = 0; gy < total_height; ++gy)
//        {
//            for (int gx = 0; gx < total_width; ++gx)
//            {
//                int cx, cy, lx, ly;
//                if (world.get_chunk_for_global_cell(gx, gy, cx, cy, lx, ly))
//				{
//					Chunk& chunk = world.chunk_at(cx, cy);
//					ElementType type = chunk.at(lx, ly).type;
//					DrawRectangle(gx * cellSize, gy * cellSize, cellSize, cellSize, element_to_color(type));
//				}
//            }
//        }
//
//        // --- Draw chunk outlines ---
//        for (int y = 0; y < CHUNK_COUNT_Y; y++)
//        {
//            for (int x = 0; x < CHUNK_COUNT_X; x++)
//            {
//                int posx = x * CHUNK_SIZE_X * cellSize + 1;
//                int posy = y * CHUNK_SIZE_Y * cellSize + 1;
//                int width = CHUNK_SIZE_X * cellSize - 1;
//                int height = CHUNK_SIZE_Y * cellSize - 1;
//
//				Chunk& chunk = world.chunk_at(x, y);
//				if (chunk.active_index == UINT32_MAX)
//				{
//                	DrawRectangleLines(posx, posy, width, height, RED);
//				}
//				else
//				{
//                	DrawRectangleLines(posx, posy, width, height, GREEN);
//				}
//            }
//        }
//
//        // --- Draw chunk diryt rect outlines ---
//        for (int y = 0; y < CHUNK_COUNT_Y; y++)
//        {
//            for (int x = 0; x < CHUNK_COUNT_X; x++)
//            {
//                int posx = x * CHUNK_SIZE_X * cellSize;
//                int posy = y * CHUNK_SIZE_Y * cellSize;
//
//				Chunk& chunk = world.chunk_at(x, y);
//
//				Rect r = chunk.get_dirty_rect();
//
//                int width  =  (r.max_x - r.min_x + 1) * cellSize;
//                int height =  (r.max_y - r.min_y + 1) * cellSize;
//				if (r.max_x == -1 || r.max_y == -1) {continue;}
//                DrawRectangleLines(	posx + cellSize*r.min_x,
//									posy + cellSize*r.min_y, width, height, PURPLE);
//            }
//        }
//
//        // --- Draw brush preview ---
//        DrawCircleLines(static_cast<int>(mouse_pos.x), static_cast<int>(mouse_pos.y), brush_size * cellSize, RED);
//		DrawFPS(30, 30);
//
//        EndDrawing();
//    }
//
//    CloseWindow();
//    return 0;
//}
