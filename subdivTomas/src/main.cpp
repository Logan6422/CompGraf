#include <algorithm>
#include <stdexcept>
#include <vector>
#include <string>
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
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
std::vector<std::string> models_names = {"cubo", "icosahedron", "plano",
                                         "suzanne", "star"};
int current_model = 0;
bool fill = true, nodes = true, wireframe = true, smooth = false,
     reload_mesh = true, mesh_modified = false;

// extraa callbacks
void keyboardCallback(GLFWwindow* glfw_win, int key, int scancode, int action,
                      int mods);

SubDivMesh mesh;
void subdivide(SubDivMesh& mesh);
glm::vec3 interpolar(int* n, int largo, SubDivMesh& mesh);

int main() {

    // initialize window and setup callbacks
    Window window(800, 600, "CG Demo");
    setCommonCallbacks(window);
    glfwSetKeyCallback(window, keyboardCallback);
    CameraSettings& cs = window.getCamera();
    cs.view_fov = 60.f;

    // setup OpenGL state and load shaders
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.8f, 0.8f, 0.9f, 1.f);
    Shader shader_flat("shaders/flat"), shader_smooth("shaders/smooth"),
        shader_wireframe("shaders/wireframe");
    SubDivMeshRenderer renderer;

    // main loop
    Material material;
    material.ka = material.kd = glm::vec3{.8f, .4f, .4f};
    material.ks = glm::vec3{.5f, .5f, .5f};
    material.shininess = 50.f;

    FrameTimer timer;
    do {

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (reload_mesh) {
            mesh = SubDivMesh("models/" + models_names[current_model] + ".dat");
            reload_mesh = false;
            mesh_modified = true;
        }
        if (mesh_modified) {
            renderer = makeRenderer(mesh, false);
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
            Shader& shader = smooth ? shader_smooth : shader_flat;
            shader.use();
            setMatrixes(window, shader);
            shader.setLight(glm::vec4{2.f, 1.f, 5.f, 0.f},
                            glm::vec3{1.f, 1.f, 1.f}, 0.25f);
            shader.setMaterial(material);
            renderer.drawTriangles(shader);
        }

        // settings sub-window
        window.ImGuiDialog("CG Example", [&]() {
            if (ImGui::Combo(".dat (O)", &current_model, models_names))
                reload_mesh = true;
            ImGui::Checkbox("Fill (F)", &fill);
            ImGui::Checkbox("Wireframe (W)", &wireframe);
            ImGui::Checkbox("Nodes (N)", &nodes);
            ImGui::Checkbox("Smooth Shading (S)", &smooth);
            if (ImGui::Button("Subdivide (D)")) {
                subdivide(mesh);
                mesh_modified = true;
            }
            if (ImGui::Button("Reset (R)"))
                reload_mesh = true;
            ImGui::Text("Nodes: %i, Elements: %i", mesh.n.size(),
                        mesh.e.size());
        });

        // finish frame
        window.finishFrame();

    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
             !glfwWindowShouldClose(window));
}

void keyboardCallback(GLFWwindow* glfw_win, int key, int scancode, int action,
                      int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case 'D':
            subdivide(mesh);
            mesh_modified = true;
            break;
        case 'F':
            fill = !fill;
            break;
        case 'N':
            nodes = !nodes;
            break;
        case 'W':
            wireframe = !wireframe;
            break;
        case 'S':
            smooth = !smooth;
            break;
        case 'R':
            reload_mesh = true;
            break;
        case 'O':
        case 'M':
            current_model = (current_model + 1) % models_names.size();
            reload_mesh = true;
            break;
        }
    }
}

// La struct Arista guarda los dos indices de nodos de una arista
// Siempre pone primero el menor indice, para facilitar la búsqueda en lista
// ordenada;
//    es para usar con el Mapa de más abajo, para asociar un nodo nuevo a una
//    arista vieja
struct Arista {
    int n[2];
    Arista(int n1, int n2) {
        n[0] = n1;
        n[1] = n2;
        if (n[0] > n[1])
            std::swap(n[0], n[1]);
    }
    Arista(Elemento& e, int i) { // i-esima arista de un elemento
        n[0] = e[i];
        n[1] = e[(i + 1) % e.nv];
        if (n[0] > n[1])
            std::swap(n[0], n[1]); // pierde el orden del elemento
    }
    const bool operator<(const Arista& a) const {
        return (n[0] < a.n[0] || (n[0] == a.n[0] && n[1] < a.n[1]));
    }
};

// Mapa sirve para guardar una asociación entre una arista y un indice de nodo
// (que no es de la arista)
using Mapa = std::map<Arista, int>;

