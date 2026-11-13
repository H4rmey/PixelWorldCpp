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
    static inline LiquidMovement      liquid_movement;
    static inline EmptyMovement       empty_movement;

  public:
    // ---------------------------------------------------------
    // Core Elements
    // ---------------------------------------------------------

    static Element create_empty()
    {
        // 0x828282FF = Gray (130, 130, 130, 255)
        return Element(RenderLayer::None, ElementType::Empty, 0x828282FF, &empty_movement,
                       Physics::Empty);
    }

    static Element create_sand()
    {
        Physics p   = Physics::Sand;
        p.canMove_x = false;
        p.canMove_y = false;
        // 0xC2B280FF = Sand/Tan
        return Element(RenderLayer::Solid, ElementType::Sand, 0xC2B280FF, &sand_movement, p);
    }

    static Element create_water()
    {
        // 0x4444FFFF = Blue
        return Element(RenderLayer::Liquid, ElementType::Water, 0x4444FFFF, &liquid_movement,
                       Physics::Water);
    }

    static Element create_stone()
    {
        // 0x888888FF = Dark Gray
        return Element(RenderLayer::Solid, ElementType::Stone, 0x888888FF, &solid_movement,
                       Physics::Stone);
    }

    static Element create_gas()
    {
        // 0x55DDDDFF = Light Cyan/Greenish
        return Element(RenderLayer::Gas, ElementType::Gas, 0x55DDDDFF, &gas_movement, Physics::Gas);
    }

    static Element create_scaffolding()
    {
        // 0x8B4513FF = Brown
        return Element(RenderLayer::Solid, ElementType::Scaffolding, 0x8B4513FF,
                       &scaffolding_movement, Physics::Scaffolding);
    }

    static Element create_bedrock()
    {
        // 0x000000FF = Black
        return Element(RenderLayer::Solid, ElementType::Bedrock, 0x000000FF, &empty_movement,
                       Physics::Empty);
    }

    static Element create_custom()
    {
        // 0x800080FF = Purple
        return Element(RenderLayer::None, ElementType::Custom, 0x800080FF, &empty_movement,
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
        // 0x444444FF = Very Dark Gray
        return Element(RenderLayer::Solid, ElementType::Stone, 0x444444FF, &solid_movement, p);
    }

    static Element create_pumice()
    {
        Physics p  = Physics::Stone;
        p.hardness = 100;
        p.mass     = 15;
        p.density  = 50;
        // 0xBBBBBBFF = Light Gray
        return Element(RenderLayer::Solid, ElementType::Stone, 0xBBBBBBFF, &solid_movement, p);
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
