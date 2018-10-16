/*
 * ChiralRodInteraction.cpp
 *
 *  Created on: 29/Oct/2013
 *      Author: Flavio
 */

#include "ChiralRodInteraction.h"

template<typename number>
ChiralRodInteraction<number>::ChiralRodInteraction() : BaseInteraction<number, ChiralRodInteraction<number> >() {
	this->_int_map[1] = &ChiralRodInteraction<number>::_chiral_pot;
	_length = (number) - 1.0f;
	_chiral_max = (number) 0.f;
	_chiral_min = (number) 0.f;
	_chiral_alpha = (number) 0.f;
	_chiral_delta = (number) 0.f;
	_chiral_interval = (number) 0.f;
	_epsilon = (number) 0.f;
}

template<typename number>
ChiralRodInteraction<number>::~ChiralRodInteraction() {

}

template<typename number>
void ChiralRodInteraction<number>::get_settings(input_file &inp) {
	IBaseInteraction<number>::get_settings(inp);

	std::string tmps;
	getInputString (&inp, "sim_type", tmps, 1);
	if (tmps.compare("MC") and tmps.compare("MC2")) throw oxDNAException ("Cannot run ChiralRodInteraction with MD");


	float tmpf;
	getInputFloat (&inp, "length", &tmpf, 1);
	_length = (number) tmpf;
	if (_length < 0.) throw oxDNAException ("Cannot run ChiralRod Interaction with negative lenght");

	getInputFloat (&inp, "chiral_delta", &tmpf, 1);
	_chiral_delta = (number) tmpf;

	getInputFloat (&inp, "chiral_alpha", &tmpf, 1);
	_chiral_alpha = (number) tmpf;

	getInputFloat (&inp, "chiral_interval", &tmpf, 1);
	_chiral_interval = (number) tmpf;

	getInputNumber (&inp, "chiral_epsilon", &_epsilon, 0);


	// this assumes that the length of the cylinders is larger than
	// the delta by quite a lot...
	this->_rcut = 1.001 * (_length + 1. + _chiral_delta);

}

template<typename number>
void ChiralRodInteraction<number>::init() {

	_chiral_min = _chiral_alpha - _chiral_interval;
	_chiral_max = _chiral_alpha + _chiral_interval;

	OX_LOG(Logger::LOG_INFO, "Initializing ChiralRod interaction with length %g, chiral_delta = %g, chiral_alpha = %g, chiral_interval = %g (min:%g, max:%g)", _length, _chiral_delta, _chiral_alpha, _chiral_interval, _chiral_min, _chiral_max);

	this->_sqr_rcut = SQR(this->_rcut);
}

template<typename number>
void ChiralRodInteraction<number>::allocate_particles(BaseParticle<number> **particles, int N) {
	for(int i = 0; i < N; i++) particles[i] = new SpheroCylinder<number>(_length);
}

