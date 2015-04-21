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
#include "boruvkasequential.hpp"

using namespace std;

namespace parallel { 
	//METHODS
	vector<Edge> boruvkaParallel(vector<Edge*>,int);
	void getAsMap(vector<Edge*> *);
	void initializeRoot();
	void initializeMaps();
	void resetMaps();
	vector<int> getKeyset();
	vector<int> getNewKeyset();
	void print(vector<int>);
	void setCheapestEdgesForEachComponent();
	void* setCheapestEdgesForEachComponentThreadBody(void *);
	void print(map<int,Edge>);
	bool edgeCompare(Edge, Edge);
	void setNewGroups();
	void setNewGroupsThreadBody(void *);
	void updateNewGroups(int, int);
	void updateInEdges(int, int, Edge);
	int getRoot(int);
	void unionFind(int, int);
	void setNewOutEdges();
	void* setNewOutEdgesThreadBody(void *);
	void deleteAll(vector<Edge>*, Edge);
	void deleteAllCycles(int);
	bool contains(map<int, vector<int> >, int);
	bool contains(vector<int>, int);
	void print(vector<Edge>);
	void print(map<int,vector<int> >);
	void print(map<int,vector<Edge> >);
	void print(Edge *);

	//VARIABLES
	int num_threads;			//number of threads to be used 
	int N;					//number of nodes in the graph
	int * root;				//array that keeps track of the root of tree to which each vertex belongs through one iteration, root[i] is the index of the vertex that is the chosen as the root of the tree to which the i-th vertex belongs
	map<int, vector<Edge> > graph;		//keeps the original structure of the graph at all times
	map<int, vector<int> > groups;		//keeps track of component grouping through one iteration
	vector<Edge> * outEdges;		//keeps track of outgoing edges for each component through one iteration
	
	map<int, vector<Edge> > inEdges;	//keeps track of inner edges (mst edges) for each component through one iteration
	Edge* cheapest;				//keeps track of the cheapest outgoing edge for each component through one iteration
	map<int, vector<int> > newGroups;	//stores the new groups as arrays of subcomponent indices
	vector<Edge> * newOutEdges;		//stores the new outgoing edges of each component, edges eligible to be picked as cheapest
	
	vector<int> keyset;			//stores the indices of the components between iterations

