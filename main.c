#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <glad/glad.h>
#include <glad/glad.c>
//#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include "include/gp_lib.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

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

int k_left_shift_switch = 0;
int k_left_counter = 0;
int k_down_counter = 0;
int k_right_counter = 0;
int k_a_counter = 0;
int k_s_counter = 0;
int k_d_counter = 0;

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

void update_camera(Camera* cam, float* view_matrix) {
    set_float3(&(cam->position), k_left_counter+15 ,k_down_counter+15, k_right_counter+15);
    set_float3(&(cam->look_at_point), k_a_counter + 0, k_s_counter + 10, k_d_counter + 0);

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
    return 0;
}

void teardown() {
	log("Exiting program\n");
	glfwTerminate();
}

BOOL should_exit_gameplay_loop() {
	return glfwWindowShouldClose(window);
}

void gameplay_loop(int w, int h) {
	char* debug_string = (char*) malloc(200 * sizeof(char));

	GLuint model_vao;
    int model_point_count = 0;
    assert(load_mesh("models/cat.obj", &model_vao, &model_point_count));


	float view_matrix[] = M_MAT4_IDENTITY();
	float projection_matrix[] = M_MAT4_IDENTITY();
	
    float aspect = w / (float)h;
    m_mat4_perspective(projection_matrix, 10.0, aspect, 0.1, 100.0);
    m_mat4_identity(view_matrix);

    Camera* camera = (Camera*) malloc(sizeof(Camera));
   	update_camera(camera, view_matrix);

    char str_vert_shader[] =
		"#version 150\n"
		"in vec3 position;\n"
		"uniform mat4 u_view_matrix;"
		"uniform mat4 u_model_matrix;"
        "uniform mat4 u_projection_matrix;"
		"void main(){\n"
		"gl_Position =  u_projection_matrix * u_view_matrix * u_model_matrix * vec4(position, 1.0);\n"
		"}\n";

    char str_frag_shader[] =
		"#version 150\n"
		"out vec4 frag_color;\n"
		"void main() {\n"
		"frag_color = vec4(0.9,0.5,0.3,1.0);\n"
		"}\n";

	float model_matrix[] = M_MAT4_IDENTITY();
	float model_scale_matrix[] = M_MAT4_IDENTITY();
	float model_rotation_matrix[] = M_MAT4_IDENTITY();
	float3 model_scale;
	set_float3(&model_scale, 0.03,0.03,0.03);
	m_mat4_scale(model_scale_matrix, &model_scale);



    GLuint model_program = compile_shader_program(str_vert_shader,
    													  str_frag_shader,
    													  "position", "uv", "normal");

	GLuint loc_model_matrix = glGetUniformLocation(model_program, "u_model_matrix");
    GLuint loc_view_matrix = glGetUniformLocation(model_program, "u_view_matrix");
    GLuint loc_projecion_matrix = glGetUniformLocation(model_program, "u_projection_matrix");

	glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, w, h);
	int frame = 0;
    while(!should_exit_gameplay_loop()) {
    	frame_timer();
    	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    	update_camera(camera, view_matrix);

		m_mat4_rotation_axis(model_rotation_matrix, &Y_AXIS, 0.01 * frame);		
		m_mat4_mul(model_matrix, model_scale_matrix, model_rotation_matrix);

    	{
			glUseProgram(model_program);
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
			sprintf(debug_string, "Look at the cat\n%.1f %.1f %.1f --> %.1f %.1f %.1f",
																 camera->position.x,
															     camera->position.y,
															     camera->position.z,
																 camera->look_at_point.x,
																 camera->look_at_point.y,
																 camera->look_at_point.z);
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

	int increment = 10;

    switch(key) {
        case GLFW_KEY_RIGHT:
        	log("GLFW_KEY_RIGHT\n");
        	k_right_counter+=increment;
        break;
        case GLFW_KEY_LEFT:
        	log("GLFW_KEY_LEFT\n");
        	k_left_counter+=increment;
        break;
        case GLFW_KEY_UP:
        	log("GLFW_KEY_UP\n");
        break;
        case GLFW_KEY_DOWN:
        	log("GLFW_KEY_DOWN\n");
        	k_down_counter+=increment;
        break;
        case GLFW_KEY_A:
        	log("GLFW_KEY_A\n");
        	k_a_counter+=increment;
        break;
        case GLFW_KEY_S:
        	log("GLFW_KEY_S\n");
        	k_s_counter+=increment;
        break;
        case GLFW_KEY_D:
        	log("GLFW_KEY_DOWN\n");
        	k_d_counter+=increment;
        break;
        default:
        	log("key %d not mapped", key);
    }

	if(key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void mousebutton_callback(GLFWwindow * window, int button, int action, int mods) {}

void error_callback(int error, const char* description) {
	log("Error: %d -> %s\n", error, description );
}

int main(int argc, char const *argv[]) {
	int w = 600;
	int h = 400;

	init(w, h);

	gameplay_loop(w, h);

	teardown();

	return 0;
}


void frame_timer()
{
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
