//=============================================================================================
// Mintaprogram: Z�ld h�romsz�g. Ervenyes 2019. osztol.
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
// Nev    : 
// Neptun : 
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
#include "iostream"

class Object;
class PointCollection;

using namespace std;

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char* const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char* const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";

GPUProgram gpuProgram; // vertex and fragment shaders

enum WindowState {
	IDLE = 0,
	DRAW_POINTS,
	DRAW_LINES,
	POINT_SELECTED,
	MOVE_LINE,
	MOVING_LINE,
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
		// two floats/attrib, not fixed-point, stride, offset: tightly packed
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
		//cout << "pont felveve" << endl;
		points.getVtxArray().push_back(point);
		points.updateGPU();
	}
	const vec2* pointNearby(vec2 click) {
		for (const vec2& vertex : points.getVtxArray()) {
			if (vertex.x >= click.x - 0.015f && vertex.x <= click.x + 0.015f
				&& vertex.y >= click.y - 0.015f && vertex.y <= click.y + 0.015f) {
				//cout << "kijelolve: " << vertex.x  << " " << vertex.y << endl;
				return &vertex;
			}
		}
		return NULL;
	}
	void draw() {
		points.Draw(GL_POINTS, vec3(1.0f, 0.0f, 0.0f));
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
		//cout << "iranyvektor: " << cParallelVector.x << " " << cParallelVector.y << endl;
		cNormalVector = vec2(-1 * cParallelVector.y, cParallelVector.x);
		//cout << "normalvektor: " << cNormalVector.x << " " << cNormalVector.y << " ";
		A = cNormalVector.x;
		B = cNormalVector.y;
		C = -1 * cNormalVector.x * p1.x + -1 * cNormalVector.y * p1.y;
		//cout << C << endl;
	}

	vec2* intersectPoint(Line otherLine) {
		// a1*b2 - a2*b1 // oszt�
		float denominator = A * otherLine.B - otherLine.A * B;
		if (denominator == 0) return nullptr;

		vec2 intersectPoint;
		intersectPoint.x = (otherLine.B * C - B * otherLine.C) / (otherLine.A * B - A * otherLine.B);
		intersectPoint.y = (otherLine.A * C - A * otherLine.C) / (A * otherLine.B - otherLine.A * B);
		
		return &intersectPoint;
	}

	bool pointOnLine(vec2 point) {
		float pointEq = A * point.x + B * point.y + C;
		if (pointEq <= 0.01f && pointEq >= -0.01f) {
			return true;
		}
		return false;
	}

	vector<vec2> getEnds() {
		return lineEnds;
	}

	// viewport negyzet oldalaival valo talalkozas pontjait adja vissza
	void inViewPort() {
		vector<vec2> vertices;
		int numberOfIntersects = 0;

		// k�perny� teteje:
		Line top(vec2(1.0f, 1.0f), vec2(-1.0f, 1.0f));

		vec2* intersect = intersectPoint(top);
		if (intersect != nullptr) {
			if (intersect->x >= -1 && intersect->x <= 1) {
				numberOfIntersects++;
				vertices.push_back(*intersect);
			}
		}

		// k�perny� alja:
		Line bottom(vec2(-1.0f, -1.0f), vec2(1.0f, -1.0f));

		intersect = intersectPoint(bottom);
		if (intersect != nullptr) {
			if (intersect->x >= -1 && intersect->x <= 1) {
				numberOfIntersects++;
				vertices.push_back(*intersect);
			}
		}
		
		// k�perny� jobb oldala:
		Line right(vec2(1.0f, 1.0f), vec2(1.0f, -1.0f));

		intersect = intersectPoint(right);
		if (intersect != nullptr && numberOfIntersects <= 2) {
			if (intersect->y >= -1 && intersect->y <= 1) {
				numberOfIntersects++;
				vertices.push_back(*intersect);
			}
		}

		// k�perny� bal oldala:
		Line left(vec2(-1.0f, 1.0f), vec2(-1.0f, -1.0f));

		intersect = intersectPoint(left);
		if (intersect != nullptr && numberOfIntersects <= 2) {
			if (intersect->y >= -1 && intersect->y <= 1) {
				numberOfIntersects++;
				vertices.push_back(*intersect);
			}
		}
		
		//return vertices;
		lineEnds = vertices;
	}

	void move(vec2 point) {
		cP0 = point;
		cout << "p0: " << cP0.x << " " << cP0.y << endl;
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
			// k�t utols� elemet kiveszi a constructionpointok k�z�l
			vec2 p1 = constructionPoints.back();
			constructionPoints.pop_back();
			vec2 p2 = constructionPoints.back();
			constructionPoints.pop_back();

			// ezekb�l l�trehozza a vonalat
			Line newLine(p1, p2);
			
			newLine.inViewPort();
			lines.push_back(newLine);
			updateEndPoints();

			endPoints.updateGPU();

			constructionPoints.clear();
		}
	}

	void selectLineForIntersect(vec2 p) {
		for (Line& lineIt : lines) {
			if (lineIt.pointOnLine(p)) {
				if (selectedLines.size() < 2) {
					selectedLines.push_back(&lineIt);
					cout << "vonal kivalasztva kereszetezhez" << endl;
				}
				if (selectedLines.size() == 2) {
					addPointAtIntersection();
				}
			}
		}
	}

	void selectLineForMove(vec2 p) {
		for (Line& lineIt : lines) {
			if (lineIt.pointOnLine(p)) {
				//selectedLines.push_back(&lineIt);
				lineIt.move(p);
				lineIt.inViewPort();
				updateEndPoints();
				endPoints.updateGPU();

				break;
			}
		}
	}

	void move(vec2 p1) {
		selectedLines.back()->move(p1);
		updateEndPoints();
		endPoints.updateGPU();
	}

	void emptySelectedArray() {
		selectedLines.clear();
	}

	void addPointAtIntersection() {
		Line* tempLine = selectedLines.back();
		selectedLines.pop_back();
		vec2* intersectPoint = tempLine->intersectPoint(*selectedLines.back());
		selectedLines.pop_back();
		pontok->addPoint(*intersectPoint);
	}

	void draw() {
		//cout << "drawlines" << endl;
		endPoints.Draw(GL_LINES, vec3(0.0f, 1.0f, 1.0f));
	}
};

