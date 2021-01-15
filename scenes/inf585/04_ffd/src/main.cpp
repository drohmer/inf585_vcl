/**
    Objective: Implement a Free-Form-Deformation (using Bezier functions) on an abitrary mesh embedded in a grid that can be interactively manipulated
	- The grid structure and interactive manipulation is already coded
	- The deformation of the mesh in relation of the grid still have to be coded in the function ffd
	- Remarks: 
	--- The actual FFD deformation is to be implemented in the file ffd.cpp
	--- The grid is set to be in [0,1] in its initial configuration. The initial mesh is also supposed to have coordinates between [0,1]. We can therefore assimilate the parameters (u,v,w)\in[0,1] of the Bezier polynomials to the global coordinates of the mesh positions in its initial configuration.
	--- FFD computation can be computationally costly - an automatic scheduling system limits the number of computation per second while manipulating the grid (see timer_update_shape)
	--- Tips, you can store any 3D grid-like storage values as grid_3D<float>. (internally stored as a single std::vector<float>, but provides access to the values as grid(kx,ky,kz))
	----- You can store a buffer of 3D-organized values as buffer<grid_3D<float>>, or a 3D grid-organized buffers of values as grid_3D<buffer<float>>.

*/

#include "vcl/vcl.hpp"
#include <iostream>

#include "initialization.hpp"
#include "interface.hpp"
#include "ffd.hpp"
#include "grid_helper.hpp"

using namespace vcl;


struct user_interaction_parameters {
	vec2 mouse_prev;
	timer_fps fps_record;
	bool cursor_on_gui = false;
	
	gui_widget widget;
	picking_parameters picking;
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
mesh_drawable global_frame;

void mouse_click_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);

void initialize_data();
void display_scene();
void create_new_surface();
void update_visual_grid(buffer<vec3>& segments_grid, grid_3D<vec3> const& grid);
void display_grid();


mesh shape;                    // Mesh structure of the deformed shape
mesh_drawable visual;          // Visual representation of the deformed shape

grid_3D<vec3> grid;              // Data of the (x,y,z) grid


mesh_drawable sphere;       // visual element of the grid
buffer<vec3> segments_grid; // edges data for grid representation
segments_drawable segments_grid_visual; // visual representation of the grid edges

// update timers
timer_event_periodic timer_update_shape(0.05f);
bool require_shape_update=false;

// TO DO: You may need to add extra variables to store precomputed informations

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
	
	std::cout<<"Initialize data ..."<<std::endl;
	initialize_data();

	std::cout<<"Start animation loop ..."<<std::endl;
	user.fps_record.start();
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

		timer_update_shape.update();
		if(timer_update_shape.event && require_shape_update) // scheduling system to avoid too many times the FFD deformation
		{
			ffd_deform(shape.position, grid); // Call of the deformation - you may need to add extra parameters to this function

			// Update of the visual modifications
			visual.update_position(shape.position);
			shape.compute_normal();
			visual.update_normal(shape.normal);
			require_shape_update = false;

			update_visual_grid(segments_grid, grid);
			segments_grid_visual = segments_drawable(segments_grid);
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
	segments_drawable::default_shader = shader_uniform_color;


	global_frame = mesh_drawable(mesh_primitive_frame());
	sphere = mesh_drawable(mesh_primitive_sphere(0.02f));
	sphere.shading.color = {0,0,1};

	int const Nx = 4, Ny = 4, Nz=4;
	grid = initialize_grid(Nx, Ny, Nz);
	update_visual_grid(segments_grid, grid);
	segments_grid_visual = segments_drawable(segments_grid);

	create_new_surface();
}

void display_scene() 
{
	if(user.widget.display_frame)
		draw(global_frame, scene);

	if(user.widget.surface)
		draw(visual, scene);
	if(user.widget.wireframe) 
		draw_wireframe(visual, scene, {0,0,0} );

	display_grid();
}

void display_grid()
{
	if(user.widget.display_grid_sphere){
		size_t const N = grid.dimension.x;
		for (size_t kx = 0; kx < N; ++kx) {
			for (size_t ky = 0; ky < N; ++ky) {
				for (size_t kz = 0; kz < N; ++kz) {
					sphere.transform.translate = grid(kx,ky,kz);
					draw(sphere, scene);
				}
			}
		}
	}

	if(user.widget.display_grid_edge)
		draw(segments_grid_visual, scene);
}

void create_new_surface() // call this function every time we change of surface
{
	// The detail of the initialization functions are in the file initialization.cpp
	switch (user.widget.surface_type)
	{
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

	if(user.widget.reset_grid)
		grid = initialize_grid(int(grid.dimension.x), int(grid.dimension.y), int(grid.dimension.z));

	require_shape_update = true;
}




void window_size_callback(GLFWwindow* , int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	float const fov = 50.0f * pi /180.0f;
	float const z_min = 0.1f;
	float const z_max = 100.0f;
	scene.projection = projection_perspective(fov, aspect, z_min, z_max);
	scene.projection_inverse = projection_perspective_inverse(fov, aspect, z_min, z_max);
}


void mouse_click_callback(GLFWwindow* , int button, int action, int )
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		user.picking.active = false;
		require_shape_update=true;
	}

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
		if (!state.mouse_click_left) // Handle grid point picking here
		{
			vec3 const ray_direction = camera_ray_direction(scene.camera.matrix_frame(),scene.projection_inverse, p1);
			vec3 const ray_origin = scene.camera.position();

			int index=0;
			intersection_structure intersection = intersection_ray_spheres_closest(ray_origin, ray_direction, grid.data, 0.03f, &index);
			if (intersection.valid == true) {
				vec3 const& normal = shape.normal[index];
				user.picking = {true, index, p1, intersection.position, normal};
			}
		}
		if (state.mouse_click_left && user.picking.active) // Grid point translation
		{
			// Get vector orthogonal to camera orientation
			vec3 const n = scene.camera.front();
			// Compute intersection between current ray and the plane orthogonal to the view direction and passing by the selected object
			vec3 const ray_direction = camera_ray_direction(scene.camera.matrix_frame(),scene.projection_inverse, p1);
			vec3 const ray_origin = scene.camera.position();
			intersection_structure intersection = intersection_ray_plane(ray_origin, ray_direction, user.picking.p_clicked, n);

			grid[user.picking.index] = intersection.position;
			require_shape_update = true;
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


