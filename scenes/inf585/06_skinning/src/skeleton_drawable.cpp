#include "skeleton_drawable.hpp"

namespace vcl
{
	skeleton_drawable::skeleton_drawable()
		:segments(), joint_frame(), joint_sphere(), data()
	{}

	skeleton_drawable::skeleton_drawable(buffer<affine_rt> const& skeleton, buffer<int> const& parent_index)
		: segments(), joint_frame(), joint_sphere(), data(skeleton)
	{
		size_t const N = parent_index.size();
		buffer<vec3> edges;
		for (size_t k = 1; k < N; ++k){
			size_t const parent = parent_index[k];
			assert_vcl_no_msg(parent>=0 && parent<N);
			edges.push_back(skeleton[k].translate);
			edges.push_back(skeleton[parent].translate);
		}
		
		segments = segments_drawable(edges);
		joint_frame = mesh_drawable(mesh_primitive_frame());
		joint_sphere = mesh_drawable(mesh_primitive_sphere());
	}

	void skeleton_drawable::clear()
	{
		segments.clear();
		joint_frame.clear();
		joint_sphere.clear();
		data.clear();
	}

	void skeleton_drawable::update(buffer<affine_rt> const& skeleton, buffer<int> const& parent_index)
	{
		data = skeleton;

		size_t const N = skeleton.size();
		buffer<vec3> edges;
		for (size_t k = 1; k < N; ++k){
			size_t const parent = parent_index[k];
			assert_vcl_no_msg(parent>=0 && parent<N);
			edges.push_back(skeleton[k].translate);
			edges.push_back(skeleton[parent].translate);
		}

		segments.update(edges);
	}
}