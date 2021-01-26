#include "simulation.hpp"

using namespace vcl;


// Fill value of force applied on each particle
// - Gravity
// - Drag
// - Spring force
// - Wind force
void compute_forces(grid_2D<vec3>& force, grid_2D<vec3> const& position, grid_2D<vec3> const& velocity, grid_2D<vec3>& normals, simulation_parameters const& parameters, float wind_magnitude)
{
    size_t const N = force.size();        // Total number of particles of the cloth Nu x Nv
    size_t const N_dim = force.dimension[0]; // Number of particles along one dimension (square dimension)

    float const K  = parameters.K;
    float const m  = parameters.mass_total/N;
    float const mu = parameters.mu;
    float const	L0 = 1.0f/(N_dim-1.0f);

    // Gravity
    const vec3 g = {0,0,-9.81f};
    for(size_t k=0; k<N; ++k)
        force[k] = m*g;

    // Drag
    for(size_t k=0; k<N; ++k)
        force[k] += -mu*m*velocity[k];


    // TO DO: Add spring forces ...


}

void numerical_integration(grid_2D<vec3>& position, grid_2D<vec3>& velocity, grid_2D<vec3> const& force, float mass, float dt)
{
    size_t const N = position.size();

    for(size_t k=0; k<N; ++k)
    {
        velocity[k] = velocity[k] + dt*force[k]/mass;
        position[k] = position[k] + dt*velocity[k];
    }

}

void apply_constraints(grid_2D<vec3>& position, grid_2D<vec3>& velocity, std::map<size_t, vec3> const& positional_constraints, obstacles_parameters const& obstacles)
{
    // Fixed positions of the cloth
    for(const auto& constraints : positional_constraints)
        position[constraints.first] = constraints.second;

    // To do: apply external constraints
}


void initialize_simulation_parameters(simulation_parameters& parameters, float L_cloth, size_t N_cloth)
{
	parameters.mass_total = 0.8f;
	parameters.K  = 5.0f;
	parameters.mu = 10.0f;
}

bool detect_simulation_divergence(grid_2D<vec3> const& force, grid_2D<vec3> const& position)
{
    bool simulation_diverged = false;
    const size_t N = position.size();
    for(size_t k=0; simulation_diverged==false && k<N; ++k)
    {
        const float f = norm(force[k]);
        const vec3& p = position[k];

        if( std::isnan(f) ) // detect NaN in force
        {
            std::cout<<"NaN detected in forces"<<std::endl;
            simulation_diverged = true;
        }

        if( f>600.0f ) // detect strong force magnitude
        {
            std::cout<<" **** Warning : Strong force magnitude detected "<<f<<" at vertex "<<k<<" ****"<<std::endl;
            simulation_diverged = true;
        }

        if( std::isnan(p.x) || std::isnan(p.y) || std::isnan(p.z) ) // detect NaN in position
        {
            std::cout<<"NaN detected in positions"<<std::endl;
            simulation_diverged = true;
        }
    }

    return simulation_diverged;

}