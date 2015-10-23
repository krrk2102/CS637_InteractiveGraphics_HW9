// HW9 part1 for CS 637
// Shangqi Wu

#include "Angel.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <limits>

using namespace std;

typedef vec4 color4;

// Height and width of main window. 
const int h = 500;
const int w = 500;

// 16 control points for each patch.
const int NumCtrlPts = 16;
// 2 points for one axis. 
const int NumAxisPts = 6;

// True for perspective projection, false for parallel projection. 
bool perspective = true;
// This mode specifies which control point is being modified currently. 
int mode = -1;

// sample_res is the sampling resolution, it is 10 default.
int sample_res = 10;
// Geometry matrix. 
mat4 geometryX;
mat4 geometryY;
mat4 geometryZ;
// This is the basis matrix specially defined for 
const mat4 Mbasis(
	1, 0, 0, 0,
	-3, 3, 0, 0,
	3, -6, 3, 0,
	-1, 3, -3, 1
	);

// RGBA color for background of main window.
float Red = 0;
float Green = 0;
float Blue = 0;
float Alpha = 1;

// Radius of camera rotation and its delta.
float cameraRadius = 10;
float dr = 0.1;
// Height of camera and its delta.
float cameraHeight = 3;
float dh = 0.5;
// Current position and its delta of a circling camera
float t = 0;
float dt = 0.005;

// Initial position of look-at, camera position, and up vector for projection.
vec4 at(0, 3, 0, 1);
vec4 eye(0, 3, 3, 1);
vec4 up(0, 1, 0, 1);

// Phong shading model parameters of light source 1, which rotates around the object.
float Idr1 = 0;
float Idg1 = 0.4;
float Idb1 = 0.4;
float Isr1 = 0.2;
float Isg1 = 0.2;
float Isb1 = 0.2;
float Iar1 = 0;
float Iag1 = 0.05;
float Iab1 = 0.05;

// Phong shading model parameters of light source 2, which moves with camera.
float Idr2 = 0.3;
float Idg2 = 0.3;
float Idb2 = 0;
float Isr2 = 0.2;
float Isg2 = 0.2;
float Isb2 = 0.2;
float Iar2 = 0.05;
float Iag2 = 0.05;
float Iab2 = 0;

// Shininess parameter for phong shading.
float shininess = 100;

// Phong shading model parameters of material property. 
float kdr = 1;
float kdg = 1;
float kdb = 1;
float ksr = 1;
float ksg = 1;
float ksb = 1;
float kar = 1;
float kag = 1;
float kab = 1;

// Position parameters of light source 2. 
float lightHeight = 10;
float lightRadius = 10;
float dhlight2 = 0.2;
float drlight2 = 0.2;
float tlight = 1;
float dtlight = 0.1;

// IDs for main window. 
int MainWindow;

// Vector for vertices.
vector<vec4> control_points;
vector<color4> control_points_color;
vector<vec4> render_vertices;
vector<vec4> render_normals;
vector<vec4> render_texcoord;
// Delta for each move of a control point. 
const float dct = 0.1;

// ID for shaders program.
GLuint basic, program;
const GLuint loc_ver_basic = 5, loc_col_basic = 6;

GLuint frameBuffer, colorBuffer, depthBuffer, texture;
GLint samples;

//--------------------------------------------------------------------------

vec4
product(const vec4 &a, const vec4 &b)
{
	return vec4(a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]);
}

//--------------------------------------------------------------------------

