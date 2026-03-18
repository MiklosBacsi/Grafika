//=============================================================================================
// OpenGL keretrendszer
//=============================================================================================
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES		// M_PI
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <string>

#define GLUT 1
#define GLFW 2
#define FRAMEWORK GLFW

#if FRAMEWORK == GLUT
// GLUT implementation
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/glew.h>		// must be downloaded
#include <GL/freeglut.h>	// must be downloaded unless you have an Apple
// Resolution of screen
const unsigned int windowWidth = 600, windowHeight = 600;
#else
#define GLAD_GL_IMPLEMENTATION
#include <glad/glad.h>
#endif

//#define USE_GLM
#ifdef USE_GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;
inline mat4 translate(const vec3& v) { return translate(mat4(1.0f), v); }
inline mat4 scale(const vec3& v) { return scale(mat4(1.0f), v); }
inline mat4 rotate(float angle, const vec3& v) { return rotate(mat4(1.0f), angle, v); }
#else
namespace glm {
	//--------------------------
	struct vec2 {
		//--------------------------
		float x, y;

		vec2() { x = y = 0; }
		vec2(float x0) { x = y = x0; }
		vec2(float x0, float y0) { x = x0; y = y0; }
		vec2 operator*(float a) const { return vec2(x * a, y * a); }
		vec2 operator/(float a) const { return vec2(x / a, y / a); }
		vec2 operator+(const vec2& v) const { return vec2(x + v.x, y + v.y); }
		vec2 operator-(const vec2& v) const { return vec2(x - v.x, y - v.y); }
		vec2 operator*(const vec2& v) const { return vec2(x * v.x, y * v.y); }
		vec2 operator-() const { return vec2(-x, -y); }
		void operator+=(const vec2& v) { x += v.x; y += v.y; }
	};

	inline float dot(const vec2& v1, const vec2& v2) {
		return (v1.x * v2.x + v1.y * v2.y);
	}

	inline float length(const vec2& v) { return sqrtf(dot(v, v)); }

	inline vec2 normalize(const vec2& v) { return v * (1 / length(v)); }

	inline vec2 operator*(float a, const vec2& v) { return vec2(v.x * a, v.y * a); }

	//--------------------------
	struct vec3 {
		//--------------------------
		float x, y, z;

		vec3() { x = y = z = 0; }
		vec3(float x0) { x = y = z = x0; }
		vec3(float x0, float y0, float z0) { x = x0; y = y0; z = z0; }
		vec3(vec2 v) { x = v.x; y = v.y; z = 0; }

		vec3 operator*(float a) const { return vec3(x * a, y * a, z * a); }
		vec3 operator/(float a) const { return vec3(x / a, y / a, z / a); }
		vec3 operator+(const vec3& v) const { return vec3(x + v.x, y + v.y, z + v.z); }
		vec3 operator-(const vec3& v) const { return vec3(x - v.x, y - v.y, z - v.z); }
		vec3 operator*(const vec3& v) const { return vec3(x * v.x, y * v.y, z * v.z); }
		vec3 operator-()  const { return vec3(-x, -y, -z); }
		void operator+=(const vec3& v) { x += v.x; y += v.y; z += v.z; }
		void operator-=(const vec3& v) { x -= v.x; y -= v.y; z -= v.z; }
		void operator/=(float d) { x /= d; y /= d; z /= d; }
	};

