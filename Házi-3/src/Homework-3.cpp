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

const char * const vertexSource = R"(
	#version 330
	precision highp float;
	layout(location = 0) in vec2 vertexPos;
	uniform mat4 MVP;
	out vec2 texCoord;
	void main() {
		gl_Position = MVP * vec4(vertexPos.x, vertexPos.y, 0, 1);
		texCoord = (vertexPos + vec2(1.0)) * 0.5;
	}
)";

const char * const fragmentSource = R"(
	#version 330
	precision highp float;
	uniform vec3 color;
	uniform sampler2D tex;
	uniform int useTexture;
	in vec2 texCoord;
	out vec4 fragmentColor;
	void main() {
		if (useTexture != 0) {
			fragmentColor = vec4(texture(tex, texCoord).rgb, 1);
		} else {
			fragmentColor = vec4(color, 1);
		}
	}
)";

GPUProgram* gpuProgram = nullptr;
mat4 identityMatrix = mat4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);

struct Map {
	std::vector<vec3> original;
	std::vector<vec3> current;
	Geometry<vec2>* geom;
	Texture* texture;

	Map() : geom(nullptr), texture(nullptr) {}

	void init() {
		geom = new Geometry<vec2>();
		geom->Vtx().push_back(vec2(-1, -1));
		geom->Vtx().push_back(vec2(1, -1));
		geom->Vtx().push_back(vec2(-1, 1));
		geom->Vtx().push_back(vec2(1, 1));
		geom->updateGPU();

		texture = new Texture(64, 64);
	}

	void decodeRLE() {
		const uint16_t rleData[] = {
			2308,13,84,5,152,17,76,17,144,29,64,25,136,45,16,5,32,29,132,65,
			4,21,8,33,128,85,12,41,120,93,4,73,20,9,52,101,4,81,4,29,12,5,
			12,1129,14,241,18,241,18,245,14,145,6,97,18,125,6,9,10,97,18,53,
			10,61,30,93,22,45,14,61,30,93,22,45,18,5,6,49,30,93,30,37,22,
			61,18,97,30,37,22,61,14,97,22,9,6,37,22,45,6,9,6,9,10,93,26,
			33,6,5,26,41,6,5,10,5,6,101,26,33,42,49,6,101,14,5,6,41,46,21,
			6,17,6,9,6,93,14,53,50,29,14,105,6,9,10,41,54,13,34,93,14,13,
			6,41,54,9,42,85,30,49,26,21,58,81,34,45,26,21,62,5,6,65,42,69,
			6,13,62,73,50,41,10,5,14,13,66,69,42,9,6,37,106,61,58,37,106,
			61,34,5,18,37,6,13,90,13,6,25,10,17,30,9,10,57,98,33,30,5,26,
			33,4,33,102,9,18,13,50,5,6,9,10,9,6,4,17,4,17,122,5,6,5,46,5,
			8,9,10,9,6,8,37,82,5,10,5,10,5,14,9,38,5,24,6,13,6,16,29,18,
			9,34,5,58,17,10,13,6,5,36,17,12,41,6,17,6,5,74,53,32,17,20,57,
			6,5,50,5,6,5,6,57,32,21,20,61,18,5,22,5,18,5,6,61,28,17,24,
			61,4,6,5,6,21,4,21,6,65,4,9,20,9,28,29,4,45,6,121,24,5,32,29,
			4,37,6,5,6,121,20,9,28,205,20,5,28,81,4,125,16,5,24,5,4,209,
			12,5,4,5,16,217,8,17,16,913
		};
		original.resize(64 * 64);
		size_t pix = 0;
		for (size_t i = 0; i < sizeof(rleData) / sizeof(uint16_t); ++i) {
			uint16_t word = rleData[i];
			uint16_t h = word >> 2;
			uint16_t idx = word & 3;
			vec3 col = (idx == 0) ? vec3(1,1,1) : (idx == 1) ? vec3(0,0,1) : (idx == 2) ? vec3(0,1,0) : vec3(0,0,0);
			for (uint16_t j = 0; j < h && pix < 4096; ++j) {
				original[pix++] = col;
			}
		}
	}

