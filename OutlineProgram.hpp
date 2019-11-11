#pragma once

#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"

struct FlatProgram{
	FlatProgram();
	~FlatProgram();

	GLuint program = 0;

	//Attribute (per-vertex variable) locations:
	GLuint Position_vec4 = -1U;
	GLuint Normal_vec3 = -1U;
	GLuint Color_vec4 = -1U;
	GLuint TexCoord_vec2 = -1U;

	//Uniform (per-invocation variable) locations:
	GLuint OBJECT_TO_CLIP_mat4 = -1U;
	GLuint OBJECT_TO_LIGHT_mat4x3 = -1U;
	GLuint NORMAL_TO_LIGHT_mat3 = -1U;
  GLuint USE_TEX_bool = -1U;

	//Textures:
	//TEXTURE0 - texture that is accessed by TexCoord
};


struct OutlineProgram0{
	OutlineProgram0();
	~OutlineProgram0();

	GLuint program = 0;

	//Attribute (per-vertex variable) locations:
	GLuint Position_vec4 = -1U;
	GLuint Normal_vec3 = -1U;
	GLuint Color_vec4 = -1U;
	GLuint TexCoord_vec2 = -1U;

	//Uniform (per-invocation variable) locations:
	GLuint OBJECT_TO_CLIP_mat4 = -1U;

	//Textures:
	//TEXTURE0 - texture that is accessed by TexCoord
};

struct OutlineProgram1 {
	OutlineProgram1();
	~OutlineProgram1();

	GLuint program = 0;

};

extern Load< FlatProgram > flat_program;
extern Load< OutlineProgram0 > outline_program_0;
extern Load< OutlineProgram1 > outline_program_1;
extern GLuint vao_empty;
//For convenient scene-graph setup, copy this object:
// NOTE: by default, has texture bound to 1-pixel white texture -- so it's okay to use with vertex-color-only meshes.
extern Scene::Drawable::Pipeline flat_program_pipeline;
