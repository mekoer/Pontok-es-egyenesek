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
	DRAW_POINTS = 1,
	DRAW_LINES = 2,
	POINT_CHOSEN = 3
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
		cout << "pont felveve" << endl;
		points.getVtxArray().push_back(point);
		points.updateGPU();
	}
	const vec2* pointNearby(vec2 click) {
		for (const vec2& vertex : points.getVtxArray()) {
			if (vertex.x >= click.x - 0.01f && vertex.x <= click.x + 0.01f
				&& vertex.y >= click.y - 0.01f && vertex.y <= click.y + 0.01f) {
				cout << "kijelolve: " << vertex.x  << " " << vertex.y << endl;
				return &vertex;
			}
		}
		return NULL;
	}
	void drawPoints() {
		points.Draw(GL_POINTS, vec3(1.0f, 0.0f, 0.0f));
	}
};

class Line {
	vec2 cP0;
	vec2 cParallelVector;
	vec2 cNormalVector;
public:
	Line(vec2 p1, vec2 p2) {
		cParallelVector = p2 - p1;
		cNormalVector = vec2(-1 * cParallelVector.y, cParallelVector.x);
		cP0 = p1;
	}

	vec2 intersectPoint(Line otherLine) {

	}

	bool pointOnLine(vec2 point) {
		float c = (-1) * (cNormalVector.x * cP0.x + cNormalVector.y * cP0.y);
		if (cNormalVector.x * point.x + cNormalVector.y * point.y + c == 0) {
			return true;
		}
		return false;
	}

	void inViewPort() {

	}

	void move(vec2 point) {

	}
};

class LineCollection {
	Object endPoints;
	vector<Line> lines;
public:
	LineCollection() {
		endPoints = Object();
	}

	// hozzaad egyetlen pontot
	void addVertex() {

	}

	// felvesz egy uj vonalat
	void addLine(Line line) {
		lines.push_back(line);
		endPoints.updateGPU();
	}

	void drawLines() {
		endPoints.Draw(GL_LINES, vec3(0.0f, 1.0f, 1.0f));
	}
};

PointCollection* pontok;
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


	pontok->drawPoints();
	vonalak->drawLines();

	glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	switch (key) {
	case 'p':
		//cout << state << endl;
		windowState = DRAW_POINTS;
		break;
	case 'l':
		//cout << state << endl;
		windowState = DRAW_LINES;
		break;
	default:
		//cout << state << endl;
		windowState = IDLE;
		break;
	}
	//if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	//float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	//float cY = 1.0f - 2.0f * pY / windowHeight;
	//printf("Mouse moved to (%3.2f, %3.2f)\n", cX, cY);
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;

	if (state == GLUT_DOWN) {
		switch (windowState)
		{
		case IDLE:
			cout << windowState << endl;
			break;
		case DRAW_POINTS:
			switch (button) {
			case GLUT_LEFT_BUTTON:
				cout << windowState << endl;
				printf("Left button at (%3.2f, %3.2f)\n", cX, cY);
				pontok->addPoint(vec2(cX, cY));
				//glutPostRedisplay();
				break;
				//case GLUT_MIDDLE_BUTTON: printf("Middle button at (%3.2f, %3.2f)\n", cX, cY); break;
				//case GLUT_RIGHT_BUTTON:  printf("Right button at (%3.2f, %3.2f)\n", cX, cY);  break;
			}
			break;
		case DRAW_LINES:
			switch (button) {
			case GLUT_LEFT_BUTTON:
				cout << windowState << endl;
				printf("Left button at (%3.2f, %3.2f)\n", cX, cY);
				//line hozzaadasa ide
				vec2 cP(cX, cY);
				const vec2* clickedPoint = pontok->pointNearby(cP);
				if (clickedPoint != nullptr) {
					vec2 temp(clickedPoint->x, clickedPoint->y);
					endpoint1 = temp;
					cout << endpoint1.x << " " << endpoint1.y << endl;
					windowState = POINT_CHOSEN;
				}
				break;
			}
			break;
			//case GLUT_MIDDLE_BUTTON: printf("Middle button at (%3.2f, %3.2f)\n", cX, cY); break;
			//case GLUT_RIGHT_BUTTON:  printf("Right button at (%3.2f, %3.2f)\n", cX, cY);  break;
			//}
		case POINT_CHOSEN:
			switch (button) {
			case GLUT_LEFT_BUTTON:
				cout << windowState << endl;
				printf("Left button at (%3.2f, %3.2f)\n", cX, cY);
				//line hozzaadasa ide
				vec2 cP(cX, cY);
				const vec2* clickedPoint = pontok->pointNearby(cP);
				if (clickedPoint != NULL) {
					vec2 temp(clickedPoint->x, clickedPoint->y);
					endpoint2 = temp;
					//cout << temp.x << " " << temp.y << " " << endpoints[1].x << " " << endpoints[1].y << endl;
					cout << endpoint1.x << " " << endpoint1.y << " " << endpoint2.x << " " << endpoint2.y << endl;
					windowState = DRAW_LINES;
					
					vonalak->addLine(Line(endpoint1, endpoint2), endpoint1, endpoint2);
					
					glutPostRedisplay();
				}

				//glutPostRedisplay();

				break;
			}
			break;
		}
	}
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
}


