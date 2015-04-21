#ifndef BORUVKA_HPP
#define BORUVKA_HPP

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

namespace boruvka {

	vector<Edge*> boruvka(vector<Edge*>,int);
	void getAsMap(vector<Edge*> *);
	void initializeRoot();
	void initializeMaps();
	bool edgeCompare(Edge*, Edge*);
	void resetMaps();
	void setCheapestEdgesForEachComponent();
	void updateNewGroups(int, int);
	void updateInEdges(int, int, Edge*);
	void unionFind(int, int);
	void setNewGroupsAndNewInEdges();
	void setNewOutEdges();
	bool contains(map<int, vector<int> > *, int);
	void deleteAll(vector<Edge*>*, Edge* );
	void deleteAllCycles(int);
	bool contains(vector<int> *, int );
	void print(map<int,vector<int> >);
	void print(map<int,vector<Edge*> >);
	void print(vector<Edge*> );

}
#endif
