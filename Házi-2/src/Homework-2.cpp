//=============================================================================================
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni, ideertve ChatGPT-t is tarsait is
// - felesleges programsorokat a beadott programban hagyni 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL es GLM fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Bacsi Miklos
// Neptun : OECCSS
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================//=============================================================================================
#include "framework.h"

int windowWidth = 600, windowHeight = 600;
const float g = 5.0f;
const float BALL_R = 1.0f;
const int SPLINE_SEGS = 100;

mat4 TranslationMatrix(vec2 v) {
	return mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, v.x, v.y, 0, 1);
}

mat4 ScalingMatrix(vec2 v) {
	return mat4(v.x, 0, 0, 0, 0, v.y, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
}

const char * const vertexSource = R"(
	#version 330
	precision highp float;
	layout(location = 0) in vec2 vertexPos;
	uniform mat4 MVP;
	void main() {
		gl_Position = MVP * vec4(vertexPos.x, vertexPos.y, 0, 1);
	}
)";

const char * const fragmentSource = R"(
	#version 330
	precision highp float;
	uniform vec3 color;
	out vec4 fragmentColor;
	void main() {
		fragmentColor = vec4(color, 1);
	}
)";

GPUProgram* gpuProgram;

struct Camera {
	vec2 wCenter; vec2 wSize;
	Camera() : wCenter(25, 25), wSize(50, 50) {}
	mat4 MVP() { return ScalingMatrix(vec2(2.0f / wSize.x, 2.0f / wSize.y)) * TranslationMatrix(vec2(-wCenter.x, -wCenter.y)); }
	vec2 pToW(int pX, int pY) { 
		float cX = 2.0f * (float)pX / (float)windowWidth - 1.0f;
		float cY = 1.0f - 2.0f * (float)pY / (float)windowHeight;
		return vec2(cX * wSize.x / 2.0f + wCenter.x, cY * wSize.y / 2.0f + wCenter.y);
	}
} camera;

class ClosedCatmullRomSpline {
	std::vector<vec2> cps;
	std::vector<float> knotValues;
	std::vector<vec2> polyline;
	Geometry<vec2>* splineGeom;
	Geometry<vec2>* cpGeom;

	void computeKnots() {
		int n = cps.size();
		if (n < 2) return;
		knotValues.resize(n);
		knotValues[0] = 0.0f;
		for (int i = 1; i < n; ++i) knotValues[i] = knotValues[i - 1] + length(cps[i] - cps[i - 1]);
	}

	float getParam(int i) const {
		int n = cps.size();
		float totalLen = knotValues[n - 1] + length(cps[0] - cps[n - 1]);
		int ii = ((i % n) + n) % n;
		int rounds = (i < 0) ? (i - n + 1) / n : i / n;
		return knotValues[ii] + rounds * totalLen;
	}

	vec2 velocity(int i) const {
		int n = cps.size();
		float dt0 = getParam(i) - getParam(i - 1);
		float dt1 = getParam(i + 1) - getParam(i);
		vec2 v0 = (cps[i % n] - cps[(i - 1 + n) % n]) / dt0;
		vec2 v1 = (cps[(i + 1) % n] - cps[i % n]) / dt1;
		return 0.5f * (v0 + v1);
	}

	vec2 hermite(vec2 p0, vec2 v0, float t0, vec2 p1, vec2 v1, float t1, float t) const {
		float dt = t1 - t0;
		float s = (t - t0) / dt;
		return p0 * (2 * s * s * s - 3 * s * s + 1) + v0 * dt * (s * s * s - 2 * s * s + s) +
			   p1 * (-2 * s * s * s + 3 * s * s) + v1 * dt * (s * s * s - s * s);
	}

public:
	ClosedCatmullRomSpline() : splineGeom(nullptr), cpGeom(nullptr) {}
	void init() { splineGeom = new Geometry<vec2>(); cpGeom = new Geometry<vec2>(); }
	void addControlPoint(vec2 p) { cps.push_back(p); computeKnots(); if (cps.size() >= 3) updateSpline(); }
	void moveCP(int idx, vec2 pos) { if (idx >= 0 && idx < (int)cps.size()) { cps[idx] = pos; computeKnots(); if (cps.size() >= 3) updateSpline(); } }
	int findCP(vec2 pos) { for (int i = 0; i < (int)cps.size(); i++) if (length(cps[i] - pos) < 2.0f) return i; return -1; }

	vec2 r(float u) const {
		int n = cps.size();
		float totalLen = knotValues[n - 1] + length(cps[0] - cps[n - 1]);
		float t = u * totalLen;
		for (int i = 0; i < n; i++) {
			float t0 = getParam(i), t1 = getParam(i + 1);
			if (t >= t0 && t <= t1) return hermite(cps[i % n], velocity(i), t0, cps[(i + 1) % n], velocity(i + 1), t1, t);
		}
		return cps[0];
	}

