#pragma once
#include "Figure.h"
#include <iostream>
#include "glm/glm.hpp"
class Cylinder :
	public Figure
{
	void calculate(float radius, float height, int numberOfEdges);
public:
	Cylinder(float radius, float height, int numberOfEdges);
	void recalculate(float radius, float height, int numberOfEdges);
	~Cylinder();
};

