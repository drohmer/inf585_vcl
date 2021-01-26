#include "vcl/vcl.hpp"
#include <iostream>

#include "simulation.hpp"


using namespace vcl;

struct gui_parameters {
	bool display_frame = true;
	bool wireframe = false;
	float wind_magnitude = 1.0f;
	bool run = true;
};

struct user_interaction_parameters {
	vec2 mouse_prev;
	timer_fps fps_record;
	mesh_drawable global_frame;
	gui_parameters gui;
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



struct cloth_structure
{
	int N_cloth;
    grid_2D<vec3> position;
    grid_2D<vec3> velocity;
    grid_2D<vec3> forces;

    grid_2D<vec3> normal;

    buffer<uint3> triangle_connectivity;
    mesh_drawable visual;
	std::map<size_t,vec3> positional_constraints;

	simulation_parameters parameters;
};


void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_click_callback(GLFWwindow* window, int button, int action, int mods);
void window_size_callback(GLFWwindow* window, int width, int height);


void initialize_data();
void display_scene();
void display_interface();
void initialize_cloth();


cloth_structure cloth;
obstacles_parameters obstacles;
GLuint texture_cloth;

mesh_drawable ground;
mesh_drawable sphere;

timer_basic timer;

int main(int, char* argv[])
{


	std::cout << "Run " << argv[0] << std::endl;

	GLFWwindow* window = create_window(1280,1024);
	window_size_callback(window, 1280, 1024);
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
		
		if(user.gui.display_frame) draw(user.global_frame, scene);
		display_interface();

		if(user.gui.run){
			float const dt = 0.005f * timer.scale;
			float const m = cloth.parameters.mass_total/cloth.position.size();
			size_t const N_substeps = 5;
			for(size_t k_substep=0; k_substep<N_substeps; ++k_substep){
				compute_forces(cloth.forces, cloth.position, cloth.velocity, cloth.normal, cloth.parameters, user.gui.wind_magnitude);
				numerical_integration(cloth.position, cloth.velocity, cloth.forces, m, dt);
				apply_constraints(cloth.position, cloth.velocity, cloth.positional_constraints, obstacles);


				bool simulation_diverged = detect_simulation_divergence(cloth.forces, cloth.position);
				if(simulation_diverged==true)
				{
					std::cerr<<" **** Simulation has diverged **** "<<std::endl;
					std::cerr<<" > Stop simulation iterations"<<std::endl;
					user.gui.run = false;
					break;
				}
			}
		}

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
	scene.camera.look_at({4,3,2}, {0,0,0}, {0,0,1});

	ground = mesh_drawable(mesh_primitive_quadrangle({-1.5f,-1.5f,0},{-1.5f,1.5f,0},{1.5f,1.5f,0},{1.5f,-1.5f,0}));
	sphere = mesh_drawable(mesh_primitive_sphere());

	ground.transform.translate = {0,0,obstacles.z_ground};
	sphere.transform.translate = obstacles.sphere_center;
	sphere.transform.scale = obstacles.sphere_radius;
	sphere.shading.color = {1,0,0};

	ground.texture = opengl_texture_to_gpu(image_load_png("assets/wood.png"));
	texture_cloth = opengl_texture_to_gpu(image_load_png("assets/cloth.png"));

	cloth.N_cloth = 30;
	initialize_cloth();
	initialize_simulation_parameters(cloth.parameters, 1.0f, cloth.position.dimension.x);

}

void initialize_cloth()
{
	size_t const N_cloth = cloth.N_cloth;
	float const z0 = 1.0f;
	mesh const cloth_mesh = mesh_primitive_grid({0,0,z0},{1,0,z0},{1,1,z0},{0,1,z0},N_cloth, N_cloth);
	cloth.position = grid_2D<vec3>::from_buffer(cloth_mesh.position, N_cloth, N_cloth);

	cloth.velocity.clear();
	cloth.velocity.resize(N_cloth*N_cloth);

	cloth.forces.clear();
	cloth.forces.resize(N_cloth,N_cloth);

	cloth.visual.clear();
	cloth.visual = mesh_drawable(cloth_mesh);
	cloth.visual.texture = texture_cloth;
	cloth.visual.shading.phong = {0.3f, 0.7f, 0.05f, 32};

	cloth.triangle_connectivity = cloth_mesh.connectivity;
	cloth.normal = grid_2D<vec3>::from_buffer(cloth_mesh.normal, N_cloth, N_cloth);
	
	cloth.positional_constraints.clear();
	cloth.positional_constraints[cloth.position.index_to_offset(0,0)] = cloth.position(0,0);
	cloth.positional_constraints[cloth.position.index_to_offset(N_cloth-1,0)] = cloth.position(int(N_cloth-1),0);

}



void display_scene()
{
	cloth.visual.update_position(cloth.position.data);
	normal_per_vertex(cloth.position.data, cloth.triangle_connectivity, cloth.normal.data);
	cloth.visual.update_normal(cloth.normal.data);
	draw(cloth.visual, scene);

	if (user.gui.wireframe)
		draw_wireframe(cloth.visual, scene);

	draw(sphere, scene);
	draw(ground, scene);
}
void display_interface()
{
	ImGui::Checkbox("Frame", &user.gui.display_frame);
	ImGui::Checkbox("Wireframe", &user.gui.wireframe); ImGui::SameLine();
	ImGui::Checkbox("Texture", &cloth.visual.shading.use_texture);
	ImGui::SliderFloat("Time scale", &timer.scale, 0.05f, 2.0f, "%.2f s");

	ImGui::SliderFloat("Stiffness", &cloth.parameters.K, 0.1f, 10.0f, "%.2f s");
	ImGui::SliderFloat("Damping", &cloth.parameters.mu, 0.0f, 30.0f, "%.2f s");
	ImGui::SliderFloat("Wind", &user.gui.wind_magnitude, 0.0f, 50.0f, "%.2f s");
	ImGui::SliderFloat("Mass", &cloth.parameters.mass_total, 0.0f, 5.0f, "%.2f s");
	ImGui::SliderInt("Samples", &cloth.N_cloth, 5, 50);
	bool change_samples = ImGui::IsItemDeactivatedAfterEdit();
	bool restart = ImGui::Button("Restart"); ImGui::SameLine();
	bool run = ImGui::Checkbox("run", &user.gui.run);
	if (run) {
		if(user.gui.run) timer.start();
		else timer.stop();
	}
	if(restart || change_samples) {
		initialize_cloth();
		user.gui.run = true;
	}
	

}


void window_size_callback(GLFWwindow* , int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	scene.projection = projection_perspective(50.0f*pi/180.0f, aspect, 0.1f, 100.0f);
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



