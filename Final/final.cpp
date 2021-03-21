#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include <SOIL2\soil2.h>
#include <string>
#include <iostream>
#include <fstream>
#include <glm\gtc\type_ptr.hpp> // glm::value_ptr
#include <glm\gtc\matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include "Torus.h"
#include "Sphere.h"
#include "Utils.h"
#include "ImportedModel.h"
#include <stack>

using namespace std;

float toRadians(float degrees) { return (degrees * 2.0f * 3.14159f) / 360.0f; }

#define numVAOs 1
#define numVBOs 17

//mouse-related variables
double xpos = 0;
double ypos = 0;
float xClip;
float yClip;
GLFWcursorposfun callback;

Utils util = Utils();

//positioning
float cameraX, cameraY, cameraZ;
float torLocX, torLocY, torLocZ;
float gndLocX, gndLocY, gndLocZ;
float magLocX, magLocY, magLocZ;
float sphereLocX, sphereLocY, sphereLocZ;
float pyrLocX, pyrLocY, pyrLocZ;

//shader files
GLuint renderingGroundProgram, renderingProgramCubeMap, renderingSphereProgram, renderingBProgram, renderingGroundProgram1;

GLuint vao[numVAOs];
GLuint vbo[numVBOs];

//textures
GLuint skyboxTexture, groundTexture, heightMap, shuttleTexture;

//camera movement variables
float rotAmt = 0.0f;
float keyAngle = 0;
float keyAngleVert = 0;
float keyAngleZ = 0;

// variable allocation for display
GLuint vLoc, mvLoc, projLoc, nLoc;
int width, height;
float aspect;
glm::mat4 pMat, vMat, mMat, mvMat, invTrMat;
glm::vec3 currentLightPos, transformed;
float lightPos[3];
GLuint globalAmbLoc, ambLoc, diffLoc, specLoc, posLoc, mambLoc, mdiffLoc, mspecLoc, mshiLoc, sLoc;
glm::vec3 origin(0.0f, 0.0f, 0.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);


//objects
int numGroundVertices;
int numTorusVertices, numTorusIndices;
int numSphereVertices, numSphereIndices;
int numMagVertices;
int numShutVertices;
int numPyramidVertices;
Torus myTorus(0.8f, 0.4f, 48);
Sphere mySphere = Sphere(48);
ImportedModel ground("grid.obj");
ImportedModel shuttle("shuttle.obj");
ImportedModel pyramid("pyr.obj");

stack<glm::mat4> mvStack;


//lighting
glm::vec3 lightLoc(2, 40, 3);

float globalAmbient[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
float lightAmbient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
float lightDiffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float lightSpecular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };


// gold material
float* gMatAmb = Utils::goldAmbient();
float* gMatDif = Utils::goldDiffuse();
float* gMatSpe = Utils::goldSpecular();
float gMatShi = Utils::goldShininess();

// bronze material
float* bMatAmb = Utils::bronzeAmbient();
float* bMatDif = Utils::bronzeDiffuse();
float* bMatSpe = Utils::bronzeSpecular();
float bMatShi = Utils::bronzeShininess();

float thisAmb[4], thisDif[4], thisSpe[4], matAmb[4], matDif[4], matSpe[4];
float thisShi, matShi;

//shadows
void passOne(GLFWwindow* window, double currentTime);
void passTwo(GLFWwindow* window, double currentTime);
int scSizeX, scSizeY;
GLuint shadowTex, shadowBuffer;
glm::mat4 lightVmatrix;
glm::mat4 lightPmatrix;
glm::mat4 shadowMVP1;
glm::mat4 shadowMVP2;
glm::mat4 b;




