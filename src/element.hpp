#pragma once

#include "element_type.hpp"

namespace godot {

class IMovement;
class Chunk;

class Element {
public:
	ElementType type = ElementType::Empty;

	// Component pointer (non-owning)
	const IMovement* movement = nullptr;

	Element() = default;

	Element(ElementType t, const IMovement *move) :
			type(t), movement(move) {}

	inline bool is_empty() const {
		return type == ElementType::Empty;
	}

	static const Element &Empty() {
		static Element empty(ElementType::Empty, nullptr);
		return empty;
	}
};
} // godot namespace
