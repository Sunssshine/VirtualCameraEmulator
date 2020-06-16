#include "CurvedBlade.h"



CurvedBlade::CurvedBlade(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, float twist, int levelsNum)
{
	calculate(p1, p2, p3, p4, twist, levelsNum);
	loadFigure();
	freeData();
}

void CurvedBlade::calculate(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, float twist, int levelsNum)
{
	float step = 1.0f / levelsNum;
	int sizeOfIteration = 4 * 6;  // 4 number of points per segment, 6 number of values per point (x,y,z + normal vector)
	size = sizeOfIteration * levelsNum;
	vertices = new float[size];

	indicesSize = 6 * levelsNum; // 6 number of indices per segment
	indices = new GLuint[indicesSize];
	// generate spline curve vector
	std::vector <glm::vec3> splinePoints(levelsNum);
	for (size_t i = 0; i < levelsNum; i++)
	{
		splinePoints[i] = glm::vec3(glm::catmullRom(p1, p2, p3, p4, i*step), 0.0f);
		splinePoints[i] = glm::rotateX(splinePoints[i], glm::radians(twist*i));
	}

	for (size_t i = 0; i < levelsNum; i++)
	{
		// left lower corner
		if (i == 0)
		{
			vertices[i * sizeOfIteration] =     p2.x;
			vertices[i * sizeOfIteration + 1] = p2.y;
			vertices[i * sizeOfIteration + 2] = 0; // z 
		}
		else
		{
			vertices[i * sizeOfIteration] = vertices[(i - 1) * sizeOfIteration + 6];
			vertices[i * sizeOfIteration + 1] = vertices[(i - 1) * sizeOfIteration + 7];
			vertices[i * sizeOfIteration + 2] = vertices[(i - 1) * sizeOfIteration + 8];
		}

		// place for normals (3,4,5 indices)
		// left upper corner
		vertices[i * sizeOfIteration + 6] = splinePoints[i].x;
		vertices[i * sizeOfIteration + 7] = splinePoints[i].y;
		vertices[i * sizeOfIteration + 8] = splinePoints[i].z;
		// place for normals (9,10,11 indices)

		// right lower corner
		if (i == 0)
		{
			vertices[i * sizeOfIteration + 12] = p2.x;
			vertices[i * sizeOfIteration + 13] = -p2.y;
			vertices[i * sizeOfIteration + 14] = 0; // z
		}
		else
		{
			vertices[i * sizeOfIteration + 12] = vertices[(i - 1) * sizeOfIteration + 18];
			vertices[i * sizeOfIteration + 13] = vertices[(i - 1) * sizeOfIteration + 19];
			vertices[i * sizeOfIteration + 14] = vertices[(i - 1) * sizeOfIteration + 20]; // z
		}
		// place for normals (15,16,17 indices)

		// right upper corner
		vertices[i * sizeOfIteration + 18] = splinePoints[i].x;
		vertices[i * sizeOfIteration + 19] = -splinePoints[i].y;
		vertices[i * sizeOfIteration + 20] = -splinePoints[i].z;
		// place for normals (21,22,23 indices)
		// calc normal vector for this side
		glm::vec3 lowerLeft = glm::vec3(
			vertices[i * sizeOfIteration],
			vertices[i * sizeOfIteration + 1],
			0
		);

		glm::vec3 upperLeft = glm::vec3(
			vertices[i * sizeOfIteration + 6],
			vertices[i * sizeOfIteration + 7],
			0
		);

		glm::vec3 lowerRight = glm::vec3(
			vertices[i * sizeOfIteration + 12],
			vertices[i * sizeOfIteration + 13],
			0
		);

		// notice that we know that one of plane vectors always direct to up (0, 1, 0) for cylinder
		glm::vec3 normalVec = glm::normalize(
			glm::cross(lowerRight - lowerLeft, upperLeft - lowerLeft)
		);

		vertices[i * sizeOfIteration + 3] = normalVec.x;
		vertices[i * sizeOfIteration + 4] = normalVec.y;
		vertices[i * sizeOfIteration + 5] = normalVec.z;
		vertices[i * sizeOfIteration + 9] = normalVec.x;
		vertices[i * sizeOfIteration + 10] = normalVec.y;
		vertices[i * sizeOfIteration + 11] = normalVec.z;
		vertices[i * sizeOfIteration + 15] = normalVec.x;
		vertices[i * sizeOfIteration + 16] = normalVec.y;
		vertices[i * sizeOfIteration + 17] = normalVec.z;
		vertices[i * sizeOfIteration + 21] = normalVec.x;
		vertices[i * sizeOfIteration + 22] = normalVec.y;
		vertices[i * sizeOfIteration + 23] = normalVec.z;

		// set indices for this side
		indices[i * 6] = i * 4;
		indices[i * 6 + 1] = i * 4 + 1;
		indices[i * 6 + 2] = i * 4 + 2;
		indices[i * 6 + 3] = i * 4 + 1;
		indices[i * 6 + 4] = i * 4 + 2;
		indices[i * 6 + 5] = i * 4 + 3;
	}
	
}

void CurvedBlade::recalculate(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, float twist, int levelsNum)
{
	if (!isFigureLoaded())
		return;

	calculate(p1, p2, p3, p4, twist, levelsNum);
	loadBuffers();
	freeData();
}


CurvedBlade::~CurvedBlade()
{
}