	void updateTexture() {
		current = original;
		texture->updateTexture(64, 64, current);
	}

	void draw() {
		gpuProgram->setUniform(identityMatrix, "MVP");
		gpuProgram->setUniform(1, "useTexture");
		gpuProgram->setUniform(0, "tex");
		texture->Bind(0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		geom->Draw(gpuProgram, GL_TRIANGLE_STRIP, vec3(1,1,1));
	}
};

float merc(float lat_deg) {
	float lat_rad = lat_deg * M_PI / 180.0f;
	return logf(tanf(M_PI / 4.0f + lat_rad / 2.0f));
}

vec3 mapToSphere(float wx, float wy, float merc_min, float merc_max) {
	float lon_rad = wx * M_PI;
	float merc_p = merc_min + (merc_max - merc_min) * (wy + 1.0f) * 0.5f;
	float lat_rad = 2.0f * atanf(expf(merc_p)) - M_PI / 2.0f;
	float cl = cosf(lat_rad);
	return vec3(cl * cosf(lon_rad), cl * sinf(lon_rad), sinf(lat_rad));
}

vec2 sphereToMap(const vec3& p, float merc_min, float merc_max) {
	float lat_rad = asinf(p.z);
	float lon_rad = atan2f(p.y, p.x);
	float merc_p = logf(tanf(M_PI / 4.0f + lat_rad / 2.0f));
	float wy = -1.0f + 2.0f * (merc_p - merc_min) / (merc_max - merc_min);
	float wx = lon_rad / M_PI;
	return vec2(wx, wy);
}

float sphericalTriangleArea(const vec3& a, const vec3& b, const vec3& c, float R) {
	auto getTangent = [&](const vec3& p, const vec3& q) -> vec3 {
		return normalize(q - dot(q, p) * p);
	};
	auto angleAt = [&](const vec3& p, const vec3& q, const vec3& r) -> float {
		vec3 tq = getTangent(p, q);
		vec3 tr = getTangent(p, r);
		float ca = dot(tq, tr);
		float ca_clamped = ca < -1.0f ? -1.0f : (ca > 1.0f ? 1.0f : ca);
		float ang = acosf(ca_clamped);
		vec3 cr = cross(tq, tr);
		if (dot(cr, p) < 0) ang = 2 * M_PI - ang;
		return ang;
	};
	float A = angleAt(a, b, c);
	float B = angleAt(b, a, c);
	float C = angleAt(c, a, b);
	float excess = A + B + C - M_PI;
	if (excess < 0) excess = -excess;
	return excess * R * R;
}

bool pointInSphericalTriangle(const vec3& s, const vec3& p, const vec3& q, const vec3& r) {
	auto sameSide = [&](const vec3& v1, const vec3& v2, const vec3& third, const vec3& test) -> bool {
		vec3 n = cross(v1, v2);
		return dot(third, n) * dot(test, n) >= 0;
	};
	return sameSide(p, q, r, s) && sameSide(q, r, p, s) && sameSide(r, p, q, s);
}

class App : public glApp {
	Map map;
	Geometry<vec2>* stationGeom;
	std::vector<Geometry<vec2>*> pathGeoms;
	std::vector<vec3> stationSpherePositions;
	float mercMin, mercMax, earthR;

