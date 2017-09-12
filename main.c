#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <glad/glad.h>
#include <glad/glad.c>
//#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#define GP_INCLUDE_FILEWATCHER
#include "include/gp_lib.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define MV_EASY_FONT_IMPLEMENTATION
#include "mv_easy_font.h"

#define DEBUG
#ifdef DEBUG
	#ifdef __ANDROID__
		#define log(...) __android_log_print(ANDROID_LOG_DEBUG, "PRINTF", __VA_ARGS__);
	#else
		#define log(...) printf(__VA_ARGS__);
	#endif
#else
	#define log(...)
#endif

#define TRUE 1
#define FALSE 0
#define BOOL int

#define WIN GLFWwindow

float rads_camera_x = M_PI/2.0;
float distance_camera = 2.5;
float rads_light_x = 1.0;
float distance_light = 1.0;
float3 light_dir = {1.0,1.0,1.0};

int frame = 0;

void windowclose_callback(WIN * window);
void windowsize_callback(WIN * window, int width, int height);
void cursorposition_callback(WIN * window, double mx, double my);
void key_callback(WIN* window, int key, int scancode, int action, int mods);
void mousebutton_callback(WIN * window, int button, int action, int mods);
void error_callback(int error, const char* description);
void frame_timer();


WIN* window;

typedef struct Camera {
    float3 position;
    float3 look_at_point;
    float3 direction;
    float3 up;
} Camera;

typedef struct Arrow {
    float3 a;
    float3 b;
    float3 c;
    float3 a2;
    float3 b2;
    float3 c2;
} Arrow;

void load_texture(const char* file, GLuint* tex) {
	unsigned char * data;
    int width = 0;
    int height = 0;
    int bitdepth = 0;
    GLuint texture;
    data = stbi_load(file, &width, &height, &bitdepth, STBI_rgb_alpha);
    log("load_texture: %s w: %d h: %d bitdepth: %d\n", file, width, height, bitdepth);
    // Create one OpenGL texture
    glGenTextures(1, tex);

    glBindTexture(GL_TEXTURE_2D, *tex);

    if (bitdepth <= 1) {
    	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    } else if (bitdepth == 3) {
		glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    } else {
    	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);



    free(data);
}

void update_camera(Camera* cam, float* view_matrix) {


    set_float3(&(cam->position), distance_camera*cos(rads_camera_x), 0.5,distance_camera*sin(rads_camera_x));
    set_float3(&(cam->look_at_point), 0, 0.5,  0);

	set_float3(&(cam->direction), 
									cam->look_at_point.x - cam->position.x,
									cam->look_at_point.y - cam->position.y,
									cam->look_at_point.z - cam->position.z);

	set_float3(&cam->up, 0,1,0);
	m_mat4_lookat(view_matrix, &(cam->position), &(cam->direction), &(cam->up));	
}

