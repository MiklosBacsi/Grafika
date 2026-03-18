//=============================================================================================
// Roller Coaster
//=============================================================================================
#include "framework.h"

// vertex shader in GLSL
const char * vertSource = R"(
	#version 330
	uniform mat4 MVP;			// Model-View-Projection matrix
	layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0

	void main() {
		gl_Position = MVP * vec4(vertexPosition.x, vertexPosition.y, 0, 1); 		// transform to clipping space
//		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1); 		// transform to clipping space
	}
)";

// fragment shader in GLSL
const char * fragSource = R"(
	#version 330

	uniform vec3 color;
	out vec4 outColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() { outColor = vec4(color, 1); }
)";

// 2D camera
struct Camera { // 2D kamera
	float wCx = 0, wCy = 0;	    // ablak k�z�ppontja
	float wWx = 20, wWy = 20;	// ablak sz�less�g �s magass�g
public:
	mat4 V() { return translate(vec3(-wCx, -wCy, 0)); }
	mat4 P() { return scale(vec3(2.0f / wWx, 2.0f / wWy, 1)); }
	mat4 Vinv() { return translate(vec3(wCx, wCy, 0)); }
	mat4 Pinv() { return scale(vec3(wWx / 2.0f, wWy / 2.0f, 1)); }
};

class CatmullRomSpline {
protected:
	Geometry<vec2>  curve;	// B�zier g�rbe
	Geometry<vec2>  cps;	// B�zier g�rbe kontrollpontjai
	vec2 a0, a1, a2, a3;    // polinom egy�tthat�k
	int tauMax = 0;         // az akt�v param�tertartom�ny
	void Polynom(float tau) {
		for (unsigned int i = 0; i < tauMax; i++) {
			if (i <= tau && tau < i + 1) {
				vec2 p0 = cps.Vtx()[i], p1 = cps.Vtx()[i + 1];
				vec2 vPrev = (i > 0) ? (p0 - cps.Vtx()[i - 1]) : vec2(0, 0);
				vec2 vCur = (p1 - p0);
				vec2 vNext = (i < cps.Vtx().size() - 2) ? (cps.Vtx()[i + 2] - p1) : vec2(0, 0);
				vec2 v0 = (vPrev + vCur) / 2.0f;
				vec2 v1 = (vCur + vNext) / 2.0f;
				a0 = p0;
				a1 = v0;
				a2 = (p1 - p0) * 3.0f - (v1 + v0 * 2.0f);
				a3 = (p0 - p1) * 2.0f + (v1 + v0);
				return;
			}
		}
	}
public:
	void update() {
		tauMax = cps.Vtx().size() - 1;
		cps.updateGPU();
		curve.Vtx().clear();
		if (cps.Vtx().size() > 0) {
			const int nVertices = 100; // A k�zel�t� t�r�ttvonal cs�cssz�ma
			for (int i = 0; i < nVertices; i++) {	// Tesszell�ci�
				float tau = (float)i * tauMax / nVertices;
				curve.Vtx().push_back(r(tau));
			}
			curve.Vtx().push_back(r(tauMax - 0.0001f));
			curve.updateGPU();
		}
	}
	float paramMax() { return tauMax; }
	vec2 r(float tau) { // Catmull-Rom spline
		Polynom(tau);
		tau = tau - (int)tau;
		return ((a3 * tau + a2) * tau + a1) * tau + a0;
	}
	vec2 r1d(float tau) { // Els� deriv�lt
		Polynom(tau);
		tau = tau - (int)tau;
		return (3.0f * a3 * tau + 2.0f * a2) * tau + a1; 
	}
	vec2 r2d(float tau) { // M�sodik deriv�lt
		Polynom(tau);
		tau = tau - (int)tau;
		return 6.0f * a3 * tau + 2.0f * a2;
	}
	void Draw(GPUProgram* gpuProgram, Camera camera) {
		mat4 MVP = camera.P() * camera.V();
//		mat4 MVP = scale(vec3(1.0f, 1.0f, 1));

		gpuProgram->setUniform(MVP, "MVP");
		curve.Draw(gpuProgram, GL_LINE_STRIP, vec3(1, 1, 0)); // g�rbe
		cps.Draw(gpuProgram, GL_POINTS, vec3(1, 0, 0));	// kontrollpontok
	}
};

