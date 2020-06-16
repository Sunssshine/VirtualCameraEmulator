#include "Figure.h"
#include <iostream>


Figure::Figure()
{
}


Figure::~Figure()
{
	unloadFigure();
}

void Figure::genBuffers()
{
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &VAO);
}


bool Figure::isFigureLoaded()
{
	return isLoaded;
}

void Figure::loadFigure()
{
	if (isLoaded)
		return;

	genBuffers();
	loadBuffers();
	initVAO();
	isLoaded = true;
}

void Figure::draw()
{
	if (!isLoaded)
		return;

	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indicesSize, GL_UNSIGNED_INT, 0);
}

void Figure::unloadFigure()
{
	if (!isLoaded)
		return;

	unloadBuffers();

	VAO = 0;
	VBO = 0;
	EBO = 0;

	isLoaded = false;
}

void Figure::initVAO()
{

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Figure::loadBuffers()
{
	loadVBO();
	loadEBO();
}

void Figure::loadVBO()
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), vertices, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Figure::loadEBO()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize * sizeof(GLuint), indices, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Figure::unloadVBO()
{
	glDeleteBuffers(1, &VBO);
}

void Figure::unloadEBO()
{
	glDeleteBuffers(1, &EBO);
}

void Figure::unloadVAO()
{
	glDeleteVertexArrays(1, &VAO);
}

void Figure::freeData()
{
	delete[] vertices;

	delete[] indices;

	vertices = nullptr;
	indices = nullptr;
}

void Figure::unloadBuffers()
{
	unloadVBO();
	unloadEBO();
	unloadVAO();
}
