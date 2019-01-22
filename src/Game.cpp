//
//  Game.cpp
//
//  Copyright � 2018 Alun Evans. All rights reserved.
//
#include "Game.h"
#include "Shader.h"
#include "extern.h"
#include "Parsers.h"
#include "GuardMovementScript.h"
#include <functional>

Game::Game() {}

int createFree(float aspect, ControlSystem& sys) {
	int ent_player = ECS.createEntity("PlayerFree");
	Camera& player_cam = ECS.createComponentForEntity<Camera>(ent_player);
	lm::vec3 the_position(10.0f, 6.0f, -5.0f);
	ECS.getComponentFromEntity<Transform>(ent_player).translate(the_position);
	player_cam.position = the_position;
	player_cam.forward = lm::vec3(-1.0f, -0.5f, 0.0f);
	player_cam.setPerspective(60.0f*DEG2RAD, aspect, 0.1f, 10000.0f);

	ECS.main_camera = ECS.getComponentID<Camera>(ent_player);

	sys.control_type = ControlTypeFree;

	return ent_player;
}

int createPlayer(float aspect, ControlSystem& sys) {
    int ent_player = ECS.createEntity("PlayerFPS");
    Camera& player_cam = ECS.createComponentForEntity<Camera>(ent_player);
	lm::vec3 the_position(0.0f, 15.0f, 70.0f);
    ECS.getComponentFromEntity<Transform>(ent_player).translate(the_position);
    player_cam.position = the_position;
    player_cam.forward = lm::vec3(0.0f, 0.0f, -1.0f);
    player_cam.setPerspective(60.0f*DEG2RAD, aspect, 0.01f, 10000.0f);
    
    //FPS colliders 
	//each collider ray entity is parented to the playerFPS entity
    int ent_down_ray = ECS.createEntity("Down Ray");
    Transform& down_ray_trans = ECS.createComponentForEntity<Transform>(ent_down_ray);
    down_ray_trans.parent = ECS.getComponentID<Transform>(ent_player); //set parent as player entity *transform*!
    Collider& down_ray_collider = ECS.createComponentForEntity<Collider>(ent_down_ray);
    down_ray_collider.collider_type = ColliderTypeRay;
    down_ray_collider.direction = lm::vec3(0.0,-1.0,0.0);
    down_ray_collider.max_distance = 100.0f;
    
    int ent_left_ray = ECS.createEntity("Left Ray");
    Transform& left_ray_trans = ECS.createComponentForEntity<Transform>(ent_left_ray);
    left_ray_trans.parent = ECS.getComponentID<Transform>(ent_player); //set parent as player entity *transform*!
    Collider& left_ray_collider = ECS.createComponentForEntity<Collider>(ent_left_ray);
    left_ray_collider.collider_type = ColliderTypeRay;
    left_ray_collider.direction = lm::vec3(-1.0,0.0,0.0);
    left_ray_collider.max_distance = 1.0f;
    
    int ent_right_ray = ECS.createEntity("Right Ray");
    Transform& right_ray_trans = ECS.createComponentForEntity<Transform>(ent_right_ray);
    right_ray_trans.parent = ECS.getComponentID<Transform>(ent_player); //set parent as player entity *transform*!
    Collider& right_ray_collider = ECS.createComponentForEntity<Collider>(ent_right_ray);
    right_ray_collider.collider_type = ColliderTypeRay;
    right_ray_collider.direction = lm::vec3(1.0,0.0,0.0);
    right_ray_collider.max_distance = 1.0f;
    
    int ent_forward_ray = ECS.createEntity("Forward Ray");
    Transform& forward_ray_trans = ECS.createComponentForEntity<Transform>(ent_forward_ray);
    forward_ray_trans.parent = ECS.getComponentID<Transform>(ent_player); //set parent as player entity *transform*!
    Collider& forward_ray_collider = ECS.createComponentForEntity<Collider>(ent_forward_ray);
    forward_ray_collider.collider_type = ColliderTypeRay;
    forward_ray_collider.direction = lm::vec3(0.0,0.0,-1.0);
    forward_ray_collider.max_distance = 1.0f;
    
    int ent_back_ray = ECS.createEntity("Back Ray");
    Transform& back_ray_trans = ECS.createComponentForEntity<Transform>(ent_back_ray);
    back_ray_trans.parent = ECS.getComponentID<Transform>(ent_player); //set parent as player entity *transform*!
    Collider& back_ray_collider = ECS.createComponentForEntity<Collider>(ent_back_ray);
    back_ray_collider.collider_type = ColliderTypeRay;
    back_ray_collider.direction = lm::vec3(0.0,0.0,1.0);
    back_ray_collider.max_distance = 1.0f;
    
    //the control system stores the FPS colliders 
	sys.FPS_collider_down = ECS.getComponentID<Collider>(ent_down_ray);
	sys.FPS_collider_left = ECS.getComponentID<Collider>(ent_left_ray);
	sys.FPS_collider_right = ECS.getComponentID<Collider>(ent_right_ray);
	sys.FPS_collider_forward = ECS.getComponentID<Collider>(ent_forward_ray);
	sys.FPS_collider_back = ECS.getComponentID<Collider>(ent_back_ray);
    
	ECS.main_camera = ECS.getComponentID<Camera>(ent_player);

	sys.control_type = ControlTypeFPS;

	//BOX COLLIDER TEST
	int ent_hitbox = ECS.createEntity("Hitbox Player");
	Transform& hitbox_trans = ECS.createComponentForEntity<Transform>(ent_hitbox);
	hitbox_trans.parent = ECS.getComponentID<Transform>(ent_player);
	Collider& hitbox_collider = ECS.createComponentForEntity<Collider>(ent_hitbox);
	hitbox_collider.collider_type = ColliderTypeBox;
	hitbox_collider.local_halfwidth = lm::vec3(0.3f, 1.0f, 0.3f);
	hitbox_collider.local_center = lm::vec3(0.0f, 0.0f, 0.0f);

    return ent_player;
}

