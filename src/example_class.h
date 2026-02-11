#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "world_manager.hpp"

namespace godot {

class ExampleClass : public RefCounted {
    GDCLASS(ExampleClass, RefCounted)

private:
	WorldManager m_world_manager;


protected:
	static void _bind_methods();

public:
    ExampleClass();
    ~ExampleClass();
};

} // namespace godot

