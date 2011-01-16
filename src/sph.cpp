#include "sph.h"
#include <sys/time.h>
#include <cstdio>

#include <iostream>

using namespace std;

#define OPEN_MP 0

const float core_radius = 1.1f;
const float gas_constant = 1000.0f;
const float mu = 0.1f;
const float rest_density = 1.2f;
const float point_damping = 2.0f;
const float sigma = 1.0f;

const float timestep = 0.01f;

GridElement *grid;
GridElement *sleeping_grid;


inline float kernel(const Vector3f &r, const float h) {
	return 315.0f / (64.0f * PI_FLOAT * POW9(h)) * CUBE(SQR(h) - dot(r, r));
}

inline Vector3f gradient_kernel(const Vector3f &r, const float h) {
	return -945.0f / (32.0f * PI_FLOAT * POW9(h)) * SQR(SQR(h) - dot(r, r)) * r;
}

inline float laplacian_kernel(const Vector3f &r, const float h) {
	return   945.0f / (32.0f * PI_FLOAT * POW9(h))
	       * (SQR(h) - dot(r, r)) * (7.0f * dot(r, r) - 3.0f * SQR(h));
}

inline Vector3f gradient_pressure_kernel(const Vector3f &r, const float h) {
	if (dot(r, r) < SQR(0.001f)) {
		return Vector3f(0.0f);
	}

	return -45.0f / (PI_FLOAT * POW6(h)) * SQR(h - length(r)) * normalize(r);
}

inline float laplacian_viscosity_kernel(const Vector3f &r, const float h) {
	return 45.0f / (PI_FLOAT * POW6(h)) * (h - length(r));
}

inline void add_density(Particle &particle, Particle &neighbour) {
	if (particle.id > neighbour.id) {
		return;
	}

	Vector3f r = particle.position - neighbour.position;
	if (dot(r, r) > SQR(core_radius)) {
		return;
	}

    float common = kernel(r, core_radius);
    particle.density += neighbour.mass * common;
	neighbour.density += particle.mass * common;
}

void sum_density(GridElement &grid_element, Particle &particle) {
	list<Particle> &plist = grid_element.particles;
	for (list<Particle>::iterator piter = plist.begin(); piter != plist.end(); piter++) {
		add_density(particle, *piter);
	}
}

inline void sum_all_density(int i, int j, int k, Particle &particle) {
	for (int z = k - 1; z <= k + 1; z++) {
		for (int y = j - 1; y <= j + 1; y++) {
			for (int x = i - 1; x <= i + 1; x++) {
				if (   (x < 0) || (x >= GRID_WIDTH)
					|| (y < 0) || (y >= GRID_HEIGHT)
					|| (z < 0) || (z >= GRID_DEPTH)) {
					continue;
				}

				sum_density(GRID(x, y, z), particle);
			}
		}
	}
}

void update_densities(int i, int j, int k) {
	GridElement &grid_element = GRID(i, j, k);

	list<Particle> &plist = grid_element.particles;
	for (list<Particle>::iterator piter = plist.begin(); piter != plist.end(); piter++) {
		sum_all_density(i, j, k, *piter);
	}
}

inline void add_forces(Particle &particle, Particle &neighbour) {
	if (particle.id >= neighbour.id) {
		return;
	}

	Vector3f r = particle.position - neighbour.position;
	if (dot(r, r) > SQR(core_radius)) {
		return;
	}

	/* Compute the pressure force. */
	Vector3f common = 0.5f * gas_constant * (  (particle.density - rest_density)
	                                + (neighbour.density - rest_density))
	         * gradient_pressure_kernel(r, core_radius);
	particle.force += -neighbour.mass / neighbour.density * common;
	neighbour.force -= -particle.mass / particle.density * common;

	/* Compute the viscosity force. */
	common = mu * (neighbour.velocity - particle.velocity)
	         * laplacian_viscosity_kernel(r, core_radius);
	particle.force += neighbour.mass / neighbour.density * common;
	neighbour.force -= particle.mass / particle.density * common;

	/* Compute the gradient of the color field. */
	common = gradient_kernel(r, core_radius);
	particle.color_gradient += neighbour.mass / neighbour.density * common;
	neighbour.color_gradient -= particle.mass / particle.density * common;

	/* Compute the gradient of the color field. */
	float value = laplacian_kernel(r, core_radius);
	particle.color_laplacian += neighbour.mass / neighbour.density * value;
	neighbour.color_laplacian += particle.mass / particle.density * value;
}

void sum_forces(GridElement &grid_element, Particle &particle) {
	list<Particle>  &plist = grid_element.particles;
	for (list<Particle>::iterator piter = plist.begin(); piter != plist.end(); piter++) {
		add_forces(particle, *piter);
	}
}

void sum_all_forces(int i, int j, int k, Particle &particle) {
	for (int z = k - 1; z <= k + 1; z++) {
		for (int y = j - 1; y <= j + 1; y++) {
			for (int x = i - 1; x <= i + 1; x++) {
				if (   (x < 0) || (x >= GRID_WIDTH)
					|| (y < 0) || (y >= GRID_HEIGHT)
					|| (z < 0) || (z >= GRID_DEPTH)) {
					continue;
				}

				sum_forces(GRID(x, y, z), particle);
			}
		}
	}
}

