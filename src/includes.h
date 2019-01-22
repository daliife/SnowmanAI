//
//  includes.h
//  02-GameLoop
//
//  Copyright © 2018 Alun Evans. All rights reserved.
//
#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include "linmath.h"
#include "shaders_default.h"

bool glCheckError();
void print(lm::vec3 v);
void print(std::string s);
void print(float f);
void print(int i);