int createAI(GraphicsSystem& g_sys, ScriptSystem& s_sys) {

	// AI Entity
	int snowman_shader = g_sys.loadShader("snowman_shader", "data/shaders/surface.vert", "data/shaders/surface.frag");
	int snowman_geom_id = g_sys.createGeometryFromFile("data/assets/snowman.obj");
	int snowman_material_id = g_sys.createMaterial();
	Material& snowman_material = g_sys.getMaterial(snowman_material_id);
	snowman_material.shader_id = snowman_shader;
	snowman_material.diffuse_map = Parsers::parseTexture("data/assets/snowman_c.tga");

	int snowman_entity = ECS.createEntity("snowman");
	Transform& snowman_transform = ECS.getComponentFromEntity<Transform>(snowman_entity);
	snowman_transform.scaleLocal(0.02f, 0.02f, 0.02f);
	snowman_transform.translate( 0, 10.5, -10);
	Mesh& snowman_mesh_comp = ECS.createComponentForEntity<Mesh>(snowman_entity);
	snowman_mesh_comp.geometry = snowman_geom_id;
	snowman_mesh_comp.material = snowman_material_id;

	//BOX COLLIDER TEST
	//Collider& snowman_collider = ECS.createComponentForEntity<Collider>(snowman_entity);
	//snowman_collider.collider_type = ColliderTypeBox;
	//snowman_collider.local_halfwidth = lm::vec3(75, 100, 75);
	//snowman_collider.local_center = lm::vec3(0.0, 75, 0.0);
	//ECS.createComponentForEntity<Collider>(snowman_entity);
	 
	GuardMovementScript* gps = new GuardMovementScript(snowman_entity);
	s_sys.registerScript(gps);

	//Forward Ray for AI Script
	int ent_AI_forward_ray = ECS.createEntity("Forward Ray AI");
	Transform& AI_forward_ray_trans = ECS.createComponentForEntity<Transform>(ent_AI_forward_ray);
	AI_forward_ray_trans.translate(0, 70, 0);
	AI_forward_ray_trans.scaleLocal(10, 10, 10);
	AI_forward_ray_trans.parent = ECS.getComponentID<Transform>(snowman_entity); //set parent as player entity *transform*!
	Collider& AI_forward_ray_collider = ECS.createComponentForEntity<Collider>(ent_AI_forward_ray);
	AI_forward_ray_collider.collider_type = ColliderTypeRay;
	AI_forward_ray_collider.direction = lm::vec3(0.0, 0.0, 1.0);
	AI_forward_ray_collider.max_distance = 50.0f;
	gps->AI_collider_forward = ECS.getComponentID<Collider>(ent_AI_forward_ray);

	int ent_down_ray_snow = ECS.createEntity("Down Ray AI");
	Transform& down_ray_trans = ECS.createComponentForEntity<Transform>(ent_down_ray_snow);
	down_ray_trans.parent = ECS.getComponentID<Transform>(snowman_entity); //set parent as player entity *transform*!
	Collider& down_ray_collider = ECS.createComponentForEntity<Collider>(ent_down_ray_snow);
	down_ray_collider.collider_type = ColliderTypeRay;
	down_ray_collider.direction = lm::vec3(0.0, -1.0, 0.0);
	down_ray_collider.max_distance = 100.0f;

	RigidBody& rigidbody_snowman = ECS.createComponentForEntity<RigidBody>(snowman_entity);
	rigidbody_snowman.useGravity = true;

	//int gui_image = ECS.createEntity("gui image");
	//GUIElement& eli = ECS.createComponentForEntity<GUIElement>(gui_image);
	//eli.texture = Parsers::parseTexture("data/assets/buttons3.tga");
	//eli.anchor = GUIAnchorBottomRight;
	//eli.offset = lm::vec2(-10, 10);
	//eli.onClick = std::bind(&GuardMovementScript::toggleAI, gps);

	//int gui_image_2 = ECS.createEntity("gui image 2");
	//GUIElement& eli_2 = ECS.createComponentForEntity<GUIElement>(gui_image_2);
	//eli_2.texture = Parsers::parseTexture("data/assets/buttons2.tga");
	//eli_2.anchor = GUIAnchorBottomRight;
	//eli_2.offset = lm::vec2(-120, 10);
	//eli_2.onClick = std::bind(&GuardMovementScript::toggleInversion, gps);

	//int gui_image_3 = ECS.createEntity("gui image 3");
	//GUIElement& eli_3 = ECS.createComponentForEntity<GUIElement>(gui_image_3);
	//eli_3.texture = Parsers::parseTexture("data/assets/buttons1.tga");
	//eli_3.anchor = GUIAnchorBottomRight;
	//eli_3.offset = lm::vec2(-230, 10);
	//eli.onClick = std::bind(&GuardMovementScript::toggleAI, gps);

	return snowman_entity;
}

