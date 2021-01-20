#pragma once

#include "vcl/vcl.hpp"


namespace vcl
{
	// Helper structure storing an animated skeleton
	struct skeleton_animation_structure
	{
		buffer<int> parent_index;          // Connectivity of the skeleton. Stores at index i, the index of the parent joint
		buffer<affine_rt> rest_pose_local; // Rest pose storage of the rigid transforms (rotation and translation) of the joints in local coordinates

		buffer<float> animation_time;      // Sequence of time corresponding to the animation
		buffer<buffer<affine_rt>> animation_geometry_local; // Storage of all rigid transforms of the joints for every frame in local coordinates (for all time, for all joints)

		// Number of joints in the skeleton
		size_t number_joint() const;
		// Number of frames of the animation
		size_t number_animation_frame() const;

		// Evaluate the interpolated joint rigid transforms in local coordinates at the time t
		buffer<affine_rt> evaluate_local(float t) const;
		// Evaluate the interpolated joint rigid transforms in global coordinates at the time t
		buffer<affine_rt> evaluate_global(float t) const;

		// Return the rigid transforms of the joints of the rest pose in global coordinates
		buffer<affine_rt> rest_pose_global() const;

		// Apply scaling to the entire skeleton (scale the translation part of the rigid transforms)
		void scale(float s);

	};

	// Convert a skeleton defined in local coordinates to global coordinates
	buffer<affine_rt> skeleton_local_to_global(buffer<affine_rt> const& local, buffer<int> const& parent_index);
}