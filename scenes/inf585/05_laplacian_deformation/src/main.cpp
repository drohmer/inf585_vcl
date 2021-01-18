/**
   The current program handle a set of constraints that can be interactively manipulated.
   The function to build the matrix and the righ-hand-side vector are empty, therefore the surface is not deformed.
   Objectives: 
	  1- Fill the build_matrix() and update_deformation() functions in the file "deformation.cpp" to implement a Laplacian deformation
	  2- Add the possibility to handle an As-Rigid-As-Possible deformation
*/


#include "vcl/vcl.hpp"
#include <iostream>
#include <set>

#include "deformation.hpp"

using namespace vcl;

struct picking_parameter {
	std::set<int> constraints_temporary;   // Indices of position currently in the selection cursor (before releasing the mouse)
	vec2 selection_p0;                     // Screen position of the first corner of the selection
	vec2 selection_p1;                     // Screen position of the second corner of the selection
	bool constraints_selection_mode=false; // Switch between selection mode and displacement
	bool active;
};

struct user_interaction_parameters {
	vec2 mouse_prev;
	timer_fps fps_record;
	bool cursor_on_gui = false;
	bool wireframe=false;

	timer_event_periodic timer_update = timer_event_periodic(0.2f);
	bool surface_need_update = false;

	picking_parameter picking;
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

// Constraints applied to the vertices
constraint_structure constraints;

void mouse_click_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);
void initialize_data();
void display_scene();
void display_interface();
void display_selection_rectangle();


// Surface data
mesh shape;
buffer<vec3> initial_position;
buffer<buffer<unsigned int>> one_ring;

// Least-square data
linear_system_structure linear_system;

// Visual helper
curve_drawable curve_selection;
mesh_drawable global_frame;
mesh_drawable visual;
mesh_drawable sphere;





int main(int, char* argv[])
{

	std::cout << "Run " << argv[0] << std::endl;
	scene.projection = projection_perspective(50*pi/180.0f, 1.0f, 0.1f, 100.0f);
	scene.projection_inverse = projection_perspective_inverse(50*pi/180.0f, 1.0f, 0.1f, 100.0f);

	GLFWwindow* window = create_window(1280,1024);
	std::cout << opengl_info_display() << std::endl;;

	imgui_init(window);
	glfwSetCursorPosCallback(window, mouse_move_callback);
	glfwSetMouseButtonCallback(window, mouse_click_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);
	
	std::cout<<"Initialize data ..."<<std::endl;
	initialize_data();

	std::cout<<"Start animation loop ..."<<std::endl;
	user.fps_record.start();
	user.timer_update.start();
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

		if(user.fps_record.event) {
			std::string const title = "VCL Display - "+str(user.fps_record.fps)+" fps";
			glfwSetWindowTitle(window, title.c_str());
		}

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


static int offset(int ku, int kv, int Nv)
{
    return kv+Nv*ku;
}

void initialize_data()
{
	GLuint const shader_mesh = opengl_create_shader_program(opengl_shader_preset("mesh_vertex"), opengl_shader_preset("mesh_fragment"));
	GLuint const shader_uniform_color = opengl_create_shader_program(opengl_shader_preset("single_color_vertex"), opengl_shader_preset("single_color_fragment"));
	GLuint const texture_white = opengl_texture_to_gpu(image_raw{1,1,image_color_type::rgba,{255,255,255,255}});
	mesh_drawable::default_shader = shader_mesh;
	mesh_drawable::default_texture = texture_white;
	curve_drawable::default_shader = shader_uniform_color;

	sphere = mesh_drawable(mesh_primitive_sphere());
	curve_selection = curve_drawable({vec3{0,0,0},vec3{0,0,0},vec3{0,0,0},vec3{0,0,0},vec3{0,0,0}});
	
	global_frame = mesh_drawable(mesh_primitive_frame());
	

    const int N = 15;
	shape = mesh_primitive_grid({-1,-1,0}, {1,-1,0}, {1,1,0}, {-1,1,0}, N, N);
	initial_position = shape.position;
	one_ring = connectivity_one_ring(shape.connectivity);

    constraints.fixed[offset(0,0,N)]      = shape.position[offset(0,0,N)];
    constraints.fixed[offset(N-1,0,N)]    = shape.position[offset(N-1,0,N)];
    constraints.target[offset(0,N-1,N)]   = shape.position[offset(0,N-1,N)];
    constraints.target[offset(N-1,N-1,N)] = shape.position[offset(N-1,N-1,N)];
	visual = mesh_drawable(shape);

	build_matrix(linear_system, constraints, shape, initial_position, one_ring);
    update_deformation(linear_system, constraints, shape, visual, initial_position, one_ring);

    user.surface_need_update = false;
}


void display_constraints()
{
    // Display fixed positional constraints
    sphere.transform.scale = 0.05f;
    sphere.shading.color = {0,0,1};
    for(const auto& c : constraints.fixed) {
        const int k = c.first;
        sphere.transform.translate = shape.position[k];
        draw(sphere, scene);
    }

    // Display temporary constraints in yellow
    sphere.shading.color = {1,1,0};
    for(int idx : user.picking.constraints_temporary) {
        sphere.transform.translate = shape.position[idx];
        draw(sphere, scene);
    }

    // Display target constraints
    for(const auto& c : constraints.target) {
        const int k = c.first;
        const vec3& p = c.second;

        // The real target position in white
        sphere.shading.color = {1,1,1};
        sphere.transform.scale = 0.04f;
        sphere.transform.translate = p;
        draw(sphere, scene);

        // The real position in red
        sphere.shading.color = {1,0,0};
        sphere.transform.scale = 0.05f;
        sphere.transform.translate = shape.position[k];
        draw(sphere, scene);
    }
}


void display_scene() 
{
	draw(global_frame, scene);

	visual.shading.color = {1,1,1};
	visual.shading.phong = {0.3f,0.6f,0.3f,64.0f};
	draw(visual, scene);

	if (user.wireframe) {
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		visual.shading.color = {0,0,0};
		visual.shading.phong = {1.0f,0.0f,0.0f,64.0f};
		draw(visual, scene);
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}

	display_constraints();

	if(user.picking.constraints_selection_mode)
		display_selection_rectangle();

    if (user.surface_need_update) {
        user.timer_update.update();
        if( user.timer_update.event )
			update_deformation(linear_system, constraints, shape, visual, initial_position, one_ring);
    }
}


void window_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	float const fov = 50.0f*pi/180.0f;
	scene.projection = projection_perspective(fov, aspect, 0.1f, 100.0f);
	scene.projection_inverse = projection_perspective_inverse(fov, aspect, 0.1f, 100.0f);
}