int createSpheresDebug(GraphicsSystem& graphics_system_) {

	int gun_shader = graphics_system_.loadShader("gun_shader", "data/shaders/surface.vert", "data/shaders/surface.frag");
	int gun_geom_id = graphics_system_.createGeometryFromFile("data/assets/sphere.obj");
	int gun_material_id = graphics_system_.createMaterial();
	Material& gun_material = graphics_system_.getMaterial(gun_material_id);
	gun_material.shader_id = gun_shader;
	gun_material.diffuse_map = Parsers::parseTexture("data/assets/white.tga");
	
	int sphere_entity = ECS.createEntity("sphere_1");
	Transform& sphere_transform = ECS.getComponentFromEntity<Transform>(sphere_entity);
	sphere_transform.scale(0.5, 0.5, 0.5);
	sphere_transform.translate(-30, 2, 20);
	Mesh& sphere_mesh = ECS.createComponentForEntity<Mesh>(sphere_entity);
	sphere_mesh.geometry = gun_geom_id;
	sphere_mesh.material = gun_material_id;
	Collider& sphere_collider = ECS.createComponentForEntity<Collider>(sphere_entity);
	sphere_collider.collider_type = ColliderTypeSphere;
	sphere_collider.local_center = lm::vec3(0.0, 0.0, 0.0);
	sphere_collider.radius = 1.0f;

	int sphere_entity_2 = ECS.createEntity("sphere_2");
	Transform& sphere_transform_2 = ECS.getComponentFromEntity<Transform>(sphere_entity_2);
	sphere_transform_2.translate(-30, 2, 15);
	Mesh& sphere_mesh_2 = ECS.createComponentForEntity<Mesh>(sphere_entity_2);
	sphere_mesh_2.geometry = gun_geom_id;
	sphere_mesh_2.material = gun_material_id;
	Collider& sphere_collider_2 = ECS.createComponentForEntity<Collider>(sphere_entity_2);
	sphere_collider_2.collider_type = ColliderTypeSphere;
	sphere_collider_2.local_center = lm::vec3(0.0, 0.0, 0.0);
	sphere_collider_2.radius = 1.0f;

	int sphere_entity_3 = ECS.createEntity("sphere_3");
	Transform& sphere_transform_3 = ECS.getComponentFromEntity<Transform>(sphere_entity_3);
	sphere_transform_3.scale(3, 3, 3);
	sphere_transform_3.translate(-30, 5, 10);
	Mesh& sphere_mesh_3 = ECS.createComponentForEntity<Mesh>(sphere_entity_3);
	sphere_mesh_3.geometry = gun_geom_id;
	sphere_mesh_3.material = gun_material_id;
	Collider& sphere_collider_3 = ECS.createComponentForEntity<Collider>(sphere_entity_3);
	sphere_collider_3.collider_type = ColliderTypeSphere;
	sphere_collider_3.local_center = lm::vec3(0.0, 0.0, 0.0);
	sphere_collider_3.radius = 3.0f;

	return sphere_entity;
}

