#pragma once

#include "vcl/vcl.hpp"


void initialize_density_color(vcl::grid_2D<vcl::vec3>& density, size_t N);
void initialize_density_visual(vcl::mesh_drawable& density_visual, size_t N);
void initialize_grid(vcl::segments_drawable& grid_visual, size_t N);

void update_velocity_visual(vcl::segments_drawable& velocity_visual, vcl::buffer<vcl::vec3>& velocity_grid_data, vcl::grid_2D<vcl::vec2> const& velocity, float scale);

void mouse_velocity_to_grid(vcl::grid_2D<vcl::vec2>& velocity, vcl::vec2 const& mouse_velocity, vcl::mat4 const& P_inv, vcl::vec2 const& p_mouse);
void density_to_velocity_curl(vcl::grid_2D<vcl::vec3>& density, vcl::grid_2D<vcl::vec2> const& velocity);