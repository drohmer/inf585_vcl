#include "vcl/vcl.hpp"
#include <iostream>

#include "simulation.hpp"


using namespace vcl;


struct gui_parameters {
	bool display_color     = true;
	bool display_particles = true;
	bool display_radius    = false;
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
	vec3 light;
};
scene_environment scene;


void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_click_callback(GLFWwindow* window, int button, int action, int mods);
void window_size_callback(GLFWwindow* window, int width, int height);


void initialize_data();
void display_scene();
void display_interface();
void update_field_color(grid_2D<vec3>& field, vcl::buffer<particle_element> const& particles);


timer_basic timer;


sph_parameters_structure sph_parameters; // Physical parameter related to SPH
buffer<particle_element> particles;      // Storage of the particles
mesh_drawable sphere_particle; // Sphere used to display a particle
curve_drawable curve_visual;   // Circle used to display the radius h of influence

grid_2D<vec3> field;      // grid used to represent the volume of the fluid under the particles
mesh_drawable field_quad; // quad used to display this field color



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
		user.cursor_on_gui = ImGui::IsAnyWindowFocused();

		ImGui::Begin("GUI",NULL,ImGuiWindowFlags_AlwaysAutoResize);

		float const dt = 0.005f * timer.scale;
		simulate(dt, particles, sph_parameters);

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






void initialize_sph()
{
    // Initial particle spacing (relative to h)
    float const c = 0.7f;
	float const h = sph_parameters.h;


	// Fill a square with particles
	particles.clear();
    float const epsilon = 1e-3f;
    for(float x=h; x<1.0f-h; x=x+c*h)
    {
        for(float y=-1.0f+h; y<1.0f-h; y=y+c*h)
        {
            particle_element particle;
            particle.p = {x+h/8.0*rand_interval(),y+h/8.0*rand_interval(),0}; // a zero value in z position will lead to a 2D simulation
            particles.push_back(particle);
        }
    }

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

	field.resize(30,30);
	field_quad = mesh_drawable( mesh_primitive_quadrangle({-1,-1,0},{1,-1,0},{1,1,0},{-1,1,0}) );
	field_quad.shading.phong = {1,0,0};
	field_quad.texture = opengl_texture_to_gpu(field);

	initialize_sph();
	sphere_particle = mesh_drawable(mesh_primitive_sphere());
	sphere_particle.transform.scale = 0.01f;
	curve_visual.color = {1,0,0};
	curve_visual = curve_drawable(curve_primitive_circle());
}


void display_scene()
{
	if(user.gui.display_particles){
		for (size_t k = 0; k < particles.size(); ++k) {
			vec3 const& p = particles[k].p;
			sphere_particle.transform.translate = p;
			draw(sphere_particle, scene);
		}
	}

	if(user.gui.display_radius){
		curve_visual.transform.scale = sph_parameters.h;
		for (size_t k = 0; k < particles.size(); k+=10) {
			curve_visual.transform.translate = particles[k].p;
			draw(curve_visual, scene);
		}
	}

	if(user.gui.display_color){
		update_field_color(field, particles);
		opengl_update_texture_gpu(field_quad.texture, field);
		draw(field_quad, scene);
	}

}
void display_interface()
{
	ImGui::SliderFloat("Timer scale", &timer.scale, 0.01f, 4.0f, "%0.2f");

	bool const restart = ImGui::Button("Restart");
	if(restart)
		initialize_sph();

	ImGui::Checkbox("Color", &user.gui.display_color);
	ImGui::Checkbox("Particles", &user.gui.display_particles);
	ImGui::Checkbox("Radius", &user.gui.display_radius);

}


void window_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	float const L = 1.1f;
	scene.projection = projection_orthographic(-aspect*L,aspect*L,-L,L,-10.0f,10.0f);
}

void mouse_click_callback(GLFWwindow* window, int button, int action, int mods)
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

	user.mouse_prev = p1;
}

void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", scene.camera.matrix_view());
	opengl_uniform(shader, "light", scene.light, false);
}

void update_field_color(grid_2D<vec3>& field, vcl::buffer<particle_element> const& particles)
{
	field.fill({1,1,1});
	float const d = 0.1f;
	int const Nf = int(field.dimension.x);
	for (int kx = 0; kx < Nf; ++kx) {
		for (int ky = 0; ky < Nf; ++ky) {

			float f = 0.0f;
			vec3 const p0 = { 2.0f*(kx/(Nf-1.0f)-0.5f), 2.0f*(ky/(Nf-1.0f)-0.5f), 0.0f};
			for (size_t k = 0; k < particles.size(); ++k) {
				vec3 const& pi = particles[k].p;
				float const r = norm(pi-p0)/d;
				f += 0.25f*std::exp(-r*r);
			}
			field(kx,Nf-1-ky) = vec3(clamp(1-f,0,1),clamp(1-f,0,1),1);
		}
	}


}

