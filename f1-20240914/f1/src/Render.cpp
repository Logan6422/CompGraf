#include <glm/ext.hpp>
#include "Render.hpp"
#include "Callbacks.hpp"

extern bool wireframe, play, top_view, antialiasing;

// matrices que definen la camara
glm::mat4 projection_matrix, view_matrix;

// función para renderizar cada "parte" del auto
void renderPart(const Car &car, const std::vector<Model> &v_models, const glm::mat4 &matrix, Shader &shader) {
	// select a shader
	for(const Model &model : v_models) {
		shader.use();
		
		// matrixes
		if (play) {
			/// @todo: modificar una de estas matrices para mover todo el auto (todas
			///        las partes) a la posición (y orientación) que le corresponde en la pista
			
			glm::mat4 m = glm::translate(glm::mat4(1.f),glm::vec3(car.x,0,car.y));
			m = glm::rotate(m,-car.ang,glm::vec3(0.0f,1.0f,0.0f));
			
			shader.setMatrixes(m*matrix,view_matrix,projection_matrix);
		} else {
			glm::mat4 model_matrix = glm::rotate(glm::mat4(1.f),view_angle,glm::vec3{1.f,0.f,0.f}) *
						             glm::rotate(glm::mat4(1.f),model_angle,glm::vec3{0.f,1.f,0.f}) *
			                         matrix;
			shader.setMatrixes(model_matrix,view_matrix,projection_matrix);
		}
		
		// setup light and material
		shader.setLight(glm::vec4{20.f,40.f,20.f,0.f}, glm::vec3{1.f,1.f,1.f}, 0.35f);
		shader.setMaterial(model.material);
		
		// send geometry
		shader.setBuffers(model.buffers);
		glPolygonMode(GL_FRONT_AND_BACK,(wireframe and (not play))?GL_LINE:GL_FILL);
		model.buffers.draw();
	}
}

// función que actualiza las matrices que definen la cámara
void setViewAndProjectionMatrixes(const Car &car) {
	projection_matrix = glm::perspective( glm::radians(view_fov), float(win_width)/float(win_height), 0.1f, 100.f );
	if (play) {
		if (top_view) {
			/// @todo: modificar el look at para que en esta vista el auto siempre apunte hacia arriba
			glm::vec3 pos_auto = {car.x, 0.f, car.y};
			view_matrix = glm::lookAt(pos_auto+glm::vec3{0.f,30.f,0.f}, pos_auto, glm::vec3{cos(car.ang),0.f,sin(car.ang)});
		} else {
			/// @todo: definir view_matrix de modo que la camara persiga al auto desde "atras"
			glm::vec3 pos_auto = {car.x, 0.f, car.y};
			view_matrix = glm::lookAt(pos_auto+glm::vec3{-4*cos(car.ang),1.f,-4*sin(car.ang)}, pos_auto, glm::vec3{0.f,1.f		,0.f});
		}
	} else {
		view_matrix = glm::lookAt( glm::vec3{0.f,0.f,3.f}, view_target, glm::vec3{0.f,1.f,0.f} );
	}
}

