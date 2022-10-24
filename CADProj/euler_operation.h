#ifndef EULER_OPERATION_H
#define EULER_OPERATION_H

#include "half_edge_structure.h"

class EulerOperation
{
public:

	// 生成一个含有一个点v的面，并且构成一个新的体。从无到有构造一个点、一个面、一个体
	static Solid* mvsf(Point p, Vertex*& v);

	// 生成一个新点v2，同时生成一条连接新点v2与一给定点v1的新边e
	static HalfEdge* mev(Vertex* v1, Point p, Loop* lp);

	// 连接面f1上的两个点v1，v2，以两给定点生成一条新边e，并生成一个新面f2
	static Loop* mef(Vertex* v1, Vertex* v2, Loop*& lp);

	// 消去环中的一条边e，生成该边某一邻面上的一个新的内环
	static Loop* kemr(Vertex* v1, Vertex* v2, Loop* lp);

	// 删除与面f1相接触的一个面f2，生成面f1上的一个内环，并生成体上的一个通孔，进而在体内构造一个柄，或将两物体合并为一个物体
	static void kfmrh(Loop* outlp, Loop* lp);

	// 扫掠操作
	static Solid* sweep(Face* f, Point dir, double distance);
};

#endif