int init(int w, int h) {
    glfwSetErrorCallback(error_callback);
	if(!glfwInit()) {
        log("glfwInit Failed\n");
        return 1;
    } else {
        log("glfwInit Success\n");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(w, h, "stuff", NULL, NULL);
    if(!window) {
        log("glfwCreateWindow Failed\n");
        glfwTerminate();
        return 1;
    }else {
    	log("glfwCreateWindow Success\n");
    }
	
	log("Setting callbacks\n");
    glfwSetWindowCloseCallback(window, windowclose_callback);
    glfwSetWindowSizeCallback(window, windowsize_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mousebutton_callback);
    glfwSetCursorPosCallback(window, cursorposition_callback);

    log("Setting context\n");
    glfwMakeContextCurrent(window);

    if(!gladLoadGL()) {
        log("Something went wrong!\n");
        return 1;
    }

	mv_ef_init("extra/Inconsolata-Regular.ttf", 48.0, NULL, NULL);

	start_filewatcher("shaders/");

    return 0;
}

void teardown() {
	log("Exiting program\n");
	stop_filewatcher();
	glfwTerminate();
}

BOOL should_exit_gameplay_loop() {
	return glfwWindowShouldClose(window);
}

GLuint load_arrow_shaders() {
    char* str_vert = gp_read_entire_file_alloc("shaders/arrow_vertex.glsl");
    char* str_frag = gp_read_entire_file_alloc("shaders/arrow_fragment.glsl");
    GLuint program = compile_shader_program(str_vert,str_frag,
                                                          "position", NULL, NULL, NULL);
    free(str_vert);
    free(str_frag);

    return program;
}

void load_arrow_mesh(GLuint* vao, Arrow* arr, int len, int* arrows_point_count) {
    assert(arr != NULL);
    assert(len != 0);

    int elems_arrow = 6;

    int floats_per_arrow = 3 * elems_arrow;

    *arrows_point_count = floats_per_arrow * len;

    // Generate a VAO
    glGenVertexArrays(1, vao);
    glBindVertexArray(*vao);

    GLfloat* points = NULL;
    GLfloat* colors = NULL;

    //printf("Arrows: loading positions\n");
    points = (GLfloat*) malloc(len * elems_arrow * 3 * sizeof(GLfloat));
    for (int i = 0; i < len; ++i) {
        Arrow* arrow = &(arr[i]);

        points[i * floats_per_arrow + 0] = (GLfloat)arrow->a.x;
        points[i * floats_per_arrow + 1] = (GLfloat)arrow->a.y;
        points[i * floats_per_arrow + 2] = (GLfloat)arrow->a.z;
        points[i * floats_per_arrow + 3] = (GLfloat)arrow->b.x;
        points[i * floats_per_arrow + 4] = (GLfloat)arrow->b.y;
        points[i * floats_per_arrow + 5] = (GLfloat)arrow->b.z;
        points[i * floats_per_arrow + 6] = (GLfloat)arrow->c.x;
        points[i * floats_per_arrow + 7] = (GLfloat)arrow->c.y;
        points[i * floats_per_arrow + 8] = (GLfloat)arrow->c.z;

        points[i * floats_per_arrow + 9] =  (GLfloat)arrow->a2.x;
        points[i * floats_per_arrow + 10] = (GLfloat)arrow->a2.y;
        points[i * floats_per_arrow + 11] = (GLfloat)arrow->a2.z;
        points[i * floats_per_arrow + 12] = (GLfloat)arrow->b2.x;
        points[i * floats_per_arrow + 13] = (GLfloat)arrow->b2.y;
        points[i * floats_per_arrow + 14] = (GLfloat)arrow->b2.z;
        points[i * floats_per_arrow + 15] = (GLfloat)arrow->c2.x;
        points[i * floats_per_arrow + 16] = (GLfloat)arrow->c2.y;
        points[i * floats_per_arrow + 17] = (GLfloat)arrow->c2.z;
    }

    float3 r = {1,0,0};
    float3 g = {0,1,0};
    float3 b = {0,0,1};
    //printf("Arrows: loading colors\n");
    colors = (GLfloat*) malloc(len * elems_arrow * 3 * sizeof(GLfloat));
    for (int i = 0; i < len; ++i) {
     
        colors[i * floats_per_arrow + 0] = (GLfloat)g.x;
        colors[i * floats_per_arrow + 1] = (GLfloat)g.y;
        colors[i * floats_per_arrow + 2] = (GLfloat)g.z;
        colors[i * floats_per_arrow + 3] = (GLfloat)g.x;
        colors[i * floats_per_arrow + 4] = (GLfloat)g.y;
        colors[i * floats_per_arrow + 5] = (GLfloat)g.z;
        colors[i * floats_per_arrow + 6] = (GLfloat)g.x;
        colors[i * floats_per_arrow + 7] = (GLfloat)g.y;
        colors[i * floats_per_arrow + 8] = (GLfloat)g.z;

        colors[i * floats_per_arrow + 9] =  (GLfloat)r.x;
        colors[i * floats_per_arrow + 10] = (GLfloat)r.y;
        colors[i * floats_per_arrow + 11] = (GLfloat)r.z;
        colors[i * floats_per_arrow + 12] = (GLfloat)r.x;
        colors[i * floats_per_arrow + 13] = (GLfloat)r.y;
        colors[i * floats_per_arrow + 14] = (GLfloat)r.z;
        colors[i * floats_per_arrow + 15] = (GLfloat)r.x;
        colors[i * floats_per_arrow + 16] = (GLfloat)r.y;
        colors[i * floats_per_arrow + 17] = (GLfloat)r.z;
    }
    
    // Copy mesh data to VBO
    {
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            3 * (len * elems_arrow) * sizeof(GLfloat),
            points,
            GL_STATIC_DRAW);

        glVertexAttribPointer(0,3,GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);
        free(points);
        //printf("Arrows: VertexAttribArray 0 -> Positions\n");
    }
        

    {
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            3 * (len * elems_arrow) * sizeof(GLfloat),
        colors,
        GL_STATIC_DRAW);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(1);
        free(colors);
        //printf("Arrows: VertexAttribArray 1 -> Colors\n");
    }

    glBindVertexArray(0);
    //printf("Arrows: Mesh loaded\n");
}

