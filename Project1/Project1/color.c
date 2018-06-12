#include "color.h"
#include "temp.h"
static G_nodeList spillWorkList = NULL;
static G_nodeList freezeWorkList = NULL;
static G_nodeList simplifyorkList = NULL;

static Live_moveList workListMoves = NULL;
static Live_moveList constrainedMoves = NULL;
static Live_moveList coalescedMoves = NULL;
static Live_moveList activeMoves = NULL;
static Live_moveList frozenMoves = NULL;

static G_nodeList coalescedNodes = NULL;
static G_nodeList spilledNodes = NULL;
static G_nodeList coloredNodes = NULL;


static void decrementDegree(G_node m);
static void simplify();
static void makeWorklist(G_nodeList initial);
static Live_moveList nodeMoves(G_node n);
static bool moveRelated(G_node n);
static G_nodeList adjacent(G_node n);
static void enableMoves(G_nodeList nodes);
static void Coalesce();
static bool Conservative(G_nodeList nodes);
static void addWorkList(G_node u);
static G_node getAlias(G_node n);
static void Combine(G_node u, G_node v);
static void Freeze();
static void freezeMoves(G_node u);
static void selectSpill();
static void assignColors(Temp_tempList colors);
static void addEdge(G_node u, G_node v);

static bool **adjSet = NULL;
static G_nodeList* adjList = NULL;
static G_nodeList selectStack = NULL;
static TAB_table moveList = NULL;
static int *degree = NULL;
static bool*preColored = NULL;
static TAB_table alias = NULL;
static int K;
static G_table color= NULL;
string registers[7] = { "%eax", "%ecx", "%edx", "%ebx", "%esi", "%edi", "%ebp" };
struct COL_result COL_color(G_graph ig, Temp_map initial, Temp_tempList regs) {

}
void addEdge(G_node u, G_node v) {
	if (!u || !v)
		return;
	int m = u->mykey;
	int n = v->mykey;
	if (m != n&&adjSet[m][n] == false) {
		adjSet[m][n] = adjSet[n][m] = true;
		if (!preColored[m]) {
			degree[m] += 1;
			adjList[m] = G_NodeList(v, adjList[m]);
		}
		if (!preColored[n]) {
			degree[n] += 1;
			adjList[n] = G_NodeList(u, adjList[n]);
		}
	}
}
void makeWorklist(G_nodeList nodes) {

}
G_nodeList adjacent(G_node n) {
	return intersection_G_nodeList(adjList[n->mykey],
		union_G_nodeList(selectStack,coalescedNodes));
}
Live_moveList nodeMoves(G_node n) {
	return intersection_LiveMoveList(TAB_look(moveList,n),
		union_Live_moveList(activeMoves, workListMoves));
}
bool isEmptyLiveMoveList(Live_moveList movelist) {
	return movelist == NULL;
}
bool moveRelated(G_node n) {
	return !isEmptyLiveMoveList(nodeMoves(node));
}
void simplify() {
	while (simplifyorkList) {
		G_node n = simplifyorkList->head;
		simplifyorkList = simplifyorkList->tail;
		selectStack = G_NodeList(n, selectStack);
		G_nodeList nodelist = adjacent(n);
		for (; nodelist; nodelist = nodelist->tail)
			decrementDegree(nodelist->head);
	}
}
void decrementDegree(G_node m) {
	int d = degree[m->mykey];
	degree[m->mykey]--;
	if (d == K) {
		enableMoves(union_G_nodeList(G_NodeList(m, NULL), adjacent(m));
		spillWorkList = except_G_nodeList(spillWorkList, G_NodeList(m, NULL));
		if (moveRelated(m))
			freezeWorkList = union_G_nodeList(G_NodeList(m, NULL), freezeWorkList);
		else
			simplifyorkList= union_G_nodeList(G_NodeList(m, NULL), simplifyWorkList);
	}
}
void makeWorklist(G_nodeList initial) {
	while (initial) {
		G_node node = initial->head;
		if (degree[node->mykey] >= K)
			spillWorkList = union_G_nodeList(spillWorkList, G_NodeList(node, NULL));
		else if moveRelated(node)
			freezeWorkList= union_G_nodeList(freezeWorkList, G_NodeList(node, NULL));
		else
			simplifyorkList= union_G_nodeList(simplifyWorkList, G_NodeList(node, NULL));
		initial = initial->tail;
	}
}
void enableMoves(G_nodeList nodes) {
	while (nodes) {
		Live_moveList nodelist = nodeMoves(nodes->head);
		while (nodelist) {
			Live_moveList m=Live_MoveList(nodelist->src, nodelist->dst, NULL);
			if (intersection_Live_moveList(
				m,activeMoves) != NULL)
				activeMoves = except_Live_moveList(activeMoves,m, NULL));
				worklistMoves = except_Live_moveList(worklistMoves, m, NULL));
		}
	}
}
static bool OK(G_node t1, G_node t2) {
	return degree[t1->mykey] < K || preColored[t1] || 
		adjSet[t1->mykey][t2->mykey];
}
G_node getAlias(G_node n) {
	if (intersection_G_nodeList(coalescedNodes,
		G_NodeList(n, NULL) != NULL)
		return getAlias(TAB_look(alias,n));
}
void addWorkList(G_node u) {
	if (preColored[u->mykey] && !moveRelated(u) && degree[u->mykey] < K) {
		freezeWorkList = except_G_nodeList(freezeWorkList, G_NodeList(u, NULL));
		simplifyorkList = union_G_nodeList(simplifyorkList, G_NodeList(u, NULL));
	}
}
void Freeze() {
	G_nodelist u = freezeWorkList->head;
	freezeWorkList = except_G_nodeList(freezeWorkList, u);
	simplifyorkList = union_G_nodeList(simplifyorkList, u);
	freezeMoves(u);
}
bool Conservative(G_nodeList nodes) {
	int k = 0;
	while (nodes) {
		if (degree[nodes->head->mykey] >= K)
			k++;
		nodes = nodes->tail;
	}
	return k < K;
}
void Coalesce() {
	while (workListMoves) {
		G_node x = getAlias(workListMoves->dst);
		G_node y = getAlias(workListMoves->src);
		G_node u,v;
		if (preColored[y->mykey]) {
			u = y;
			v = x;
		}
		else {
			u = x;
			v = y;
		}
		if (u == v) {
			coalescedMoves = union_Live_moveList(coalescedMoves,
				Live_MoveList(workListMoves->src,workListMoves->dst,NULL));
			addWorkList(u);
		}
		else if (preColored[v->mykey] && adjSet[u->mykey][v->mykey]) {
			constrainedMoves= union_Live_moveList(constrainedMoves,
				Live_MoveList(workListMoves->src, workListMoves->dst, NULL));
			addWorkList(u);
			addWorkList(v);
		}
		else {
			bool flag = true;
			G_nodeList nodelist = adjacent(v);
			while (nodelist) {
				if (!OK(nodelist->head, u)) {
					flag = false;
					break;
				}
				nodelist = nodelist->tail;
			}
			if ((preColored[u->mykey]&&flag)||
				(!preColored[u->mykey]&&
					Combine(union_G_nodeList(adjacent(u),adjacent(v))))) {
				coalescedMoves = union_Live_moveList(coalescedMoves,
					Live_MoveList(workListMoves->src, workListMoves->dst, NULL));
				Combine(u, v);
				addWorkList(u);
			}
			else
				activeMoves = union_Live_moveList(activeMoves,
					Live_MoveList(workListMoves->src, workListMoves->dst, NULL));
		}		
	}
}
void Combine(G_node u, G_node v) {
	G_nodeList u_list = G_NodeList(u, NULL);
	G_nodeList v_list = G_NodeList(v, NULL);
	if (intersection_G_nodeList(freezeWorkList, v_list))
		freezeWorkList = except_G_nodeList(freezeWorkList, v_list);
	else
		spillWorkList = except_G_nodeList(spillWorkList, v_list);
	coalescedNodes = union_G_nodeList(coalescedNodes, v_list);
	TAB_enter(alias, v, u);
	TAB_enter(moveList, u, union_Live_moveList(TAB_look(moveList, u),
		TAB_look(moveList,v)));
	enableMoves(v_list);
	G_nodeList t = adjacent(v);
	while (t) {
		addEdge(t->head,u);
		decrementDegree(t->head);
		t = t->tail;
	}
	if (degree[u->mykey] >= K&&
		intersection_G_nodeList(
			freezeWorkList, u_list)) {
		freezeWorkList = except_G_nodeList(freezeWorkList, u_list);
		spillWorkList = except_G_nodeList(spillWorkList, u_list);
	}
}
void selectSpill() {
	G_node m= NULL;
	int maxDegree = -1;
	G_nodeList nodelist = spillWorkList;
	for (; nodelist; nodelist = nodelist->tail) {
		if (degree[nodelist->head->mykey] > maxDegree) {
			m = nodelist->head;
			maxDegree = degree[nodelist->head->mykey];
		}
	}
	simplifyWorkList = except_G_nodeList(simplifyWorkList, m);
	spillWorkList = except_G_nodeList(spillWorkList, m);
	freezeMoves(m);
}
void freezeMoves(G_node u) {
	Live_moveList ml= nodeMoves(u);
	for (; ml; ml = ml->tail) {
		G_node x = ml->dst;
		G_node y = ml->src;
		G_node v;
		if (getAlias(y) == getAlias(u))
			v = getAlias(x);
		else
			v = getAlias(y);
		activeMoves = except_Live_moveList(activeMoves,
			Live_MoveList(ml->src,ml->dst,NULL));
		freezeMoves = union_Live_moveList(freezeMoves,
			Live_MoveList(ml->src, ml->dst, NULL));
		if (nodeMoves(v) == NULL&&degree[v->mykey] < K) {
			freezeWorkList = except_G_nodeList(freezeWorkList,
				G_NodeList(v,NULL));
			simpilfyWorkList = union_G_nodeList(simpilfyWorkList,
				G_NodeList(v, NULL));
		}
	}
}
Temp_map assignColors() {
	Temp_map temp_map = F_initialTempMap();
	while (selectStack != NULL) {
		G_node n = selectStack->head;
		selectStack = selectStack->tail;
		int okColors[K];
		for (j = 0; j < K; j++)
			okColors = 1;
		G_nodeList nl = adjacent(n);
		bool flag = true;
		for (; nl; nl = nl->tail)
			if (preColored[getAlias(nl->head)->mykey] ||
				intersection_G_nodeList(coloredNodes,
					G_NodeList(getAlias(nl->head), NULL)))
				okColors[(int)G_look(color, getAlias(nl->head))] = 0;
		for (int i = 0; i < K; i++) {
			if (okColors[i] == 1) {
				coloredNodes = G_NodeList(n, coloredNodes);
				G_enter(color, node, i);
				Temp_enter(temp_map, (Temp_temp)G_nodeInfo(node), registers[i]);
				flag = false;
				break;
			}
		}
		if (!flag)
			spilledNodes = G_NodeList(n,spilledNodes);
	}
	G_nodeList nl = coalescedNodes;
	while (nl) {
		int color_num = (int)G_look(color, getAlias(nl->head));
		G_enter(color, nl->head, color_num);
		Temp_enter(temp_map, (Temp_temp)G_nodeInfo(nl->head), registers[color_num]);
		nl = nl->tail;
	}
	return temp_map;
}