void installLights(int renderingProgram, glm::mat4 vMatrix) {
	transformed = glm::vec3(vMatrix * glm::vec4(currentLightPos, 1.0));
	lightPos[0] = transformed.x;
	lightPos[1] = transformed.y;
	lightPos[2] = transformed.z;

	matAmb[0] = thisAmb[0]; matAmb[1] = thisAmb[1]; matAmb[2] = thisAmb[2]; matAmb[3] = thisAmb[3];
	matDif[0] = thisDif[0]; matDif[1] = thisDif[1]; matDif[2] = thisDif[2]; matDif[3] = thisDif[3];
	matSpe[0] = thisSpe[0]; matSpe[1] = thisSpe[1]; matSpe[2] = thisSpe[2]; matSpe[3] = thisSpe[3];
	matShi = thisShi;

	// get location of materials and light values for uniform variables in the shader
	globalAmbLoc = glGetUniformLocation(renderingProgram, "globalAmbient");
	ambLoc = glGetUniformLocation(renderingProgram, "light.ambient");
	diffLoc = glGetUniformLocation(renderingProgram, "light.diffuse");
	specLoc = glGetUniformLocation(renderingProgram, "light.specular");
	posLoc = glGetUniformLocation(renderingProgram, "light.position");
	mambLoc = glGetUniformLocation(renderingProgram, "material.ambient");
	mdiffLoc = glGetUniformLocation(renderingProgram, "material.diffuse");
	mspecLoc = glGetUniformLocation(renderingProgram, "material.specular");
	mshiLoc = glGetUniformLocation(renderingProgram, "material.shininess");


	//  setting light values and materials in uniform variables in the shader
	glProgramUniform4fv(renderingProgram, globalAmbLoc, 1, globalAmbient);
	glProgramUniform4fv(renderingProgram, ambLoc, 1, lightAmbient);
	glProgramUniform4fv(renderingProgram, diffLoc, 1, lightDiffuse);
	glProgramUniform4fv(renderingProgram, specLoc, 1, lightSpecular);
	glProgramUniform3fv(renderingProgram, posLoc, 1, lightPos);
	glProgramUniform4fv(renderingProgram, mambLoc, 1, matAmb);
	glProgramUniform4fv(renderingProgram, mdiffLoc, 1, matDif);
	glProgramUniform4fv(renderingProgram, mspecLoc, 1, matSpe);
	glProgramUniform1f(renderingProgram, mshiLoc, matShi);
}