	inline float dot(const vec3& v1, const vec3& v2) { return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z); }

	inline float length(const vec3& v) { return sqrtf(dot(v, v)); }

	inline vec3 normalize(const vec3& v) { return v * (1 / length(v)); }

	inline vec3 cross(const vec3& v1, const vec3& v2) {
		return vec3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
	}

	inline vec3 operator*(float a, const vec3& v) { return vec3(v.x * a, v.y * a, v.z * a); }

	//--------------------------
	struct vec4 {
		//--------------------------
		float x, y, z, w;

		vec4(float x0 = 0, float y0 = 0, float z0 = 0, float w0 = 0) { x = x0; y = y0; z = z0; w = w0; }
		float& operator[](int j) { return *(&x + j); }
		float operator[](int j) const { return *(&x + j); }

		vec4 operator*(float a) const { return vec4(x * a, y * a, z * a, w * a); }
		vec4 operator/(float d) const { return vec4(x / d, y / d, z / d, w / d); }
		vec4 operator+(const vec4& v) const { return vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
		vec4 operator-(const vec4& v)  const { return vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
		vec4 operator*(const vec4& v) const { return vec4(x * v.x, y * v.y, z * v.z, w * v.w); }
		void operator+=(const vec4 right) { x += right.x; y += right.y; z += right.z, w += right.z; }
	};

	inline float dot(const vec4& v1, const vec4& v2) {
		return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w);
	}

	inline vec4 operator*(float a, const vec4& v) {
		return vec4(v.x * a, v.y * a, v.z * a, v.w * a);
	}

	//---------------------------
	struct mat4 { // row-major matrix 4x4
	//---------------------------
		vec4 cols[4];
	public:
		mat4() {}
		mat4(float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33) {
			cols[0][0] = m00; cols[0][1] = m01; cols[0][2] = m02; cols[0][3] = m03;
			cols[1][0] = m10; cols[1][1] = m11; cols[1][2] = m12; cols[1][3] = m13;
			cols[2][0] = m20; cols[2][1] = m21; cols[2][2] = m22; cols[2][3] = m23;
			cols[3][0] = m30; cols[3][1] = m31; cols[3][2] = m32; cols[3][3] = m33;
		}
		mat4(vec4 it, vec4 jt, vec4 kt, vec4 ot) {
			cols[0] = it; cols[1] = jt; cols[2] = kt; cols[3] = ot;
		}

		vec4& operator[](int i) { return cols[i]; }
		vec4 operator[](int i) const { return cols[i]; }
		operator float* () const { return (float*)this; }
	};

	inline vec4 operator*(const mat4& mat, const vec4& v) {
		return mat[0] * v[0] + mat[1] * v[1] + mat[2] * v[2] + mat[3] * v[3];
	}

	inline mat4 operator*(const mat4& left, const mat4& right) {
		mat4 result;
		for (int i = 0; i < 4; i++) result[i] = left * right.cols[i];
		return result;
	}


	inline mat4 translate(const vec3& v) {
		return mat4(vec4(1, 0, 0, 0),
			vec4(0, 1, 0, 0),
			vec4(0, 0, 1, 0),
			vec4(v.x, v.y, v.z, 1));
	}

	inline mat4 scale(const vec3& s) {
		return mat4(vec4(s.x, 0, 0, 0),
			vec4(0, s.y, 0, 0),
			vec4(0, 0, s.z, 0),
			vec4(0, 0, 0, 1));
	}

	inline mat4 rotate(float angle, const vec3& ww) {
		float c = cosf(angle), s = sinf(angle);
		vec3 w = normalize(ww);
		return mat4(vec4(c * (1 - w.x * w.x) + w.x * w.x, w.x * w.y * (1 - c) + w.z * s, w.x * w.z * (1 - c) - w.y * s, 0),
			vec4(w.x * w.y * (1 - c) - w.z * s, c * (1 - w.y * w.y) + w.y * w.y, w.y * w.z * (1 - c) + w.x * s, 0),
			vec4(w.x * w.z * (1 - c) + w.y * s, w.y * w.z * (1 - c) - w.x * s, c * (1 - w.z * w.z) + w.z * w.z, 0),
			vec4(0, 0, 0, 1));
	}

	inline mat4 lookAt(vec3 wEye, vec3 wLookat, vec3 wVup) { // view matrix: translates the center to the origin
		vec3 w = normalize(wEye - wLookat);
		vec3 u = normalize(cross(wVup, w));
		vec3 v = cross(w, u);
		return mat4(u.x, v.x, w.x, 0,
			u.y, v.y, w.y, 0,
			u.z, v.z, w.z, 0,
			0, 0, 0, 1) * translate(wEye * (-1));
	}

	inline mat4 perspective(float fov, float asp, float fp, float bp) { // projection matrix
		return mat4(1 / (tanf(fov / 2) * asp), 0, 0, 0,
			0, 1 / tanf(fov / 2), 0, 0,
			0, 0, -(fp + bp) / (bp - fp), -1,
			0, 0, -2 * fp * bp / (bp - fp), 0);
	}
}
using namespace glm;
#endif

#define FILE_OPERATIONS
#ifdef FILE_OPERATIONS
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

#include "lodepng.h"
#endif

//---------------------------
class GPUProgram {
//--------------------------
	GLuint shaderProgramId = 0;
	bool waitError = true;

	bool checkShader(unsigned int shader, std::string message) { // shader ford�t�si hib�k kezel�se
		GLint infoLogLength = 0, result = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (!result) {
			std::string errorMessage(infoLogLength, '\0');
			glGetShaderInfoLog(shader, infoLogLength, NULL, (GLchar *)errorMessage.data());
			printf("%s! \n Log: \n%s\n", message.c_str(), errorMessage.c_str());
			if (waitError) getchar();
			return false;
		}
		return true;
	}

