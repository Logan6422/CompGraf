#version 330 core
//shader simple que solo transforma los vertices al espacio del pov luz
in vec3 vertexPosition;

uniform mat4 modelMatrix; // coloca el objeto en la escena (world space)

uniform mat4 lightViewMatrix; //matriz que determina la direccion a la que mira la luz
uniform mat4 lightProjectionMatrix; 


/// render del mapa de profundidad
void main() {
	mat4 lightSpaceMatrix = lightProjectionMatrix * lightViewMatrix;
	// la combinacion da una matriz de transformacion del espacio-luz que transforma
	// cada VERTICE del world-space en el espacio visible desde la fuente de luz
	// (lo que necesitamos para representar el depth map)
	
	gl_Position = lightSpaceMatrix * modelMatrix * vec4(vertexPosition,1.f); 
//	// toma un modelo por objeto, un vertice y transforma todos los vertices en el espacio-luz
	// usando el lightSpaceMatrix (DEFINIDO PREVIAMENTE)
	// (en resumen toma un modelo y lo transforma al espacio-luz con la matriz creada anterior)
}
