#include "flowgraph.h"
Temp_tempList FG_def(G_node n) {
	if (!n) {
		assert(0&&"Invalid use for flowgraph");
	}
	AS_instr instr=(AS_instr)G_nodeInfo(n);
	switch (instr->kind)
	{
		case I_OPER: { return instr->u.OPER.dst; }
		case I_LABEL: {return NULL; }
		case I_MOVE: { return instr->u.MOVE.dst; }
		default:
			break;
	}
}
Temp_tempList FG_use(G_node n) {
	if (!n) {
		assert(0 && "Invalid use for flowgraph");
	}
	AS_instr instr = (AS_instr)G_nodeInfo(n);
	switch (instr->kind)
	{
	case I_OPER: { return instr->u.OPER.src; }
	case I_LABEL: {return NULL; }
	case I_MOVE: { return instr->u.MOVE.src; }
	default:
		break;
	}
}
bool FG_isMove(G_node n) {
	if (!n) {
		assert(0 && "Invalid use for flowgraph");
	}
	AS_instr instr = (AS_instr)G_nodeInfo(n);
	return instr->kind == I_MOVE;
}
G_graph FG_AssemFlowGraph(AS_instrList il) {
	G_graph graph = G_Graph();
	TAB_table table_label = TAB_empty();
	assert(il&&"as_instrlist can't be null");
	G_node prev_node=NULL;
	G_nodeList nodelist = NULL;
	for (; il; il = il->tail) {
		G_node node = G_Node(graph,il->head);
		if (prev_node)G_addEdge(prev_node, node);
		switch (il->head->kind)
		{
			case I_OPER: {
				nodelist = G_NodeList(node, nodelist);
				break;
			}
			case I_LABEL: {
				//用来之后jump域连线使用
				TAB_enter(table_label, il->head->u.LABEL.label,node);
				break;
			}
			case I_MOVE: {
				nodelist = G_NodeList(node, nodelist);
				break;
			}
			default:
				break;
		}
		prev_node = node;
	}
	for (; nodelist; nodelist = nodelist->tail) {
		G_node node = nodelist->head;
		AS_instr instr=(AS_instr)G_nodeInfo(node);
		if (instr->kind == I_OPER) {
			if (instr->u.OPER.jumps->labels != NULL) {
				Temp_label label = instr->u.OPER.jumps->labels->head;
				//查找jump域对应label的node，进行连接
				G_node succ = (G_node)TAB_look(table_label,label);
				G_addEdge(node, succ);
			}
		}
	}

}