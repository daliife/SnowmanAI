 #include "CollisionSystem.h"
#include "extern.h"
#include <math.h>

using namespace lm;

//nothing to initialise so far
void CollisionSystem::init() { }

void CollisionSystem::update(float dt) {
    //necesary variables
    lm::vec3 col_point;
    
    //reset all collisions every frame
    auto& colliders = ECS.getAllComponents<Collider>();
    for (auto& col : colliders){
        col.colliding = false;
        col.collision_distance = 10000000.0f;
        col.other = -1;
    }
    
    //test ray-box collision. This works by looping over ray colliders. For each one, we loop over box colliders
    //test collision between ray and box, updating collision distance for each collision found
    //then for future collision tests only look as far as existing stored collision distance
    for (size_t i = 0; i < colliders.size(); i++) {
        
        //if collider is ray
        if (colliders[i].collider_type == ColliderTypeRay) {
            
            //test all other colliders
            for (size_t j = 0; j < colliders.size(); j++) {
                if (j == i) continue; // no self-test
                
                if (colliders[j].collider_type == ColliderTypeBox) {
                    float col_distance = 0; //temp var to store distance
                    if (intersectSegmentBox(colliders[i], //the ray
                                            colliders[j], //the box
                                            col_point, //reference to collision point
                                            col_distance, //reference to collision distance
                                            colliders[i].collision_distance)){ //only look as far as current nearest collider

                        colliders[i].colliding = colliders[j].colliding = true;
						colliders[i].other = j; colliders[j].other = i;
                        colliders[i].collision_point = colliders[j].collision_point = col_point;
                        colliders[i].collision_distance = colliders[j].collision_distance = col_distance;
                    }
                }
				if (colliders[j].collider_type == ColliderTypeSphere) {
					float col_distance = 0; //temp var to store distance
					if (intersectSegmentSphere(colliders[i],
											   colliders[j],
											   col_distance,
										       col_point,
										       colliders[i].collision_distance)){
						if (col_distance < colliders[i].max_distance) {
							colliders[i].colliding = colliders[j].colliding = true;
							colliders[i].other = j; colliders[j].other = i;
							colliders[i].collision_point = colliders[j].collision_point = col_point;
							colliders[i].collision_distance = colliders[j].collision_distance = col_distance;
						}
					}
				}
            }
        }

		////TODO: Box to box collider
		//if (colliders[i].collider_type == ColliderTypeBox) {
		//	//test all other colliders
		//	for (size_t j = 0; j < colliders.size(); j++) {
		//		if (j == i) continue; // no self-test
		//		if (colliders[j].collider_type == ColliderTypeBox) {
		//			float col_distance = 0; //temp var to store distance
		//			if (intersectBoxBox(auto a, auto b) { //only look as far as current nearest collider
		//				colliders[i].colliding = colliders[j].colliding = true;
		//				colliders[i].other = j; colliders[j].other = i;
		//				colliders[i].collision_point = colliders[j].collision_point = col_point;
		//				colliders[i].collision_distance = colliders[j].collision_distance = col_distance;
		//			}
		//		}
		//	}
		//}

    }
}

