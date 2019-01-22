#include "PhysicsSystem.h"
#include "extern.h"

void PhysicsSystem::init() {}

void PhysicsSystem::update(float dt) {

	float FPS_height = 0.2f;
	float detection_threshold = 0.01f;

	//For every rigidbody apply raytracer down physics
	auto& rigidbodies = ECS.getAllComponents<RigidBody>();
	for (auto &rb : rigidbodies) {
		if (rb.useGravity) {
			//TODO: So far it is hardcoded to only update the AI downray. There should be implemented the collision detection for sphere/box and box/box to be applied in all kind of objects
			Collider& collider_down = ECS.getComponentFromEntity<Collider>(ECS.getEntity("Down Ray AI"));
			Transform& transform = ECS.getComponentFromEntity<Transform>(rb.owner);
			float dist_from_ground = (transform.position() - collider_down.collision_point).length();
			if (collider_down.colliding && dist_from_ground < FPS_height + detection_threshold) {
				transform.position(transform.position().x, collider_down.collision_point.y + FPS_height, transform.position().z);
			} else {
				transform.translate(0.0f, gravity*dt, 0.0f);
				dist_from_ground = (transform.position() - collider_down.collision_point).length();
				if (collider_down.colliding && dist_from_ground < FPS_height + detection_threshold) {
					transform.position(transform.position().x, collider_down.collision_point.y + FPS_height, transform.position().z);
				}
			}
		}
	}

}