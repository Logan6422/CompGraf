# version 330 core
in vec2 fragTexCoords;

out vec4 fragColor;

void main() {
	/// @@TODO: Use tex coords as color
	
	// Escalar los valores de fragTexCoords
	float red = clamp(fragTexCoords.x * 255.0, 0.0, 255.0); // Color Rojo
	float blue = clamp(fragTexCoords.y * 255.0, 0.0, 255.0); // Color Azul

	
	// Asignar el color final
	fragColor = vec4(red / 255.0, 0.0, blue / 255.0, 1.0);
	
}