void createMultiGlyph() {
	
	//create entity and component
	int multi_glyph = ECS.createEntity("multi glyph");
	GUIText& el_multi = ECS.createComponentForEntity<GUIText>(multi_glyph);
	el_multi.width = 300;
	el_multi.height = 200;
	el_multi.font_face = "data/fonts/arial.ttf";
	el_multi.font_size = 24;
	el_multi.text = "SIMPLE SNOWMAN AI";
	el_multi.color = lm::vec3(1.0, 1.0, 1.0);
	el_multi.anchor = GUIAnchorTopLeft;
	el_multi.offset = lm::vec2(20, -20);

	//create entity and component
	int multi_glyph_2 = ECS.createEntity("multi glyph 2");
	GUIText& el_multi_2 = ECS.createComponentForEntity<GUIText>(multi_glyph_2);
	el_multi_2.width = 400;
	el_multi_2.height = 100;
	el_multi_2.font_face = "data/fonts/arial.ttf";
	el_multi_2.font_size = 20;
	el_multi_2.text = "1 - Toggle Snowman AI\n2 - Toggle direction patrol\n3 - Change detection method\nhold shift - Run\nE/W/Space - Vertical movement (freeview)";
	el_multi_2.color = lm::vec3(1.0, 1.0, 1.0);
	el_multi_2.anchor = GUIAnchorBottomLeft;
	el_multi_2.offset = lm::vec2(40, 40);

}

void Game::init() {

	//******* INIT SYSTEMS *******
	control_system_.init();
	graphics_system_.init(window_width_, window_height_);
	script_system_.init(&control_system_);
	gui_system_.init(window_width_, window_height_);
	physics_system_.init();

    //******** AUTOMATIC LOADING **********//
    Parsers::parseJSONLevel("data/assets/level_custom.json", graphics_system_);
    
	//******** PROJECT LOADING ELEMENTS **********//
	createAI(graphics_system_, script_system_);
	createSpheresDebug(graphics_system_);

	//******** UI **********//
	createMultiGlyph();

	//******** CAMERA-PLAYER CREATION **********//
	createFree((float)window_width_ / (float)window_height_, control_system_);
	createPlayer((float)window_width_ / (float)window_height_, control_system_);

    //******* INIT DEBUG SYSTEM *******
	gui_system_.lateInit();
	script_system_.lateInit();
	debug_system_.init();
    debug_system_.setActive(true);

}

//Entry point for game update code
void Game::update(float dt) {

	//update input
	control_system_.update(dt);

	//collision
	collision_system_.update(dt);

	//update physics
	physics_system_.update(dt);

	//render
	graphics_system_.update(dt);
    
    //debug
    debug_system_.update(dt);

	//gui
	gui_system_.update(dt);

	//update scripts
	script_system_.update(dt);
   
}

//update game viewports
void Game::update_viewports(int window_width, int window_height) {
	window_width_ = window_width;
	window_height_ = window_height;

	auto& cameras = ECS.getAllComponents<Camera>();
	for (auto& cam : cameras) {
		cam.setPerspective(60.0f*DEG2RAD, (float)window_width_ / (float) window_height_, 0.01f, 10000.0f);
	}

	graphics_system_.updateMainViewport(window_width_, window_height_);
	gui_system_.updateViewport(window_width_, window_height_);
}