void update_forces(int i, int j, int k) {
	GridElement &grid_element = GRID(i, j, k);
	list<Particle>&plist = grid_element.particles;
	for (list<Particle>::iterator piter = plist.begin(); piter != plist.end(); piter++) {
		sum_all_forces(i, j, k, *piter);
	}
}

inline void update_particle(Particle &particle) {
	if (length(particle.color_gradient) > 0.001f) {
		particle.force +=   -sigma * particle.color_laplacian
		                  * normalize(particle.color_gradient);
	}

	Vector3f acceleration =   particle.force / particle.density
	               - point_damping * particle.velocity / particle.mass;
	particle.velocity += timestep * acceleration;

	particle.position += timestep * particle.velocity;
}

void update_particles(int i, int j, int k) {
	GridElement &grid_element = GRID(i, j, k);

	list<Particle> &plist = grid_element.particles;
	for (list<Particle>::iterator piter = plist.begin(); piter != plist.end(); piter++) {
		update_particle(*piter);
	}
}

inline void reset_particle(Particle &particle) {
	particle.density = 0.0f;
	particle.force = Vector3f(0.0f);
	particle.color_gradient = Vector3f(0.0f);
	particle.color_laplacian = 0.0f;
}

void reset_particles() {
	for (int k = 0; k < GRID_DEPTH; k++) {
		for (int j = 0; j < GRID_HEIGHT; j++) {
			for (int i = 0; i < GRID_WIDTH; i++) {
				GridElement &grid_element = GRID(i, j, k);

				list<Particle> &plist = grid_element.particles;
				for (list<Particle>::iterator piter = plist.begin(); piter != plist.end(); piter++) {
					reset_particle(*piter);
				}
			}
		}
	}
}

inline void insert_into_grid(int i, int j, int k) {
	GridElement &grid_element = GRID(i, j, k);

	list<Particle> &plist = grid_element.particles;
	for (list<Particle>::iterator piter = plist.begin(); piter != plist.end(); piter++) {
		ADD_TO_GRID(sleeping_grid, *piter);
	}
}

void update_grid() {
	for (int k = 0; k < GRID_DEPTH; k++) {
		for (int j = 0; j < GRID_HEIGHT; j++) {
			for (int i = 0; i < GRID_WIDTH; i++) {
				insert_into_grid(i, j, k);
				GRID(i, j, k).particles.clear();
			}
		}
	}

	/* Swap the grids. */
	swap(grid, sleeping_grid);
}

void update_densities() {
	timeval tv1, tv2;

	gettimeofday(&tv1, NULL);

#if OPEN_MP
	#pragma omp parallel for
#endif
	for (int k = 0; k < GRID_DEPTH; k++) {
		for (int j = 0; j < GRID_HEIGHT; j++) {
			for (int i = 0; i < GRID_WIDTH; i++) {
				update_densities(i, j, k);
			}
		}
	}

	gettimeofday(&tv2, NULL);
	int time = 1000 * (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) / 1000;
	printf("TIME[update_densities]: %dms\n", time);
}

void update_forces() {
	timeval tv1, tv2;

	gettimeofday(&tv1, NULL);

#if OPEN_MP
	#pragma omp parallel for
#endif
	for (int k = 0; k < GRID_DEPTH; k++) {
		for (int j = 0; j < GRID_HEIGHT; j++) {
			for (int i = 0; i < GRID_WIDTH; i++) {
				update_forces(i, j, k);
			}
		}
	}

	gettimeofday(&tv2, NULL);
	int time = 1000 * (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) / 1000;
	printf("TIME[update_forces]   : %dms\n", time);
}

void update_particles() {
	timeval tv1, tv2;

	gettimeofday(&tv1, NULL);

#if OPEN_MP
	#pragma omp parallel for
#endif
	for (int k = 0; k < GRID_DEPTH; k++) {
		for (int j = 0; j < GRID_HEIGHT; j++) {
			for (int i = 0; i < GRID_WIDTH; i++) {
				update_particles(i, j, k);
			}
		}
	}

	gettimeofday(&tv2, NULL);
	int time = 1000 * (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) / 1000;
	printf("TIME[update_particles]: %dms\n", time);
}

void update(void(*inter_hook)() = NULL, void(*post_hook)() = NULL) {
	reset_particles();

    update_densities();
    update_forces();

    /* User supplied hook, e.g. for adding custom forces (gravity, ...). */
	if (inter_hook != NULL) {
		inter_hook();
	}

    update_particles();

    /* User supplied hook, e.g. for handling collisions. */
	if (post_hook != NULL) {
		post_hook();
	}

	update_grid();
}

void init_particles(Particle *particles, int count) {
	grid = new GridElement[GRID_WIDTH * GRID_HEIGHT * GRID_DEPTH];
	sleeping_grid = new GridElement[GRID_WIDTH * GRID_HEIGHT * GRID_DEPTH];

	for (int x = 0; x < count; x++) {
		particles[x].id = x;
		ADD_TO_GRID(grid, particles[x]);
	}
}

