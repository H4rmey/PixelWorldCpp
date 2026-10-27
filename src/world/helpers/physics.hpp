#pragma once

#include <cstdint>

struct Physics
{
    // player interactions
    float hardness   = 0; // how hard it is to break the blok
    float elasticity = 0; // how bouncy is the element
    float stickyness = 0; // how fast can the player move on the element
    float dampening  = 0; // fall damage negation

    // between element interactions
    float viscosity    = 0; // used for how liquidy a liquid is
    float mass         = 0; // how much a cell falls
    float density      = 0; // if a cell should "sink" into another
    float friction_x   = 0; // when in motion how fast does it tend to stop
    float friction_y   = 0; // when in motion how fast does it tend to stop
    float stability    = 0; // breaking a cell, does it also trigger other cells
    float conductivity = 0; // how good can the element conduct electricity

    // element and player interactions
    float temperature     = 0; // how warm/cold is the element
    float heat_retention  = 0; // how long does the element retain heat
    float heat_resistance = 0; // how long does it take to heat up the element

    // movement
    int8_t velocity_x             = 0;    // unused???
    int8_t velocity_y             = 0;    // unused???
    int8_t terminal_velocity_x    = 0;    // increased when falling
    int8_t terminal_velocity_y    = 0;    // increased when moving
    int8_t velocity_resolve_dir_y = 0;    // 0=none, -1=left, 1=right
    int8_t velocity_resolve_dir_x = 0;    // 0=none, -1=down, 1=up
    bool   canMove_x              = true; // can the block move on the x axis
    bool   canMove_y              = true; // can the block move on the y axis

    // used for scaffolding:
    bool is_h_anchor = false; // is the block stable on the x axis
    bool is_v_anchor = false; // is the block stable on the y axis

    // Declare the statics inside
    static Physics Empty;
    static Physics Sand;
    static Physics Water;
    static Physics Stone;
    static Physics Air;
    static Physics Gas;
    static Physics Scaffolding;
};

inline Physics Physics::Empty = {.hardness               = 0,
                                 .elasticity             = 0,
                                 .stickyness             = 0,
                                 .dampening              = 0,
                                 .viscosity              = 0,
                                 .mass                   = 0,
                                 .density                = 0,
                                 .friction_x             = 0,
                                 .friction_y             = 0,
                                 .stability              = 0,
                                 .conductivity           = 0,
                                 .temperature            = 0,
                                 .heat_retention         = 0,
                                 .heat_resistance        = 0,
                                 .velocity_x             = 0,
                                 .velocity_y             = 0,
                                 .terminal_velocity_x    = 0,
                                 .terminal_velocity_y    = 0,
                                 .velocity_resolve_dir_y = 0,
                                 .velocity_resolve_dir_x = 0,
                                 .canMove_x              = true,
                                 .canMove_y              = true,
                                 .is_h_anchor            = false,
                                 .is_v_anchor            = false};

inline Physics Physics::Sand = {.hardness               = 10,
                                .elasticity             = 0,
                                .stickyness             = 0,
                                .dampening              = 0,
                                .viscosity              = 200,
                                .mass                   = 8,
                                .density                = 160,
                                .friction_x             = 0.1f,
                                .friction_y             = 0.1f,
                                .stability              = 0.1f,
                                .conductivity           = 0,
                                .temperature            = 20,
                                .heat_retention         = 0,
                                .heat_resistance        = 0,
                                .velocity_x             = 0,
                                .velocity_y             = 0,
                                .terminal_velocity_x    = 100,
                                .terminal_velocity_y    = 100,
                                .velocity_resolve_dir_y = 0,
                                .velocity_resolve_dir_x = 0,
                                .canMove_x              = true,
                                .canMove_y              = true,
                                .is_h_anchor            = false,
                                .is_v_anchor            = false};