	void rebuildPaths() {
		for (auto p : pathGeoms) delete p;
		pathGeoms.clear();
		size_t n = stationSpherePositions.size();
		if (n < 2) return;
		for (size_t i = 0; i < n; ++i) {
			vec3 s1 = stationSpherePositions[i];
			vec3 s2 = stationSpherePositions[(i + 1) % n];
			Geometry<vec2>* pg = new Geometry<vec2>();
			float d = dot(s1, s2);
			float d_clamped = d < -1.0f ? -1.0f : (d > 1.0f ? 1.0f : d);
			if (fabsf(d) >= 0.99999f) {
				pg->Vtx().push_back(sphereToMap(s1, mercMin, mercMax));
			} else {
				float theta = acosf(d_clamped);
				vec3 perp = normalize(s2 - d * s1);
				for (int k = 0; k <= 100; ++k) {
					float alpha = (k / 100.0f) * theta;
					vec3 pp = cosf(alpha) * s1 + sinf(alpha) * perp;
					pg->Vtx().push_back(sphereToMap(pp, mercMin, mercMax));
				}
			}
			pg->updateGPU();
			pathGeoms.push_back(pg);
		}
	}

	void updateDarkenedTexture() {
		size_t n = stationSpherePositions.size();
		map.current = map.original;
		if (n < 3) {
			map.texture->updateTexture(64, 64, map.current);
			return;
		}
		vec3 fanCenter = stationSpherePositions[0];
		float totalArea = 0.0f;
		for (size_t i = 1; i < n - 1; ++i) {
			vec3 p = fanCenter;
			vec3 q = stationSpherePositions[i];
			vec3 r = stationSpherePositions[i + 1];
			totalArea += sphericalTriangleArea(p, q, r, earthR);
			for (int ty = 0; ty < 64; ++ty) {
				for (int tx = 0; tx < 64; ++tx) {
					float wx = 2.0f * (tx + 0.5f) / 64.0f - 1.0f;
					float wy = 2.0f * (ty + 0.5f) / 64.0f - 1.0f;
					vec3 sp = mapToSphere(wx, wy, mercMin, mercMax);
					if (pointInSphericalTriangle(sp, p, q, r)) {
						size_t idx = ty * 64 + tx;
						map.current[idx] = map.current[idx] * 0.5f;
					}
				}
			}
		}
		map.texture->updateTexture(64, 64, map.current);
		printf("Area: %.1f million km^2\n", totalArea / 1000000.0f);
	}

public:
	App() : glApp("Mercator terület számító"), stationGeom(nullptr) {}

	void onInitialization() {
		glViewport(0, 0, windowWidth, windowHeight);
		glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
		glLineWidth(3.0f);
		glPointSize(10.0f);
		gpuProgram = new GPUProgram();
		gpuProgram->create(vertexSource, fragmentSource);

		map.init();
		map.decodeRLE();
		map.updateTexture();

		stationGeom = new Geometry<vec2>();
		mercMin = merc(-85.0f);
		mercMax = merc(85.0f);
		earthR = 40000.0f / (2.0f * M_PI);
	}

	void onDisplay() {
		glClear(GL_COLOR_BUFFER_BIT);
		map.draw();
		gpuProgram->setUniform(identityMatrix, "MVP");
		gpuProgram->setUniform(0, "useTexture");
		for (auto pg : pathGeoms) {
			pg->Draw(gpuProgram, GL_LINE_STRIP, vec3(1, 1, 0));
		}
		if (stationGeom && !stationGeom->Vtx().empty()) {
			stationGeom->Draw(gpuProgram, GL_POINTS, vec3(1, 0, 0));
		}
	}

	void onMousePressed(MouseButton but, int pX, int pY) {
		if (but != MOUSE_LEFT) return;
		float wx = 2.0f * pX / windowWidth - 1.0f;
		float wy = 1.0f - 2.0f * pY / windowHeight;
		vec3 sphereP = mapToSphere(wx, wy, mercMin, mercMax);
		stationSpherePositions.push_back(sphereP);
		stationGeom->Vtx().push_back(vec2(wx, wy));
		stationGeom->updateGPU();
		rebuildPaths();
		updateDarkenedTexture();
		refreshScreen();
	}
} app;