void subdivide(SubDivMesh& mesh) {

    /// @@@@@: Implementar Catmull-Clark... lineamientos:
    //  Los nodos originales estan en las posiciones 0 a #n-1 de m.n,
    //  Los elementos orignales estan en las posiciones 0 a #e-1 de m.e
    //  1) Por cada elemento, agregar el centroide (nuevos nodos: #n a #n+#e-1)
    // buscar el elemento dentro del mesh
    // Reccorer 3 o 4 nodos
    // interpolar punto medio -> crear nuevo nodo
    // agregar al elemento
    // std::vector<Elemento> meshViejo = mesh.e;
    int n0 = mesh.n.size();
    for (Elemento& elem : mesh.e) {
        Nodo nod = Nodo(interpolar(elem.n, elem.nv, mesh));
        mesh.n.push_back(nod);
    }

    //  2) Por cada arista de cada cara, agregar un pto en el medio que es
    //      promedio de los vertices de la arista y los centroides de las caras
    //      adyacentes. Aca hay que usar los elementos vecinos.
    //      En los bordes, cuando no hay vecinos, es simplemente el promedio de
    //      los vertices de la arista Hay que evitar procesar dos veces la misma
    //      arista (como?) Mas adelante vamos a necesitar determinar cual punto
    //      agregamos en cada arista, y ya que no se pueden relacionar los
    //      indices con una formula simple se sugiere usar Mapa como estructura
    //      auxiliar

    //      recorrer la mesh cada elemento
    //      la artista chekeaqueas si es frontera (-1),
    //      si es interior, ya esta en el map? sino ->, guardar arista +
    //      interpol de los nodos interiores en el mapa
    Mapa mapa;
    int ncurrent = 0;
    for (Elemento& elem : mesh.e) {
        for (int i = 0; i < elem.nv; ++i) {
            Arista ar = Arista(elem, i);
            // v[i] indice de la arista i del elemento, cual es el elemento
            // vecino -1 indica frontera
            if (mapa.find(ar) != mapa.end())
                continue; // si ya la proceso, la salta
            if (elem.v[i] == -1) {
                // Es frontera
                Nodo nod = Nodo(interpolar(ar.n, 2, mesh));
                mesh.n.push_back(nod);
                mapa[ar] = mesh.n.size() - 1;
            } else {
                // Es interior
                int arr[4];
                arr[0] = ar.n[0];
                arr[1] = ar.n[1];
                // El nodo n0+0 es el centroide del elemento 1
                // El nodo n0+1 es el centroide del elemento 2
                arr[2] = n0 + ncurrent;
                // Queremos el centroide del elemento actual y el del
                // elemento con arista vecina
                arr[3] = n0 + elem.v[i];
                Nodo nod = Nodo(interpolar(arr, 4, mesh));
                mesh.n.push_back(nod);
                mapa[ar] = mesh.n.size() - 1;
            }
        }
        ++ncurrent;
    }
    // Tenemos en el mapa por cada arista su punto interpolado
    // Y si es interior, apunta al interpolado entre centroides

    //  3) Armar los elementos nuevos
    //      Los quads se dividen en 4, (uno reemplaza al original, los otros 3
    //      se agregan) Los triangulos se dividen en 3, (uno reemplaza al
    //      original, los otros 2 se agregan) Para encontrar los nodos de las
    //      aristas usar el mapa que armaron en el paso 2 Ordenar los nodos de
    //      todos los elementos nuevos con un mismo criterio (por ej, siempre
    //      poner primero al centroide del elemento), para simplificar el
    //      paso 4.

    ncurrent = 0;
    unsigned nElemInicial = mesh.e.size();
    for (unsigned j = 0; j < nElemInicial; ++j) {
        // Pasamos el elemnto por copia en ves de por referencia, si
        // fuere por copia al insertar el nuevo elemento, la
        // refrencia se vuelve invalida
        Elemento elem = mesh.e[j];
        // Recorremos los nodos del elemento y generamos un elemento nuevo
        // por nodo usando el criterio: centroide - ar_now - vert_i - ar_next
        int i = 0;
        int centroide = n0 + ncurrent; // centroide de esta cara
        Arista ar_now(elem, 0);
        Arista ar_next(elem, (i + 1) % elem.nv);
        int nodo_ar_now = mapa[ar_now];
        int nodo_vert = elem.n[(i + 1) % elem.nv];
        int nodo_ar_next = mapa[ar_next];
        // creamos nuevo elemento,por el primero  reemplazamos al original
        // mesh.reemplazarElemento(ncurrent, nodo_ar_now, nodo_vert,
        // nodo_ar_next,
        //                         centroide);
        mesh.reemplazarElemento(ncurrent, centroide, nodo_ar_now, nodo_vert,
                                nodo_ar_next);
        for (i = 1; i < elem.nv; ++i) {
            // aristas (con wrap para hacerlo ciclico y que no se rompa)
            Arista ar_now(elem, i);
            Arista ar_next(elem, (i + 1) % elem.nv);

            // buscar en el mapa
            nodo_ar_now = mapa[ar_now];
            nodo_vert = elem.n[(i + 1) % elem.nv];
            nodo_ar_next = mapa[ar_next];

            // creamos nuevo elemento y lo agregamos
            mesh.agregarElemento(centroide, nodo_ar_now, nodo_vert,
                                 nodo_ar_next);
            // mesh.agregarElemento(nodo_ar_now, nodo_vert, nodo_ar_next,
            //                      centroide);
        }
        ++ncurrent;
    }

    mesh.makeVecinos();

    //  4) Calcular las nuevas posiciones de los nodos originales
    //      Para nodos interiores: (4r-f+(n-3)p)/n
    //         f=promedio de nodos interiores de las caras (los agregados en el
    //         paso 1) r=promedio de los pts medios de las aristas (los
    //         agregados en el paso 2) p=posicion del nodo original n=cantidad
    //         de elementos para ese nodo
    //      Para nodos del borde: (r+p)/2
    //
    //         r=promedio de los dos pts medios de las aristas
    //         p=posicion del nodo original
    //      Ojo: en el paso 3 cambio toda la SubDivMesh, analizar donde quedan
    //      en los nuevos elementos (¿de que tipo son?) los nodos de las caras y
    //      los de las aristas que se agregaron antes.
    // tips:
    //   no es necesario cambiar ni agregar nada fuera de este método, (con Mapa
    //   como
    //     estructura auxiliar alcanza)
    //   sugerencia: probar primero usando el cubo (es cerrado y solo tiene
    //   quads)
    //               despues usando la piramide (tambien cerrada, y solo
    //               triangulos) despues el ejemplo plano (para ver que pasa en
    //               los bordes) finalmente el mono (tiene mezcla y elementos
    //               sin vecinos)
    //   repaso de como usar un mapa:
    //     para asociar un indice (i) de nodo a una arista (n1-n2):
    //     elmapa[Arista(n1,n2)]=i; para saber si hay un indice asociado a una
    //     arista:  ¿elmapa.find(Arista(n1,n2))!=elmapa.end()? para recuperar el
    //     indice (en j) asociado a una arista: int j=elmapa[Arista(n1,n2)];
    for (int i = 0; i < n0; ++i) {
        // Vamos a recorrer todos los nodos originales [0,n0).
        // Los centroides y puntos  medios estan en [n0 mesh.n.size())
        Nodo& nod = mesh.n[i];
        glm::vec3 p = nod.p;
        glm::vec3 r, f;
        f = r = {0, 0, 0};
        int nsize = nod.e.size();
        if (!nod.es_frontera) {
            // Es interior
            for (unsigned j = 0; j < nsize; ++j) {
                int ne = nod.e[j];
                Elemento& elem = mesh.e[ne];
                // El orden de los nodos en el elemento es
                // centroide - arista1 - nodo compartido - arista2
                int centroide = elem.n[0];
                int cen1 = elem.n[1];
                f += mesh.n[centroide].p;
                r += mesh.n[cen1].p;
            }
            f /= nsize;
            r /= nsize;
            r *= 4;
            p *= nsize - 3; // si usamos el mesh.n.size() hay que castearlo a
                            // int, porq sino tenemos mesh.n.size< 3 y como
                            // es
            // unsigned crea un underflow
            nod.p = r - f + p;
            nod.p /= nsize;
        } else {
            // Es frontera
            if (nsize == 1) {
                int ne = nod.e[0];
                Elemento& elem = mesh.e[ne];
                int cen1 = elem.n[1];
                int cen2 = elem.n[3];
                r += mesh.n[cen1].p;
                r += mesh.n[cen2].p;
            } else {
                for (unsigned j = 0; j < nsize; ++j) {
                    int ne = nod.e[j];
                    Elemento& elem = mesh.e[ne];
                    int cen1 = elem.n[1];
                    int cen2 = elem.n[3];
                    if (mesh.n[cen1].es_frontera) {
                        r += mesh.n[cen1].p;
                    } else
                    // if (mesh.n[cen2].es_frontera)
                    {
                        r += mesh.n[cen2].p;
                    }
                }
            }
            r /= 2;
            nod.p = p + r;
            nod.p /= 2;
        }
    }
    //
    // Esta llamada valida si la estructura de datos quedó consistente (si todos
    // los índices están dentro del rango válido, y si son correctas las
    // relaciones entre los .n de los elementos y los .e de los nodos). Mantener
    // al final de esta función para ver que la subdivisión implementada no
    // rompa esos invariantes.
    mesh.verificarIntegridad();
}
glm::vec3 interpolar(int* n, int largo, SubDivMesh& mesh) {
    //
    glm::vec3 inter = {0, 0, 0};
    for (int i = 0; i < largo; ++i) {
        int& indice = n[i];
        inter += mesh.n[indice].p;
    }
    inter /= largo;
    return inter;
}
