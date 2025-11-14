#include <algorithm>
#include <stdexcept>
#include <vector>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Model.hpp"
#include "Window.hpp"
#include "Callbacks.hpp"
#include "Debug.hpp"
#include "Shaders.hpp"
#include "SubDivMesh.hpp"
#include "SubDivMeshRenderer.hpp"

#define VERSION 20251006

// models and settings
std::vector<std::string> models_names = { "cubo", "icosahedron", "plano", "suzanne", "star" };
int current_model = 0;
bool fill = true, nodes = true, wireframe = true, smooth = false, 
	 reload_mesh = true, mesh_modified = false;

// extraa callbacks
void keyboardCallback(GLFWwindow* glfw_win, int key, int scancode, int action, int mods);

SubDivMesh mesh;
void subdivide(SubDivMesh &mesh);

int main() {
	
	// initialize window and setup callbacks
	Window window(800, 600, "CG Demo");
	setCommonCallbacks(window);
	glfwSetKeyCallback(window, keyboardCallback);
	CameraSettings &cs = window.getCamera();
	cs.view_fov = 60.f;
	
	// setup OpenGL state and load shaders
	glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); 
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.8f,0.8f,0.9f,1.f);
	Shader shader_flat("shaders/flat"),
	       shader_smooth("shaders/smooth"),
		   shader_wireframe("shaders/wireframe");
	SubDivMeshRenderer renderer;
	
	// main loop
	Material material;
	material.ka = material.kd = glm::vec3{.8f,.4f,.4f};
	material.ks = glm::vec3{.5f,.5f,.5f};
	material.shininess = 50.f;
	
	FrameTimer timer;
	do {
		
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		if (reload_mesh) {
			mesh = SubDivMesh("models/"+models_names[current_model]+".dat");
			reload_mesh = false; mesh_modified = true;
		}
		if (mesh_modified) {
			renderer = makeRenderer(mesh,false);
			mesh_modified = false;
		}
		
		if (nodes) {
			shader_wireframe.use();
			setMatrixes(window, shader_wireframe);
			renderer.drawPoints(shader_wireframe);
		}
		
		if (wireframe) {
			shader_wireframe.use();
			setMatrixes(window, shader_wireframe);
			renderer.drawLines(shader_wireframe);
		}
		
		if (fill) {
			Shader &shader = smooth ? shader_smooth : shader_flat;
			shader.use();
			setMatrixes(window, shader);
			shader.setLight(glm::vec4{2.f,1.f,5.f,0.f}, glm::vec3{1.f,1.f,1.f}, 0.25f);
			shader.setMaterial(material);
			renderer.drawTriangles(shader);
		}
		
		// settings sub-window
		window.ImGuiDialog("CG Example",[&](){
			if (ImGui::Combo(".dat (O)", &current_model,models_names)) reload_mesh = true;
			ImGui::Checkbox("Fill (F)",&fill);
			ImGui::Checkbox("Wireframe (W)",&wireframe);
			ImGui::Checkbox("Nodes (N)",&nodes);
			ImGui::Checkbox("Smooth Shading (S)",&smooth);
			if (ImGui::Button("Subdivide (D)")) { subdivide(mesh); mesh_modified = true; }
			if (ImGui::Button("Reset (R)")) reload_mesh = true;
			ImGui::Text("Nodes: %i, Elements: %i",mesh.n.size(),mesh.e.size());
		});
		
		// finish frame
		window.finishFrame();
		
	} while( glfwGetKey(window,GLFW_KEY_ESCAPE)!=GLFW_PRESS && !glfwWindowShouldClose(window) );
}

void keyboardCallback(GLFWwindow* glfw_win, int key, int scancode, int action, int mods) {
	if (action==GLFW_PRESS) {
		switch (key) {
		case 'D': subdivide(mesh); mesh_modified = true; break;
		case 'F': fill = !fill; break;
		case 'N': nodes = !nodes; break;
		case 'W': wireframe = !wireframe; break;
		case 'S': smooth = !smooth; break;
		case 'R': reload_mesh=true; break;
		case 'O': case 'M': current_model = (current_model+1)%models_names.size(); reload_mesh = true; break;
		}
	}
}

// La struct Arista guarda los dos indices de nodos de una arista
// Siempre pone primero el menor indice, para facilitar la búsqueda en lista ordenada;
//    es para usar con el Mapa de más abajo, para asociar un nodo nuevo a una arista vieja
struct Arista {
	int n[2];
	Arista(int n1, int n2) {
		n[0]=n1; n[1]=n2;
		if (n[0]>n[1]) std::swap(n[0],n[1]);
	}
	Arista(Elemento &e, int i) { // i-esima arista de un elemento
		n[0]=e[i]; n[1]=e[i+1];
		if (n[0]>n[1]) std::swap(n[0],n[1]); // pierde el orden del elemento
	}
	const bool operator<(const Arista &a) const {
		return (n[0]<a.n[0]||(n[0]==a.n[0]&&n[1]<a.n[1]));
	}
};