	bool checkLinking(unsigned int program) { 	// shader szerkeszt�si hib�k kezel�se
		GLint infoLogLength = 0, result = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &result);
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (!result) {
			std::string errorMessage(infoLogLength, '\0');
			glGetProgramInfoLog(program, infoLogLength, nullptr, (GLchar *)errorMessage.data());
			printf("Failed to link shader program! \n Log: \n%s\n", errorMessage.c_str());
			if (waitError) getchar();
			return false;
		}
		return true;
	}

	int getLocation(const std::string& name) {	// uniform v�ltoz� c�m�nek lek�rdez�se
		int location = glGetUniformLocation(shaderProgramId, name.c_str());
		if (location < 0) printf("uniform %s cannot be set\n", name.c_str());
		return location;
	}

public:
	GPUProgram() { }
	GPUProgram(const char* const vertexShaderSource, const char * const fragmentShaderSource, const char * const geometryShaderSource = nullptr) {
		create(vertexShaderSource, fragmentShaderSource, geometryShaderSource);
	}

	void create(const char* const vertexShaderSource, const char * const fragmentShaderSource, const char * const geometryShaderSource = nullptr) {
		// Program l�trehoz�sa a forr�s sztringb�l
		GLuint  vertexShader = glCreateShader(GL_VERTEX_SHADER);
		if (!vertexShader) {
			printf("Error in vertex shader creation\n");
			exit(1);
		}
		glShaderSource(vertexShader, 1, (const GLchar**)&vertexShaderSource, NULL);
		glCompileShader(vertexShader);
		if (!checkShader(vertexShader, "Vertex shader error")) return;

		// Program l�trehoz�sa a forr�s sztringb�l, ha van geometria �rnyal�
		GLuint geometryShader = 0;
		if (geometryShaderSource != nullptr) {
			geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
			if (!geometryShader) {
				printf("Error in geometry shader creation\n");
				exit(1);
			}
			glShaderSource(geometryShader, 1, (const GLchar**)&geometryShaderSource, NULL);
			glCompileShader(geometryShader);
			if (!checkShader(geometryShader, "Geometry shader error")) return;
		}

		// Program l�trehoz�sa a forr�s sztringb�l
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		if (!fragmentShader) {
			printf("Error in fragment shader creation\n");
			exit(1);
		}

		glShaderSource(fragmentShader, 1, (const GLchar**)&fragmentShaderSource, NULL);
		glCompileShader(fragmentShader);
		if (!checkShader(fragmentShader, "Fragment shader error")) return;

		shaderProgramId = glCreateProgram();
		if (!shaderProgramId) {
			printf("Error in shader program creation\n");
			exit(-1);
		}
		glAttachShader(shaderProgramId, vertexShader);
		glAttachShader(shaderProgramId, fragmentShader);
		if (geometryShader > 0) glAttachShader(shaderProgramId, geometryShader);

		// Connect the fragmentColor to the frame buffer memory
		glBindFragDataLocation(shaderProgramId, 0, "fragmentColor");	// this output goes to the frame buffer memory

		// Szerkeszt�s
		if (!link()) return;

		// Ez fusson
		glUseProgram(shaderProgramId);
	}

	bool link() {
		glLinkProgram(shaderProgramId);
		return checkLinking(shaderProgramId);
	}

	void Use() { glUseProgram(shaderProgramId); } 		// make this program run

	void setUniform(int i, const std::string& name) {
		int location = getLocation(name);
		if (location >= 0) glUniform1i(location, i);
	}

	void setUniform(float f, const std::string& name) {
		int location = getLocation(name);
		if (location >= 0) glUniform1f(location, f);
	}

	void setUniform(const vec2& v, const std::string& name) {
		int location = getLocation(name);
		if (location >= 0) glUniform2fv(location, 1, &v.x);
	}

	void setUniform(const vec3& v, const std::string& name) {
		int location = getLocation(name);
		if (location >= 0) glUniform3fv(location, 1, &v.x);
	}

	void setUniform(const vec4& v, const std::string& name) {
		int location = getLocation(name);
		if (location >= 0) glUniform4fv(location, 1, &v.x);
	}

	void setUniform(const mat4& mat, const std::string& name) {
		int location = getLocation(name);
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_FALSE, (float *)mat);
	}

	~GPUProgram() { if (shaderProgramId > 0) glDeleteProgram(shaderProgramId); }
};