void mouse_click_callback(GLFWwindow* window, int button, int action, int mods)
{
	vec2 const cursor = glfw_get_mouse_cursor(window);
	glfw_state state = glfw_current_state(window);

	bool const mouse_release_left   = (button==GLFW_MOUSE_BUTTON_LEFT && action==GLFW_RELEASE);
    bool const mouse_release_right  = (button==GLFW_MOUSE_BUTTON_RIGHT && action==GLFW_RELEASE);

    // Reset selection at every click pressed/released
    user.picking.selection_p0 = cursor;
    user.picking.selection_p1 = cursor;

	// Click in selection mode
	if ( user.picking.constraints_selection_mode &&  state.key_shift)
	{
		// Release left click add target constraints (convert constraints_temporary to target)
        if(mouse_release_left) {
            if(user.picking.constraints_temporary.empty()) // Release the mouse with no position => clear all target constraints
                constraints.target.clear();
            else{
                // Otherwise add new target constraints
                for(int idx : user.picking.constraints_temporary) {
                    constraints.target[idx] = shape.position[idx];

                    if( constraints.fixed.find(idx)!=constraints.fixed.end() )
                        constraints.fixed.erase(idx); // Erase fixed constraints if it is now a target constraint
                }
            }
        }

		// Release right click add fixed constraints (convert constraints_temporary to fixed)
        if(mouse_release_right) {
            if(user.picking.constraints_temporary.empty()) // Release the mouse with no position => clear all fixed constraints
                constraints.fixed.clear();
            else{
                // Otherwise add new fixed constraints
                for(int idx : user.picking.constraints_temporary) {
                    constraints.fixed[idx] = shape.position[idx];
                    if( constraints.target.find(idx)!=constraints.target.end() )
                        constraints.target.erase(idx); // Erase target constraints if it is now a fixed constraint
                }
            }
        }


	}

	// If mouse click/released in selection mode: rebuild the matrix
    if(state.key_shift && user.picking.constraints_selection_mode){
        build_matrix(linear_system,constraints,shape,initial_position,one_ring);
        user.surface_need_update = false;
		update_deformation(linear_system,constraints,shape,visual,initial_position,one_ring);
        user.picking.constraints_temporary.clear();
    }

}



