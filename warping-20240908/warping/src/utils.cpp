#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "utils.hpp"
#include "Debug.hpp"

BoundingBox::BoundingBox(glm::vec3 &p1, glm::vec3 &p2) 
	: pmin({std::min(p1.x,p2.x),std::min(p1.y,p2.y),std::min(p1.z,p2.z)}),
      pmax({std::max(p1.x,p2.x),std::max(p1.y,p2.y),std::max(p1.z,p2.z)}) 
{
	
}
	
bool BoundingBox::contiene(glm::vec3 &p) const {
	return p.x>=pmin.x && p.x<=pmax.x &&
		p.y>=pmin.y && p.y<=pmax.y &&
		p.z>=pmin.z && p.z<=pmax.z;
}

Pesos calcularPesos(glm::vec3 x0, glm::vec3 x1, glm::vec3 x2, glm::vec3 x) {
	/// @todo: implementar

	glm::vec3 a_tot = glm::cross((x1 - x0),(x2-x0));
	glm::vec3 a0 = glm::cross((x1-x),(x2-x));
	glm::vec3 a1 = glm::cross((x2-x),(x0-x));
	glm::vec3 a2 = glm::cross((x0-x),(x1-x));
	
	float a0_proy = glm::dot(a0,a_tot)/(glm::dot(a_tot,a_tot));
	float a1_proy = glm::dot(a1,a_tot)/glm::dot(a_tot,a_tot);
	float a2_proy = glm::dot(a2,a_tot)/glm::dot(a_tot,a_tot);
	
	
//	cg_error("debe implementar la funcion calcularPesos (utils.cpp)");

	return {a0_proy,a1_proy,a2_proy};
}