// Calculates whether a Ray collider (treated as a segment with a finite distance)
// collides with a box collider.
// - ray: reference to ray collider object
// - box: reference to the box collider object
// - col_point: reference to an empty vec3 which will be updated with the collision point
// - reference to a float which will be updated with the distance to the nearest collider
// - optional variable which specifies the maximum distance along ray which to search
bool CollisionSystem::intersectSegmentBox(Collider& ray, Collider& box, lm::vec3& col_point, float& col_distance, float max_distance) {
    //the general approach of this function is as follows
    // - transform ray and box into world space and apply any offsets
    // - create six planes of box
    // - calculate collision of ray with each plane
    // note that there is an inherent optimization in that the intersectSegmentQuad
    // function already discards cases where ray points in same direction as quad
    // normal, so in fact we only test collisions for maximum 3 faces
    
    //get model matrices
    Transform& ray_model = ECS.getComponentFromEntity<Transform>(ray.owner);
    Transform& box_model = ECS.getComponentFromEntity<Transform>(box.owner);
    //get reference to all transforms in ECS, for world pos calculations
    std::vector<Transform>& all_transforms = ECS.getAllComponents<Transform>();
    
    //*** TRANSFORM BOX TO WORLD ***//
    //get world matrices from scene graph
    mat4 box_global = box_model.getGlobalMatrix(all_transforms);
    
    //get each corner of box in local space
    float x = box.local_halfwidth.x;
    float y = box.local_halfwidth.y;
    float z = box.local_halfwidth.z;
    vec3 off = box.local_center;
    vec3 a( -x,   y,  z);
    vec3 b( -x,  -y,  z);
    vec3 c(  x,  -y,  z);
    vec3 d(  x,   y,  z);
    vec3 e( -x,   y,  -z);
    vec3 f( -x,  -y,  -z);
    vec3 g(  x,  -y,  -z);
    vec3 h(  x,   y,  -z);
    
    //move center
    a = a + off; b = b + off; c = c + off; d = d + off;
    e = e + off; f = f + off; g = g + off; h = h + off;
    
    //multiply by model matrix
    a = box_global * a; b = box_global * b; c = box_global * c; d = box_global * d;
    e = box_global * e; f = box_global * f; g = box_global * g; h = box_global * h;
    
    
    //*** TRANSFORM RAY TO WORLD ***//
    mat4 ray_global = ray_model.getGlobalMatrix(all_transforms);
    
    //translate the center of ray locally before applying global position, then get position
    ray_global.translateLocal(ray.local_center.x, ray.local_center.y, ray.local_center.z);
    vec3 p = ray_global.position();
    
    //direction is more complex as we must rotate the it without translation or scale
    //To do this we muts multiply the direction by the InverseTranspose of the global model
    //setting translation component to zero first. This is similar to the normal matrix in a shader
    mat4 inv = ray_global;
    inv.m[12] = 0.0; inv.m[13] = 0.0; inv.m[14] = 0.0;
    inv.inverse();
    mat4 inv_trans = inv.transpose();
    vec3 q = inv_trans * ray.direction.normalize(); //normalize direction as there's no guarantee it's length = 1!
    
    //now scale q by max distance to get segment size - safe to do this as direction was normalized
    float test_distance = (ray.max_distance < max_distance ? ray.max_distance : max_distance);
    q = q * test_distance;
    
    //so far q was DIRECTION (length = ray.max_distance), now make it POINT from p
    q = p + q;
    
    //now do tests
    //quads are:
    //abcd; dcgh, hgfe, efba, adhe, bfgc
    bool abcd = intersectSegmentQuad(p, q, a, b, c, d, col_point);
    if (abcd) {
        col_distance = (ray_global.position()-col_point).length();
        return true;
    }
    bool dcgh = intersectSegmentQuad(p, q, d, c, g, h, col_point);
    if (dcgh) {
        col_distance = (ray_global.position()-col_point).length();
        return true;
    }
    bool hgfe = intersectSegmentQuad(p, q, h, g, f, e, col_point);
    if (hgfe) {
        col_distance = (ray_global.position()-col_point).length();
        return true;
    }
    bool efba = intersectSegmentQuad(p, q, e, f, b, a, col_point);
    if (efba) {
        col_distance = (ray_global.position()-col_point).length();
        return true;
    }
    bool adhe = intersectSegmentQuad(p, q, a, d, h, e, col_point);
    if (adhe) {
        col_distance = (ray_global.position()-col_point).length();
        return true;
    }
    bool bfgc = intersectSegmentQuad(p, q, b, f, g, c, col_point);
    if (bfgc) {
        col_distance = (ray_global.position()-col_point).length();
        return true;
    }
    
    //no collision! return false
    return false;
}

