// HW9 part2 for CS 637
// Shangqi Wu

#include "Angel.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

typedef vec4 color4;

// Height and width of main window. 
const int h = 500;
const int w = 500;

// True for perspective projection, false for parallel projection. 
bool perspective = true;
// Ture for  marble-like 3D procedural texture, false for normal white plastic look. 
bool texture = true;

// RGBA color for background of main window.
float Red = 0;
float Green = 0;
float Blue = 0;
float Alpha = 1;

// Radius of camera rotation and its delta.
float cameraRadius = 1;
float dr = 0.05;
// Height of camera and its delta.
float cameraHeight = 0;
float dh = 0.05;
// Current position and its delta of a circling camera
float t = 0;
float dt = 0.05;

// Initial position of look-at, camera position, and up vector for projection.
vec4 at(0, 0, 0, 1);
vec4 eye(0, 0, 0, 1);
vec4 up(0, 1, 0, 1);

// Phong shading model parameters of light source 1, which rotates around the object.
float Idr1 = 0.4;
float Idg1 = 0.4;
float Idb1 = 0.4;
float Isr1 = 0.2;
float Isg1 = 0.2;
float Isb1 = 0.2;
float Iar1 = 0.05;
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
float shininess = 2000;

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
float lightHeight = 1;
float lightRadius = 1;
float dhlight2 = 0.2;
float drlight2 = 0.2;
float tlight = 1;
float dtlight = 0.05;

// IDs for main window. 
int MainWindow;

// Vector for vertices.
vector<vec4> original_vertices;
// Vector containing vertices of every triangle to be drawn.
vector<vec4> vertices;
// Vector for average normal vectors of every vertice. 
vector<vec4> original_normals;
// Vector for average normal for corresponding vertice in a triangle. 
vector<vec4> normals;

// ID for shaders program.
GLuint program;
// IDs for FBO components.
GLuint frameBuffer, colorBuffer, depthBuffer;
// Number of max MSAA. 
GLint samples;

//--------------------------------------------------------------------------

vec4
product(const vec4 &a, const vec4 &b)
{
	return vec4(a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]);
}

//--------------------------------------------------------------------------

void
init(void)
{
	// Generating FBO for rendering.
	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

	// Get max factor of multisample. 
	glGetIntegerv(GL_MAX_SAMPLES, &samples);

	// Generate renderbuffer for color. 
	glGenRenderbuffers(1, &colorBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, colorBuffer);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, w, h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBuffer);

	// Generate renderbuffer for depth. 
	glGenRenderbuffers(1, &depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, w, h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

	// Check if FBO is ready for render. 
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		cerr << "FBO incomplete." << endl;
		exit(1);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(vec4) + normals.size()*sizeof(vec4), NULL, GL_STATIC_DRAW);
	// Pass vertices & normals data to opengl buffer object.
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size()*sizeof(vec4), vertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertices.size()*sizeof(vec4), normals.size()*sizeof(vec4), normals.data());
	cout << "glBufferData(), glBufferSubData() for main window initialization." << endl;

	// Load shaders and use the resulting shader program.
	program = InitShader("vshader_texture_3d.glsl", "fshader_texture_3d.glsl");
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
		BUFFER_OFFSET(vertices.size()*sizeof(vec4)));
	cout << "glEnableVertexAttribArray(), glVertexAttribPointer() for main window initialization." << endl;
}

//----------------------------------------------------------------------------

void
recal(void)
{
	// Calculate renewed camera position. 
	eye = vec4(cameraRadius*sin(t), cameraHeight, cameraRadius*cos(t), 1);

	// Light 1 is within camera coordinate.
	vec4 light1_pos = vec4(0, 0, 1, 1);
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
	if (perspective) projection *= Perspective(90, 1, 1e-10, 1e10);
	else projection *= Ortho(-1, 1, -1, 1, -100, 100);

	// Implementing modelview. 
	modelview *= LookAt(eye, at, up);

	glUseProgram(program);
	// Pass model and projection matrix to vertex shader. 
	GLint loc_modelview = glGetUniformLocation(program, "modelview");
	glUniformMatrix4fv(loc_modelview, 1, GL_TRUE, modelview);
	GLint loc_projection = glGetUniformLocation(program, "projection");
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

        GLint loc_texture = glGetUniformLocation(program, "texture");
        glUniform1i(loc_texture, texture);
	cout << "glGetUniformLocation(), glUniformMatrix4fv() for transformation matrix." << endl;
}

//----------------------------------------------------------------------------