// función que rendiriza todo el auto, parte por parte
void renderCar(const Car &car, const std::vector<Part> &parts, Shader &shader) {
	const Part &axis = parts[0], &body = parts[1], &wheel = parts[2],
	           &fwing = parts[3], &rwing = parts[4], &helmet = parts[antialiasing?5:6];


	
	if (body.show or play) {
		glm::mat4 mbody(  1.0f, 0.0f, 0.0f, 0.0f, // nuevo eje x
						  0.0f, 1.0f, 0.0f, 0.0f, // nuevo eje y	
						  0.0f, 0.0f, 1.0f, 0.0f, // nuevo eje z
						  0.0f, 0.1f, 0.0f, 1.0f );
		renderPart(car,body.models, mbody,shader);
	}
	
	
	if (body.show or play) {
		glm::mat4 mbody(  1.0f, 0.0f, 0.0f, 0.0f, // nuevo eje x
						  0.0f, 1.0f, 0.0f, 0.0f, // nuevo eje y	
						  0.0f, 0.0f, 1.0f, 0.0f, // nuevo eje z
						  0.0f, 0.1f, 0.0f, 1.0f );
		renderPart(car,body.models, mbody,shader);
	}
	
	if (wheel.show or play) {
		glm::mat4 mwheel1(0.1f, 0.0f, 0.0f, 0.0f, // nuevo eje x
						 0.0f, 0.1f, 0.0f, 0.0f, // nuevo eje y
						 0.0f, 0.0f, 0.1f, 0.0f, // nuevo eje z
						 0.5f, 0.1f, 0.3f, 1.0f );
		mwheel1 = glm::rotate(mwheel1,car.rang1*2,glm::vec3(0.0f,-1.0f,1.f));
		mwheel1 = glm::rotate(mwheel1,-car.rang2,glm::vec3(0.0f,0.0f,1.f));
		
		glm::mat4 mwheel4(0.1f, 0.0f, 0.0f, 0.0f, // nuevo eje x
						  0.0f, 0.1f, 0.0f, 0.0f, // nuevo eje y
						  0.0f, 0.0f, 0.1f, 0.0f, // nuevo eje z
						  0.5f, 0.1f, -0.3f, 1.0f );
		mwheel4 = glm::rotate(mwheel4,car.rang1*2,glm::vec3(0.0f,-1.0f,0.f));
		mwheel4 = glm::rotate(mwheel4,-car.rang2,glm::vec3(0.0f,0.0f,1.f));
		
		glm::mat4 mwheel2(0.1f, 0.0f, 0.0f, 0.0f, // nuevo eje x
						  0.0f, 0.1f, 0.0f, 0.0f, // nuevo eje y
						  0.0f, 0.0f, 0.1f, 0.0f, // nuevo eje z
						  -0.9f, 0.1f, 0.3f, 1.0f );
		mwheel2 = glm::rotate(mwheel2,-car.rang2,glm::vec3(0.0f,0.0f,1.f));
		
		glm::mat4 mwheel3(0.1f, 0.0f, 0.0f, 0.0f, // nuevo eje x
						  0.0f, 0.1f, 0.0f, 0.0f, // nuevo eje y
						  0.0f, 0.0f, 0.1f, 0.0f, // nuevo eje z
						  -0.9f, 0.1f, -0.3f, 1.0f );
		mwheel3 = glm::rotate(mwheel3,-car.rang2,glm::vec3(0.0f,0.0f,1.f));
		
		
		
		renderPart(car,wheel.models, mwheel1,shader); // x4
		renderPart(car,wheel.models, mwheel2,shader); 
		renderPart(car,wheel.models, mwheel3,shader); 
		renderPart(car,wheel.models, mwheel4,shader); 
	}
	
	
	
	if (fwing.show or play) {
		float scl = 0.30f;
		glm::mat4 mfwing( 0.0f, 0.0f, scl, 0.0f,// nuevo eje x
						 0.0f, scl, 0.0f, 0.0f, // nuevo eje y
						 -scl, 0.0f, 0.0f, 0.0f,// nuevo eje z
						 0.9f, 0.0f, 0.0f, 1.0f );
		
		renderPart(car,fwing.models, mfwing,shader);
	}
	
	if (rwing.show or play) {
		float scl = 0.30f;
		glm::mat4 mrwing( 0.0f, 0.0f, scl, 0.0f,// nuevo eje x
						  0.0f, scl, 0.0f, 0.0f,// nuevo eje y
						 -scl, 0.0f, 0.0f, 0.0f, // nuevo eje z
						  -1.0f, 0.25f, 0.0f, 1.0f );
		

		renderPart(car,rwing.models, mrwing,shader);
	}
	
	if (helmet.show or play) {
		float angle = 90;
		glm::radians(angle);
		
//		glm::mat4 mhelmet(0.0f, 0.0f, 0.1f, 0.0f,// nuevo eje x (rotado y escalado)
//						  -0.1f, 0.0f, 0.0f, 0.0f,// nuevo eje y (escalado)
//						  0.0f, -0.1f, 0.0f, 0.0f, // nuevo eje z (rotado y escalado)
//						  0.0f, 0.2f, 0.0f, 1.0f); // traslación en Y
		
		glm::mat4 mhelmet(0.0f, 0.0f, 0.1f, 0.0f,// nuevo eje x (rotado y escalado)
						  0.0f, 0.1f, 0.0f, 0.0f,  // nuevo eje y (escalado)
						  -0.1f, 0.0f, 0.0f, 0.0f, // nuevo eje z (rotado y escalado)
						  0.0f, 0.2f, 0.0f, 1.0f); // traslación en Y
		
		renderPart(car,helmet.models, mhelmet,shader);
	}
	
	if (axis.show and (not play)) renderPart(car,axis.models,glm::mat4(1.f),shader);
}

// función que renderiza la pista
void renderTrack() {
	static Model track = Model::loadSingle("models/track",Model::fDontFit);
	static Shader shader("shaders/texture");
	shader.use();
	shader.setMatrixes(glm::mat4(1.f),view_matrix,projection_matrix);
	shader.setMaterial(track.material);
	shader.setBuffers(track.buffers);
	track.texture.bind();
	static float aniso = -1.0f;
	if (aniso<0) glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	track.buffers.draw();
}

void renderShadow(const Car &car, const std::vector<Part> &parts) {
	static Shader shader_shadow("shaders/shadow");
	glEnable(GL_STENCIL_TEST); glClear(GL_STENCIL_BUFFER_BIT);
	glStencilFunc(GL_EQUAL,0,~0); glStencilOp(GL_KEEP,GL_KEEP,GL_INCR);
	renderCar(car,parts,shader_shadow);
	glDisable(GL_STENCIL_TEST);
}
