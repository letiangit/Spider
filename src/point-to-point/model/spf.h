// A C++ program for Dijkstra's single source shortest path algorithm.
// The program is for adjacency matrix representation of the graph

#include <stdio.h>
#include <limits.h>


#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"

// Number of vertices in the graph

#ifndef SPF_H
#define SPF_H

namespace ns3 
{
    class ShortPath 
{
public:
    //#define V 22
    #define V 24 //two GES station
    uint32_t graph[V][V];

    ShortPath ();
    uint32_t minDistance(uint32_t dist[], bool sptSet[]);
    uint32_t *   printSolution(uint32_t dist[], uint32_t preNode[], uint32_t n);
    uint32_t *  dijkstra(uint32_t graph[V][V], uint32_t src);
    uint32_t RoutingTable[V];

};
}
#endif



