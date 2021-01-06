/**


*/

#include "vcl/vcl.hpp"
#include <iostream>

#include "initialization.hpp"
#include "interface.hpp"
#include "deformation.hpp"

using namespace vcl;


// Visual elements used to display the picked elements
struct picking_visual_parameters {
	mesh_drawable sphere;   // the sphere indicating which vertex is under the mouse
	curve_drawable circle;  // circle used to show the radius of influence of the deformation
};

struct user_interaction_parameters {
	vec2 mouse_prev;
	timer_fps fps_record;
	bool cursor_on_gui = false;
	
	gui_widget widget;
	picking_parameters picking; // the picking parameters are defined in the file deformation.hpp
	picking_visual_parameters picking_visual;
};
user_interaction_parameters user;

struct scene_environment
{
	camera_around_center camera;
	mat4 projection;
	mat4 projection_inverse;
	vec3 light;
};
scene_environment scene;


void mouse_click_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);
void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void initialize_data();
void display_scene();
void create_new_surface();
mesh_drawable global_frame;


mesh shape;            // Mesh structure of the deformed shape
mesh_drawable visual;  // Visual representation of the deformed shape
buffer<vec3> position_saved;  // Extra storage of the shape position before the current deformation
buffer<vec3> normal_saved;    // Extra storage of the shape normals before the current deformation
timer_event_periodic timer_update_normal(0.15f); // timer with periodic events used to update the normals
bool require_normal_update;  // indicator if the normals need to be updated



int main(int, char* argv[])
{

	std::cout << "Run " << argv[0] << std::endl;
	
	int const width = 1280, height = 1024;
	GLFWwindow* window = create_window(width, height);
	window_size_callback(window, width, height);
	std::cout << opengl_info_display() << std::endl;;

	imgui_init(window);
	glfwSetCursorPosCallback(window, mouse_move_callback);
	glfwSetMouseButtonCallback(window, mouse_click_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetScrollCallback(window, mouse_scroll_callback);
	
	std::cout<<"Initialize data ..."<<std::endl;
	initialize_data();



	std::cout<<"Start animation loop ..."<<std::endl;
	user.fps_record.start();
	timer_update_normal.start();
	glEnable(GL_DEPTH_TEST);
	while (!glfwWindowShouldClose(window))
	{
		scene.light = scene.camera.position();
		user.fps_record.update();
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);
		imgui_create_frame();
				
		ImGui::Begin("GUI",NULL,ImGuiWindowFlags_AlwaysAutoResize);
		user.cursor_on_gui = ImGui::IsAnyWindowFocused();

		if(user.fps_record.event) {
			std::string const title = "VCL Display - "+str(user.fps_record.fps)+" fps";
			glfwSetWindowTitle(window, title.c_str());
		}

		bool const new_surface = display_interface(user.widget);
		if (new_surface)
			create_new_surface();
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

	user.picking_visual.sphere = mesh_drawable(mesh_primitive_sphere(0.02f));
	user.picking_visual.sphere.shading.color = {1,0,0};

	user.picking_visual.circle = curve_drawable(curve_primitive_circle(1.0f,{0,0,0},{0,0,1},40));
	user.picking_visual.circle.color = {1,0,0};

	global_frame = mesh_drawable(mesh_primitive_frame());
	create_new_surface();
}

void display_scene() 
{
	// Display the global frame
	if(user.widget.display_frame)
		draw(global_frame, scene);

	// Display the deformed shape
	draw(visual, scene);
	if(user.widget.wireframe) 
		draw_wireframe(visual, scene, {0,0,0} );
		

	// Periodically update the normal
	//  Doesn't do it all the time as the computation is costly
	timer_update_normal.update();
	if (timer_update_normal.event && require_normal_update) {
		shape.compute_normal();
		visual.update_normal(shape.normal);
		require_normal_update = false;
	}

	// Display of the circle of influence oriented along the local normal of the surface
	if (user.picking.active)
	{
		user.picking_visual.sphere.transform.translate = shape.position[user.picking.index];
		draw(user.picking_visual.sphere, scene);

		user.picking_visual.circle.transform.scale = user.widget.falloff;
		user.picking_visual.circle.transform.translate = shape.position[user.picking.index];
		user.picking_visual.circle.transform.rotate = rotation_between_vector({0,0,1}, user.picking.n_clicked);
		draw(user.picking_visual.circle, scene);
	}

}
void create_new_surface()
{
	// The detail of the initialization functions are in the file initialization.cpp
	switch (user.widget.surface_type)
	{
	case surface_plane:
		shape = initialize_plane(); 
		break;
	case surface_cylinder:
		shape = initialize_cylinder(); 
		break;
	case surface_sphere:
		shape = initialize_sphere(); 
		break;
	case surface_cube:
		shape = initialize_cube(); 
		break;
	case surface_mesh:
		shape = initialize_mesh();
		break;
	}

	// Clear previous surface before seting the values of the new one
	visual.clear();
    visual = mesh_drawable(shape);

    position_saved = shape.position;
    normal_saved = shape.normal;
	require_normal_update = false;
}




void window_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	float const fov = 50.0f * pi /180.0f;
	float const z_min = 0.1f;
	float const z_max = 100.0f;
	scene.projection = projection_perspective(fov, aspect, z_min, z_max);
	scene.projection_inverse = projection_perspective_inverse(fov, aspect, z_min, z_max);
}