class InteractiveCurve : public  CatmullRomSpline {
	int picked = -1;		// kiv�lasztott kontrollpont sorsz�ma
public:
	void AddControlPoint(vec2 wP) {
		cps.Vtx().push_back(wP);
		update();
	}
	void PickControlPoint(vec2 wP) {
		for (unsigned int p = 0; p < cps.Vtx().size(); p++) {
			if (length(cps.Vtx()[p] - wP) < 1.0f) picked = p;
		}
	}
	void MoveControlPoint(vec2 wP) {
		if (picked >= 0) {
			cps.Vtx()[picked] = wP;
			update();
		}
	}
	void UnPickControlPoint() { picked = -1; }
};

class Circle : public Geometry<vec2> {
public:
	Circle() {
		const int nVertices = 100;
		for (int i = 0; i < nVertices; i++) {
			float phi = i * 2.0f * (float)M_PI / nVertices;
			vtx.push_back(vec2(cosf(phi), sinf(phi)));
		}
		updateGPU();
	}
};

class Spokes : public Geometry<vec2> {
public:
	Spokes() {
		const int nSpokes = 4;
		for (int i = 0; i < nSpokes; i++) {
			float phi = i * (float)M_PI / nSpokes;
			vtx.push_back(vec2(cosf(phi), sinf(phi)));
			vtx.push_back(vec2(-cosf(phi), -sinf(phi)));
		}
		updateGPU();
	}
};

const float g = 40.0f;
enum State { STANDBY, ROLLING, FLYING };

class Wheel {
	Circle circle; // ker�k bels� �s k�rvonal
	Spokes spokes; // k�ll�k
	float R = 1.0f;	// ker�k sugara
	float lambda = 1.0f;	// alakt�nyez�
	float tau = 0;	// hol vagyunk a g�rb�n
	vec2 touch, position, drdtau, T, N, linearVelocity;
	float yStart, speed, angle, angularVelocity;
	State state = STANDBY;
	InteractiveCurve* track;
	const float epsilon = 0.01f;
public:
	Wheel(InteractiveCurve* r)  {
		track = r;
		state = STANDBY;
	}
	State getState() { return state; }
	void SetFormFactor(float _lambda) {
		lambda = _lambda;
	}
	void Start() {
		printf("Start\n");
		state = ROLLING;
		tau = 0.01f;
		touch = track->r(tau);
		drdtau = track->r1d(tau);
		T = normalize(drdtau);	// �rint�
		N = vec2(-T.y, T.x);	// norm�lvektor
		position = touch + N * R;
		yStart = position.y;
		speed = 0.01;
		angle = 0;
	}

	void Animate(float dt) {
		switch (state) {
		case ROLLING: {
			float ds = speed * dt;	// megtett �t
			tau += ds / length(drdtau); // param�terv�ltoz�s
			if (tau > track->paramMax() || tau < 0) {
				state = FLYING; // Innent�l ferde haj�t�s forg�ssal
				break;
			}
			angle -= ds / R;	// elfordul�s v�ltoz�s
			touch = track->r(tau); // �j poz�ci�
			drdtau = track->r1d(tau); // �j deriv�lt
			T = normalize(drdtau);	// �rint�
			N = vec2(-T.y, T.x);	// norm�lvektor
			position = touch + N * R;
			// sebess�g energiamegmarad�sb�l
			if (yStart - position.y < 0) {
				Start();
				break;
			}
			speed = sqrtf(2 / (1 + lambda) * g * (yStart - position.y));
			// Lees�sn�l eszerint mozog
			linearVelocity = T * speed;
			angularVelocity = -speed / R;
			// g�rb�let sz�m�t�s a sz�ks�ges centripet�lis gyorsul�shoz
			vec2 d2rdtau2 = track->r2d(tau);
			float curvature = dot(d2rdtau2, N) / dot(drdtau, drdtau);
			// a nyom�er� el�g a p�ly�n tart�shoz?
			if (speed * speed * curvature + N.y * g < 0) {
				state = FLYING; // Innent�l ferde haj�t�s forg�ssal
			}
			break;
		}
		case FLYING:
			// Newton t�rv�ny megold�sa Euler integr�l�ssal
			linearVelocity += vec2(0, -g) * dt;
			position += linearVelocity * dt;
			// Perd�let megmarad�s
			angle += angularVelocity * dt;
		}
	}

