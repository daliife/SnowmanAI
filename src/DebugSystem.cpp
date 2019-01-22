#include "DebugSystem.h"
#include "extern.h"
#include "Parsers.h"
#include "math.h"

#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

DebugSystem::~DebugSystem() {
    delete grid_shader_;
    delete icon_shader_;
}

void DebugSystem::init() {

    //init booleans
    draw_grid_ = false;
    draw_icons_ = false;
    draw_frustra_ = false;
    draw_colliders_ = false;
    
    //compile debug shaders from strings in header file
    grid_shader_ = new Shader();
    grid_shader_->compileFromStrings(line_vertex_shader_, line_fragment_shader_);
    icon_shader_ = new Shader();
    icon_shader_->compileFromStrings(icon_vertex_shader_, icon_fragment_shader_);
    
    //create geometries
    createGrid_();
    createIcon_();
    createCube_();
    createRay_();
	createSphere_();
    
    //create texture for light icon
	icon_light_texture_ = Parsers::parseTexture("data/assets/icon_light.tga");
	icon_camera_texture_ = Parsers::parseTexture("data/assets/icon_camera.tga");

}

//draws debug information or not
void DebugSystem::setActive(bool a) {
    draw_grid_ = a;
    draw_icons_ = a;
    draw_frustra_ = a;
    draw_colliders_ = a;
}

