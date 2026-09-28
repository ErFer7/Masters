#ifndef INPUT_H
#define INPUT_H

namespace Dijkstra {

#define NUM_NODES 100
#define NONE 9999
#define OUT_OF_MEMORY -1
#define QUEUE_SIZE 1000

struct _NODE {
    int dist;
    int prev;
};

struct _QITEM {
    int node;
    int dist;
    int prev;
    struct _QITEM *next;
};

class Dijkstra {
   public:
    Dijkstra();

    ~Dijkstra() = default;

    int run();

   private:
    inline void reset() { dijkstra_checksum = 0; }

    int dijkstra_return(void);
    int dijkstra_enqueue(int node, int dist, int prev);
    void dijkstra_dequeue(int *node, int *dist, int *prev);
    int dijkstra_qcount(void);
    int dijkstra_find(int chStart, int chEnd);

   private:
    _NODE dijkstra_rgnNodes[NUM_NODES];

    int dijkstra_queueCount;
    int dijkstra_queueNext;
    _QITEM *dijkstra_queueHead;
    _QITEM dijkstra_queueItems[QUEUE_SIZE];

    int dijkstra_checksum = 0;
    unsigned char dijkstra_AdjMatrix[NUM_NODES][NUM_NODES];
    static const unsigned char dijkstra_AdjMatrix_data[NUM_NODES][NUM_NODES];
};
}  // namespace Dijkstra

#endif /* INPUT_H */
