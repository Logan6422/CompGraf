#version 330 core

in vec3 vertexPosition;
in vec3 vertexNormal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec4 lightPosition;

out vec3 fragPosition;
out vec3 fragNormal;
out vec4 lightVSPosition;

void main() {
	mat4 viewModelMatrix = viewMatrix * modelMatrix;
	vec4 vmPos = viewModelMatrix * vec4(vertexPosition,1.f);
	gl_Position = projectionMatrix * vmPos;
	fragPosition = vec3(vmPos);
	fragNormal = mat3(transpose(inverse(viewModelMatrix))) * vertexNormal;
	lightVSPosition = viewMatrix * lightPosition;
}