//called once per frame
void DebugSystem::update(float dt) {
    
    //get the camera view projection matrix
    lm::mat4 vp = ECS.getComponentInArray<Camera>(ECS.main_camera).view_projection;
    
    //line drawing first
    if (draw_grid_ || draw_frustra_ || draw_colliders_) {
    
        //use line shader to draw all lines and boxes
        glUseProgram(grid_shader_->program);
        GLint u_mvp = glGetUniformLocation(grid_shader_->program, "u_mvp");
        GLint u_color = glGetUniformLocation(grid_shader_->program, "u_color");
        GLint u_color_mod = glGetUniformLocation(grid_shader_->program, "u_color_mod");
        GLint u_size_scale = glGetUniformLocation(grid_shader_->program, "u_size_scale");
        GLint u_center_mod = glGetUniformLocation(grid_shader_->program, "u_center_mod");
        
        if (draw_grid_) {
            //set uniforms and draw grid
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, vp.m);
            glUniform3fv(u_color, 4, grid_colors);
            glUniform3f(u_size_scale, 1.0, 1.0, 1.0);
            glUniform3f(u_center_mod, 0.0, 0.0, 0.0);
            glUniform1i(u_color_mod, 0);
            glBindVertexArray(grid_vao_); //GRID
            glDrawElements(GL_LINES, grid_num_indices, GL_UNSIGNED_INT, 0);
        }
        
        if (draw_frustra_) {
            //draw frustra for all cameras
            auto& cameras = ECS.getAllComponents<Camera>();
            for (auto& cc : cameras) {
                lm::mat4 cam_iv = cc.view_matrix;
                cam_iv.inverse();
                lm::mat4 cam_ip = cc.projection_matrix;
                cam_ip.inverse();
                lm::mat4 cam_ivp = cc.view_projection;
                cam_ivp.inverse();
                lm::mat4 mvp =  vp * cam_ivp;
                
                //set uniforms and draw cube
                glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.m);
                glUniform1i(u_color_mod, 1); //set color to index 1 (red)
                glBindVertexArray(cube_vao_); //CUBE
                glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
            }
        }
        
        if (draw_colliders_) {
            //draw all colliders
            auto& colliders = ECS.getAllComponents<Collider>();
            for (auto& cc : colliders){
                //get transform for collider
                Transform& tc = ECS.getComponentFromEntity<Transform>(cc.owner);
                //get the colliders local model matrix in order to draw correctly
                lm::mat4 collider_matrix = tc.getGlobalMatrix(ECS.getAllComponents<Transform>());
                
                if (cc.collider_type == ColliderTypeBox) {
                
                    //now move by the box by its offset
                    collider_matrix.translateLocal(cc.local_center.x, cc.local_center.y, cc.local_center.z);
					//convert -1 -> +1 geometry to size of collider box
					collider_matrix.scaleLocal(cc.local_halfwidth.x, cc.local_halfwidth.y, cc.local_halfwidth.z);
                    //set mvp
                    lm::mat4 mvp = vp * collider_matrix;
                    
                    //set uniforms and draw
                    glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.m);
                    glUniform1i(u_color_mod, 2); //set color to index 2 (green)
                    glBindVertexArray(cube_vao_); //CUBE
                    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
                }

				if (cc.collider_type == ColliderTypeSphere) {

					//collider_matrix.scaleLocal(cc.radius, cc.radius, cc.radius);
					collider_matrix.rotateLocal(DEG2RAD * 90, lm::vec3(1, 0, 0)); //Rotation to put the junctions in north/south
					collider_matrix.translateLocal(cc.local_center.x, cc.local_center.y, cc.local_center.z);
					lm::mat4 mvp = vp * collider_matrix;

					//set uniforms and draw
					glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.m);
					glUniform1i(u_color_mod, 1); //set color to index 2 (green)
					glBindVertexArray(sphere_vao_); //SPHERE
					
					glEnable(GL_PRIMITIVE_RESTART); //Special parameters for long/lat
					glPrimitiveRestartIndex(GL_PRIMITIVE_RESTART_FIXED_INDEX);
					
					glDrawElements(GL_LINES, 903, GL_UNSIGNED_INT, 0);
				}
                
                if (cc.collider_type == ColliderTypeRay) {
                    //ray natively goes from (0,0,0 to 0,0,1) (see definition in createRay_())
                    //we need to rotate this vector so that it matches the direction specified by the component
                    //to do this we first need to find angle and axis between the two vectors
                    lm::vec3 buffer_vec(0,0,1);
                    lm::vec3 dir_norm = cc.direction;
                    dir_norm.normalize();
                    float rotation_angle = acos(dir_norm.dot(buffer_vec));
                    //if angle is PI, vector is opposite to buffer vec
                    //so rotation axis can be anything (we set it to 0,1,0 vector
                    lm::vec3 rotation_axis = lm::vec3(0,1,0);
                    //otherwise, we calculate rotation axis with cross product
                    if (rotation_angle < 3.14159f){
                        rotation_axis = dir_norm.cross(buffer_vec).normalize();
                    }
                    //now we rotate the buffer vector to
                    if (rotation_angle > 0.00001f) {
                        //only rotate if we have to
                        collider_matrix.rotateLocal(rotation_angle, rotation_axis);
                    }
                    //apply distance scale
                    collider_matrix.scaleLocal(cc.max_distance, cc.max_distance, cc.max_distance);
                    //apply center offset
                    collider_matrix.translateLocal(cc.local_center.x, cc.local_center.y, cc.local_center.z);
                    
                    //set uniforms
                    lm::mat4 mvp = vp * collider_matrix;
                    glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.m);
                    //set color to index 2 (green)
                    glUniform1i(u_color_mod, 3);
                    
                    //bind the cube vao
                    glBindVertexArray(collider_ray_vao_);
                    glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, 0);
                }
            }
        }
    }
    
    if (draw_icons_) {
        //switch to icon shader
        glUseProgram(icon_shader_->program);
        
        //get uniforms
        GLint u_mvp = glGetUniformLocation(icon_shader_->program, "u_mvp");
        GLint u_icon = glGetUniformLocation(icon_shader_->program, "u_icon");
        glUniform1i(u_icon, 0);
              
        //for each light - bind light texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, icon_light_texture_);
    
        auto& lights = ECS.getAllComponents<Light>();
        for (auto& curr_light : lights) {
            Transform& curr_light_transform = ECS.getComponentFromEntity<Transform>(curr_light.owner);

            lm::mat4 mvp_matrix = vp * curr_light_transform.getGlobalMatrix(ECS.getAllComponents<Transform>());;
            //BILLBOARDS
            //the mvp for the light contains rotation information. We want it to look at the camera always.
            //So we zero out first three columns of matrix, which contain the rotation information
            //this is an extremely simple billboard
            lm::mat4 bill_matrix;
            for (int i = 12; i < 16; i++) bill_matrix.m[i] = mvp_matrix.m[i];
            
            //send this new matrix as the MVP
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, bill_matrix.m);
            glBindVertexArray(icon_vao_);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        
        //bind camera texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, icon_camera_texture_);
        
        //for each camera, exactly the same but with camera texture
        auto& cameras = ECS.getAllComponents<Camera>();
        for (auto& curr_camera : cameras) {
            Transform& curr_cam_transform = ECS.getComponentFromEntity<Transform>(curr_camera.owner);
            lm::mat4 mvp_matrix = vp * curr_cam_transform.getGlobalMatrix(ECS.getAllComponents<Transform>());
            
            // billboard as above
            lm::mat4 bill_matrix;
            for (int i = 12; i < 16; i++) bill_matrix.m[i] = mvp_matrix.m[i];
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, bill_matrix.m);
            glBindVertexArray(icon_vao_);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            
        }
    }
    glBindVertexArray(0);
}

