#include "OutlineProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Scene::Drawable::Pipeline outline_program_pipeline;

Load< OutlineProgram0 > outline_program_0(LoadTagEarly, []() -> OutlineProgram0 const * {
	OutlineProgram0 *ret = new OutlineProgram0();

	//----- build the pipeline template -----
	outline_program_pipeline.program = ret->program;

	outline_program_pipeline.OBJECT_TO_CLIP_mat4 = ret->OBJECT_TO_CLIP_mat4;

	//make a 1-pixel white texture to bind by default:
	GLuint tex;
	glGenTextures(1, &tex);

	glBindTexture(GL_TEXTURE_2D, tex);
	std::vector< glm::u8vec4 > tex_data(1, glm::u8vec4(0xff));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	outline_program_pipeline.textures[0].texture = tex;
	outline_program_pipeline.textures[0].target = GL_TEXTURE_2D;

	return ret;
});

OutlineProgram0::OutlineProgram0() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
  program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"uniform mat4 OBJECT_TO_CLIP;\n"
		"in vec4 Position;\n"
		"in vec3 Normal;\n"
		"in vec4 Color;\n"
		"in vec2 TexCoord;\n"
    "out vec3 normal;\n"
		"void main() {\n"
		"	gl_Position = OBJECT_TO_CLIP * Position;\n"
    " normal = Normal;\n"
		"}\n"
	,
		//fragment shader:
		"#version 330\n"
    "in vec3 normal;\n"
		"out vec4 fragNormalZ;\n"
		"void main() {\n"
    " fragNormalZ = vec4(normalize(normal), gl_FragCoord.z);\n"
		"}\n"
	);
	//As you can see above, adjacent strings in C/C++ are concatenated.
	// this is very useful for writing long shader programs inline.

	//look up the locations of vertex attributes:
	Position_vec4 = glGetAttribLocation(program, "Position");
	Normal_vec3 = glGetAttribLocation(program, "Normal");
	Color_vec4 = glGetAttribLocation(program, "Color");
	TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");

	//look up the locations of uniforms:
	OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program, "OBJECT_TO_CLIP");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

OutlineProgram0::~OutlineProgram0() {
	glDeleteProgram(program);
	program = 0;
}

/*
==========================================================================
*/

GLuint vao_empty = 0;

Load< void > init_vao_empty(LoadTagEarly, [](){
	glGenVertexArrays(1, &vao_empty);
});

Load< OutlineProgram1 > outline_program_1(LoadTagEarly);

OutlineProgram1::OutlineProgram1() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"void main() {\n"
		"	gl_Position = vec4(4 * (gl_VertexID & 1) - 1,  2 * (gl_VertexID & 2) - 1, 0.0, 1.0);\n"
		"}\n"
	,
		//fragment shader:
		"#version 330\n"
		"uniform sampler2DRect COLOR_TEX;\n"
		"uniform sampler2DRect NORMAL_Z_TEX;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
    " ivec2 pos = ivec2(gl_FragCoord);\n"
    " vec4 x0 = texelFetch(NORMAL_Z_TEX, ivec2(pos.x - 1, pos.y));\n"
    " vec4 x1 = texelFetch(NORMAL_Z_TEX, ivec2(pos.x + 1, pos.y));\n"
    " vec4 y0 = texelFetch(NORMAL_Z_TEX, ivec2(pos.x, pos.y - 1));\n"
    " vec4 y1 = texelFetch(NORMAL_Z_TEX, ivec2(pos.x, pos.y + 1));\n"
    " vec4 cin = texelFetch(COLOR_TEX, pos);\n"
    " vec4 cout = vec4(0.0, 0.0, 0.0, 0.0);\n"
    " if (dot(x0.xyz, x1.xyz) > 0.95 &&\n"
    "  dot(y0.xyz, y1.xyz) > 0.95 &&\n"
    "  abs(x0.w - x1.w) < 0.01 &&\n"
    "  abs(y0.w - y1.w) < 0.01) {\n"
    "  cout = vec4(1.0, 1.0, 1.0, 1.0);\n"
    " }\n"
		"	fragColor = cout;\n"
		"}\n"
	);

	GLuint COLOR_TEX_sampler2D = glGetUniformLocation(program, "COLOR_TEX");

  GLuint NORMAL_Z_TEX_sampler2D = glGetUniformLocation(program, "NORMAL_Z_TEX");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now
	glUniform1i(COLOR_TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0
  glUniform1i(NORMAL_Z_TEX_sampler2D, 1); //set TEX to sample from GL_TEXTURE1
	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

OutlineProgram1::~OutlineProgram1() {
	glDeleteProgram(program);
	program = 0;
}
