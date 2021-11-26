#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include<random>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <FreeImage/FreeImage.h>

#include "Shaders/LoadShaders.h"
#include "My_Shading.h"
GLuint h_ShaderProgram_simple, h_ShaderProgram_TXPS, h_ShaderProgram_GS; // handles to shader programs

// for simple shaders
GLint loc_ModelViewProjectionMatrix_simple, loc_primitive_color;

// for Phong Shading (Textured) shaders
#define NUMBER_OF_LIGHT_SUPPORTED 7 
GLint loc_global_ambient_color,gloc_global_ambient_color, ploc_global_ambient_color;
loc_light_Parameters loc_light[NUMBER_OF_LIGHT_SUPPORTED],
gloc_light[NUMBER_OF_LIGHT_SUPPORTED],ploc_light[NUMBER_OF_LIGHT_SUPPORTED];

loc_Material_Parameters loc_material, ploc_material, gloc_material;
GLint loc_ModelViewProjectionMatrix_TXPS, loc_ModelViewMatrix_TXPS, loc_ModelViewMatrixInvTrans_TXPS;
GLint loc_ModelViewProjectionMatrix_GS, loc_ModelViewMatrix_GS, loc_ModelViewMatrixInvTrans_GS;
GLint loc_texture, loc_flag_texture_mapping, loc_flag_fog;
GLint gloc_texture, gloc_flag_texture_mapping, gloc_flag_fog;
GLint ploc_texture, ploc_flag_texture_mapping, ploc_flag_fog;

#define PHONG 0
#define GOURAUD 1


// include glm/*.hpp only if necessary
//#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, lookAt, perspective, etc.
#include <glm/gtc/matrix_inverse.hpp> // inverseTranspose, etc.
glm::mat4 ModelViewProjectionMatrix, ModelViewMatrix, ViewProjectionMatrix;
glm::mat3 ModelViewMatrixInvTrans;
glm::mat4 ViewMatrix, ProjectionMatrix;

glm::mat4 fixedViewMatrix;

#define TO_RADIAN 0.01745329252f  
#define TO_DEGREE 57.295779513f
#define BUFFER_OFFSET(offset) ((GLvoid *) (offset))

#define LOC_VERTEX 0
#define LOC_NORMAL 1 
#define LOC_TEXCOORD 2

// lights in scene
Light_Parameters light[NUMBER_OF_LIGHT_SUPPORTED];

Light_Parameters OriginLight[NUMBER_OF_LIGHT_SUPPORTED];
Light_Parameters RandomLight[NUMBER_OF_LIGHT_SUPPORTED];

glm::vec3 special_light_destination[3] =
{
glm::vec3(0.0f, 0.0f,
	400.0f),
	glm::vec3(
	350.0f, 0.0f,-200.0f),
	glm::vec3(
-350.0f,0.0f,
		200.0f)
		};

glm::vec3 special_light_point[3][3] =
{ glm::vec3(0.0f,0.0f,-200.0f),glm::vec3(-350.0f,0.0f,200.0f),glm::vec3(350.0f,0.0f,200.0f),
glm::vec3(350.0f,0.0f,-200.0f),glm::vec3(-350.0f, 0.0f,-200.0f),glm::vec3(0.0f,0.0f,400.0f),
glm::vec3(-200.0f,0.0f,200.0f),glm::vec3(200.0f,0.0f, 200.0f),glm::vec3(0.0f,0.0f,-300.0f) };

glm::vec3 origin_special_light_point[3][3] = { special_light_point[0][0],special_light_point[0][1],special_light_point[0][2],
special_light_point[1][0],special_light_point[1][1],special_light_point[1][2],
special_light_point[2][0],special_light_point[2][1],special_light_point[2][2] };

// texture stuffs
#define N_TEXTURES_USED 4
#define TEXTURE_ID_FLOOR 0
#define TEXTURE_ID_TIGER 1
#define TEXTURE_ID_SILVER 2
#define TEXTURE_ID_SPIDER 3
GLuint texture_names[N_TEXTURES_USED];
int flag_texture_mapping;

void switch_shader(int now_shader);
void initialize_lights_and_material(void);
void set_up_shader(void);
void set_up_special_light(void);
void set_up_special_light_shader(void);
int flag_special_light = 0;

glm::vec3 now_spider_position;

void My_glTexImage2D_from_file(char *filename) {
	FREE_IMAGE_FORMAT tx_file_format;
	int tx_bits_per_pixel;
	FIBITMAP *tx_pixmap, *tx_pixmap_32;

	int width, height;
	GLvoid *data;
	
	tx_file_format = FreeImage_GetFileType(filename, 0);
	// assume everything is fine with reading texture from file: no error checking
	tx_pixmap = FreeImage_Load(tx_file_format, filename);
	tx_bits_per_pixel = FreeImage_GetBPP(tx_pixmap);

	fprintf(stdout, " * A %d-bit texture was read from %s.\n", tx_bits_per_pixel, filename);
	if (tx_bits_per_pixel == 32)
		tx_pixmap_32 = tx_pixmap;
	else {
		fprintf(stdout, " * Converting texture from %d bits to 32 bits...\n", tx_bits_per_pixel);
		tx_pixmap_32 = FreeImage_ConvertTo32Bits(tx_pixmap);
	}

	width = FreeImage_GetWidth(tx_pixmap_32);
	height = FreeImage_GetHeight(tx_pixmap_32);
	data = FreeImage_GetBits(tx_pixmap_32);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
	fprintf(stdout, " * Loaded %dx%d RGBA texture into graphics memory.\n\n", width, height);

	FreeImage_Unload(tx_pixmap_32);
	if (tx_bits_per_pixel != 32)
		FreeImage_Unload(tx_pixmap);
}

// fog stuffs
// you could control the fog parameters interactively: FOG_COLOR, FOG_NEAR_DISTANCE, FOG_FAR_DISTANCE   
int flag_fog;

// for tiger animation
#define CAM_TSPEED 0.25f
#define CAM_RSPEED 0.1f
unsigned int tiger_timestamp_scene = 0; // the global clock in the scene
unsigned int ben_timestamp_scene = 0;
unsigned int spider_timestamp_scene = 0;
unsigned int wolf_timestamp_scene = 0;
unsigned int light_timestamp_scene = 0;
int flag_tiger_animation, flag_polygon_fill;
int flag_ben_animation;
int flag_spider_animation;
int flag_wolf_animation;
int cur_frame_tiger = 0, cur_frame_ben = 0, cur_frame_wolf, cur_frame_spider = 0;
float go_distance_ben = 0.0f; //own
float rotation_angle_ben = 0.0f; //own
float rotation_angle_spider = 0.0f; //own, 원을 그리면서 생기는 거미의 대가리 방향 회전
float rotation_sin_angle_spider = 0.0f; //own, sin으로 생기는 거미의 대가리 방향 회전
float move_sin_distance_spider = 0.0f; //own, sin으로 생기는 x축 폭 이동 
float jump_sin_wolf = 0.0f; //own, sin으로 만드는 늑대의 jump 거리
float angle_sin_wolf = 0.0f; //own, sin으로 생기는 늑대의 몸체 각도
float go_distance_wolf = 0.0f; //own, 늑대가 일자로 움직임 -x 방향으로
float x_distance_tiger = 0.0f;
float z_distance_tiger = 0.0f;
float rotation_angle_tiger = 0.0f;
int EC_idx = 1;
int Center_idx = 1;
int flag_shader = PHONG;
float fovy = 45.0f;
int cameramove = 0;
int prevx;
int prevy;
float aspect_ratio;
int prev_idx = 1;
enum AXES{X,Y,Z};
enum AXES nowaxe = Z;
glm::vec3 VUP = glm::vec3(0.0f, 1.0f, 0.0f);

glm::vec3 EC[6] = { glm::vec3(1.0f, 1.0f, 1.0f),
				 glm::vec3(400.0f, 500.0f, 800.0f),
				 glm::vec3(-300.0f, 80.0f, -100.0f),
				 glm::vec3(0.0f, 400.0f, -600.0f),
				 glm::vec3(-170.0f, 80.0f, 130.0f),
				 glm::vec3(700.0f, 300.0f, 0.0f)};

glm::vec3 Center[6] = { glm::vec3(0.0f, 0.0f, 0.0f),
				 glm::vec3(0.0f, 0.0f, 0.0f),
				 glm::vec3(0.0f, 0.0f, 100.0f),
				 glm::vec3(-400.0f, 200.0f, -100.0f),
				 glm::vec3(300.0f, 200.0f, -200.0f),
				 glm::vec3(0.0f, 200.0f, -100.0f) };

glm::vec3 EC5origin = glm::vec3(700.0f, 300.0f, 0.0f);
glm::vec3 Center5origin = glm::vec3(0.0f, 200.0f, -100.0f);
glm::vec3 EC1origin = glm::vec3(400.0f, 500.0f, 800.0f);
glm::vec3 Center1origin = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 VUPorigin = glm::vec3(0.0f, 1.0f, 0.0f);


//Camera modify start
typedef struct _Camera {
	float pos[3];
	float uaxis[3], vaxis[3], naxis[3];
	float fovy, aspect_ratio, near_c, far_c;
	int move;
} Camera;

Camera camera[6];

void set_ViewMatrix_from_camera_frame(void) {
	ViewMatrix = glm::mat4(camera[EC_idx].uaxis[0], camera[EC_idx].vaxis[0], camera[EC_idx].naxis[0], 0.0f,
		camera[EC_idx].uaxis[1], camera[EC_idx].vaxis[1], camera[EC_idx].naxis[1], 0.0f,
		camera[EC_idx].uaxis[2], camera[EC_idx].vaxis[2], camera[EC_idx].naxis[2], 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	ViewMatrix = glm::translate(ViewMatrix, glm::vec3(-camera[EC_idx].pos[0], -camera[EC_idx].pos[1], -camera[EC_idx].pos[2]));
}

void initialize_camera(void) {
	for (int i = 0; i < 6; ++i)
	{
		camera[i].pos[0] = EC[i].x;
		camera[i].pos[1] = EC[i].y;
		camera[i].pos[2] = EC[i].z;
		glm::vec3 n = normalize(Center[i] - EC[i]);
		glm::vec3 u = normalize(cross(n, VUP));
		glm::vec3 v = cross(u, n);
		camera[i].naxis[0] = -n.x;
		camera[i].naxis[1] = -n.y;
		camera[i].naxis[2] = -n.z;
		camera[i].uaxis[0] = u.x;
		camera[i].uaxis[1] = u.y;
		camera[i].uaxis[2] = u.z;
		camera[i].vaxis[0] = v.x;
		camera[i].vaxis[1] = v.y;
		camera[i].vaxis[2] = v.z;
	}
	fovy = 45.0f;
	ProjectionMatrix = glm::perspective(fovy * TO_RADIAN, aspect_ratio, 100.0f, 20000.0f);
}

void renew_cam_position(int del) {
	switch (nowaxe) {
	case X:
		camera[EC_idx].pos[0] += CAM_TSPEED * del * (camera[EC_idx].uaxis[0]);
		camera[EC_idx].pos[1] += CAM_TSPEED * del * (camera[EC_idx].uaxis[1]);
		camera[EC_idx].pos[2] += CAM_TSPEED * del * (camera[EC_idx].uaxis[2]);
		break;
	case Y:
		camera[EC_idx].pos[0] += CAM_TSPEED * del * (camera[EC_idx].vaxis[0]);
		camera[EC_idx].pos[1] += CAM_TSPEED * del * (camera[EC_idx].vaxis[1]);
		camera[EC_idx].pos[2] += CAM_TSPEED * del * (camera[EC_idx].vaxis[2]);
		break;
	case Z:
		camera[EC_idx].pos[0] += CAM_TSPEED * del * (camera[EC_idx].naxis[0]);
		camera[EC_idx].pos[1] += CAM_TSPEED * del * (camera[EC_idx].naxis[1]);
		camera[EC_idx].pos[2] += CAM_TSPEED * del * (camera[EC_idx].naxis[2]);
		break;
	}
}

void renew_cam_orientation_rotation_around_v_axis(int angle) {
	// let's get a help from glm
	glm::mat3 RotationMatrix;
	glm::vec3 direction;

	RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), CAM_RSPEED * TO_RADIAN * angle,
		glm::vec3(camera[EC_idx].vaxis[0], camera[EC_idx].vaxis[1], camera[EC_idx].vaxis[2])));

	direction = RotationMatrix * glm::vec3(camera[EC_idx].uaxis[0], camera[EC_idx].uaxis[1], camera[EC_idx].uaxis[2]);
	camera[EC_idx].uaxis[0] = direction.x; camera[EC_idx].uaxis[1] = direction.y; camera[EC_idx].uaxis[2] = direction.z;
	direction = RotationMatrix * glm::vec3(camera[EC_idx].naxis[0], camera[EC_idx].naxis[1], camera[EC_idx].naxis[2]);
	camera[EC_idx].naxis[0] = direction.x; camera[EC_idx].naxis[1] = direction.y; camera[EC_idx].naxis[2] = direction.z;
}
//Camera modify end










