#define M_MATH_IMPLEMENTATION
#include "m_math.h"

#include "../../assimp/cimport.h"
#include "../../assimp/postprocess.h"
#include "../../assimp/scene.h"
#include "../../assimp/version.h"

//
//
// Version 0.2 (19/08/2017)
//
//


void gp_log(char *s);
void gp_log_float3(char *s, float3 f);
void gp_log_matrix(char*name, float matrix[]);


////////////////////////////////////////////////////
// float3 extensions

float3 ORIGIN = {0,0,0};
float3 X_AXIS = {1,0,0};
float3 Y_AXIS = {0,1,0};
float3 Z_AXIS = {0,0,1};

float3 make_float3(float x, float y, float z) {
	float3 t;
	t.x = x;
	t.y = y;
	t.z = z;
	return t;
}

void set_float3(float3* v, float x, float y, float z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

void set_float3(float* arr, float3 v) {
	arr[0] = v.x;
	arr[1] = v.y;
	arr[2] = v.z;
}

float* malloc_mat4_identity() {
	float* matrix = (float*) malloc(16 * sizeof(float));
	m_mat4_identity(matrix);
	return matrix;
}

void copy_mat4(float* dest, float* orig) {
	memcpy(dest, orig, (16 * sizeof(float)));
}

////////////////////////////////////////////////////
// math related functions

float radians_to_degrees(float radians) {
   return radians * 180/M_PI;
}

float degrees_to_radians(float degrees) {
    return degrees * M_PI / 180;
}

float dist_euclidean(float3 a, float3 b) {
	float3 sub;
	M_SUB3(sub, a,b);

	return sqrtf(sub.x*sub.x  + sub.y*sub.y + sub.z*sub.z);
}

////////////////////////////////////////////////////
// random related functions

float rand_float_range(float min, float max) {
	return min + (max-min) * m_randf();
}

float3 rand_in_unit_sphere() {
    float3 res;
    float len = 0;
    do {
        res.x = 2.0 * m_randf() - 1;
        res.y = 2.0 * m_randf() - 1;
        res.z = 2.0 * m_randf() - 1;
        //squared length
        len = res.x*res.x + res.y*res.y + res.z*res.z;
    } while(len >= 1.0);

    return res;
}

////////////////////////////////////////////////////
// model loading


int load_mesh(const char* file_name, GLuint* vao, int* point_count) {

	const aiScene* scene = aiImportFile(file_name, 
aiProcess_Triangulate|
aiProcess_ConvertToLeftHanded|
aiProcess_OptimizeMeshes);
	
	if(!scene) {
		printf("Error reading mesh %s \n", file_name);
		return 0;
	}
	printf("%s\n", file_name);
	printf("%d animations \n", scene->mNumAnimations);
	printf("%d cameras \n", scene->mNumCameras);
	printf("%d lights \n", scene->mNumLights);
	printf("%d materials \n", scene->mNumMaterials);
	printf("%d meshes \n", scene->mNumMeshes);
	printf("%d textures \n", scene->mNumTextures);
		
	assert(scene->mNumMeshes > 0);

	// Getting first mesh in file
	const aiMesh* mesh = scene->mMeshes[0];
	printf("Mesh[0] has %d vertices\n",mesh->mNumVertices);

	// Keep point_count
	*point_count = mesh->mNumVertices;

	// Generate a VAO
	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);

	// Copy assimp data

	GLfloat* points = NULL;
	GLfloat* normals = NULL;
	GLfloat* tex_coords = NULL;

	if (mesh->HasPositions()){
		printf("Loading positions\n");
		points = (GLfloat*) malloc(*point_count * 3 * sizeof(GLfloat));
		for (int i = 0; i < *point_count; ++i) {
			const aiVector3D* vp = &(mesh->mVertices[i]);
			points[i * 3 + 0] = (GLfloat)vp->x;
			points[i * 3 + 1] = (GLfloat)vp->y;
			points[i * 3 + 2] = (GLfloat)vp->z;
				
		}
	}

	if (mesh->HasNormals()){
		printf("Loading normals\n");
		normals = (GLfloat*) malloc(*point_count * 3 * sizeof(GLfloat));
		for (int i = 0; i < *point_count; ++i) {
			const aiVector3D* vn = &(mesh->mNormals[i]);
			normals[i * 3 + 0] = (GLfloat)vn->x;
			normals[i * 3 + 1] = (GLfloat)vn->y;
			normals[i * 3 + 2] = (GLfloat)vn->z;
		}
	}

	if (mesh->HasTextureCoords(0)){
		printf("Loading textureCoords(0)\n");
		tex_coords = (GLfloat*) malloc(*point_count * 2 * sizeof(GLfloat));
		for (int i = 0; i < *point_count; ++i) {
			const aiVector3D* vt = &(mesh->mTextureCoords[0][i]);
			tex_coords[i * 2 + 0] = (GLfloat)vt->x;
			tex_coords[i * 2 + 1] = (GLfloat)vt->y;
		}
	}

	// Copy mesh data to VBO

	if (mesh->HasPositions()){
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(
			GL_ARRAY_BUFFER,
			3* (*point_count) * sizeof(GLfloat),
			points,
			GL_STATIC_DRAW);

		glVertexAttribPointer(0,3,GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);

		printf("VertexAttribArray 0 -> Positions\n");

		// Free temporary local memory;
		free(points);
	}

	if (mesh->HasNormals()){
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(
			GL_ARRAY_BUFFER,
			3 * (*point_count) * sizeof(GLfloat),
		normals,
		GL_STATIC_DRAW);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(1);

		printf("VertexAttribArray 1 -> Normals\n");

		// Free temporary local memory;
		free(normals);
	}

	if (mesh->HasTextureCoords(0)){
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(
			GL_ARRAY_BUFFER,
			2 * (*point_count) * sizeof(GLfloat),
			tex_coords,
			GL_STATIC_DRAW);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(2);

		printf("VertexAttribArray 2 -> TextureCoords\n");

		// Free temporary local memory;
		free(tex_coords);
	}

	if (mesh->HasTangentsAndBitangents()){
		// @missing
	}

	glBindVertexArray(0);

	aiReleaseImport(scene);
	printf("Mesh loaded\n");
	return 1;

}


