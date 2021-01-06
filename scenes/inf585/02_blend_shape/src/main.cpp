/**
	Objective: Implement the Blend Shape deformation on the character face
	- Observe the relation between the change of weights by the sliders (blend_shapes_sliders) and the call of the function update_blend_shape
	- Add the necessary precomputation in the function initialize_data, and the update of the deformed face in update_blend_shape.
*/

#undef SOLUTION

#include "vcl/vcl.hpp"
#include <iostream>

using namespace vcl;

struct user_interaction_parameters {
	vec2 mouse_prev;
	timer_fps fps_record;
	bool cursor_on_gui = false;
	bool display_face = true;
	bool display_body = true;
	bool display_wireframe = false;
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
void window_size_callback(GLFWwindow* window, int width, int height);

void initialize_data();
void display_scene();
void blend_shapes_sliders(); // Handle the sliders related to the weights of the blend shapes
void update_blend_shape();   // Update the current visualized face with respect to the new weights


buffer<mesh> faces_storage; // Store all initial key-frame faces
mesh_drawable face;         // Face currently displayed
mesh_drawable body;         // The static body of the character

buffer<float> weights;                // Blend Shapes weights


int main(int, char* argv[])
{
	std::cout << "Run " << argv[0] << std::endl;

	GLFWwindow* window = create_window(1280,1024);
	window_size_callback(window, 1280, 1024);
	std::cout << opengl_info_display() << std::endl;;

	imgui_init(window);
	glfwSetCursorPosCallback(window, mouse_move_callback);
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
		ImGui::Text("Display: ");
		ImGui::Checkbox("Face", &user.display_face); ImGui::SameLine();
		ImGui::Checkbox("Body", &user.display_body);
		ImGui::Checkbox("Wireframe", &user.display_wireframe); 

		if(user.fps_record.event) {
			std::string const title = "VCL Display - "+str(user.fps_record.fps)+" fps";
			glfwSetWindowTitle(window, title.c_str());
		}

		blend_shapes_sliders();
		display_scene();

		user.cursor_on_gui = ImGui::IsAnyWindowFocused();
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
	GLuint const texture_white = opengl_texture_to_gpu(image_raw{1,1,image_color_type::rgba,{255,255,255,255}});
	mesh_drawable::default_shader = shader_mesh;
	mesh_drawable::default_texture = texture_white;

	scene.camera.center_of_rotation = {0,6.5,0};
	scene.camera.distance_to_center = 3.0f;

	// Load faces
	std::cout<<" Load faces ... "<<std::endl;
	faces_storage.push_back(mesh_load_file_obj("assets/face_00.obj"));
	faces_storage.push_back(mesh_load_file_obj("assets/face_01.obj"));
	faces_storage.push_back(mesh_load_file_obj("assets/face_02.obj"));
	faces_storage.push_back(mesh_load_file_obj("assets/face_03.obj"));
	faces_storage.push_back(mesh_load_file_obj("assets/face_04.obj"));
	faces_storage.push_back(mesh_load_file_obj("assets/face_05.obj"));
	face = mesh_drawable(faces_storage[0]);

	std::cout<<" Load body ... "<<std::endl;
	body = mesh_drawable(mesh_load_file_obj("assets/body.obj"));

	size_t N_face = faces_storage.size();
	weights.resize(N_face);

	// To Do:
	//  You can add here any precomputation you need to do. For instance, precomputing the difference between a pose and the reference one.
	//  Help: 
	//    faces_storage[0].position[k] - refers to the k-th vertex position of the reference pose
	//    faces_storage[k_face].position[k] - refers to the k-th vertex of the pose k_face (for k_face>0)

}

void display_scene() 
{
	if(user.display_face)
		draw(face, scene);
	if(user.display_wireframe)
		draw_wireframe(face, scene, {0,0,1});
	if(user.display_body)
		draw(body, scene);
}

void blend_shapes_sliders()
{
	// Slider of the GUI
	bool const slider_01 = ImGui::SliderFloat("w1", &weights[0], 0.0f, 1.0f);
	bool const slider_02 = ImGui::SliderFloat("w2", &weights[1], 0.0f, 1.0f);
	bool const slider_03 = ImGui::SliderFloat("w3", &weights[2], 0.0f, 1.0f);
	bool const slider_04 = ImGui::SliderFloat("w4", &weights[3], 0.0f, 1.0f);
	bool const slider_05 = ImGui::SliderFloat("w5", &weights[4], 0.0f, 1.0f);
	// return values slider_0x are true if the slider is modified, otherwise they are false.

	// If one of the slider of the GUI is modified, then call the function update_blend_shape
	if(slider_01 || slider_02 || slider_03 || slider_04 || slider_05)
		update_blend_shape();
}

void update_blend_shape()
{
	// To Do:
	//  Compute here the new face considering the current blend shape weights
	//  Note:
	//    - This function is called every time one of the slider is modified. You should avoid creating new mesh_drawable: Try to only update existing ones.
	//  Help: 
	//    - weights[k_face] - referes to the blend shape weights for the pose k_face
	//    - face.update_position( "buffer of new positions" ) - allows to update the displayed position (same with update_normal)
	//    - the function normal_per_vertex(...) - can be used to automatically compute new normals
	//    - If you use buffer, you can use operator + (or +=) between two buffers of same size to add their values. You can also multiply a buffer with a scalar value.

	std::cout<<"Called update_blend_shape with weights values: "<<weights<<std::endl; // line to remove once you have seen its action
}




void window_size_callback(GLFWwindow* window, int width, int height)
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
	if(!user.cursor_on_gui)
	{
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


