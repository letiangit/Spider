// A C++ program for Dijkstra's single source shortest path algorithm.
// The program is for adjacency matrix representation of the graph

#include <stdio.h>
#include <limits.h>
#include "ns3/spf.h"



// Number of vertices in the graph

// A utility function to find the vertex with minimum distance value, from
// the set of vertices not yet included in shortest path tree

namespace ns3 {
ShortPath::ShortPath ()
{
    
}

uint32_t 
ShortPath::minDistance(uint32_t dist[], bool sptSet[])
{
    // Initialize min value
    uint32_t min = INT_MAX, min_index;
    
    for (uint32_t v = 0; v < V; v++)
    if (sptSet[v] == false && dist[v] <= min)
    min = dist[v], min_index = v;
    
    return min_index;
}

// A utility function to print the constructed distance array
uint32_t *  
ShortPath::printSolution(uint32_t dist[],uint32_t PreNodes[V], uint32_t src)
{
    //std::cout << " src " << src << "  Vertex Distance from Source " << std::endl;
    uint32_t node;
    uint32_t nodePre;
    uint32_t nexthop;
    for (uint32_t i = 0; i < V; i++)
    {
       //printf("%d tt %d\n", i, dist[i]);
       if (src == i)
            continue;
        
        //std::cout <<  src << " -> " << i << ", dist "<< dist[i]  << std::endl;
        nexthop = SearchPath (PreNodes, src, i);
        //std::cout <<  " next hop " << nexthop  << std::endl;
        RoutingTable[i] = nexthop;
        
       // std::cout << " "  << std::endl;

    }
    return &RoutingTable[0];
}

// Funtion that implements Dijkstra's single source shortest path algorithm
// for a graph represented using adjacency matrix representation
uint32_t * 
ShortPath::dijkstra(uint32_t graphinput[V][V], uint32_t src)
{
    
    for (uint32_t ii = 0; ii < V; ii++)
    {
        for (uint32_t jj = 0; jj < V; jj++)
        {
            graph[ii][jj] = graphinput[ii][jj];
        }
    } 

    
    uint32_t dist[V];	 // The output array. dist[i] will hold the shortest
    // distance from src to i
    uint32_t PreNodes_test[V]={0};
    
    bool sptSet[V]; // sptSet[i] will true if vertex i is included in shortest
    // path tree or shortest distance from src to i is finalized
    
    // Initialize all distances as INFINITE and stpSet[] as false
    for (uint32_t i = 0; i < V; i++)
    dist[i] = INT_MAX, sptSet[i] = false;
    
    // Distance of source vertex from itself is always 0
    dist[src] = 0;
    
    uint32_t m = 0;
    uint32_t sequence[V]={0};
    
    // Find shortest path for all vertices
    for (uint32_t count = 0; count < V-1; count++)
    {
        // Pick the minimum distance vertex from the set of vertices not
        // yet processed. u is always equal to src in first iteration.
        uint32_t u = minDistance(dist, sptSet);
        
        // Mark the picked vertex as processed
        sptSet[u] = true;
        
         //std::cout << " m " << m << ", " << graph[10][0] << std::endl;
        
        // Update dist value of the adjacent vertices of the picked vertex.
        for (uint32_t v = 0; v < V; v++)
        
        // Update dist[v] only if is not in sptSet, there is an edge from
        // u to v, and total weight of path from src to v through u is
        // smaller than current value of dist[v]
        {
        //if (u != v && graph[u][v] == 0)
        //    { graph[u][v] = INT_MAX;} for uni 
            
        //if (!sptSet[v] && dist[u] != INT_MAX
        //    && dist[u]+graph[u][v] < dist[v] && graph[u][v] != INT_MAX && graph[u][v] != 0)
            
            
        if (!sptSet[v] && dist[u] != INT_MAX
            && dist[u]+graph[u][v] < dist[v] && graph[u][v] != INT_MAX )
            {
                dist[v] = dist[u] + graph[u][v];
                PreNodes_test[v] =  u;
                //std::cout << " src " << src << " v " << v << ", u "<< u <<  ",dist[v] " << dist[v] << ", m " << m << std::endl;
                //std::cout << " dist[u] " << dist[u] << " graph[u][v] " << graph[u][v]  << std::endl;

            }
        }
        
        m++;
    }
    
    // print the constructed distance array
    return printSolution(dist, PreNodes_test, src);
}


uint32_t 
ShortPath::SearchPath(uint32_t prev[V],uint32_t src, uint32_t dst) // v: src, u dst
{
	uint32_t que[V];
	uint32_t tot = 1;
	que[tot] = dst;
	tot++;
	uint32_t tmp = prev[dst];
	while(tmp != src)
	{
		que[tot] = tmp;
		tot++;
		tmp = prev[tmp];
	}
	que[tot] = src;
        /*
	for(uint32_t i=tot; i>=1; --i)
		if(i != 1)
			std::cout << que[i] << " -> ";
		else
			std::cout << que[i] << std::endl;
        */
        return que[tot-1]; // next hop
}

}