// Test for collision between a segment PQ and a directed, plane quad (ABDC)
// Approach is to do two ray-in-triangle tests for triangles of quad
// see pages 188 - 190 for Real Time Collision Detection (Erikson) for more info
// - p: ray start
// - q: ray end
// - abcd: points of quad, given counterclockwise
bool CollisionSystem::intersectSegmentQuad(lm::vec3 p, lm::vec3 q, lm::vec3 a, lm::vec3 b, lm::vec3 c, lm::vec3 d, lm::vec3& r) {
    vec3 ab = b - a;
    vec3 ac = c - a;
    vec3 qp = p - q;
    
    vec3 n = ab.cross(ac); //triangle normal
    
    //angle between normal and the segment. if <= 0 segment is parallel or points away
    float den = qp.dot(n);
    if (den <= 0.0f) return 0;
    
    //intersection with plane of triangle, compute value of t
    vec3 ap = p - a;
    float t = ap.dot(n);
    if (t < 0.0f) return false;
    if (t > den) return false; //only for segment, comment for ray test
    
    //now test against individual triangles using barycentric coords
    vec3 e = qp.cross(ap); //orthogonal to plane and segment
    
    //abc
    bool abc_collide = true;
    float v = ac.dot(e);
    if (v < 0.0f || v > den) {
        abc_collide = false;
    };
    float w = -(ab.dot(e));
    if (w < 0.0f || v + w > den) {
        abc_collide = false;
    }
    if (abc_collide){
        //intersect, get ray parameter distance
        float ood = 1.0f / den;
        v *= ood;
        w *= ood;
        float u = 1.0f - v - w;
        r = a*u + b*v + c*w;
        return true;
    }
    
    //acd
    vec3 ad = d - a;
    v = ad.dot(e);
    if (v < 0.0f || v > den) {
        return false;
    };
    w = -(ac.dot(e));
    if (w < 0.0f || v + w > den) {
        return false;
    }
    //intersect acd, get ray parameter distance
    float ood = 1.0f / den;
    v *= ood;
    w *= ood;
    float u = 1.0f - v - w;
    r = a*u + c*v + d*w;
    return true;
}

//See page 190 of Real Time Collision Detection
bool CollisionSystem::intersectSegmentTriangle(lm::vec3 p, lm::vec3 q, lm::vec3 a, lm::vec3 b, lm::vec3 c) {
    vec3 ab = b - a;
    vec3 ac = c - a;
    vec3 qp = p - q;
    
    vec3 n = ab.cross(ac); //triangle normal
    
    //demoninator (angle between normal and line). if <= 0 segment is parallel or points away
    float d = qp.dot(n);
    if (d <= 0.0f) return 0;
    
    //intersection with plane of triangle, compute value of t
    vec3 ap = p - a;
    float t = ap.dot(n);
    if (t < 0.0f) return false;
    if (t > d) return false; //only for segment, comment for ray test
    
    //barycentric coords
    vec3 e = qp.cross(ap);
    float v = ac.dot(e);
    if (v < 0.0f || v > d) return false;
    float w = -(ab.dot(e));
    if (w < 0.0f || v + w > d) return false;
    
    return true;
}

//See page 188 of Real Time Collision Detection
bool CollisionSystem::intersectLineQuad(lm::vec3 p, lm::vec3 q, lm::vec3 a, lm::vec3 b, lm::vec3 c, lm::vec3 d, lm::vec3& r) {
    vec3 pq = q - p;
    vec3 pa = a - p;
    vec3 pb = b - p;
    vec3 pc = c - p;
    
    vec3 m = pc.cross(pq);
    float v = pa.dot(m);
    if (v >= 0.0f) {
        //test triangle abc
        float u = -pb.dot(m);
        if (u < 0.0f) return false;
        float w = (pq.cross(pb)).dot(pa);
        if (w < 0.0f) return false;
        //if true compute r from barycentric coords
        float denom = 1.0f / (u + v + w);
        u *= denom; v *= denom; w *= denom;
        r = a*u + b*v + c*w;
    } else {
        //test triangle dac
        vec3 pd = d - p;
        float u = pd.dot(m);
        if (u < 0.0f) return false;
        float w = (pq.cross(pa)).dot(pd);
        if (w < 0.0f) return false;
        v = -v;
        //if true compute r from barycentric coords
        float denom = 1.0f / (u + v + w);
        u *= denom; v *= denom; w *= denom;
        r = a*u + d*v + c*w;
    }
    return true;
}

