#include "OutlineProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Scene::Drawable::Pipeline flat_program_pipeline;

Load< FlatProgram > flat_program(LoadTagEarly, []() -> FlatProgram const * {
	FlatProgram *ret = new FlatProgram();

	//----- build the pipeline template -----
	flat_program_pipeline.program = ret->program;

	flat_program_pipeline.OBJECT_TO_CLIP_mat4 = ret->OBJECT_TO_CLIP_mat4;
	flat_program_pipeline.OBJECT_TO_LIGHT_mat4x3 = ret->OBJECT_TO_LIGHT_mat4x3;
	flat_program_pipeline.NORMAL_TO_LIGHT_mat3 = ret->NORMAL_TO_LIGHT_mat3;

	return ret;
});

FlatProgram::FlatProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"uniform mat4 OBJECT_TO_CLIP;\n"
		"uniform mat4x3 OBJECT_TO_LIGHT;\n"
		"uniform mat3 NORMAL_TO_LIGHT;\n"
		"in vec4 Position;\n"
		"in vec3 Normal;\n"
		"in vec4 Color;\n"
		"in vec2 TexCoord;\n"
		"out vec4 color;\n"
		"out vec2 texCoord;\n"
		"void main() {\n"
		"	gl_Position = OBJECT_TO_CLIP * Position;\n"
		"	color = Color;\n"
		"	texCoord = TexCoord;\n"
		"}\n"
	,
		//fragment shader:
		"#version 330\n"
		"uniform sampler2D TEX;\n"
    "uniform uint USE_TEX;\n"
    "uniform vec4 UNIFORM_COLOR;\n"
		"in vec4 color;\n"
		"in vec2 texCoord;\n"
		"layout(location=0) out vec4 fragColor;\n"
		"void main() {\n"
    " vec4 cout = vec4(1.0, 1.0, 1.0, 1.0);\n"
    " if (USE_TEX == 0U) {\n"
    "   cout = color;\n"
    " } else if (USE_TEX == 1U) {\n"
    "   cout = texture(TEX, texCoord);\n"
    " } else if (USE_TEX == 2U) {\n"
    "   cout = UNIFORM_COLOR;\n"
    " }\n"
		"	fragColor = cout;\n"
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
	OBJECT_TO_LIGHT_mat4x3 = glGetUniformLocation(program, "OBJECT_TO_LIGHT");
	NORMAL_TO_LIGHT_mat3 = glGetUniformLocation(program, "NORMAL_TO_LIGHT");

  USE_TEX_uint = glGetUniformLocation(program, "USE_TEX");
  UNIFORM_COLOR_vec4 = glGetUniformLocation(program, "UNIFORM_COLOR");

	GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

FlatProgram::~FlatProgram() {
	glDeleteProgram(program);
	program = 0;
}

/*
==========================================================================
*/

Load< OutlineProgram0 > outline_program_0(LoadTagEarly, []() -> OutlineProgram0 const * {
	OutlineProgram0 *ret = new OutlineProgram0();
	return ret;
});

