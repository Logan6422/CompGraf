#include <algorithm>
#include <stdexcept>
#include <vector>
#include <limits>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Model.hpp"
#include "Window.hpp"
#include "Callbacks.hpp"
#include "Debug.hpp"
#include "Shaders.hpp"
#include "BezierRenderer.hpp"
#include "Spline.hpp"

#define VERSION 20241022

// settings
bool show_axis = false, show_fish = false, show_spline = true, show_poly = true, animate = true;

// curva
static const int degree = 3;
int ctrl_pt = -1, cant_pts = 6; // pto de control seleccionado (para el evento de arrastre)
Spline spline( { {-1.f,0.f,0.f}, {0.f,0.f,-1.f}, {1.f,0.f,0.f}, {0.f,0.f,1.f} }, true );

void updateControlPointsAround(Spline &spline, int ctrl_pt) {
	// Puntos interpolados
	glm::vec3 P_prev = spline.getControlPoint(ctrl_pt - 3); // Punto anterior
	glm::vec3 P_curr = spline.getControlPoint(ctrl_pt);     // Punto actual
	glm::vec3 P_next = spline.getControlPoint(ctrl_pt + 3); // Punto siguiente
	
	// Calcular las distancias entre los puntos
	float dist_prev = glm::length(P_curr - P_prev); // Distancia entre P actual y anterior
	float dist_next = glm::length(P_next - P_curr); // Distancia entre P siguiente y actual
	
	// Calcular las velocidades 
	glm::vec3 v_prev = (P_curr - P_prev) / dist_prev; // Velocidad del anterior
	glm::vec3 v_next = (P_next - P_curr) / dist_next; // Velocidad del siguiente
	
	// Calcular la velocidad ponderada para suavidad (Continuidad G1)
	glm::vec3 v_avg = (dist_next * v_prev + dist_prev * v_next) / (dist_prev + dist_next);
	
	// Factor de ajuste para los puntos de control (1/3)
	float factor_prev = dist_prev / 3.0f;
	float factor_next = dist_next / 3.0f;
	
	// Calcular los puntos de control intermedios
	glm::vec3 P_prev_ctrl = P_curr - factor_prev * v_avg; // Punto de control anterior
	glm::vec3 P_next_ctrl = P_curr + factor_next * v_avg; // Punto de control siguiente
	
	// Actualizar los puntos de control en la spline
	spline.setControlPoint(ctrl_pt - 1, P_prev_ctrl); // Actualiza punto previo
	spline.setControlPoint(ctrl_pt + 1, P_next_ctrl); // Actualiza punto siguiente
}

// callbacks
void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void keyboardCallback(GLFWwindow* glfw_win, int key, int scancode, int action, int mods);
void characterCallback(GLFWwindow* glfw_win, unsigned int code);

glm::mat4 getTransform(const Spline &spline, double t) {
	// Obtener la posición y la derivada en el parámetro t
	glm::vec3 pos = spline.at(t);
	glm::vec3 deriv;
	spline.at(t, deriv);  // Esto devuelve la derivada en el punto t
	
	// Normalizar la derivada para obtener el eje X (hacia adelante)
	glm::vec3 e_x = glm::normalize(deriv);
	
	// vector y aux
	glm::vec3 up(0.f, 1.f, 0.f);
	
	// Calcular el eje Z (hacia arriba) usando el producto cruzado entre e_x y up
	glm::vec3 e_z = glm::normalize(glm::cross(e_x, up));
	
	// Calcular el eje Y (hacia el lado) usando el producto cruzado entre e_z y e_x
	glm::vec3 e_y = glm::normalize(glm::cross(e_z, e_x));
	
	// Escalar el pez
	float scale_factor = 0.5f; 
	e_x *= scale_factor;
	e_y *= scale_factor;
	e_z *= scale_factor;
	
	// Crear la matriz de transformación
	glm::mat4 m(1.f);
	m[0] = glm::vec4(e_x, 0.f);  // Eje X local
	m[1] = glm::vec4(e_y, 0.f);  // Eje Y local
	m[2] = glm::vec4(e_z, 0.f);  // Eje Z local
	m[3] = glm::vec4(pos, 1.f);  // Posición (origen local)
	
	return m;
}


// cuando cambia la cant de tramos, regenerar la spline
void remapSpline(Spline &spline, int cant_pts) {
	if (cant_pts<3) return;
	if (static_cast<int>(spline.getPieces().size()) == cant_pts) return;
	std::vector<glm::vec3> vp;
	double dt = 1.0/cant_pts;
	for(int i=0;i<cant_pts;++i)
		vp.push_back(spline.at(i*dt));
	spline = Spline(vp,true);
	for(int i=0;i<spline.getControlPointsCount();i+=degree) 
		updateControlPointsAround(spline,i);
}