void
initVertices(void)
{
	// Vector containing vertices of every triangle to be drawn.
	vector<vec4> vertices;
	// Vector for average normal for corresponding vertice in a triangle. 
	vector<vec4> normals;
	// The vector to represent texture coordinates.
	vector<vec4> texcoord;
	render_vertices.clear();
	render_normals.clear();
	render_texcoord.clear();

	// Initialize geometry matrix. 
	for (int i = 0; i < NumCtrlPts; i++) {
		geometryX[i / 4][i % 4] = control_points[i].x;
		geometryY[i / 4][i % 4] = control_points[i].y;
		geometryZ[i / 4][i % 4] = control_points[i].z;
	}

	// Start sampling.
	float step = 1 / static_cast<float>(sample_res - 1); float positionS = 0; float positionT = 0;
	mat4 transposedMbasis = transpose(Mbasis);
	for (int i = 0; i < sample_res; i++) {
		for (int j = 0; j < sample_res; j++) {
			vec4 S(1, positionS, pow(positionS, 2), pow(positionS, 3));
			vec4 T(1, positionT, pow(positionT, 2), pow(positionT, 3));
			vec4 tmp = transposedMbasis * geometryX * Mbasis * S;
			float x = T.x*tmp.x + T.y*tmp.y + T.z*tmp.z + T.w*tmp.w;
			tmp = transposedMbasis * geometryY * Mbasis * S;
			float y = T.x*tmp.x + T.y*tmp.y + T.z*tmp.z + T.w*tmp.w;
			tmp = transposedMbasis * geometryZ * Mbasis * S;
			float z = T.x*tmp.x + T.y*tmp.y + T.z*tmp.z + T.w*tmp.w;
			vertices.push_back(vec4(x, y, z, 1));
			texcoord.push_back(vec4(positionS, positionT, 0, 1));
			positionS += step;
		}
		positionS = 0;
		positionT += step;
	}

	// Initialize zero normal for each sample point. 
	normals.resize(vertices.size());
	for (int i = 0; i < static_cast<int>(normals.size()); i++)
		normals[i] = vec4(0, 0, 0, 0);

	// Divide small patches for triangles to be rendered. 
	vector<int> triangles;
	for (int i = 0; i < sample_res - 1; i++)
		for (int j = 0; j < sample_res - 1; j++) {
			triangles.push_back(sample_res*(i + 1) + j);
			triangles.push_back(sample_res*i + j);
			triangles.push_back(sample_res*i + j + 1);
			triangles.push_back(sample_res*(i + 1) + j);
			triangles.push_back(sample_res*i + j + 1);
			triangles.push_back(sample_res*(i + 1) + j + 1);
		}

	// Calculate average normal for each sample point. 
	for (int i = 0; i < static_cast<int>(triangles.size()); i += 3) {
		vec4 tmpnorm = normalize(vec4(cross((vertices[triangles[i + 1]] - vertices[triangles[i]]), (vertices[triangles[i + 2]] - vertices[triangles[i]]))));
		normals[triangles[i]] = normalize(normals[triangles[i]] + tmpnorm);
		normals[triangles[i]].w = 1;
		normals[triangles[i + 1]] = normalize(normals[triangles[i + 1]] + tmpnorm);
		normals[triangles[i + 1]].w = 1;
		normals[triangles[i + 2]] = normalize(normals[triangles[i + 2]] + tmpnorm);
		normals[triangles[i + 2]].w = 1;
	}

	// Reorder vertices and normals for triangle rendering. 
	for (int i = 0; i < static_cast<int>(triangles.size()); i++) {
		render_vertices.push_back(vertices[triangles[i]]);
		render_normals.push_back(normals[triangles[i]]);
		render_texcoord.push_back(texcoord[triangles[i]]);
	}
}

//--------------------------------------------------------------------------

