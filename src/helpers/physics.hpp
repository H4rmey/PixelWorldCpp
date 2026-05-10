#pragma once

#include <cstdint>

struct Physics {
    // matter
    uint8_t viscosity   = 0;
    uint8_t mass        = 0;
    uint8_t density     = 0;
    uint8_t hardness    = 0;
    uint8_t temperature = 0;
    uint8_t volume      = 0;
    uint8_t pressure    = 0;
    uint8_t elasticity  = 0;
    float   friction    = 0;
    float   stability   = 0;

    // movement
    int8_t velocity_x             = 0;
    int8_t velocity_y             = 0;
    int8_t terminal_velocity_x    = 0;
    int8_t terminal_velocity_y    = 0;
    int8_t velocity_resolve_dir_y = 0; // 0=none, -1=left, 1=right
    int8_t velocity_resolve_dir_x = 0; // 0=none, -1=down, 1=up
    bool   canMove_x              = true;
    bool   canMove_y              = true;
    bool   is_v_anchor            = false;
    bool   is_h_anchor            = false;

    // Declare the statics inside
    static Physics Empty;
    static Physics Sand;
    static Physics Water;
    static Physics Stone;
    static Physics Air;
    static Physics Gas;
    static Physics Scaffolding;

    void set_terminal_velocity_y(int new_vel_y) {
        terminal_velocity_y = new_vel_y;
    }
};

// Define them immediately after (still in the .hpp)
inline Physics Physics::Empty = {.viscosity           = 0,
                                 .mass                = 0,
                                 .density             = 0,
                                 .hardness            = 0,
                                 .temperature         = 0,
                                 .volume              = 0,
                                 .pressure            = 0,
                                 .elasticity          = 0,
                                 .friction            = 0,
                                 .stability           = 0,
                                 .velocity_x          = 0,
                                 .velocity_y          = 0,
                                 .terminal_velocity_x = 0,
                                 .terminal_velocity_y = 0,
                                 .canMove_x           = true,
                                 .canMove_y           = true};

inline Physics Physics::Sand = {.viscosity           = 200,
                                .mass                = 150,
                                .density             = 160,
                                .hardness            = 10,
                                .temperature         = 20,
                                .volume              = 1,
                                .pressure            = 0,
                                .elasticity          = 0,
                                .friction            = 0.1f,
                                .stability           = 0.1f,
                                .velocity_x          = 0,
                                .velocity_y          = 0,
                                .terminal_velocity_x = 100,
                                .terminal_velocity_y = 100,
                                .canMove_x           = true,
                                .canMove_y           = true};

inline Physics Physics::Water = {.viscosity           = 100,
                                 .mass                = 100,
                                 .density             = 100,
                                 .hardness            = 0,
                                 .temperature         = 20,
                                 .volume              = 1,
                                 .pressure            = 0,
                                 .elasticity          = 5,
                                 .friction            = 0.1f,
                                 .stability           = 0.5f,
                                 .velocity_x          = 0,
                                 .velocity_y          = 0,
                                 .terminal_velocity_x = 100,
                                 .terminal_velocity_y = 100,
                                 .canMove_x           = true,
                                 .canMove_y           = true};

inline Physics Physics::Stone = {.viscosity           = 255,
                                 .mass                = 250,
                                 .density             = 250,
                                 .hardness            = 200,
                                 .temperature         = 20,
                                 .volume              = 1,
                                 .pressure            = 0,
                                 .elasticity          = 0,
                                 .friction            = 0,
                                 .stability           = 0.5f,
                                 .velocity_x          = 0,
                                 .velocity_y          = 0,
                                 .terminal_velocity_x = 0,
                                 .terminal_velocity_y = 0,
                                 .canMove_x           = false,
                                 .canMove_y           = false};

inline Physics Physics::Air = {.viscosity           = 10,
                               .mass                = 1,
                               .density             = 1,
                               .hardness            = 0,
                               .temperature         = 20,
                               .volume              = 1,
                               .pressure            = 0,
                               .elasticity          = 0,
                               .friction            = 0,
                               .stability           = 0.5f,
                               .velocity_x          = 0,
                               .velocity_y          = 0,
                               .terminal_velocity_x = 25,
                               .terminal_velocity_y = 50,
                               .canMove_x           = true,
                               .canMove_y           = true};

inline Physics Physics::Gas = {.viscosity           = 5,
                               .mass                = 2,
                               .density             = 2,
                               .hardness            = 0,
                               .temperature         = 40,
                               .volume              = 1,
                               .pressure            = 10,
                               .elasticity          = 0,
                               .friction            = 0,
                               .stability           = 0.5f,
                               .velocity_x          = 0,
                               .velocity_y          = 0,
                               .terminal_velocity_x = 25,
                               .terminal_velocity_y = 30,
                               .canMove_x           = true,
                               .canMove_y           = true};

inline Physics Physics::Scaffolding = {.viscosity           = 200,
                                       .mass                = 150,
                                       .density             = 160,
                                       .hardness            = 10,
                                       .temperature         = 20,
                                       .volume              = 1,
                                       .pressure            = 0,
                                       .elasticity          = 0,
                                       .friction            = 0.1f,
                                       .stability           = 0.1f,
                                       .velocity_x          = 0,
                                       .velocity_y          = 0,
                                       .terminal_velocity_x = 100,
                                       .terminal_velocity_y = 100,
                                       .canMove_x           = true,
                                       .canMove_y           = true};
