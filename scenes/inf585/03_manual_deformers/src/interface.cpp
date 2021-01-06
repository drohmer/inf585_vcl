#include "interface.hpp"



bool display_interface(gui_widget& gui)
{
	ImGui::Checkbox("Display frame", &gui.display_frame);
	ImGui::Checkbox("Wireframe", &gui.wireframe);

	// Create interface
	ImGui::Text("Surface type:"); // Select surface to be deformed
	int* ptr_surface_type  = reinterpret_cast<int*>(&gui.surface_type); // trick - use pointer to select enum
	bool new_surface = false;
    new_surface |= ImGui::RadioButton("Plane",ptr_surface_type, surface_plane); ImGui::SameLine();
    new_surface |= ImGui::RadioButton("Cylinder",ptr_surface_type, surface_cylinder); ImGui::SameLine();
    new_surface |= ImGui::RadioButton("Sphere",ptr_surface_type, surface_sphere); ImGui::SameLine();
    new_surface |= ImGui::RadioButton("Cube",ptr_surface_type, surface_cube);  ImGui::SameLine();
    new_surface |= ImGui::RadioButton("Mesh",ptr_surface_type, surface_mesh);

	ImGui::Text("Deformer type:"); // Select the type of deformation to apply
	int* ptr_deformer_type = (int*)&gui.deformer_type;
	ImGui::RadioButton("Translate",ptr_deformer_type, deform_translate); ImGui::SameLine();
    ImGui::RadioButton("Twist",ptr_deformer_type, deform_twist); ImGui::SameLine();
    ImGui::RadioButton("Scale",ptr_deformer_type, deform_scale);

	ImGui::Text("Deformer direction:"); // Select the type of deformation to apply
	int* ptr_deformer_direction = (int*)&gui.deformer_direction;
    ImGui::RadioButton("View space",ptr_deformer_direction, direction_view_space); ImGui::SameLine();
    ImGui::RadioButton("Surface normal",ptr_deformer_direction, direction_surface_normal);

	// Select falloff distance using slider
    ImGui::SliderFloat("Falloff distance", &gui.falloff, 0.01f, 0.8f);

	return new_surface;
}