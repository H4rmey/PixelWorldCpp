#include "example_class.h"

using namespace godot;

void ExampleClass::_bind_methods() { }

ExampleClass::ExampleClass() {
	print_line("created");
}

ExampleClass::~ExampleClass() {
	print_line("destroyed");
}