	//A parallel implementation of the boruvka MST algorihm
	vector<Edge> boruvkaParallel(vector<Edge*> edges, int n){
		N = n;
		getAsMap(&edges);
		initializeRoot();
		initializeMaps();
		int i = 0;
		while(groups.size() > 1){
			resetMaps();
			setCheapestEdgesForEachComponent();
			setNewGroups();
			if (newGroups.size() == 1) break;
			setNewOutEdges();
		}

		return inEdges[0];
	}
	//transforms a list of edges into a map (adjacency map),
	//where key is the graphnode, and value is a list of all adjacent edges to that node
	void getAsMap(vector<Edge*> *edges){

		for(int i = 0; i<N; i++){
			graph[i] = *(new vector<Edge>());
		}
		vector<Edge*>::iterator it;
		for(it = edges->begin(); it != edges->end(); it++){
			int x = (*it)->x;
			int y = (*it)->y;
			graph[x].insert(graph[x].end(), (**it));
			graph[y].insert(graph[y].end(), (**it));
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
		inEdges = *(new map<int, vector<Edge> >());
		outEdges = new vector<Edge>[N];
		newGroups = *(new map<int, vector<int> >());
		newOutEdges = new vector<Edge>[N];
		cheapest = new Edge[N];

		for(int i = 0; i<N; i++){
			groups[i] = *(new vector<int>());
			groups[i].insert(groups[i].end(),i);
			outEdges[i] = *(new vector<Edge>(graph[i]));
			inEdges[i] = *(new vector<Edge>());
		}
	}
	//resets the map newOutEdges, and presets 'newGroups' and 'keyset'
	void resetMaps(){
		newOutEdges = new vector<Edge>[N];
		newGroups = groups;
		keyset = getKeyset();
	}
	//returns a vector of component id-s of the components that are input in the current iteration
	// i.e. the components stored in 'groups'
	vector<int> getKeyset(){
		vector<int> keys;
		map<int,vector<int> >::iterator it;
		for(it = groups.begin(); it != groups.end(); it++){
			keys.insert(keys.end(),it->first);
		}
		return keys;
	}
	//returns a vector of component id-s of the new components that are obtained from the current iteration
	// i.e. the components stored in 'newGroups'
	vector<int> getNewKeyset(){
		vector<int> keys;
		map<int,vector<int> >::iterator it;
		for(it = newGroups.begin(); it != newGroups.end(); it++){
			keys.insert(keys.end(),it->first);
		}
		return keys;

	}
	//fill the array "cheapest" with the cheapest outgoing edge for each component
	//splits the workload to threads.
	//index: component id
	//value: cheapest edge for that component
	void setCheapestEdgesForEachComponent(){
		pthread_t thread[num_threads];
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

		for (int i=0; i<num_threads; ++i)
		{
			pthread_create(&thread[i], &attr,
			       setCheapestEdgesForEachComponentThreadBody, (void *) i);
		}

		for (int i=0; i<num_threads; ++i)
		{
			pthread_join(thread[i], NULL);
		}
	}
	//a separate function for finding the cheapest edges, executed by each thread separately
	void* setCheapestEdgesForEachComponentThreadBody(void * args){
		int tid = (int)args;
	
		int num_components = keyset.size();
		int start = tid * ((num_components+(num_threads-1))/num_threads);
		int end = min(start + ((num_components+(num_threads-1))/num_threads),num_components); 
		
		for(int i = start; i<end; i++){
			int id = keyset.at(i);
			vector<Edge> *compEdges = &(outEdges[id]);
			sort (compEdges->begin(), compEdges->end(), edgeCompare);
			cheapest[id] = compEdges->at(0);
		}
	}
	//compares two edges by their weight
	//this function is used for sorting the edge lists,
	//.. when comparing the equvalency between the parallel and sequential solution
	bool edgeCompare (Edge e1, Edge e2) { 
		return (e1.w < e2.w); 
	}
	//finds components that need to be merged, and merges them i.e. their 'newGroups' and 'inEdges' entries
	void setNewGroups(){
		for(int i = 0; i<keyset.size(); i++){
			int id = keyset.at(i);
			Edge picked = cheapest[id];

			int root1 = getRoot(picked.x);
			int root2 = getRoot(picked.y);
		
			if (root1 == root2) {
				continue;
			}
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
	//updates the map "newGroups" by merging the too components (root1 and root2) given as input.
	void updateNewGroups(int root1, int root2){
		newGroups[root1].insert(newGroups[root1].end(),newGroups[root2].begin(),newGroups[root2].end());
		newGroups.erase(root2);
	}
	//updates the map "inEdges" by merging the inner (MST) edges for the two components given as input
	//.. and adding the edge "picked" that connects these two components
	void updateInEdges(int root1, int root2, Edge picked){
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
	//iterates through all new components and their subcomponents and finds all outgoing edges for each component
	//split the workload to threads
	void setNewOutEdges(){

		pthread_t thread[num_threads];
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		keyset = getNewKeyset();
		for (int i=0; i<num_threads; i++)
		{
			pthread_create(&thread[i], &attr,
			       setNewOutEdgesThreadBody, (void *) i);
		}

		for (int i=0; i<num_threads; i++)
		{
			pthread_join(thread[i], NULL);
		}
		outEdges = newOutEdges;
		groups = newGroups;
	
	}
	// a separate function for each thread that find the cheapest edge for each component in a ceratin range of indices.
	void* setNewOutEdgesThreadBody(void * args){

		int tid = (int)args;
	
		int num_components = keyset.size();
		int start = tid * ((num_components+(num_threads-1))/num_threads);
		int end = min(start + ((num_components+(num_threads-1))/num_threads),num_components); 
	
		for(int i = start; i<end; i++){
			int newComponentID = keyset.at(i);

			vector<int> subcomponents = newGroups[newComponentID];
			newOutEdges[newComponentID] = *(new vector<Edge>());

			vector<int>::iterator sub_it;
			for(sub_it = subcomponents.begin(); sub_it != subcomponents.end(); sub_it++){
				if(!contains(groups,(*sub_it))){
					continue;
				}

				newOutEdges[newComponentID].insert(newOutEdges[newComponentID].end(),outEdges[*sub_it].begin(), outEdges[*sub_it].end());
				outEdges[*sub_it].clear();
			}
		
			vector<Edge>::iterator e_it;
			for(e_it = inEdges[newComponentID].begin(); e_it != inEdges[newComponentID].end(); e_it++){
				deleteAll(&newOutEdges[newComponentID],*e_it);
			}
			deleteAllCycles(newComponentID);\
		}
	
	}
	//deletes all duplicates of edge 'e' in the vector 'v'
	void deleteAll(vector<Edge> *v, Edge e){
		vector<Edge>::iterator it = v->begin();
		while(it != v->end()){
			if(edgeEquals(&(*it),&e)){
				*it = v->back();
				v->pop_back();
				continue;
			}
			it++;
		}
	}
	//iterates through all edges of component 'id' and deletes all edges that have two endpoints in the same component
	void deleteAllCycles(int id){
		vector<Edge> *v = &newOutEdges[id];
		vector<int> subs = groups[id];
	
		vector<Edge>::iterator it = v->begin();
		while(it != v->end()){
			int x = (*it).x;
			int y = (*it).y;
			if(contains(subs,x) && contains(subs,y)){
				*it = v->back();
				v->pop_back();		
				continue;
			}
			it++;
		}
	}
	//checks if the map 'm' contains the key 'val'
	bool contains(map<int, vector<int> > m, int val){
		return m.find(val) != m.end();
	}
	//checks if vector 'v' contains the value 'val'
	bool contains(vector<int> v, int val){
		return find(v.begin(),v.end(),val) != v.end();	
	}
	void print(vector<Edge> v){
		vector<Edge>::iterator it;
		for(it = v.begin(); it != v.end(); it++){
			cout<<it->x<<" "<<it->y<<" "<<it->w<<endl;
		}
		cout<<endl;
		
	}
	void print(map<int,vector<int> > g){
		map<int, vector<int> >::iterator it;
		for(it = g.begin(); it != g.end(); it++){
			int id = it->first;
			vector<int> vec = it->second;

			cout<<"compid "<<id<<"....";
			print(vec);
		}
		cout<<endl;
		
	}
	void print(map<int,vector<Edge> > g){
		map<int, vector<Edge> >::iterator it;
		for(it = g.begin(); it != g.end(); it++){
			int id = it->first;
			vector<Edge> vec = it->second;

			cout<<"compid "<<id<<"....";
			vector<Edge>::iterator vit;
			for(vit = vec.begin(); vit != vec.end(); vit++){
				cout<<(*vit).x<<" "<<(*vit).y<<" "<<(*vit).w <<", ";
			}
			cout<<endl;
		}
		cout<<endl;
	}
	void print(Edge * g){
		for(int i = 0; i<keyset.size(); i++){
			Edge e = cheapest[keyset.at(i)];
			cout<<"compid.."<<keyset.at(i)<<" edge "<<e.x<<" "<<e.y<<" "<<e.w<<endl;	
		}
		cout<<endl;
	}
	void print(map<int,Edge> g){
		for(int i = 0; i<keyset.size(); i++){
			Edge e = cheapest[keyset.at(i)];
			cout<<"compid.."<<keyset.at(i)<<" edge "<<e.x<<" "<<e.y<<" "<<e.w<<endl;	
		}
		cout<<endl;
	}
	void print(vector<int> v){
		vector<int>::iterator it;
		for(it = v.begin(); it != v.end(); it++){
			cout<<*it<<" ";
		}
		cout<<endl;
		
	}
}
int N,E;
int nodes[7] = {100,200,400,500,700,850,1000};
bool areEqual(vector<Edge*>, vector<Edge> );
int main(){
	
	int t;
	cout<<"Enter number of threads: "<<endl;
	scanf("%d",&t);
	cout<<"MATCH(?) #Nodes\t#Edges\tSequential time\tParallel time\t#threads  Speedup"<<endl;
	for(int i = 0; i<7; i++){
		for(int j = 0; j<3; j++){
			N = nodes[i];
			parallel::num_threads = t;
			vector<Edge*> g;	

			vector<Edge*> gb = *(new vector<Edge*>());
			vector<Edge> gbparallel = *(new vector<Edge>());
	
			g = generateGraph(N);	

			long long timeboruvka = timer();
			gb = boruvka::boruvka(g,N);
			timeboruvka = timer() - timeboruvka;

			long long timeparallel = timer();
			gbparallel = parallel::boruvkaParallel(g,N);
			timeparallel = timer() - timeparallel;
			bool ok = areEqual(gb,gbparallel);

			cout<<(ok?"MATCH\t":"ERROR\t")<<N<<"\t"<<g.size()<<"\t"<<(timeboruvka/1000000.0)<<"\t"<<(timeparallel/1000000.0)<<"\t"<<t<<"\t  "<<timeboruvka*1.0/timeparallel<<endl;
		}
	}
	return 0;
}
bool areEqual(vector<Edge*> v1, vector<Edge> v2){
	sort(v1.begin(),v1.end(),boruvka::edgeCompare);
	sort(v2.begin(),v2.end(),parallel::edgeCompare);

	vector<Edge*>::iterator it1 = v1.begin();
	vector<Edge>::iterator it2 = v2.begin();

	bool ok = true;
	int sum1 = 0;
	int sum2 = 0;
	while(it1 != v1.end() && it2 != v2.end()){
		if(((*it2).x != (**it1).x)){
			ok = false;
			break;
		}
		if(((*it2).y != (**it1).y)){
			ok = false;
			break;
		}
		if(((*it2).w != (**it1).w)){
			ok = false;
			break;
		}
		sum1 += (**it1).w;
		sum2 += (*it2).w;
		it1++;
		it2++;
	}
	return ok;
}
