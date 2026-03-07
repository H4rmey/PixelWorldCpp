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
        case ElementType::Empty:  	return BLACK;
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

    InitWindow(total_width * cellSize, total_height * cellSize, "World Simulation");
    //SetTargetFPS(60);

    WorldManager world;
    IMovement *movement_ptr;
    SandMovement sand_movement;
	SolidMovement solid_movement;
	GasMovement   gas_movement;
	LiquidMovement liquid_movement;
	PowderMovement powder_movement;
	EmptyMovement empty_movement;
	movement_ptr = &sand_movement;

    int brush_size = BRUSH_SIZE; // initial brush radius
    ElementType brush_type = ElementType::Sand;


    while (!WindowShouldClose()) {

		// --- Input: Adjust brush size ---
		if (IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_EQUAL)) brush_size = std::min(90, brush_size + 1);

		// Check multiple minus variants for different layouts
		if (IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_MINUS))
			brush_size = std::max(0, brush_size - 1);

        // --- Input: Change element type with number keys ---
		if (IsKeyPressed(KEY_ONE)) { brush_type = ElementType::Sand; movement_ptr = &sand_movement; }
		if (IsKeyPressed(KEY_TWO)) { brush_type = ElementType::Water; movement_ptr = &liquid_movement; }
		if (IsKeyPressed(KEY_THREE)) { brush_type = ElementType::Stone; movement_ptr = &solid_movement; }
		if (IsKeyPressed(KEY_FOUR)) { brush_type = ElementType::Gas; movement_ptr = &gas_movement; }
		if (IsKeyPressed(KEY_FIVE)) { brush_type = ElementType::Custom; movement_ptr = &empty_movement; }
		if (IsKeyPressed(KEY_SIX)) { brush_type = ElementType::Bedrock; movement_ptr = &empty_movement; }
		if (IsKeyPressed(KEY_SEVEN)) { brush_type = ElementType::Empty; movement_ptr = &empty_movement; }
        // optionally 8/9 can be used for other custom types

        // --- Mouse input: Place / remove elements ---
        Vector2 mouse_pos = GetMousePosition();
        int gx = static_cast<int>(mouse_pos.x) / cellSize;
        int gy = static_cast<int>(mouse_pos.y) / cellSize;

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
            bool erase = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
            for (int dy = -brush_size; dy <= brush_size; dy++) {
                for (int dx = -brush_size; dx <= brush_size; dx++) {
                    if (dx*dx + dy*dy <= brush_size*brush_size) { // circle check
                        int px = gx + dx;
                        int py = gy + dy;

                        int cx, cy, lx, ly;
                        if (world.get_chunk_for_global_cell(px, py, cx, cy, lx, ly)) {
                            ElementType type_to_set = erase ? ElementType::Empty : brush_type;

                            IMovement* move_ptr;
							if (erase)
								move_ptr = &empty_movement;
							else
								move_ptr = movement_ptr;

                            world.chunk_at(cx, cy).set_element_local(lx, ly, Element(type_to_set, move_ptr));
                        }
                    }
                }
            }
        }

        world.step();

        BeginDrawing();
        ClearBackground(WHITE);

        // --- Draw all elements ---
        for (int gy = 0; gy < total_height; ++gy)
        {
            for (int gx = 0; gx < total_width; ++gx)
            {
                int cx, cy, lx, ly;
                if (world.get_chunk_for_global_cell(gx, gy, cx, cy, lx, ly))
				{
					Chunk& chunk = world.chunk_at(cx, cy);
					ElementType type = chunk.at(lx, ly).type;
					DrawRectangle(gx * cellSize, gy * cellSize, cellSize, cellSize, element_to_color(type));
				}
            }
        }

        // --- Draw chunk outlines ---
        for (int y = 0; y < CHUNK_COUNT_Y; y++)
        {
            for (int x = 0; x < CHUNK_COUNT_X; x++)
            {
                int posx = x * CHUNK_SIZE_X * cellSize + 1;
                int posy = y * CHUNK_SIZE_Y * cellSize + 1;
                int width = CHUNK_SIZE_X * cellSize - 1;
                int height = CHUNK_SIZE_Y * cellSize - 1;

				Chunk& chunk = world.chunk_at(x, y);
				if (chunk.active_index == UINT32_MAX)
				{
                	DrawRectangleLines(posx, posy, width, height, RED);
				}
				else
				{
                	DrawRectangleLines(posx, posy, width, height, GREEN);
				}
            }
        }

        // --- Draw chunk diryt rect outlines ---
        for (int y = 0; y < CHUNK_COUNT_Y; y++)
        {
            for (int x = 0; x < CHUNK_COUNT_X; x++)
            {
                int posx = x * CHUNK_SIZE_X * cellSize;
                int posy = y * CHUNK_SIZE_Y * cellSize;

				Chunk& chunk = world.chunk_at(x, y);

				Rect r = chunk.get_dirty_rect();

                int width  =  (r.max_x - r.min_x + 1) * cellSize;
                int height =  (r.max_y - r.min_y + 1) * cellSize;
				if (r.max_x == -1 || r.max_y == -1) {continue;}
                DrawRectangleLines(	posx + cellSize*r.min_x,
									posy + cellSize*r.min_y, width, height, PURPLE);
            }
        }

        // --- Draw brush preview ---
        DrawCircleLines(static_cast<int>(mouse_pos.x), static_cast<int>(mouse_pos.y), brush_size * cellSize, RED);
		DrawFPS(30, 30);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
