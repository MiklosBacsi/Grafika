//=============================================================================================
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni, ide�rtve ChatGPT-t �s t�rsait is
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

const char * const vertexSource = R"(
	#version 330
	precision highp float;
	layout(location = 0) in vec2 cP;
	void main() {
		gl_Position = vec4(cP.x, cP.y, 0, 1);
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

struct Line {
	vec2 *p1, *p2;

	Line(vec2* _p1, vec2* _p2) : p1(_p1), p2(_p2) {
		float a, b, c;
		getImplicit(a, b, c);
		printf("Line added\n");
		printf("    Implicit: %.1f x + %.1f y + %.1f = 0\n", a, b, c);
		printf("    Parametric: r(t) = <%.1f, %.1f> + <%.1f, %.1f>t\n", p1->x, p1->y, p2->x - p1->x, p2->y - p1->y);
	}

	void getImplicit(float& a, float& b, float& c) const {
		a = p1->y - p2->y;
		b = p2->x - p1->x;
		c = -a * p1->x - b * p1->y;
	}

	float dist(vec2 m) const {
		float a, b, c;
		getImplicit(a, b, c);
		return fabsf(a * m.x + b * m.y + c) / sqrtf(a * a + b * b);
	}
};

struct Circle {
	vec2 *p1, *p2, *p3;

	Circle(vec2* _p1, vec2* _p2, vec2* _p3) : p1(_p1), p2(_p2), p3(_p3) {
		vec2 center; float r;
		getGeometry(center, r);
		printf("Circle added\n");
		printf("    Implicit: (x - %.1f)^2 + (y - %.1f)^2 - %.1f^2 = 0\n", center.x, center.y, r);
		printf("    Parametric: r(t) = <%.1f + %.1f*cos(t), %.1f + %.1f*sin(t)>\n", center.x, r, center.y, r);
	}

	void getGeometry(vec2& center, float& r) const {
		float x1 = p1->x, y1 = p1->y, x2 = p2->x, y2 = p2->y, x3 = p3->x, y3 = p3->y;
		float D = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
		if (fabsf(D) < 1e-6) { center = *p1; r = 0; return; }
		center.x = ((x1*x1 + y1*y1)*(y2-y3) + (x2*x2 + y2*y2)*(y3-y1) + (x3*x3 + y3*y3)*(y1-y2)) / D;
		center.y = ((x1*x1 + y1*y1)*(x3-x2) + (x2*x2 + y2*y2)*(x1-x3) + (x3*x3 + y3*y3)*(x2-x1)) / D;
		r = length(*p1 - center);
	}

	float dist(vec2 m) const {
		vec2 c; float r;
		getGeometry(c, r);
		return fabsf(length(m - c) - r);
	}
};

class LineApp : public glApp {
	GPUProgram* gpuProgram;
	Geometry<vec2>* pointGeom;

	std::vector<Line> lines;
	std::vector<Circle> circles;

	char state = 'p'; 
	std::vector<vec2*> pickedForObj; 
	vec2* movedPoint = nullptr;      

	vec2 PixelToNDC(int pX, int pY) {
		return vec2(2.0f * pX / (float)windowWidth - 1.0f, 1.0f - 2.0f * pY / (float)windowHeight);
	}

public:
	LineApp() : glApp("Pontok, egyenesek és körök") {
		gpuProgram = nullptr;
		pointGeom = nullptr;
	}

	void onInitialization() {
		glViewport(0, 0, windowWidth, windowHeight);
		glLineWidth(3);
		glPointSize(10);
		glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		
		gpuProgram = new GPUProgram();
		gpuProgram->create(vertexSource, fragmentSource);
		pointGeom = new Geometry<vec2>(); 	
		pointGeom->Vtx().reserve(1000);
	}

	void onDisplay() {	
		// Background
		glClear(GL_COLOR_BUFFER_BIT);

		// Circles
		for (auto& circ : circles) {
			Geometry<vec2> tmp;
			vec2 c; float r; circ.getGeometry(c, r);
			for (int i = 0; i <= 100; i++) {
				float phi = i * 2.0f * M_PI / 100.0f;
				tmp.Vtx().push_back(vec2(c.x + r * cosf(phi), c.y + r * sinf(phi)));
			}
			tmp.updateGPU();
			tmp.Draw(gpuProgram, GL_LINE_STRIP, vec3(0, 1, 0));
		}

		// Lines
		for (auto& l : lines) {
			Geometry<vec2> tmp;
			tmp.Vtx().push_back(*l.p1 - normalize(*l.p2 - *l.p1) * 5.0f);
			tmp.Vtx().push_back(*l.p1 + normalize(*l.p2 - *l.p1) * 5.0f);
			tmp.updateGPU();
			tmp.Draw(gpuProgram, GL_LINES, vec3(0, 1, 1));
		}

		// Points
		if (pointGeom && pointGeom->Vtx().size() > 0) {
			pointGeom->Draw(gpuProgram, GL_POINTS, vec3(1, 0, 0));
		}
	}

	void onKeyboard(int key) {
		if (key == 'p' || key == 'l' || key == 'c' || key == 'i') {
			state = key;
			pickedForObj.clear();
			if (state == 'l') printf("Define lines\n");
			if (state == 'c') printf("Define circles\n");
			if (state == 'i') printf("Intersect\n");
		}
	}

	void onMousePressed(MouseButton but, int pX, int pY) {
		vec2 m = PixelToNDC(pX, pY);

		// Move point
		if (but == MOUSE_RIGHT) {
			float minD = 0.05f;
			for (int i = 0; i < (int)pointGeom->Vtx().size(); i++) {
				if (length(pointGeom->Vtx()[i] - m) < minD) {
					movedPoint = &pointGeom->Vtx()[i];
					printf("Move\n");
					break;
				}
			}
			return;
		}

		if (but != MOUSE_LEFT) return;

		// Point
		if (state == 'p') {
			addPoint(m.x, m.y);
		}

		// Line & Circle
		else if (state == 'l' || state == 'c') {
			vec2* found = nullptr; float minD = 0.05f;
			for (int i = 0; i < (int)pointGeom->Vtx().size(); i++) {
				if (length(pointGeom->Vtx()[i] - m) < minD) { found = &pointGeom->Vtx()[i]; break; }
			}
			if (found) {
				pickedForObj.push_back(found);
				if (state == 'l' && pickedForObj.size() == 2) {
					lines.push_back(Line(pickedForObj[0], pickedForObj[1]));
					pickedForObj.clear();
				}
				else if (state == 'c' && pickedForObj.size() == 3) {
					circles.push_back(Circle(pickedForObj[0], pickedForObj[1], pickedForObj[2]));
					pickedForObj.clear();
				}
			}
		}

		// Intersect
		else if (state == 'i') {
			static int s1 = -10000;
			int found = -1000;
			for (int i = 0; i < (int)lines.size(); i++) if (lines[i].dist(m) < 0.02f) found = i;
			if (found == -1000) {
				for (int i = 0; i < (int)circles.size(); i++) if (circles[i].dist(m) < 0.02f) found = -(i + 1);
			}
			if (found != -1000) {
				if (s1 == -10000) {
					s1 = found;
				}
				else { 
					intersect(s1, found); 
					s1 = -10000;
				}
			}
		}

		refreshScreen();
	}

	void onMouseReleased(MouseButton but, int pX, int pY) {
		// Deselect after releasing point
		if (but == MOUSE_RIGHT)
			movedPoint = nullptr;
	}

	void onMouseMotion(int pX, int pY) {
		// Move selected point
		if (movedPoint) {
			*movedPoint = PixelToNDC(pX, pY);
			pointGeom->updateGPU();
			refreshScreen();
		}
	}

	void intersect(int id1, int id2) {
		float a1, b1, c1, a2, b2, c2;
		vec2 ctr1, ctr2; float r1, r2;

		// Line-Line
		if (id1 >= 0 && id2 >= 0) {
			lines[id1].getImplicit(a1, b1, c1);
			lines[id2].getImplicit(a2, b2, c2);
			float det = a1 * b2 - a2 * b1;
			if (fabsf(det) > 1e-6) addPoint((b1 * c2 - b2 * c1) / det, (a2 * c1 - a1 * c2) / det);
		}

		// Line-Circle
		else if ((id1 >= 0 && id2 < 0) || (id1 < 0 && id2 >= 0)) {
			Line& l = lines[id1 >= 0 ? id1 : id2];
			Circle& circ = circles[id1 < 0 ? -id1 - 1 : -id2 - 1];
			l.getImplicit(a1, b1, c1); circ.getGeometry(ctr1, r1);
			vec2 n = normalize(vec2(a1, b1));
			float dist = (a1 * ctr1.x + b1 * ctr1.y + c1) / sqrtf(a1*a1 + b1*b1);
			if (fabsf(dist) <= r1) {
				vec2 p0 = ctr1 - n * dist;
				float h = sqrtf(r1 * r1 - dist * dist);
				vec2 v = vec2(-n.y, n.x);
				addPoint(p0.x + v.x * h, p0.y + v.y * h);
				if (h > 1e-3) addPoint(p0.x - v.x * h, p0.y - v.y * h);
			}
		}
		
		// Circle-Circle
		else {
			circles[-id1-1].getGeometry(ctr1, r1);
			circles[-id2-1].getGeometry(ctr2, r2);
			float d = length(ctr1 - ctr2);
			if (d > 0 && d <= r1 + r2 && d >= fabsf(r1 - r2)) {
				float a = (r1*r1 - r2*r2 + d*d) / (2*d);
				float h = sqrtf(std::max(0.0f, r1*r1 - a*a));
				vec2 p2 = ctr1 + a * (ctr2 - ctr1) / d;
				addPoint(p2.x + h*(ctr2.y-ctr1.y)/d, p2.y - h*(ctr2.x-ctr1.x)/d);
				if (h > 1e-3) addPoint(p2.x - h*(ctr2.y-ctr1.y)/d, p2.y + h*(ctr2.x-ctr1.x)/d);
			}
		}
	}

	void addPoint(float x, float y) {
		pointGeom->Vtx().push_back(vec2(x, y));
		pointGeom->updateGPU();
		printf("Point %.1f, %.1f added\n", x, y);
	}
};

LineApp app;