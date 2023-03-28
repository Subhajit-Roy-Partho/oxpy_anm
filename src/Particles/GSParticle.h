//
// Created by jonah on 2/25/21.
//

#ifndef GS_PARTICLE_H
#define GS_PARTICLE_H


#include "BaseParticle.h"
//#include <set>

/**
 * @brief A customisable particle. Used by CGDNAInteraction.
 * A literal replica of ANMParticle
 */

class GSParticle: public BaseParticle {
protected:

public:
    GSParticle();
    virtual ~GSParticle(); //

    virtual bool is_rigid_body() { return false; }

    virtual bool is_bonded(BaseParticle *q); //Returns if a particle is bonded or not bool
    virtual void add_bonded_neighbor(BaseParticle *nn); //To add neighbour connected particles by populating bonded_neighs vector.
    std::vector<int> bonded_neighs; //Contains the connected particles.
};
#endif //GS_PARTICLE_H
