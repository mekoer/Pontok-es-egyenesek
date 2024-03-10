//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Mayer Ádám
// Neptun : XYJP9S
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

class Object;
class PointCollection;

using namespace std;

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char* const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 1);		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char* const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color.x, color.y, color.z, 1);	// computed color is the color of the primitive
	}
)";

GPUProgram gpuProgram;

enum WindowState {
	IDLE = 0,
	DRAW_POINTS,
	DRAW_LINES,
	POINT_SELECTED,
	MOVE_LINE,
	LINE_INTERSECTION
};

int windowState = WindowState::IDLE;

class Object {
	unsigned int vao, vbo;
	vector<vec2> vtx;
public:
	Object() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	}

	vector<vec2>& getVtxArray() {
		return vtx;
	}

	void updateGPU() {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec2), &vtx[0], GL_DYNAMIC_DRAW);
	}

	void Draw(int type, vec3 color) {
		if (vtx.size() > 0) {
			glBindVertexArray(vao);
			gpuProgram.setUniform(color, "color");
			glDrawArrays(type, 0, vtx.size());
		}
	}
};

class PointCollection {
	Object points;
public:
	PointCollection() {
		points = Object();
	}
	void addPoint(vec2 point) {
		points.getVtxArray().push_back(point);
		printf("Point added at (%3.2f, %3.2f)\n", point.x, point.y);
		points.updateGPU();
	}
	const vec2* pointNearby(vec2 click) {
		for (const vec2& vertex : points.getVtxArray()) {
			if (fabs(vertex.x - click.x) <= 0.01f && fabs(vertex.y - click.y) <= 0.01f) {
				return &vertex;
			}
		}
		return nullptr;
	}
	void draw() {
		points.Draw(GL_POINTS, vec3(1, 0, 0));
	}
};

PointCollection* pontok;

class Line {
	vec2 cP0;
	vec2 cParallelVector;
	vec2 cNormalVector;
	vector<vec2> lineEnds;

protected:
	// implicit egyenlet parameterek
	float A, B, C;
public:
	Line(vec2 p1, vec2 p2) {
		cP0 = p1;
		cParallelVector = p2 - p1;
		cNormalVector = vec2(-1 * cParallelVector.y, cParallelVector.x);
		A = cNormalVector.x;
		B = cNormalVector.y;
		C = -1 * cNormalVector.x * p1.x + -1 * cNormalVector.y * p1.y;
	}

	void print() {
		printf("Line added\n	(%3.2f)x + (%3.2f)y + (%3.2f) = 0\n", A, B, C);
		printf("	r(t) = (%3.2f, %3.2f) + (%3.2f, %3.2f)t\n",
			cP0.x, cP0.y, cParallelVector.x, cParallelVector.y);
	}

	vec2 intersectPoint(Line otherLine) {
		vec2 intersectPoint;
		intersectPoint.x = (otherLine.B * C - B * otherLine.C) / (otherLine.A * B - A * otherLine.B);
		intersectPoint.y = (otherLine.A * C - A * otherLine.C) / (A * otherLine.B - otherLine.A * B);

		return intersectPoint;
	}

	int pointOnLine(vec2 point) {
		float pointDist = fabs(A * point.x + B * point.y + C) / sqrtf(A*A + B*B);
		if (fabs(pointDist) <= 0.01f) {
			return 1;
		}
		return 0;
	}

	vector<vec2> getEnds() {
		return lineEnds;
	}

	void inViewPort() {
		vector<vec2> vertices;
		int numberOfIntersects = 0;

		// top:
		Line top(vec2(1.0f, 1.0f), vec2(-1.0f, 1.0f));
		vec2 intersect = intersectPoint(top);

		if (fabs(intersect.x) <= 1) {
			numberOfIntersects++;
			vertices.push_back(intersect);
		}

		// bottom:
		Line bottom(vec2(-1.0f, -1.0f), vec2(1.0f, -1.0f));
		intersect = intersectPoint(bottom);
		if (fabs(intersect.x) <= 1) {
			numberOfIntersects++;
			vertices.push_back(intersect);
		}

		// right:
		Line right(vec2(1.0f, 1.0f), vec2(1.0f, -1.0f));
		intersect = intersectPoint(right);
		if (numberOfIntersects <= 2) {
			if (fabs(intersect.y) < 1) {
				numberOfIntersects++;
				vertices.push_back(intersect);
			}
		}

		// left:
		Line left(vec2(-1.0f, 1.0f), vec2(-1.0f, -1.0f));
		intersect = intersectPoint(left);
		if (numberOfIntersects <= 2) {
			if (fabs(intersect.y) < 1) {
				numberOfIntersects++;
				vertices.push_back(intersect);
			}
		}

		lineEnds = vertices;
	}

	void move(vec2 point) {
		cP0 = point;
		C = -1 * cNormalVector.x * cP0.x + -1 * cNormalVector.y * cP0.y;
	}
};

