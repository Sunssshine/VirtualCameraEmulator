#include "Shader.h"

Shader::Shader(const GLchar * vertexPath, const GLchar * fragmentPath)
{
	// 1. Load shaders code from files
	std::string vertexCode;
	std::string fragmentCode;
	std::ifstream vShaderFile; // vertex shader file stream
	std::ifstream fShaderFile; // fragment shader file stream
	
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		vShaderFile.open(vertexPath);
		fShaderFile.open(fragmentPath);
		std::stringstream vShaderStream, fShaderStream;

		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		
		vShaderFile.close();
		fShaderFile.close();

		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
	}


	// 2. compile shaders
	unsigned int vertex, fragment;

	// vertex Shader compile
	vertex = compileShader(GL_VERTEX_SHADER, vertexCode.c_str());
	fragment = compileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str());
	
	// create program
	ID = createProgram(vertex, fragment);

	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

Shader::~Shader()
{
}

void Shader::use()
{
	glUseProgram(ID);
}

void Shader::setBool(const std::string & name, bool value) const
{
	glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string & name, int value) const
{
	glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string & name, float value) const
{
	glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setVec2(const std::string & name, float x, float y) const
{
	glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
}

void Shader::setVec2(const std::string & name, glm::vec2 vector) const
{
	glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &vector[0]);
}

void Shader::setVec3(const std::string & name, float x, float y, float z) const
{
	glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}

void Shader::setVec3(const std::string & name, glm::vec3 vector) const
{
	glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &vector[0]);
}

void Shader::setVec4(const std::string & name, float x, float y, float z, float w) const
{
	glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
}

void Shader::setVec4(const std::string & name, glm::vec4 vector) const
{
	glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &vector[0]);
}

void Shader::setMat2(const std::string & name, glm::mat2 matrix) const
{
	glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &matrix[0][0]);
}

void Shader::setMat3(const std::string & name, glm::mat3 matrix) const
{
	glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &matrix[0][0]);
}

void Shader::setMat4(const std::string & name, glm::mat4 matrix) const
{
	glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &matrix[0][0]);
}

unsigned int Shader::compileShader(GLenum shaderType, const char * shaderCode)
{
	unsigned int shader;
	int success;
	char infoLog[1024];
	shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderCode, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 1024, NULL, infoLog);
		switch (shaderType)
		{
		case GL_VERTEX_SHADER:
		{
			std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
			break;
		}
		case GL_FRAGMENT_SHADER:
		{
			std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
			break;
		}
		default:
			break;
		}
	}

	return shader;
}

unsigned int Shader::createProgram(unsigned int vertexShader, unsigned int fragmentShader)
{
	int success;
	char infoLog[1024];
	int programID = glCreateProgram();
	glAttachShader(programID, vertexShader);
	glAttachShader(programID, fragmentShader);
	glLinkProgram(programID);

	glGetProgramiv(programID, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programID, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	return programID;
}