int main() {
	
	// initialize window and setup callbacks
	Window window(win_width,win_height,"CG Demo");
	glfwSetFramebufferSizeCallback(window, common_callbacks::viewResizeCallback);
	glfwSetCursorPosCallback(window, mouseMoveCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCharCallback(window, characterCallback);
	
	// setup OpenGL state and load the model
	glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); 
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glClearColor(0.4f,0.4f,0.7f,1.f);
	Shader shader_fish("shaders/fish");
	Shader shader_phong("shaders/phong");
	auto fish = Model::load("models/fish");
	auto axis = Model::load("models/axis",Model::fDontFit);
	BezierRenderer bezier_renderer(500);
	model_angle = .33; view_angle = .85;
	
	glm::vec4 light_pos = {2.f,2.f,4.f,0.f};
	
	// main loop
	FrameTimer ftime;
	float t = 0.f, speed = .05f;
	do {
		
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		remapSpline(spline,cant_pts);
		
		// draw models and curve
		float dt = ftime.newFrame();
		if (animate) {
			t += dt*speed; while (t>1.f) t-=1.f; 
		}
		if (show_fish) {
			shader_fish.use();
			shader_fish.setLight(light_pos, glm::vec3{1.f,1.f,1.f}, 0.15f);
			shader_fish.setUniform("time",t*20);
			glm::mat4 m = getTransform(spline, t);
			auto mats = common_callbacks::getMatrixes();
			for(Model &model : fish) {
				shader_fish.setMatrixes(mats[0]*m,mats[1],mats[2]);
				shader_fish.setBuffers(model.buffers);
				shader_fish.setMaterial(model.material);
				model.buffers.draw();
			}
		}
		
		if (show_spline or show_poly) {
			setMatrixes(bezier_renderer.getShader());
			for(const auto & curve : spline.getPieces()) {
				bezier_renderer.update(curve);
				glPointSize(1);
				if (show_spline) bezier_renderer.drawCurve();
				glPointSize(5);
				if (show_poly) bezier_renderer.drawPoly();
			}
		}
		
		if (show_axis) {
			shader_phong.use();
			shader_phong.setLight(light_pos, glm::vec3{1.f,1.f,1.f}, 0.15f);
			setMatrixes(shader_phong);
			glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
			for(const Model &model : axis) {
				shader_phong.setBuffers(model.buffers);
				shader_phong.setMaterial(model.material);
				model.buffers.draw();
			}
		}
		
		// settings sub-window
		window.ImGuiDialog("CG Example",[&](){
			ImGui::Checkbox("Pez (P)",&show_fish);
			ImGui::Checkbox("Spline (S)",&show_spline);
			ImGui::Checkbox("Pol. Ctrl. (C)",&show_poly);
			ImGui::Checkbox("Ejes (J)",&show_axis);
			ImGui::Checkbox("Animar (A)",&animate);
			ImGui::SliderFloat("Velocidad",&speed,0.005f,0.5f);
			ImGui::SliderFloat("T",&t,0.f,1.f);
			if (ImGui::InputInt("Cant. Pts.",&cant_pts,1,1))
				if (cant_pts<3) cant_pts=3;
		});
		
		// finish frame
		window.finishFrame();
		
	} while( glfwGetKey(window,GLFW_KEY_ESCAPE)!=GLFW_PRESS && !glfwWindowShouldClose(window) );
}

void characterCallback(GLFWwindow* glfw_win, unsigned int code) {
	switch (code) {
		case 'a': case 'A':animate = !animate; break;
		case 's': case 'S':show_spline = !show_spline; break;
		case 'p': case 'P':show_fish = !show_fish; break;
		case 'j': case 'J':show_axis = !show_axis; break;
		case 'c': case 'C':show_poly = !show_poly; break;
		case '+': ++cant_pts; break;
		case '-': --cant_pts; break;
	}
}

glm::vec3 viewportToPlane(double xpos, double ypos) {
	auto ms = common_callbacks::getMatrixes(); // { model, view, projection }
	auto inv_matrix = glm::inverse(ms[2]*ms[1]*ms[0]); // ndc->world
	auto pa = inv_matrix * glm::vec4{ float(xpos)/win_width*2.f-1.f,
		                              (1.f-float(ypos)/win_height)*2.f-1.f,
		                              0.f, 1.f }; // point on near
	auto pb = inv_matrix * glm::vec4{ float(xpos)/win_width*2.f-1.f,
		                              (1.f-float(ypos)/win_height)*2.f-1.f,
		                              1.f, 1.f }; // point on far
	float alpha = pa[1]/(pa[1]-pb[1]);
	auto p = pa*(1-alpha) + pb*alpha; // point on plane
	return {p[0]/p[3],0.f,p[2]/p[3]};
}

void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
	if (ctrl_pt==-1) common_callbacks::mouseMoveCallback(window,xpos,ypos);
	else {
		spline.setControlPoint(ctrl_pt,viewportToPlane(xpos,ypos));
		if (ctrl_pt%degree==0) {
			updateControlPointsAround(spline,ctrl_pt);
			updateControlPointsAround(spline,ctrl_pt+degree);
			updateControlPointsAround(spline,ctrl_pt-degree);
		}
	}
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (ImGui::GetIO().WantCaptureMouse) return;
	ctrl_pt = -1;
	if (action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		auto p = viewportToPlane(xpos,ypos);
		float dmin = .1f;
		for(int i=0;i<spline.getControlPointsCount(); ++i) {
			double aux = glm::distance(p,spline.getControlPoint(i));
			if (aux < dmin) { dmin = aux; ctrl_pt = i; }
		}
	}
	if (ctrl_pt==-1) common_callbacks::mouseButtonCallback(window,button,action,mods);
}

