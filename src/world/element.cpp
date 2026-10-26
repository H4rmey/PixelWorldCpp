#include "element.hpp"

// Static empty element instance
static Element EMPTY_ELEMENT(RenderLayer::None, ElementType::Empty, 0, nullptr, Physics::Empty);
static Element OUT_OF_BOUNDS(RenderLayer::None, ElementType::OutOfBounds, 0, nullptr,
                             Physics::Empty);

Element &Element::Empty() { return EMPTY_ELEMENT; }
Element &Element::OutOfBounds() { return OUT_OF_BOUNDS; }
