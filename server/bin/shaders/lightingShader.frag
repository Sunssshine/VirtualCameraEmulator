#version 330 core
out vec4 FragColor;
  
uniform vec3 objectColor;
uniform vec3 lightColor;

//uniform vec3 viewPos;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
}; 
  
uniform Material material;

in vec3 lightPos;
in vec3 Normal;
in vec3 FragPos;

void main()
{
	vec3 viewPos = vec3(0.0f, 0.0f, 0.0f);
	// calc ambient light part
	vec3 ambientPart = lightColor * material.ambient;
	
	// calc diffuse light part
	float diffuseStrength = 1.0;
    vec3 normalVector = normalize(Normal);
	vec3 lightDirection = normalize(lightPos - FragPos);
	float diffuseInclination = max(dot(normalVector, lightDirection), 0.0);
	vec3 diffusePart = (diffuseInclination * material.diffuse) * lightColor;

	// calc specular light part
	float specularStrength = 0.5;
	vec3 viewDirection = normalize(vec3(0.0f) - FragPos);
	vec3 reflectDirection = reflect(-lightDirection, normalVector);
	float specularInclination = max(dot(viewDirection, reflectDirection), 0.0);
	float specular = pow(specularInclination, material.shininess);
	vec3 specularPart = (material.specular * specular) * lightColor;


	// calc result color
	vec3 result = (diffusePart + ambientPart + specularPart) * objectColor;

	//calc alpha
	float viewDirLength = length(viewPos - FragPos);
	float newAlpha = viewDirLength;

    FragColor = vec4(result, pow(newAlpha,2));
}