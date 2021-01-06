#pragma once

#include "vcl/vcl.hpp"

enum surface_type_enum {
	surface_plane,
	surface_cylinder,
	surface_sphere,
	surface_cube,
	surface_mesh
};

enum deformer_type_enum {
	deform_translate,
	deform_twist,
	deform_scale
};

enum deformer_direction_enum {
	direction_view_space,
	direction_surface_normal
};

struct gui_widget {
	bool display_frame = true;
	surface_type_enum surface_type = surface_plane;                // Type of surface to be deformed
	deformer_type_enum deformer_type = deform_translate;           // Type of deformation type
	deformer_direction_enum deformer_direction = direction_view_space;  // Type of deformation direction
	bool wireframe = false;   // Display wireframe
	float falloff = 1/5.0f;   // Falloff distance (can be adjusted from the GUI or with mouse scroll)
};


bool display_interface(gui_widget& gui);