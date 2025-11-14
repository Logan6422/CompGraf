#version 330 core

void main()
{
	// gl_FragDepth = gl_FragCoord.z;
}
//como no hay buffer de color los fragmentos resultantes no necesitan ningun 
// procesamiento -> se tiene un fragment shader vacio
/// al final de su ejecucion se actualiza el buffer de profundidad
