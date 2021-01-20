/**
	Objectives:
	- Complete rigid transform interpolation in the function evaluate_local function in file skeleton.cpp
	- Complete the Linear Blend Skinning computation in the function skinning.cpp
*/

#include "vcl/vcl.hpp"
#include <iostream>

#include "skeleton.hpp"
#include "skeleton_drawable.hpp"
#include "skinning.hpp"
#include "skinning_loader.hpp"


using namespace vcl;

struct gui_parameters {
	bool display_frame = true;
	bool surface_skinned = true;
	bool wireframe_skinned = false;
	bool surface_rest_pose = false;
	bool wireframe_rest_pose = false;

	bool skeleton_current_bone = true;
	bool skeleton_current_frame = false;
	bool skeleton_current_sphere = false;

	bool skeleton_rest_pose_bone = false;
	bool skeleton_rest_pose_frame = false;
	bool skeleton_rest_pose_sphere = false;
};

struct user_interaction_parameters {
	vec2 mouse_prev;
	timer_fps fps_record;
	gui_parameters gui;
	mesh_drawable global_frame;
	bool cursor_on_gui;
};
user_interaction_parameters user;

struct scene_environment
{
	camera_around_center camera;
	mat4 projection;
	vec3 light;
};
scene_environment scene;

struct visual_shapes_parameters
{
	mesh_drawable surface_skinned;
	mesh_drawable surface_rest_pose;
	skeleton_drawable skeleton_current;
	skeleton_drawable skeleton_rest_pose;
};
visual_shapes_parameters visual_data;

struct skinning_current_data
{
	buffer<vec3> position_rest_pose;
	buffer<vec3> position_skinned;
	buffer<vec3> normal_rest_pose;
	buffer<vec3> normal_skinned;

	buffer<affine_rt> skeleton_current;
	buffer<affine_rt> skeleton_rest_pose;
};

skeleton_animation_structure skeleton_data;
rig_structure rig;
skinning_current_data skinning_data;



timer_interval timer;

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);

void initialize_data();
void display_scene();
void display_interface();
void compute_deformation();
void update_new_content(mesh const& shape, GLuint texture_id);





int main(int, char* argv[])
{

	std::cout << "Run " << argv[0] << std::endl;

	int const width = 1280, height = 1024;
	GLFWwindow* window = create_window(width, height);
	window_size_callback(window, width, height);
	std::cout << opengl_info_display() << std::endl;;

	imgui_init(window);
	glfwSetCursorPosCallback(window, mouse_move_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);

	std::cout<<"Initialize data ..."<<std::endl;
	initialize_data();

	std::cout<<"Start animation loop ..."<<std::endl;
	user.fps_record.start();
	timer.start();
	glEnable(GL_DEPTH_TEST);
	while (!glfwWindowShouldClose(window))
	{
		scene.light = scene.camera.position();
		user.fps_record.update();
		timer.update();

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);
		imgui_create_frame();
		if(user.fps_record.event) {
			std::string const title = "VCL Display - "+str(user.fps_record.fps)+" fps";
			glfwSetWindowTitle(window, title.c_str());
		}

		ImGui::Begin("GUI",NULL,ImGuiWindowFlags_AlwaysAutoResize);
		user.cursor_on_gui = ImGui::IsAnyWindowFocused();

		if(user.gui.display_frame)
			draw(user.global_frame, scene);
		

		display_interface();
		compute_deformation();
		display_scene();

		ImGui::End();
		imgui_render_frame(window);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	imgui_cleanup();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}


void initialize_data()
{
	GLuint const shader_mesh = opengl_create_shader_program(opengl_shader_preset("mesh_vertex"), opengl_shader_preset("mesh_fragment"));
	GLuint const shader_uniform_color = opengl_create_shader_program(opengl_shader_preset("single_color_vertex"), opengl_shader_preset("single_color_fragment"));
	GLuint const texture_white = opengl_texture_to_gpu(image_raw{1,1,image_color_type::rgba,{255,255,255,255}});
	mesh_drawable::default_shader = shader_mesh;
	mesh_drawable::default_texture = texture_white;
	curve_drawable::default_shader = shader_uniform_color;
	segments_drawable::default_shader = shader_uniform_color;

	user.global_frame = mesh_drawable(mesh_primitive_frame());
	user.gui.display_frame = false;
	scene.camera.distance_to_center = 2.5f;

	mesh shape;
	load_cylinder(skeleton_data, rig, shape);
	load_animation_bend_zx(skeleton_data.animation_geometry_local, 
		skeleton_data.animation_time, 
		skeleton_data.parent_index);
	update_new_content(shape, texture_white);
}

