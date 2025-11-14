in vec3 fragNormal;
in vec3 fragPosition;
in vec2 fragTexCoords;
in vec4 lightVSPosition;
in vec4 fragPosLightSpace; // frag pov luz

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

// una vez tenido el mapa de profundidad
// la funcion valida si el fragmento esta en una sombra (en el frag shader)
// PERO SE HACE UNA TRANSFORMACION DE LA MATRIZ DE LUZ


// shadow calc se encarga de todo (o almost)
/// REPRESENTACION DE LAS SOMBRAS
float calcShadow(vec4 fragPosLightSpace) {
	vec3 projCoords = vec3(fragPosLightSpace) / fragPosLightSpace.w; // normalizar las cords del frag (transformar pos del frag espacio-luz en espacio de recorte)
	// esto devuelve posicion de fragmentos del espacio-luz en [-1,1] <- espacio de recorte
	
	projCoords = projCoords * 0.5 + 0.5; // depth map esta 0,1 asi q se ajusta el projCords
	
	// Si estamos fuera del rango válido, no calculamos sombras
	if(projCoords.z > 1.0 || projCoords.z < 0.0) return -2.0;
	
	float currentDepth = projCoords.z; // depth de la luz
	// calculamos el bias 
	float bias = max(0.05 * (1.0 - dot(fragNormal, normalize(-lightVSPosition.xyz))), 0.005);
	
	
	// PCF: suavizado mediante promedio
	// tamaño de cada texel en el mapa de sombras
	//texture size devuelve el ancho y alto de la textura de muestra del mimap 0
	//texture size devuelve el tamanio de un solo texel
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(depthTexture, 0); 
	for(int x = -1; x <= 1; ++x) { 
		for(int y = -1; y <= 1; ++y) { //recorre un texel de 3x3 (tamanio estandar)
			vec2 offset = vec2(x, y) * texelSize;  //offset relativo de texel convierte el desplazamiento de unidades normalizadas a espacio de textura UV
 			float closestDepth = texture(depthTexture, projCoords.xy + offset).s; //obtiene la profundidad de (r pq el mapa de sombras
																				//es un valor en escala de grises almacenado en el canal rojo)
			// profundidad alctual desde la perspectiva de la luz
			// valida cual frag actual esta en sombra
			shadow += (currentDepth - bias > closestDepth ? 1.0 : 0.0); //Comparar la profundidad del fragmento con la profundidad del texel y acumular si está en sombra.
		}
	}
	shadow /= 9.0; // Promediamos los 9 muestreos
	
	return shadow;
}

// Utilizando blin phong
// esto se hace al final del fragment shader 
// la difusa y especular se multiplica 
// por 0 o 1 (1 sombra, 0 sino)<- componente de la sombra 
// componente ambiente fuera de la ecuacion

vec4 calcColor(vec3 textureColor) {
	// blinn phong
	vec3 phong = calcPhong(lightVSPosition, lightColor, ambientStrength,
						   ambientColor*textureColor, diffuseColor*textureColor,
						   specularColor, shininess);
	
	float shadow = calcShadow(fragPosLightSpace); // calculo la sombra 
	
	// inversa de la componente sombra "cuanto NO esta en sombra"
	if(shadow < 0) { 
		if (shadow==-1) return vec4(1.f,0.f,0.f,1.f);
		else            return vec4(0.f,1.f,0.f,1.f);
	}
	
	
	vec3 ambientComponent = ambientColor*textureColor*ambientStrength;
	vec3 lighting = mix(phong, ambientComponent, shadow);
	return vec4(lighting+emissionColor,opacity);
}

// BIAS: es un valor pequeño para corregir errores visuales causados por las limitaicones de precision en la precision de 
// calculos de profundidad al determinar si un fragmento esta sombreado o no
// el SHADOW ACNE ocurre debido a la comparacion entre profundidad almacenada
// en el shadow map y la profundidad del fragmento desde la perspectiva de la luz
// UN BIAS DEMASIADO GRANDE PROVOCA EL PETER PANNING (sombras desplazadas?)


/* 
1) la camara de vista del espectador esta en -1,1 pero en la matriz de la luz de proyeccion esta de 0,1

2)Precisión limitada: Los valores de profundidad suelen estar representados en un buffer de 24 bits, 
lo que puede no ser suficiente para escenas grandes o detalles muy cercanos a la cámara, 
generando errores de redondeo y comparación.
*/


//PROBLEMA:
//SHADOW ACNE
//El shadow map esta limitado por la resolucion, multiples fragmentos pueden utilizar el mismo valor del depth map cuando estan suficientemente lejos del source light
//SOLUCION:
//SHADOW BIAS
//Se aplica un offset a la profundidad de la superficie (o shadow map) de forma tal que los fragmentos no se consideran sobre la superficie.

//PROBLEMA:
//PETER PANNING
//Una desventaja del shadow bias es que aplicas el oofest a la profundidad real de los objetos
//como resultado se puede ver un desface en las sombras comparadas con la posicion actual del objeto1

////PROBLEMA:
//PCF
//Como el depth map tiene una resolucion fija la profundidad suele abarcar mas de un fragmento por texel por ende varios fragmentos toman muestras del mismo valor de profundidad
//del mapa y llegan a las mismas sombras 
//SOLUCION:
//Incrementar la resolucion del depth map o tratando de ajustar el frustrum de la luz lo mas posible a la escena
//Otra solucion parcial es PCF o PERCENTAJE CLOSER FILTERING
//Agrupa funciones de filtrado que producen sombras mas suaves // la idea es samplear mas de una vez el depth map, cada vez con coordenadas de textura un poco diferente
//por cada chequeo se fija si esta en sombra y todos los sub resultados son combinados en un resultado average que da una sombra mas suave.
