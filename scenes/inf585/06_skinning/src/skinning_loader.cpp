#include "skinning_loader.hpp"

using namespace vcl;

void load_animation_bend_z(buffer<buffer<affine_rt>>& animation_skeleton, buffer<float>& animation_time, buffer<int> const& )
{
	animation_time = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
	animation_skeleton.resize(animation_time.size());

	rotation const r0 = rotation();
	rotation const rz1 = rotation({0,0,1}, pi/4.0f);
	rotation const rz2 = rotation({0,0,1}, pi/2.0f);
	vec3 const t0 = {0.0f, 0, 0};
	vec3 const tx = {0.5f, 0, 0};
	animation_skeleton[0] = {affine_rt{r0,t0}, affine_rt{r0,tx}, affine_rt{r0,tx}};
	animation_skeleton[1] = {affine_rt{r0,t0}, affine_rt{rz1,tx}, affine_rt{r0,tx}};
	animation_skeleton[2] = {affine_rt{r0,t0}, affine_rt{rz2,tx}, affine_rt{r0,tx}};
	animation_skeleton[3] = animation_skeleton[1];
	animation_skeleton[4] = animation_skeleton[0];
}

void load_animation_bend_zx(buffer<buffer<affine_rt>>& animation_skeleton, buffer<float>& animation_time, buffer<int> const& )
{
	animation_time = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
	animation_skeleton.resize(animation_time.size());

	rotation const r0 = rotation();
	rotation const rz = rotation({0,0,1}, pi/2.0f);
	rotation const rx = rotation({1,0,0}, pi/2.0f);
	rotation const rxn = rotation({1,0,0}, -pi/2.0f);
	vec3 const t0 = {0.0f, 0, 0};
	vec3 const tx = {0.5f, 0, 0};
	animation_skeleton[0] = {affine_rt{r0,t0}, affine_rt{r0,tx}, affine_rt{r0,tx}};
	animation_skeleton[1] = {affine_rt{r0,t0}, affine_rt{rz,tx}, affine_rt{r0,tx}};
	animation_skeleton[2] = {affine_rt{r0,t0}, affine_rt{rx,tx}*rz, affine_rt{r0,tx}};
	animation_skeleton[3] = {affine_rt{r0,t0}, affine_rt{rxn,tx}*rz, affine_rt{r0,tx}};
	animation_skeleton[4] = {affine_rt{r0,t0}, affine_rt{rz,tx}, affine_rt{r0,tx}};
	animation_skeleton[5] = animation_skeleton[0];

}

void load_animation_twist_x(vcl::buffer<vcl::buffer<vcl::affine_rt>>& animation_skeleton, vcl::buffer<float>& animation_time, vcl::buffer<int> const& )
{
	animation_time = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
	animation_skeleton.resize(animation_time.size());

	rotation const r0 = rotation();
	rotation const rx1 = rotation({1,0,0}, pi/2.0f);
	rotation const rx2 = rotation({1,0,0}, pi);
	vec3 const t0 = {0.0f, 0, 0};
	vec3 const tx = {0.5f, 0, 0};
	animation_skeleton[0] = {affine_rt{r0,t0}, affine_rt{r0,tx}, affine_rt{r0,tx}};
	animation_skeleton[1] = {affine_rt{r0,t0}, affine_rt{rx1,tx}, affine_rt{r0,tx}};
	animation_skeleton[2] = {affine_rt{r0,t0}, affine_rt{rx2,tx}, affine_rt{r0,tx}};
	animation_skeleton[3] = animation_skeleton[1];
	animation_skeleton[4] = animation_skeleton[0];
}

void load_cylinder(skeleton_animation_structure& skeleton_data, rig_structure& rig, mesh& shape)
{
	// Skeleton
	skeleton_data.parent_index = {-1, 0, 1};
	
	rotation r0 = rotation();
	skeleton_data.rest_pose_local.resize(3);
	skeleton_data.rest_pose_local[0] = affine_rt( r0, vec3{0.0f,0,0} );
	skeleton_data.rest_pose_local[1] = affine_rt( r0, vec3{0.5f,0,0} );
	skeleton_data.rest_pose_local[2] = affine_rt( r0, vec3{0.5f,0,0} );

	// mesh
	size_t const N = 50;
	shape = mesh_primitive_cylinder(0.1f, {0,0,0}, {1,0,0}, N,N);
	
	rig.joint.clear();
	rig.weight.clear();

	for (size_t ku = 0; ku < N; ++ku)
	{
		for (size_t kv = 0; kv < N; ++kv)
		{
			const float u = ku/float(N-1.0f);
			float const alpha = 3.0f; // power for skinning weights evolution
			float w0, w1;

			if (u < 0.5f) {
				w1 = 0.5f*std::pow(u/0.5f, alpha);
				w0 = 1-w1;
			}
			else {
				w0 = 0.5f*std::pow(1-(u-0.5f)/0.5f, alpha);
				w1 = 1-w0;
			}
			
			rig.joint.push_back( buffer<int>{0, 1});
			rig.weight.push_back( buffer<float>{w0, w1});
		}
	}
}

