#include "deformation.hpp"



using namespace vcl;

void apply_deformation(mesh& shape, // The position of shape are the one to be deformed
	vec2 const& tr,                 // Input gesture of the user in the 2D-screen coordinates - tr must be converted into a transformation applied to the positions of shape
	buffer<vec3> const& position_before_deformation,  // Initial reference position before the deformation
	buffer<vec3> const& normal_before_deformation,    // Initial reference normals before the deformation
	gui_widget const& widget,                         // Current values of the GUI widget
	picking_parameters const& picking,                // Information on the picking point
	rotation const& camera_orientation)               // Current camera orientation - allows to convert the 2D-screen coordinates into 3D coordinates
{
	float const r = widget.falloff; // radius of influence of the deformation
	size_t const N = shape.position.size();
	for (size_t k = 0; k < N; ++k)
	{
		vec3& p_shape = shape.position[k];                             // position to deform
		vec3 const& p_shape_original = position_before_deformation[k]; // reference position before deformation
		vec3 const& p_clicked = picking.p_clicked;                     // 3D position of picked position
		vec3 const& n_clicked = picking.n_clicked;                     // normal of the surface (before deformation) at the picked position

		float const dist = norm( p_clicked - p_shape_original );       // distance between the picked position and the vertex before deformation

		// TO DO: Implement the deformation models
		// **************************************************************** //
		// ...
		if (widget.deformer_type == deform_translate) // Case of translation
		{
			// Hint: You can convert the 2D translation in screen space into a 3D translation in the view plane in multiplying 
			//       camera_orientation * (tr.x, tr.y, 0)
			vec3 const translation = camera_orientation*vec3(tr,0.0f);

			// Fake deformation (linear translation in the screen space) 
			//   the following lines should be modified to get the expected smooth deformation
			if (dist < r)
				p_shape = p_shape_original + (1-dist/r)*translation;

		}
		if (widget.deformer_type == deform_twist)
		{
			// Deformation to implement
		}
		if (widget.deformer_type == deform_scale)
		{
			// Deformation to implement"
		}

	}


}