void set_arrow(Arrow* dest, float3 a, float3 b) {
    float scale = 0.03;

    float3 dir = b - a;
    float3 normal;


    float dot = M_DOT3(dir,Z_AXIS);
    if(dot != -1.0 && dot != 1.0) {
        M_CROSS3(normal, dir, Z_AXIS);
    } else {
        M_CROSS3(normal, dir, Y_AXIS);
    }
    
    M_NORMALIZE3(dir, dir);
    M_NORMALIZE3(normal, normal);

    normal = normal * scale;
    dir = dir * scale;


    dest->a = (normal + a);
    dest->b = ((normal * -1.0) + a);
    dest->c = b;

    float3 ortho;
    M_CROSS3(ortho, dir, normal);
    M_NORMALIZE3(ortho , ortho);    
    ortho = ortho * scale;
    dest->a2 = (ortho + a);
    dest->b2 = ((ortho * -1.0) + a);
    dest->c2 = b;
}

#define NUMBER_ARROWS 4
void update_arrows(Arrow* arrows, int number_arrows, Camera camera, float3* lights, int number_lights) {
    assert(arrows != NULL);
    assert(number_arrows == NUMBER_ARROWS);

    int idx = 0;
    set_arrow(&arrows[idx++], ORIGIN, X_AXIS);
    set_arrow(&arrows[idx++], ORIGIN, Y_AXIS);
    set_arrow(&arrows[idx++], ORIGIN, Z_AXIS);    
    set_arrow(&arrows[idx++], ORIGIN, light_dir);
}

GLuint load_model_shaders() {
	//char* str_vert = gp_read_entire_file_alloc("shaders/model_vertex_normal_mapping_2.glsl");
	//char* str_frag = gp_read_entire_file_alloc("shaders/model_fragment_normal_mapping_2.glsl");
    char* str_vert = gp_read_entire_file_alloc("shaders/model_vertex_pbr_1.glsl");
    char* str_frag = gp_read_entire_file_alloc("shaders/model_fragment_pbr_1.glsl");
    GLuint program = compile_shader_program(str_vert,str_frag,
    													  "position", "normal", "uv", "tangent");
    free(str_vert);
    free(str_frag);

    return program;
}