template<typename number>
number ChiralRodInteraction<number>::pair_interaction(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {
	return pair_interaction_nonbonded(p, q, r, update_forces);
}

template<typename number>
number ChiralRodInteraction<number>::pair_interaction_bonded(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {
	return (number) 0.f;
}

template<typename number>
number ChiralRodInteraction<number>::pair_interaction_nonbonded(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {
	LR_vector<number> computed_r(0, 0, 0);
	if(r == NULL) {
		computed_r = this->_box->min_image(p->pos, q->pos);
		r = &computed_r;
	}

	return _chiral_pot (p, q, r, update_forces);
}

/* OLD VERSION
template<typename number>
number ChiralRodInteraction<number>::_chiral_pot(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {
	if (update_forces) throw oxDNAException ("No forces, figlio di ndrocchia");

	number rnorm = (*r).norm();
	if (rnorm > this->_sqr_rcut) return (number) 0.f;

	LR_vector<number> my_r;
	SpheroCylinder<number> * pp = NULL, * qq = NULL;
	// the algorithm is not symmetric, so we have to compute things always in the same order
	if (p->index < q->index) {
		pp = dynamic_cast< SpheroCylinder<number> *> (p);
		qq = dynamic_cast< SpheroCylinder<number> *> (q);
		my_r = *r;
	}
	else {
		pp = dynamic_cast< SpheroCylinder<number> *> (q);
		qq = dynamic_cast< SpheroCylinder<number> *> (p);
		my_r = this->_box->min_image (pp, qq);
	}

	number fact = 0.5 / (0.5 + _chiral_delta);
	bool outer_cylinder_overlap = InteractionUtils::cylinder_overlap<number> (pp, qq, fact*my_r, fact*_length);
	if (outer_cylinder_overlap == false) {
		return (number) 0.f;
	}

	bool inner_cylinder_overlap = InteractionUtils::cylinder_overlap<number> (pp, qq, my_r, _length);
	if (inner_cylinder_overlap) {
		//printf ("OVERLAP here %d %d\n", pp->index, qq->index);
		//printf ("p.pos: %g %g %g\n", pp->pos.x, pp->pos.y, pp->pos.z);
		//printf ("q.pos: %g %g %g\n", qq->pos.x, qq->pos.y, qq->pos.z);
		//printf ("my_r:  %g %g %g (%g)\n", my_r.x, my_r.y, my_r.z, sqrt(my_r.norm()));

		this->set_is_infinite (true);
		return (number) 1.e12;
	}

	// if we end up here, it means we have to deal with chirality
	LR_vector<number> chiral_axis =  pp->orientation.v3.cross(qq->orientation.v3);
	if (pp->orientation.v3 * qq->orientation.v3 < (number) 0.f)
		chiral_axis =  -chiral_axis;

	my_r.normalize();
	number chiral_angle = acos(chiral_axis * my_r); // assumes r_ij goes from i to j

	if (_chiral_min < chiral_angle && chiral_angle < _chiral_max) {
		// we's chiral
		return (number) 0.f;
	}
	else {
		// we's overlapping
		//printf ("OVERLAP here2 %d %d\n", pp->index, qq->index);
		//printf ("p.pos: %g %g %g\n", pp->pos.x, pp->pos.y, pp->pos.z);
		//printf ("q.pos: %g %g %g\n", qq->pos.x, qq->pos.y, qq->pos.z);
		//printf ("my_r:  %g %g %g (%g)\n", my_r.x, my_r.y, my_r.z, sqrt(my_r.norm()));
		this->set_is_infinite(true);
		return (number) 1.e12;
	}

	return (number) 0.f;

}
*/

template<typename number>
number ChiralRodInteraction<number>::_chiral_pot(BaseParticle<number> *p, BaseParticle<number> *q, LR_vector<number> *r, bool update_forces) {
	if (update_forces) throw oxDNAException ("No forces, figlio di ndrocchia");

	number rnorm = (*r).norm();
	if (rnorm > this->_sqr_rcut) return (number) 0.f;

	LR_vector<number> my_r;
	SpheroCylinder<number> * pp = NULL, * qq = NULL;
	// the algorithm is not symmetric, so we have to compute things always in the same order
	if (p->index < q->index) {
		pp = static_cast< SpheroCylinder<number> *> (p);
		qq = static_cast< SpheroCylinder<number> *> (q);
		my_r = *r;
	}
	else {
		pp = static_cast< SpheroCylinder<number> *> (q);
		qq = static_cast< SpheroCylinder<number> *> (p);
		my_r = this->_box->min_image (pp, qq);
	}

	// first we compute the distance between the two spherocylinders
	number hlength = _length / 2.f;
	number drdotu1 = my_r * pp->orientation.v3;
	number drdotu2 = my_r * qq->orientation.v3;
	number u1dotu2 = pp->orientation.v3 * qq->orientation.v3;

	number mu, lambda;

	number cc = 1. - u1dotu2 * u1dotu2;
	if (cc < 1.e-12) {
		// parallel line segments
		lambda = drdotu1 / 2.;
		mu = -drdotu2 / 2.;
		if (fabs(lambda) > hlength) lambda = copysign(hlength, lambda);
		if (fabs(mu) > hlength) mu = copysign(hlength, mu);
	}
	else {
		// line segments not parallel
		lambda = ( drdotu1 - u1dotu2 * drdotu2) / cc;
		mu     = (-drdotu2 + u1dotu2 * drdotu1) / cc;
		if (!(fabs (lambda) <= hlength && fabs (mu) <= hlength)) {
			number aux1 = fabs(lambda) - hlength;
			number aux2 = fabs(mu) - hlength;
			if (aux1 > aux2) {
				lambda = copysign (hlength, lambda);
				mu = lambda * u1dotu2 - drdotu2;
				if (fabs(mu) > hlength) mu = copysign (hlength, mu);
			}
			else {
				mu = copysign (hlength, mu);
				lambda = mu * u1dotu2 + drdotu1;
				if (fabs(lambda) > hlength) lambda = copysign(hlength, lambda);
			}
		}
	}
	LR_vector<number> dist = my_r - lambda * pp->orientation.v3 + mu * qq->orientation.v3;
	number dnorm = dist.norm();

	// early exit if spherocylinders too far away
	if (dnorm > ((number) 1.f + _chiral_delta)*((number) 1.f + _chiral_delta))
		return (number) 0.f;

	// we check if it goes through both rims
	//fabs(lambda + (dist * u1)) < hlength && fabs(mu + (-dist * u2)) < hlength
	bool through_rims = false;
	//if (fabs(lambda + (dist * u1)) < hlength && fabs(mu - (dist * u2)) < hlength) // the line below is equivalent
	if (fabs(mu * u1dotu2 + drdotu1) < hlength && fabs(lambda * u1dotu2 - drdotu2) < hlength)
		through_rims = true;

	if (through_rims) {
		// overlap between inner rims
		if (dnorm <= (number)1.f) {
			//printf ("overlap di tipo 1 %d %d\n", pp->index, qq->index);
			this->set_is_infinite(true);
			return (number) 1.e12;
		}

		// if we got here, it means that we have to handle the chirality.

		// first, we check that the projection of the v1 vector is positive for
		// both particles, for right handed helix
		//LR_vector<number> chiral_dir = pp->orientation.v3.cross(qq->orientation.v3);
		//if (pp->orientation.v3 * qq->orientation.v3 < 0.)
		//	chiral_dir = -chiral_dir;

		LR_vector<number> pv3 = pp->orientation.v3;
		LR_vector<number> qv3;
		if (u1dotu2 >= (number) 0.f)
			qv3 = qq->orientation.v3;
		else
			qv3 = -qq->orientation.v3;

		//handedness
		//if ((pp->orientation.v3.cross(qq->orientation.v3)) * dist < (number)0.f) {
		if (pv3.cross(qv3) * dist < 0.f) {
		//if ((pp->orientation.v1 * chiral_dir) > (number) 0.f && (qq->orientation.v1 * chiral_dir) > (number)0.f) {

			//LR_vector<number> pv3 = pp->orientation.v3 - ((pp->orientation.v3 * dist) / dnorm) * dist;
			//LR_vector<number> qv3 = qq->orientation.v3 - ((qq->orientation.v3 * dist) / dnorm) * dist;
			//pv3.normalize();
			//qv3.normalize();

			// the angle between pv3 and pq3 is what we are looking for
			number angle = LRACOS((pv3 * qv3));

			if (angle > (number)(M_PI / 2.)) throw oxDNAException ("should not happen %g\n", angle);

			if (_chiral_min < angle && angle < _chiral_max) {
				if (pp->index == 20 && false) {
					printf ("## %d %d %g\n", pp->index, qq->index, angle);
					printf ("## pp->orientation.v3: %g, %g, %g\n", pp->orientation.v3.x, pp->orientation.v3.y, pp->orientation.v3.z);
					printf ("## qq->orientation.v3: %g, %g, %g\n", qq->orientation.v3.x, qq->orientation.v3.y, qq->orientation.v3.z);
					printf ("## dist:               %g, %g, %g\n", dist.x, dist.y, dist.z);
				}
				// they are in a happy chiral configuration
				//printf ("XXX %d - %g %g %g -- %d - %g %g %g \n",
				//printf("@@ %d %d %g\n", pp->index, qq->index, angle);
				return -_epsilon;
			}
			else {
				// wrong angle
				// they are not happily chiral, so they are overlapping
				// printf ("overlap di tipo 2 %d %d (%g)\n", pp->index, qq->index, angle);
				this->set_is_infinite(true);
				return (number) 1.e12;
			}
		}
		else { // wrong handedness
			// printf ("overlap di tipo 3 %d %d\n", pp->index, qq->index);
 			this->set_is_infinite(true);
			return (number) 1.e12;
		}
	}
	else {
		// actually compute the cylinder overlap
		number fact = 0.5 / (0.5 + _chiral_delta);
		bool cylinder_overlap = InteractionUtils::cylinder_overlap<number> (pp, qq, fact * my_r, fact * _length);
		if (cylinder_overlap == true) {
			// printf ("overlap di tipo 4 %d %d\n", pp->index, qq->index);
			this->set_is_infinite(true);
			return (number)1.e12;
		}
		else {
			return (number) 0.f;
		}
	}
}

template<typename number>
void ChiralRodInteraction<number>::check_input_sanity(BaseParticle<number> **particles, int N) {

}

template<typename number>
bool ChiralRodInteraction<number>::generate_random_configuration_overlap (BaseParticle<number> *p, BaseParticle<number> *q) {
	LR_vector<number> dr = this->_box->min_image (p->pos, q->pos);
	number energy = _chiral_pot(p, q, &dr, false);

	if (this->_is_infinite == true) {
		this->set_is_infinite (false);
		dr = this->_box->min_image (q->pos, p->pos);
		_chiral_pot(q, p, &dr, false);
		if (this->_is_infinite == false) throw oxDNAException ("NOT SYMMETRIC");
	}


	this->set_is_infinite (false);
	return (energy > (number) this->_energy_threshold);
}

template class ChiralRodInteraction<float>;
template class ChiralRodInteraction<double>;
