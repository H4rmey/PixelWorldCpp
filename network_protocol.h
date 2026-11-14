#pragma once
#include <cstdint>

#pragma pack(push, 1)

// Client -> Server: Sent when the user clicks the mouse
struct ClientInputPacket
{
    uint8_t packet_id; // Always 0 for Input
    int32_t mouse_world_x;
    int32_t mouse_world_y;
    uint8_t brush_size;
    uint8_t brush_element; // ElementType cast to uint8_t
    uint8_t is_erasing;    // 1 if right click, 0 if left click
};

// Server -> Client: Sent after a physics step
struct NetPixelUpdate
{
    int32_t  global_x;
    int32_t  global_y;
    uint8_t  element_type;
    uint32_t color; // Use the 32-bit integer directly
};
#pragma pack(pop)
