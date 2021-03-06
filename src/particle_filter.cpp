/*
 * particle_filter.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: Tiffany Huang
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h> 
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of 
	//   x, y, theta and their uncertainties from GPS) and all weights to 1. 
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).

num_particles = 100;

std::default_random_engine gen;

std::normal_distribution<double> N_x (x, std[0]);
std::normal_distribution<double> N_y (y, std[1]);
std::normal_distribution<double> N_theta (theta, std[2]);

for (int i = 0; i < num_particles; i++){
	
	Particle particle;
	particle.id = i;
	particle.x = N_x(gen);
	particle.y = N_y(gen);
	particle.theta = N_theta(gen);
	particle.weight = 1;

	particles.push_back(particle);
	weights.push_back(1);

	is_initialized = true;

}


}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/

default_random_engine gen;

for (int i = 0; i < num_particles; i++)
{

	double new_x;
	double new_y;
	double new_theta;

	if (yaw_rate == 0){
		new_x = particles[i].x + velocity * delta_t * cos(particles[i].theta);
		new_y = particles[i].y + velocity * delta_t * sin(particles[i].theta);
		new_theta = particles[i].theta;
	}

	else{
		new_x = particles[i].x + velocity/yaw_rate * (sin (particles[i].theta + yaw_rate * delta_t) - sin(particles[i].theta));
		new_y = particles[i].y + velocity/yaw_rate * (cos(particles[i].theta) - cos(particles[i].theta + yaw_rate * delta_t));
		new_theta = particles[i].theta + yaw_rate * delta_t;
	}

	normal_distribution<double> N_x (new_x, std_pos[0]);
	normal_distribution<double> N_y (new_y, std_pos[1]);
	normal_distribution<double> N_theta (new_theta, std_pos[2]);

	particles[i].x 		= N_x(gen);
	particles[i].y 		= N_y(gen);
	particles[i].theta  = N_theta(gen);

	std::cout << "X_new is: " << new_x << "     Y_new is: " << new_y << "   theta_new is: " << new_theta << std::endl;


}

}

void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the 
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to 
	//   implement this method and use it as a helper during the updateWeights phase.

	for (unsigned int i = 0; i < observations.size(); i++)
	{

		//populate the current observation
		LandmarkObs obs = observations[i];

		// init minimum distance to maximum possible
    	double min_dist = numeric_limits<double>::max();

		int map_id = -1;

		for (unsigned int j = 0; j < predicted.size(); j++){

			LandmarkObs pred = predicted[j];

			//Calculate the distance to the landmark
			double distance = dist( obs.x, obs.y, pred.x, pred.y);
			//std::cout << "the calculated distance to the ladmark is: " << distance << std::endl;


			if (distance < min_dist){

				min_dist = distance;
				//This is the Map ID with the closest calculated distance to the particle
				map_id = pred.id;
			}
		}

    // set the observation's id to the nearest predicted landmark's id
    observations[i].id = map_id;


	}

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
		const std::vector<LandmarkObs> &observations, const Map &map_landmarks) {
	// TODO: Update the weights of each particle using a mult-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation 
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html

	//Get each of the particles coordinates
	for (unsigned int i = 0; i < num_particles; i++){

		double x_particle 		= particles[i].x;
		double y_particle 		= particles[i].y;
		double theta_particle	= particles[i].theta;

		// create a vector to hold the map landmark locations predicted to be within sensor range of the particle
		std:vector<LandmarkObs> predictions;

		//Iterate through all of the landmark locations and calculate
		for (unsigned int j = 0; j < map_landmarks.landmark_list.size(); j++ ){

			float lm_x  = map_landmarks.landmark_list[j].x_f;
			float lm_y  = map_landmarks.landmark_list[j].y_f;
			int   lm_id = map_landmarks.landmark_list[j].id_i; 

			
			//if the landmark is within sensor range of the particle
			if (dist(lm_x, x_particle, lm_y, y_particle) <= sensor_range){
				
				//generate and store a list of these landmarks
				predictions.push_back(LandmarkObs{ lm_id, lm_x, lm_y});
			}

		//create and store a list of the particles with state transformed from vehicle coords to particle coords
		vector<LandmarkObs> transform_os;

		for (unsigned int j = 0; j < observations.size(); j++ ){

			float x_m = observations[j].x * cos(theta_particle) - observations[j].y * sin(theta_particle);
			float y_m = observations[j].x * sin(theta_particle) + observations[j].y * cos(theta_particle);


			transform_os.push_back(LandmarkObs{ observations[j].id, x_m, y_m});

		}

    	// perform dataAssociation for the predictions and transformed observations on current particle
    	dataAssociation(predictions, transform_os);


	}

}

}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight. 
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution




}

Particle ParticleFilter::SetAssociations(Particle& particle, const std::vector<int>& associations, 
                                     const std::vector<double>& sense_x, const std::vector<double>& sense_y)
{
    //particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
    // associations: The landmark id that goes along with each listed association
    // sense_x: the associations x mapping already converted to world coordinates
    // sense_y: the associations y mapping already converted to world coordinates

    particle.associations= associations;
    particle.sense_x = sense_x;
    particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
