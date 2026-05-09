#pragma once

#include "helpers/element_type.h"
#include "helpers/matter_state.h"
#include "helpers/physics.hpp"

class IMovement;
class Chunk;

class Element {
  public:
    ElementType      type     = ElementType::Empty;
    MatterState      state    = MatterState::None;
    const IMovement *movement = nullptr;
    Physics          physics;

    Element() = default;

    Element(MatterState m, ElementType t, const IMovement *move,
            const Physics &p)
        : state(m), type(t), movement(move), physics(p) {}

    inline bool is_empty() const { return type == ElementType::Empty; }

    // Returns a reference to a static empty element
    static Element &Empty();
};