void compute_deformation()
{
	float const t = timer.t;

	skinning_data.skeleton_current = skeleton_data.evaluate_global(t);
	visual_data.skeleton_current.update(skinning_data.skeleton_current, skeleton_data.parent_index);

	// Compute skinning deformation
	skinning_LBS_compute(skinning_data.position_skinned, skinning_data.normal_skinned, 
		skinning_data.skeleton_current, skinning_data.skeleton_rest_pose, 
		skinning_data.position_rest_pose, skinning_data.normal_rest_pose,
		rig);
	visual_data.surface_skinned.update_position(skinning_data.position_skinned);
	visual_data.surface_skinned.update_normal(skinning_data.normal_skinned);
	
}

void display_scene()
{
	if(user.gui.surface_skinned) 
		draw(visual_data.surface_skinned, scene);
	if (user.gui.wireframe_skinned)
		draw_wireframe(visual_data.surface_skinned, scene, {0.5f, 0.5f, 0.5f});

	draw(visual_data.skeleton_current, scene);

	if(user.gui.surface_rest_pose)
		draw(visual_data.surface_rest_pose, scene);
	if (user.gui.wireframe_rest_pose)
		draw_wireframe(visual_data.surface_rest_pose, scene, {0.5f, 0.5f, 0.5f});

	draw(visual_data.skeleton_rest_pose, scene);

}

void update_new_content(mesh const& shape, GLuint texture_id)
{
	visual_data.surface_skinned.clear();
	visual_data.surface_skinned = mesh_drawable(shape);
	visual_data.surface_skinned.texture = texture_id;

	visual_data.surface_rest_pose.clear();
	visual_data.surface_rest_pose = mesh_drawable(shape);
	visual_data.surface_rest_pose.texture = texture_id;

	skinning_data.position_rest_pose = shape.position;
	skinning_data.position_skinned = skinning_data.position_rest_pose;
	skinning_data.normal_rest_pose = shape.normal;
	skinning_data.normal_skinned = skinning_data.normal_rest_pose;

	skinning_data.skeleton_current = skeleton_data.rest_pose_global();
	skinning_data.skeleton_rest_pose = skinning_data.skeleton_current;

	visual_data.skeleton_current.clear();
	visual_data.skeleton_current = skeleton_drawable(skinning_data.skeleton_current, skeleton_data.parent_index);

	visual_data.skeleton_rest_pose.clear();
	visual_data.skeleton_rest_pose = skeleton_drawable(skinning_data.skeleton_rest_pose, skeleton_data.parent_index);
	
	timer.t_min = skeleton_data.animation_time[0];
	timer.t_max = skeleton_data.animation_time[skeleton_data.animation_time.size()-1];
	timer.t = skeleton_data.animation_time[0];
}

