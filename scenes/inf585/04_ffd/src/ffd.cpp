#include "ffd.hpp"



using namespace vcl;

// Helper to compute permutation(n, k) - k among n
//   avoids the recursive formula (too costly)
int binomial_coeff(int n, int k)
{
    int res = 1;
    if(k>n-k)
        k = n - k;
    for(int i=0; i<k; ++i) {
        res *= (n-i);
        res /= (i+1);
    }
    return res;
}


// Computation of the FFD deformation on the position with respect to the grid
void ffd_deform(buffer<vec3>& position, grid_3D<vec3> const& grid /** You may want to add extra parameters*/)
{
	// Get dimension of the grid
	size_t const Nx = grid.dimension.x;
	size_t const Ny = grid.dimension.y;
	size_t const Nz = grid.dimension.z;

	// Number of position to deform
	size_t const N_vertex = position.size();
	for (size_t k = 0; k < N_vertex; ++k)
	{
		// Loop over all grid points
		for (size_t kx = 0; kx < Nx; ++kx) {
			for (size_t ky = 0; ky < Ny; ++ky) {
				for (size_t kz = 0; kz < Nz; ++kz) {

					// TO DO: Should do some computations depending on the relative coordinates of the vertex k (you may need extra parameters to handle this), and the grid position.
					// Note: A grid position can be accessed as grid(kx,ky,kz)
					// ...
				}
			}
		}
		
	}
}