////////////////////////////////////////////////////
// shader stuff

GLuint compile_shader(GLenum type, const char *src) {
    GLuint shader;
    GLint compiled;
    shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled) {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        GLchar strInfoLog[infoLogLength];
        glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);
        printf("Compilation error in shader %s\n", strInfoLog);
        glDeleteShader(shader);
        return 0;
    }
    printf("Success!\n");
    return shader;
}

int compile_shader_program(const char* str_vert_shader, const char* str_frag_shader, const char* attrib_name_0, const char* attrib_name_1, const char* attrib_name_2) {
    GLuint vert_shader;
    GLuint frag_shader;
    GLuint prog_object;

    vert_shader = compile_shader(GL_VERTEX_SHADER, str_vert_shader);
    if(vert_shader == 0) {
        gp_log("Error compiling vert shader");
        return 0;
    }

    frag_shader = compile_shader(GL_FRAGMENT_SHADER, str_frag_shader);
    if(frag_shader == 0) {
        gp_log("Error compiling frag shader");
        return 0;
    }

    gp_log("Creating shader program");

    prog_object = glCreateProgram();
    glAttachShader(prog_object, vert_shader);
    glAttachShader(prog_object, frag_shader);

    if (attrib_name_0 != NULL) {
        printf("Binding attrib 0 ---> %s\n", attrib_name_0);
        glBindAttribLocation(prog_object, 0, attrib_name_0);
    }

    if (attrib_name_1 != NULL) {
        printf("Binding attrib 1 ---> %s\n", attrib_name_1);
        glBindAttribLocation(prog_object, 1, attrib_name_1);
    }

    if (attrib_name_2 != NULL) {
        printf("Binding attrib 2 ---> %s\n", attrib_name_2);
        glBindAttribLocation(prog_object, 2, attrib_name_2);
    }

    gp_log("Linking shader program");
    glLinkProgram(prog_object);

    return prog_object;
}

////////////////////////////////////////////////////
// file stuff

