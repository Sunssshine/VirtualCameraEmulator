#include "Paraboloid.h"


Paraboloid::Paraboloid(float affectX, float affectY, float height, int numberOfEdges, int numberOfLevels)
{
	calculate(affectX, affectY, height, numberOfEdges, numberOfLevels);
	loadFigure();
	freeData();
}



void Paraboloid::calculate(float affectX, float affectY, float height, int numberOfEdges, int numberOfLevels)
{
	float horizontalStep = 360.0f / numberOfEdges;
	float verticalStep = height / (numberOfLevels-1);
	float currLevel = 0;
	float currRadius = currLevel;


	size = 24 * numberOfEdges * numberOfLevels;
	vertices = new float[size];

	indicesSize = 6 * numberOfEdges * numberOfLevels;
	indices = new unsigned int[indicesSize];

	for (int i = 0; i < numberOfEdges; i++)
	{
		for (int j = 0; j < 24; j++)
		{
			vertices[i * 24 + j] = currLevel;
		}
		for (int j = 0; j < 6; j++)
		{
			indices[i * 6 + j] = 0;
		}
	}
	int currLevelNumber = 1;

	while (currLevelNumber < numberOfLevels)
	{
		currLevel = verticalStep * (currLevelNumber);
		currRadius = sqrt(currLevel*1.2f); // can be changed
		for (size_t i = 0; i < numberOfEdges; i++)
		{
			// TODO add backtrace for prev sincos value
			// lower left corner

			vertices[currLevelNumber*numberOfEdges*24 + i * 24]      = vertices[(currLevelNumber - 1)*numberOfEdges * 24 + i * 24 + 6];
			vertices[currLevelNumber*numberOfEdges*24 + i * 24 + 1]  = vertices[(currLevelNumber - 1)*numberOfEdges * 24 + i * 24 + 7];
			vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 2]  = vertices[(currLevelNumber - 1)*numberOfEdges * 24 + i * 24 + 8];
			// normals 3,4,5
			// upper left corner
			if (i == 0)
			{
				vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 6]  = currRadius * glm::cos(glm::radians(i*horizontalStep));
				vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 7]  = currRadius * glm::sin(glm::radians(i*horizontalStep));
			}
			else
			{
				vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 6] = vertices[currLevelNumber*numberOfEdges * 24 + (i-1) * 24 + 18];
				vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 7] = vertices[currLevelNumber*numberOfEdges * 24 + (i-1) * 24 + 19];
			}
			vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 8]  = currLevel;
			// normals 9,10,11
			// lower right corner
			vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 12] = vertices[(currLevelNumber - 1)*numberOfEdges * 24 + i * 24 + 18];
			vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 13] = vertices[(currLevelNumber - 1)*numberOfEdges * 24 + i * 24 + 19];
			vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 14] = vertices[(currLevelNumber - 1)*numberOfEdges * 24 + i * 24 + 20];
			// normals 15,16,17
			// upper right corner
			vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 18] = currRadius * glm::cos(glm::radians((i+1)*horizontalStep));
			vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 19] = currRadius * glm::sin(glm::radians((i+1)*horizontalStep));
			vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 20] = currLevel;
			// normals 21,22,23
			glm::vec3 lowerLeft = glm::vec3(
				vertices[currLevelNumber*numberOfEdges * 24 + i * 24],
				vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 1],
				vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 2]
			);

			glm::vec3 upperLeft = glm::vec3(
				vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 6],
				vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 7],
				vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 8]
			);

			glm::vec3 upperRight = glm::vec3(
				vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 18],
				vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 19],
				vertices[currLevelNumber*numberOfEdges * 24 + i * 24 + 20]
			);


				// notice that we know that one of plane vectors always direct to up (0, 1, 0) for cylinder
			glm::vec3 normalVec = glm::normalize(
					glm::cross(upperRight - upperLeft, upperLeft - lowerLeft)
			);

			vertices[currLevelNumber*numberOfEdges*24 + i * 24 + 3]  = normalVec.x;
			vertices[currLevelNumber*numberOfEdges*24 + i * 24 + 4]  = normalVec.y;
			vertices[currLevelNumber*numberOfEdges*24 + i * 24 + 5]  = normalVec.z;
			vertices[currLevelNumber*numberOfEdges*24 + i * 24 + 9]  = normalVec.x;
			vertices[currLevelNumber*numberOfEdges*24 + i * 24 + 10] = normalVec.y;
			vertices[currLevelNumber*numberOfEdges*24 + i * 24 + 11] = normalVec.z;
			vertices[currLevelNumber*numberOfEdges*24 + i * 24 + 15] = normalVec.x;
			vertices[currLevelNumber*numberOfEdges*24 + i * 24 + 16] = normalVec.y;
			vertices[currLevelNumber*numberOfEdges*24 + i * 24 + 17] = normalVec.z;
			vertices[currLevelNumber*numberOfEdges*24 + i * 24 + 21] = normalVec.x;
			vertices[currLevelNumber*numberOfEdges*24 + i * 24 + 22] = normalVec.y;
			vertices[currLevelNumber*numberOfEdges*24 + i * 24 + 23] = normalVec.z;

			// set indices for this side
			
			indices[currLevelNumber*numberOfEdges * 6 + i * 6]     = currLevelNumber * numberOfEdges * 4 + i * 4;
			indices[currLevelNumber*numberOfEdges * 6 + i * 6 + 1] = currLevelNumber * numberOfEdges * 4 + i * 4 + 1;
			indices[currLevelNumber*numberOfEdges * 6 + i * 6 + 2] = currLevelNumber * numberOfEdges * 4 + i * 4 + 2;
			indices[currLevelNumber*numberOfEdges * 6 + i * 6 + 3] = currLevelNumber * numberOfEdges * 4 + i * 4 + 1;
			indices[currLevelNumber*numberOfEdges * 6 + i * 6 + 4] = currLevelNumber * numberOfEdges * 4 + i * 4 + 2;
			indices[currLevelNumber*numberOfEdges * 6 + i * 6 + 5] = currLevelNumber * numberOfEdges * 4 + i * 4 + 3;
		}
		
		//vertices[currLevelNumber] = prevLevelCurve[;

		currLevelNumber++;
	}

}

void Paraboloid::recalculate(float affectX, float affectY, float height, int numberOfEdges, int numberOfLevels)
{
	if (!isFigureLoaded())
		return;

	calculate(affectX, affectY, height, numberOfEdges, numberOfLevels);
	loadBuffers();
	freeData();
}

Paraboloid::~Paraboloid()
{
}