// axes object
GLuint axes_VBO, axes_VAO;
GLfloat axes_vertices[6][3] = {
	{ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }
};
GLfloat axes_color[3][3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };

void prepare_axes(void) { // draw coordinate axes
	// initialize vertex buffer object
	glGenBuffers(1, &axes_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, axes_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axes_vertices), &axes_vertices[0][0], GL_STATIC_DRAW);

	// initialize vertex array object
	glGenVertexArrays(2, &axes_VAO);
	glBindVertexArray(axes_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, axes_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

 void draw_axes(void) {
	 // assume ShaderProgram_simple is used
	 glBindVertexArray(axes_VAO);
	 glUniform3fv(loc_primitive_color, 1, axes_color[0]);
	 glDrawArrays(GL_LINES, 0, 2);
	 glUniform3fv(loc_primitive_color, 1, axes_color[1]);
	 glDrawArrays(GL_LINES, 2, 2);
	 glUniform3fv(loc_primitive_color, 1, axes_color[2]);
	 glDrawArrays(GL_LINES, 4, 2);
	 glBindVertexArray(0);
 }

 // floor object
#define TEX_COORD_EXTENT 1.0f
 GLuint rectangle_VBO, rectangle_VAO;
 GLfloat rectangle_vertices[6][8] = {  // vertices enumerated counterclockwise
	 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
	 { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, TEX_COORD_EXTENT, 0.0f },
	 { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, TEX_COORD_EXTENT, TEX_COORD_EXTENT },
	 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
	 { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, TEX_COORD_EXTENT, TEX_COORD_EXTENT },
	 { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, TEX_COORD_EXTENT }
 };

 Material_Parameters material_floor;

 void prepare_floor(void) { // Draw coordinate axes.
	 // Initialize vertex buffer object.
	 glGenBuffers(1, &rectangle_VBO);

	 glBindBuffer(GL_ARRAY_BUFFER, rectangle_VBO);
	 glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_vertices), &rectangle_vertices[0][0], GL_STATIC_DRAW);

	 // Initialize vertex array object.
	 glGenVertexArrays(1, &rectangle_VAO);
	 glBindVertexArray(rectangle_VAO);

	 glBindBuffer(GL_ARRAY_BUFFER, rectangle_VBO);
	 glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(0));
	 glEnableVertexAttribArray(0);
	 glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(3 * sizeof(float)));
 	 glEnableVertexAttribArray(1);
	 glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(6 * sizeof(float)));
	 glEnableVertexAttribArray(2);

	 glBindBuffer(GL_ARRAY_BUFFER, 0);
	 glBindVertexArray(0);
	 
	 material_floor.ambient_color[0] = 0.0f;
	 material_floor.ambient_color[1] = 0.05f;
	 material_floor.ambient_color[2] = 0.0f;
	 material_floor.ambient_color[3] = 1.0f;
	

	 material_floor.diffuse_color[0] = 0.2f;
	 material_floor.diffuse_color[1] = 0.2f;
	 material_floor.diffuse_color[2] = 0.2f;
	 material_floor.diffuse_color[3] = 1.0f;
	 /*
	 material_floor.specular_color[0] = 0.24f;
	 material_floor.specular_color[1] = 0.5f;
	 material_floor.specular_color[2] = 0.24f;
	 material_floor.specular_color[3] = 1.0f;
	 */
	 
	 //modify start
	 material_floor.specular_color[0] = 0.2f;
	 material_floor.specular_color[1] = 0.2f;
	 material_floor.specular_color[2] = 0.2f;
	 material_floor.specular_color[3] = 1.0f;
	 //modify end
	 
	 material_floor.specular_exponent = 2.5f;

	 material_floor.emissive_color[0] = 0.0f;
	 material_floor.emissive_color[1] = 0.0f;
	 material_floor.emissive_color[2] = 0.0f;
	 material_floor.emissive_color[3] = 1.0f;

 	 glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);


	 glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_FLOOR);
	 glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_FLOOR]);
 	 My_glTexImage2D_from_file("Data/static_objects/checker_tex.jpg");

	 glGenerateMipmap(GL_TEXTURE_2D);

 	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

//	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

//   float border_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
//	 glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
 }

 void set_material_floor(void) {
	 // assume ShaderProgram_TXPS is used
	 glUniform4fv(loc_material.ambient_color, 1, material_floor.ambient_color);
	 glUniform4fv(loc_material.diffuse_color, 1, material_floor.diffuse_color);
	 glUniform4fv(loc_material.specular_color, 1, material_floor.specular_color);
	 glUniform1f(loc_material.specular_exponent, material_floor.specular_exponent);
	 glUniform4fv(loc_material.emissive_color, 1, material_floor.emissive_color);
 }

 void draw_floor(void) {
	 glFrontFace(GL_CCW);

	 glBindVertexArray(rectangle_VAO);
	 glDrawArrays(GL_TRIANGLES, 0, 6);
	 glBindVertexArray(0);
 }

 // tiger object
#define N_TIGER_FRAMES 12
GLuint tiger_VBO, tiger_VAO;
int tiger_n_triangles[N_TIGER_FRAMES];
int tiger_vertex_offset[N_TIGER_FRAMES];
GLfloat *tiger_vertices[N_TIGER_FRAMES];

Material_Parameters material_tiger;

// ben object
#define N_BEN_FRAMES 30
GLuint ben_VBO, ben_VAO;
int ben_n_triangles[N_BEN_FRAMES];
int ben_vertex_offset[N_BEN_FRAMES];
GLfloat *ben_vertices[N_BEN_FRAMES];

Material_Parameters material_ben;

// wolf object
#define N_WOLF_FRAMES 17
GLuint wolf_VBO, wolf_VAO;
int wolf_n_triangles[N_WOLF_FRAMES];
int wolf_vertex_offset[N_WOLF_FRAMES];
GLfloat *wolf_vertices[N_WOLF_FRAMES];

Material_Parameters material_wolf;

// spider object
#define N_SPIDER_FRAMES 16
GLuint spider_VBO, spider_VAO;
int spider_n_triangles[N_SPIDER_FRAMES];
int spider_vertex_offset[N_SPIDER_FRAMES];
GLfloat *spider_vertices[N_SPIDER_FRAMES];

Material_Parameters material_spider;

// dragon object
GLuint dragon_VBO, dragon_VAO;
int dragon_n_triangles;
GLfloat *dragon_vertices;

Material_Parameters material_dragon;

// optimus object
GLuint optimus_VBO, optimus_VAO;
int optimus_n_triangles;
GLfloat *optimus_vertices;

Material_Parameters material_optimus;

// cow object
GLuint cow_VBO, cow_VAO;
int cow_n_triangles;
GLfloat *cow_vertices;

Material_Parameters material_cow;

// bike object
GLuint bike_VBO, bike_VAO;
int bike_n_triangles;
GLfloat *bike_vertices;

Material_Parameters material_bike;

// bus object
GLuint bus_VBO, bus_VAO;
int bus_n_triangles;
GLfloat *bus_vertices;

Material_Parameters material_bus;

// godzilla object
GLuint godzilla_VBO, godzilla_VAO;
int godzilla_n_triangles;
GLfloat *godzilla_vertices;

Material_Parameters material_godzilla;

// ironman object
GLuint ironman_VBO, ironman_VAO;
int ironman_n_triangles;
GLfloat *ironman_vertices;

Material_Parameters material_ironman;

// tank object
GLuint tank_VBO, tank_VAO;
int tank_n_triangles;
GLfloat *tank_vertices;

Material_Parameters material_tank;



int read_geometry(GLfloat **object, int bytes_per_primitive, char *filename) {
	int n_triangles;
	FILE *fp;

	// fprintf(stdout, "Reading geometry from the geometry file %s...\n", filename);
	fp = fopen(filename, "rb");
	if (fp == NULL){
		fprintf(stderr, "Cannot open the object file %s ...", filename);
		return -1;
	}
	fread(&n_triangles, sizeof(int), 1, fp);

	*object = (float *)malloc(n_triangles*bytes_per_primitive);
	if (*object == NULL){
		fprintf(stderr, "Cannot allocate memory for the geometry file %s ...", filename);
		return -1;
	}

	fread(*object, bytes_per_primitive, n_triangles, fp);
	// fprintf(stdout, "Read %d primitives successfully.\n\n", n_triangles);
	fclose(fp);

	return n_triangles;
}

