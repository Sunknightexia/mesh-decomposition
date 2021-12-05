#ifndef NETFLOW_H
#define NETFLOW_H
#include <vector>
#include <list>
#include <stack>//using for dfs
#include <queue>//using for bfs
using namespace std;
class Node{
public:
    bool visited;// dfs时是否查找到
    float deltaflow;//可增加的最大流
    int parent;//来自哪里的增流路径
};
class FordFulkerson{
public:
    float** distance;
    float** flow;
    float** capacity;
    vector<vector<int>> edges;
    Node* nodes;
    int N;
    FordFulkerson(float** distance, int N){
        this->N = N;
        this->distance = distance;
        this->flow  = new float*[N];
        this->capacity =  new float*[N];
        this->edges.resize(N);
        this->nodes = new Node[N];
        for(unsigned int i=0;i<N;i++){
            this->flow[i] = new float[N];
            this->capacity[i] = new float[N];
            for(unsigned int j=0;j<N;j++){
                if(distance[i][j]!=-1){
                    this->flow[i][j] = 0;
                    this->capacity[i][j] = distance[i][j];
                    this->edges[i].push_back(j);
                }
            }
        }
    }
    void init(){
        for(int i=0;i<this->N;i++){
            this->nodes[i].visited = false;
            this->nodes[i].deltaflow = 0;
            this->nodes[i].parent = -1;
        }
    }
    bool findway(int s, int d){
        // 寻找增流路径
        queue<int> Q;
        Q.push(s);
        this->nodes[s].deltaflow = 1e10;
        this->nodes[s].visited = true;
        while(Q.size()>0){
            int cur = Q.front();
            Q.pop();
            if(cur==d){
                break;
            }
            for(int i=0;i<this->edges[cur].size();i++){
                int v = this->edges[cur][i];
                if(!this->nodes[v].visited && this->capacity[cur][v]>0){
                    Q.push(v);
                    this->nodes[v].visited = true;
                    this->nodes[v].deltaflow = min(this->capacity[cur][v],this->nodes[cur].deltaflow);
                    this->nodes[v].parent = cur;
                }
            }
        }
        return this->nodes[d].visited;
    }
    void updateway(int s, int d){
        //增流
        int v = d;
        int minflow = this->nodes[v].deltaflow;
        while(v!=s){
            int parent = this->nodes[v].parent;
            this->flow[parent][v] += minflow;
            this->capacity[parent][v] -= minflow;
            this->capacity[v][parent] += minflow;
            v = parent;
        }
    }
    void ff(int s, int d){
        init();
        while(findway(s, d)){
            updateway(s,d);
            init();
        }
    }
    void print(int s, int d){
        for(int i=0;i<N;i++){
            for(int j=0;j<this->edges[i].size();j++){
                cout<<i<<"->"<<this->edges[i][j]<<":"<<this->flow[i][this->edges[i][j]]<<endl;
            }
        }
    }
    vector<int> cut(int s, int d){
        //从s出发，用bfs寻找对应的分割部分
        init();
        vector<int> spart;
        queue<int> Q;
        Q.push(s);
        this->nodes[s].visited = true;
        while(Q.size()>0){
            int cur = Q.front();
            Q.pop();
            spart.push_back(cur);
            if(cur==d){
                break;
            }
            for(int i=0;i<this->edges[cur].size();i++){
                int v = this->edges[cur][i];
                if(!this->nodes[v].visited && this->capacity[cur][v]!=0&&this->flow[cur][v]>0){
                    Q.push(v);
                    this->nodes[v].visited = true;
                    this->nodes[v].parent = cur;
                }
            }
        }
        return spart;
    }
};
#endif