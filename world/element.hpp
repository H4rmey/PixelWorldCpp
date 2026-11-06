#pragma once

#include "helpers/element_type.h"
#include "helpers/physics.hpp"
#include "helpers/renderLayer.h"

class IMovement;
class Chunk;

class Element
{
  public:
    ElementType      type     = ElementType::Empty; // The single source of identity!
    RenderLayer      layer    = RenderLayer::None;
    uint32_t         color    = 0x00000000;
    const IMovement *movement = nullptr;
    Physics          physics;

    Element() = default;

    Element(RenderLayer m, ElementType t, uint32_t col, const IMovement *move, const Physics &p)
        : layer(m), type(t), color(col), movement(move), physics(p)
    {
    }

    // Type checking is now lightning fast and ignores physics changes (like falling velocity)
    inline bool is_empty() const { return type == ElementType::Empty; }
};
