#version 330 core

// propiedades del material
uniform vec3 objectColor;
uniform float shininess;
uniform float specularStrength;

// propiedades de la luz
uniform float ambientStrength;
uniform vec3 lightColor;
uniform vec4 lightPosition;

// propiedades de la camara
uniform vec3 cameraPosition;

// propiedades del fragmento interpoladas por el vertex shader
in vec3 fragNormal;
in vec3 fragPosition;

// resultado
out vec4 fragColor;

// phong simplificado
void main() {	// ambient
	vec3 ambient = lightColor * ambientStrength * objectColor ;
	
	// diffuse
	vec3 norm = normalize(fragNormal);
	vec3 lightDir = normalize(vec3(lightPosition)); // luz directional (en el inf.)
/*	vec3 diffuse = lightColor * objectColor * max(dot(norm,lightDir),0.f);*/
	
	
	
	float val = (max(dot(norm,lightDir),0.f));
	vec3 diffuse;
	
	if((val < 1) && (val > 0.66)){
		diffuse = lightColor * objectColor * 1;
	}else if((val < 0.77) && (val > 0.33)){
		diffuse = lightColor * objectColor * 0.5;
	}else{
		diffuse = lightColor * objectColor * 0;
	}
	
	
	
	// specular
	vec3 specularColor = specularStrength * vec3(1.f,1.f,1.f);
	vec3 viewDir = normalize(cameraPosition-fragPosition);
	vec3 halfV = normalize(lightDir + viewDir); // blinn
	vec3 specular /*= lightColor * specularColor * pow(max(dot(norm,halfV),0.f),shininess)*/;
	float valSpec = pow(max(dot(norm,halfV),0.f),shininess);

	if((valSpec < pow(1,shininess)) && (valSpec > pow(0.6,shininess))){
		specular = lightColor * specularColor * 1;
	}else if((valSpec < pow(0.6,shininess)) && (valSpec > pow(0.33,shininess))){
		specular = lightColor * specularColor * 0.5;
	}else{
		specular = lightColor * specularColor * 0.2;
	}
	
	
	
	// result
	fragColor = vec4(ambient+diffuse+specular,1.f);
}




