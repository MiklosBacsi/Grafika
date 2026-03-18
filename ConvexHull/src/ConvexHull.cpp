//=============================================================================================
// Convex Hull
//=============================================================================================
#include "framework.h"

// vertex shader in GLSL
const char * vertexSource = R"(
	#version 330

	layout(location = 0) in vec2 cP;	// Attrib Array 0

	void main() {
		gl_Position = vec4(cP, 0, 1);
	}
)";

// fragment shader in GLSL
const char * fragmentSource = R"(
	#version 330

	uniform vec3 color;
	out vec4 fragmentColor;		

	void main() {
		fragmentColor = vec4(color, 1); // extend RGB to RGBA
	}
)";

struct ConvexHull {
	Geometry<vec2> setPoints, hullPoints;

	void addPoint(vec2 p) { setPoints.Vtx().push_back(p); }
	void update() {
		if (setPoints.Vtx().size() >= 3) {
			findHull();
			hullPoints.updateGPU();
		}
		if (setPoints.Vtx().size() >= 1) setPoints.updateGPU();
	}
	vec2 * pickPoint(vec2 p) {
		for (auto& v : setPoints.Vtx()) {
			if (length(p - v) < 0.05f) return &v;
		}
		return nullptr;
	}

	void findHull() {
		hullPoints.Vtx().clear();
		vec2 * vStart = &setPoints.Vtx()[0];
		for (auto& v : setPoints.Vtx()) if (v.y < vStart->y) vStart = &v;

		vec2 vCur = *vStart, dir(1, 0), *vNext = vStart;
		do {
			float minAngle = M_PI;
			for (auto& v : setPoints.Vtx()) {
				float len = length(v - vCur);
				if (len > 0) {
					float phi = acos(dot(dir, v - vCur) / len);
					if (phi < minAngle) { minAngle = phi; vNext = &v; }
				}
			}
			hullPoints.Vtx().push_back(*vNext);
			dir = normalize(*vNext - vCur);
			vCur = *vNext;
		} while (vStart != vNext);
	}

	void Draw(GPUProgram * gpuProgram) {
		if (hullPoints.Vtx().size() >= 3) {
			hullPoints.Draw(gpuProgram, GL_TRIANGLE_FAN, vec3(0, 1, 1));
			hullPoints.Draw(gpuProgram, GL_LINE_LOOP, vec3(1, 1, 1));
		}
		setPoints.Draw(gpuProgram, GL_POINTS, vec3(1, 0, 0));
	}
};

int winWidth = 600, winHeight = 600;

class ConvexHullApp : public glApp {
	ConvexHull* hull = nullptr;
	vec2* pickedPoint = nullptr;
	GPUProgram gpuProgram;  // �rnyal� programok

	vec2 PixelToNDC(int pX, int pY) {
		vec2 cP;
		cP.x = 2.0f * pX / winWidth - 1;	// flip y axis
		cP.y = 1.0f - 2.0f * pY / winHeight;
		return cP;
	}
public:
	ConvexHullApp() : glApp("ConvexHull") { }
	void onInitialization() {
		glViewport(0, 0, winWidth, winHeight);
		glLineWidth(3);
		glPointSize(10);
		hull = new ConvexHull;
		gpuProgram.create(vertexSource, fragmentSource);
	}
	void onDisplay() {
		glClearColor(0, 0, 0, 0);	
		glClear(GL_COLOR_BUFFER_BIT); 
		hull->Draw(&gpuProgram);
	}
	void onMousePressed(MouseButton button, int pX, int pY) {
		if (button == MOUSE_LEFT) {  
			hull->addPoint(PixelToNDC(pX, pY));
			hull->update();
			refreshScreen();
		}
		if (button == MOUSE_RIGHT) {  
			pickedPoint = hull->pickPoint(PixelToNDC(pX, pY));
		}
	}
	void onMouseReleased(MouseButton button, int pX, int pY) {
		if (button == MOUSE_RIGHT) pickedPoint = nullptr;
	}
	void onMouseMotion(int pX, int pY) {
		if (pickedPoint) {
			*pickedPoint = PixelToNDC(pX, pY);
			hull->update();
			refreshScreen();
		}    
	}
} app;

