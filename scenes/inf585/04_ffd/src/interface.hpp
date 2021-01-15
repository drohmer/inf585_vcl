#pragma once

#include "vcl/vcl.hpp"

enum surface_type_enum {
	surface_cylinder,
	surface_sphere,
	surface_cube,
	surface_mesh
};

struct gui_widget {
	bool display_frame = false;
	bool surface = true;
	bool wireframe = false;
	bool display_grid_sphere = true;
	bool display_grid_edge = true;
	bool reset_grid = false;
	surface_type_enum surface_type = surface_cylinder;  // Type of surface to be deformed
};


bool display_interface(gui_widget& gui);