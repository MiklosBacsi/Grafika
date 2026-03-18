//=============================================================================================
// Tesseract
//=============================================================================================
#include "framework.h"

const char * const vertSource = R"(
	#version 330

	uniform float cosa, sina, cosb, sinb;
	layout(location = 0) in vec4 vtx;	
	out float depthCue;

	void main() {
		vec4 vr1, vr2;
		// Forgat�s a zw s�kban alfa sz�ggel
		vr1 = vec4(vtx.xy, vtx.z*cosa-vtx.w*sina, vtx.z*sina+vtx.w*cosa);
		// Forgat�s a xw s�kban beta sz�ggel
		vr2 = vec4(vr1.x*cosb-vr1.w*sinb, vr1.yz, vr1.x*sinb+vr1.w*cosb);
		// Sk�l�z�s �s eltol�s a kamera el�
		vec4 p4d = vr2 * 0.4f + vec4(0, 0, 1, 1); 
		// Intenzit�s sz�m�t�s a t�vols�gb�l
		depthCue = 1 / (dot(p4d, p4d) - 0.4f);
		// Perspekt�v transzform�ci�
		gl_Position = vec4(p4d.xyz, p4d.z);
	}
)";

const char * const fragSource = R"(
	#version 330

	uniform vec3 color;
	in float depthCue;
	out vec4 outColor;
		
	void main() { outColor = vec4(color * depthCue, 1); }
)";

class Tesseract : public Geometry<vec4> {
	float alpha, beta;
	vec3 color;
	void addPoint(int c) {
		vec4 p(c & 1 ? 1 : -1, c & 2 ? 1 : -1, c & 4 ? 1 : -1, c & 8 ? 1 : -1);
		vtx.push_back(p);
	}
public:
	Tesseract() {
		const int maxcode = 15;
		for (int code = 0; code <= maxcode; code++) {
			for (int bit = 1; bit <= maxcode; bit <<= 1) {
				if ((code & bit) == 0) {
					addPoint(code);
					addPoint(code + bit);
				}
			}
		}
		updateGPU();
	}
	void Animate(float t) {
		alpha = t / 2; beta = t / 3;
		float s = 2 * M_PI / 3;
		color = vec3(sin(t) + 0.5, sin(t + s) + 0.5, sin(t + 2 * s) + 0.5);
	}
	void Draw(GPUProgram* gpuProgram) {
		gpuProgram->setUniform(cosf(alpha), "cosa");
		gpuProgram->setUniform(sinf(alpha), "sina");
		gpuProgram->setUniform(cosf(beta), "cosb");
		gpuProgram->setUniform(sinf(beta), "sinb");
		gpuProgram->setUniform(color, "color");
		Bind();
		glDrawArrays(GL_LINES, 0, (int)vtx.size());
	}
};


const int winWidth = 600, winHeight = 600;

class TesseractApp : public glApp {
	GPUProgram* gpuProgram;  // �rnyal� programok
	Tesseract* tesseract;    // 4D kocka
public:
	TesseractApp() : glApp("Tesseract") { }
	void onInitialization() {
		tesseract = new Tesseract();
		gpuProgram = new GPUProgram(vertSource, fragSource);
	}
	void onDisplay() {
		glClearColor(0, 0, 0, 0);     // h�tt�r sz�n
		glClear(GL_COLOR_BUFFER_BIT); // alkalmaz�i ablak t�rl�se
		glViewport(0, 0, winWidth, winHeight); // keletkez� f�nyk�p m�rete
		tesseract->Draw(gpuProgram);
	}

	void onTimeElapsed(float startTime, float endTime) {
		tesseract->Animate(endTime);
		refreshScreen(); // �jrarajzol�s
	}
} app;

