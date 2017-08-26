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

float counter_x_axis = 0;
float counter_y_axis = 0;
float counter_z_axis = 0;
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

void load_texture(const char* file, GLuint* tex) {
	unsigned char * data;
    int width = 0;
    int height = 0;
    int bitdepth = 0;
    GLuint texture;
    data = stbi_load(file, &width, &height, &bitdepth, 0);
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
	float3 offset;

    float rads = counter_x_axis * 8 + (frame/50.0f);
	set_float3(&offset, 1.9*cos(rads), 1 + counter_y_axis * 100, 1.9*sin(rads));
	

    set_float3(&(cam->position), offset.x, offset.y, offset.z);
    set_float3(&(cam->look_at_point), 0, 0,  0);

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

GLuint load_model_shaders() {
	char* str_vert = gp_read_entire_file_alloc("shaders/model_vertex.glsl");
	char* str_frag = gp_read_entire_file_alloc("shaders/model_fragment_pbr_1.glsl");
    GLuint program = compile_shader_program(str_vert,str_frag,
    													  "position", "normal", "uv", "tangents");
    free(str_vert);
    free(str_frag);

    return program;
}

void gameplay_loop(int w, int h) {
	char* debug_string = (char*) malloc(200 * sizeof(char));

	GLuint model_vao;
    int model_point_count = 0;
    assert(load_mesh("models/round.obj", &model_vao, &model_point_count));

    GLuint model_texture;
    load_texture("textures/texture_3.png", &model_texture);

	GLuint pbr_albedomap_texture;
	GLuint pbr_normalmap_texture;
	GLuint pbr_metallicmap_texture;
	GLuint pbr_roughnessmap_texture;
	GLuint pbr_aomap_texture;

	log("Loading PBR assets\n");
	load_texture("textures/pbr/wall/albedo.png", &pbr_albedomap_texture);
	load_texture("textures/pbr/wall/normal.png", &pbr_normalmap_texture);
	load_texture("textures/pbr/wall/metallic.png", &pbr_metallicmap_texture);
	load_texture("textures/pbr/wall/roughness.png", &pbr_roughnessmap_texture);
	load_texture("textures/pbr/wall/ao.png", &pbr_aomap_texture);

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

    float camera_world[3];

	float model_matrix[] = M_MAT4_IDENTITY();
	float model_scale_matrix[] = M_MAT4_IDENTITY();
	float model_rotation_matrix[] = M_MAT4_IDENTITY();
	float3 model_scale;
	set_float3(&model_scale,0.8,0.8,0.8);
	m_mat4_scale(model_scale_matrix, &model_scale);

	GLuint model_program = load_model_shaders();

    GLuint loc_time = glGetUniformLocation(model_program, "u_time");
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
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glViewport(0, 0, w, h);
	
    while(!should_exit_gameplay_loop()) {
    	frame_timer();
    	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    	update_camera(camera, view_matrix);

		m_mat4_rotation_axis(model_rotation_matrix, &Y_AXIS, 0.00001 * frame);		
		m_mat4_mul(model_matrix, model_scale_matrix, model_rotation_matrix);

    	{
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
            copy_to_arr(camera_world, &camera->position);
            glUniformMatrix3fv(loc_camera_world, 1, GL_FALSE, camera_world);

        	glUniformMatrix4fv(loc_model_matrix, 1, GL_FALSE, model_matrix);
			glUniformMatrix4fv(loc_view_matrix, 1, GL_FALSE, view_matrix);
			glUniformMatrix4fv(loc_projecion_matrix, 1, GL_FALSE, projection_matrix);
			glBindVertexArray(model_vao);
			glDrawArrays(GL_TRIANGLES, 0, model_point_count);
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

	float increment = 0.005;

    switch(key) {
        case GLFW_KEY_RIGHT:
        break;
        case GLFW_KEY_LEFT:
        break;
        case GLFW_KEY_UP:
        break;
        case GLFW_KEY_DOWN:
        break;
        case GLFW_KEY_A:
        	counter_x_axis -= increment;
        	log("- x_axis\n");
        break;
        case GLFW_KEY_D:
        	counter_x_axis += increment;
        	log("+ x_axis\n");
        break;
        case GLFW_KEY_W:
        	counter_z_axis -= increment;
        	log("- z_axis\n");
        break;
        case GLFW_KEY_S:
        	counter_z_axis += increment;
        	log("+ z_axis\n");
        break;
        case GLFW_KEY_Q:
        	counter_y_axis -= increment;
        	log("- y_axis\n");
        break;
        case GLFW_KEY_E:
        	counter_y_axis += increment;
        	log("+ y_axis\n");
        break;
        default:
        	log("key %d not mapped directly\n", key);
    }

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
