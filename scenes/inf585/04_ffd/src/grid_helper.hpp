#pragma once

#include "vcl/vcl.hpp"

vcl::grid_3D<vcl::vec3> initialize_grid(int Nx, int Ny, int Nz);
void update_visual_grid(vcl::buffer<vcl::vec3>& segments_grid, vcl::grid_3D<vcl::vec3> const& grid);