//---------------------------
template<class T>
class Geometry {
//---------------------------
	unsigned int vao, vbo;	// GPU
protected:
	std::vector<T> vtx;	// CPU
public:
	Geometry() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableVertexAttribArray(0);
		int nf = (int)(sizeof(T) / sizeof(float));
		if (nf <= 4) glVertexAttribPointer(0, nf, GL_FLOAT, GL_FALSE, 0, NULL);
	}
	std::vector<T>& Vtx() { return vtx; }
	void updateGPU() {	// CPU -> GPU
		if (vtx.size() > 0) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(T), &vtx[0], GL_DYNAMIC_DRAW);
		}
	}
	void Bind() { glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo); } // aktiv�l�s
	void Draw(GPUProgram* prog, int type, vec3 color) {
		if (vtx.size() > 0) {
			prog->setUniform(color, "color");
			glBindVertexArray(vao);
			glDrawArrays(type, 0, (int)vtx.size());
		}
	}
	virtual ~Geometry() {
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
	}
};

//---------------------------
class Texture {
//---------------------------
	unsigned int textureId = 0;
public:
#ifdef FILE_OPERATIONS
	Texture(const fs::path pathname, bool transparent = false, int sampling = GL_LINEAR) {
		if (textureId == 0) glGenTextures(1, &textureId);  				// azonos�t� gener�l�s
		glBindTexture(GL_TEXTURE_2D, textureId);    // k�t�s
		unsigned int width, height;
		unsigned char* pixels;
		if (transparent) {
			lodepng_decode32_file(&pixels, &width, &height, pathname.string().c_str());
			for (unsigned int y = 0; y < height; ++y) {
				for (unsigned int x = 0; x < width; ++x) {
					float sum = 0;
					for (int c = 0; c < 3; ++c) {
						sum += pixels[4 * (x + y * width) + c];
					}
					pixels[4 * (x + y * width) + 3] = (unsigned char)(sum / 6);
				}
			}

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels); // GPU-ra
		}
		else {
			lodepng_decode24_file(&pixels, &width, &height, pathname.string().c_str());
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels); // GPU-ra
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampling); // sz�r�s
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampling);
		printf("%s, w: %d, h: %d\n", pathname.string().c_str(), width, height);
	}
#endif
	Texture(int width, int height) {
		glGenTextures(1, &textureId); // azonos�t� gener�l�sa
		// procedur�lis text�ra el��ll�t�sa programmal
		const vec3 yellow(1, 1, 0), blue(0, 0, 1);
		std::vector<vec3> image(width * height);
		for (int x = 0; x < width; x++) for (int y = 0; y < height; y++) {
			image[y * width + x] = (x & 1) ^ (y & 1) ? yellow : blue;
		}
		updateTexture(width, height, image);
	}

	void updateTexture(int width, int height, std::vector<vec3>& image) {
		glBindTexture(GL_TEXTURE_2D, textureId);    // ez az akt�v innent�l
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, &image[0]); // To GPU
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	Texture() {
		glGenTextures(1, &textureId); // azonos�t� gener�l�sa
		glBindTexture(GL_TEXTURE_2D, textureId);    // ez az akt�v innent�l
	}

	void Bind(int textureUnit) {
		glActiveTexture(GL_TEXTURE0 + textureUnit); // aktiv�l�s
		glBindTexture(GL_TEXTURE_2D, textureId); // piros ny�l
	}
	~Texture() {
		if (textureId > 0) glDeleteTextures(1, &textureId);
	}
};

enum MouseButton { MOUSE_LEFT, MOUSE_MIDDLE, MOUSE_RIGHT };
enum SpecialKeys { KEY_RIGHT = 262, KEY_LEFT = 263, KEY_DOWN = 264, KEY_UP = 265 };

bool pollKey(int key);
float getElapsedTime();

//---------------------------
class glApp {
//---------------------------
public:
	glApp(const char* caption);
	glApp(unsigned int major, unsigned int minor,        // K�rt OpenGL major.minor verzi�
		unsigned int winWidth, unsigned int winHeight, // Alkalmaz�i ablak felbont�sa
		const char* caption);       // Megfog�cs�k sz�vege
	void refreshScreen(); // Ablak �rv�nytelen�t�se
	// Esem�nykezel�k
	virtual void onInitialization() {}    // Inicializ�ci�
	virtual void onDisplay() {}           // Ablak �rv�nytelen
	virtual void onKeyboard(int key) {}   // Klaviat�ra gomb lenyom�s
	virtual void onKeyboardUp(int key) {} // Klaviat�ra gomb elenged
	// Eg�r gomb lenyom�s/elenged�s
	virtual void onMousePressed(MouseButton but, int pX, int pY) {}
	virtual void onMouseReleased(MouseButton but, int pX, int pY) {}
	// Eg�r mozgat�s lenyomott gombbal
	virtual void onMouseMotion(int pX, int pY) {}
	// Telik az id�
	virtual void onTimeElapsed(float startTime, float endTime) {}
};

