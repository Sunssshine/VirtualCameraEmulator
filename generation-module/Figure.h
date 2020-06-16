#pragma once
#include <GL/gl3w.h>

class Figure
{
	void genBuffers();
	void loadVBO();
	void loadEBO();
	void initVAO();

	void unloadVBO();
	void unloadEBO();
	void unloadVAO();
	void unloadBuffers();

	bool isLoaded = false;

	GLuint VAO = 0;
	GLuint VBO = 0;
	GLuint EBO = 0;

protected:

	float * vertices = nullptr;
	int size = 0;
	GLuint * indices = nullptr;
	int indicesSize = 0;

	void freeData();
	void loadBuffers();


public:

	Figure();
	
	bool isFigureLoaded();
	void loadFigure();
	void draw();
	void unloadFigure();
	
	~Figure();
};