// See page 178 of Real Time Collision Detection
bool CollisionSystem::intersectSegmentSphere(Collider& ray, Collider& sphere, float& col_distance, lm::vec3& col_point, float max_distance) {

	//get model matrices
	Transform& ray_model = ECS.getComponentFromEntity<Transform>(ray.owner);
	Transform& sphere_model = ECS.getComponentFromEntity<Transform>(sphere.owner);
	
	//get reference to all transforms in ECS, for world pos calculations
	std::vector<Transform>& all_transforms = ECS.getAllComponents<Transform>();

	//*** TRANSFORM SPHERE TO WORLD ***//
	mat4 sphere_gm = sphere_model.getGlobalMatrix(all_transforms);

	//move center and multiply by model matrix
	vec3 sphere_final= vec3(0, 0, 0);
	sphere_final = sphere.local_center;
	sphere_final = sphere_gm * sphere_final;

	//*** TRANSFORM RAY TO WORLD ***//
	mat4 ray_global = ray_model.getGlobalMatrix(all_transforms);

	//translate the center of ray locally before applying global position, then get position
	ray_global.translateLocal(ray.local_center.x, ray.local_center.y, ray.local_center.z);
	vec3 p = ray_global.position();

	//direction is more complex as we must rotate the it without translation or scale
	//To do this we muts multiply the direction by the InverseTranspose of the global model
	//setting translation component to zero first. This is similar to the normal matrix in a shader
	mat4 inv = ray_global;
	inv.m[12] = 0.0; inv.m[13] = 0.0; inv.m[14] = 0.0;
	inv.inverse();
	mat4 inv_trans = inv.transpose();
	vec3 q = inv_trans * ray.direction.normalize(); //normalize direction as there's no guarantee it's length = 1!

	//now scale q by max distance to get segment size - safe to do this as direction was normalized
	float test_distance = (ray.max_distance < max_distance ? ray.max_distance : max_distance);
	q = q * test_distance;

	//so far q was DIRECTION (length = ray.max_distance), now make it POINT from p
	q = p + q;

	//Variable adaptation
	lm::vec3 d = (q-p).normalize();
	lm::vec3 m = p - sphere_final;
	
	// Exit if r’s origin outside s (c > 0) and r pointing away from s (b > 0)
	float b = m.dot(d);
	float c = m.dot(m) - sphere.radius * sphere.radius;
	if (c > 0.0f && b > 0.0f) return false;
		
	// A negative discriminant corresponds to ray missing sphere
	float discr = b * b - c;
	if (discr < 0.0f) return false;
	
	// Ray now found to intersect sphere, compute smallest t value of intersection
	col_distance = -b - sqrt(discr);
	
	// If t is negative, ray started inside sphere so clamp t to zero
	if (col_distance < 0.0f) col_distance = 0.0f;
	col_point = p + d * col_distance;
	
	return true;

}

// See page 179 of Real Time Collision Detection 
//More efficient performance-wise (CURRENTLY NOT WORKING)
bool CollisionSystem::intersectRaySphere(Collider& ray, Collider& sphere) {

	//TODO: Convert like intersectSegmentSphere and multiply for globalMatrix, etc

	lm::vec3 p = ray.local_center;
	lm::vec3 d = (sphere.local_center - ray.local_center).normalize();
	Sphere s;
	s.c = sphere.local_center;
	s.r = sphere.radius;

	lm::vec3 m = p - s.c;
	float c = m.dot(m) - s.r * s.r;
	// If there is definitely at least one real root, there must be an intersection
	if (c <= 0.0f) return true;
	float b = m.dot(d);
	// Early exit if ray origin outside sphere and ray pointing away from sphere
	if (b > 0.0f) return false;
	float disc = b * b - c;
	// A negative discriminant corresponds to ray missing sphere
	if (disc < 0.0f) return false;
	// Now ray must hit sphere
	return true;
}

