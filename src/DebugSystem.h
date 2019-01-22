#pragma once
#include "includes.h"
#include "Shader.h"

class DebugSystem {
public:
    ~DebugSystem();
    void init();
    void update(float dt);
    
    void setActive(bool a);
private:
    //bools to draw or not
    bool draw_grid_;
    bool draw_icons_;
    bool draw_frustra_;
    bool draw_colliders_;
    
    //cube for frustra and boxes
    void createCube_();
    GLuint cube_vao_;

	//sphere for colliders
	void createSphere_();
	void createIcosahedron_();
	GLuint sphere_vao_;

    //colliders
    void createRay_();
    GLuint collider_ray_vao_;
    GLuint collider_box_vao_;
    
    //icons
    void createIcon_();
    GLuint icon_vao_;
	GLuint icon_light_texture_;
	GLuint icon_camera_texture_;
    
    //grid
    void createGrid_();
    GLuint grid_vao_;
    GLuint grid_num_indices;
    float grid_colors[12] = {
        0.7f, 0.7f, 0.7f, //grey
        1.0f, 0.5f, 0.5f, //red
        0.5f, 1.0f, 0.5f, //green
        0.5f, 0.5f, 1.0f }; //blue
    
    //shaders
    Shader* grid_shader_;
    Shader* icon_shader_;
    
    const char* line_vertex_shader_ =
    "#version 330\n"
    "layout(location = 0) in vec4 a_vertex; \n"
    "uniform mat4 u_mvp;\n"
    "uniform vec3 u_color[4];\n"
    "uniform int u_color_mod;\n"
    "out lowp vec4 v_color;\n"
    "void main() {;\n"
    "    gl_Position = u_mvp * vec4(a_vertex.xyz, 1); \n"
    "   v_color = vec4(u_color[ int(a_vertex.w) + u_color_mod ], 1.0f);\n"
    "}\n";
    
    const char* line_fragment_shader_ =
    "#version 330\n"
    "in vec4 v_color;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() {\n"
    "    fragColor = v_color;\n"
    "}\n";
    
    const char* icon_vertex_shader_ =
    "#version 330\n"
    "layout(location = 0) in vec3 a_vertex; \n"
    "layout(location = 1) in vec2 a_uv; \n"
    "out vec2 v_uv;\n"
    "uniform mat4 u_mvp;\n"
    "void main() {\n"
    "    gl_Position = u_mvp * vec4(a_vertex, 1); \n"
    "    v_uv = a_uv; \n"
    "}\n";
    
    const char* icon_fragment_shader_ =
    "#version 330\n"
    "in vec2 v_uv;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "uniform sampler2D u_icon;\n"
    "void main() {\n"
    "    fragColor = texture(u_icon, v_uv);\n"
    "}\n";
    
};

