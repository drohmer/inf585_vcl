#pragma once

#include "interface.hpp"
#include "vcl/vcl.hpp"

// current context of picking
struct picking_parameters {
	bool active;              // true if a vertex has been selected
	int index;                // the index of the selected vertex
	vcl::vec2 screen_clicked; // 2D position on screen where the mouse was clicked when the picking occured
	vcl::vec3 p_clicked;      // The 3D position corresponding to the picking
	vcl::vec3 n_clicked;      // The normal of the shape at the picked position (when picking occured)
};

// Deform the shape with respect to the 2D interactive gesture represented by the translation vector tr
void apply_deformation(vcl::mesh& shape, 
	vcl::vec2 const& tr,
	vcl::buffer<vcl::vec3> const& position_before_deformation, 
	vcl::buffer<vcl::vec3> const& normal_before_deformation, 
	gui_widget const& widget, 
	picking_parameters const& picking, 
	vcl::rotation const& camera_orientation);