void
initBuffers(void)
{
	// FBO for rendering. 
	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

	glGetIntegerv(GL_MAX_SAMPLES, &samples);

	glGenRenderbuffers(1, &colorBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, colorBuffer);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, w, h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBuffer);

	glGenRenderbuffers(1, &depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, w, h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		cerr << "FBO incomplete." << endl;
		exit(1);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Texture pixel data. 
	GLubyte* pixelData = new GLubyte[4 * w * h];
	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++) {
			pixelData[i * h * 4 + j * 4 + 3] = 255;
			if (j % 16 < 8) {
				pixelData[i * h * 4 + j * 4] = 0;
				pixelData[i * h * 4 + j * 4 + 1] = 0;
				pixelData[i * h * 4 + j * 4 + 2] = 180;
			}
			else{
				pixelData[i * h * 4 + j * 4] = 255;
				pixelData[i * h * 4 + j * 4 + 1] = 255;
				pixelData[i * h * 4 + j * 4 + 2] = 255;
			}
		}

	// Set up texture property. 
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
	glGenerateMipmap(GL_TEXTURE_2D);

	delete pixelData;
	pixelData = NULL;

	// Create a vertex array object.
	GLuint vao[1];
	glGenVertexArrays(1, vao);
	glBindVertexArray(vao[0]);
	cout << "glGenVertexArrays(), glBindVertexArray() for main window initialization." << endl;

	// Create and initialize a buffer object.
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	cout << "glGenBuffer(), glBindBuffer() for main window initialization." << endl;
	glBufferData(GL_ARRAY_BUFFER, render_vertices.size()*sizeof(vec4) + render_normals.size()*sizeof(vec4) + render_texcoord.size()*sizeof(vec4) + control_points.size()*sizeof(vec4) + control_points_color.size()*sizeof(color4), NULL, GL_STATIC_DRAW);
	cout << "glBufferData(), glBufferSubData() for main window initialization." << endl;

	// Load shaders and use the resulting shader program.
	program = InitShader("vshader_texture.glsl", "fshader_texture.glsl");
	LinkShader(program);
	cout << "InitShader(), glUseProgram() for main window initialization." << endl;

	// Initialize the vertex position attribute from the vertex shader.
	GLuint loc_ver = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(loc_ver);
	glVertexAttribPointer(loc_ver, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	// Pass normal vectors of each triangle to vertex shader
	GLuint loc_col = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(loc_col);
	glVertexAttribPointer(loc_col, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(render_vertices.size()*sizeof(vec4)));
	cout << "glEnableVertexAttribArray(), glVertexAttribPointer() for main window initialization." << endl;

	GLuint loc_tex = glGetAttribLocation(program, "TexCoord");
	glEnableVertexAttribArray(loc_tex);
	glVertexAttribPointer(loc_tex, 4, GL_FLOAT, GL_FALSE, 0, 
		BUFFER_OFFSET(render_vertices.size()*sizeof(vec4) + render_normals.size()*sizeof(vec4)));

	// Load shader program for control points and axis. 
	basic = InitShader("vshaderbasic.glsl", "fshaderbasic.glsl");
	glBindAttribLocation(basic, loc_ver_basic, "vVertex");
	glBindAttribLocation(basic, loc_col_basic, "vColor");
	LinkShader(basic);

	glEnableVertexAttribArray(loc_ver_basic);
	glVertexAttribPointer(loc_ver_basic, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(render_vertices.size()*sizeof(vec4) + render_normals.size()*sizeof(vec4) + render_texcoord.size()*sizeof(vec4)));

	glEnableVertexAttribArray(loc_col_basic);
	glVertexAttribPointer(loc_col_basic, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(render_vertices.size()*sizeof(vec4) + render_normals.size()*sizeof(vec4) + render_texcoord.size()*sizeof(vec4) + control_points.size()*sizeof(vec4)));
}

//----------------------------------------------------------------------------

void
initData(void)
{
	// Pass vertices & normals data to opengl buffer object.
	glBufferSubData(GL_ARRAY_BUFFER, 0, render_vertices.size()*sizeof(vec4), render_vertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, render_vertices.size()*sizeof(vec4), render_normals.size()*sizeof(vec4), render_normals.data());
	glBufferSubData(GL_ARRAY_BUFFER, render_vertices.size()*sizeof(vec4) + render_normals.size()*sizeof(vec4), render_texcoord.size()*sizeof(vec4), render_texcoord.data());
	glBufferSubData(GL_ARRAY_BUFFER, render_vertices.size()*sizeof(vec4) + render_normals.size()*sizeof(vec4) + render_texcoord.size()*sizeof(vec4), control_points.size()*sizeof(vec4), control_points.data());
	glBufferSubData(GL_ARRAY_BUFFER, render_vertices.size()*sizeof(vec4) + render_normals.size()*sizeof(vec4) + render_texcoord.size()*sizeof(vec4) + control_points.size()*sizeof(vec4), control_points_color.size()*sizeof(color4), control_points_color.data());
}

