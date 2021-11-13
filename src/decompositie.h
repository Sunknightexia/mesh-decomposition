#ifndef DECOMPOSITE_H
#define DECOMPOSITE_H
class Decomposition{
public:
    vector<Vertex>* vertices;
    int nface;
    vector<glm::vec3> faces;
    float** distance;
    Decomposition(vector<Vertex>* vertices){
        this.vertices = vertices;
        
        this.nface = vertices->size()/3;

        this.distance = new float*[this.nface];
        for(int i=0;i<this.nface;i++){
            this.distance[i] = new float[this.nface];
        }
        this.calcDistance();
    }
    void calcDistance(){
        for(int i=0;i<this.nface;i++){
            
        }
    }
}
#endif