#pragma once
#include "includes.h"
#include "Components.h"

class PhysicsSystem {

	public:
		void init();
		void update(float dt);
		float gravity = -9.8f;

	private:
		//Nothing so far

};