void load_rectangle(skeleton_animation_structure& skeleton_data, rig_structure& rig, mesh& shape)
{
	// Skeleton
	skeleton_data.parent_index = {-1, 0, 1};
	
	rotation r0 = rotation();
	skeleton_data.rest_pose_local.resize(3);
	skeleton_data.rest_pose_local[0] = affine_rt( r0, vec3{0.0f,0,0} );
	skeleton_data.rest_pose_local[1] = affine_rt( r0, vec3{0.5f,0,0} );
	skeleton_data.rest_pose_local[2] = affine_rt( r0, vec3{0.5f,0,0} );

	// mesh
	float const a = 0.1f;
	shape = mesh_primitive_cubic_grid({0,-a,-a}, {1,-a,-a}, {1,a,-a}, {0,a,-a}, {0,-a,a}, {1,-a,a}, {1,a,a}, {0,a,a}, 35, 10, 10);
	
	rig.joint.clear();
	rig.weight.clear();

	size_t const N = shape.position.size();
	for(size_t k=0; k<N; ++k)
	{
		float const u = shape.position[k].x;
		float const alpha = 3.0f;
		float w0, w1;

		if (u < 0.5f) {
			w1 = 0.5f*std::pow(u/0.5f, alpha);
			w0 = 1-w1;
		}
		else {
			w0 = 0.5f*std::pow(1-(u-0.5f)/0.5f, alpha);
			w1 = 1-w0;
		}
		rig.joint.push_back( buffer<int>{0, 1});
		rig.weight.push_back( buffer<float>{w0, w1});
	}

}


//Map correspondance between skinning weights and vertices (that have been duplicated to load the texture coordinates)
template <typename T>
vcl::buffer<T> map_correspondance(vcl::buffer<T> value, vcl::buffer<vcl::buffer<int> > const& correspondance)
{
    // find the maximal index used for destination
    int max_index = 0;
    for(size_t k1=0; k1<correspondance.size(); ++k1)
        for(size_t k2=0; k2<correspondance[k1].size(); ++k2)
            if(max_index<correspondance[k1][k2])
                max_index = correspondance[k1][k2];

    vcl::buffer<T> new_value;
    new_value.resize(max_index+1);

    // Apply correspondance (copy value[k] in its destination)
    for(size_t k=0; k<correspondance.size(); ++k)
        for(size_t k2=0; k2<correspondance[k].size(); ++k2)
            new_value[correspondance[k][k2]] = value[k];
    return new_value;
}

void load_skinning_anim(std::string const& directory, skeleton_animation_structure& skeleton_data)
{
	buffer<buffer<vec3>> anim_translation; read_from_file(directory+"skeleton_animation_position_local.txt", anim_translation);
	buffer<buffer<quaternion>> anim_quaternion; read_from_file(directory+"skeleton_animation_quaternion_local.txt", anim_quaternion);
	buffer<float> anim_time; read_from_file(directory+"skeleton_animation_time.txt", anim_time);

	assert_vcl_no_msg(anim_translation.size()==anim_quaternion.size());
	assert_vcl_no_msg(anim_translation.size()==anim_time.size());
	assert_vcl_no_msg(anim_translation.size()>0);

	size_t const N_time = anim_translation.size();
	size_t const N_joint = anim_translation[0].size();
	skeleton_data.animation_geometry_local.resize_clear(N_time);
	for (size_t kt = 0; kt < N_time; ++kt)
	{
		assert_vcl_no_msg(anim_quaternion[kt].size()==N_joint);
		assert_vcl_no_msg(anim_translation[kt].size()==N_joint);
		skeleton_data.animation_geometry_local[kt].resize(N_joint);
		for (size_t kj = 0; kj < N_joint; ++kj)
		{
			affine_rt const T = affine_rt(rotation(anim_quaternion[kt][kj]), anim_translation[kt][kj]);
			skeleton_data.animation_geometry_local[kt][kj] = T;
		}
	}

	skeleton_data.animation_time = anim_time;

}

void load_skinning_data(std::string const& directory, skeleton_animation_structure& skeleton_data, rig_structure& rig, mesh & shape, GLuint& texture_id)
{
	buffer<buffer<int> > vertex_correspondance;
	shape = mesh_load_file_obj(directory+"mesh.obj", vertex_correspondance);
	shape.fill_empty_field();
	texture_id = opengl_texture_to_gpu(image_load_png(directory+"texture.png"));
	
	buffer<vec3> skeleton_translation; read_from_file(directory+"skeleton_geometry_translation_local.txt", skeleton_translation);
	buffer<quaternion> skeleton_quaternion; read_from_file(directory+"skeleton_geometry_quaternion_local.txt", skeleton_quaternion);
	
	buffer<int> parent_index; read_from_file(directory+"skeleton_parent_index.txt", parent_index);

	
	buffer<buffer<float>> weights; read_from_file(directory+"rig_weights.txt", weights);
	buffer<buffer<int>> joints; read_from_file(directory+"rig_bones.txt", joints);

	

	weights = map_correspondance(weights, vertex_correspondance);
	joints = map_correspondance(joints, vertex_correspondance);

	assert_vcl_no_msg(skeleton_translation.size()==skeleton_quaternion.size());
	assert_vcl_no_msg(parent_index.size()==skeleton_translation.size());
	assert_vcl_no_msg(weights.size()==joints.size());
	assert_vcl_no_msg(shape.position.size()==joints.size());
	

	size_t const N_joint = skeleton_translation.size();
	skeleton_data.rest_pose_local.resize(N_joint);
	for(size_t k=0; k<N_joint; ++k)
	{
		quaternion const q = normalize(skeleton_quaternion[k]);
		affine_rt const T = affine_rt(rotation(q), skeleton_translation[k]);
		skeleton_data.rest_pose_local[k] = T;
	}

	skeleton_data.parent_index = parent_index;

	rig.weight = weights;
	rig.joint = joints;

}