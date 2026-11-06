#pragma once

#include "element.hpp"
#include "helpers/element_type.h"
#include "helpers/physics.hpp"
#include "helpers/renderLayer.h"
#include "movementSystems.hpp"

class ElementFactory
{
  private:
    // 1. Static stateless movement instances.
    static inline ScaffoldingMovement scaffolding_movement;
    static inline SandMovement        sand_movement;
    static inline SolidMovement       solid_movement;
    static inline GasMovement         gas_movement;
    static inline LiquidMovement      liquid_movement; // Fixed from WaterMovement
    static inline EmptyMovement       empty_movement;  // Added to replace nullptr

  public:
    // ---------------------------------------------------------
    // Core Elements
    // ---------------------------------------------------------

    static Element create_empty()
    {
        // Passed &empty_movement instead of nullptr
        return Element(RenderLayer::None, ElementType::Empty, 0x00000000, &empty_movement,
                       Physics::Empty);
    }

    static Element create_sand()
    {
        Physics p   = Physics::Sand;
        p.canMove_x = false;
        p.canMove_y = false;
        return Element(RenderLayer::Solid, ElementType::Sand, 0xFFC2B280, &sand_movement, p);
    }

    static Element create_water()
    {
        return Element(RenderLayer::Liquid, ElementType::Water, 0x994444FF, &liquid_movement,
                       Physics::Water);
    }

    static Element create_stone()
    {
        return Element(RenderLayer::Solid, ElementType::Stone, 0xFF888888, &solid_movement,
                       Physics::Stone);
    }

    static Element create_gas()
    {
        return Element(RenderLayer::Gas, ElementType::Gas, 0x55DDDDDD, &gas_movement, Physics::Gas);
    }

    static Element create_scaffolding()
    {
        return Element(RenderLayer::Solid, ElementType::Scaffolding, 0xFF8B4513,
                       &scaffolding_movement, Physics::Scaffolding);
    }

    static Element create_bedrock()
    {
        // Mirrors KEY_SIX in main.cpp
        return Element(RenderLayer::Solid, ElementType::Bedrock, 0xFF000000, &empty_movement,
                       Physics::Empty);
    }

    static Element create_custom()
    {
        // Mirrors KEY_FIVE in main.cpp
        return Element(RenderLayer::None, ElementType::Custom, 0xFF800080, &empty_movement,
                       Physics::Empty);
    }

    // ---------------------------------------------------------
    // Variations (Tweaking base physics templates)
    // ---------------------------------------------------------

    static Element create_granite()
    {
        Physics p  = Physics::Stone;
        p.hardness = 800;
        p.mass     = 50;

        return Element(RenderLayer::Solid, ElementType::Stone, 0xFF444444, &solid_movement, p);
    }

    static Element create_pumice()
    {
        Physics p  = Physics::Stone;
        p.hardness = 100;
        p.mass     = 15;
        p.density  = 50;

        return Element(RenderLayer::Solid, ElementType::Stone, 0xFFBBBBBB, &solid_movement, p);
    }

    // ---------------------------------------------------------
    // Generic Spawner
    // ---------------------------------------------------------

    static Element create(ElementType type)
    {
        switch (type)
        {
        case ElementType::Sand:
            return create_sand();
        case ElementType::Water:
            return create_water();
        case ElementType::Stone:
            return create_stone();
        case ElementType::Gas:
            return create_gas();
        case ElementType::Scaffolding:
            return create_scaffolding();
        case ElementType::Bedrock:
            return create_bedrock();
        case ElementType::Custom:
            return create_custom();
        case ElementType::Empty:
        default:
            return create_empty();
        }
    }
};
