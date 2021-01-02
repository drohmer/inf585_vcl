/**
	Objective: Reproduce the scene with the falling and bouncing sphere.

	- Complete the function "display_scene" to compute the expected trajectory
*/


#include "vcl/vcl.hpp"
#include <iostream>

using namespace vcl;

// ****************************************** //
// Structure specific to the current scene
// ****************************************** //

struct user_interaction_parameters {
	vec2 mouse_prev;
	bool cursor_on_gui;
	bool display_frame = true;
};
user_interaction_parameters user; // Variable used to store user interaction and GUI parameter

struct scene_environment
{
	camera_around_center camera;
	mat4 projection;
	vec3 light;
};
scene_environment scene; // Generic elements of the scene (camera, light, etc.)

// Particle structure 
struct particle_structure
{
	vec3 p0;  // Initial position
	vec3 v0;  // Initial velocity
	float t0; // Time of birth
};


// ****************************************** //
// Functions signatures
// ****************************************** //

// Callback functions
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);


void initialize_data(); // Initialize the data of this scene
void create_new_particle(float current_time);
void display_scene(float current_time);   
void remove_old_particles(float current_time);

// ****************************************** //
// Global variables
// ****************************************** //

std::vector<particle_structure> particles; // The container of the active particles

mesh_drawable sphere;        // Sphere used to represent the particle
mesh_drawable ground;        // Visual representation of the ground
mesh_drawable global_frame;  // Frame used to see the global coordinate system

timer_event_periodic timer(0.5f);  // Timer with periodic event



// ****************************************** //
// Functions definitions
// ****************************************** //

// Main function with creation of the scene and animation loop
int main(int, char* argv[])
{
	std::cout << "Run " << argv[0] << std::endl;

	// create GLFW window and initialize OpenGL
	GLFWwindow* window = create_window(1280,1024); 
	window_size_callback(window, 1280, 1024);
	std::cout << opengl_info_display() << std::endl;;

	imgui_init(window); // Initialize GUI library

	// Set GLFW callback functions
	glfwSetCursorPosCallback(window, mouse_move_callback); 
	glfwSetWindowSizeCallback(window, window_size_callback);
	
	std::cout<<"Initialize data ..."<<std::endl;
	initialize_data();

	std::cout<<"Start animation loop ..."<<std::endl;
	timer.start();
	glEnable(GL_DEPTH_TEST);
	while (!glfwWindowShouldClose(window))
	{
		scene.light = scene.camera.position();
		timer.update();

		// Clear screen
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);

		// Create GUI interface for the current frame
		imgui_create_frame();
		ImGui::Begin("GUI",NULL,ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Checkbox("Display frame", &user.display_frame);
		ImGui::SliderFloat("Time Scale", &timer.scale, 0.0f, 2.0f, "%.1f");


		//display_scene();
		if(user.display_frame)
			draw(global_frame, scene);
		
		// ****************************************** //
		// Specific calls of this scene
		// ****************************************** //

		// if there is a periodic event, insert a new particle
		if (timer.event) 
			create_new_particle(timer.t);

		remove_old_particles(timer.t);

		// Display the scene (includes the computation of the particle position at current time)
		display_scene(timer.t);

		// ****************************************** //
		// ****************************************** //


		// Display GUI
		ImGui::End();
		imgui_render_frame(window);

		// Swap buffer and handle GLFW events
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
	// Load and set the common shaders
	GLuint const shader_mesh = opengl_create_shader_program(opengl_shader_preset("mesh_vertex"), opengl_shader_preset("mesh_fragment"));
	GLuint const shader_single_color = opengl_create_shader_program(opengl_shader_preset("single_color_vertex"), opengl_shader_preset("single_color_fragment"));
	GLuint const texture_white = opengl_texture_to_gpu(image_raw{1,1,image_color_type::rgba,{255,255,255,255}});

	mesh_drawable::default_shader = shader_mesh;
	mesh_drawable::default_texture = texture_white;
	global_frame = mesh_drawable(mesh_primitive_frame());
	scene.camera.look_at({2,3,2}, {0,0,0}, {0,0,1});

	// Prepare the sphere used to display the particle and the ground
	float const radius_sphere = 0.05f;
	ground = mesh_drawable(mesh_primitive_disc(1.0, {0,0,-radius_sphere}, {0,0,1}, 60));
	sphere = mesh_drawable(mesh_primitive_sphere(radius_sphere));
	sphere.shading.color = {0.5f,0.8f,1.0f};
}

void create_new_particle(float current_time)
{
	particle_structure new_particle;

	float const theta = rand_interval(0,2*pi);
	new_particle.p0 = {0,0,0};
	new_particle.v0 = {0.8f*std::sin(theta),0.8f*std::cos(theta),5};
	new_particle.t0 = current_time;

	particles.push_back(new_particle);
}

void display_scene(float current_time)
{
	vec3 const g = {0,0,-9.81f}; // gravity constant

	// Compute the position of each particle at the current time
	//   Then display it as a sphere
	size_t N = particles.size();
	for (size_t k = 0; k < N; ++k)
	{
		vec3 const& p0 = particles[k].p0;             // initial position
		vec3 const& v0 = particles[k].v0;             // initial velocity
		float const t = current_time-particles[k].t0; // elapsed time since particle birth
			
		// TO DO: Modify this computation to model the bouncing effect
		// **************************************************************** //
		vec3 p = 0.5f*g*t*t + v0*t + p0; // currently only models the first parabola
		// ... to adapt
		// **************************************************************** //

		// Set the position of the visual sphere representation
		sphere.transform.translate = p;

		// Display the particle as a sphere
		draw(sphere, scene);
	}

	// DIsplay the ground
	draw(ground, scene);
}

void remove_old_particles(float current_time)
{
	// Loop over all active pactiles
	for (auto it = particles.begin(); it != particles.end();)
	{
		// if a particle is too old, remove it
		if (current_time - it->t0 > 3)
			it = particles.erase(it);

		// Go to the next particle if we are not already on the last one
		if(it!=particles.end()) 
			++it;
	}
}


// Function called every time the screen is resized
void window_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	scene.projection = projection_perspective(50.0f*pi/180.0f, aspect, 0.1f, 100.0f);
}

// Function called every time the mouse is moved
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
	vec2 const  p1 = glfw_get_mouse_cursor(window, xpos, ypos);
	vec2 const& p0 = user.mouse_prev;

	glfw_state state = glfw_current_state(window);
	user.cursor_on_gui = ImGui::IsAnyWindowFocused();

	// Handle camera rotation
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

// Uniform data used when displaying an object in this scene
void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", scene.camera.matrix_view());
	opengl_uniform(shader, "light", scene.light, false);
}