//----------------------------------------------------------------------------

void
recal(void)
{
	// Calculate renewed camera position. 
	eye = vec4(cameraRadius*sin(t), cameraHeight, cameraRadius*cos(t), 1);

	// Light 1 is within camera coordinate.
	vec4 light1_pos = vec4(0, 0, 10, 1);
	color4 light1_diffuse = color4(Idr1, Idg1, Idb1, 1);
	color4 light1_specular = color4(Isr1, Isg1, Isb1, 1);
	color4 light1_ambient = color4(Iar1, Iag1, Iab1, 1);

	// Light 2 is within world coordinate.
	vec4 light2_pos = vec4(lightRadius*sin(tlight), lightHeight, lightRadius*cos(tlight), 1);
	color4 light2_diffuse = color4(Idr2, Idg2, Idb2, 1);
	color4 light2_specular = color4(Isr2, Isg2, Isb2, 1);
	color4 light2_ambient = color4(Iar2, Iag2, Iab2, 1);

	// Material property.
	color4 material_diffuse(kdr, kdg, kdb, 1);
	color4 material_ambient(kar, kag, kab, 1);
	color4 material_specular(ksr, ksg, ksb, 1);

	// Create model and projection matrix.
	mat4 modelview;
	mat4 projection;

	// Implementing projection.
	if (perspective) projection *= Perspective(90, 1, 1, 1e10);
	else projection *= Ortho(-10, 10, -10, 10, -100, 100);

	// Implementing modelview. 
	modelview *= LookAt(eye, at, up);

	// Pass modelview and projection matrix to shader for control points and axis.
	glUseProgram(basic);
	GLint loc_modelview = glGetUniformLocation(basic, "modelview");
	glUniformMatrix4fv(loc_modelview, 1, GL_TRUE, modelview);
	GLint loc_projection = glGetUniformLocation(basic, "projection");
	glUniformMatrix4fv(loc_projection, 1, GL_TRUE, projection);

	glUseProgram(program);
	// Pass modelview and projection matrix to vertex shader. 
	loc_modelview = glGetUniformLocation(program, "modelview");
	glUniformMatrix4fv(loc_modelview, 1, GL_TRUE, modelview);
	loc_projection = glGetUniformLocation(program, "projection");
	glUniformMatrix4fv(loc_projection, 1, GL_TRUE, projection);
	GLint loc_eyeposition = glGetUniformLocation(program, "eyeposition");
	glUniform4f(loc_eyeposition, eye.x, eye.y, eye.z, eye.w);

	// Pass positions of light sources to vertex shader.
	GLint loc_light1_pos = glGetUniformLocation(program, "light1_pos");
	GLint loc_light2_pos = glGetUniformLocation(program, "light2_pos");
	glUniform4f(loc_light1_pos, light1_pos.x, light1_pos.y, light1_pos.z, light1_pos.w);
	glUniform4f(loc_light2_pos, light2_pos.x, light2_pos.y, light2_pos.z, light2_pos.w);

	// Calculate and pass color products of each light source to vertex shader.
	vec4 d_pro1 = product(light1_diffuse, material_diffuse);
	vec4 d_pro2 = product(light2_diffuse, material_diffuse);
	vec4 a_pro1 = product(light1_ambient, material_ambient);
	vec4 a_pro2 = product(light2_ambient, material_ambient);
	vec4 s_pro1 = product(light1_specular, material_specular);
	vec4 s_pro2 = product(light2_specular, material_specular);
	GLint loc_diffuse_product1 = glGetUniformLocation(program, "light1_diffuse_product");
	GLint loc_diffuse_product2 = glGetUniformLocation(program, "light2_diffuse_product");
	GLint loc_specular_product1 = glGetUniformLocation(program, "light1_specular_product");
	GLint loc_specular_product2 = glGetUniformLocation(program, "light2_specular_product");
	GLint loc_ambient_product1 = glGetUniformLocation(program, "light1_ambient_product");
	GLint loc_ambient_product2 = glGetUniformLocation(program, "light2_ambient_product");
	glUniform4f(loc_diffuse_product1, d_pro1.x, d_pro1.y, d_pro1.z, d_pro1.w);
	glUniform4f(loc_diffuse_product2, d_pro2.x, d_pro2.y, d_pro2.z, d_pro2.w);
	glUniform4f(loc_specular_product1, s_pro1.x, s_pro1.y, s_pro1.z, s_pro1.w);
	glUniform4f(loc_specular_product2, s_pro2.x, s_pro2.y, s_pro2.z, s_pro2.w);
	glUniform4f(loc_ambient_product1, a_pro1.x, a_pro1.y, a_pro1.z, a_pro1.w);
	glUniform4f(loc_ambient_product2, a_pro2.x, a_pro2.y, a_pro2.z, a_pro2.w);

	GLint loc_shininess = glGetUniformLocation(program, "shininess");
	glUniform1f(loc_shininess, shininess);
	cout << "glGetUniformLocation(), glUniformMatrix4fv() for transformation matrix." << endl;
}

