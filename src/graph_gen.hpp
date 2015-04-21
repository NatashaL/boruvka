#ifndef GRAPH_GEN_HPP
#define GRAPH_GEN_HPP

#include <vector>
#include <set>
#include <map>
using namespace std;

typedef struct Edge{
	int x;	//one endpoint
	int y;	//other endpoint
	int w;	//weight
	int getOtherEndpoint(int endpoint){
		return x == endpoint ? y : (y == endpoint ? x : -1);
	}
}Edge;

vector<Edge*> generateGraph(int);
bool contains(map<int, vector<Edge*> >, Edge);
bool contains(set<int>*, int);
bool edgeEquals (Edge*, Edge*);
bool isConnected(map<int, vector<Edge*> > , int);
void print(vector<Edge*> );
void print(set<int>);
void print(vector<int>);

#endif
