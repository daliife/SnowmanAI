#include "GuardMovementScript.h"
#include "extern.h"
#include <math.h>

void GuardMovementScript::init() {

	// Hardcoded sockets for testing
	sockets.push_back({ 10,0,10 });
	sockets.push_back({ 20,0,0 });
	sockets.push_back({ 10,0,-10 });
	sockets.push_back({ -10,0,-10 });
	sockets.push_back({ -20,0,0 });
	sockets.push_back({ -10,0,10 });

}

void GuardMovementScript::update(float dt) {

	//TODO: Ask or create onKeyDown and Released, otherwise it enters several simes

	// TOGGLES FOR BUTTON INPUTS
	if (input_->GetKey(GLFW_KEY_1)) {
		toggleAI();
	}
	if (input_->GetKey(GLFW_KEY_2)) {
		toggleInversion();
	}
	if (input_->GetKey(GLFW_KEY_3)) {
		switch (detectionType){
			case DetectionRadius:
			detectionType = DetectionRaytrace;
			break;
			case DetectionRaytrace:
			detectionType = DetectionFrustum;
			break;
			case DetectionFrustum:
			detectionType = DetectionRadius;
			break;
		}
		std::cout << "Detection: " << detectionType << std::endl;
	}

	if (isActive) {

		// Player detected
		if (currentState == 0 && playerDetection(detectionType)) currentState = 1;
		
		// Following player until lost during x seconds
		if (currentState == 1) {
			if (!playerDetection(detectionType)) {
				isTimerOn = true;
				if (timer > max_pursuit_time) {
					currentState = 2;
					isTimerOn = true;
					timer = 0.0f;
				}
			} else {
				timer = 0.0f;
			}
		}

		// While idling check if player is detected
		if (currentState == 2 && playerDetection(detectionType)) currentState = 1;
		
		// After x seconds idling, go back to patrol
		if (currentState == 2 && timer > max_idle_time) {
			currentState = 0;
			isTimerOn = false;
			timer = 0.0f;
		}
		
		Camera& camera = ECS.getComponentInArray<Camera>(1); //player cam is 1, debug is 0
		switch (currentState) {
			case 0:
				patrol(dt);
				break;
			case 1:
				attack(camera, dt);
				break;
			case 2:
				idle(dt);
				break;
		}

		// Update timer for idling or lost player
		if (isTimerOn) timer += dt;

	}

}

void GuardMovementScript::attack(Camera& camera, float dt) {

	Transform& transform = ECS.getComponentFromEntity<Transform>(owner_);
	moveGuard(camera.position, transform, dt);
	rotateGuard(camera.position, transform, dt);

}

void GuardMovementScript::idle(float dt) {

	Transform& transform = ECS.getComponentFromEntity<Transform>(owner_);
	transform.rotateLocal(turn_speed_idle * dt,lm::vec3(0,1,0));

}

void GuardMovementScript::patrol(float dt) {

	// Get go-to location and transform
	Transform& transform = ECS.getComponentFromEntity<Transform>(owner_);
	lm::vec3 temp(sockets[curr_socket].x, sockets[curr_socket].y, sockets[curr_socket].z);

	//Update socket if close to point
	lm::vec3 dir = temp - transform.position();
	dir.y = 0.0f;
	float distance = sqrt(dir.dot(dir));
	//std::cout << "distance - " << distance << std::endl;
	if (distance < 1.0f){  
		if (invertDirection) {
			if (curr_socket <= 0) {
				curr_socket = sockets.size() - 1;
			}
			else {
				curr_socket--;
			}
		}
		else {
			if (curr_socket >= (sockets.size() - 1)) {
				curr_socket = 0;
			}
			else {
				curr_socket++;
			}
		}

	}else{
		moveGuard(temp, transform, dt);
		rotateGuard(temp, transform, dt);
	}

}

void GuardMovementScript::moveGuard(lm::vec3 target, Transform& transform, float dt) {

	// Get direction
	lm::vec3 dir = target - transform.position();
	//lm::vec3 dir = transform.front();
	dir.normalize();
	dir.y = 0.0f;

	float move_speed = 1.0f;
	switch (currentState){
	case 0:
		move_speed = patrol_speed;
		break;
	case 1:
		move_speed = attack_speed;
		break;
	case 2:
		move_speed = idle_speed;
		break;

	}
	
	// Translate transform
	float distance = sqrt(dir.dot(dir));
	if (distance > close_range) transform.translate(dir * (move_speed * dt));

}

void GuardMovementScript::rotateGuard(lm::vec3 target, Transform& transform, float dt) {

	//Get directions
	lm::vec3 patrolDir = transform.front();
	patrolDir.normalize();
	lm::vec3 desiredDir = target - transform.position();
	desiredDir.normalize();

	// Apply rotation with sign to transform
	float angle_rotation = acos(desiredDir.dot(patrolDir));
	float angle_sign = transform.front().cross(desiredDir).y;
	
	float threshold_angle = 0.001f;
	if (angle_sign < threshold_angle && angle_sign > -threshold_angle) angle_rotation = 0;
	if (angle_sign < 0.0f) {
		angle_rotation *= -1;
		angle_sign *= -1;
	}

	// Apply local rotation
	transform.rotateLocal(( turn_speed * angle_rotation * dt), lm::vec3(0, -angle_sign, 0));

}

bool GuardMovementScript::playerDetection(DetectionType type) {

	if (type == DetectionRadius) {
		Camera& camera = ECS.getComponentInArray<Camera>(1); //player cam is 1, debug is 0
		Transform& transform = ECS.getComponentFromEntity<Transform>(owner_);
		lm::vec3 dir = camera.position - transform.position();
		float distance = sqrt(dir.dot(dir));
		return distance < vision_range;
	}

	if (type == DetectionRaytrace) {
		//TODO: How to distinguish between environment and player + Player doesnt have collider/hitbox
		auto& colliders = ECS.getAllComponents<Collider>();
		Collider& collider_forward = colliders[AI_collider_forward];
		//if(collider_forward.colliding) std::cout << "COL - " << collider_forward.collision_distance << std::endl;
		return collider_forward.colliding && collider_forward.collision_distance < 10.0f;
	}

	if (type == DetectionFrustum) {
		//TODO: Implement
	}

}