	void Draw(GPUProgram * gpuProgram, Camera camera) {
		if (state == ROLLING || state == FLYING) {
			if (position.y < camera.wCy - camera.wWy / 2) Start();
			mat4 M = translate(vec3(position.x, position.y, 0)) * 
				     rotate(angle, vec3(0, 0, 1)) * scale(vec3(R, R, 1));
			mat4 MVP = camera.P() * camera.V() * M;			
			gpuProgram->setUniform(MVP, "MVP");
			circle.Draw(gpuProgram, GL_TRIANGLE_FAN, vec3(0, 0, 1));
			circle.Draw(gpuProgram, GL_LINE_LOOP, vec3(1, 1, 1));
			spokes.Draw(gpuProgram, GL_LINES, vec3(1, 1, 1));
		}
	}
};

const int winWidth = 600, winHeight = 600;

class MyApp : public glApp {
	GPUProgram* gpuProgram;	// vertex and fragment shaders
	InteractiveCurve* track;
	Wheel* wheel;
	Camera camera;
public:
	MyApp() : glApp(3, 3, winWidth, winHeight, "Roller coaster") { }

	// Initialization, create an OpenGL context
	void onInitialization() {
		glPointSize(10);
		glLineWidth(2);
		track = new InteractiveCurve();
		wheel = new Wheel(track);
		wheel->SetFormFactor(0.0f);
		gpuProgram = new GPUProgram(vertSource, fragSource);
	}
	// Window has become invalid: Redraw
	void onDisplay() {
		glClearColor(0, 0, 0, 0);     // background color
		glViewport(0, 0, winWidth, winHeight);
		glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer	
		track->Draw(gpuProgram, camera);
		wheel->Draw(gpuProgram, camera);
	}
	void onKeyboard(int key) {
		switch(key) {
		case ' ': wheel->Start(); break;
		case '0': wheel->SetFormFactor(0.0f); break;
		case '1': wheel->SetFormFactor(0.5f); break;
		case '2': wheel->SetFormFactor(1.0f); break;
		}
	}
	void onMousePressed(MouseButton button, int pX, int pY) {
		float cX = 2.0f * pX / winWidth - 1;	// flip y axis
		float cY = 1.0f - 2.0f * pY / winHeight;
		vec4 wP = camera.Vinv() * camera.Pinv() * vec4(cX, cY, 0, 1);
		if (button == MOUSE_LEFT) {
			track->AddControlPoint(vec2(wP.x, wP.y));
			refreshScreen();
		}
		if (button == MOUSE_RIGHT) {
			track->PickControlPoint(vec2(wP.x, wP.y));
		}
	}
	void onMouseReleased(MouseButton button, int pX, int pY) {
		if (button == MOUSE_RIGHT) track->UnPickControlPoint();
	}
	// Move mouse with key pressed
	void onMouseMotion(int pX, int pY) {
		float cX = 2.0f * pX / winWidth - 1;	// flip y axis
		float cY = 1.0f - 2.0f * pY / winHeight;
		vec4 wP = camera.Vinv() * camera.Pinv() * vec4(cX, cY, 0, 1);
		track->MoveControlPoint(vec2(wP.x, wP.y));
		refreshScreen();
	}

	void onTimeElapsed(float startTime, float endTime) {
		if (wheel->getState() == STANDBY) return;
		const float dt = 0.1f; // dt is �infinitesimal�

		for (float t = startTime; t < endTime; t += dt) {
			float Dt = fmin(dt, endTime - t);
			wheel->Animate(Dt);
		}
		refreshScreen(); 
	}
} app;
