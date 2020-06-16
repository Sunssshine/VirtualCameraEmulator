#include "Color.h"



void Color::calcNextColor()
{
	if (currRGBPart == 0)
	{
		if (currColor[0] > 0.99f && currColor[2] <= 0.01f)
		{
			currRGBPart += 1;
		}
		else
		{
			if (currColor[currRGBPart] < 1.00f)
			{
				currColor[currRGBPart] += 0.01f;
			}
			else
			{
				currColor[2] -= 0.01f;
			}
		}
	}
	else
	{
		if (currColor[currRGBPart] > 0.99f && currColor[currRGBPart - 1] <= 0.01f)
		{
			currRGBPart += 1;
			if (currRGBPart == 3)
			{
				currRGBPart = 0;
			}
		}
		else
		{
			if (currColor[currRGBPart] < 1.00f)
			{
				currColor[currRGBPart] += 0.01f;
			}
			else
			{
				currColor[currRGBPart - 1] -= 0.01f;
			}
		}
	}
}

Color::Color()
{
	currColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
}

Color::Color(glm::vec3 color)
{
	currColor = glm::vec4(color, 1.0f);
}

Color::Color(glm::vec4 color)
{
	currColor = color;
}

void Color::doColorShift(bool action)
{
	isColorShift = action;
}



glm::vec4 Color::getColor()
{
	if (isColorShift)
	{
		calcNextColor();
	}
	return currColor;
}

void Color::setColor(glm::vec4 color)
{
	currColor = color;
}

void Color::setColor(glm::vec3 color)
{
	currColor = glm::vec4(color, 1.0f);
}

void Color::set_R(float value)
{
	currColor.r = value;
}

void Color::set_G(float value)
{
	currColor.g = value;
}

void Color::set_B(float value)
{
	currColor.b = value;
}

void Color::set_A(float value)
{
	currColor.a = value;
}


Color::~Color()
{
}
