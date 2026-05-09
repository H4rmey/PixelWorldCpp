#include "element.hpp"

// Static empty element instance
static Element EMPTY_ELEMENT(MatterState::None, ElementType::Empty, nullptr,
                             Physics::Empty);
static Element OUT_OF_BOUNDS(MatterState::None, ElementType::OutOfBounds,
                             nullptr, Physics::Empty);

Element &Element::Empty() { return EMPTY_ELEMENT; }
Element &Element::OutOfBounds() { return OUT_OF_BOUNDS; }