void
display(void)
{
	recal(); // Calculates vertices & colors for objects in main window. 

	// Bind FBO and render. 
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer);
	glClearColor(Red, Green, Blue, Alpha); // Set background color of main window.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear main window.
	glViewport(0, 0, w, h);
	glUseProgram(program);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glDrawArrays(GL_TRIANGLES, 0, vertices.size()); // Draw the points by one triangle.
	glBindTexture(GL_TEXTURE_2D, 0);

	// Finish rendering and copy rendered image to screen. 
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBuffer);
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
keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 033: exit(EXIT_SUCCESS); break; // "Esc": exit the program.
	case (int)'w': cameraHeight += dh; break; // Increasing camera height.
	case (int)'s': cameraHeight -= dh; break; // Decreasing camera height.
	case (int)'a': cameraRadius += dr; break;// Incresing camera radius, the object looks smaller under perspective projection.
	case (int)'d': if (cameraRadius > dr) cameraRadius -= dr; break; // Decreasing camera radius, the object looks larger under perspective projection.
	case (int)'e': dt += 0.025; break; // Double camera rotation speed.
	case (int)'q': dt -= 0.025; break; // Half camera rotation speed.
	case (int)'t': lightHeight += dhlight2; break; // Increasing light height.
	case (int)'g': lightHeight -= dhlight2; break; // Decreasing light height.
	case (int)'h': lightRadius += drlight2; break; // Increasing light orbit radius, the light source becomes farther to the object.
	case (int)'f': lightRadius -= drlight2; break; // Decreasing light orbit radius, the loght source becomes closer to the object. 
	case (int)'y': tlight += dtlight; break; // Rotate light clockwise.
	case (int)'r': tlight -= dtlight; break; // Rotate light counter-clockwise. 
	case (int)'v': dtlight *= 2; break;
	case (int)'c': dtlight /= 2; break;
	case (int)'u': dhlight2 *= 2; break;
	case (int)'j': dhlight2 /= 2; break;
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
	case 1: // Marble look, 3D Texture. (Default)
		kdr = 1; kdg = 1; kdb = 1;
		ksr = 1; ksg = 1; ksb = 1;
		kar = 1; kag = 1; kab = 1;
		shininess = 2000;
		texture = true;
		break;
	case 2: // White plastic. 
		kdr = 1; kdg = 1; kdb = 1;
		ksr = 1; ksg = 1; ksb = 1;
		kar = 1; kag = 1; kab = 1;
		shininess = 100;
		texture = false;
		break;
	case 3: // Gold.
		kdr = 1; kdg = 215.0 / 255.0; kdb = 0;
		ksr = 5; ksg = 5; ksb = 5;
		kar = 0.5; kag = 0.5*215.0 / 255.0; kab = 0.5;
		shininess = 5000;
		texture = false;
		break;
	case 4: // Silver.
		kdr = 233.0 / 255.0; kdg = 233.0 / 255.0; kdb = 216.0 / 255.0;
		ksr = 0.1; ksg = 0.1; ksb = 0.1;
		kar = 233.0 / 255.0; kag = 233.0 / 255.0; kab = 233.0 / 255.0;
		shininess = 10;
		texture = false;
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
	case 1: // Both light set to white.
		Idr1 = 0.4; Idg1 = 0.4; Idb1 = 0.4;
		Isr1 = 0.2; Isg1 = 0.2; Isb1 = 0.2;
		Iar1 = 0.05; Iag1 = 0.05; Iab1 = 0.05;
		Idr2 = 0.3; Idg2 = 0.3; Idb2 = 0.3;
		Isr2 = 0.2; Isg2 = 0.2; Isb2 = 0.2;
		Iar2 = 0.05; Iag2 = 0.05; Iab2 = 0.05;
		break;
	case 2: // Light 1: blue, moving with camera; light 2: yellow, rotating around object.
		Idr1 = 0.0; Idg1 = 0.4; Idb1 = 0.4;
		Isr1 = 0.2; Isg1 = 0.2; Isb1 = 0.2;
		Iar1 = 0.0; Iag1 = 0.05; Iab1 = 0.05;
		Idr2 = 0.3; Idg2 = 0.3; Idb2 = 0;
		Isr2 = 0.2; Isg2 = 0.2; Isb2 = 0.2;
		Iar2 = 0.05; Iag2 = 0.05; Iab2 = 0;
		break;
	case 3: // Light 1 only.
		Idr1 = 0.4; Idg1 = 0.4; Idb1 = 0.4;
		Isr1 = 0.2; Isg1 = 0.2; Isb1 = 0.2;
		Iar1 = 0.05; Iag1 = 0.05; Iab1 = 0.05;
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

	// Create submenu for material or 3D textureselection. 
	submenu_id_m = glutCreateMenu(MainSubMenuMaterial);
	glutAddMenuEntry("Marble - 3D Texture", 1);
	glutAddMenuEntry("White Plastic - No Texture", 2);
	glutAddMenuEntry("Gold - No Texture", 3);
	glutAddMenuEntry("Silver - No Texture", 4);

	// Create submenu for light control. 
	submenu_id_l = glutCreateMenu(MainSubMenuLight);
	glutAddMenuEntry("White Light", 1);
	glutAddMenuEntry("Color Light", 2);
	glutAddMenuEntry("Light 1 Only", 3);
	glutAddMenuEntry("Light 2 Only", 4);

	glutCreateMenu(NULL); // Set menu in main window. 
	cout << "glutCreateMenu() for main window menu." << endl;
	glutAddSubMenu("Camera Rotation", submenu_id_r);
	glutAddSubMenu("Projection", submenu_id_p);
	glutAddSubMenu("Material & 3D Texture", submenu_id_m);
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
	ifstream infile;

	do {
		string input;
		cout << "Please enter your input smf file name:" << endl;
		cin >> input;
		infile.open(input.c_str());
		if (!infile) cout << "Your input file path is incorrect." << endl;
	} while (!infile);

	bool storev = false;
	bool storef = false;
	string str;
	vector<vector<int> > faces;
	vector<double> ver_pos;
	vector<int> ver_no;

	// Read file content.
	while (infile) {
		infile >> str;
		if (str.compare("v") == 0) {
			storev = true;
		}
		else if (str.compare("f") == 0){
			storef = true;
		}
		else if (storev == true){ // Add vertex to its vector.
			// Store a vertice.
			ver_pos.push_back(atof(str.c_str()));
			if (ver_pos.size() == 3) {
				vec4 ver(ver_pos[0], ver_pos[1], ver_pos[2], 1);
				original_vertices.push_back(ver);
				storev = false;
				ver_pos.clear();
			}
		}
		else if (storef == true){ // Add face to its vector.
			ver_no.push_back(atoi(str.c_str()));
			// Store vertices for a triangle and calculate its normal vector.
			if (ver_no.size() == 3) {
				faces.push_back(ver_no);
				storef = false;
				ver_no.clear();
			}
		}
	}
	infile.close();

	original_normals.resize(original_vertices.size());
	for (int i = 0; i < (int) original_normals.size(); i++)
		original_normals[i] = vec4(0, 0, 0, 0);

	for (int i = 0; i < (int) faces.size(); i++) {
		vec4 tmpnorm = normalize(vec4(cross((original_vertices[faces[i][1] - 1] - original_vertices[faces[i][0] - 1]), (original_vertices[faces[i][2] - 1] - original_vertices[faces[i][0] - 1]))));
		original_normals[faces[i][0] - 1] = normalize(original_normals[faces[i][0] - 1] + tmpnorm);
		original_normals[faces[i][1] - 1] = normalize(original_normals[faces[i][1] - 1] + tmpnorm);
		original_normals[faces[i][2] - 1] = normalize(original_normals[faces[i][2] - 1] + tmpnorm);
	}

	for (int i = 0; i < (int) faces.size(); i++) {
		vertices.push_back(original_vertices[faces[i][0] - 1]);
		vertices.push_back(original_vertices[faces[i][1] - 1]);
		vertices.push_back(original_vertices[faces[i][2] - 1]);
		normals.push_back(original_normals[faces[i][0] - 1]);
		normals.push_back(original_normals[faces[i][1] - 1]);
		normals.push_back(original_normals[faces[i][2] - 1]);
	}
}

