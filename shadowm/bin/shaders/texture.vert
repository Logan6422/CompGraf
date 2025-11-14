#version 330 core

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoords;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec4 lightPosition;

uniform mat4 lightViewMatrix;
uniform mat4 lightProjectionMatrix;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoords;
out vec4 lightVSPosition;
out vec4 fragPosLightSpace; //transforma el world space vertice positino al light space para usarlo en el fragment shader

//renderiza sombras 

//transformacion al light-space
void main() {
	mat4 vm = viewMatrix * modelMatrix;
	vec4 vmp = vm * vec4(vertexPosition,1.f);
	gl_Position = projectionMatrix * vmp;
	fragPosition = vec3(vmp)/vmp.w;
	fragNormal = mat3(transpose(inverse(vm))) * vertexNormal;
	
	lightVSPosition = viewMatrix * lightPosition;
	fragTexCoords = vertexTexCoords; // cordenada de textura del vertice, mapea 1:1 frag a texture
	mat4 lightSpaceMatrix = lightProjectionMatrix * lightViewMatrix;
	fragPosLightSpace = lightSpaceMatrix * modelMatrix * vec4(vertexPosition,1.f);	
}