void mouse_click_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		user.picking.active = false;
		position_saved = shape.position;
		normal_saved = shape.normal;

		shape.compute_normal();
		visual.update_normal(shape.normal);
	}

}

void mouse_scroll_callback(GLFWwindow* window, double x_offset, double y_offset)
{
    // Increase/decrease falloff distance when scrolling the mouse
    if(user.picking.active)
        user.widget.falloff = std::max(user.widget.falloff + float(y_offset)*0.01f, 1e-6f);
}

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{

	vec2 const  p1 = glfw_get_mouse_cursor(window, xpos, ypos);
	vec2 const& p0 = user.mouse_prev;

	glfw_state state = glfw_current_state(window);
	
	auto& camera = scene.camera;
	if(!user.cursor_on_gui)
	{
		if(!state.key_shift)
		{
			if(state.mouse_click_left && !state.key_ctrl)
				scene.camera.manipulator_rotate_trackball(p0, p1);
			if(state.mouse_click_left && state.key_ctrl)
				camera.manipulator_translate_in_plane(p1-p0);
			if(state.mouse_click_right)
				camera.manipulator_scale_distance_to_center( (p1-p0).y );
		}
	}

	
	if(state.key_shift)
	{
		// Handle vertex picking here

		if (!state.mouse_click_left)
		{
			// Select vertex along the mouse position when pressing shift (without mouse click)
			//   - Throw a ray in the scene along the direction pointed by the mouse
			//   - Then compute the closest intersection with all the spheres surrounding the vertices

			vec3 const ray_direction = camera_ray_direction(scene.camera.matrix_frame(),scene.projection_inverse, p1);
			vec3 const ray_origin = scene.camera.position();

			int index=0;
			intersection_structure intersection = intersection_ray_spheres_closest(ray_origin, ray_direction, shape.position, 0.03f, &index);
			if (intersection.valid == true) {
				vec3 const& normal = shape.normal[index];
				user.picking = {true, index, p1, intersection.position, normal};
			}
		}
		// Deformation (press on shift key + left click on the mouse when a vertex is already selected)
		if (state.mouse_click_left && user.picking.active)
		{
			// Current translation in 2D window coordinates
			vec2 const translation = p1 - user.picking.screen_clicked;

			// Apply the deformation on the surface
			apply_deformation(shape, translation, position_saved, normal_saved, user.widget, user.picking, scene.camera.orientation());
			visual.update_position(shape.position);

			// Update the visual model
			require_normal_update = true;
		}
	}
	else
		user.picking.active = false; // Unselect picking when shift is released
	

	user.mouse_prev = p1;
}

void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", scene.camera.matrix_view());
	opengl_uniform(shader, "light", scene.light, false);
}