///////////////////////////////////////////////
// **** Functions to create geometry ********//
///////////////////////////////////////////////

//creates a simple quad to store a light texture
void DebugSystem::createIcon_() {
    float is = 0.5f;
    GLfloat icon_vertices[12]{-is, -is, 0, is, -is, 0, is, is, 0, -is, is, 0};
    GLfloat icon_uvs[8]{ 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
    GLuint icon_indices[6]{ 0, 1, 2, 0, 2, 3 };
    glGenVertexArrays(1, &icon_vao_);
    glBindVertexArray(icon_vao_);
    GLuint vbo;
    //positions
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,  sizeof(icon_vertices), icon_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    //uvs
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(icon_uvs), icon_uvs, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    //indices
    GLuint ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(icon_indices), icon_indices, GL_STATIC_DRAW);
    //unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void DebugSystem::createRay_() {
    //4th component is color
    GLfloat icon_vertices[8]{0, 0, 0, 0,
        0, 0, 1, 0};
    GLuint icon_indices[2]{ 0, 1 };
    glGenVertexArrays(1, &collider_ray_vao_);
    glBindVertexArray(collider_ray_vao_);
    GLuint vbo;
    //positions
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,  sizeof(icon_vertices), icon_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    //indices
    GLuint ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(icon_indices), icon_indices, GL_STATIC_DRAW);
    //unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void DebugSystem::createCube_() {
    
    //4th component is color!
    const GLfloat quad_vertex_buffer_data[] = {
        -1.0f,  -1.0f,  -1.0f,  0.0f,	//near bottom left
        1.0f,   -1.0f,  -1.0f,  0.0f,   //near bottom right
        1.0f,   1.0f,   -1.0f,  0.0f,   //near top right
        -1.0f,  1.0f,   -1.0f,  0.0f,   //near top left
        -1.0f,  -1.0f,  1.0f,   0.0f,   //far bottom left
        1.0f,   -1.0f,  1.0f,   0.0f,   //far bottom right
        1.0f,   1.0f,   1.0f,   0.0f,   //far top right
        -1.0f,  1.0f,   1.0f,   0.0f,   //far top left
    };
    
    const GLuint quad_index_buffer_data[] = {
        0,1, 1,2, 2,3, 3,0, //top
        4,5, 5,6, 6,7, 7,4, // bottom
        4,0, 7,3, //left
        5,1, 6,2, //right
    };

    glGenVertexArrays(1, &cube_vao_);
    glBindVertexArray(cube_vao_);
    
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_buffer_data), quad_vertex_buffer_data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_index_buffer_data), quad_index_buffer_data, GL_STATIC_DRAW);
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

}