//----------------------------------------------------------------------------

void
display(void)
{
	recal(); // Calculates vertices & colors for objects in main window. 

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer);
	glClearColor(Red, Green, Blue, Alpha); // Set background color of main window.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear main window.
	glViewport(0, 0, w, h);

	glUseProgram(program);
	glActiveTexture(GL_TEXTURE0);
	// Pass texture to shaders. 
	GLuint loc_texutre = glGetUniformLocation(program, "texture");
	glUniform1i(loc_texutre, 0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glDrawArrays(GL_TRIANGLES, 0, render_vertices.size()); // Draw the points by one triangle.
	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(basic);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glDrawArrays(GL_POINTS, 0, NumCtrlPts); // Draw control points.
	glDrawArrays(GL_LINES, NumCtrlPts, NumAxisPts); // Draw xyz axis. 

	glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBlitFramebuffer(0, 0, w - 1, h - 1, 0, 0, w - 1, h - 1, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glutSwapBuffers(); // Double buffer swapping. 
	glFlush(); // Flush. 
	cout << "glClearColor(), glClear(), glDrawArrays(), glutSwapBuffers(), glFlush() for main window display function." << endl;
}

//----------------------------------------------------------------------------

void
RotationFunc(void)
{
	t += dt; // Camera rotation animation.
	glutPostRedisplay(); // Redisplay function.
}

//----------------------------------------------------------------------------

void
SetColor(int num = -1)
{
	// Default color for control points are cyan. 
	for (int i = 0; i < NumCtrlPts; i++)
		control_points_color[i] = color4(0, 1, 1, 1);
	// Selected point will be red. 
	if (num != -1) control_points_color[num] = color4(1, 0, 0, 1);
	glBufferSubData(GL_ARRAY_BUFFER, render_vertices.size()*sizeof(vec4) + render_normals.size()*sizeof(vec4) + render_texcoord.size()*sizeof(vec4) + control_points.size()*sizeof(vec4), control_points_color.size()*sizeof(color4), control_points_color.data());
}

//----------------------------------------------------------------------------

void
keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 033: exit(EXIT_SUCCESS); break; // "Esc": exit the program.
	case (int)'w': cameraHeight += dh; break; // Increasing camera height.
	case (int)'s': cameraHeight -= dh; break; // Decreasing camera height.
	case (int)'a': cameraRadius += dr; break;// Incresing camera radius, the object looks smaller under perspective projection.
	case (int)'d': if (cameraRadius > dr) cameraRadius -= dr; break; // Decreasing camera radius, the object looks larger under perspective projection.
	case (int)'e': dt += 0.0025; break; // Double camera rotation speed.
	case (int)'q': dt -= 0.0025; break; // Half camera rotation speed.
	case (int)'x': t += dt; break; // Allows you rotate camera by one step. 
	case (int)'z': t -= dt; break;
	case (int)'t': lightHeight += dhlight2; break; // Increasing light height.
	case (int)'g': lightHeight -= dhlight2; break; // Decreasing light height.
	case (int)'h': lightRadius += drlight2; break; // Increasing light orbit radius, the light source becomes farther to the object.
	case (int)'f': lightRadius -= drlight2; break; // Decreasing light orbit radius, the loght source becomes closer to the object. 
	case (int)'y': tlight += dtlight; break; // Rotate light 2 counter-clockwise.
	case (int)'r': tlight -= dtlight; break; // Rotate light 2 clockwise.
	case (int)'v': dtlight *= 2; break; // Make light 2 rotate 2x faster.
	case (int)'c': dtlight /= 2; break; // Make light 2 rotate half speed.
	case (int)'0': mode = 0; SetColor(mode); break; // Pick control point 0.
	case (int)'1': mode = 1; SetColor(mode); break; // Pick control point 1.
	case (int)'2': mode = 2; SetColor(mode); break; // So on so forth...
	case (int)'3': mode = 3; SetColor(mode); break;
	case (int)'4': mode = 4; SetColor(mode); break;
	case (int)'5': mode = 5; SetColor(mode); break;
	case (int)'6': mode = 6; SetColor(mode); break;
	case (int)'7': mode = 7; SetColor(mode); break;
	case (int)'8': mode = 8; SetColor(mode); break;
	case (int)'9': mode = 9; SetColor(mode); break;
	case (int)'!': mode = 10; SetColor(mode); break;
	case (int)'@': mode = 11; SetColor(mode); break;
	case (int)'#': mode = 12; SetColor(mode); break;
	case (int)'$': mode = 13; SetColor(mode); break;
	case (int)'%': mode = 14; SetColor(mode); break;
	case (int)'^': mode = 15; SetColor(mode); break;
	case (int)'&': mode = -1; SetColor(); break; // Stop picking any control point. 
	case (int)'i': if (mode > -1 && mode < NumCtrlPts) { control_points[mode].y += dct; initVertices(); initData(); } break; // Increase y of current control points. 
	case (int)'k': if (mode > -1 && mode < NumCtrlPts) { control_points[mode].y -= dct; initVertices(); initData(); } break; // Decrease y of current control points. 
	case (int)'l': if (mode > -1 && mode < NumCtrlPts) { control_points[mode].x += dct; initVertices(); initData(); } break; // Increase x of current control points. 
	case (int)'j': if (mode > -1 && mode < NumCtrlPts) { control_points[mode].x -= dct; initVertices(); initData(); } break; // Decrease x of current control points. 
	case (int)'o': if (mode > -1 && mode < NumCtrlPts) { control_points[mode].z += dct; initVertices(); initData(); } break; // Increase z of current control points. 
	case (int)'u': if (mode > -1 && mode < NumCtrlPts) { control_points[mode].z -= dct; initVertices(); initData(); } break; // Decrease z of current control points. 
	case (int)'m': sample_res++; initVertices(); initBuffers(); initData(); break; // Increase sampling resolution by 1.
	case (int)'n': if (sample_res > 2) { sample_res--; initVertices(); initBuffers(); initData(); } break; // Decrease sampling resolution by 1, until there are only 2 points in each direction. 
	}
	glutPostRedisplay();
	cout << "glutPostRedisplay() for keyboard function." << endl;
}