void gameplay_loop(int w, int h) {
	char* debug_string = (char*) malloc(200 * sizeof(char));

	GLuint model_vao;
    int model_point_count = 0;
    //assert(load_mesh("models/chest/Chest.obj", &model_vao, &model_point_count));
    //assert(load_mesh("models/FireHydrant/FireHydrantMesh.obj", &model_vao, &model_point_count));
    assert(load_mesh("models/round.obj", &model_vao, &model_point_count));
    

    GLuint model_texture;
    load_texture("textures/texture_3.png", &model_texture);

	GLuint pbr_albedomap_texture;
	GLuint pbr_normalmap_texture;
	GLuint pbr_metallicmap_texture;
	GLuint pbr_roughnessmap_texture;
	GLuint pbr_aomap_texture;

    
    int tex_int = 5;
    switch (tex_int) {
        case 0:
            load_texture("models/FireHydrant/fire_hydrant_Base_Color.png", &pbr_albedomap_texture);
            load_texture("models/FireHydrant/fire_hydrant_Normal_OpenGL.png",&pbr_normalmap_texture);
            load_texture("models/FireHydrant/fire_hydrant_Roughness.png", &pbr_roughnessmap_texture);
            load_texture("models/FireHydrant/fire_hydrant_Metallic.png", &pbr_metallicmap_texture);
            load_texture("models/FireHydrant/fire_hydrant_Mixed_AO.png", &pbr_aomap_texture);
            break;
        case 1:
            load_texture("models/chest/chest_albedo.png", &pbr_albedomap_texture);
            load_texture("models/chest/chest_normal.png", &pbr_normalmap_texture);
            load_texture("models/chest/chest_metalness.png", &pbr_metallicmap_texture);
            load_texture("models/chest/chest_roughness.png", &pbr_roughnessmap_texture);
            load_texture("models/chest/chest_ao.png", &pbr_aomap_texture);
            break;
        case 2:
            load_texture("textures/pbr/scuffed-plastic/albedo.png", &pbr_albedomap_texture);
            load_texture("textures/pbr/scuffed-plastic/normal.png",&pbr_normalmap_texture);
            load_texture("textures/pbr/scuffed-plastic/roughness.png", &pbr_roughnessmap_texture);
            load_texture("textures/pbr/scuffed-plastic/metal.png", &pbr_metallicmap_texture);
            //load_texture("textures/pbr/scuffed-plastic_AO.png", &pbr_aomap_texture);
            break;
        case 3:
            load_texture("textures/pbr/bamboo-wood-semigloss/albedo.png", &pbr_albedomap_texture);
            load_texture("textures/pbr/bamboo-wood-semigloss/normal.png",&pbr_normalmap_texture);
            load_texture("textures/pbr/bamboo-wood-semigloss/roughness.png", &pbr_roughnessmap_texture);
            load_texture("textures/pbr/bamboo-wood-semigloss/metal.png", &pbr_metallicmap_texture);
            //load_texture("textures/pbr/scuffed-plastic_AO.png", &pbr_aomap_texture);
            break;
        case 4:
            load_texture("textures/pbr/rustediron-streaks/albedo.png", &pbr_albedomap_texture);
            load_texture("textures/pbr/rustediron-streaks/normal.png",&pbr_normalmap_texture);
            load_texture("textures/pbr/rustediron-streaks/roughness.png", &pbr_roughnessmap_texture);
            load_texture("textures/pbr/rustediron-streaks/metal.png", &pbr_metallicmap_texture);
            //load_texture("textures/pbr/scuffed-plastic_AO.png", &pbr_aomap_texture);
            break;
        case 5:
            load_texture("textures/pbr/wall/albedo.png", &pbr_albedomap_texture);
            load_texture("textures/pbr/wall/normal.png",&pbr_normalmap_texture);
            load_texture("textures/pbr/wall/roughness.png", &pbr_roughnessmap_texture);
            load_texture("textures/pbr/wall/metallic.png", &pbr_metallicmap_texture);
            load_texture("textures/pbr/wall/ao.png", &pbr_aomap_texture);
            break;
        default:
            printf("wrong tex_int\n");

    }
    
	int max_texture_units = 0;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
	log("This HW supports: %d texture image units", max_texture_units);

	float view_matrix[] = M_MAT4_IDENTITY();
	float projection_matrix[] = M_MAT4_IDENTITY();
	
    float aspect = w / (float)h;
    m_mat4_perspective(projection_matrix, 10.0, aspect, 0.1, 100.0);
    m_mat4_identity(view_matrix);

    Camera* camera = (Camera*) malloc(sizeof(Camera));
   	update_camera(camera, view_matrix);

	float model_matrix[] = M_MAT4_IDENTITY();
	float model_scale_matrix[] = M_MAT4_IDENTITY();
	float model_rotation_matrix[] = M_MAT4_IDENTITY();
	float3 model_scale;
	set_float3(&model_scale,0.8,0.8,0.8);
	m_mat4_scale(model_scale_matrix, &model_scale);

	GLuint model_program = load_model_shaders();


    GLuint arrow_program = load_arrow_shaders();
    GLuint arrow_vao;
    
    int number_arrows = NUMBER_ARROWS;
    Arrow* arrows = (Arrow*) malloc(number_arrows * sizeof(Arrow));
    
    update_arrows(arrows, number_arrows, *camera, NULL, 0);
    int arrows_point_count = 0;
    load_arrow_mesh(&arrow_vao, arrows, number_arrows, &arrows_point_count);

    GLuint loc_arrow_model_matrix = glGetUniformLocation(arrow_program, "u_model_matrix");
    GLuint loc_arrow_view_matrix = glGetUniformLocation(arrow_program, "u_view_matrix");
    GLuint loc_arrow_projecion_matrix = glGetUniformLocation(arrow_program, "u_projection_matrix");

    GLuint loc_time = glGetUniformLocation(model_program, "u_time");
    GLuint loc_light = glGetUniformLocation(model_program, "u_light");
    GLuint loc_camera_world = glGetUniformLocation(model_program, "u_camera_world");
	GLuint loc_model_matrix = glGetUniformLocation(model_program, "u_model_matrix");
    GLuint loc_view_matrix = glGetUniformLocation(model_program, "u_view_matrix");
    GLuint loc_projecion_matrix = glGetUniformLocation(model_program, "u_projection_matrix");
    GLuint loc_texture_0 = glGetUniformLocation(model_program, "u_texture");

    GLuint loc_pbr_albedomap = glGetUniformLocation(model_program, "u_albedoMap");
    GLuint loc_pbr_normalmap = glGetUniformLocation(model_program, "u_normalMap");
    GLuint loc_pbr_metallicmap = glGetUniformLocation(model_program, "u_metallicMap");
    GLuint loc_pbr_roughnessmap = glGetUniformLocation(model_program, "u_roughnessMap");
    GLuint loc_pbr_aomap = glGetUniformLocation(model_program, "u_aoMap");

	glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.5f, 0.5f, 1.0f);
	glViewport(0, 0, w, h);
	
    while(!should_exit_gameplay_loop()) {
    	frame_timer();
    	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    	update_camera(camera, view_matrix);

        update_arrows(arrows, number_arrows, *camera, NULL, 0);
        load_arrow_mesh(&arrow_vao, arrows, number_arrows, &arrows_point_count);


		m_mat4_rotation_axis(model_rotation_matrix, &Y_AXIS, 0.00001 * frame);		
		m_mat4_mul(model_matrix, model_scale_matrix, model_rotation_matrix);

    	if(1){
			glUseProgram(model_program);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, model_texture);
			glUniform1i(loc_texture_0, 0);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, pbr_albedomap_texture);
			glUniform1i(loc_pbr_albedomap, 1);

			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, pbr_normalmap_texture);
			glUniform1i(loc_pbr_normalmap, 2);

			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, pbr_metallicmap_texture);
			glUniform1i(loc_pbr_metallicmap, 3);

			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, pbr_roughnessmap_texture);
			glUniform1i(loc_pbr_roughnessmap, 4);

			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, pbr_aomap_texture);
			glUniform1i(loc_pbr_aomap, 5);


            float time =  frame/500.0f;
            glUniform1f(loc_time, time);

            sprintf(debug_string, "-> %f %f %f - light %f %f %f",camera->position.x,camera->position.y,camera->position.z, light_dir.x,light_dir.y,light_dir.z);
            glUniform3f(loc_camera_world,camera->position.x,camera->position.y,camera->position.z );
            glUniform3f(loc_light, light_dir.x, light_dir.y, light_dir.z);

        	glUniformMatrix4fv(loc_model_matrix, 1, GL_FALSE, model_matrix);
			glUniformMatrix4fv(loc_view_matrix, 1, GL_FALSE, view_matrix);
			glUniformMatrix4fv(loc_projecion_matrix, 1, GL_FALSE, projection_matrix);
			glBindVertexArray(model_vao);
			glDrawArrays(GL_TRIANGLES, 0, model_point_count);
    	}

        if (1) {
            glUseProgram(arrow_program);
            
            glUniformMatrix4fv(loc_arrow_model_matrix, 1, GL_FALSE, model_matrix);
            glUniformMatrix4fv(loc_arrow_view_matrix, 1, GL_FALSE, view_matrix);
            glUniformMatrix4fv(loc_arrow_projecion_matrix, 1, GL_FALSE, projection_matrix);
            
            glBindVertexArray(arrow_vao);
            glDrawArrays(GL_TRIANGLES, 0, arrows_point_count);
        }

		if (1){
	    	float offset[2] = {15.0, -15.0};
			float font_size = 18.0;
			float width, height;
			//sprintf(debug_string, "-> %.1f %.1f %.1f --> %.1f %.1f %.1f",camera->position.x,camera->position.y,camera->position.z,camera->look_at_point.x,camera->look_at_point.y,camera->look_at_point.z);
            if (filewatcher_context->files_changed > 0) {
				filewatcher_context->files_changed = 0;

				printf("Recompiling model shader\n");
				GLuint new_program = load_model_shaders();
				if (new_program != 0) {
					printf("Replacing model shaders\n");
					model_program = new_program;
				} else {
					printf("\n\n\n\nERROR replacing shaders\n Keeping the old one for now\n\n\n\n");
				}
			}
			mv_ef_string_dimensions(debug_string, &width, &height, font_size); // for potential alignment
			mv_ef_draw(debug_string, NULL, offset, font_size);
    	}

		glfwSwapBuffers(window);
        glfwPollEvents();
        ++frame;
	}

	free(debug_string);
}

