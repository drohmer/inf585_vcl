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

// Compute the FFD deformation
void ffd_deform(vcl::buffer<vcl::vec3>& position, vcl::grid_3D<vcl::vec3> const& grid /** You may want to add other parameters*/);