//----------------------------------------------------------------------------

void
MainSubMenuRotation(int id)
{
	switch (id) {
	case 1: glutIdleFunc(RotationFunc); break; // Start or stop camera rotation.
	case 2: glutIdleFunc(NULL); break; // Start or stop light rotation.
	}
	glutPostRedisplay();
	cout << "glutPostRedisplay() for idle rotation." << endl;
}

//----------------------------------------------------------------------------
void
MainSubMenuMaterial(int id)
{
	switch (id) {
	case 1: // White plastic. (Default)
		kdr = 1; kdg = 1; kdb = 1;
		ksr = 1; ksg = 1; ksb = 1;
		kar = 1; kag = 1; kab = 1;
		shininess = 100;
		break;
	case 2: // Gold.
		kdr = 1; kdg = 215.0 / 255.0; kdb = 0;
		ksr = 5; ksg = 5; ksb = 5;
		kar = 0.5; kag = 0.5*215.0 / 255.0; kab = 0.5;
		shininess = 5000;
		break;
	case 3: // Silver.
		kdr = 233.0 / 255.0; kdg = 233.0 / 255.0; kdb = 216.0 / 255.0;
		ksr = 0.1; ksg = 0.1; ksb = 0.1;
		kar = 233.0 / 255.0; kag = 233.0 / 255.0; kab = 233.0 / 255.0;
		shininess = 10;
		break;
	}
	glutPostRedisplay();
	cout << "glutPostRedisplay() for material change." << endl;
}