void display_interface()
{
	ImGui::Checkbox("Display frame", &user.gui.display_frame);
	ImGui::Spacing(); ImGui::Spacing();

	ImGui::SliderFloat("Time", &timer.t, timer.t_min, timer.t_max, "%.2f s");
	ImGui::SliderFloat("Time Scale", &timer.scale, 0.05f, 2.0f, "%.2f s");

	ImGui::Spacing(); ImGui::Spacing();

	ImGui::Text("Deformed "); 
	ImGui::Text("Surface: ");ImGui::SameLine();
	ImGui::Checkbox("Plain", &user.gui.surface_skinned); ImGui::SameLine();
	ImGui::Checkbox("Wireframe", &user.gui.wireframe_skinned);

	ImGui::Text("Skeleton: "); ImGui::SameLine();
	ImGui::Checkbox("Bones", &user.gui.skeleton_current_bone); ImGui::SameLine();
	ImGui::Checkbox("Frame", &user.gui.skeleton_current_frame); ImGui::SameLine();
	ImGui::Checkbox("Sphere", &user.gui.skeleton_current_sphere);

	ImGui::Spacing(); ImGui::Spacing();

	ImGui::Text("Rest Pose");
	ImGui::Text("Surface: ");ImGui::SameLine();
	ImGui::Checkbox("Plain##Rest", &user.gui.surface_rest_pose); ImGui::SameLine();
	ImGui::Checkbox("Wireframe##Rest", &user.gui.wireframe_rest_pose);

	ImGui::Text("Skeleton: "); ImGui::SameLine();
	ImGui::Checkbox("Bones##Rest", &user.gui.skeleton_rest_pose_bone); ImGui::SameLine();
	ImGui::Checkbox("Frame##Rest", &user.gui.skeleton_rest_pose_frame); ImGui::SameLine();
	ImGui::Checkbox("Sphere##Rest", &user.gui.skeleton_rest_pose_sphere);

	ImGui::Spacing(); ImGui::Spacing();


	visual_data.skeleton_current.display_segments = user.gui.skeleton_current_bone;
	visual_data.skeleton_current.display_joint_frame = user.gui.skeleton_current_frame;
	visual_data.skeleton_current.display_joint_sphere = user.gui.skeleton_current_sphere;
	visual_data.skeleton_rest_pose.display_segments = user.gui.skeleton_rest_pose_bone;
	visual_data.skeleton_rest_pose.display_joint_frame = user.gui.skeleton_rest_pose_frame;
	visual_data.skeleton_rest_pose.display_joint_sphere = user.gui.skeleton_rest_pose_sphere;

	mesh new_shape;
	bool update = false;
	ImGui::Text("Cylinder"); ImGui::SameLine();
	bool const cylinder_bend_z = ImGui::Button("Bend z###CylinderBendZ");  ImGui::SameLine();
	if (cylinder_bend_z) {
		update=true;
		load_cylinder(skeleton_data, rig, new_shape);
		load_animation_bend_z(skeleton_data.animation_geometry_local, 
			skeleton_data.animation_time, 
			skeleton_data.parent_index);
	}
	bool const cylinder_bend_zx = ImGui::Button("Bend zx###CylinderBendZX");
	if(cylinder_bend_zx)std::cout<<cylinder_bend_zx<<std::endl;
	if (cylinder_bend_zx) {
		update=true;
		load_cylinder(skeleton_data, rig, new_shape);
		load_animation_bend_zx(skeleton_data.animation_geometry_local, 
			skeleton_data.animation_time, 
			skeleton_data.parent_index);
	}

	ImGui::Text("Rectangle"); ImGui::SameLine();
	bool const rectangle_bend_z = ImGui::Button("Bend z###RectangleBendZ");  ImGui::SameLine();
	if (rectangle_bend_z) {
		update=true;
		load_rectangle(skeleton_data, rig, new_shape);
		load_animation_bend_z(skeleton_data.animation_geometry_local, 
			skeleton_data.animation_time, 
			skeleton_data.parent_index);
	}
	bool const rectangle_bend_zx = ImGui::Button("Bend zx###RectangleBendZX");
	if (rectangle_bend_zx) {
		update=true;
		load_rectangle(skeleton_data, rig, new_shape);
		load_animation_bend_zx(skeleton_data.animation_geometry_local, 
			skeleton_data.animation_time, 
			skeleton_data.parent_index);
	}
	bool const rectangle_twist_x = ImGui::Button("Twist x###RectangleTwistX");
	if (rectangle_twist_x) {
		update=true;
		load_rectangle(skeleton_data, rig, new_shape);
		load_animation_twist_x(skeleton_data.animation_geometry_local, 
			skeleton_data.animation_time, 
			skeleton_data.parent_index);
	}

	ImGui::Text("Marine"); ImGui::SameLine();
	bool const marine_run = ImGui::Button("Run"); ImGui::SameLine();
	bool const marine_walk = ImGui::Button("Walk"); ImGui::SameLine();
	bool const marine_idle = ImGui::Button("Idle");

	GLuint texture_id = mesh_drawable::default_texture;
	if (marine_run || marine_walk || marine_idle) load_skinning_data("assets/marine/", skeleton_data, rig, new_shape, texture_id);
	if(marine_run) load_skinning_anim("assets/marine/anim_run/", skeleton_data);
	if(marine_walk) load_skinning_anim("assets/marine/anim_walk/", skeleton_data);
	if(marine_idle) load_skinning_anim("assets/marine/anim_idle/", skeleton_data);
	if (marine_run || marine_walk || marine_idle) {
		update=true;
		normalize_weights(rig.weight);
		float const scaling = 0.005f;
		for(auto& p: new_shape.position) p *= scaling;
		skeleton_data.scale(scaling);
	}

	if (update) 
		update_new_content(new_shape, texture_id);

}



void window_size_callback(GLFWwindow* , int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	scene.projection = projection_perspective(50.0f*pi/180.0f, aspect, 0.1f, 100.0f);
}


void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
	vec2 const  p1 = glfw_get_mouse_cursor(window, xpos, ypos);
	vec2 const& p0 = user.mouse_prev;

	glfw_state state = glfw_current_state(window);

	auto& camera = scene.camera;
	if(!user.cursor_on_gui){
		if(state.mouse_click_left && !state.key_ctrl)
			scene.camera.manipulator_rotate_trackball(p0, p1);
		if(state.mouse_click_left && state.key_ctrl)
			camera.manipulator_translate_in_plane(p1-p0);
		if(state.mouse_click_right)
			camera.manipulator_scale_distance_to_center( (p1-p0).y );
	}

	user.mouse_prev = p1;
}

void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", scene.camera.matrix_view());
	opengl_uniform(shader, "light", scene.light, false);
}