OutlineProgram0::OutlineProgram0() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
  program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
    "uniform mat4 OBJECT_TO_WORLD;\n"
    "uniform mat4 OBJECT_TO_CLIP;\n"
    "uniform float OBJECT_SMOOTH_ID;\n"
		"in vec4 Position;\n"
		"in vec3 Normal;\n"
		"in vec4 Color;\n"
		"in vec2 TexCoord;\n"
    "out vec4 normal;\n"
    "out vec4 position;\n"
		"void main() {\n"
    " normal.xyz = (OBJECT_TO_WORLD * vec4(Normal, 0.0)).xyz;\n"
    " normal.w = OBJECT_SMOOTH_ID;\n"
    " position.xyz = (OBJECT_TO_WORLD * Position).xyz;\n"
    " position.w = 1.0;"
    "	gl_Position = OBJECT_TO_CLIP * Position;\n"
		"}\n"
	,
		//fragment shader:
    "#version 330\n"
    "in vec4 normal;\n"
    "in vec4 position;\n"
		"layout(location = 0) out vec4 fragNormal;\n"
		"layout(location = 1) out vec4 fragPosition;\n"
		"void main() {\n"
    " fragNormal.xyz = normalize(normal.xyz);\n"
    " fragNormal.w = normal.w;\n"
    " fragPosition = position;\n"
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
  OBJECT_TO_WORLD_mat4 = glGetUniformLocation(program, "OBJECT_TO_WORLD");
	OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program, "OBJECT_TO_CLIP");
  OBJECT_SMOOTH_ID_float = glGetUniformLocation(program, "OBJECT_SMOOTH_ID");
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
		"	gl_Position = vec4(4 * (gl_VertexID & 1) - 1,  2 * (gl_VertexID & 2) - 1, 0.01, 1.0);\n"
		"}\n"
	,
		//fragment shader:
		"#version 330\n"
    "uniform vec3 EYE;\n"
		"uniform sampler2DRect COLOR_TEX;\n"
		"uniform sampler2DRect NORMAL_TEX;\n"
		"uniform sampler2DRect POSITION_TEX;\n"
		"layout(location=0) out vec4 fragColor;\n"
		"void main() {\n"
    " ivec2 pos = ivec2(gl_FragCoord);\n"
    " vec4 n_sid = texelFetch(NORMAL_TEX, ivec2(pos.x, pos.y));\n"
    " vec3 n = n_sid.xyz;\n"
    " float sid = n_sid.w;\n"
    " vec4 p_bg = texelFetch(POSITION_TEX, ivec2(pos.x, pos.y));\n"
    " vec3 p = p_bg.xyz;\n"
    " float bg = p_bg.w;\n"
    " float dist2 = dot(p.xyz - EYE, p.xyz - EYE);\n"
    " float off = 1.0;\n"
    // " if (sid == 0.0) off = floor(5.0 / (sqrt(dist2) + 1.0) + 1.0);\n"
    " vec4 nx0 = texelFetch(NORMAL_TEX, ivec2(pos.x - off, pos.y));\n"
    " vec4 nx1 = texelFetch(NORMAL_TEX, ivec2(pos.x + off, pos.y));\n"
    " vec4 ny0 = texelFetch(NORMAL_TEX, ivec2(pos.x, pos.y - off));\n"
    " vec4 ny1 = texelFetch(NORMAL_TEX, ivec2(pos.x, pos.y + off));\n"
    " vec4 px0 = texelFetch(POSITION_TEX, ivec2(pos.x - off, pos.y));\n"
    " vec4 px1 = texelFetch(POSITION_TEX, ivec2(pos.x + off, pos.y));\n"
    " vec4 py0 = texelFetch(POSITION_TEX, ivec2(pos.x, pos.y - off));\n"
    " vec4 py1 = texelFetch(POSITION_TEX, ivec2(pos.x, pos.y + off));\n"
    " vec4 cin = texelFetch(COLOR_TEX, pos);\n"
    " vec4 cout = vec4(0.0, 0.0, 0.0, 1.0);\n"
    " float dpx0 = dot(px0.xyz - p, n);\n"
    " float dpx1 = dot(px1.xyz - p, n);\n"
    " float dpy0 = dot(py0.xyz - p, n);\n"
    " float dpy1 = dot(py1.xyz - p, n);\n"
    " bool sm = (sid != 0.0 && nx0.w == sid && nx1.w == sid && ny0.w == sid && ny1.w == sid);\n"
    " float NT = 0.95;\n"
    " float PT = 0.1;\n"
    " bool not_bent = (dot(nx0.xyz, n) > NT && dot(nx1.xyz, n) > NT && dot(ny0.xyz, n) > NT && dot(ny1.xyz, n) > NT);\n"
    " bool flvl = (abs(dpx0) < PT) && (abs(dpx1) < PT) && (abs(dpy0) < PT) && (abs(dpy1) < PT);\n"
//    " cout = vec4(dpx0, dpx1, dpy1, 1.0);\n"
//    " if (!flvl) {\n"
//    "   if (abs(dpx0) >= PT) cout = vec4(1.0, 0.0, 1.0, 1.0);\n"
//    "   else if (abs(dpx1) >= PT) cout = vec4(1.0, 1.0, 0.0, 1.0);\n"
//    "   else if (abs(dpy0) >= PT) cout = vec4(0.0, 1.0, 0.0, 1.0);\n"
//    "   else if (abs(dpy1) >= PT) cout = vec4(1.0, 0.0, 0.0, 1.0);\n"
//    "   else cout = vec4(0.0, 1.0, 1.0, 1.0);\n"
//    " }\n"
//    " else cout = vec4(1.0, 1.0, 1.0, 1.0);\n"
    " if (sm || (not_bent && flvl)) {\n"
    "   cout = cin;\n"
    " }\n"
		"	fragColor = cout;\n"
		"}\n"
	);

  EYE_vec3 = glGetUniformLocation(program, "EYE");

	GLuint COLOR_TEX_sampler2D = glGetUniformLocation(program, "COLOR_TEX");
  GLuint NORMAL_TEX_sampler2D = glGetUniformLocation(program, "NORMAL_TEX");
  GLuint POSITION_TEX_sampler2D = glGetUniformLocation(program, "POSITION_TEX");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now
	glUniform1i(COLOR_TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0
  glUniform1i(NORMAL_TEX_sampler2D, 1); //set TEX to sample from GL_TEXTURE1
  glUniform1i(POSITION_TEX_sampler2D, 2); //set TEX to sample from GL_TEXTURE2
	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

OutlineProgram1::~OutlineProgram1() {
	glDeleteProgram(program);
	program = 0;
}