//----------------------------------------------------------------------------

int
main(int argc, char **argv)
{
	readfile(); // Read input smf file.

	glutInit(&argc, argv); // Initializing environment.
	cout << "glutInit(&argc,argv) called." << endl;
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // Enable depth.
	cout << "glutInitDisplayMode() called." << endl;
	glutInitWindowSize(w, h);
	cout << "glutInitWindowSize() called." << endl;
	glutInitWindowPosition(100, 100);
	cout << "glutInitWindowPosition() called." << endl;

	MainWindow = glutCreateWindow("ICG_hw9_part2"); // Initializing & setting main window.
	cout << "glutCreateWindow() for main window." << endl;
	glewExperimental = GL_TRUE;
	glewInit();
	cout << "glewInit() for main window." << endl;
	init(); // Initializing VAOs & VBOs. 
	glutDisplayFunc(display); // Setting display function for main window.
	cout << "glutDisplayFunc() for main window." << endl;
	glutKeyboardFunc(keyboard); // Setting keyboard function for main window.
	cout << "glutKeyboardFunc() for main window." << endl;
	setMainWinMenu(); // Setting menu for main window. 
	glutIdleFunc(RotationFunc); // Start animation by default.
	cout << "glutIdleFunc() for main window." << endl;

	glEnable(GL_DEPTH_TEST);
	cout << "glEnable( GL_DEPTH_TEST ) called." << endl;
	cout << "glutMainLoop() called." << endl;

	glutMainLoop(); // Start main loop. 
	return 0;
}