LineCollection* vonalak;

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	glPointSize(10);
	glLineWidth(3);

	pontok = new PointCollection;
	vonalak = new LineCollection;
	//pontok->addPoint(vec2(0.0f, 0.0f));
	//pontok->addPoint(vec2(0.0f, 0.5f));
	//pontok->addPoint(vec2(0.5f, 0.0f));
	//pontok->addPoint(vec2(0.5f, 0.5f));
	
	/*pontok->addPoint(vec2(0.3f, 0.0f));
	pontok->addPoint(vec2(0.2f, 0.2f));
	
	vonalak->addVertex(vec2(0.2f, 0.2f));
	vonalak->addVertex(vec2(0.3f, 0.0f));*/

	// create program for the GPU
	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

	// Set color to (0, 1, 0) = green
	//int location = glGetUniformLocation(gpuProgram.getId(), "color");
	//glUniform3f(location, 0.0f, 1.0f, 0.0f); // 3 floats

	float MVPtransf[4][4] = { 1, 0, 0, 0,    // MVP matrix, 
							  0, 1, 0, 0,    // row-major!
							  0, 0, 1, 0,
							  0, 0, 0, 1 };

	int location = glGetUniformLocation(gpuProgram.getId(), "MVP");	// Get the GPU location of uniform variable MVP
	glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);	// Load a 4x4 row-major float matrix to the specified location
	
	

	pontok->draw();
	vonalak->draw();

	glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	switch (key) {
	case 'p':
		windowState = DRAW_POINTS;
		break;
	case 'l':
		windowState = DRAW_LINES;
		break;
	case 'm':
		vonalak->emptySelectedArray();
		windowState = MOVE_LINE;
		break;
	case 'i':
		vonalak->emptySelectedArray();
		windowState = LINE_INTERSECTION;
		break;
	default:
		windowState = IDLE;
		break;
	}
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;
	//printf("Mouse moved to (%3.2f, %3.2f)\n", cX, cY);

	if (windowState == MOVE_LINE) {
		vonalak->selectLineForMove(vec2(cX, cY));
		cout << "at: " << cX << " " << cY << endl;
	}
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;

	if (state == GLUT_DOWN) {
		switch (windowState) {
		case IDLE:
			printf("Left button at (%3.2f, %3.2f)\n", cX, cY);
			break;
		case DRAW_POINTS:
			if (button == GLUT_LEFT_BUTTON) {
				printf("Left button at (%3.2f, %3.2f)\n", cX, cY);
				pontok->addPoint(vec2(cX, cY));
			}
			break;
		case DRAW_LINES:
			if (button == GLUT_LEFT_BUTTON) {
				printf("Left button at (%3.2f, %3.2f)\n", cX, cY);
				vec2 cP(cX, cY);
				const vec2* clickedPoint = pontok->pointNearby(cP);
				if (clickedPoint != nullptr) {
					vonalak->addVertex(*clickedPoint);
				}
			}
			break;
		/*case MOVE_LINE:
			vonalak->selectLineForMove(vec2(cX, cY));
			break;*/
		case LINE_INTERSECTION:
			vonalak->selectLineForIntersect(vec2(cX, cY));
			break;
		}
	}
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
}


