# boruvka
A sequential vs a parallel implementation of Borůvka's MST algorithm
<br/>
<br/>

#### 1. Description of the algorithm

Borůvka's algorithm is based on merging of disjoint components. Given a connected, undirected graph G with N vertices and M weighted edges, a minimum spanning tree can be found.
In the beginning of the procedure each vertex is considered as a separate component. In each step the algorithm chooses the cheapest outgoing edge for each component, and connects (merges) the newly derived component. The fact that for each component, we only consider the outgoing edges (edges that have one endpoint in one component, and the other in some other component) guarantees that no cycle will occur while building the spanning tree.
Contrary to the other well-known algorithms like Kruskal’s algorithm where edge-contractions are performed in an increasing weight order, Boruvka’s algorithm proceeds in an unordered fashion.

<b>Step 1:</b> Loop through each component (initially each vertex) and select the outgoing edge with the lightest weight.

<b>Step 2:</b> Find which components have been connected by these cheapest edges, and merge them into one component (keeping track of the inner edges that have been chosen to obtain that component).

<b>Step 3:</b> For each new component, filter the outgoing edges.
<br/>
<br/>

#### 2. How to build and execute the program(s). 

Requirments: g++ compiler, pthreads.h.

Once these are obtained on the system, the executable is made by running the command ```make``` in terminal, in the source folder: ```./src/```. The makefile used to compile the sources is ready and named ```Makefile```.

Only one executable needs to be run: ```boruvkaparallel```. Run it by typing ```./boruvkaparallel``` in terminal. The program will then ask for a number as input, which would be the number of threads that the algorithm will use. Once this input is provided, it will continue to run and finish by itself, generating a series of graphs with different sizes and comparing the results from both the sequential and parallel solution. The output will look something like 
```
Enter number of threads: 4
MATCH(?) #Nodes	#Edges	Sequential time	Parallel time	#threads  Speedup
MATCH     100   600	    0.008614        0.005717        4         1.50673
MATCH 	  100   600	    0.008578        0.006097        4         1.40692
MATCH     100   600	    0.008435        0.004611        4         1.82932
MATCH     200   1200    0.032752        0.017808        4         1.83917
MATCH     200   1200    0.027258        0.010852        4         2.5118
MATCH     200   1200    0.030247        0.014398        4         2.10078
MATCH     400   2400    0.090859        0.043136        4         2.10634
MATCH     400   2400    0.123951        0.039333        4         3.15132
MATCH     400   2400    0.089912        0.032076        4         2.80309
MATCH     500   3000    0.175052        0.048373        4         3.6188
MATCH     500   3000    0.145857        0.035424        4         4.11793
MATCH     500   3000    0.131287        0.038624        4         3.3991
MATCH     700   4200    0.245696        0.075887        4         3.23766
MATCH     700   4200    0.241725        0.073397        4         3.29339
MATCH     700   4200    0.328799        0.086756        4         3.78993
MATCH     850   5100    0.399241        0.114474        4         3.48761
MATCH     850   5100    0.518437        0.122843        4         4.22032
MATCH     850   5100    0.564343        0.168829        4         3.34269
MATCH     1000  6000    0.772289        0.218541        4         3.53384
MATCH     1000  6000    0.685104        0.150132        4         4.56334
MATCH     1000  6000    0.595818        0.168208        4         3.54215

```

The executable runs both the sequential and the parallelized solution, compares their output and outputs information about the size of the graphs, the running time of the algorithms, the number of threads used, the speedup factor and whether the resulting MSTs from both implementations match.

The results in the table above are obtained by running the program on a dual core (with hyperthreading enabled) CPU.