	void updateSpline() {
		polyline.clear();
		for (int i = 0; i <= SPLINE_SEGS; i++) polyline.push_back(r((float)i / SPLINE_SEGS));
		splineGeom->Vtx() = polyline;
		splineGeom->updateGPU();
	}

	void draw() {
		if (!splineGeom) return;
		gpuProgram->setUniform(camera.MVP(), "MVP");
		if (cps.size() >= 3) splineGeom->Draw(gpuProgram, GL_LINE_STRIP, vec3(1, 1, 0));
		cpGeom->Vtx() = cps; cpGeom->updateGPU();
		glPointSize(10); cpGeom->Draw(gpuProgram, GL_POINTS, vec3(1, 0, 0));
	}

	const std::vector<vec2>& getPolyline() { return polyline; }
};

struct Ball {
	vec2 pos, vel;
	Geometry<vec2>* ballGeom;

	Ball(vec2 p, vec2 v) : pos(p), vel(v) {
		ballGeom = new Geometry<vec2>();
		ballGeom->Vtx().push_back(vec2(0, 0));
		for (int i = 0; i <= 64; i++) {
			float phi = i * 2.0f * M_PI / 64.0f;
			ballGeom->Vtx().push_back(vec2(cosf(phi), sinf(phi)) * BALL_R);
		}
		ballGeom->updateGPU();
	}

	void update(float dt, ClosedCatmullRomSpline& spline) {
		float timeLeft = dt;
		for (int iter = 0; iter < 10 && timeLeft > 0.001f; iter++) {
			float t_hit = timeLeft; vec2 normal; bool hit = false;
			const auto& wall = spline.getPolyline();
			if (wall.size() < 2) break;

			for (size_t i = 0; i < wall.size() - 1; i++) {
				vec2 A = wall[i], B = wall[i+1];
				vec2 seg = B - A; vec2 n = normalize(vec2(-seg.y, seg.x));
				float a = -n.y * g * 0.5f;
				float b = dot(n, vel);
				float c = dot(n, pos - A);

				float disc = b * b - 4 * a * c;
				if (disc >= 0) {
					float sqrtD = sqrtf(disc);
					float ts[2] = { (-b - sqrtD) / (2 * a + 1e-9f), (-b + sqrtD) / (2 * a + 1e-9f) };
					for (float t : ts) {
						if (t > 1e-4f && t < t_hit) {
							vec2 centerAtT = pos + vel * t + vec2(0, -0.5f * g * t * t);
							float proj = dot(centerAtT - A, seg) / dot(seg, seg);
							if (proj >= 0 && proj <= 1.0f) { t_hit = t; normal = n; hit = true; }
						}
					}
				}
			}
			pos = pos + vel * t_hit + vec2(0, -0.5f * g * t_hit * t_hit);
			vel = vel + vec2(0, -g * t_hit);
			timeLeft -= t_hit;
			if (hit) {
				if (dot(vel, normal) > 0) normal = -normal;
				vel = vel - normal * dot(vel, normal) * 2.0f;
				pos = pos + normal * 0.001f;
			}
		}
	}

	void draw() {
		gpuProgram->setUniform(camera.MVP() * TranslationMatrix(pos), "MVP");
		ballGeom->Draw(gpuProgram, GL_TRIANGLE_FAN, vec3(0, 0, 1));
	}
};

class App : public glApp {
	ClosedCatmullRomSpline spline;
	std::vector<Ball*> balls;
	vec2 rightStart; bool slingshotActive = false;
	int movedCPIdx = -1;
	int lastX, lastY;

public:
	App() : glApp("Pattogo Labda") { }

	void onInitialization() {
		gpuProgram = new GPUProgram();
		gpuProgram->create(vertexSource, fragmentSource);
		spline.init();
		glLineWidth(3);
		glClearColor(0, 0, 0, 1);
	}

	void onDisplay() {
		glClear(GL_COLOR_BUFFER_BIT);
		spline.draw();
		for (auto b : balls) b->draw();
	}

	void onMousePressed(MouseButton but, int pX, int pY) {
		vec2 w = camera.pToW(pX, pY);
		if (but == MOUSE_LEFT) {
			int found = spline.findCP(w);
			if (found != -1) movedCPIdx = found;
			else spline.addControlPoint(w);
		} else if (but == MOUSE_RIGHT) {
			rightStart = w; slingshotActive = true;
		}
		refreshScreen();
	}

	void onMouseReleased(MouseButton but, int pX, int pY) {
		if (but == MOUSE_RIGHT && slingshotActive) {
			vec2 w = camera.pToW(pX, pY);
			balls.push_back(new Ball(w, rightStart - w));
			slingshotActive = false;
		}
		movedCPIdx = -1;
		refreshScreen();
	}

	void onMouseMotion(int pX, int pY) {
		lastX = pX; lastY = pY;
		if (movedCPIdx != -1) spline.moveCP(movedCPIdx, camera.pToW(pX, pY));
		refreshScreen();
	}

	void onTimeElapsed(float tStart, float tEnd) {
		for (auto b : balls) b->update(tEnd - tStart, spline);
		refreshScreen();
	}
} app;