void setupVertices(void) {
	float cubeVertexPositions[108] =
	{ -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f, 1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
		1.0f, -1.0f, -1.0f, 1.0f, -1.0f,  1.0f, 1.0f,  1.0f, -1.0f,
		1.0f, -1.0f,  1.0f, 1.0f,  1.0f,  1.0f, 1.0f,  1.0f, -1.0f,
		1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, 1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f
	};


	numGroundVertices = ground.getNumVertices();
	std::vector<glm::vec3> groundVert = ground.getVertices();
	std::vector<glm::vec2> groundTex = ground.getTextureCoords();
	std::vector<glm::vec3> groundNorm = ground.getNormals();

	std::vector<float> groundpvalues;
	std::vector<float> groundtvalues;
	std::vector<float> groundnvalues;

	for (int i = 0; i < numGroundVertices; i++) {
		groundpvalues.push_back((groundVert[i]).x);
		groundpvalues.push_back((groundVert[i]).y);
		groundpvalues.push_back((groundVert[i]).z);
		groundtvalues.push_back((groundTex[i]).x);
		groundtvalues.push_back((groundTex[i]).y);
		groundnvalues.push_back((groundNorm[i]).x);
		groundnvalues.push_back((groundNorm[i]).y);
		groundnvalues.push_back((groundNorm[i]).z);
	}

	glGenVertexArrays(1, vao);
	glBindVertexArray(vao[0]);
	glGenBuffers(numVBOs, vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertexPositions), cubeVertexPositions, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, groundpvalues.size() * 4, &groundpvalues[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, groundtvalues.size() * 4, &groundtvalues[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
	glBufferData(GL_ARRAY_BUFFER, groundnvalues.size() * 4, &groundnvalues[0], GL_STATIC_DRAW);

	std::vector<int> ind = mySphere.getIndices();
	std::vector<glm::vec3> sphereVert = mySphere.getVertices();
	std::vector<glm::vec2> sphereTex = mySphere.getTexCoords();
	std::vector<glm::vec3> sphereNorm = mySphere.getNormals();

	std::vector<float> spherepvalues;
	std::vector<float> spheretvalues;
	std::vector<float> spherenvalues;

	int numIndices = mySphere.getNumIndices();
	for (int i = 0; i < numIndices; i++) {
		spherepvalues.push_back((sphereVert[ind[i]]).x);
		spherepvalues.push_back((sphereVert[ind[i]]).y);
		spherepvalues.push_back((sphereVert[ind[i]]).z);
		spheretvalues.push_back((sphereTex[ind[i]]).s);
		spheretvalues.push_back((sphereTex[ind[i]]).t);
		spherenvalues.push_back((sphereNorm[ind[i]]).x);
		spherenvalues.push_back((sphereNorm[ind[i]]).y);
		spherenvalues.push_back((sphereNorm[ind[i]]).z);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);
	glBufferData(GL_ARRAY_BUFFER, spherepvalues.size() * 4, &spherepvalues[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
	glBufferData(GL_ARRAY_BUFFER, spheretvalues.size() * 4, &spheretvalues[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
	glBufferData(GL_ARRAY_BUFFER, spherenvalues.size() * 4, &spherenvalues[0], GL_STATIC_DRAW);

	numTorusVertices = myTorus.getNumVertices();
	numTorusIndices = myTorus.getNumIndices();

	std::vector<int> torind = myTorus.getIndices();
	std::vector<glm::vec3> torvert = myTorus.getVertices();
	std::vector<glm::vec2> tortex = myTorus.getTexCoords();
	std::vector<glm::vec3> tornorm = myTorus.getNormals();

	std::vector<float> torpvalues;
	std::vector<float> tortvalues;
	std::vector<float> tornvalues;

	for (int i = 0; i < numTorusVertices; i++) {
		torpvalues.push_back(torvert[i].x);
		torpvalues.push_back(torvert[i].y);
		torpvalues.push_back(torvert[i].z);
		tortvalues.push_back(tortex[i].s);
		tortvalues.push_back(tortex[i].t);
		tornvalues.push_back(tornorm[i].x);
		tornvalues.push_back(tornorm[i].y);
		tornvalues.push_back(tornorm[i].z);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);
	glBufferData(GL_ARRAY_BUFFER, torpvalues.size() * 4, &torpvalues[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[8]);
	glBufferData(GL_ARRAY_BUFFER, tortvalues.size() * 4, &tortvalues[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[9]);
	glBufferData(GL_ARRAY_BUFFER, tornvalues.size() * 4, &tornvalues[0], GL_STATIC_DRAW);


	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[10]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, torind.size() * 4, &torind[0], GL_STATIC_DRAW);

	numPyramidVertices = pyramid.getNumVertices();
	std::vector<glm::vec3> pyrvert = pyramid.getVertices();
	std::vector<glm::vec2> pyrtex = pyramid.getTextureCoords();
	std::vector<glm::vec3> pyrnorm = pyramid.getNormals();;
	std::vector<float> pyramidPvalues;
	std::vector<float> pyramidNvalues;
	std::vector<float> pyramidTvalues;

	for (int i = 0; i < numPyramidVertices; i++) {
		pyramidPvalues.push_back((pyrvert[i]).x);
		pyramidPvalues.push_back((pyrvert[i]).y);
		pyramidPvalues.push_back((pyrvert[i]).z);
		pyramidTvalues.push_back((pyrtex[i]).s);
		pyramidTvalues.push_back((pyrtex[i]).t);
		pyramidNvalues.push_back((pyrnorm[i]).x);
		pyramidNvalues.push_back((pyrnorm[i]).y);
		pyramidNvalues.push_back((pyrnorm[i]).z);


	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo[11]);
	glBufferData(GL_ARRAY_BUFFER, pyramidPvalues.size() * 4, &pyramidPvalues[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[12]);
	glBufferData(GL_ARRAY_BUFFER, pyramidTvalues.size() * 4, &pyramidTvalues[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[13]);
	glBufferData(GL_ARRAY_BUFFER, pyramidNvalues.size() * 4, &pyramidNvalues[0], GL_STATIC_DRAW);
	
	numShutVertices = shuttle.getNumVertices();
	std::vector<glm::vec3> shutVert =  shuttle.getVertices();
	std::vector<glm::vec2> shutTex =  shuttle.getTextureCoords();
	std::vector<glm::vec3> shutNorm =  shuttle.getNormals();

	std::vector<float> shutPvalues;
	std::vector<float> shutNvalues;
	std::vector<float> shutTvalues;

	for (int i = 0; i < numShutVertices; i++) {
		shutPvalues.push_back((shutVert[i]).x);
		shutPvalues.push_back(( shutVert[i]).y);
		shutPvalues.push_back(( shutVert[i]).z);
		shutTvalues.push_back(( shutTex[i]).s);
		shutTvalues.push_back(( shutTex[i]).t);
		shutNvalues.push_back(( shutNorm[i]).x);
		shutNvalues.push_back(( shutNorm[i]).y);
		shutNvalues.push_back(( shutNorm[i]).z);


	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo[14]);
	glBufferData(GL_ARRAY_BUFFER, shutPvalues.size() * 4, &shutPvalues[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[15]);
	glBufferData(GL_ARRAY_BUFFER,shutTvalues.size() * 4, &shutTvalues[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[16]);
	glBufferData(GL_ARRAY_BUFFER, shutNvalues.size() * 4, &shutNvalues[0], GL_STATIC_DRAW);
}

void setupShadowBuffers(GLFWwindow* window) {
	glfwGetFramebufferSize(window, &width, &height);
	scSizeX = width;
	scSizeY = height;
	//generating shadow buffers
	glGenFramebuffers(1, &shadowBuffer);

	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
		scSizeX, scSizeY, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	// potentially reduces shadow border artifacts
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void init(GLFWwindow* window) {
	//initializing rendering programs
	renderingGroundProgram = Utils::createShaderProgram("vertGShader.glsl", "fragGShader.glsl");
	renderingBProgram = Utils::createShaderProgram("vertBShader.glsl", "fragBShader.glsl");
	renderingProgramCubeMap = Utils::createShaderProgram("vertCShader.glsl", "fragCShader.glsl");
	renderingGroundProgram1 = Utils::createShaderProgram("vertGShader1.glsl", "fragGShader1.glsl");
	renderingSphereProgram = Utils::createShaderProgram("vertSShader.glsl", "fragSShader.glsl");

	glfwGetFramebufferSize(window, &width, &height);
	aspect = (float)width / (float)height;
	pMat = glm::perspective(1.0472f, aspect, 0.1f, 1000.0f);

	setupVertices();
	setupShadowBuffers(window);
	
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	//initializing positions
	torLocX = 0.0f; torLocY = 0.0f; torLocZ = 0.0f;
	magLocX = 3; magLocY = 1.0; magLocZ = 2;
	sphereLocX = 0.0f; sphereLocY = 25.0f; sphereLocZ = 0.0f;
	pyrLocX = 25; pyrLocY = 3; pyrLocZ = 25;
	cameraX = 0.0f; cameraY = 45.0f; cameraZ = 80.0f;
	gndLocX = 0.0f; gndLocY = 0.0f; gndLocZ = 0.0f;

	//initializing textures
	groundTexture = Utils::loadTexture("grass.jpg");
	heightMap =  Utils::loadTexture("height.jpg");
	skyboxTexture = Utils::loadCubeMap("cubeMap");
	shuttleTexture = Utils::loadTexture("spstob_1.jpg");

	b = glm::mat4(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f);

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{

	//camera controls with various keys
	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		keyAngleVert += 5.0;

	}
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		keyAngleVert -= 5.0;
	}
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		keyAngle -= 5.0;
	}
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		keyAngle 
			+= 5.0;
	}
	if (key == GLFW_KEY_MINUS && action == GLFW_PRESS) {
		cameraZ
			+= 5.0;
	}
	if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS) {
		cameraZ
			-= 5.0;
	}
	if (key == GLFW_KEY_W && action == GLFW_PRESS) {
		keyAngleZ
			+= 5.0;
	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		keyAngleZ
			-= 5.0;
	}
}

void display(GLFWwindow* window, double currentTime) {
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);

	glfwGetCursorPos(window, &xpos, &ypos);
	//converting screen coordinates to world coordinates so that spotlight shines on objects that the cursor is on
	xClip = 300 * (((float)xpos / 400.0f) - 1.0f);
	yClip =(-300 * (1.0f - (((float)ypos) / 400.0f)));
	currentLightPos = glm::vec3(xClip, lightLoc.y, yClip);

	lightVmatrix = glm::lookAt(currentLightPos, origin, up);
	lightPmatrix = glm::perspective(toRadians(60.0f), aspect, 0.1f, 1000.0f);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowTex, 0);

	glDrawBuffer(GL_NONE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);	// for reducing
	glPolygonOffset(2.0f, 4.0f);		//  shadow artifacts

	//getting depth buffer info 
	passOne(window, currentTime);

	glDisable(GL_POLYGON_OFFSET_FILL);	// artifact reduction, continued

	//assinging shadow texture, GL_TEXTURE2 is available for all shader files
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	glDrawBuffer(GL_FRONT);

	passTwo(window, currentTime);
}

stack<glm::mat4> mStack; //stack to keep track of model matrices
void passOne(GLFWwindow* window, double currentTime) {
	glUseProgram(renderingGroundProgram1);

	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(gndLocX, gndLocY, gndLocZ));
	mStack.push(mMat);
	mMat = glm::scale(mMat, glm::vec3(100.0, 100.0, 100.0));

	sLoc = glGetUniformLocation(renderingGroundProgram1, "shadowMVP");
	shadowMVP1 = lightPmatrix * lightVmatrix * mMat;
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP1));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, heightMap);

	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);

	glDrawArrays(GL_TRIANGLES, 0, numGroundVertices);

	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(sphereLocX, sphereLocY, sphereLocZ));
	mStack.push(mMat);
	mMat = scale(mMat, glm::vec3(3, 3, 3));

	shadowMVP1 = lightPmatrix * lightVmatrix * mMat;
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP1));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLES, 0, mySphere.getNumIndices());

	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(torLocX, torLocY, torLocZ));
	mStack.push(mMat);
	mMat = glm::rotate(mMat, (float)(currentTime * 0.25), glm::vec3(0.0, 1.0, 1.0));
	mMat = scale(mMat, glm::vec3(10, 10, 10));

	shadowMVP1 = lightPmatrix * lightVmatrix * mMat;
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP1));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[10]);
	glDrawElements(GL_TRIANGLES, numTorusIndices, GL_UNSIGNED_INT, 0);

	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(pyrLocX, pyrLocY, pyrLocZ));
	mMat = glm::rotate(mMat, (float)(currentTime * 0.25), glm::vec3(0.0, 1.0, 0.0));
	mMat = scale(mMat, glm::vec3(3, 3, 3));

	shadowMVP1 = lightPmatrix * lightVmatrix * mMat;
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP1));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[11]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glDrawArrays(GL_TRIANGLES, 0, numPyramidVertices);

	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(-45, pyrLocY, -45));
	mMat = glm::rotate(mMat, (float)(currentTime * 0.25), glm::vec3(0.0, 1.0, 0.0));
	mMat = scale(mMat, glm::vec3(3, 3, 3));

	shadowMVP1 = lightPmatrix * lightVmatrix * mMat;
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP1));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[11]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glDrawArrays(GL_TRIANGLES, 0, numPyramidVertices);

	glUseProgram(renderingBProgram);
	
	mMat = glm::translate(mStack.top(), glm::vec3(0,0,0));
	mStack.push(mMat);
	mMat = glm::translate(mMat, glm::vec3(sin((float)currentTime) * -20.0, 0.0f, cos((float)currentTime) * -20.0));
	mMat = scale(mMat, glm::vec3(6, 6, 6));
	mMat = rotate(mMat, toRadians(90.0f), glm::vec3(0.0, 1.0, 0.0));
	mMat = rotate(mMat, (float)currentTime, glm::vec3(0.0, 1.0, 0.0));

	sLoc = glGetUniformLocation(renderingBProgram, "shadowMVP");
	shadowMVP1 = lightPmatrix * lightVmatrix * mMat;
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP1));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[14]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);

	glDrawArrays(GL_TRIANGLES, 0, numShutVertices);

	mStack.pop();mStack.pop(); mStack.pop();
}

