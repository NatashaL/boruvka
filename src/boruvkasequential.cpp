#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "timer.h"
#include <map>
#include <vector>
#include <iostream>
#include <typeinfo>
#include <algorithm> 
#include <set>
#include "graph_gen.hpp"
using namespace std;

namespace boruvka{

	vector<Edge*> boruvka(vector<Edge*>,int);
	void getAsMap(vector<Edge*> *);
	void initializeRoot();
	void initializeMaps();
	bool edgeCompare(Edge*, Edge*);
	void resetMaps();
	void setCheapestEdgesForEachComponent();
	void updateNewGroups(int, int);
	void updateInEdges(int, int, Edge*);
	int getRoot(int);
	void unionFind(int, int);
	void setNewGroupsAndNewInEdges();
	void setNewOutEdges();
	void deleteAll(vector<Edge*>*, Edge* );
	void deleteAllCycles(int);
	bool contains(map<int, vector<int> > *, int);
	bool contains(vector<int> *, int );
	void print(map<int,vector<int> >);
	void print(map<int,vector<Edge*> >);
	void print(vector<Edge*> );


	int N;					//number of nodes in the graph
	int * root;				//array that keeps track of the root of tree to which each vertex belongs through one iteration, root[i] is the index of the vertex that is the chosen as the root of the tree to which the i-th vertex belongs

	map<int, vector<Edge*> > graph;		//keeps the original structure of the graph at all times
	map<int, vector<int> > groups;		//keeps track of component grouping through one iteration
	map<int, vector<Edge*> > inEdges;	//keeps track of outgoing edges for each component through one iteration
	map<int, vector<Edge*> > outEdges;	//keeps track of inner edges (mst edges) for each component through one iteration
	map<int, Edge*> cheapest;		//keeps track of the cheapest outgoing edge for each component through one iteration

	map<int, vector<int> > newGroups;	//stores the new groups as arrays of subcomponent indices
	map<int, vector<Edge*> > newOutEdges;	//stores the new outgoing edges of each component, edges eligible to be picked as cheapest


	//executes the sequential boruvka algorithm and returns a list of MST edges for the input graph
	vector<Edge*> boruvka(vector<Edge*> edges,int n){
		N = n;
		getAsMap(&edges);
		initializeRoot();
		initializeMaps();
		while(groups.size() > 1){
			resetMaps();
			setCheapestEdgesForEachComponent();
			setNewGroupsAndNewInEdges();	
			setNewOutEdges();
			if(newGroups.size() == 1) break;
			outEdges = newOutEdges;
			groups = newGroups;
		}

		return inEdges[0];	//last remaining component is always the one with index 0 - the lowest index
	}
	//transforms a list of edges into a map (adjacency map),
	//where key is the graphnode, and value is a list of all adjacent edges to that node
	void getAsMap(vector<Edge*> *edges){

		for(int i = 0; i<N; i++){
			graph[i] = *(new vector<Edge*>());
		}
		vector<Edge*>::iterator it;
		for(it = edges->begin(); it != edges->end(); it++){
			int x = (*it)->x;
			int y = (*it)->y;
			graph[x].insert(graph[x].end(), (*it));
			graph[y].insert(graph[y].end(), (*it));
		}
	}
	//initializes "root" array, so that each node is its own separate component
	void initializeRoot(){
		root = new int[N];
		for(int i = 0; i<N; i++){
			root[i] = i;
		}
	}
	//initializes maps: groups, outEdges and inEdges
	//groups:
	//	- is filled with one element array for each component
	//	- each component has only one subcomponent: itself
	//outEdges:
	//	- is filled with copy arrays of the array of incident edges in map "graph"
	//	- initially each node is its own component, so the outgoing edges of the component are the same as the incident edges of the node
	//inEdges:
	//	- is filled with empty arrays for each component
	//	- loops are ignored this way, they do not contribute to the MST
	//	- initially each component is a single node, so there are no inner MST edges that keep the subcomponents connected
	void initializeMaps(){
		groups = *(new map<int, vector<int> >());
		inEdges = *(new map<int, vector<Edge*> >());
		outEdges = *(new map<int, vector<Edge*> >());
		newGroups = *(new map<int, vector<int> >());
		newOutEdges = *(new map<int, vector<Edge*> >());

		for(int i = 0; i<N; i++){
			groups[i] = *(new vector<int>());
			groups[i].insert(groups[i].end(),i);
			outEdges[i] = *(new vector<Edge*>(graph[i]));
			inEdges[i] = *(new vector<Edge*>());
		}
	}
	//compares edges by their weights
	bool edgeCompare (Edge* e1, Edge* e2) { 
		return (e1->w < e2->w); 
	}
	//clears the two maps
	void resetMaps(){
		newGroups = groups;
		newOutEdges.clear();
	}

