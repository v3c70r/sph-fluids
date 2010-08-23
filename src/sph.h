#include <list>

using namespace std;

struct Particle;
struct GridElement;

typedef list<Particle>			ParticleList;
typedef ParticleList::iterator	ParticleListIter;

#ifndef _SPH_H_
#define _SPH_H_

#include "Vector.h"


#define PI_FLOAT				3.141592653589793f

#define SQR(x)					((x) * (x))
#define CUBE(x)					((x) * (x) * (x))
#define POW6(x)					(CUBE(x) * CUBE(x))
#define POW9(x)					(POW6(x) * CUBE(x))

#define MAX_PARTICLES			10
#define GRID_WIDTH				20
#define GRID_HEIGHT				20
#define GRID_DEPTH				20

#define GRID_POINT(g,i,j,k)		g                                   \
								[                                   \
								    (k) * GRID_HEIGHT * GRID_WIDTH  \
								  + (j) * GRID_WIDTH                \
								  + (i)                             \
								]

#define GRID(i,j,k)				GRID_POINT(grid,i,j,k)
#define SLEEPING_GRID(i,j,k)	GRID_POINT(sleeping_grid,i,j,k)

#define ADD_TO_GRID(g,p)		do { \
									int i, j, k; \
									i = (int)((p).position.x / core_radius); \
									j = (int)((p).position.y / core_radius); \
									k = (int)((p).position.z / core_radius); \
									GridElement *gelement = &GRID_POINT(g,i,j,k); \
									int *pcount = &gelement->particle_count; \
									if (*pcount < MAX_PARTICLES) \
										gelement->particles[(*pcount)++] = (p); \
									else \
										gelement->overflow_particles.push_back(p); \
								} while (0)


struct Particle
{
	float			mass;
	float			density;
	Vector3f		position;
	Vector3f		velocity;
	Vector3f		force;
	Vector3f		color_gradient;
	float			color_laplacian;

	Vector3f		p_force, v_force, t_force;

	Particle() { mass = 1.0f; }
};

struct GridElement
{
	int				particle_count;
	Particle		particles[MAX_PARTICLES];
	ParticleList	overflow_particles;

	GridElement() : particle_count(0) { }
};

extern GridElement *grid;
template <typename Function>
void foreach_particle(Function function)
{
	GridElement			*grid_element;
	Particle			*particles;
	ParticleList		*plist;
	ParticleListIter	piter;

	for (int i = 0; i < GRID_WIDTH; i++)
		for (int j = 0; j < GRID_HEIGHT; j++)
			for (int k = 0; k < GRID_DEPTH; k++)
			{
				grid_element = &GRID(i, j, k);

				/* Handle the particles directly stored in the grid. */
				particles = grid_element->particles;
				for (int x = 0; x < grid_element->particle_count; x++)
					function(particles[x]);

				/* Handle the particles not stored in the grid. */
				plist = &grid_element->overflow_particles;
				for (piter = plist->begin(); piter != plist->end(); piter++)
					function(*piter);
			}
}

#endif /* _SPH_H_ */