// See page 101 of Real Time Collision Detection 
//TODO
//- Epsilon variable?
//- Matrix 3x3 problem
//- Check validity input parameters
bool intersectBoxBox(OBB &a, OBB &b) {
	//float ra, rb;
	//lm::mat4 R, AbsR;
	////Matrix33 R, AbsR;

	//// Compute rotation matrix expressing b in a’s coordinate frame
	//for (int i = 0; i < 3; i++){
	//	for (int j = 0; j < 3; j++){
	//		R.set[i][j] = a.u[i].dot(b.u[j]);
	//	}
	//}
	//// Compute translation vector t
	//lm::vec3 t = b.c - a.c;
	//// Bring translation into a’s coordinate frame
	//t = lm::vec3((t.dot(a.u[0])), (t.dot(a.u[2])), (t.dot(a.u[2])));
	//// Compute common subexpressions. Add in an epsilon term to
	//// counteract arithmetic errors when two edges are parallel and
	//// their cross product is (near) null (see text for details)
	//for (int i = 0; i < 3; i++)
	//	for (int j = 0; j < 3; j++)
	//		AbsR[i][j] = abs(R[i][j]) + EPSILON;
	//// Test axes L = A0, L = A1, L = A2
	//for (int i = 0; i < 3; i++) {
	//	ra = a.e[i];
	//	rb = b.e[0] * AbsR[i][0] + b.e[1] * AbsR[i][1] + b.e[2] * AbsR[i][2];
	//	if (abs(t[i]) > ra + rb) return false;
	//}
	//// Test axes L = B0, L = B1, L = B2
	//for (int i = 0; i < 3; i++) {
	//	ra = a.e[0] * AbsR[0][i] + a.e[1] * AbsR[1][i] + a.e[2] * AbsR[2][i];
	//	rb = b.e[i];
	//	if (abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb)return false;
	//}
	//// Test axis L = A0 x B0
	//ra = a.e[1] * AbsR[2][0] + a.e[2] * AbsR[1][0];
	//rb = b.e[1] * AbsR[0][2] + b.e[2] * AbsR[0][1];
	//if (Abs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb) return false;
	//// Test axis L = A0 x B1
	//ra = a.e[1] * AbsR[2][1] + a.e[2] * AbsR[1][1];
	//rb = b.e[0] * AbsR[0][2] + b.e[2] * AbsR[0][0];
	//if (Abs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb) return false;
	//// Test axis L = A0 x B2
	//ra = a.e[1] * AbsR[2][2] + a.e[2] * AbsR[1][2];
	//rb = b.e[0] * AbsR[0][1] + b.e[1] * AbsR[0][0];
	//if (Abs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb) return false;
	//// Test axis L = A1 x B0
	//ra = a.e[0] * AbsR[2][0] + a.e[2] * AbsR[0][0];
	//rb = b.e[1] * AbsR[1][2] + b.e[2] * AbsR[1][1];
	//if (Abs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb) return false;
	//// Test axis L = A1 x B1
	//ra = a.e[0] * AbsR[2][1] + a.e[2] * AbsR[0][1];
	//rb = b.e[0] * AbsR[1][2] + b.e[2] * AbsR[1][0];
	//if (Abs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb) return false;
	//// Test axis L = A1 x B2
	//ra = a.e[0] * AbsR[2][2] + a.e[2] * AbsR[0][2];
	//rb = b.e[0] * AbsR[1][1] + b.e[1] * AbsR[1][0];
	//if (Abs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb) return false;
	//// Test axis L = A2 x B0
	//ra = a.e[0] * AbsR[1][0] + a.e[1] * AbsR[0][0];
	//rb = b.e[1] * AbsR[2][2] + b.e[2] * AbsR[2][1];
	//if (Abs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb) return false;
	//// Test axis L = A2 x B1
	//ra = a.e[0] * AbsR[1][1] + a.e[1] * AbsR[0][1];
	//rb = b.e[0] * AbsR[2][2] + b.e[2] * AbsR[2][0];
	//if (Abs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb) return false;
	//// Test axis L = A2 x B2
	//ra = a.e[0] * AbsR[1][2] + a.e[1] * AbsR[0][2];
	//rb = b.e[0] * AbsR[2][1] + b.e[1] * AbsR[2][0];
	//if (Abs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb) return false;
	//// Since no separating axis is found, the OBBs must be intersecting
	//return true;

}

// -----------------
//BOOK LINK: http://www.r-5.org/files/books/computers/algo-list/realtime-3d/Christer_Ericson-Real-Time_Collision_Detection-EN.pdf
// -----------------