	//fill the map "cheapest" with the cheapest outgoing edge for each component
	//key: component id
	//value: cheapest edge for that component
	void setCheapestEdgesForEachComponent(){
		map<int, vector<Edge*> >::iterator comp_it;
		for(comp_it = outEdges.begin(); comp_it != outEdges.end(); comp_it++){
			int componentID = comp_it->first;
			vector<Edge*> compEdges = outEdges[componentID];
			sort (compEdges.begin(), compEdges.end(), edgeCompare);
			cheapest[componentID] = compEdges.at(0);
		}
	}
	//updates the map "newGroups" by merging the too components (root1 and root2) given as input.
	void updateNewGroups(int root1, int root2){
		newGroups[root1].insert(newGroups[root1].end(),newGroups[root2].begin(),newGroups[root2].end());
		newGroups.erase(root2);
	}
	//updates the map "inEdges" by merging the inner (MST) edges for the two components given as input
	//.. and adding the edge "picked" that connects these two components
	void updateInEdges(int root1, int root2, Edge* picked){
		inEdges[root1].insert(inEdges[root1].end(),inEdges[root2].begin(),inEdges[root2].end());
		inEdges[root1].insert(inEdges[root1].end(),picked);
		inEdges.erase(root2);
	}
	//returns the root (index) of the component that contains 'comp'
	int getRoot(int comp){
		while(root[comp] != comp){
			comp = root[comp];
		}
		return comp;
	}
	//merges two components comp1 and comp2 i.e. their 'root' entries
	void unionFind(int comp1, int comp2) {
		if (comp1 < comp2) {
			root[comp2] = comp1;
		} else {
			root[comp1] = comp2;
		}
		return;
	}
	//iterates through the chosen cheapest edges and looks for components that needs to be merged. 
	//.. delegates the merging to the previously mentioned functions
	void setNewGroupsAndNewInEdges(){
		map<int, Edge*>::iterator ch_it;
		for(ch_it = cheapest.begin(); ch_it != cheapest.end(); ch_it++){

			Edge * picked = ch_it->second;

			int root1 = getRoot(picked->x);
			int root2 = getRoot(picked->y);	

			if (root1 == root2) 
				continue;
			if (root1 < root2){
				updateNewGroups(root1,root2);
				updateInEdges(root1,root2,picked);
			}
			else{
				updateNewGroups(root2,root1);
				updateInEdges(root2,root1,picked);
			}
			unionFind(root1,root2);
		}
	}
	//iterates through all new components and their subcomponents and finds all outgoing edges for each component
	void setNewOutEdges(){

		map<int, vector<int> >::iterator ng_it;
		for(ng_it = newGroups.begin(); ng_it != newGroups.end(); ng_it++){
			int newComponentID = ng_it->first;
			vector<int> subcomponents = ng_it->second;

			newOutEdges[newComponentID] = *(new vector<Edge*>());
			vector<int>::iterator sub_it;
			for(sub_it = subcomponents.begin(); sub_it != subcomponents.end(); sub_it++){

				if(!contains(&groups,(*sub_it))){
					continue;
				}

				newOutEdges[newComponentID].insert(newOutEdges[newComponentID].end(),outEdges[*sub_it].begin(), outEdges[*sub_it].end());
				outEdges.erase(*sub_it);
			}
			vector<Edge*>::iterator e_it;
			for(e_it = inEdges[newComponentID].begin(); e_it != inEdges[newComponentID].end(); e_it++){
				deleteAll(&newOutEdges[newComponentID],*e_it);
			}
			deleteAllCycles(newComponentID);
		}
	}
	//deletes all duplicates of edge 'e' in the vector 'v'
	void deleteAll(vector<Edge*> *v, Edge *e){
		vector<Edge*>::iterator it = v->begin();
		while(it != v->end()){
			if(edgeEquals(*it,e)){
				*it = v->back();
				v->pop_back();
				continue;
			}
			it++;
		}
	}
	//iterates through all edges of component 'id' and deletes all edges that have two endpoints in the same component
	void deleteAllCycles(int id){
		vector<Edge*> *v = &newOutEdges[id];
		vector<int> subs = groups[id];
	
		vector<Edge*>::iterator it = v->begin();
		while(it != v->end()){
			int x = (**it).x;
			int y = (**it).y;
			if(contains(&subs,x) && contains(&subs,y)){
				*it = v->back();
				v->pop_back();		
				continue;
			}
			it++;
		}
	}
	//checks if the map 'm' contains the key 'val'
	bool contains(map<int, vector<int> > *m, int val){
		return m->find(val) != m->end();
	}
	//checks if vector 'v' contains the value 'val'
	bool contains(vector<int> * v, int val){
		return find(v->begin(),v->end(),val) != v->end();	
	}
	void print(map<int,vector<int> > g){
		map<int, vector<int> >::iterator it;
		for(it = g.begin(); it != g.end(); it++){
			int id = it->first;
			vector<int> vec = it->second;

			cout<<"compid "<<id<<"....";
			vector<int>::iterator vit;
			for(vit = vec.begin(); vit != vec.end(); vit++){
				cout<<*vit<<", ";
			}
			cout<<endl;
		}
		cout<<endl;
	}
	void print(map<int,vector<Edge*> > g){
		map<int, vector<Edge*> >::iterator it;
		for(it = g.begin(); it != g.end(); it++){
			int id = it->first;
			vector<Edge*> vec = it->second;

			cout<<"compid "<<id<<"....";
			vector<Edge*>::iterator vit;
			for(vit = vec.begin(); vit != vec.end(); vit++){
				cout<<(**vit).x<<" "<<(**vit).y<<" "<<(**vit).w <<", ";
			}
			cout<<endl;
		}
		cout<<endl;
	}
	void print(vector<Edge*> g){
		vector<Edge*>::iterator it;
		for(it = g.begin(); it != g.end(); it++){
			Edge e = **it;
			cout<<e.x<<" "<<e.y<<" "<<e.w <<endl;;
		}
		cout<<endl;
	}
}