inline Physics Physics::Water = {.hardness               = 0,
                                 .elasticity             = 5,
                                 .stickyness             = 0,
                                 .dampening              = 0,
                                 .viscosity              = 6,
                                 .mass                   = 100,
                                 .density                = 100,
                                 .friction_x             = 0.1f,
                                 .friction_y             = 0.1f,
                                 .stability              = 0.5f,
                                 .conductivity           = 0,
                                 .temperature            = 20,
                                 .heat_retention         = 0,
                                 .heat_resistance        = 0,
                                 .velocity_x             = 0,
                                 .velocity_y             = 0,
                                 .terminal_velocity_x    = 100,
                                 .terminal_velocity_y    = 100,
                                 .velocity_resolve_dir_y = 0,
                                 .velocity_resolve_dir_x = 0,
                                 .canMove_x              = true,
                                 .canMove_y              = true,
                                 .is_h_anchor            = false,
                                 .is_v_anchor            = false};

inline Physics Physics::Stone = {.hardness               = 200,
                                 .elasticity             = 0,
                                 .stickyness             = 0,
                                 .dampening              = 0,
                                 .viscosity              = 0,
                                 .mass                   = 32,
                                 .density                = 250,
                                 .friction_x             = 0.9f,
                                 .friction_y             = 0.9f,
                                 .stability              = 0.9f,
                                 .conductivity           = 0,
                                 .temperature            = 20,
                                 .heat_retention         = 0,
                                 .heat_resistance        = 0,
                                 .velocity_x             = 0,
                                 .velocity_y             = 0,
                                 .terminal_velocity_x    = 0,
                                 .terminal_velocity_y    = 0,
                                 .velocity_resolve_dir_y = 0,
                                 .velocity_resolve_dir_x = 0,
                                 .canMove_x              = true,
                                 .canMove_y              = true,
                                 .is_h_anchor            = false,
                                 .is_v_anchor            = false};

inline Physics Physics::Air = {.hardness               = 0,
                               .elasticity             = 0,
                               .stickyness             = 0,
                               .dampening              = 0,
                               .viscosity              = 10,
                               .mass                   = 1,
                               .density                = 1,
                               .friction_x             = 0,
                               .friction_y             = 0,
                               .stability              = 0.5f,
                               .conductivity           = 0,
                               .temperature            = 20,
                               .heat_retention         = 0,
                               .heat_resistance        = 0,
                               .velocity_x             = 0,
                               .velocity_y             = 0,
                               .terminal_velocity_x    = 25,
                               .terminal_velocity_y    = 50,
                               .velocity_resolve_dir_y = 0,
                               .velocity_resolve_dir_x = 0,
                               .canMove_x              = true,
                               .canMove_y              = true,
                               .is_h_anchor            = false,
                               .is_v_anchor            = false};

inline Physics Physics::Gas = {.hardness               = 0,
                               .elasticity             = 0,
                               .stickyness             = 0,
                               .dampening              = 0,
                               .viscosity              = 5,
                               .mass                   = 2,
                               .density                = 2,
                               .friction_x             = 0,
                               .friction_y             = 0,
                               .stability              = 0.5f,
                               .conductivity           = 0,
                               .temperature            = 40,
                               .heat_retention         = 0,
                               .heat_resistance        = 0,
                               .velocity_x             = 0,
                               .velocity_y             = 0,
                               .terminal_velocity_x    = 25,
                               .terminal_velocity_y    = 30,
                               .velocity_resolve_dir_y = 0,
                               .velocity_resolve_dir_x = 0,
                               .canMove_x              = true,
                               .canMove_y              = true,
                               .is_h_anchor            = false,
                               .is_v_anchor            = false};

inline Physics Physics::Scaffolding = {.hardness               = 10,
                                       .elasticity             = 0,
                                       .stickyness             = 0,
                                       .dampening              = 0,
                                       .viscosity              = 200,
                                       .mass                   = 150,
                                       .density                = 160,
                                       .friction_x             = 0.1f,
                                       .friction_y             = 0.1f,
                                       .stability              = 0.1f,
                                       .conductivity           = 0,
                                       .temperature            = 20,
                                       .heat_retention         = 0,
                                       .heat_resistance        = 0,
                                       .velocity_x             = 0,
                                       .velocity_y             = 0,
                                       .terminal_velocity_x    = 100,
                                       .terminal_velocity_y    = 100,
                                       .velocity_resolve_dir_y = 0,
                                       .velocity_resolve_dir_x = 0,
                                       .canMove_x              = true,
                                       .canMove_y              = true,
                                       .is_h_anchor            = false,
                                       .is_v_anchor            = false};
