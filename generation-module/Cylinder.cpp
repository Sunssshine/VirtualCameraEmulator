#include "Cylinder.h"





Cylinder::Cylinder(float radius, float height, int numberOfEdges)
{
	calculate(radius, height, numberOfEdges);
	loadFigure();
	freeData();
}

void Cylinder::calculate(float radius, float height, int numberOfEdges)
{
	float step = 360.0f / numberOfEdges;
	float halfHeight = height / 2;
	int sizeOfIteration = 4 * (3 + 3); // 2 is number of circles in cylinder, 3 is number of coordinates (x,y,z), and another 3 for normal vector
	size = numberOfEdges * sizeOfIteration; 
	vertices = new float[size];
	indicesSize = numberOfEdges * 6 + numberOfEdges * 6;
	indices = new unsigned int[indicesSize]; // 3 first triangle, 3 second triangle of quad edge
	for (int i = 0; i < numberOfEdges; i++)
	{
		// left lower corner
		if (i == 0)
		{
			vertices[i * sizeOfIteration] = radius * glm::cos(glm::radians(i*step));
			vertices[i * sizeOfIteration + 1] = radius * glm::sin(glm::radians(i*step));
		}
		else
		{
			vertices[i * sizeOfIteration] = vertices[(i-1) * sizeOfIteration + 12];
			vertices[i * sizeOfIteration + 1] = vertices[(i-1) * sizeOfIteration + 13];
		}

		vertices[i * sizeOfIteration + 2] = -halfHeight;
		// place for normals (3,4,5 indices)
		// left upper corner
		vertices[i * sizeOfIteration + 6] = vertices[i * sizeOfIteration];
		vertices[i * sizeOfIteration + 7] = vertices[i * sizeOfIteration + 1];
		vertices[i * sizeOfIteration + 8] = halfHeight;
		// place for normals (9,10,11 indices)
		// right lower corner
		vertices[i * sizeOfIteration + 12] = radius * glm::cos(glm::radians((i + 1) * step));
		vertices[i * sizeOfIteration + 13] = radius * glm::sin(glm::radians((i + 1) * step));
		vertices[i * sizeOfIteration + 14] = -halfHeight;
		// place for normals (15,16,17 indices)
		// right upper corner
		vertices[i * sizeOfIteration + 18] = vertices[i * sizeOfIteration + 12];
		vertices[i * sizeOfIteration + 19] = vertices[i * sizeOfIteration + 13];
		vertices[i * sizeOfIteration + 20] = halfHeight;
		// place for normals (21,22,23 indices)
		// calc normal vector for this side
		glm::vec3 lowerLeft = glm::vec3(
			vertices[i * sizeOfIteration],
			vertices[i * sizeOfIteration + 1],
			-halfHeight
		);

		glm::vec3 lowerRight = glm::vec3(
			vertices[i * sizeOfIteration + 12],
			vertices[i * sizeOfIteration + 13],
			-halfHeight
		);

		// notice that we know that one of plane vectors always direct to up (0, 1, 0) for cylinder
		glm::vec3 normalVec = glm::normalize(
			glm::cross(lowerRight - lowerLeft, glm::vec3(0.0f, 0.0f, 1.0f))
		);

		vertices[i * sizeOfIteration + 3]  = normalVec.x;
		vertices[i * sizeOfIteration + 4]  = normalVec.y;
		vertices[i * sizeOfIteration + 5]  = normalVec.z;
		vertices[i * sizeOfIteration + 9]  = normalVec.x;
		vertices[i * sizeOfIteration + 10] = normalVec.y;
		vertices[i * sizeOfIteration + 11] = normalVec.z;
		vertices[i * sizeOfIteration + 15] = normalVec.x;
		vertices[i * sizeOfIteration + 16] = normalVec.y;
		vertices[i * sizeOfIteration + 17] = normalVec.z;
		vertices[i * sizeOfIteration + 21] = normalVec.x;
		vertices[i * sizeOfIteration + 22] = normalVec.y;
		vertices[i * sizeOfIteration + 23] = normalVec.z;
		
		// set indices for this side
		indices[i*6]     = i*4;
		indices[i*6 + 1] = i*4 + 1;
		indices[i*6 + 2] = i*4 + 2;
		indices[i*6 + 3] = i*4 + 1;
		indices[i*6 + 4] = i*4 + 2;
		indices[i*6 + 5] = i*4 + 3;
		//std::cout << indices[i * 6] << " " << indices[i * 6 + 1] << " " << indices[i * 6 + 2] << std::endl;
		//std::cout << indices[i * 6+3] << " " << indices[i * 6 + 4] << " " << indices[i * 6 + 5] << std::endl;

		//for (int k = 0; k < 4; k++)
		//{
		//	std::cout << std::endl;
		//	for (int j = 0; j < 6; j++)
		//	{
		//		std::cout << vertices[i*sizeOfIteration + j + 6*k] << " ";
		//	}
		//}
		//std::cout << std::endl << "Another side" << std::endl;
	}

	for (size_t i = 1; i < numberOfEdges; i++)
	{
		// 0, 2, 6 -> 0, 6, 10 -> 0, 10, 14
		indices[numberOfEdges * 6 + i * 6] = 0;
		indices[numberOfEdges * 6 + i * 6 + 1] = i*4 - 2;
		indices[numberOfEdges * 6 + i * 6 + 2] = i*4 + 2;
		indices[numberOfEdges * 6 + i * 6 + 3] = 1;
		indices[numberOfEdges * 6 + i * 6 + 4] = i * 4 - 1;
		indices[numberOfEdges * 6 + i * 6 + 5] = i * 4 + 3;
	}

}

void Cylinder::recalculate(float radius, float height, int numberOfEdges)
{
	if (!isFigureLoaded())
		return;

	calculate(radius, height, numberOfEdges);
	loadBuffers();
	freeData();
}

Cylinder::~Cylinder()
{

}
