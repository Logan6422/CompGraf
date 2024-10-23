# version 330 core
in vec2 fragTexCoords;

out vec4 fragColor;

void main() {
	float repeatX = mod(fragTexCoords.x, 1.0); // Limita X entre 0 y 1
	float repeatY = mod(fragTexCoords.y, 1.0); // Limita Y entre 0 y 1
	
	
	// Asignamos las coordenadas de textura como color
	fragColor = vec4(repeatX, 0.0, repeatY, 1.0);
}
