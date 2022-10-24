#include "euler_operation.h"

// 生成一个含有一个点v的面，并且构成一个新的体。从无到有构造一个点、一个面、一个体
Solid* EulerOperation::mvsf(Point p, Vertex*& v)
{
	Face* f = new Face;
	Solid* s = new Solid;
	Loop* lp = new Loop;
	
	v = new Vertex(p);
	v->nextv = v->nextv = v;
	s->sfaces.push_back(f);
	f->fsolid = s;
	f->floops.push_back(lp);
	f->nextf = f->prevf = f;
	lp->lface = f;

	return s;
}

// 生成一个新点v2，同时生成一条连接新点v2与一给定点v1的新边e
HalfEdge* EulerOperation::mev(Vertex* v1, Point p, Loop* lp)
{
	// make edge
	HalfEdge* he1 = new HalfEdge;
	HalfEdge* he2 = new HalfEdge;
	HalfEdge* he = new HalfEdge;

	Edge* eg = new Edge; // 在面上沿环的方向前进，左侧总在面内，右侧总在面外

	Vertex* v2 = new Vertex(p);
	he1->startv = v1;
	he1->endv = v2;
	he2->startv = v2;
	he2->endv = v1;
	he1->wloop = he2->wloop = lp;
	he1->edge = he2->edge = eg;
	eg->he1 = he1;
	eg->he2 = he2;

	he1->adjacent = he2;
	he2->adjacent = he1;

	v2->nextv = v1->nextv;
	v2->prevv = v1;
	v1->nextv->prevv = v2;
	v1->nextv = v2;

	// join in the loop
	// 1.init edge
	if (lp->ledge == NULL) {
		he2->next = he1;
		he2->prev = he1;
		he1->prev = he2;
		he1->next = he2;
		lp->ledge = he1;
	}
	// 2. not start edge
	else {
		for (he = lp->ledge; he->next->startv != v1; he = he->next);
		he2->next = he->next;
		he->next->prev = he2;
		he->next = he1;
		he2->prev = he1;
		he1->prev = he;
		he1->next = he2;
	}
	return he1;
}

// 连接面f1上的两个点v1，v2，以两给定点生成一条新边e，并生成一个新面f2
Loop* EulerOperation::mef(Vertex* v1, Vertex* v2, Loop*& lp)
{
	Face* f2 = new Face;
	Loop* newlp = new Loop;

	Edge* eg = new Edge;
	HalfEdge* he1 = new HalfEdge;
	HalfEdge* he2 = new HalfEdge;
	HalfEdge* orihe1 = new HalfEdge;
	HalfEdge* orihe2 = new HalfEdge;

	eg->he1 = he1;
	eg->he2 = he2;
	he1->edge = he2->edge = eg;
	he1->adjacent = he2;
	he2->adjacent = he1;
	he1->startv = v1;
	he1->endv = v2;
	he2->startv = v2;
	he2->endv = v1;

	for (orihe1 = lp->ledge; orihe1->next->startv != v1; orihe1 = orihe1->next);
	for (orihe2 = lp->ledge; orihe2->next->startv != v2; orihe2 = orihe2->next);

	he1->prev = orihe1;
	he1->next = orihe2->next;
	orihe2->next->prev = he1;
	he2->prev = orihe2;
	he2->next = orihe1->next;
	orihe1->next->prev = he2;
	orihe1->next = he1;
	orihe2->next = he2;

	lp->ledge = he1;
	he1->wloop = lp;
	he2->wloop = newlp;
	newlp->ledge = he2;
	newlp->lface = f2;

	f2->floops.push_back(newlp);
	f2->fsolid = lp->lface->fsolid;
	f2->fsolid->sfaces.push_back(f2);

	// 返回size较大的环，如果size相同，返回新环
	if (newlp->size() > lp->size()) {
		Loop* tmp = newlp;
		newlp = lp;
		lp = tmp;
	}
	return newlp;
}

// 消去环中的一条边e，生成该边某一邻面上的一个新的内环
Loop* EulerOperation::kemr(Vertex* v1, Vertex* v2, Loop* lp)
{
	// 设v1在外环上，v2在内环上
	Loop* newlp = new Loop; // 新生成的内环
	HalfEdge* he1 = new HalfEdge;
	HalfEdge* he2 = new HalfEdge;

	for (he1 = lp->ledge; he1->startv != v1 || he1->endv != v2; he1 = he1->next);
	for (he2 = lp->ledge; he2->startv != v2 || he2->endv != v1; he2 = he2->next);

	lp->ledge = he1->prev;
	newlp->ledge = he1->next;
	newlp->lface = lp->lface;
	lp->lface->floops.push_back(newlp);

	he1->next->prev = he2->prev;
	he2->prev->next = he1->next;

	he1->prev->next = he2->next;
	he2->next->prev = he1->prev;

	delete(he1);
	delete(he2);

	if (newlp->size() > lp->size()) {
		Loop* tmp = newlp;
		newlp = lp;
		lp = tmp;
	}
	return newlp;
}

// 删除与面f1相接触的一个面f2，生成面f1上的一个内环，并生成体上的一个通孔，进而在体内构造一个柄，或将两物体合并为一个物体
void EulerOperation::kfmrh(Loop* outlp, Loop* lp)
{
	Face* f1 = outlp->lface;
	Face* f2 = lp->lface;

	// add inner loop to face1
	f1->floops.push_back(lp);

	// delete face
	Solid* s = f1->fsolid;
	for (auto iter = s->sfaces.begin(); iter != s->sfaces.end(); iter++) {
		if (*iter == f2) {
			s->sfaces.erase(iter);
			break;
		}
	}
}

Solid* EulerOperation::sweep(Face* f, Point dir, double distance)
{
	Solid* s = f->fsolid;
	for (int i = 0; i < f->floops.size(); i++) {
		Loop* lp = f->floops[i];
		HalfEdge* nowEdge = lp->ledge;
		Vertex* firstV = nowEdge->startv;

		HalfEdge* edgeUp1 = mev(firstV, firstV->p + dir * distance, lp);
		Vertex* firstUp = edgeUp1->endv;

		Vertex* prevUp = firstUp;
		nowEdge = nowEdge->next;
		Vertex* nextV = nowEdge->startv;
		while (nextV != firstV) {

			HalfEdge* edgeUp = mev(nextV, nextV->p + dir * distance, lp);
			Vertex* Up = edgeUp->endv;
			mef(prevUp, Up, lp);
			prevUp = Up;
			nowEdge = nowEdge->next;
			nextV = nowEdge->startv;
		}
		mef(prevUp, firstUp, lp);
	}
	kfmrh(s->sfaces[0]->floops[0], s->sfaces[1]->floops[0]);
	return s;
}