#ifndef HALF_EDGE_STRUCTURE_H
#define HALF_EDGE_STRUCTURE_H

#include <iostream>
#include <vector>

using namespace std;

struct Vertex;
struct HalfEdge;
struct Edge;
struct Loop;
struct Face;
struct Solid;

struct Point {
	double x;
	double y;
	double z;

	Point(){}
	Point(double _x, double _y, double _z) { this->x = _x; this->y = _y; this->z = _z; }

	Point operator*(double d) {
		Point newp;
		newp.x = this->x * d;
		newp.y = this->y * d;
		newp.z = this->z * d;
		return newp;
	}

	Point operator+(const Point& p) {
		Point newp;
		newp.x = this->x + p.x;
		newp.y = this->y + p.y;
		newp.z = this->z + p.z;
		return newp;
	}
};

struct Vertex {
	Vertex* prevv;
	Vertex* nextv;

	Point p;

	Vertex() :prevv(NULL), nextv(NULL) {}
	Vertex(Point _p) { this->p = _p; this->prevv = this->nextv = NULL; }

};

struct HalfEdge {

	HalfEdge* prev;
	HalfEdge* next;
	HalfEdge* adjacent;

	Vertex* startv;
	Vertex* endv;

	Loop* wloop;
	Edge* edge;

	HalfEdge():prev(NULL), next(NULL), adjacent(NULL), startv(NULL),endv(NULL),wloop(NULL),edge(NULL){}
};

struct Edge {
	Edge* preve;
	Edge* nexte;

	HalfEdge* he1; // the edge's left  half edge
	HalfEdge* he2; // the edge's right half edge

	Edge():preve(NULL), nexte(NULL), he1(NULL), he2(NULL) {}
};

struct Loop {
	Loop* prevl;
	Loop* nextl;

	Face* lface;
	HalfEdge* ledge;

	Loop():prevl(NULL), nextl(NULL), lface(NULL), ledge(NULL) {}

	int size() {
		if (ledge == NULL) return 0;
		HalfEdge* starthe, * he;
		starthe = he = ledge;
		int num = 0;
		do {
			he = he->next;
			num++;
		} while (he != starthe);
		return num;
	}
};

struct Face {
	Face* prevf;
	Face* nextf;

	vector<Loop*> floops;// 1 out loop and >=0 inner loop

	Solid* fsolid;

	Face():prevf(NULL), nextf(NULL), floops(NULL),fsolid(NULL){}
};

struct Solid {

	vector<Face*> sfaces;

	Solid():sfaces(NULL){}

	void printSolid() {
		cout << "Solid s:" << endl;
		for (int i = 0; i < sfaces.size(); i++) {
			cout << "  Face " << i << " :" << endl;
			for (int j = 0; j < sfaces[i]->floops.size(); j++) {
				cout << "    Loop " << j << " :" << endl;
				HalfEdge* starte = sfaces[i]->floops[j]->ledge;
				HalfEdge* e = starte->next;
				Vertex* startv = starte->startv;
				cout << "    point:(" << startv->p.x << "," << startv->p.y << "," << startv->p.z << ")" << endl;
				while (e!= starte) {
					cout << "    point:(" << e->startv->p.x << "," << e->startv->p.y << "," << e->startv->p.z << ")" << endl;
					e = e->next;
				}
				cout << endl;
			}
			cout << endl;
		}
	}
};



#endif