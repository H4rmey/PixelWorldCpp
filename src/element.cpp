#include "element.hpp"

// Static empty element instance
static Element EMPTY_ELEMENT(MatterState::None, ElementType::Empty, nullptr,
                             Physics::Empty);

Element &Element::Empty() { return EMPTY_ELEMENT; }
