#include "element.hpp"

// Static empty element instance
static const Element EMPTY_ELEMENT(ElementType::Empty, nullptr);

const Element &Element::Empty() { return EMPTY_ELEMENT; }
