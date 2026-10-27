#pragma once

#include "element.hpp" // Make sure this points to your Element class file
#include "helpers/element_type.h"
#include "helpers/physics.hpp"
#include "helpers/renderLayer.h"
#include "movementSystems.hpp"

class ElementFactory
{
  private:
    // 1. Static stateless movement instances.
    // Every element of the same type shares the same movement logic pointer.
    static inline ScaffoldingMovement scaffolding_movement;
    static inline SandMovement        sand_movement;
    static inline SolidMovement       solid_movement;
    static inline GasMovement         gas_movement;
    static inline WaterMovement       water_movement; // Assuming WaterMovement exists

  public:
    // ---------------------------------------------------------
    // Core Elements
    // ---------------------------------------------------------

    static Element create_empty()
    {
        // 0x00000000 = Fully transparent black
        return Element(RenderLayer::None, ElementType::Empty, 0x00000000, nullptr, Physics::Empty);
    }

    static Element create_sand()
    {
        // 0xFFC2B280 = Standard Sand Color (AARRGGBB)
        return Element(RenderLayer::Solid, ElementType::Sand, 0xFFC2B280, &sand_movement,
                       Physics::Sand);
    }

    static Element create_water()
    {
        // 0x994444FF = Semi-transparent blue
        return Element(RenderLayer::Liquid, ElementType::Water, 0x994444FF, &water_movement,
                       Physics::Water);
    }

    static Element create_stone()
    {
        // 0xFF888888 = Gray
        return Element(RenderLayer::Solid, ElementType::Stone, 0xFF888888, &solid_movement,
                       Physics::Stone);
    }

    static Element create_gas()
    {
        // 0x55DDDDDD = Highly transparent light gray/white
        return Element(RenderLayer::Gas, ElementType::Gas, 0x55DDDDDD, &gas_movement, Physics::Gas);
    }

    static Element create_scaffolding()
    {
        // 0xFF8B4513 = SaddleBrown
        return Element(RenderLayer::Solid, ElementType::Scaffolding, 0xFF8B4513,
                       &scaffolding_movement, Physics::Scaffolding);
    }

    // ---------------------------------------------------------
    // Variations (Tweaking base physics templates)
    // ---------------------------------------------------------

    static Element create_granite()
    {
        // Granite is just Stone with tweaked physics values and a darker color
        Physics p  = Physics::Stone;
        p.hardness = 800; // Much harder to break
        p.mass     = 50;  // Heavier

        // Notice we still classify it as ElementType::Stone for generic game logic
        return Element(RenderLayer::Solid, ElementType::Stone, 0xFF444444, &solid_movement, p);
    }

    static Element create_pumice()
    {
        // Pumice is a porous stone that is extremely light
        Physics p  = Physics::Stone;
        p.hardness = 100;
        p.mass     = 15;
        p.density  = 50; // Lower density than water (100), meaning it would float!

        return Element(RenderLayer::Solid, ElementType::Stone, 0xFFBBBBBB, &solid_movement, p);
    }

    // ---------------------------------------------------------
    // Generic Spawner (Great for UI Menus or Brushes)
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
        case ElementType::Empty:
        default:
            return create_empty();
        }
    }
};
