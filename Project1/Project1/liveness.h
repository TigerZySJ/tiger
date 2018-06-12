#pragma once
#ifndef LIVENESS_H
#define LIVENESS_H
#include "graph.h"
#include "flowgraph.h"
#include "util.h"
#include "table.h"
typedef struct Live_moveList_ *Live_moveList;
struct Live_moveList_ { 
	G_node src, dst;
	Live_moveList tail;
};
struct Live_graph {
	G_graph graph;
	Live_moveList moves;
};
Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail);
Temp_temp Live_gtemp(G_node n);
struct Live_graph Live_liveness(G_graph flow);
Live_moveList Live_moveList_Copy(Live_moveList t1);
Live_moveList union_Live_moveList(Live_moveList t1, Live_moveList t2);
Live_moveList except_Live_moveList(Live_moveList t1, Live_moveList t2);
Live_moveList intersection_Live_moveList(Live_moveList t1, Live_moveList t2);
#endif // !LIVENESS_H