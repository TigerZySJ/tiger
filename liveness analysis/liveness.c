#pragma once
#include "liveness.h"
static void enterLiveMap(G_table t, G_node flowNode, Temp_tempList temps) {
	G_enter(t, flowNode,temps);
}

static Temp_tempList lookupLiveMap(G_table t, G_node flowNode) {
	return (Temp_tempList)G_look(t, flownode);
}

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail) {
	Live_moveList list = (Live_moveList)checked_malloc(sizeof(*list));
	list->dst = dst;
	list->src = src;
	list->tail = tail;
	return list;
}

Temp_temp Live_gtemp(G_node n) {
	return (Temp_temp)G_nodeInfo(n);
}

static void InOutTable(G_graph g, G_table in, G_table out) {
	asser(g&&"graph can't be null");
	G_nodeList nodelist = G_nodes(g);
	//��ʼ��in��out��
	for (;nodelist; nodelist = nodelist->tail) {
		G_enter(in,nodelist->head,NULL);
		G_enter(out, nodelist->head, NULL);
	}
	bool flag = TRUE;

	while (flag) {
		nodelist = G_nodes(g);
		flag = FALSE;
		for (; nodelist; nodelist = nodelist->tail) {
			Temp_tempList in_list = (Temp_tempList)G_look(in, nodelist->head);
			Temp_tempList out_list = (Temp_tempList)G_look(out, nodelist->head);
			//in'[n]<-in[n] out'[n]<-out[n]
			Temp_tempList in_list_tmp = Temp_TempList_Copy(in);
			Temp_tempList out_list_tmp = Temp_TempList_Copy(out);
			//in[n] = use[n] U (out[n] - def[n])
			//out[n] = U in[s] {s, s->succ[n]}			
			in_list = unionTempList(FG_use(nodelist->head), exceptTempList(out_list,FG_def(nodelist->head)));
			G_nodeList succ = G_succ(nodelist->head);
			for (; succ; succ = succ->tail) {
				out_list = unionTempList(out_list, (Temp_tempList)G_look(in, nodelist->head));
			}
			G_enter(in,nodelist->head,in_list);
			G_enter(out, nodelist->head, out_list);
			//���in[n]�Ƿ����out[n]
			if (!equalTempList(in_list, in_list_tmp) ||
				!equalTempList(out_list, out_list_tmp))
				flag = TRUE;
		}
	}
}

