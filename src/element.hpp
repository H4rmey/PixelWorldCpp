#pragma once

#include "helpers/element_type.h"

class IMovement;
class Chunk;

class Element {
public:
    ElementType type = ElementType::Empty;
    const IMovement* movement = nullptr;

    Element() = default;

    Element(ElementType t, const IMovement* move)
        : type(t), movement(move) {}

    inline bool is_empty() const {
        return type == ElementType::Empty;
    }

    // Returns a reference to a static empty element
    static const Element& Empty();
};