char* gp_read_entire_file_alloc(const char *filename) {
    printf("Loading %s\n", filename);
    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("Error: Could not load file: %s\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    printf("Filesize = %d\n", (int)fsize);

    char *string = (char*)malloc(fsize + 1);
    fread(string, fsize, 1, f);
    string[fsize] = '\0';
    fclose(f);

    return string;
}

////////////////////////////////////////////////////
// profiling and gp_logging


void gp_log(char *s) {
    printf("%s\n", s);
}
void gp_log_float3(char *s, float3 f) {
    printf("%s %f %f %f\n", s,f.x,f.y,f.z);
}
void gp_log_matrix(char*name, float matrix[]) {
    int index = 0;
    printf("%s:\n", name);
    printf("%f %f %f %f\n", matrix[index+0], matrix[index+1],matrix[index+2],matrix[index+3]);
    index += 4;
    printf("%f %f %f %f\n", matrix[index+0], matrix[index+1],matrix[index+2],matrix[index+3]);
    index += 4;
    printf("%f %f %f %f\n", matrix[index+0], matrix[index+1],matrix[index+2],matrix[index+3]);
    index += 4;
    printf("%f %f %f %f\n", matrix[index+0], matrix[index+1],matrix[index+2],matrix[index+3]);
}

#ifdef __MACH__
	#include <sys/time.h>
	#define CLOCK_MONOTONIC 0
	int clock_gettime(int clock_id, struct timespec* t);

	int clock_gettime(int clock_id, struct timespec* t){
		//this is a straigthforward implementation of clock_gettime since is not implemented on OSX
		//original function by Dmitri Bouianov - http://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x
		//might me ineficient of inacurate when compared to the original implementation - gettimeofday is deprecated
		struct timeval now;
		int rv = gettimeofday(&now, NULL);
		if(rv) return rv;//gettimeofday returns 0 for success
		t->tv_sec = now.tv_sec;
		t->tv_nsec = now.tv_usec * 1000;
		return 0;
	}	
#endif

typedef struct g_timer{
	struct timespec start_time;
	struct timespec end_time;
}g_timer;

void start_timer(g_timer* counter) {
	clock_gettime(CLOCK_MONOTONIC, &counter->start_time);
}

void stop_timer(g_timer* counter) {
	clock_gettime(CLOCK_MONOTONIC, &counter->end_time);
}

float compute_timer_millis_diff(g_timer* counter) {
	const float p10_to_minus6 = pow(10,-6);
	long diff_sec  =counter->end_time.tv_sec - counter->start_time.tv_sec;
	long diff_nano =counter->end_time.tv_nsec - counter->start_time.tv_nsec;
	long diff_millis = diff_sec * 1000 + diff_nano * p10_to_minus6;
	return diff_millis;
}

void stop_print_timer(const char* message, g_timer* counter) {
	stop_timer(counter);
	float diff_millis= compute_timer_millis_diff(counter);
	printf("%s millis %f \n",message, diff_millis);
}

int compute_timer_fps(g_timer* counter) {
	int end_seconds = counter->end_time.tv_sec;
	int start_seconds = counter->start_time.tv_sec;
	int end_nanosec = counter->end_time.tv_nsec;
	int start_nanosec = counter->start_time.tv_nsec;

	int diff = 1000000000L * (end_seconds - start_seconds) + end_nanosec - start_nanosec;
	int fps =  1/(diff / 1000000000.0);
	return fps;
}


// CPU cycle profiling
// Returns the processor time stamp. The processor time stamp records the number of clock cycles since the last reset
//#include <stdint.h>
#ifdef _WIN32
#include <intrin.h>
uint64_t rdtsc(){
    return __rdtsc();
}
#else
// Linux/GCC
uint64_t rdtsc(){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}
#endif

struct debug_record {
	char tag[17];
	char* file_name;
	int line_number;
	uint64_t clocks;
};

struct cpu_timestamp {
	struct debug_record record;

	cpu_timestamp(int line_number, char* file_name, const char tag[17]) {
		// __FUNCTION__ is defined by the compiler not the preprocessor. Copying the string is the workaround i found.
		strcpy(record.tag, tag);
		record.file_name = file_name;
		record.line_number = line_number;
		
		record.clocks = - rdtsc();
	}

	~cpu_timestamp() {
		record.clocks += rdtsc();
		printf("ln: %d %s %s clocks: %lld \n", record.line_number, record.file_name, record.tag, record.clocks);
	}
};

// abuse c++ destructors so it is only required to define this at the start of the method
#define TIMED_BLOCK struct cpu_timestamp temp_cpu_timestamp(__LINE__, __FILE__, __FUNCTION__)


////////////////////////////////////////////////////
// file watching stuff

// @note this is really messy for now and only supports osx

#ifdef GP_INCLUDE_FILEWATCHER
	
	typedef struct FileWatcherInfo{
		int files_changed;
		char* latest_filename;
	}FileWatcherInfo;

	FileWatcherInfo* filewatcher_context;

	void start_filewatcher(char* folder);
	void stop_filewatcher();
	int files_changed_filewatcher();

	#ifdef __MACH__
		#include <CoreServices/CoreServices.h>
		FSEventStreamRef stream;

		void loop_filewatcher(
		    ConstFSEventStreamRef streamRef,
		    void *clientCallBackInfo,
		    size_t numEvents,
		    void *eventPaths,
		    const FSEventStreamEventFlags eventFlags[],
		    const FSEventStreamEventId eventIds[])
		{
			int i;
			char **paths = (char **)eventPaths;

			// printf("Callback called\n");
			for (i=0; i<numEvents; i++) {
				int count;
				/* flags are unsigned long, IDs are uint64_t */
				printf("Change %llu in %s, flags %u\n", eventIds[i], paths[i], eventFlags[i]);
			}

			if (numEvents > 0) {
				if (filewatcher_context != NULL) {
					filewatcher_context->files_changed = numEvents;
					sprintf(filewatcher_context->latest_filename, "%s", paths[0]);
				} else {
					printf("filewatcher_context is NULL ???\n");
				}	
			} else {
				filewatcher_context->files_changed = 0;
				printf("loop_filewatcher called but no events ???\n");
			}
		}

		void stop_filewatcher() {
			printf("Stop_filewatcher: stop\n");
			FSEventStreamStop(stream);
			FSEventStreamInvalidate(stream);
			FSEventStreamRelease(stream);

			free(filewatcher_context->latest_filename);
			free(filewatcher_context);
		}

		/*
		 * https://developer.apple.com/library/content/documentation/Darwin/Conceptual/FSEvents_ProgGuide/UsingtheFSEventsFramework/UsingtheFSEventsFramework.html#//apple_ref/doc/uid/TP40005289-CH4-DontLinkElementID_11
		 */
		void start_filewatcher(char* folder) {
			printf("Start_filewatcher: start\n");
			filewatcher_context = (FileWatcherInfo*) malloc(sizeof(FileWatcherInfo));
 			filewatcher_context->files_changed = 0;
 			filewatcher_context->latest_filename = (char*) malloc(200 * sizeof(char));


 			
 			/* Define variables and create a CFArray object containing
		       CFString objects containing paths to watch.
		     */
		    CFStringRef mypath = CFStringCreateWithFormat(NULL, NULL, CFSTR("%s"), folder);
		    // @note only supporting one folder for now
		    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&mypath, 1, NULL);

		    CFAbsoluteTime latency = 0.3; /* Latency in seconds */
		 	FSEventStreamContext* stream_context = NULL;
		    /* Create the stream, passing in a callback */
		    stream = FSEventStreamCreate(NULL,
		        &loop_filewatcher,
		        stream_context, // @note There's a way to pass information through this?
		        pathsToWatch,
		        kFSEventStreamEventIdSinceNow, /* Or a previous event ID */
		        latency,
		        kFSEventStreamCreateFlagFileEvents//kFSEventStreamCreateFlagNone /* Flags explained in reference */
		    );

		    /* Create the stream before calling this. */
		    FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

		    // Start stream
		    bool success = FSEventStreamStart(stream);
		    printf("Start_filewatcher: file event stream started? %d\n", success);
		}

	#else
		void start_filewatcher() {
			printf("\n\n\n\n File watcher not implemented for this platform\n\n\n\n ");
		}
		void stop_filewatcher() {
			printf("\n\n\n\n File watcher not implemented for this platform\n\n\n\n ");
		}
		int files_changed_filewatcher() {
			printf("\n\n\n\n File watcher not implemented for this platform\n\n\n\n ");	
		}
	#endif

#endif