void passTwo(GLFWwindow* window, double currentTime) {
	
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
	glfwSetKeyCallback(window, key_callback);
	

	//spotlight disappears if cursor leaves window
	if (!glfwGetWindowAttrib(window, GLFW_HOVERED))
	{
		currentLightPos = glm::vec3(NULL, NULL, NULL);
		
	}

	glUseProgram(renderingProgramCubeMap);

	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);


	//creating view matrix
	vMat = glm::lookAt(glm::vec3(cameraX, cameraY, cameraZ), origin, up);
	vMat = glm::rotate(vMat, toRadians(-10), glm::vec3(1.0, 0.0, 0.0));
	vMat = glm::rotate(vMat, toRadians(keyAngle), glm::vec3(0.0, 1.0, 0.0));
	vMat = glm::rotate(vMat, toRadians(keyAngleVert), glm::vec3(1.0, 0.0, 0.0));
	vMat = glm::rotate(vMat, toRadians(keyAngleZ), glm::vec3(0.0, 0.0, 1.0));
	mvStack.push(vMat);

	// draw cube map

	vLoc = glGetUniformLocation(renderingProgramCubeMap, "v_matrix");
	projLoc = glGetUniformLocation(renderingProgramCubeMap, "p_matrix");

	glUniformMatrix4fv(vLoc, 1, GL_FALSE, glm::value_ptr(vMat));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);


	glEnable(GL_CULL_FACE); //enable cull face function used for all objects for optimization
	glFrontFace(GL_CCW);	
	glDisable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glEnable(GL_DEPTH_TEST);

	// draw ground

	glUseProgram(renderingGroundProgram);

	thisAmb[0] = gMatAmb[0]; thisAmb[1] = gMatAmb[1]; thisAmb[2] = gMatAmb[2];  // setting ground with gold material
	thisDif[0] = gMatDif[0]; thisDif[1] = gMatDif[1]; thisDif[2] = gMatDif[2];
	thisSpe[0] = gMatSpe[0]; thisSpe[1] = gMatSpe[1]; thisSpe[2] = gMatSpe[2];
	thisShi = gMatShi;

	installLights(renderingGroundProgram, vMat);

	nLoc = glGetUniformLocation(renderingGroundProgram, "norm_matrix");
	projLoc = glGetUniformLocation(renderingGroundProgram, "proj_matrix");
	mvLoc = glGetUniformLocation(renderingGroundProgram, "mv_matrix");
	sLoc = glGetUniformLocation(renderingGroundProgram, "shadowMVP");

	
	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(gndLocX, gndLocY, gndLocZ));
	mStack.push(mMat);
	mvStack.push(mvStack.top());
	mvStack.top() *= mMat;
	mvStack.push(mvStack.top());
	mMat = glm::scale(mMat, glm::vec3(100.0, 100.0, 100.0));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(100.0, 100.0, 100.0));
	
	shadowMVP2 = b * lightPmatrix * lightVmatrix * mMat;
	
	invTrMat = glm::transpose(glm::inverse(mvStack.top()));
	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvStack.top()));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
	glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP2));
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, groundTexture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, heightMap);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glDrawArrays(GL_TRIANGLES, 0, numGroundVertices);

	mvStack.pop();

	//draw sphere

	glUseProgram(renderingSphereProgram);

	installLights(renderingSphereProgram, vMat);

	nLoc = glGetUniformLocation(renderingSphereProgram, "norm_matrix");
	projLoc = glGetUniformLocation(renderingSphereProgram, "proj_matrix");
	mvLoc = glGetUniformLocation(renderingSphereProgram, "mv_matrix");
	sLoc = glGetUniformLocation(renderingSphereProgram, "shadowMVP");


	mMat = glm::translate(mStack.top(), glm::vec3(sphereLocX, sphereLocY, sphereLocZ));
	mStack.push(mMat);
	mvStack.push(mvStack.top());
	mvStack.top() *= mMat;
	mvStack.push(mvStack.top());
	mMat = scale(mMat, glm::vec3(3, 3, 3));
	mvStack.top() = scale(mvStack.top(), glm::vec3(3, 3, 3));
	
	invTrMat = glm::transpose(glm::inverse(mvStack.top()));
	shadowMVP2 = b * lightPmatrix * lightVmatrix * mMat ;
	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvStack.top()));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
	glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP2));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLES, 0, mySphere.getNumIndices()); 

	mvStack.pop();

	//draw torus 

	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(torLocX, torLocY, torLocZ));
	mStack.push(mMat);
	mvStack.push(mvStack.top());
	mvStack.top() *= mMat;
	glm::mat4 torusMV = mvStack.top();
	glm::mat4 torusM = mMat;
	mvStack.push(mvStack.top());
	mMat = glm::rotate(mMat, (float)(currentTime * 0.25), glm::vec3(0.0, 1.0, 1.0));
	mvStack.top() = glm::rotate(mvStack.top(), (float)(currentTime * 0.25), glm::vec3(0.0, 1.0, 1.0));
	mMat = scale(mMat, glm::vec3(10, 10, 10));
	mvStack.top() = scale(mvStack.top(), glm::vec3(10, 10, 10));
	
	shadowMVP2 = b * lightPmatrix * lightVmatrix * mMat;

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvStack.top()));
	invTrMat = glm::transpose(glm::inverse(mvStack.top()));
	glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP2));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
	

	glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[8]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[9]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

	glFrontFace(GL_CCW);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glEnable(GL_DEPTH_TEST);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[10]);
	glDrawElements(GL_TRIANGLES, numTorusIndices, GL_UNSIGNED_INT, 0);

	mvStack.pop(); 

	//draw pyramid
	
	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(pyrLocX, pyrLocY, pyrLocZ));
	mStack.push(mMat);
	mvStack.push(mvStack.top());
	mvStack.top() *= mMat;
	mvStack.push(mvStack.top());
	mMat = glm::rotate(mMat, (float)(currentTime * 0.25), glm::vec3(0.0, 1.0, 0.0));
	mvStack.top() = glm::rotate(mvStack.top(), (float)(currentTime * 0.25), glm::vec3(0.0, 1.0, 0.0));
	mMat = scale(mMat, glm::vec3(3, 3, 3));
	mvStack.top() = scale(mvStack.top(), glm::vec3(3, 3, 3));

	shadowMVP2 = b * lightPmatrix * lightVmatrix * mMat;

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvStack.top()));
	invTrMat = glm::transpose(glm::inverse(mvStack.top()));
	glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP2));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));


	glBindBuffer(GL_ARRAY_BUFFER, vbo[11]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[12]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[13]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

	glFrontFace(GL_CCW);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glDrawArrays(GL_TRIANGLES, 0, numPyramidVertices);

	mvStack.pop();

	//draw 2nd pyramid

	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(-45, pyrLocY, -45));
	mStack.push(mMat);
	mvStack.push(mvStack.top());
	mvStack.top() *= mMat;
	mvStack.push(mvStack.top());
	mMat = glm::rotate(mMat, (float)(currentTime * 0.25), glm::vec3(0.0, 1.0, 0.0));
	mvStack.top() = glm::rotate(mvStack.top(), (float)(currentTime * 0.25), glm::vec3(0.0, 1.0, 0.0));
	mMat = scale(mMat, glm::vec3(3, 3, 3));
	mvStack.top() = scale(mvStack.top(), glm::vec3(3, 3, 3));

	shadowMVP2 = b * lightPmatrix * lightVmatrix * mMat;

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvStack.top()));
	invTrMat = glm::transpose(glm::inverse(mvStack.top()));
	glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP2));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[11]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[12]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[13]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

	glFrontFace(GL_CCW);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glDrawArrays(GL_TRIANGLES, 0, numPyramidVertices);
	mvStack.pop();

	//draw shuttle
	
	glUseProgram(renderingBProgram);
	installLights(renderingBProgram, vMat);

	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(torLocX, 14, torLocZ));
	mStack.push(mMat);
	mvStack.push(vMat);
	mvStack.top() *= mMat;
	mvStack.push(mvStack.top());
	mMat = glm::translate(mMat, glm::vec3(sin((float)currentTime) * -20.0, 0.0f, cos((float)currentTime) * -20.0));

	mMat = rotate(mMat, toRadians(90.0f), glm::vec3(0.0, 1.0, 0.0));
	mMat = rotate(mMat, (float)currentTime, glm::vec3(0.0, 1.0, 0.0));
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(sin((float)currentTime) * -20.0, 0.0f, cos((float)currentTime) * -20.0));
	mvStack.top() = rotate(mvStack.top(), toRadians(90.0f), glm::vec3(0.0, 1.0, 0.0));
	mvStack.top() = rotate(mvStack.top(), (float)currentTime, glm::vec3(0.0, 1.0, 0.0));
	mMat = scale(mMat, glm::vec3(3, 3, 3));
	mvStack.top() = scale(mvStack.top(), glm::vec3(6, 6, 6));

	sLoc = glGetUniformLocation(renderingBProgram, "shadowMVP");
	shadowMVP2 = b * lightPmatrix * lightVmatrix * mMat;
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP2));
	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvStack.top()));
	invTrMat = glm::transpose(glm::inverse(mvStack.top()));
	glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[14]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[15]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[16]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shuttleTexture);

	glFrontFace(GL_CCW);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);

	glDrawArrays(GL_TRIANGLES, 0, numShutVertices);
	mvStack.pop(); mvStack.pop(); mvStack.pop(); mvStack.pop(); mvStack.pop(); mvStack.pop(); mvStack.pop(); mvStack.pop();
	mStack.pop(); 	mStack.pop(); 	mStack.pop();	mStack.pop();	mStack.pop();	mStack.pop(); 
}

void window_size_callback(GLFWwindow* win, int newWidth, int newHeight) {
	aspect = (float)newWidth / (float)newHeight;
	glViewport(0, 0, newWidth, newHeight);
	pMat = glm::perspective(1.0472f, aspect, 0.1f, 1000.0f);

}

int main(void) {
	if (!glfwInit()) { exit(EXIT_FAILURE); }
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	GLFWwindow* window = glfwCreateWindow(800, 800, "Final Project", NULL, NULL);
	glfwMakeContextCurrent(window);
	if (glewInit() != GLEW_OK) { exit(EXIT_FAILURE); }
	glfwSwapInterval(1);

	glfwSetWindowSizeCallback(window, window_size_callback);

	init(window);

	while (!glfwWindowShouldClose(window)) {
		display(window, glfwGetTime());
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}