//----------------------------------------------------------------------------

void
MainSubMenuPerspective(int id)
{
	switch (id) {
	case 1: perspective = true; break; // Switch to persepctive projection.
	case 2: perspective = false; break; // Switch to parallel projection.
	}
	glutPostRedisplay();
	cout << "glutPostRedisplay() for projection mode changing." << endl;
}

//----------------------------------------------------------------------------
void
MainSubMenuLight(int id)
{
	switch (id) {
	case 1: // Light 1: blue, moving with camera; light 2: yellow, rotating around object.
		Idr1 = 0; Idg1 = 0.4; Idb1 = 0.4;
		Isr1 = 0.2; Isg1 = 0.2; Isb1 = 0.2;
		Iar1 = 0; Iag1 = 0.05; Iab1 = 0.05;
		Idr2 = 0.3; Idg2 = 0.3; Idb2 = 0;
		Isr2 = 0.2; Isg2 = 0.2; Isb2 = 0.2;
		Iar2 = 0.05; Iag2 = 0.05; Iab2 = 0;
		break;
	case 2: // Both light change to white.
		Idr1 = 0.4; Idg1 = 0.4; Idb1 = 0.4;
		Isr1 = 0.2; Isg1 = 0.2; Isb1 = 0.2;
		Iar1 = 0.05; Iag1 = 0.05; Iab1 = 0.05;
		Idr2 = 0.3; Idg2 = 0.3; Idb2 = 0.3;
		Isr2 = 0.2; Isg2 = 0.2; Isb2 = 0.2;
		Iar2 = 0.05; Iag2 = 0.05; Iab2 = 0.05;
		break;
	case 3: // Light 1 only.
		Idr1 = 0; Idg1 = 0.4; Idb1 = 0.4;
		Isr1 = 0.2; Isg1 = 0.2; Isb1 = 0.2;
		Iar1 = 0; Iag1 = 0.05; Iab1 = 0.05;
		Idr2 = 0; Idg2 = 0; Idb2 = 0;
		Isr2 = 0; Isg2 = 0; Isb2 = 0;
		Iar2 = 0; Iag2 = 0; Iab2 = 0;
		break;
	case 4: // Light 2 only.
		Idr1 = 0; Idg1 = 0; Idb1 = 0;
		Isr1 = 0; Isg1 = 0; Isb1 = 0;
		Iar1 = 0; Iag1 = 0; Iab1 = 0;
		Idr2 = 0.3; Idg2 = 0.3; Idb2 = 0;
		Isr2 = 0.2; Isg2 = 0.2; Isb2 = 0.2;
		Iar2 = 0.05; Iag2 = 0.05; Iab2 = 0;
		break;
	}
	glutPostRedisplay();
}

//----------------------------------------------------------------------------

