#pragma once
#include "Figure.h"
#include "glm/glm.hpp"
#include <vector>
class Paraboloid :
	public Figure
{
	void calculate(float affectX, float affectY, float height, int numberOfEdges, int numberOfLevels);
public:
	Paraboloid(float affectX, float affectY, float height, int numberOfEdges, int numberOfLevels);
	void recalculate(float affectX, float affectY, float height, int numberOfEdges, int numberOfLevels);
	~Paraboloid();
};