void prepare_wolf(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, wolf_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_WOLF_FRAMES; i++) {
		sprintf(filename, "Data/dynamic_objects/wolf/wolf_%02d_vnt.geom", i);
		wolf_n_triangles[i] = read_geometry(&wolf_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		wolf_n_total_triangles += wolf_n_triangles[i];

		if (i == 0)
			wolf_vertex_offset[i] = 0;
		else
			wolf_vertex_offset[i] = wolf_vertex_offset[i - 1] + 3 * wolf_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &wolf_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, wolf_VBO);
	glBufferData(GL_ARRAY_BUFFER, wolf_n_total_triangles*n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_WOLF_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, wolf_vertex_offset[i] * n_bytes_per_vertex,
			wolf_n_triangles[i] * n_bytes_per_triangle, wolf_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_WOLF_FRAMES; i++)
		free(wolf_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &wolf_VAO);
	glBindVertexArray(wolf_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, wolf_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_wolf.ambient_color[0] = 0.24725f;
	//material_wolf.ambient_color[1] = 0.1995f;
	//material_wolf.ambient_color[2] = 0.0745f;
	//material_wolf.ambient_color[3] = 1.0f;
	//
	//material_wolf.diffuse_color[0] = 0.75164f;
	//material_wolf.diffuse_color[1] = 0.60648f;
	//material_wolf.diffuse_color[2] = 0.22648f;
	//material_wolf.diffuse_color[3] = 1.0f;
	//
	//material_wolf.specular_color[0] = 0.728281f;
	//material_wolf.specular_color[1] = 0.655802f;
	//material_wolf.specular_color[2] = 0.466065f;
	//material_wolf.specular_color[3] = 1.0f;
	//
	//material_wolf.specular_exponent = 51.2f;
	//
	//material_wolf.emissive_color[0] = 0.1f;
	//material_wolf.emissive_color[1] = 0.1f;
	//material_wolf.emissive_color[2] = 0.0f;
	//material_wolf.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
void prepare_ben(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, ben_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_BEN_FRAMES; i++) {
		sprintf(filename, "Data/dynamic_objects/ben/ben_vn%d%d.geom", i / 10, i % 10);
		ben_n_triangles[i] = read_geometry(&ben_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		ben_n_total_triangles += ben_n_triangles[i];

		if (i == 0)
			ben_vertex_offset[i] = 0;
		else
			ben_vertex_offset[i] = ben_vertex_offset[i - 1] + 3 * ben_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &ben_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, ben_VBO);
	glBufferData(GL_ARRAY_BUFFER, ben_n_total_triangles*n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_BEN_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, ben_vertex_offset[i] * n_bytes_per_vertex,
			ben_n_triangles[i] * n_bytes_per_triangle, ben_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_BEN_FRAMES; i++)
		free(ben_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &ben_VAO);
	glBindVertexArray(ben_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, ben_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_ben.ambient_color[0] = 0.24725f;
	//material_ben.ambient_color[1] = 0.1995f;
	//material_ben.ambient_color[2] = 0.0745f;
	//material_ben.ambient_color[3] = 1.0f;
	//
	//material_ben.diffuse_color[0] = 0.75164f;
	//material_ben.diffuse_color[1] = 0.60648f;
	//material_ben.diffuse_color[2] = 0.22648f;
	//material_ben.diffuse_color[3] = 1.0f;
	//
	//material_ben.specular_color[0] = 0.728281f;
	//material_ben.specular_color[1] = 0.655802f;
	//material_ben.specular_color[2] = 0.466065f;
	//material_ben.specular_color[3] = 1.0f;
	//
	//material_ben.specular_exponent = 51.2f;
	//
	//material_ben.emissive_color[0] = 0.1f;
	//material_ben.emissive_color[1] = 0.1f;
	//material_ben.emissive_color[2] = 0.0f;
	//material_ben.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
void prepare_spider(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, spider_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_SPIDER_FRAMES; i++) {
		sprintf(filename, "Data/dynamic_objects/spider/spider_vnt_%d%d.geom", i / 10, i % 10);
		spider_n_triangles[i] = read_geometry(&spider_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		spider_n_total_triangles += spider_n_triangles[i];

		if (i == 0)
			spider_vertex_offset[i] = 0;
		else
			spider_vertex_offset[i] = spider_vertex_offset[i - 1] + 3 * spider_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &spider_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, spider_VBO);
	glBufferData(GL_ARRAY_BUFFER, spider_n_total_triangles*n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_SPIDER_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, spider_vertex_offset[i] * n_bytes_per_vertex,
			spider_n_triangles[i] * n_bytes_per_triangle, spider_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_SPIDER_FRAMES; i++)
		free(spider_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &spider_VAO);
	glBindVertexArray(spider_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, spider_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_spider.ambient_color[0] = 0.24725f;
	material_spider.ambient_color[1] = 0.1995f;
	material_spider.ambient_color[2] = 0.1745f;
	material_spider.ambient_color[3] = 1.0f;
	
	material_spider.diffuse_color[0] = 0.15164f;
	material_spider.diffuse_color[1] = 0.60648f;
	material_spider.diffuse_color[2] = 0.52648f;
	material_spider.diffuse_color[3] = 1.0f;
	
	material_spider.specular_color[0] = 0.728281f;
	material_spider.specular_color[1] = 0.655802f;
	material_spider.specular_color[2] = 0.466065f;
	material_spider.specular_color[3] = 1.0f;
	
	material_spider.specular_exponent = 71.2f;
	
	material_spider.emissive_color[0] = 0.0f;
	material_spider.emissive_color[1] = 0.0f;
	material_spider.emissive_color[2] = 0.0f;
	material_spider.emissive_color[3] = 1.0f;
	
	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_SPIDER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_SPIDER]);

	My_glTexImage2D_from_file("Data/static_objects/spider.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
}
void prepare_tiger(void) { // vertices enumerated clockwise
	int i, n_bytes_per_vertex, n_bytes_per_triangle, tiger_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_TIGER_FRAMES; i++) {
		sprintf(filename, "Data/dynamic_objects/tiger/Tiger_%d%d_triangles_vnt.geom", i / 10, i % 10);
		tiger_n_triangles[i] = read_geometry(&tiger_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		tiger_n_total_triangles += tiger_n_triangles[i];

		if (i == 0)
			tiger_vertex_offset[i] = 0;
		else
			tiger_vertex_offset[i] = tiger_vertex_offset[i - 1] + 3 * tiger_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &tiger_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glBufferData(GL_ARRAY_BUFFER, tiger_n_total_triangles*n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_TIGER_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, tiger_vertex_offset[i] * n_bytes_per_vertex,
		tiger_n_triangles[i] * n_bytes_per_triangle, tiger_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_TIGER_FRAMES; i++)
		free(tiger_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &tiger_VAO);
	glBindVertexArray(tiger_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	material_tiger.ambient_color[0] = 0.24725f;
	material_tiger.ambient_color[1] = 0.1995f;
	material_tiger.ambient_color[2] = 0.0745f;
	material_tiger.ambient_color[3] = 1.0f;

	material_tiger.diffuse_color[0] = 0.75164f;
	material_tiger.diffuse_color[1] = 0.60648f;
	material_tiger.diffuse_color[2] = 0.22648f;
	material_tiger.diffuse_color[3] = 1.0f;

	material_tiger.specular_color[0] = 0.728281f;
	material_tiger.specular_color[1] = 0.655802f;
	material_tiger.specular_color[2] = 0.466065f;
	material_tiger.specular_color[3] = 1.0f;

	material_tiger.specular_exponent = 51.2f;

	material_tiger.emissive_color[0] = 0.1f;
	material_tiger.emissive_color[1] = 0.1f;
	material_tiger.emissive_color[2] = 0.0f;
	material_tiger.emissive_color[3] = 1.0f;
	
	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	//My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
void prepare_dragon(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, dragon_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/dragon_vnt.geom");
	dragon_n_triangles = read_geometry(&dragon_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	dragon_n_total_triangles += dragon_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &dragon_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, dragon_VBO);
	glBufferData(GL_ARRAY_BUFFER, dragon_n_total_triangles * 3 * n_bytes_per_vertex, dragon_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(dragon_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &dragon_VAO);
	glBindVertexArray(dragon_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, dragon_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_dragon.ambient_color[0] = 0.1523f;
	material_dragon.ambient_color[1] = 0.1995f;
	material_dragon.ambient_color[2] = 0.0745f;
	material_dragon.ambient_color[3] = 1.0f;
	
	material_dragon.diffuse_color[0] = 0.12894f;
	material_dragon.diffuse_color[1] = 0.09283f;
	material_dragon.diffuse_color[2] = 0.62648f;
	material_dragon.diffuse_color[3] = 1.0f;
	
	material_dragon.specular_color[0] = 0.728281f;
	material_dragon.specular_color[1] = 0.655802f;
	material_dragon.specular_color[2] = 0.466065f;
	material_dragon.specular_color[3] = 1.0f;
	
	material_dragon.specular_exponent = 51.2f;
	
	material_dragon.emissive_color[0] = 0.1f;
	material_dragon.emissive_color[1] = 0.1f;
	material_dragon.emissive_color[2] = 0.0f;
	material_dragon.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
void prepare_optimus(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, optimus_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/optimus_vnt.geom");
	optimus_n_triangles = read_geometry(&optimus_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	optimus_n_total_triangles += optimus_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &optimus_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, optimus_VBO);
	glBufferData(GL_ARRAY_BUFFER, optimus_n_total_triangles * 3 * n_bytes_per_vertex, optimus_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(optimus_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &optimus_VAO);
	glBindVertexArray(optimus_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, optimus_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_optimus.ambient_color[0] = 0.24725f;
	//material_optimus.ambient_color[1] = 0.1995f;
	//material_optimus.ambient_color[2] = 0.0745f;
	//material_optimus.ambient_color[3] = 1.0f;
	//
	//material_optimus.diffuse_color[0] = 0.75164f;
	//material_optimus.diffuse_color[1] = 0.60648f;
	//material_optimus.diffuse_color[2] = 0.22648f;
	//material_optimus.diffuse_color[3] = 1.0f;
	//
	//material_optimus.specular_color[0] = 0.728281f;
	//material_optimus.specular_color[1] = 0.655802f;
	//material_optimus.specular_color[2] = 0.466065f;
	//material_optimus.specular_color[3] = 1.0f;
	//
	//material_optimus.specular_exponent = 51.2f;
	//
	//material_optimus.emissive_color[0] = 0.1f;
	//material_optimus.emissive_color[1] = 0.1f;
	//material_optimus.emissive_color[2] = 0.0f;
	//material_optimus.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_SILVER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_SILVER]);

	My_glTexImage2D_from_file("Data/static_objects/silver.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void prepare_bike(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, bike_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/bike_vnt.geom");
	bike_n_triangles = read_geometry(&bike_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	bike_n_total_triangles += bike_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &bike_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, bike_VBO);
	glBufferData(GL_ARRAY_BUFFER, bike_n_total_triangles * 3 * n_bytes_per_vertex, bike_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(bike_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &bike_VAO);
	glBindVertexArray(bike_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, bike_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_bike.ambient_color[0] = 0.24725f;
	//material_bike.ambient_color[1] = 0.1995f;
	//material_bike.ambient_color[2] = 0.0745f;
	//material_bike.ambient_color[3] = 1.0f;
	//
	//material_bike.diffuse_color[0] = 0.75164f;
	//material_bike.diffuse_color[1] = 0.60648f;
	//material_bike.diffuse_color[2] = 0.22648f;
	//material_bike.diffuse_color[3] = 1.0f;
	//
	//material_bike.specular_color[0] = 0.728281f;
	//material_bike.specular_color[1] = 0.655802f;
	//material_bike.specular_color[2] = 0.466065f;
	//material_bike.specular_color[3] = 1.0f;
	//
	//material_bike.specular_exponent = 51.2f;
	//
	//material_bike.emissive_color[0] = 0.1f;
	//material_bike.emissive_color[1] = 0.1f;
	//material_bike.emissive_color[2] = 0.0f;
	//material_bike.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void prepare_godzilla(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, godzilla_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/godzilla_vnt.geom");
	godzilla_n_triangles = read_geometry(&godzilla_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	godzilla_n_total_triangles += godzilla_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &godzilla_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, godzilla_VBO);
	glBufferData(GL_ARRAY_BUFFER, godzilla_n_total_triangles * 3 * n_bytes_per_vertex, godzilla_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(godzilla_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &godzilla_VAO);
	glBindVertexArray(godzilla_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, godzilla_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_godzilla.ambient_color[0] = 0.24725f;
	material_godzilla.ambient_color[1] = 0.1995f;
	material_godzilla.ambient_color[2] = 0.0745f;
	material_godzilla.ambient_color[3] = 1.0f;
	
	material_godzilla.diffuse_color[0] = 0.75164f;
	material_godzilla.diffuse_color[1] = 0.60648f;
	material_godzilla.diffuse_color[2] = 0.22648f;
	material_godzilla.diffuse_color[3] = 1.0f;
	
	material_godzilla.specular_color[0] = 0.728281f;
	material_godzilla.specular_color[1] = 0.655802f;
	material_godzilla.specular_color[2] = 0.466065f;
	material_godzilla.specular_color[3] = 1.0f;
	
	material_godzilla.specular_exponent = 51.2f;
	
	material_godzilla.emissive_color[0] = 0.1f;
	material_godzilla.emissive_color[1] = 0.1f;
	material_godzilla.emissive_color[2] = 0.0f;
	material_godzilla.emissive_color[3] = 1.0f;

	
	//modify5 start
	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]); //modify

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//modify5 end
}

void prepare_tank(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, tank_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/tank_vnt.geom");
	tank_n_triangles = read_geometry(&tank_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	tank_n_total_triangles += tank_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &tank_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, tank_VBO);
	glBufferData(GL_ARRAY_BUFFER, tank_n_total_triangles * 3 * n_bytes_per_vertex, tank_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(tank_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &tank_VAO);
	glBindVertexArray(tank_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, tank_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_tank.ambient_color[0] = 0.24725f;
	//material_tank.ambient_color[1] = 0.1995f;
	//material_tank.ambient_color[2] = 0.0745f;
	//material_tank.ambient_color[3] = 1.0f;
	//
	//material_tank.diffuse_color[0] = 0.75164f;
	//material_tank.diffuse_color[1] = 0.60648f;
	//material_tank.diffuse_color[2] = 0.22648f;
	//material_tank.diffuse_color[3] = 1.0f;
	//
	//material_tank.specular_color[0] = 0.728281f;
	//material_tank.specular_color[1] = 0.655802f;
	//material_tank.specular_color[2] = 0.466065f;
	//material_tank.specular_color[3] = 1.0f;
	//
	//material_tank.specular_exponent = 51.2f;
	//
	//material_tank.emissive_color[0] = 0.1f;
	//material_tank.emissive_color[1] = 0.1f;
	//material_tank.emissive_color[2] = 0.0f;
	//material_tank.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void set_material_tiger(void) {
	// assume ShaderProgram_TXPS is used
	glUniform4fv(loc_material.ambient_color, 1, material_tiger.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_tiger.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_tiger.specular_color);
	glUniform1f(loc_material.specular_exponent, material_tiger.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_tiger.emissive_color);
}

void set_material_godzilla(void) {
	// assume ShaderProgram_TXPS is used
	glUniform4fv(loc_material.ambient_color, 1, material_godzilla.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_godzilla.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_godzilla.specular_color);
	glUniform1f(loc_material.specular_exponent, material_godzilla.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_godzilla.emissive_color);
}

void set_material_dragon(void)
{
	glUniform4fv(loc_material.ambient_color, 1, material_dragon.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_dragon.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_dragon.specular_color);
	glUniform1f(loc_material.specular_exponent, material_dragon.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_dragon.emissive_color);
}

void set_material_spider(void)
{
	glUniform4fv(loc_material.ambient_color, 1, material_spider.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_spider.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_spider.specular_color);
	glUniform1f(loc_material.specular_exponent, material_spider.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_spider.emissive_color);
}

void draw_tiger(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(tiger_VAO);
	glDrawArrays(GL_TRIANGLES, tiger_vertex_offset[cur_frame_tiger], 3 * tiger_n_triangles[cur_frame_tiger]);
	glBindVertexArray(0);
}
void draw_ben(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(ben_VAO);
	glDrawArrays(GL_TRIANGLES, ben_vertex_offset[cur_frame_ben], 3 * ben_n_triangles[cur_frame_ben]);
	glBindVertexArray(0);
}
void draw_wolf(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(wolf_VAO);
	glDrawArrays(GL_TRIANGLES, wolf_vertex_offset[cur_frame_wolf], 3 * wolf_n_triangles[cur_frame_wolf]);
	glBindVertexArray(0);
}
void draw_spider(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(spider_VAO);
	glDrawArrays(GL_TRIANGLES, spider_vertex_offset[cur_frame_spider], 3 * spider_n_triangles[cur_frame_spider]);
	glBindVertexArray(0);
}
void draw_dragon(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(dragon_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * dragon_n_triangles);
	glBindVertexArray(0);
}
void draw_optimus(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(optimus_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * optimus_n_triangles);
	glBindVertexArray(0);
}
void draw_cow(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(cow_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * cow_n_triangles);
	glBindVertexArray(0);
}
void draw_bike(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(bike_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * bike_n_triangles);
	glBindVertexArray(0);
}
void draw_bus(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(bus_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * bus_n_triangles);
	glBindVertexArray(0);
}

void draw_godzilla(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(godzilla_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * godzilla_n_triangles);
	glBindVertexArray(0);
}

void draw_ironman(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(ironman_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * ironman_n_triangles);
	glBindVertexArray(0);
}

void draw_tank(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(tank_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * tank_n_triangles);
	glBindVertexArray(0);
}
// callbacks
float PRP_distance_scale[6] = { 0.5f, 1.0f, 2.5f, 5.0f, 10.0f, 20.0f };

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glUseProgram(h_ShaderProgram_simple);
	ModelViewMatrix = glm::scale(ViewMatrix, glm::vec3(50.0f, 50.0f, 50.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_simple, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glLineWidth(2.0f);
	draw_axes();
	glLineWidth(1.0f);
	
	switch_shader(PHONG);
	set_up_special_light();
	//set_up_special_light_shader();

	glUseProgram(h_ShaderProgram_TXPS);


  	set_material_floor();
	glUniform1i(loc_texture, TEXTURE_ID_FLOOR);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-500.0f, 0.0f, 500.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(1000.0f, 1000.0f, 1000.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f*TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_floor();
	
	//modify s
	flag_texture_mapping = 0;
	//glUseProgram(h_ShaderProgram_TXPS);
	glUniform1i(loc_flag_texture_mapping, flag_texture_mapping);
	//glUseProgram(0);
	//modify e

 	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
 	
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(x_distance_tiger, 0.0f, z_distance_tiger));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
	//ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(200.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f*TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_tiger();

	glUseProgram(h_ShaderProgram_simple);
	ModelViewProjectionMatrix = glm::scale(ModelViewProjectionMatrix, glm::vec3(20.0f, 20.0f, 20.0f));
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_simple, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_axes();



	glUseProgram(h_ShaderProgram_TXPS);
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(0.0f, 0.0f, -go_distance_ben));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, rotation_angle_ben, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(100.0f, -100.0f, -100.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_ben();
	
	
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);


	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(go_distance_wolf, jump_sin_wolf, 0.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, angle_sin_wolf, glm::vec3(0.0f, 0.0f, 1.0f));


	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(100.0f, 100.0f,100.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_wolf();
	
	//modify s
	flag_texture_mapping = 1;
	//glUseProgram(h_ShaderProgram_TXPS);
	glUniform1i(loc_flag_texture_mapping, flag_texture_mapping);
	//glUseProgram(0);
	//modify e

	set_material_spider();
	glUniform1i(loc_texture, TEXTURE_ID_SPIDER);
	
	/*
	ModelViewMatrix = glm::rotate(ViewMatrix, rotation_angle_spider, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(180.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -rotation_sin_angle_spider, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(move_sin_distance_spider, 0.0f, 0.0f));



	ModelViewMatrix = glm::rotate(ModelViewMatrix, 180.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f)); //own
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(50.0f, -50.0f, 50.0f));
	*/

	glm::mat4 ModelMatrix = glm::rotate(glm::mat4(1.0f), rotation_angle_spider, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(180.0f, 0.0f, 0.0f));
	ModelMatrix = glm::rotate(ModelMatrix, -rotation_sin_angle_spider, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(move_sin_distance_spider, 0.0f, 0.0f));

	now_spider_position = glm::vec3(ModelMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

	ModelMatrix = glm::rotate(ModelMatrix, 180.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f)); //own
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(50.0f, -50.0f, 50.0f));

	ModelViewMatrix = ViewMatrix * ModelMatrix;
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	set_up_shader();
	glUseProgram(h_ShaderProgram_TXPS);

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_spider();

	//modify s
	flag_texture_mapping = 0;
	//glUseProgram(h_ShaderProgram_TXPS);
	glUniform1i(loc_flag_texture_mapping, flag_texture_mapping);
	//glUseProgram(0);
	//modify e


	set_material_dragon();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	//ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-50.0f, 50.0f, 20.0f)); modify
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-230.0f, 200.0f, -170.0f));
	//ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f)); modify
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -135.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f*TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	//ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(3.0f, 3.0f, 3.0f)); modify
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(5.5f, 5.5f, 5.5f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_dragon();

	//modify s
	flag_texture_mapping = 1;
	//glUseProgram(h_ShaderProgram_TXPS);
	glUniform1i(loc_flag_texture_mapping, flag_texture_mapping);
	//glUseProgram(0);
	//modify e

	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_SILVER);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(80.0f, 100.0f, -50.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	//ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f*TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f)); modify
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f)); 
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(0.1f, 0.1f, 0.1f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_optimus();

	//modify s
	flag_texture_mapping = 0;
	//glUseProgram(h_ShaderProgram_TXPS);
	glUniform1i(loc_flag_texture_mapping, flag_texture_mapping);
	//glUseProgram(0);
	//modify e


	/*
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(100.0f, 30.0f, 10.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(80.0f, 80.0f, 80.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_cow();
	*/
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	//ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-80.0f, 0.0f, 80.0f)); modify
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-100.0f, 30.0f, 80.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -45.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f)); //own
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(25.0f, 25.0f, 25.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_bike();
	/*
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(100.0f, 0.0f, 80.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(3.0f, 3.0f, 3.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_bus();
	*/

	glUseProgram(h_ShaderProgram_GS);
	switch_shader(GOURAUD);
	set_material_godzilla();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-450.0f, 0.0f, -70.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 120.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(1.5f, 1.5f, 1.5f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_GS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_GS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_GS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_godzilla();

	switch_shader(PHONG);
	glUseProgram(h_ShaderProgram_TXPS);
	/*
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(0.0f, 0.0f, 120.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(20.0f, 20.0f, 20.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_ironman();
	*/

	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(300.0f, 150.0f, -300.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 150.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f)); //own
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f*TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(15.0f, 15.0f, 15.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_tank();


	//modify s
	flag_texture_mapping = 1;
	//glUseProgram(h_ShaderProgram_TXPS);
	glUniform1i(loc_flag_texture_mapping, flag_texture_mapping);
	//glUseProgram(0);
	//modify e

	glUseProgram(0);

	glutSwapBuffers();
}

void tiger_timer_scene(int value) {
	tiger_timestamp_scene = (tiger_timestamp_scene + 1) % UINT_MAX;
	cur_frame_tiger = tiger_timestamp_scene % N_TIGER_FRAMES;

	
		rotation_angle_tiger = (((tiger_timestamp_scene + 300) % 2400) / 600) * 90.0f * TO_RADIAN;
		int tmp = tiger_timestamp_scene % 2400;
		if (tmp < 300)
		{
			x_distance_tiger = 300.0f;
			z_distance_tiger = tmp;
		}
		else if (tmp < 600)
		{
			x_distance_tiger = -tmp + 600.0f;
			z_distance_tiger = 300.0f;
		}
		else if (tmp < 900)
		{
			x_distance_tiger = -tmp + 600.0f;
			z_distance_tiger = 300.0f;
		}
		else if (tmp < 1200)
		{
			x_distance_tiger = -300.0f;
			z_distance_tiger = -tmp + 1200.0f;
		}
		else if (tmp < 1500)
		{
			x_distance_tiger = -300.0f;
			z_distance_tiger = -tmp + 1200.0f;
		}
		else if (tmp < 1800)
		{
			x_distance_tiger = tmp - 1800.0f;
			z_distance_tiger = -300.0f;
		}
		else if (tmp < 2100)
		{
			x_distance_tiger = tmp - 1800.0f;
			z_distance_tiger = -300.0f;
		}
		else
		{
			x_distance_tiger = 300.0f;
			z_distance_tiger = tmp - 2400.0f;
		}
	
	glutPostRedisplay();
	if (flag_tiger_animation)
		glutTimerFunc(10, tiger_timer_scene, 0);
}

void ben_timer_scene(int value)
{
	ben_timestamp_scene = (ben_timestamp_scene + 1) % UINT_MAX;
	cur_frame_ben = ben_timestamp_scene % N_BEN_FRAMES;
	rotation_angle_ben = (2 * (ben_timestamp_scene % 180)) * TO_RADIAN;
	go_distance_ben = (500.0f - ben_timestamp_scene % 1000);
	glutPostRedisplay();
	if (flag_ben_animation)
		glutTimerFunc(10, ben_timer_scene, 0);
}

void spider_timer_scene(int value)
{
	spider_timestamp_scene = (spider_timestamp_scene + 1) % UINT_MAX;
	cur_frame_spider = spider_timestamp_scene % N_SPIDER_FRAMES;

	rotation_angle_spider = (spider_timestamp_scene % 360) * TO_RADIAN;
	move_sin_distance_spider = 50.0f * sin(((spider_timestamp_scene) % 90) * TO_RADIAN * 4);
	rotation_sin_angle_spider = 45.0f * cos(((spider_timestamp_scene) % 90) * TO_RADIAN * 4) * TO_RADIAN;
	glutPostRedisplay();
	if (flag_spider_animation)
		glutTimerFunc(10, spider_timer_scene, 0);
}

void wolf_timer_scene(int value)
{
	wolf_timestamp_scene = (wolf_timestamp_scene + 1) % UINT_MAX;
	cur_frame_wolf = wolf_timestamp_scene % N_WOLF_FRAMES;
	jump_sin_wolf = 20.0f * abs(sin(((wolf_timestamp_scene) % 90) * TO_RADIAN * 4));
	angle_sin_wolf = 45.0f * cos(((wolf_timestamp_scene) % 90) * TO_RADIAN * 4) * TO_RADIAN;
	go_distance_wolf = (500.0f - (2 * (wolf_timestamp_scene + 100) + 500) % 1000);
	glutPostRedisplay();
	if (flag_wolf_animation)
		glutTimerFunc(10, wolf_timer_scene, 0);
}

void Randomize(void)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dis(0, 99);

	float a[100];
	for (int i = 0; i < 100; ++i)
	{
		a[i] = dis(gen);
		if (i > 36)
			a[i] /= 100;
		else
			a[i] *= 10;
	}
	for (int i = 4; i < NUMBER_OF_LIGHT_SUPPORTED; ++i)
	{
		light[i].ambient_color[0] = a[i * 9];
		light[i].ambient_color[1] = a[i * 9 + 1];
		light[i].ambient_color[2] = a[i * 9 + 2];
		light[i].diffuse_color[0] = a[i * 9 + 3];
		light[i].diffuse_color[1] = a[i * 9 + 4];
		light[i].diffuse_color[2] = a[i * 9 + 5];
		light[i].specular_color[0] = a[i * 9 + 6];
		light[i].specular_color[1] = a[i * 9 + 7];
		light[i].specular_color[2] = a[i * 9 + 8];

		int now = i - 4;
		special_light_point[now][0] = glm::vec3(a[now * 9]-500.0f, 0.0f, a[now * 9 + 2]-500.0f);
		special_light_point[now][1] = glm::vec3(a[now * 9 + 3]-500.0f, 0.0f , a[now * 9 + 5]-500.0f);
		special_light_point[now][2] = glm::vec3(a[now * 9 + 6]-500.0f, 0.0f, a[now * 9 + 8]-500.0f);
	}
}

void light_timer_scene1(int value)
{

	light_timestamp_scene = (light_timestamp_scene + 1) % UINT_MAX;
	
		auto now = light_timestamp_scene % 360;
		for (int i = 0; i < 3; ++i)
		{
			glm::vec3 tmp = special_light_point[now/120][i] - special_light_destination[i];
			tmp.x *= 0.25f*(now % 120)/120.0f;
			tmp.y *= 0.25f*(now % 120)/120.0f;
			tmp.z *= 0.25f*(now % 120)/120.0f;
			special_light_destination[i] += tmp;
		}
	if (flag_special_light==1)
		glutTimerFunc(10, light_timer_scene1, 0);
}

void light_timer_scene2(int value)
{
	light_timestamp_scene = (light_timestamp_scene + 1) % UINT_MAX;
	if (light_timestamp_scene < 1080)
	{
		auto now = light_timestamp_scene;
		for (int i = 0; i < 3; ++i)
		{
			glm::vec3 tmp = now_spider_position - special_light_destination[i];
			tmp.x *= 0.25f * (now % 1080) / 1080.0f;
			tmp.y *= 0.25f * (now % 1080) / 1080.0f;
			tmp.z *= 0.25f * (now % 1080) / 1080.0f;
			special_light_destination[i] += tmp;
		}
	}
	else
	{
		for (int i = 0; i < 3; ++i)
		{
			special_light_destination[i] = now_spider_position;
		}

	}
	if (flag_special_light == 2)
		glutTimerFunc(10, light_timer_scene2, 0);
}

void light_timer_scene3(int value)
{
	light_timestamp_scene = (light_timestamp_scene + 1) % UINT_MAX;
	auto now = light_timestamp_scene % 30;
	if (now == 9 || now == 19 || now == 29) Randomize();
	
	for (int i = 0; i < 3; ++i)
	{
		glm::vec3 tmp = special_light_point[now / 10][i] - special_light_destination[i];
		tmp.x *= 0.25f * (now % 30) / 30.0f;
		tmp.y *= 0.25f * (now % 30) / 30.0f;
		tmp.z *= 0.25f * (now % 30) / 30.0f;
		special_light_destination[i] += tmp;
	}

	if (flag_special_light == 3)
		glutTimerFunc(10, light_timer_scene3, 0);
}

void reshape(int width, int height) {

	glViewport(0, 0, width, height);
	aspect_ratio = (float)width / height;
	ProjectionMatrix = glm::perspective(fovy * TO_RADIAN, aspect_ratio, 100.0f, 20000.0f);
	glutPostRedisplay();
}

void resetCamera()
{
	if (prev_idx != EC_idx)
		initialize_camera();
}

void keyboard(unsigned char key, int x, int y) {
	static int flag_cull_face = 0;
	static int PRP_distance_level = 4;

	glm::vec4 position_EC;
	glm::vec3 direction_EC;
	/*
	if ((key >= '0') && (key <= '0' + NUMBER_OF_LIGHT_SUPPORTED - 1)) {
		int light_ID = (int)(key - '0');

		glUseProgram(h_ShaderProgram_TXPS);
		light[light_ID].light_on = 1 - light[light_ID].light_on;
		glUniform1i(loc_light[light_ID].light_on, light[light_ID].light_on);
		glUseProgram(0);

		glutPostRedisplay();
		return;
	}*/

	switch (key) {

	case 'q': // toggle the animation effect.
		flag_tiger_animation = 1 - flag_tiger_animation;
		if (flag_tiger_animation) {
			glutTimerFunc(100, tiger_timer_scene, 0);
			fprintf(stdout, "^^^tiger Animation mode ON.\n");
		}
		else
			fprintf(stdout, "^^^tiger Animation mode OFF.\n");
		break;
	case 'w': // toggle the animation effect.
		flag_ben_animation = 1 - flag_ben_animation;
		if (flag_ben_animation) {
			glutTimerFunc(100, ben_timer_scene, 0);
			fprintf(stdout, "^^^ben Animation mode ON.\n");
		}
		else
			fprintf(stdout, "^^^ben Animation mode OFF.\n");
		break;
	case 'e': // toggle the animation effect.
		flag_spider_animation = 1 - flag_spider_animation;
		if (flag_spider_animation) {
			glutTimerFunc(100, spider_timer_scene, 0);
			fprintf(stdout, "^^^spider Animation mode ON.\n");
		}
		else
			fprintf(stdout, "^^^spider Animation mode OFF.\n");
		break;
	case 'r': // toggle the animation effect.
		flag_wolf_animation = 1 - flag_wolf_animation;
		if (flag_wolf_animation) {
			glutTimerFunc(100, wolf_timer_scene, 0);
			fprintf(stdout, "^^^wolf Animation mode ON.\n");
		}
		else
			fprintf(stdout, "^^^wolf Animation mode OFF.\n");
		break;

	case 'a':
		nowaxe = X;
		break;
	case 's':
		nowaxe = Y;
		break;
	case 'd':
		nowaxe = Z;
		break;

	case 'z':
		if (EC_idx == 5)
		{
			fovy = 30.0f < fovy - 1.0f ? fovy - 1.0f : 30.0f;
			ProjectionMatrix = glm::perspective(fovy * TO_RADIAN, aspect_ratio, 100.0f, 20000.0f);
			glutPostRedisplay();
		}
		break;
	case 'x':
		if (EC_idx == 5)
		{
			fovy = 60.0f > fovy + 1.0f ? fovy + 1.0f : 60.0f;
			ProjectionMatrix = glm::perspective(fovy * TO_RADIAN, aspect_ratio, 100.0f, 20000.0f);
			glutPostRedisplay();
		}
		break;
	case 'f':
		light[1].light_on = 1-light[1].light_on;
		light[4].light_on = 1 - light[4].light_on;
		light[5].light_on = 1 - light[5].light_on;
		light[6].light_on = 1 - light[6].light_on;
		set_up_shader();
		glutPostRedisplay();
		break;
	case 'g':
		light[2].light_on = 1 - light[2].light_on;
		set_up_shader();
		glutPostRedisplay();
		break;
	case 'h':
		light[3].light_on = 1 - light[3].light_on;
		set_up_shader();
		glutPostRedisplay();
		break;


	case 't':
		flag_special_light = flag_special_light==1?0:1;
		if (flag_special_light==1)
		{
			light_timestamp_scene = 0;
			fprintf(stdout, "^^^light Animation mode ON.\n");
			glutTimerFunc(100, light_timer_scene1, 0);
		}
		break;
	case 'y':
		flag_special_light = flag_special_light == 2 ? 0 : 2;
		if (flag_special_light==2)
		{
			light_timestamp_scene = 0;
			glutTimerFunc(100, light_timer_scene2, 0);
		}
		break;
	case 'u':
		if (flag_special_light == 3)
		{
			for (int i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; ++i)
				light[i] = OriginLight[i];
			for (int i = 0; i < 3; ++i)
			{
				special_light_point[i][0] = origin_special_light_point[i][0];
				special_light_point[i][1] = origin_special_light_point[i][1];
				special_light_point[i][2] = origin_special_light_point[i][2];
			}
		}
		flag_special_light = flag_special_light == 3 ? 0 : 3;
		if (flag_special_light)
		{
			light_timestamp_scene = 0;
			glutTimerFunc(100, light_timer_scene3, 0);
		}
		break;
		/*
		case 'f':
			flag_fog = 1 - flag_fog;
			glUseProgram(h_ShaderProgram_TXPS);
			glUniform1i(loc_flag_fog, flag_fog);
			glUseProgram(0);
			glutPostRedisplay();
			break;
		case 't':
			flag_texture_mapping = 1 - flag_texture_mapping;
			glUseProgram(h_ShaderProgram_TXPS);
			glUniform1i(loc_flag_texture_mapping, flag_texture_mapping);
			glUseProgram(0);
			glutPostRedisplay();
			break;
		case 'c':
			flag_cull_face = (flag_cull_face + 1) % 3;
			switch (flag_cull_face) {
			case 0:
				glDisable(GL_CULL_FACE);
				glutPostRedisplay();
				break;
			case 1: // cull back faces;
				glCullFace(GL_BACK);
				glEnable(GL_CULL_FACE);
				glutPostRedisplay();
				break;
			case 2: // cull front faces;
				glCullFace(GL_FRONT);
				glEnable(GL_CULL_FACE);
				glutPostRedisplay();
				break;
			}
			break;
			*/
			//own start
	case '1':
		EC_idx = 1;
		Center_idx = 1;
		resetCamera();
		set_ViewMatrix_from_camera_frame();
		//ViewMatrix = glm::lookAt(EC[EC_idx], Center[Center_idx], VUP);
		set_up_shader();
		glutPostRedisplay();
		break;
	case '2':
		EC_idx = 2;
		Center_idx = 2;
		resetCamera();
		set_ViewMatrix_from_camera_frame();
		//ViewMatrix = glm::lookAt(EC[EC_idx], Center[Center_idx], VUP);
		set_up_shader();
		glutPostRedisplay();
		break;
	case '3':
		EC_idx = 3;
		Center_idx = 3;
		resetCamera();
		set_ViewMatrix_from_camera_frame();
		//ViewMatrix = glm::lookAt(EC[EC_idx], Center[Center_idx], VUP);
		set_up_shader();
		glutPostRedisplay();
		break;
	case '4':
		EC_idx = 4;
		Center_idx = 4;
		resetCamera();
		set_ViewMatrix_from_camera_frame();
		//ViewMatrix = glm::lookAt(EC[EC_idx], Center[Center_idx], VUP);
		set_up_shader();
		glutPostRedisplay();
		break;
	case '5':
		EC_idx = 5;
		Center_idx = 5;
		resetCamera();
		set_ViewMatrix_from_camera_frame();
		//ViewMatrix = glm::lookAt(EC[EC_idx], Center[Center_idx], VUP);
		fixedViewMatrix = ViewMatrix;
		set_up_shader();
		glutPostRedisplay();
		break;

		/*
		case 'd':
			PRP_distance_level = (PRP_distance_level + 1) % 6;
			fprintf(stdout, "^^^ Distance level = %d.\n", PRP_distance_level);

			ViewMatrix = glm::lookAt(PRP_distance_scale[PRP_distance_level] * glm::vec3(500.0f, 300.0f, 500.0f),
				glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

			glUseProgram(h_ShaderProgram_TXPS);
			// Must update the light 1's geometry in EC.
			position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
				light[1].position[2], light[1].position[3]);
			glUniform4fv(loc_light[1].position, 1, &position_EC[0]);
			direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0],
				light[1].spot_direction[1], light[1].spot_direction[2]);
			glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]);
			glUseProgram(0);
			glutPostRedisplay();
			break;
			*/
	case 'p':
		flag_polygon_fill = 1 - flag_polygon_fill;
		if (flag_polygon_fill)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glutPostRedisplay();
		break;

	case 27: // ESC key
		glutLeaveMainLoop(); // Incur destuction callback for cleanups
		break;
	}
	
	
	prev_idx = EC_idx;
}

void cleanup(void) {
	glDeleteVertexArrays(1, &axes_VAO); 
	glDeleteBuffers(1, &axes_VBO);

	glDeleteVertexArrays(1, &rectangle_VAO);
	glDeleteBuffers(1, &rectangle_VBO);

	glDeleteVertexArrays(1, &tiger_VAO);
	glDeleteBuffers(1, &tiger_VBO);

	glDeleteTextures(N_TEXTURES_USED, texture_names);
}
/*
void renew_cam_position(int amount)
{
	if (EC_idx == 5)
	{
		glm::vec3 del = glm::vec3(amount, amount, amount);
		glm::vec3 n = normalize(Center[Center_idx] - EC[EC_idx]);
		glm::vec3 u = normalize(cross(n, VUP));
		glm::vec3 v = cross(u,n);

		switch (nowaxe)
		{
		case X:
			EC[EC_idx] += del*u * CAM_TSPEED;
			Center[Center_idx] += del * u * CAM_TSPEED;
			break;
		case Y:
			EC[EC_idx] += del * v * CAM_TSPEED;
			Center[Center_idx] += del * v * CAM_TSPEED;
			break;
		case Z:
			EC[EC_idx] += del * n * CAM_TSPEED;
			Center[Center_idx] += del * n * CAM_TSPEED;
			break;
		}
	}
}

void renew_cam_orientation_rotation_around_v_axis(int angle)
{
	if (EC_idx == 5||EC_idx==1)
	{
		glm::vec3 n = normalize(Center[Center_idx] - EC[EC_idx]);
		glm::vec3 u = normalize(cross(n, VUP));
		glm::vec3 v = cross(u, n);
		glm::mat3 RotationMatrix;
		glm::vec3 direction;
		
		switch (nowaxe)
		{
		case X:
			RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), CAM_RSPEED * TO_RADIAN * angle,u));
			direction = RotationMatrix * n;
			Center[Center_idx] = EC[EC_idx] + direction;
			break;

		case Y:
			RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), CAM_RSPEED * TO_RADIAN * angle, v));
			direction = RotationMatrix * n;
			Center[Center_idx] = EC[EC_idx] + direction;
			break;
		case Z:
			RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), CAM_RSPEED * TO_RADIAN * angle, n));
			VUP = RotationMatrix * VUP;
			break;
		}
	}
}

void set_ViewMatrix_from_camera_frame(void) {

	if (EC_idx == 5 || EC_idx == 1)
	{
		ViewMatrix = glm::lookAt(EC[EC_idx], Center[Center_idx], VUP);
	}
}
*/
void motion(int x, int y) {
	if (!cameramove) return;

	renew_cam_position(prevy - y);
	renew_cam_orientation_rotation_around_v_axis(prevx - x);

	prevx = x; prevy = y;

	set_ViewMatrix_from_camera_frame();
	set_up_shader();
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
	if ((button == GLUT_LEFT_BUTTON)) {
		if (state == GLUT_DOWN) {
			cameramove = 1;
			prevx = x; prevy = y;
		}
		else if (state == GLUT_UP) cameramove = 0;
	}
}

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glutTimerFunc(100, tiger_timer_scene, 0);
	glutTimerFunc(100, ben_timer_scene, 0);
	glutTimerFunc(100, spider_timer_scene, 0);
	glutTimerFunc(100, wolf_timer_scene, 0);
	//glutTimerFunc(100, light_timer_scene, 0);
	glutCloseFunc(cleanup);
}

void prepare_shader_program(void) {
	int i;
	char string[1024];
	ShaderInfo shader_info_simple[3] = {
		{ GL_VERTEX_SHADER, "Shaders/simple.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/simple.frag" },
		{ GL_NONE, NULL }
	};
	ShaderInfo shader_info_TXPS[3] = {
		{ GL_VERTEX_SHADER, "Shaders/Phong_Tx.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/Phong_Tx.frag" },
		{ GL_NONE, NULL }
	};
	//modify 5 start
	
	//modify 5 end

	h_ShaderProgram_simple = LoadShaders(shader_info_simple);
	loc_primitive_color = glGetUniformLocation(h_ShaderProgram_simple, "u_primitive_color");
	loc_ModelViewProjectionMatrix_simple = glGetUniformLocation(h_ShaderProgram_simple, "u_ModelViewProjectionMatrix");

	h_ShaderProgram_TXPS = LoadShaders(shader_info_TXPS);
	loc_ModelViewProjectionMatrix_TXPS = glGetUniformLocation(h_ShaderProgram_TXPS, "u_ModelViewProjectionMatrix");
	loc_ModelViewMatrix_TXPS = glGetUniformLocation(h_ShaderProgram_TXPS, "u_ModelViewMatrix");
	loc_ModelViewMatrixInvTrans_TXPS = glGetUniformLocation(h_ShaderProgram_TXPS, "u_ModelViewMatrixInvTrans");

	//modify 5 start

	//modify 5 end

	loc_global_ambient_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_global_ambient_color");
	ploc_global_ambient_color = loc_global_ambient_color;
	for (i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
		sprintf(string, "u_light[%d].light_on", i);
		loc_light[i].light_on = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		ploc_light[i].light_on = loc_light[i].light_on;

		sprintf(string, "u_light[%d].position", i);
		loc_light[i].position = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		ploc_light[i].position = loc_light[i].position;

		sprintf(string, "u_light[%d].ambient_color", i);
		loc_light[i].ambient_color = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		ploc_light[i].ambient_color = loc_light[i].ambient_color;

		sprintf(string, "u_light[%d].diffuse_color", i);
		loc_light[i].diffuse_color = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		ploc_light[i].diffuse_color = loc_light[i].diffuse_color;

		sprintf(string, "u_light[%d].specular_color", i);
		loc_light[i].specular_color = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		ploc_light[i].specular_color = loc_light[i].specular_color;

		sprintf(string, "u_light[%d].spot_direction", i);
		loc_light[i].spot_direction = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		ploc_light[i].spot_direction = loc_light[i].spot_direction;

		sprintf(string, "u_light[%d].spot_exponent", i);
		loc_light[i].spot_exponent = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		ploc_light[i].spot_exponent = loc_light[i].spot_exponent;

		sprintf(string, "u_light[%d].spot_cutoff_angle", i);
		loc_light[i].spot_cutoff_angle = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		ploc_light[i].spot_cutoff_angle = loc_light[i].spot_cutoff_angle;

		sprintf(string, "u_light[%d].light_attenuation_factors", i);
		loc_light[i].light_attenuation_factors = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		ploc_light[i].light_attenuation_factors = loc_light[i].light_attenuation_factors;
	}

	loc_material.ambient_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.ambient_color");
	loc_material.diffuse_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.diffuse_color");
	loc_material.specular_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.specular_color");
	loc_material.emissive_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.emissive_color");
	loc_material.specular_exponent = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.specular_exponent");

	ploc_material.ambient_color = loc_material.ambient_color;
	ploc_material.diffuse_color = loc_material.diffuse_color;
	ploc_material.specular_color = loc_material.specular_color;
	ploc_material.emissive_color = loc_material.emissive_color;
	ploc_material.specular_exponent = loc_material.specular_exponent;


	//modify start
	
	//modify end

	loc_texture = glGetUniformLocation(h_ShaderProgram_TXPS, "u_base_texture");
	ploc_texture = loc_texture;

	loc_flag_texture_mapping = glGetUniformLocation(h_ShaderProgram_TXPS, "u_flag_texture_mapping");
	ploc_flag_texture_mapping = loc_flag_texture_mapping;
	loc_flag_fog = glGetUniformLocation(h_ShaderProgram_TXPS, "u_flag_fog");
	ploc_flag_fog = loc_flag_fog;
}

void prepare_gouraud_shader(void)
{
	ShaderInfo shader_info_GS[3] = {
		{ GL_VERTEX_SHADER, "Shaders/Gouraud.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/Gouraud.frag" },
		{ GL_NONE, NULL }
	};
	int i;
	char string[1024];

	h_ShaderProgram_GS = LoadShaders(shader_info_GS);
	loc_ModelViewProjectionMatrix_GS = glGetUniformLocation(h_ShaderProgram_GS, "u_ModelViewProjectionMatrix");
	loc_ModelViewMatrix_GS = glGetUniformLocation(h_ShaderProgram_GS, "u_ModelViewMatrix");
	loc_ModelViewMatrixInvTrans_GS = glGetUniformLocation(h_ShaderProgram_GS, "u_ModelViewMatrixInvTrans");

	loc_global_ambient_color = glGetUniformLocation(h_ShaderProgram_GS, "u_global_ambient_color");
	gloc_global_ambient_color = loc_global_ambient_color;
	for (i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
		sprintf(string, "u_light[%d].light_on", i);
		gloc_light[i].light_on = glGetUniformLocation(h_ShaderProgram_GS, string);
		sprintf(string, "u_light[%d].position", i);
		gloc_light[i].position = glGetUniformLocation(h_ShaderProgram_GS, string);
		sprintf(string, "u_light[%d].ambient_color", i);
		gloc_light[i].ambient_color = glGetUniformLocation(h_ShaderProgram_GS, string);
		sprintf(string, "u_light[%d].diffuse_color", i);
		gloc_light[i].diffuse_color = glGetUniformLocation(h_ShaderProgram_GS, string);
		sprintf(string, "u_light[%d].specular_color", i);
		gloc_light[i].specular_color = glGetUniformLocation(h_ShaderProgram_GS, string);
		sprintf(string, "u_light[%d].spot_direction", i);
		gloc_light[i].spot_direction = glGetUniformLocation(h_ShaderProgram_GS, string);
		sprintf(string, "u_light[%d].spot_exponent", i);
		gloc_light[i].spot_exponent = glGetUniformLocation(h_ShaderProgram_GS, string);
		sprintf(string, "u_light[%d].spot_cutoff_angle", i);
		gloc_light[i].spot_cutoff_angle = glGetUniformLocation(h_ShaderProgram_GS, string);
		sprintf(string, "u_light[%d].light_attenuation_factors", i);
		gloc_light[i].light_attenuation_factors = glGetUniformLocation(h_ShaderProgram_GS, string);
	}

	gloc_material.ambient_color = glGetUniformLocation(h_ShaderProgram_GS, "u_material.ambient_color");
	gloc_material.diffuse_color = glGetUniformLocation(h_ShaderProgram_GS, "u_material.diffuse_color");
	gloc_material.specular_color = glGetUniformLocation(h_ShaderProgram_GS, "u_material.specular_color");
	gloc_material.emissive_color = glGetUniformLocation(h_ShaderProgram_GS, "u_material.emissive_color");
	gloc_material.specular_exponent = glGetUniformLocation(h_ShaderProgram_GS, "u_material.specular_exponent");

	gloc_texture = glGetUniformLocation(h_ShaderProgram_GS, "u_base_texture");

	gloc_flag_texture_mapping = glGetUniformLocation(h_ShaderProgram_GS, "u_flag_texture_mapping");
	gloc_flag_fog = glGetUniformLocation(h_ShaderProgram_GS, "u_flag_fog");

}

void switch_shader(int now_shader)
{
	flag_shader = now_shader;
	if (flag_shader == PHONG)
	{
		for (int i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; ++i)
		{
			loc_light[i] = ploc_light[i];
		}
		loc_global_ambient_color = ploc_global_ambient_color;
		loc_material = ploc_material;
		loc_texture = ploc_texture;
		loc_flag_texture_mapping = ploc_flag_texture_mapping;
		loc_flag_fog = ploc_flag_fog;
	}
	else
	{
		for (int i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; ++i)
		{
			loc_light[i] = gloc_light[i];
		}
		loc_global_ambient_color = gloc_global_ambient_color;
		loc_material = gloc_material;
		loc_texture = gloc_texture;
		loc_flag_texture_mapping = gloc_flag_texture_mapping;
		loc_flag_fog = gloc_flag_fog;
	}
}


void initialize_lights_and_material(void) { // follow OpenGL conventions for initialization

	int i;

	glUseProgram(h_ShaderProgram_TXPS);

	for (int kk = 0; kk < 2; ++kk)
	{
		if (kk == 1)
		{
			switch_shader(GOURAUD);
			glUseProgram(h_ShaderProgram_GS);
		}

		glUniform4f(loc_global_ambient_color, 0.115f, 0.115f, 0.115f, 1.0f);
		for (i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
			glUniform1i(loc_light[i].light_on, 0); // turn off all lights initially
			glUniform4f(loc_light[i].position, 0.0f, 0.0f, 1.0f, 0.0f);
			glUniform4f(loc_light[i].ambient_color, 0.0f, 0.0f, 0.0f, 1.0f);
			if (i == 0) {
				glUniform4f(loc_light[i].diffuse_color, 1.0f, 1.0f, 1.0f, 1.0f);
				glUniform4f(loc_light[i].specular_color, 1.0f, 1.0f, 1.0f, 1.0f);
			}
			else {
				glUniform4f(loc_light[i].diffuse_color, 0.0f, 0.0f, 0.0f, 1.0f);
				glUniform4f(loc_light[i].specular_color, 0.0f, 0.0f, 0.0f, 1.0f);
			}
			glUniform3f(loc_light[i].spot_direction, 0.0f, 0.0f, -1.0f);
			glUniform1f(loc_light[i].spot_exponent, 0.0f); // [0.0, 128.0]
			glUniform1f(loc_light[i].spot_cutoff_angle, 180.0f); // [0.0, 90.0] or 180.0 (180.0 for no spot light effect)
			glUniform4f(loc_light[i].light_attenuation_factors, 1.0f, 0.0f, 0.0f, 0.0f); // .w != 0.0f for no ligth attenuation
		}


		glUniform4f(loc_material.ambient_color, 0.2f, 0.2f, 0.2f, 1.0f);
		glUniform4f(loc_material.diffuse_color, 0.8f, 0.8f, 0.8f, 1.0f);
		glUniform4f(loc_material.specular_color, 0.0f, 0.0f, 0.0f, 1.0f);
		glUniform4f(loc_material.emissive_color, 0.0f, 0.0f, 0.0f, 1.0f);
		glUniform1f(loc_material.specular_exponent, 0.0f); // [0.0, 128.0]
	}
	switch_shader(PHONG);
	
	glUseProgram(0);
}

void initialize_flags(void) {
	flag_tiger_animation = 1;
	flag_ben_animation = 1;
	flag_spider_animation = 1;
	flag_wolf_animation = 1;
	flag_polygon_fill = 1;
	flag_texture_mapping = 1;
	light_timestamp_scene = 0;
	flag_fog = 0;
	EC_idx = 1;
	Center_idx = 1;

	glUseProgram(h_ShaderProgram_TXPS);
	glUniform1i(loc_flag_fog, flag_fog);
	glUniform1i(loc_flag_texture_mapping, flag_texture_mapping);
	glUseProgram(0);
}

void initialize_OpenGL(void) {

	glEnable(GL_MULTISAMPLE);


  	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	//ViewMatrix = glm::lookAt(PRP_distance_scale[0] * glm::vec3(500.0f, 300.0f, 500.0f),
	//	glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	initialize_flags();
	set_ViewMatrix_from_camera_frame();

	glGenTextures(N_TEXTURES_USED, texture_names);
}

void set_up_scene_lights(void) {
	// point_light_EC: use light 0
	light[0].light_on = 1;
	light[0].position[0] = 0.0f; light[0].position[1] = 100.0f; 	// point light position in EC
	light[0].position[2] = 0.0f; light[0].position[3] = 1.0f;

	light[0].ambient_color[0] = 0.13f; light[0].ambient_color[1] = 0.13f;
	light[0].ambient_color[2] = 0.13f; light[0].ambient_color[3] = 1.0f;

	light[0].diffuse_color[0] = 0.5f; light[0].diffuse_color[1] = 0.5f;
	light[0].diffuse_color[2] = 0.5f; light[0].diffuse_color[3] = 1.5f;

	light[0].specular_color[0] = 0.8f; light[0].specular_color[1] = 0.8f;
	light[0].specular_color[2] = 0.8f; light[0].specular_color[3] = 1.0f;

	// spot_light_WC: use light 1
	light[1].light_on = 1;
	light[1].position[0] = -100.0f; light[1].position[1] = 500.0f; // spot light position in WC
	light[1].position[2] = 50.0f; light[1].position[3] = 1.0f;

	light[1].ambient_color[0] = 0.152f; light[1].ambient_color[1] = 0.152f;
	light[1].ambient_color[2] = 0.152f; light[1].ambient_color[3] = 1.0f;

	light[1].diffuse_color[0] = 0.572f; light[1].diffuse_color[1] = 0.572f;
	light[1].diffuse_color[2] = 0.572f; light[1].diffuse_color[3] = 1.0f;

	light[1].specular_color[0] = 0.772f; light[1].specular_color[1] = 0.772f;
	light[1].specular_color[2] = 0.772f; light[1].specular_color[3] = 1.0f;

	light[1].spot_direction[0] = 0.0f; light[1].spot_direction[1] = -1.0f; // spot light direction in WC
	light[1].spot_direction[2] = 0.0f;
	light[1].spot_cutoff_angle = 10.0f;
	light[1].spot_exponent = 8.0f;


	//spot light 2 is in EC
	light[2].light_on = 1;
	light[2].position[0] = 700.0f; light[2].position[1] = 350.0f; // spot light position in WC
	light[2].position[2] = 0.0f; light[2].position[3] = 1.0f;

	light[2].ambient_color[0] = 0.152f; light[2].ambient_color[1] = 0.152f;
	light[2].ambient_color[2] = 0.152f; light[2].ambient_color[3] = 1.0f;

	light[2].diffuse_color[0] = 0.572f; light[2].diffuse_color[1] = 0.572f;
	light[2].diffuse_color[2] = 0.572f; light[2].diffuse_color[3] = 1.0f;

	light[2].specular_color[0] = 0.772f; light[2].specular_color[1] = 0.772f;
	light[2].specular_color[2] = 0.772f; light[2].specular_color[3] = 1.0f;

	glm::vec3 temp = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - glm::vec3(light[2].position[0], light[2].position[1], light[2].position[2]));
	light[2].spot_direction[0] = temp.x; light[2].spot_direction[1] = temp.y; // spot light direction in WC
	light[2].spot_direction[2] = temp.z;


	light[2].spot_cutoff_angle = 10.0f;
	light[2].spot_exponent = 8.0f;

	//spot light 3 is in MC
	light[3].light_on = 1;
	light[3].position[0] = 0.0f; light[3].position[1] = -1.5f; // spot light position in WC
	light[3].position[2] = 0.0f; light[3].position[3] = 1.0f;

	light[3].ambient_color[0] = 0.752f; light[3].ambient_color[1] = 0.152f;
	light[3].ambient_color[2] = 0.152f; light[3].ambient_color[3] = 1.0f;

	light[3].diffuse_color[0] = 0.972f; light[3].diffuse_color[1] = 0.572f;
	light[3].diffuse_color[2] = 0.572f; light[3].diffuse_color[3] = 1.0f;

	light[3].specular_color[0] = 0.972f; light[3].specular_color[1] = 0.772f;
	light[3].specular_color[2] = 0.772f; light[3].specular_color[3] = 1.0f;

	light[3].spot_direction[0] = 0.0f; light[3].spot_direction[1] = 0.607f; // spot light direction in WC
	light[3].spot_direction[2] = 0.807f;


	light[3].spot_cutoff_angle = 5.0f;
	light[3].spot_exponent = 1.0f;

	light[4] = light[3];
	light[5] = light[3];
	light[6] = light[3];

	light[4].position[0] = 0.0f, light[4].position[1] = 500.0f,
		light[4].position[2] = 400.0f, light[4].position[3] = 1.0f;
	light[4].spot_direction[0] = 0.0f, light[4].spot_direction[1] = -1.0f, light[4].spot_direction[2] = 0.0f;

	light[5].position[0] = 350.0f, light[5].position[1] = 500.0f,
		light[5].position[2] = -200.0f, light[5].position[3] = 1.0f;

	light[5].spot_direction[0] = 0.0f, light[5].spot_direction[1] = -1.0f, light[5].spot_direction[2] = 0.0f;

	light[6].position[0] = -350.0f, light[6].position[1] = 500.0f,
		light[6].position[2] = -200.0f, light[6].position[3] = 1.0f;

	light[6].spot_direction[0] = 0.0f, light[6].spot_direction[1] = -1.0f, light[6].spot_direction[2] = 0.0f;
	
	for (int i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; ++i)
		OriginLight[i] = light[i];
}

void set_up_special_light(void)
{
	for (int i = 4; i < 7; ++i)
	{
		glm::vec3 temp = glm::normalize(special_light_destination[i-4] - glm::vec3(light[i].position[0],light[i].position[1],light[i].position[2]));
		light[i].spot_direction[0] = temp.x;
		light[i].spot_direction[1] = temp.y;
		light[i].spot_direction[2] = temp.z;
	}
}

void set_up_shader(void)
{
	glUseProgram(h_ShaderProgram_TXPS);
	for (int kk = 0; kk < 2; ++kk)
	{
		if (kk == 1)
		{
			switch_shader(GOURAUD);
			glUseProgram(h_ShaderProgram_GS);
		}
		glUniform1i(loc_light[0].light_on, light[0].light_on);
		glUniform4fv(loc_light[0].position, 1, light[0].position);
		glUniform4fv(loc_light[0].ambient_color, 1, light[0].ambient_color);
		glUniform4fv(loc_light[0].diffuse_color, 1, light[0].diffuse_color);
		glUniform4fv(loc_light[0].specular_color, 1, light[0].specular_color);

		glUniform1i(loc_light[1].light_on, light[1].light_on);
		// need to supply position in EC for shading
		glm::vec4 position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
			light[1].position[2], light[1].position[3]);
		glUniform4fv(loc_light[1].position, 1, &position_EC[0]);
		glUniform4fv(loc_light[1].ambient_color, 1, light[1].ambient_color);
		glUniform4fv(loc_light[1].diffuse_color, 1, light[1].diffuse_color);
		glUniform4fv(loc_light[1].specular_color, 1, light[1].specular_color);
		// need to supply direction in EC for shading in this example shader
		// note that the viewing transform is a rigid body transform
		// thus transpose(inverse(mat3(ViewMatrix)) = mat3(ViewMatrix)
		
		glm::vec3 direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0], light[1].spot_direction[1],
			light[1].spot_direction[2]);
		
		glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]);
		glUniform1f(loc_light[1].spot_cutoff_angle, light[1].spot_cutoff_angle);
		glUniform1f(loc_light[1].spot_exponent, light[1].spot_exponent);


		//light 2
		glUniform1i(loc_light[2].light_on, light[2].light_on);
		// need to supply position in EC for shading
		if (EC_idx != 5)
		{
			position_EC = ViewMatrix * glm::vec4(light[2].position[0], light[2].position[1],
				light[2].position[2], light[2].position[3]);
			glUniform4fv(loc_light[2].position, 1, &position_EC[0]);
		}
		else
		{
			position_EC = fixedViewMatrix * glm::vec4(light[2].position[0], light[2].position[1],
				light[2].position[2], light[2].position[3]);
			glUniform4fv(loc_light[2].position, 1, &position_EC[0]);
		}
		glUniform4fv(loc_light[2].ambient_color, 1, light[2].ambient_color);
		glUniform4fv(loc_light[2].diffuse_color, 1, light[2].diffuse_color);
		glUniform4fv(loc_light[2].specular_color, 1, light[2].specular_color);
		// need to supply direction in EC for shading in this example shader
		// note that the viewing transform is a rigid body transform
		// thus transpose(inverse(mat3(ViewMatrix)) = mat3(ViewMatrix)
		if (EC_idx != 5)
		{
			direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[2].spot_direction[0], light[2].spot_direction[1],
				light[2].spot_direction[2]);
		}
		else
		{
			direction_EC = glm::mat3(fixedViewMatrix) * glm::vec3(light[2].spot_direction[0], light[2].spot_direction[1],
				light[2].spot_direction[2]);
		}
		glUniform3fv(loc_light[2].spot_direction, 1, &direction_EC[0]);
		glUniform1f(loc_light[2].spot_cutoff_angle, light[2].spot_cutoff_angle);
		glUniform1f(loc_light[2].spot_exponent, light[2].spot_exponent);


		//light 3
		glUniform1i(loc_light[3].light_on, light[3].light_on);
		// need to supply position in EC for shading
		
		position_EC = ModelViewMatrix * glm::vec4(light[3].position[0], light[3].position[1],
			light[3].position[2], light[3].position[3]);
		glUniform4fv(loc_light[3].position, 1, &position_EC[0]);
		
		glUniform4fv(loc_light[3].ambient_color, 1, light[3].ambient_color);
		glUniform4fv(loc_light[3].diffuse_color, 1, light[3].diffuse_color);
		glUniform4fv(loc_light[3].specular_color, 1, light[3].specular_color);
		// need to supply direction in EC for shading in this example shader
		// note that the viewing transform is a rigid body transform
		// thus transpose(inverse(mat3(ViewMatrix)) = mat3(ViewMatrix)
		
		direction_EC = glm::normalize(glm::mat3(ModelViewMatrixInvTrans) * glm::vec3(light[3].spot_direction[0], light[3].spot_direction[1],
			light[3].spot_direction[2]));
	
		
		glUniform3fv(loc_light[3].spot_direction, 1, &direction_EC[0]);
		glUniform1f(loc_light[3].spot_cutoff_angle, light[3].spot_cutoff_angle);
		glUniform1f(loc_light[3].spot_exponent, light[3].spot_exponent);

		for (int siba = 4; siba < 7; ++siba)
		{
			glUniform1i(loc_light[siba].light_on, light[siba].light_on);
			// need to supply position in EC for shading
			position_EC = ViewMatrix * glm::vec4(light[siba].position[0], light[siba].position[1],
				light[siba].position[2], light[siba].position[3]);
			glUniform4fv(loc_light[siba].position, 1, &position_EC[0]);
			glUniform4fv(loc_light[siba].ambient_color, 1, light[siba].ambient_color);
			glUniform4fv(loc_light[siba].diffuse_color, 1, light[siba].diffuse_color);
			glUniform4fv(loc_light[siba].specular_color, 1, light[siba].specular_color);
			// need to supply direction in EC for shading in this example shader
			// note that the viewing transform is a rigid body transform
			// thus transpose(inverse(mat3(ViewMatrix)) = mat3(ViewMatrix)

			direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[siba].spot_direction[0], light[siba].spot_direction[1],
				light[siba].spot_direction[2]);

			glUniform3fv(loc_light[siba].spot_direction, 1, &direction_EC[0]);
			glUniform1f(loc_light[siba].spot_cutoff_angle, light[siba].spot_cutoff_angle);
			glUniform1f(loc_light[siba].spot_exponent, light[siba].spot_exponent);
		}
	}
	//modify start
	switch_shader(PHONG);
	glUseProgram(0);
}

void set_up_special_light_shader(void)
{
	glUseProgram(h_ShaderProgram_TXPS);
	for (int kk = 0; kk < 2; ++kk)
	{
		if (kk == 1)
		{
			switch_shader(GOURAUD);
			glUseProgram(h_ShaderProgram_GS);
		}

		for (int i = 4; i < 7; ++i)
		{
			glUniform1i(loc_light[i].light_on, light[i].light_on);
			// need to supply position in EC for shading
			glm::vec4 position_EC = ViewMatrix * glm::vec4(light[i].position[0], light[i].position[1],
				light[i].position[2], light[i].position[3]);
			glUniform4fv(loc_light[i].position, 1, &position_EC[0]);
			glUniform4fv(loc_light[i].ambient_color, 1, light[i].ambient_color);
			glUniform4fv(loc_light[i].diffuse_color, 1, light[i].diffuse_color);
			glUniform4fv(loc_light[i].specular_color, 1, light[i].specular_color);
			// need to supply direction in EC for shading in this example shader
			// note that the viewing transform is a rigid body transform
			// thus transpose(inverse(mat3(ViewMatrix)) = mat3(ViewMatrix)

			glm::vec3 direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[i].spot_direction[0], light[i].spot_direction[1],
				light[i].spot_direction[2]);

			glUniform3fv(loc_light[i].spot_direction, 1, &direction_EC[0]);
			glUniform1f(loc_light[i].spot_cutoff_angle, light[i].spot_cutoff_angle);
			glUniform1f(loc_light[i].spot_exponent, light[i].spot_exponent);
		}
	}
	switch_shader(PHONG);
	glUseProgram(0);
}


void prepare_scene(void) {
	prepare_axes();
	prepare_floor();
	prepare_tiger();
	prepare_ben();
	prepare_wolf();
	prepare_spider();
	prepare_dragon();
	prepare_optimus();
	//prepare_cow();
	//prepare_bus();
	prepare_bike();
	prepare_godzilla();
	//prepare_ironman();
	prepare_tank();
}

void initialize_renderer(void) {
	initialize_camera();
	initialize_OpenGL();
	register_callbacks();
	prepare_shader_program();
	prepare_gouraud_shader();
	initialize_lights_and_material();
	set_up_scene_lights();
	set_up_special_light();
	set_up_shader();
	prepare_scene();
}

void initialize_glew(void) {
	GLenum error;

	glewExperimental = GL_TRUE;

	error = glewInit();
	if (error != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
		exit(-1);
	}
	fprintf(stdout, "*********************************************************\n");
	fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
	fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
	fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "*********************************************************\n\n");
}

void greetings(char *program_name, char messages[][256], int n_message_lines) {
	fprintf(stdout, "**************************************************************\n\n");
	fprintf(stdout, "  PROGRAM NAME: %s\n\n", program_name);
	fprintf(stdout, "    This program was coded for CSE4170 students\n");
	fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n**************************************************************\n\n");

	initialize_glew();
}

#define N_MESSAGE_LINES 1
void main(int argc, char *argv[]) {
	char program_name[64] = "Sogang CSE4170 3D Objects";
	char messages[N_MESSAGE_LINES][256] = { "    - Keys used: '0', '1', 'a', 't', 'f', 'c', 'd', 'y', 'u', 'i', 'o', 'ESC'"  };

	glutInit(&argc, argv);
  	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutInitWindowSize(800, 800);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
}