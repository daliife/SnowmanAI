#pragma once
#include "includes.h"
#include "ScriptSystem.h"

struct Vertex {
	float x, y, z;
};

enum DetectionType {
	DetectionRadius,
	DetectionRaytrace,
	DetectionFrustum
};

class GuardMovementScript : public Script {

public:
	GuardMovementScript(int owner) : Script(owner) {}

	void init();
	void update(float dt);

	void attack(Camera& camera, float dt);
	void patrol(float dt);
	void idle(float dt);
	
	void moveGuard(lm::vec3 target, Transform& transform, float dt);
	void rotateGuard(lm::vec3 target, Transform& transform, float dt);
	bool playerDetection(DetectionType type);

	// STATE MACHINE FOR AI
	//  0 - Patrol
	//  1 - Attack
	//  2 - Idle
	GLuint currentState = 0;

	// Patrol parameters
	DetectionType detectionType = DetectionRadius;
	
	bool isActive = false;				// Toggles AI logic on and off
	bool invertDirection = true;		// Follows sockets/path CW or CCW
	float vision_range = 15.0f;			// Deteremines the radius zone that the AI guard will start detecting the player
	float close_range = 0.6f;			// Safety zone to stop AI when too close with the player
	
	// Speeds for diferent states/movements
	float attack_speed = 7.0f;
	float patrol_speed = 5.0f;
	float idle_speed = 2.0f;
	float turn_speed = 5.0f; 
	float turn_speed_idle = 15.0f;

	// Timer for idle animation/movement
	float timer = 0.0f;
	float max_idle_time = 3.0f;
	float max_pursuit_time = 2.0f;
	bool isTimerOn = false;

	// Navigation points
	std::vector<Vertex> sockets;
	int curr_socket = 2;

	//Raycast to detect the player
	int AI_collider_forward;

	void toggleAI() {
		isActive = !isActive;
		std::cout << "Active: " << isActive << std::endl;
	}

	void toggleInversion() {
		invertDirection = !invertDirection;
		std::cout << "Inverted: " << invertDirection << std::endl;
	}

};