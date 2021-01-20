#pragma once

#include "vcl/vcl.hpp"
#include "skinning.hpp"
#include "skeleton.hpp"


void load_animation_bend_zx(vcl::buffer<vcl::buffer<vcl::affine_rt>>& animation_skeleton, vcl::buffer<float>& animation_time, vcl::buffer<int> const& parent_index);
void load_animation_bend_z(vcl::buffer<vcl::buffer<vcl::affine_rt>>& animation_skeleton, vcl::buffer<float>& animation_time, vcl::buffer<int> const& parent_index);
void load_animation_twist_x(vcl::buffer<vcl::buffer<vcl::affine_rt>>& animation_skeleton, vcl::buffer<float>& animation_time, vcl::buffer<int> const& parent_index);

void load_cylinder(vcl::skeleton_animation_structure& skeleton_data, vcl::rig_structure& rig, vcl::mesh& shape);
void load_rectangle(vcl::skeleton_animation_structure& skeleton_data, vcl::rig_structure& rig, vcl::mesh& shape);


void load_skinning_data(std::string const& directory, vcl::skeleton_animation_structure& skeleton_data, vcl::rig_structure& rig, vcl::mesh& shape, GLuint& texture_id);
void load_skinning_anim(std::string const& directory, vcl::skeleton_animation_structure& skeleton_data);