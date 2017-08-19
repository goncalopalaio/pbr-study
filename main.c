#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glad/glad.h>
#include <glad/glad.c>
//#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>


//#include "stb_truetype.h" 
//#include "mv_easy_font.h"

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

void windowclose_callback(WIN * window);
void windowsize_callback(WIN * window, int width, int height);
void cursorposition_callback(WIN * window, double mx, double my);
void key_callback(WIN* window, int key, int scancode, int action, int mods);
void mousebutton_callback(WIN * window, int button, int action, int mods);
void error_callback(int error, const char* description);

WIN* window;

int init_opengl(int w, int h) {
	// Set error callback early
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
	glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, w, h);
    while(!should_exit_gameplay_loop()) {
    	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);


		glfwSwapBuffers(window);
        glfwPollEvents();
	}
}

void windowclose_callback(GLFWwindow * window) {}

void windowsize_callback(GLFWwindow * window, int width, int height) {}

void cursorposition_callback(GLFWwindow * window, double mx, double my) {}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	BOOL pressed = (action == GLFW_PRESS);
    switch(key) {
        case GLFW_KEY_RIGHT:
        	log("GLFW_KEY_RIGHT\n");
        break;
        case GLFW_KEY_LEFT:
        	log("GLFW_KEY_LEFT\n");
        break;
        case GLFW_KEY_UP:
        	log("GLFW_KEY_UP\n");
        break;
        case GLFW_KEY_DOWN:
        	log("GLFW_KEY_DOWN\n");
        break;
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
	int w = 100;
	int h = 100;
	init_opengl(w, h);

	gameplay_loop(w, h);

	teardown();

	return 0;
}

