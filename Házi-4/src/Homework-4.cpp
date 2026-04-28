//=============================================================================================
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni, ideertve ChatGPT-t es tarsait is
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
//=============================================================================================
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

mat4 identityMatrix = mat4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);

inline float mdot(const vec4& a, const vec4& b) { return a.x*b.x + a.y*b.y + a.z*b.z - a.w*b.w; }

struct Material {
	vec3 ka, kd1, ks;
	float shininess;
};

struct Sphere {
	vec4 center;
	float radius;
	Material mat;
};

class App : public glApp {
	GPUProgram* gpuProgram = nullptr;
	Geometry<vec2>* quadGeom = nullptr;
	Texture* renderedTexture = nullptr;
	std::vector<vec3> image;
	std::vector<Sphere> spheres;

	void initSpheres() {
		vec4 eye(0,0,0,1);
		auto makeCenter = [&](vec4 d, float dist) -> vec4 {
			float len = sqrtf(d.x*d.x + d.y*d.y + d.z*d.z);
			if (len > 1e-6f) d = d * (1.0f / len);
			return eye * coshf(dist) + d * sinhf(dist);
		};
		Material m; m.ka = vec3(0.1f,0.1f,0.1f); m.ks = vec3(4,4,4); m.shininess = 50.0f;
		m.kd1 = vec3(1,0,0); spheres.push_back({makeCenter(vec4(-0.4f,0.4f,-1,0), 0.2f), 0.1f, m});
		m.kd1 = vec3(0,1,0); spheres.push_back({makeCenter(vec4( 0,   0,-1,0), 0.5f), 0.3f, m});
		m.kd1 = vec3(0,0,1); spheres.push_back({makeCenter(vec4( 0.4f,-0.4f,-1,0), 0.9f), 0.5f, m});
	}

	void renderScene() {
		const int W = 600, H = 600;
		image.resize(W * H);
		vec4 eye(0,0,0,1);
		vec3 Ia(0.2f,0.2f,0.2f);
		vec3 Il(2,2,2);

		for (int iy = 0; iy < H; ++iy) {
			for (int ix = 0; ix < W; ++ix) {
				float X = (float)ix, Y = (float)iy;
				vec4 look(0,0,-1,0), rgt(1,0,0,0), upp(0,1,0,0);
				vec4 ddir = look + rgt * ((X + 0.5f)/300.0f - 1.0f) + upp * ((Y + 0.5f)/300.0f - 1.0f);
				float dlen = sqrtf(ddir.x*ddir.x + ddir.y*ddir.y + ddir.z*ddir.z);
				vec4 u = (dlen > 1e-6f) ? ddir * (1.0f/dlen) : vec4(0,0,-1,0);

				float minT = 1e20f;
				int hitIdx = -1;
				for (size_t s = 0; s < spheres.size(); ++s) {
					const Sphere& sph = spheres[s];
					float A = mdot(sph.center, eye);
					float B = mdot(sph.center, u);
					float Cr = -coshf(sph.radius);
					float alpha = A + B;
					float beta  = -2.0f * Cr;
					float gamma = A - B;
					float disc = beta*beta - 4.0f*alpha*gamma;
					if (disc >= 0.0f && fabsf(alpha) > 1e-9f) {
						float sd = sqrtf(disc);
						float z1 = (-beta + sd) / (2.0f * alpha);
						float z2 = (-beta - sd) / (2.0f * alpha);
						if (z1 > 0.0f) {
							float t1 = logf(z1);
							if (t1 > 0.0f && t1 < minT) { minT = t1; hitIdx = (int)s; }
						}
						if (z2 > 0.0f) {
							float t2 = logf(z2);
							if (t2 > 0.0f && t2 < minT) { minT = t2; hitIdx = (int)s; }
						}
					}
				}

				vec3 col(0,0,0);
				if (hitIdx >= 0) {
					const Sphere& sph = spheres[hitIdx];
					float t = minT;
					vec4 x = eye * coshf(t) + u * sinhf(t);
					float dLight = acoshf(-mdot(x, eye));
					int D = (int)(dLight * 100.0f);
					vec3 kd = (D % 2 == 0) ? sph.mat.kd1 : vec3(1,1,1) - sph.mat.kd1;

					float coshr = coshf(sph.radius), sinhr = sinhf(sph.radius);
					vec4 n = (sph.center - x * coshr) / sinhr;

					float md = mdot(x, eye);
					vec4 l = (eye + md * x) / sinhf(dLight);

					float NdotL = fabsf(mdot(n, l));
					vec3 ambient = sph.mat.ka * Ia;
					vec3 diffuse = kd * Il * NdotL;
					vec3 specular = sph.mat.ks * Il * powf(NdotL, sph.mat.shininess);
					col = ambient + diffuse + specular;
				}
				image[iy * W + ix] = col;
			}
		}
		renderedTexture->updateTexture(600, 600, image);
	}

public:
	App() : glApp("Sugárkövetés hiperbolikus geometriában") {}

	void onInitialization() {
		glViewport(0, 0, windowWidth, windowHeight);
		glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
		gpuProgram = new GPUProgram();
		gpuProgram->create(vertexSource, fragmentSource);

		quadGeom = new Geometry<vec2>();
		quadGeom->Vtx().push_back(vec2(-1,-1));
		quadGeom->Vtx().push_back(vec2(1,-1));
		quadGeom->Vtx().push_back(vec2(-1,1));
		quadGeom->Vtx().push_back(vec2(1,1));
		quadGeom->updateGPU();

		renderedTexture = new Texture(600, 600);
		initSpheres();
		renderScene();
	}

	void onDisplay() {
		glClear(GL_COLOR_BUFFER_BIT);
		gpuProgram->setUniform(identityMatrix, "MVP");
		gpuProgram->setUniform(1, "useTexture");
		renderedTexture->Bind(0);
		quadGeom->Draw(gpuProgram, GL_TRIANGLE_STRIP, vec3(1,1,1));
	}
} app;