//�Գ��ڻ�Ծ���������ͻͼ
TAB_table table_temp;
static Live_graph interferenceGraph(G_nodeList nodelist,G_table out) {
	//��ʼ��ͼ
	G_graph g = G_graph();
	G_nodeList nodelist_tmp = nodelist;
	table_temp = TAB_empty();
	//��¼��ʱ����
	Temp_tempList templist = NULL;
	//�õ�������ʱ����
	for (; nodelist_tmp; nodelist_tmp = nodelist_tmp->tail) {
		templist = unionTempList(templist,unionTempList(FG_def(nodelist_tmp->head), FG_use(nodelist_tmp->head)));
	}
	//������������ʱ������ɵĽڵ�
	for (; templist; templist = templist->tail) {
		G_node node = G_Node(g, templist->head);
		G_enter(table_temp,templist->head,node);
	}
	//���ӳ�ͻ�ߣ���ͻ���жϷ���
	//�������MOVEָ�����ÿ��ָ���def��Ӧ����ʱ������ýڵ㴦�Ļ�Ծ�������г�ͻ
	//�����MOVEָ�Ҫ�ų�src
	nodelist_tmp = nodelist;
	for (; nodelist_tmp; nodelist_tmp = nodelist_tmp->tail) {
		AS_instr instr = (AS_instr)G_nodeInfo(nodelist_tmp->head);
		switch (instr->kind) {
			case I_MOVE:{
				Temp_tempList dstList = instr->u.MOVE.dst;				
				Temp_tempList out_active = G_look(out, nodelist->head);
				for (; dstList; dstList = dstList->tail) {
					//�õ�����def��Ӧ��temp�ڵ�
					G_node node = G_look(table_temp, dstList->head);
					Temp_tempList out_active = G_look(out, nodelist->head);
					G_node node_out;
					for (; out_active; out_active = out_active->tail) {
						node_out = G_look(table_temp, out_active->head);
						//���def��temp��Ӧ�ڵ㲻ͬ����Ҫ���Ҷ�Ӧ��temp�Ƿ���use�У����û�������ӳ�ͻ��
						if (node_out != node){
							Temp_tempList srcList = instr->u.MOVE.src;
							bool flag = 0;
							for (; srcList; srcList = srcList->tail) {
								G_node node_use = G_look(table_temp, srcList->head);
								if (node_out == node_use)
									flag = 1;
							}
							if(!flag)
								G_addEdge(node, node_out);
						}
					}
				}
			}
			case I_OPER:{
				Temp_tempList dstList = instr->u.OPER.dst;
				Temp_tempList out_active= G_look(out, nodelist->head);
				for (; dstList; dstList = dstList->tail) {
					//�õ�����def��Ӧ��temp�ڵ�
					G_node node = G_look(table_temp,dstList->head);
					Temp_tempList out_active = G_look(out, nodelist->head);
					G_node node_out;
					for (; out_active; out_active = out_active->tail) {
						node_out= G_look(table_temp, out_active->head);
						//���def��temp��Ӧ�ڵ㲻ͬ�������ӳ�ͻ��
						if (node_out != node)
							G_addEdge(node,node_out);
					}
				}
			}
			default:assert(0);
		}

	}
}

static Temp_tempList unionTempList(Temp_tempList t1, Temp_tempList t2) {
	if (!t1)
		return t2;
	if (!t2)
		return t1;
	Temp_tempList t3 = Temp_TempList_Copy(t1);

	for (; t2; t2 = t2->tail) {
		bool flag = 0;
		for (t1 = t3->head; t1; t1 = t1->tail)
			if (t1->head == t2->head)
				flag = 1;
		//�������t1�У�����t1
		if (!flag)t3 = Temp_TempList(t2->head,t3);
	}
	return t3;
}
//�󲹼�
static Temp_tempList exceptTempList(Temp_tempList t1, Temp_tempList t2) {
	if (!t1)
		return NULL;
	if (!t2)
		return t1;
	Temp_tempList t3 = NULL;
	Temp_tempList t4 = t2;
	for (; t1; t1 = t1->tail) {
		bool flag = 0;
		for (t2 = t4->head; t2; t2 = t2->tail)
			if (t1->head == t2->head)
				flag = 1;
		//t1Ԫ���������t2�У�����t3
		if (��flag)t3 = Temp_TempList(t1->head,t3);
	}
	return t3;
}
//����
static bool equalTempList(Temp_tempList t1, Temp_tempList t2) {
	if (!t1 || !t2)
		return false;
	Temp_tempList t3 = t1, t4 = t2;
	while (t3&&t4) {
		t3 = t3->tail;
		t4 = t4->tail;
	}
	//���Ȳ���
	if (t3 || t4)return false;
	for (; t1; t1 = t1->tail) {
		t3 = t2;
		bool flag = 0;
		for (; t3; t3 = t3->tail) {
			if (t3->head == t1->head)
				flag = 1;
		}
		//t2�в����ҵ�t1��ͷ��ֵ
		if (flag == 0)
			return false;
	}
}
//���������ͼ�����س�ͻͼ
struct Live_graph Live_liveness(G_graph flow) {
	struct Live_graph live_graph;
	G_table in = (G_table)TAB_empty();
	G_table out = (G_table)TAB_empty();
	InOutTable(flow, in, out);
	Live_graph graph = interferenceGraph(G_nodes(flow), out);
	live_graph.graph = graph;
}