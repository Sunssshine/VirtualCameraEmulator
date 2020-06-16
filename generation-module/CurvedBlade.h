#pragma once
#include "Figure.h"
#include "glm/glm.hpp"
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/spline.hpp"
#include "glm/gtx/rotate_vector.hpp"

class CurvedBlade :
	public Figure
{
public:
	CurvedBlade(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, float twist, int levelsNum);
	void calculate(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, float twist, int levelsNum);
	void recalculate(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, float twist, int levelsNum);
	~CurvedBlade();
};

