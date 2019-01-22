#pragma once
#include "includes.h"
#include "Components.h"

struct Collision {
    Collider* other;
    lm::vec3 point;
};

struct Sphere {
	lm::vec3 c;		// Center point
	float r;		// Radius of the sphere
};

struct OBB {
	lm::vec3 c;		// OBB center point
	lm::vec3 u[3];	// Local x-, y-, and z-axes
	lm::vec3 e;		// Positive halfwidth extents of OBB along each axis
};

class CollisionSystem {
	public:

		void init();
		void update(float dt);
    
		bool intersectSegmentBox(Collider& ray, Collider& box, lm::vec3& col_point, float& col_distance, float max_distance = 100000.0f);
    
		bool intersectSegmentTriangle(lm::vec3 p, lm::vec3 q, lm::vec3 a, lm::vec3 b, lm::vec3 c);
    
		bool intersectSegmentQuad(lm::vec3 p, lm::vec3 q, lm::vec3 a, lm::vec3 b, lm::vec3 c, lm::vec3 d, lm::vec3& r);
    
		bool intersectLineQuad(lm::vec3 p, lm::vec3 q, lm::vec3 a, lm::vec3 b, lm::vec3 c, lm::vec3 d, lm::vec3& r);

		bool intersectSegmentSphere(Collider& ray, Collider& sphere, float& col_distance, lm::vec3& col_point, float max_distance);

		bool intersectRaySphere(Collider& ray, Collider& sphere); 

		float EPSILON = 0.001f; //Needed in intersectBoxBox function
		bool intersectBoxBox(OBB &a, OBB &b);

};