#include "vcl/vcl.hpp"
#include <iostream>

#include "simulation.hpp"
#include "helper.hpp"

using namespace vcl;

struct gui_parameters {
	bool display_grid = true;
	bool display_velocity = true;
	float diffusion_velocity = 0.001f;
	float diffusion_density = 0.005f;
	float velocity_scaling = 1.0f;
	density_type_structure density_type = density_color;
};

struct user_interaction_parameters {
	vec2 mouse_prev;
	timer_fps fps_record;
	gui_parameters gui;
	bool cursor_on_gui;
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


void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_click_callback(GLFWwindow* window, int button, int action, int mods);
void window_size_callback(GLFWwindow* window, int width, int height);


void initialize_data();
void display_scene();
void display_interface();
void simulate(float dt);

timer_basic timer;

grid_2D<vec3> density, density_previous;
grid_2D<vec2> velocity, velocity_previous;
grid_2D<float> divergence;
grid_2D<float> gradient_field;

mesh_drawable density_visual;
segments_drawable grid_visual;
segments_drawable velocity_visual;
buffer<vec3> velocity_grid_data;
velocity_tracker velocity_track;



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
	glfwSetMouseButtonCallback(window, mouse_click_callback);
	
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

		float const dt = 0.2f * timer.scale;
		simulate(dt);
		opengl_update_texture_gpu(density_visual.texture, density);
		update_velocity_visual(velocity_visual, velocity_grid_data, velocity, user.gui.velocity_scaling);
		
		display_interface();
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

void simulate(float dt)
{

	velocity_previous = velocity;
	density_previous = density;

	// velocity
	diffuse(velocity, velocity_previous, user.gui.diffusion_velocity, dt, reflective); velocity_previous = velocity;
	divergence_free(velocity, velocity_previous, divergence, gradient_field); velocity_previous = velocity;
	advect(velocity, velocity_previous, velocity_previous, dt);

	// density
	if(user.gui.density_type!=view_velocity_curl){
		diffuse(density, density_previous, user.gui.diffusion_density, dt, copy); density_previous = density;
		advect(density, density_previous, velocity, dt);
	}
	else // in case you directly look at the velocity curl (no density advection in this case)
		density_to_velocity_curl(density, velocity);

}

void initialize_density(density_type_structure density_type, size_t N)
{
	if(density_type == density_color) {
        initialize_density_color(density, N);
	}

    if(density_type == density_texture){
        convert(image_load_png("assets/texture.png"), density);
	}

	if(density_type == view_velocity_curl) {
		density.resize(N,N); density.fill({1,1,1});
	}
	
	density_previous = density;
}


void initialize_fields(density_type_structure density_type)
{
	size_t const N = 60;
    velocity.resize(N,N); velocity.fill({0,0}); velocity_previous = velocity;
	initialize_density(density_type, N);
    divergence.clear(); divergence.resize(N,N);
    gradient_field.clear(); gradient_field.resize(N,N);

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

	
	scene.camera.look_at({0,0,1.0f}, {0,0,0}, {0,1,0});

	initialize_fields(user.gui.density_type);
	size_t const N = velocity.dimension.x;
	initialize_density_visual(density_visual, N);
	density_visual.texture = opengl_texture_to_gpu(density);
	initialize_grid(grid_visual, N);
	velocity_grid_data.resize(2*N*N);
	velocity_visual = segments_drawable(velocity_grid_data);
	velocity_visual.color = vec3(0,0,0);
}


void display_scene()
{
	draw(density_visual, scene);

	if(user.gui.display_grid)
		draw(grid_visual, scene);

	if(user.gui.display_velocity)
		draw(velocity_visual, scene);
}
void display_interface()
{
	ImGui::SliderFloat("Timer scale", &timer.scale, 0.01f, 4.0f, "%0.2f");
	ImGui::SliderFloat("Diffusion Density", &user.gui.diffusion_density, 0.001f, 0.2f, "%0.3f",2.0f);
	ImGui::SliderFloat("Diffusion Velocity", &user.gui.diffusion_velocity, 0.001f, 0.2f, "%0.3f",2.0f);
	ImGui::Checkbox("Grid", &user.gui.display_grid); ImGui::SameLine();
	ImGui::Checkbox("Velocity", &user.gui.display_velocity);
	ImGui::SliderFloat("Velocity scale", &user.gui.velocity_scaling, 0.1f, 10.0f, "0.2f");

	bool const cancel_velocity = ImGui::Button("Cancel Velocity"); ImGui::SameLine();
	bool const restart = ImGui::Button("Restart");

	bool new_density = false;
	int* ptr_density_type  = reinterpret_cast<int*>(&user.gui.density_type);
	new_density |= ImGui::RadioButton("Density color", ptr_density_type, density_color); ImGui::SameLine();
	new_density |= ImGui::RadioButton("Density texture", ptr_density_type, density_texture); ImGui::SameLine();
	new_density |= ImGui::RadioButton("Velocity Curl", ptr_density_type, view_velocity_curl);
	if (new_density || restart) 
		initialize_density(user.gui.density_type, velocity.dimension.x);
	if(cancel_velocity || restart)
		velocity.fill({0,0});
		
}


void window_size_callback(GLFWwindow* , int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	float const L = 1.1f;
	scene.projection = projection_orthographic(-aspect*L,aspect*L,-L,L,-10.0f,10.0f);
	scene.projection_inverse = projection_orthographic_inverse(-aspect*L,aspect*L,-L,L,0.1f,10.0f);
}

void mouse_click_callback(GLFWwindow* , int , int , int )
{
	ImGui::SetWindowFocus(nullptr);
}

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
	vec2 const  p1 = glfw_get_mouse_cursor(window, xpos, ypos);
	vec2 const& p0 = user.mouse_prev;

	glfw_state state = glfw_current_state(window);

	auto& camera = scene.camera;
	

	if(!user.cursor_on_gui){
		if(state.mouse_click_left && state.key_ctrl)
			camera.manipulator_translate_in_plane(p1-p0);
		if(state.mouse_click_right)
			camera.manipulator_scale_distance_to_center( (p1-p0).y );
	}
		
	if(!user.cursor_on_gui)
	{
		if (state.mouse_click_left)	{
			velocity_track.add(vec3(p1,0.0f), timer.t);
			mouse_velocity_to_grid(velocity, velocity_track.velocity.xy(), scene.projection_inverse, p1 );
		}
		else
			velocity_track.set_record(vec3(p1,0.0f), timer.t);
	}

	user.mouse_prev = p1;
}

void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", scene.camera.matrix_view());
	opengl_uniform(shader, "light", scene.light, false);
}



