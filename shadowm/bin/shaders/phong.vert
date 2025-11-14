#version 330 core
/// in y uniform vienen dados como dato
in vec3 vertexPosition;
in vec3 vertexNormal;

uniform mat4 modelMatrix; // trasnforma space-model a world-space
uniform mat4 viewMatrix; // transforma del world-space al view-space 
uniform mat4 projectionMatrix; // matriz de proyeccion (pasa de world al clip, donde se aplica el culling y demas)
uniform vec4 lightPosition; // pos de la luz = coordenadas

/// ambas declaradas en drawScene
uniform mat4 lightViewMatrix; // matriz de la luz q apunta al centro
uniform mat4 lightProjectionMatrix; // matriz proj ortogonal

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoords;
out vec4 lightVSPosition;
out vec4 fragPosLightSpace; // utiliza la lightSpaceMatrix, frag visto desde el pov de la luz
//phong para calcular la transfrmacion del espacio-luz
void main() {
	
	mat4 vm = viewMatrix * modelMatrix; // combina vista y modelo para aplicar transformacion
	vec4 vmp = vm * vec4(vertexPosition,1.f); // camara + modelo-espacio
	gl_Position = projectionMatrix * vmp; // pov de la camara, transformacion al view-space
	fragPosition = vec3(vmp)/vmp.w; // transformacion proyectiva 
	fragNormal = mat3(transpose(inverse(vm))) * vertexNormal; // normal del fragmento
	
	lightVSPosition = viewMatrix * lightPosition;  // pos del vertice visto desde el pov de la luz
	mat4 lightSpaceMatrix = lightProjectionMatrix * lightViewMatrix; // (frag) matriz q transforma del view-space a light-space
																	// ortogonal * pov de la luz
	fragPosLightSpace = lightSpaceMatrix * 
						modelMatrix * 
						vec4(vertexPosition,1.f); // usa la mat de tranf, model es individual (es INDEPENDIENTE de ambas vistas)
												 //  se esta viendo la escena desde el pov de la luz
													// agarra la posicion del vertice, el modelo y lo transforma
}