// Mapa sirve para guardar una asociación entre una arista y un indice de nodo (que no es de la arista)
using Mapa = std::map<Arista,int>;

std::vector<int> comunes(const std::vector<int>& a, const std::vector<int>& b) {
	std::vector<int> resultado;
	for (int i = 0; i < a.size(); ++i) {
		for (int j = 0; j < b.size(); ++j) {
			if (a[i] == b[j]) {
				bool ya_esta = false;
				for (int k = 0; k < resultado.size(); ++k) {
					if (resultado[k] == a[i]) {
						ya_esta = true;
						break;
					}
				}
				if (!ya_esta)
					resultado.push_back(a[i]);
			}
		}
	}
	return resultado;
}

void subdivide(SubDivMesh &mesh) {
	int size_orig = mesh.n.size();
	
	/// @@@@@: Implementar Catmull-Clark... lineamientos:
	//  Los nodos originales estan en las posiciones 0 a #n-1 de m.n,n22
	//  Los elementos orignales estan en las posiciones 0 a #e-1 de m.e
	//  1) Por cada elemento, agregar el centroide (nuevos nodos: #n a #n+#e-1)
	
	
	
	int size_mesh = mesh.e.size();
	for(size_t i=0;i<size_mesh;i++) { 
		glm::vec3 sum(0,0,0);
		
		Elemento e_actual = mesh.e[i];
		for(int j=0;j<e_actual.nv;j++) {
			
			Nodo nodo_actual = mesh.n[e_actual.n[j]];
			sum+= glm::vec3(nodo_actual.p[0],nodo_actual.p[1],nodo_actual.p[2]);
		}
		sum /= float(e_actual.nv);
		Nodo nuevo(sum);
		mesh.n.push_back(nuevo);
	}	
	//  2) Por cada arista de cada cara, agregar un pto en el medio que es
	//      promedio de los vertices de la arista y los centroides de las caras 
	//      adyacentes. Aca hay que usar los elementos vecinos.
	//      En los bordes, cuando no hay vecinos, es simplemente el promedio de los 
	//      vertices de la arista
	//      Hay que evitar procesar dos veces la misma arista (como?)
	//      Mas adelante vamos a necesitar determinar cual punto agregamos en cada
	//      arista, y ya que no se pueden relacionar los indices con una formula simple
	//      se sugiere usar Mapa como estructura auxiliar
	Mapa map;	

	
	size_mesh = mesh.e.size();
	for(size_t i=0;i<size_mesh;i++) {
		
		int size_elemento = mesh.e[i].nv;
		
		for(int j=0;j<size_elemento;j++) { 
			
			
			Elemento e_actual = mesh.e[i];
			int a = e_actual.n[j];
			int b = e_actual.n[(j + 1) % e_actual.nv];
			
			
			Arista actual(a,b); //crea arista actual
			if(map.find(actual) == map.end()){
				int indice_arista_1 = actual.n[0];
				int indice_arista_2 = actual.n[1];
				
				std::vector<int>element_comun =  comunes(mesh.n[indice_arista_1].e,mesh.n[indice_arista_2].e);
				int size_nodos = mesh.n.size()-1;
				
				if (element_comun.size() == 2) {
					
					
					
					Nodo n1 = mesh.n[indice_arista_1];
					Nodo n2 = mesh.n[indice_arista_2];
					
					Nodo centroide1 = mesh.n[size_orig+element_comun[0]];
					Nodo centroide2 = mesh.n[size_orig+element_comun[1]];
					
					glm::vec3 sum = n1.p+n2.p+centroide1.p+centroide2.p; 
					sum /= 4.0f;
					
					Nodo nuevo(sum);
					mesh.n.push_back(nuevo);
					map[actual] = size_nodos;
				} else {
					Elemento e_actual = mesh.e[i];
					glm::vec3 sum = (mesh.n[e_actual.n[j]].p + mesh.n[e_actual.n[(j + 1) % e_actual.nv]].p) / 2.0f;
					Nodo nuevo(sum);
					mesh.n.push_back(nuevo);
					map[actual] = size_nodos;			
				}
				
			}
			
		}
	}
	
	//  3) Armar los elementos nuevos
	//      Los quads se dividen en 4, (uno reemplaza al original, los otros 3 se agregan)
	//      Los triangulos se dividen en 3, (uno reemplaza al original, los otros 2 se agregan)
	//      Para encontrar los nodos de las aristas usar el mapa que armaron en el paso 2
	//      Ordenar los nodos de todos los elementos nuevos con un mismo criterio (por ej, 
	//      siempre poner primero al centroide del elemento), para simplificar el paso 4.
	int size_post = mesh.e.size();
	int size_nodos = mesh.n.size();

	for (size_t i = 0; i < size_post; ++i) {
		int i_centroide = size_orig + i; // índice del nodo centroide del elemento original
		Elemento e_actual = mesh.e[i];
		
		for(int j=0;j<e_actual.nv;j++) { 
			int a = e_actual.n[j];
			int b = e_actual.n[(j + 1) % e_actual.nv];
			Arista actual(a,b); //crea arista actual
			
			int c = e_actual.n[(j + 1) % e_actual.nv];
			int d = e_actual.n[(j + 2) % e_actual.nv];
			Arista siguiente(c,d);
			
			int i_arista1 = map[actual];
			int i_arista2 = map[siguiente];
			
			int i_compartido = e_actual.n[(j + 1) % e_actual.nv];
			
			
			if (j == 0) {
				mesh.reemplazarElemento(i, i_centroide, i_arista1, i_compartido, i_arista2);
			} else {
				mesh.agregarElemento(i_centroide, i_arista1, i_compartido ,i_arista2);
				
			}
			mesh.makeVecinos();
		}	
		
	}
	
	for(int i=0;i<size_nodos;i++) { 
		Nodo nodo_actual = mesh.n[i];
		glm::vec3 r = {0,0,0}; 
		glm::vec3 res = {0,0,0};
		glm::vec3 f = {0,0,0};
		glm::vec3 p = {0,0,0};
		
		if(!nodo_actual.es_frontera){
			
			float n = mesh.n[i].e.size();
			
			for(int j=0;j<n;j++) {
				int indice_elemento_actual = nodo_actual.e[j];
				Elemento elemento_actual = (mesh.e[indice_elemento_actual]);
				Nodo centroide_elemento_actual = mesh.n[elemento_actual.n[0]];
				Nodo arista1_elemento_actual = mesh.n[elemento_actual.n[1]];
				
				
				f+= centroide_elemento_actual.p;
				r+= arista1_elemento_actual.p;
				
				p = nodo_actual.p;
			}	
			f /= n;
			r /= n;
			r *= 4.0f;
			p *= n-3;
			
			nodo_actual.p = r - f + p;
			nodo_actual.p /= n;
		}
		else{
			if(nodo_actual.e.size() == 1){
				int indice_arista1_elem_actual = (mesh.e[mesh.n[i].e[0]].n[1]);
				int indice_arista2_elem_actual = (mesh.e[mesh.n[i].e[0]].n[3]);
				
				//r = promedio entre los puntos asociados a las aristas originales 
				glm::vec3 r = {0,0,0}; 
				r += mesh.n[indice_arista1_elem_actual].p;
				r += mesh.n[indice_arista2_elem_actual].p;
				r /= 2.0f;
				
				glm::vec3 p = nodo_actual.p; // p = posicion nodo actual
				
				glm::vec3 res = {0,0,0};
				
				res += r; //r
				res += p; // (r+p)
				res /= 2.0f; //(r+p)/2
				
				nodo_actual.p = res; //nueva posicion
			}
			else{
				glm::vec3 r = {0,0,0}; 
				glm::vec3 res = {0,0,0};
				glm::vec3 p = {0,0,0};
				
				int size_elemento_actual = mesh.n[i].e.size();
				
				
				
				for(int j=0;j<size_elemento_actual;j++) {
					
					Nodo nodo_actual = mesh.n[i];
					
					int indice_arista1_elemento = (mesh.e[mesh.n[i].e[j]].n[1]);
					int indice_arista2_elemento = (mesh.e[mesh.n[i].e[j]].n[3]);
					Nodo cent1 = mesh.n[indice_arista1_elemento];
					Nodo cent2 = mesh.n[indice_arista2_elemento];
					
					if(mesh.n[indice_arista1_elemento].es_frontera){
						r += cent1.p;
					}else{
						r += cent2.p;
					}
				}	
				r /= 2.0f; //promedio
				
				p = nodo_actual.p;
				
				nodo_actual.p = p+r;
				nodo_actual.p /= 2;
			}
		}
		
	}

//	//  4) Calcular las nuevas posiciones de los nodos originales
	//      Para nodos interiores: (4r-f+(n-3)p)/n
	//         f=promedio de nodos interiores de las caras (los agregados en el paso 1)
	//         r=promedio de los pts medios de las aristas (los agregados en el paso 2)
	//         p=posicion del nodo original
	//         n=cantidad de elementos para ese nodo

	mesh.verificarIntegridad();
}