void
setMainWinMenu(void)
{
	int submenu_id_r, submenu_id_p, submenu_id_m, submenu_id_l;
	// Create submenu for rotating animation.
	submenu_id_r = glutCreateMenu(MainSubMenuRotation);
	glutAddMenuEntry("Start Camera Rotation", 1);
	glutAddMenuEntry("Stop Camera Rotation", 2);

	// Create submenu for projection changing.
	submenu_id_p = glutCreateMenu(MainSubMenuPerspective);
	glutAddMenuEntry("Perspective Projection", 1);
	glutAddMenuEntry("Parallel Projection", 2);

	// Create submenu for selecting material property. 
	submenu_id_m = glutCreateMenu(MainSubMenuMaterial);
	glutAddMenuEntry("White Plastic", 1);
	glutAddMenuEntry("Gold", 2);
	glutAddMenuEntry("Silver", 3);

	// Create submenu for selecting light property. 
	submenu_id_l = glutCreateMenu(MainSubMenuLight);
	glutAddMenuEntry("Color Light", 1);
	glutAddMenuEntry("White Light", 2);
	glutAddMenuEntry("Light 1 Only", 3);
	glutAddMenuEntry("Light 2 Only", 4);

	glutCreateMenu(NULL); // Set menu in main window. 
	cout << "glutCreateMenu() for main window menu." << endl;
	glutAddSubMenu("Camera Rotation", submenu_id_r);
	glutAddSubMenu("Projection", submenu_id_p);
	glutAddSubMenu("Material", submenu_id_m);
	glutAddSubMenu("Light Color", submenu_id_l);
	cout << "glutAddMenuEntry() for main window menu." << endl;
	glutAttachMenu(GLUT_RIGHT_BUTTON);
	cout << "glutAttachMenu() for main window menu." << endl;
}

//----------------------------------------------------------------------------

void
readfile(void)
{
	// Start to read file.
	ifstream infile("./patchPoints.txt");

	if (!infile.is_open()) {
		cout << "Control point file doesn't exist, program will terminate." << endl;
		exit(0);
	}

	float x, y, z;

	// Read file content.
	while (infile) {
		if (control_points.size() == NumCtrlPts) break;
		infile >> x >> y >> z;
		control_points.push_back(vec4(x, y, z, 1));
	}
	infile.close();

	// Preset default color for control points. 
	control_points_color.resize(control_points.size());
	for (int i = 0; i < NumCtrlPts; i++) {
		control_points_color[i] = color4(0, 1, 1, 1);
	}

	// Add ending points of xyz axis. 
	control_points.push_back(vec4(0, 0, 0, 1));
	control_points.push_back(vec4(10, 0, 0, 1));
	control_points.push_back(vec4(0, 0, 0, 1));
	control_points.push_back(vec4(0, 10, 0, 1));
	control_points.push_back(vec4(0, 0, 0, 1));
	control_points.push_back(vec4(0, 0, 10, 1));

	// Default color for xyz axis.
	for (int i = 0; i < NumAxisPts; i++)
		control_points_color.push_back(color4(0.25, 0.25, 0.25, 1));
}

//----------------------------------------------------------------------------

int
main(int argc, char **argv)
{
	readfile(); // Read input control points file.

	glutInit(&argc, argv); // Initializing environment.
	cout << "glutInit(&argc,argv) called." << endl;
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // Enable depth.
	cout << "glutInitDisplayMode() called." << endl;
	glutInitWindowSize(w, h);
	cout << "glutInitWindowSize() called." << endl;
	glutInitWindowPosition(50, 50);
	cout << "glutInitWindowPosition() called." << endl;

	MainWindow = glutCreateWindow("ICG_hw9_part1"); // Initializing & setting main window.
	cout << "glutCreateWindow() for main window." << endl;
	glewExperimental = GL_TRUE;
	glewInit();
	cout << "glewInit() for main window." << endl;
	initVertices(); // Calculate coordinates for vertices. 
	initBuffers(); // Create buffers.
	initData(); // Passing vertice information to shader programs. 
	glutDisplayFunc(display); // Setting display function for main window.
	cout << "glutDisplayFunc() for main window." << endl;
	glutKeyboardFunc(keyboard); // Setting keyboard function for main window.
	cout << "glutKeyboardFunc() for main window." << endl;
	setMainWinMenu(); // Setting menu for main window. 
	glutIdleFunc(RotationFunc); // Start animation by default.
	cout << "glutIdleFunc() for main window." << endl;

	glPointSize(2.0);
	glLineWidth(2.0);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	cout << "glEnable( GL_DEPTH_TEST ) called." << endl;
	cout << "glutMainLoop() called." << endl;

	glutMainLoop(); // Start main loop. 
	return 0;
}


