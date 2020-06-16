#ifndef SHADER_H
#define SHADER_H

#include <GL/gl3w.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include "glm/glm.hpp"

class Shader
{
public:
	// the program ID
	unsigned int ID;

	// constructor reads and builds the shader
	Shader(const GLchar* vertexPath, const GLchar* fragmentPath);
	~Shader();

	void use();

	void setBool(const std::string &name, bool value) const;
	void setInt(const std::string &name, int value) const;
	void setFloat(const std::string &name, float value) const;

	void setVec2(const std::string &name, float x, float y) const;
	void setVec2(const std::string &name, glm::vec2 vector) const;

	void setVec3(const std::string &name, float x, float y, float z) const;
	void setVec3(const std::string &name, glm::vec3 vector) const;

	void setVec4(const std::string &name, float x, float y, float z, float w) const;
	void setVec4(const std::string &name, glm::vec4 vector) const;

	void setMat2(const std::string &name, glm::mat2 matrix) const;
	void setMat3(const std::string &name, glm::mat3 matrix) const;
	void setMat4(const std::string &name, glm::mat4 matrix) const;
private:
	unsigned int compileShader(GLenum shaderType, const char * shaderCode);
	unsigned int createProgram(unsigned int vertexShader, unsigned int fragmentShader);
};

#endif

