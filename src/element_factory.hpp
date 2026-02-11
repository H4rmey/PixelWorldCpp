#pragma once

#include "movement_systems.hpp"

namespace godot {

class ElementFactory {
public:
	// Global movement components (singletons)
	inline static SolidMovement solidMovement{};
	inline static SandMovement sandMovement{};
	inline static WaterMovement waterMovement{};
	inline static GasMovement gasMovement{};
	inline static LiquidMovement liquidMovement{};
	inline static PowderMovement powderMovement{};
	inline static SolidMovement emptyMovement{}; // empty is just immobile

public:
	static Element make_empty() {
		return Element(ElementType::Empty, &emptyMovement);
	}

	static Element make_sand() {
		return Element(ElementType::Sand, &sandMovement);
	}

	static Element make_water() {
		return Element(ElementType::Water, &waterMovement);
	}

	static Element make_gas() {
		return Element(ElementType::Gas, &gasMovement);
	}

	static Element make_liquid() {
		return Element(ElementType::Custom, &liquidMovement);
	}

	static Element make_powder() {
		return Element(ElementType::Custom, &powderMovement);
	}

	static Element make_solid() {
		return Element(ElementType::Stone, &solidMovement);
	}
};
} // godot namespace
