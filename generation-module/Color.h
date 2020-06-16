#pragma once
#include "glm/glm.hpp"
class Color
{
	glm::vec4 currColor;
	int currRGBPart = 0;
	bool isColorShift = false;
	void calcNextColor();
public:
	Color();
	Color(glm::vec3 color);
	Color(glm::vec4 color);
	void doColorShift(bool isShift);
	glm::vec4 getColor();
	void setColor(glm::vec4 color);
	void setColor(glm::vec3 color);
	void set_R(float value);
	void set_G(float value);
	void set_B(float value);
	void set_A(float value);
	~Color();
};