void DebugSystem::createIcosahedron_() {

	const GLfloat icosahedronVertices[] = {
		0, -0.525731, 0.850651,             // vertices[0]
		0.850651, 0, 0.525731,              // vertices[1]
		0.850651, 0, -0.525731,             // vertices[2]
		-0.850651, 0, -0.525731,            // vertices[3]
		-0.850651, 0, 0.525731,             // vertices[4]
		-0.525731, 0.850651, 0,             // vertices[5]
		0.525731, 0.850651, 0,              // vertices[6]
		0.525731, -0.850651, 0,             // vertices[7]
		-0.525731, -0.850651, 0,            // vertices[8]
		0, -0.525731, -0.850651,            // vertices[9]
		0, 0.525731, -0.850651,             // vertices[10]
		0, 0.525731, 0.850651              // vertices[11]
	};

	const GLuint icosahedronIndicies[] = {
		1, 2, 6,
		1, 7, 2,
		3, 4, 5,
		4, 3, 8,
		6, 5, 11,
		5, 6, 10,
		9, 10, 2,
		10, 9, 3,
		7, 8, 9,
		8, 7, 0,
		11, 0, 1,
		0, 11, 4,
		6, 2, 10,
		1, 6, 11,
		3, 5, 10,
		5, 4, 11,
		2, 7, 9,
		7, 1, 0,
		3, 9, 8,
		4, 8, 0,
	};
	
	glGenVertexArrays(1, &sphere_vao_);
	glBindVertexArray(sphere_vao_);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(icosahedronVertices), icosahedronVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(icosahedronIndicies), icosahedronIndicies, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

}

void DebugSystem::createSphere_() {

	int i, j;
	int lats =  20, longs = 20;
	float size = 1.0f;
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;
	int indicator = 0;

	for (i = 0; i <= lats; i++) {
		double lat0 = M_PI * (-0.5 + (double)(i - 1) / lats);
		double z0 = size * sin(lat0);
		double zr0 =  cos(lat0);

		double lat1 = M_PI * (-0.5 + (double)i / lats);
		double z1 = size * sin(lat1);
		double zr1 =  cos(lat1);

		for (j = 0; j <= longs; j++) {
			double lng = 2 * M_PI * (double)(j - 1) / longs;
			double x = size * cos(lng);
			double y = size * sin(lng);

			vertices.push_back(x * zr0);
			vertices.push_back(y * zr0);
			vertices.push_back(z0);
			indices.push_back(indicator);
			indicator++;

			vertices.push_back(x * zr1);
			vertices.push_back(y * zr1);
			vertices.push_back(z1);
			indices.push_back(indicator);
			indicator++;
		}
		indices.push_back(GL_PRIMITIVE_RESTART_FIXED_INDEX);
	}

	glGenVertexArrays(1, &sphere_vao_);
	glBindVertexArray(sphere_vao_);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

}

//creates the debug grid for our scene
void DebugSystem::createGrid_() {
    
    std::vector<float> grid_vertices;
    const float size = 100.0f; //outer width and height
    const int div = 100; // how many divisions
    const int halfdiv = div / 2;
    const float step = size / div; // gap between divisions
    const float half = size / 2; // middle of grid
    
    float p; //temporary variable to store position
    for (int i = 0; i <= div; i++) {
        
        //lines along z-axis, need to vary x position
        p = -half + (i*step);
        //one end of line
        grid_vertices.push_back(p);
        grid_vertices.push_back(0);
        grid_vertices.push_back(half);
        if (i == halfdiv) grid_vertices.push_back(1); //color
        else grid_vertices.push_back(0);
        
        //other end of line
        grid_vertices.push_back(p);
        grid_vertices.push_back(0);
        grid_vertices.push_back(-half);
        if (i == halfdiv) grid_vertices.push_back(1); //color
        else grid_vertices.push_back(0);
        
        //lines along x-axis, need to vary z positions
        p = half - (i * step);
        //one end of line
        grid_vertices.push_back(-half);
        grid_vertices.push_back(0);
        grid_vertices.push_back(p);
        if (i == halfdiv) grid_vertices.push_back(3); //color
        else grid_vertices.push_back(0);
        
        //other end of line
        grid_vertices.push_back(half);
        grid_vertices.push_back(0);
        grid_vertices.push_back(p);
        if (i == halfdiv) grid_vertices.push_back(3); //color
        else grid_vertices.push_back(0);
    }
    
    //indices
    const int num_indices = (div + 1) * 4;
    GLuint grid_line_indices[num_indices];
    for (int i = 0; i < num_indices; i++)
        grid_line_indices[i] = i;
    
    grid_num_indices = num_indices;
    
    //gl buffers
    glGenVertexArrays(1, &grid_vao_);
    glBindVertexArray(grid_vao_);
    GLuint vbo;
    //positions
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, grid_vertices.size() * sizeof(float), &(grid_vertices[0]), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    
    //indices
    GLuint ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(grid_line_indices), grid_line_indices, GL_STATIC_DRAW);
    
    //unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}