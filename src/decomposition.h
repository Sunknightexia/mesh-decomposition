#ifndef DECOMPOSITION_H
#define DECOMPOSITION_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
using namespace std;
class Decomposition{
public:
    float etaConvex = 1.0;
    float etaConcave = 1.0;
    Decomposition(){
        cout<<"init the decomposition using parameter eta convex:"<<etaConvex<<"concave:"<<etaConcave<<endl;
    }
    Decomposition(float etaConvex, float etaConcave){
        this->etaConvex = etaConvex;
        this->etaConcave = etaConcave;
        cout<<"init the decomposition using parameter eta convex:"<<etaConvex<<"concave:"<<etaConcave<<endl;
    }
    float calcAngDistance(glm::vec3 begin, glm::vec3 end, glm::vec3 a1, glm::vec3 b1){
        glm::vec3 an = glm::normalize(glm::cross(end-begin,a1-end));
        glm::vec3 bn = -glm::normalize(glm::cross(end-begin,b1-end));
        // calc angle distance
        float eta;
        if(glm::dot(a1+b1-(begin+end),an+bn)<0)//convex
            eta = etaConvex;
        else//concave
            eta = etaConcave;
        float ang_dist = eta*(1-glm::dot(an,bn));

        return ang_dist;
    }
    float calcAngDistance(glm::vec3 an, glm::vec3 bn, glm::vec3 begin, glm::vec3 end, glm::vec3 a1, glm::vec3 b1){
        // calc angle distance
        float eta;
        if(glm::dot(a1+b1-(begin+end),an+bn)<0)//convex
            eta = etaConvex;
        else//concave
            eta = etaConcave;
        float ang_dist = eta*(1-glm::dot(an,bn));

        return ang_dist;
    }
    float calcGeoDistance(glm::vec3 begin, glm::vec3 end, glm::vec3 a1, glm::vec3 b1){
        // calc center of triangle
        glm::vec3 a0 = glm::vec3((begin.x+end.x+a1.x)/3,(begin.y+end.y+a1.y)/3,(begin.z+end.z+a1.z)/3);
        glm::vec3 b0 = glm::vec3((begin.x+end.x+b1.x)/3,(begin.y+end.y+b1.y)/3,(begin.z+end.z+b1.z)/3);
        // calc geo distance
        float a =glm::length(a0-begin);
        float b =glm::length(b0-begin);
        glm::vec3 c = glm::normalize(end-begin);
        float cost_1= glm::dot(glm::normalize(a0-begin),c);
        float cost_2= glm::dot(glm::normalize(b0-begin),c);
        float cost = cost_1*cost_2-sqrt(1-cost_1*cost_1)*sqrt(1-cost_2*cost_2);
        return sqrt(a*a+b*b-2*a*b*cost);
    }
    void floyd(float** distance, int** paths, unsigned int vn){
        // due to the graph is undirected graph, thus the distance can be calculated for half of matrix
        for(unsigned int jn=0;jn<vn;jn++){
            for(unsigned int i=0;i<vn;i++){
                for(unsigned int j=i+1;j<vn;j++){
                    if(distance[i][jn]!=-1 && distance[j][jn]!=-1){
                        float temp = distance[i][jn]+ distance[jn][j];
                        if(distance[i][j]==-1||temp<distance[i][j]){
                            distance[i][j] = temp;
                            distance[j][i] = temp;
                            paths[i][j] = jn;
                            paths[j][i] = jn;
                        }
                    }
                }
            }
        }
    }
};

#endif