void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
	user.cursor_on_gui = ImGui::IsAnyWindowFocused();
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


	// Select new constraints using rectangle on screen
    if(user.picking.constraints_selection_mode){
        if( (state.mouse_click_left || state.mouse_click_right) && state.key_shift) {
            user.picking.selection_p1 = p1;
            user.picking.constraints_temporary.clear();

            // Compute extremal coordinates of the selection box
            float const x_min = std::min(user.picking.selection_p0.x, user.picking.selection_p1.x);
            float const x_max = std::max(user.picking.selection_p0.x, user.picking.selection_p1.x);
            float const y_min = std::min(user.picking.selection_p0.y, user.picking.selection_p1.y);
            float const y_max = std::max(user.picking.selection_p0.y, user.picking.selection_p1.y);

            mat4 const T = scene.projection * scene.camera.matrix_view();
            size_t const N = shape.position.size();
            for(size_t k=0; k<N; ++k)
            {
                // Compute projection coordinate of each vertex
                vec3 const& p = shape.position[k];
                vec4 p_screen = T*vec4(p,1.0f);
                p_screen /= p_screen.w;

                // Check if the projected coordinates are within the screen box
                float const x = p_screen.x;
                float const y = p_screen.y;
                if(x>x_min && x<x_max && y>y_min && y<y_max)
                    user.picking.constraints_temporary.insert(int(k)); // add a new possible constraint in the selection
            }

        }
    }
	else{
		 // Otherwise can displace constraints using shift + drag and drop
		if ((state.mouse_click_left || state.mouse_click_right) && state.key_shift)
		{
			user.picking.selection_p1 = p1;
			vec2 const tr_2D = p1 - user.mouse_prev; // translation in screen coordinates
			vec3 const tr = scene.camera.orientation() * vec3(tr_2D, 0.0f); // translation in 3D

			// Apply the translation to all target constraints
			for(auto& it : constraints.target) {
				vec3& p = it.second;
				p += tr;
			}
		}

        user.surface_need_update = true;

	}

	user.mouse_prev = p1;
	
}

void display_selection_rectangle()
{
	vec3 const d0 = camera_ray_direction(scene.camera.matrix_frame(), scene.projection_inverse, user.picking.selection_p0);
	vec3 const d1 = camera_ray_direction(scene.camera.matrix_frame(), scene.projection_inverse, vec2{user.picking.selection_p0.x, user.picking.selection_p1.y});
	vec3 const d2 = camera_ray_direction(scene.camera.matrix_frame(), scene.projection_inverse, user.picking.selection_p1);
	vec3 const d3 = camera_ray_direction(scene.camera.matrix_frame(), scene.projection_inverse, vec2{user.picking.selection_p1.x, user.picking.selection_p0.y});
	vec3 const p = scene.camera.position();

	vec3 const p0 = p + d0;
    vec3 const p1 = p + d1;
    vec3 const p2 = p + d2;
    vec3 const p3 = p + d3;

	curve_selection.update({p0,p1,p2,p3,p0});
	draw(curve_selection, scene);
}

void display_interface()
{
	ImGui::Checkbox("Wireframe", &user.wireframe);

	bool change_active_weight  = ImGui::SliderFloat("Weight Fixed", &constraints.weight_fixed, 0.05f, 10.0f, "%.3f", 3);
    bool change_passive_weight = ImGui::SliderFloat("Weight Target", &constraints.weight_target, 0.05f, 10.0f, "%.3f", 3);
	ImGui::Checkbox("Select constraint", &user.picking.constraints_selection_mode);

    if(change_active_weight || change_passive_weight) {
        build_matrix(linear_system,constraints,shape,initial_position,one_ring);
        user.surface_need_update = false;
		update_deformation(linear_system, constraints, shape, visual, initial_position, one_ring);
    }
}

void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", scene.camera.matrix_view());
	opengl_uniform(shader, "light", scene.light, false);
}