class LineCollection {
	Object endPoints;
	vector<vec2> constructionPoints;
	vector<Line> lines;
	vector<Line*> selectedLines;
public:
	LineCollection() {
		endPoints = Object();
	}

	void updateEndPoints() {
		endPoints.getVtxArray().clear();

		for (Line& lineIt : lines) {
			vector<vec2> ends = lineIt.getEnds();
			endPoints.getVtxArray().push_back(ends.back());
			ends.pop_back();
			endPoints.getVtxArray().push_back(ends.back());
		}
	}

	void addVertex(vec2 vtx) {
		if (constructionPoints.size() < 2) {
			constructionPoints.push_back(vtx);
		}
		if (constructionPoints.size() == 2) {
			vec2 p1 = constructionPoints.back();
			constructionPoints.pop_back();
			vec2 p2 = constructionPoints.back();
			constructionPoints.pop_back();

			Line newLine(p1, p2);
			newLine.print();

			newLine.inViewPort();
			lines.push_back(newLine);
			updateEndPoints();

			endPoints.updateGPU();

			constructionPoints.clear();
		}
	}

	void selectLineForIntersect(vec2 p) {
		for (Line& lineIt : lines) {
			if (lineIt.pointOnLine(p) == 1) {
				if (selectedLines.size() < 2) {
					selectedLines.push_back(&lineIt);
				}
				if (selectedLines.size() == 2) {
					addPointAtIntersection();
				}
			}
		}
	}

	void selectLineForMove(vec2 point) {
		for (Line& lineIt : lines) {
			if (lineIt.pointOnLine(point) == 1) {
				selectedLines.push_back(&lineIt);
				break;
			}
		}
	}

	void move(vec2 point) {
		selectedLines.back()->move(point);
		selectedLines.back()->inViewPort();
		updateEndPoints();
		endPoints.updateGPU();
	}

	void emptySelectedArray() {
		selectedLines.clear();
	}
	vector<Line*> getSelected() {
		return selectedLines;
	}

	void addPointAtIntersection() {
		Line* tempLine = selectedLines.back();
		selectedLines.pop_back();
		vec2 intersectPoint = tempLine->intersectPoint(*selectedLines.back());
		selectedLines.pop_back();
		pontok->addPoint(intersectPoint);
	}

	void draw() {
		endPoints.Draw(GL_LINES, vec3(0, 1, 1));
	}
};

LineCollection* vonalak;

void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	glPointSize(10);
	glLineWidth(3);

	pontok = new PointCollection;
	vonalak = new LineCollection;

	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

void onDisplay() {
	glClearColor(0.2f, 0.2f, 0.2f, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	vonalak->draw();
	pontok->draw();

	glutSwapBuffers();
}

void onKeyboard(unsigned char key, int pX, int pY) {
	switch (key) {
	case 'p':
		printf("Place points\n");
		windowState = DRAW_POINTS;
		break;
	case 'l':
		printf("Define lines\n");
		windowState = DRAW_LINES;
		break;
	case 'm':
		printf("Move\n");
		vonalak->emptySelectedArray();
		windowState = MOVE_LINE;
		break;
	case 'i':
		printf("Intersect\n");
		vonalak->emptySelectedArray();
		windowState = LINE_INTERSECTION;
		break;
	default:
		windowState = IDLE;
		break;
	}
}

void onKeyboardUp(unsigned char key, int pX, int pY) {}

void onMouseMotion(int pX, int pY) {
	float cX = 2.0f * pX / windowWidth - 1;
	float cY = 1.0f - 2.0f * pY / windowHeight;

	if (windowState == MOVE_LINE) {
		if (!(vonalak->getSelected().empty())) {
			if (fabs(cX) <= 1 && fabs(cY) <= 1) {
				vonalak->move(vec2(cX, cY));
				glutPostRedisplay();
			}
		}
	}
}

void onMouse(int button, int state, int pX, int pY) {
	float cX = 2.0f * pX / windowWidth - 1;
	float cY = 1.0f - 2.0f * pY / windowHeight;

	if (state == GLUT_DOWN) {
		switch (windowState) {
		case IDLE:
			break;
		case DRAW_POINTS:
			if (button == GLUT_LEFT_BUTTON) {
				pontok->addPoint(vec2(cX, cY));
			}
			break;
		case DRAW_LINES:
			if (button == GLUT_LEFT_BUTTON) {
				vec2 cP(cX, cY);
				const vec2* clickedPoint = pontok->pointNearby(cP);
				if (clickedPoint != nullptr) {
					vonalak->addVertex(*clickedPoint);
				}
			}
			break;
		case MOVE_LINE:
			if (button == GLUT_LEFT_BUTTON) {
				vonalak->emptySelectedArray();
				vonalak->selectLineForMove(vec2(cX, cY));
			}
			break;
		case LINE_INTERSECTION:
			vonalak->selectLineForIntersect(vec2(cX, cY));
			break;
		}
	}
	if (state == GLUT_UP)
		glutPostRedisplay();
}

void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME);
}