void windowclose_callback(GLFWwindow * window) {}

void windowsize_callback(GLFWwindow * window, int width, int height) {}

void cursorposition_callback(GLFWwindow * window, double mx, double my) {}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	BOOL pressed = (action == GLFW_PRESS);
	BOOL released = (action == GLFW_RELEASE);

	float increment = 0.5;
    float light_speed = 0.3;
    switch(key) {
        case GLFW_KEY_RIGHT:
            rads_light_x += light_speed;
        break;
        case GLFW_KEY_LEFT:
            rads_light_x -= light_speed;
        break;
        case GLFW_KEY_UP:
            distance_light += light_speed;
        break;
        case GLFW_KEY_DOWN:
            distance_light -= light_speed;
        break;
        case GLFW_KEY_A:
        	rads_camera_x -= increment * 0.1;
        	log("- x_axis\n");
        break;
        case GLFW_KEY_D:
        	rads_camera_x += increment * 0.1;
        	log("+ x_axis\n");
        break;
        case GLFW_KEY_Q:
        	distance_camera -= increment;
        	log("- y_axis\n");
        break;
        case GLFW_KEY_E:
        	distance_camera += increment;
        	log("+ y_axis\n");
        break;
        default:
        	log("key %d not mapped directly\n", key);
    }

    light_dir.x = distance_light * cos(rads_light_x);
    light_dir.z = distance_light * sin(rads_light_x);

	if(key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void mousebutton_callback(GLFWwindow * window, int button, int action, int mods) {}

void error_callback(int error, const char* description) {
	log("Error: %d -> %s\n", error, description );
}

int main(int argc, char const *argv[]) {
	int w = 1000;
	int h = 800;

	init(w, h);

	gameplay_loop(w, h);

	teardown();

	return 0;
}

void frame_timer() {
    static double t1 = 0.0;
    static double avg_dt = 0.0;
    static double avg_dt2 = 0.0;
    static int avg_counter = 0;
    static int num_samples = 60;

    double t2 = glfwGetTime();
    double dt = t2-t1;
    t1 = t2;

    avg_dt += dt;
    avg_dt2 += dt*dt;
    avg_counter++;

    if (avg_counter == num_samples) {
        avg_dt  /= num_samples;
        avg_dt2 /= num_samples;
        double std_dt = sqrt(avg_dt2 - avg_dt*avg_dt);
        double ste_dt = std_dt / sqrt(num_samples);

        char window_title_string[128];
        sprintf(window_title_string, "dt: avg = %.3fms, std = %.3fms, ste = %.4fms. fps = %.1f", 1000.0*avg_dt, 1000.0*std_dt, 1000.0*ste_dt, 1.0/avg_dt);
        glfwSetWindowTitle(window, window_title_string);

        num_samples = 1.0/avg_dt;
        
        avg_dt = 0.0;
        avg_dt2 = 0.0;
        avg_counter = 0;
    }
}
