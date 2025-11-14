in vec3 fragNormal;
in vec3 fragPosition;
in vec2 fragTexCoords;
in vec4 lightVSPosition;
in vec4 fragPosLightSpace;

uniform sampler2D depthTexture;
uniform vec3 ambientColor;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform vec3 emissionColor;
uniform float shininess;
uniform float opacity;

uniform float ambientStrength;
uniform vec3 lightColor;

out vec4 fragColor;

#include "calcPhong.frag"

float calcShadow(vec4 fragPosLightSpace) {
	vec3 projCoords = vec3(fragPosLightSpace) / fragPosLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;
	
	float currentDepth = projCoords.z;
	
	float closestDepth = texture(depthTexture, projCoords.xy).r;
	return currentDepth > closestDepth ? 1.0 : 0.0;
	
}

vec4 calcColor(vec3 textureColor) {
	vec3 phong = calcPhong(lightVSPosition, lightColor, ambientStrength,
						   ambientColor*textureColor, diffuseColor*textureColor,
						   specularColor, shininess);
	
	float shadow = calcShadow(fragPosLightSpace);
	vec3 ambientComponent = ambientColor*textureColor*ambientStrength;
	vec3 lighting = mix(phong, ambientComponent, shadow);
	return vec4(lighting+emissionColor,opacity);
}
