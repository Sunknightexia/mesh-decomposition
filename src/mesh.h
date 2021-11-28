

#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.h"
#include "decomposition.h"
#include "netflow.h"
#include <string>
#include <vector>
#include <map>
#include <set>
using namespace std;

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    // position
    glm::vec3 Position;
    // colors
    glm::vec3 Color;
    // normal
    glm::vec3 Normal;
    // texCoords
    glm::vec2 TexCoords;
    // tangent
    glm::vec3 Tangent;
    // bitangent
    glm::vec3 Bitangent;
	//bone indexes which will influence this vertex
	int m_BoneIDs[MAX_BONE_INFLUENCE];
	//weights from each bone
	float m_Weights[MAX_BONE_INFLUENCE];
};
struct Edge {
    // face indice of left and right
    unsigned int left;
    unsigned int right;
    // vertice indice of left and right
    unsigned int leftv;
    unsigned int rightv;
    float ang_d;
    float geo_d;
};
struct Face {
    glm::vec3 Normal;
    unsigned int type;
};
struct Texture {
    unsigned int id;
    string type;
    string path;
};

class Mesh {
public:
    // mesh Data
    vector<Vertex>       vertices;
    vector<Face> faces;
    vector<unsigned int> indices;
    vector<Texture>      textures;
    unsigned int VAO;
    // edge map to face; distance of faces
    vector<map<unsigned int, Edge>>* edge2face;
    // face map to edge; used for 最小割
    int** face2edgeb;
    int** face2edgee;
    float** face2edgew;
    float ** weights;
    int** paths;
    float sum_angD, sum_geoD;
    float** probs;
    int * typeindex;
    float avgAng_d;
    float avgGeo_d;
    // fuzzy matrix
    float** fuzzycap;
    vector<int> C;
    vector<int> A;
    vector<int> B;
    vector<int> total;
    Decomposition decompositionMachine = Decomposition(0.2,1.0);
    void initWeights(){
        unsigned int N = this->faces.size();
        weights = new float*[N];
        paths = new int*[N];
        face2edgeb = new int*[N];
        face2edgee = new int*[N];
        face2edgew = new float*[N];
        for(unsigned int i=0;i<N;i++){
            weights[i] = new float[N];
            paths[i] = new int[N];
            face2edgeb[i] = new int[N];
            face2edgee[i] = new int[N];
            face2edgew[i] = new float[N];
        }
        sum_angD = 0;
        sum_geoD = 0;
        for(unsigned int i=0;i<N;i++){
            this->weights[i][i] = 0;
            paths[i][i] = i;
            face2edgeb[i][i] = -1;
            face2edgee[i][i] = -1;
            for(unsigned int j=i+1;j<N;j++){
                this->weights[i][j] = -1;
                this->weights[j][i] = -1;
                this->paths[i][j] = j;
                this->paths[j][i] = i;
                face2edgeb[i][j] = -1;
                face2edgeb[j][i] = -1;
                face2edgee[i][j] = -1;
                face2edgee[j][i] = -1;
                face2edgew[i][j] = -1;
                face2edgew[j][i] = -1;
            }
        }
        avgAng_d = 0;
        avgGeo_d = 0;
        int adjacentN = 0;
        for(unsigned int i=0;i<edge2face->size();i++){
            for(map<unsigned int, Edge>::iterator it=edge2face->at(i).begin();it != edge2face->at(i).end();it++){
                unsigned int j = it->first;
                unsigned int left = it->second.left;
                unsigned int right = it->second.right;
                unsigned int leftv = it->second.leftv;
                unsigned int rightv = it->second.rightv;
                it->second.ang_d = decompositionMachine.calcAngDistance(faces[left].Normal,faces[right].Normal,vertices[i].Position,vertices[j].Position,vertices[left].Position,vertices[right].Position);
                it->second.geo_d = decompositionMachine.calcGeoDistance(vertices[i].Position,vertices[j].Position,vertices[leftv].Position,vertices[rightv].Position);
                avgAng_d += it->second.ang_d;
                avgGeo_d += it->second.geo_d;
                adjacentN += 1;

                face2edgeb[left][right] = leftv;
                face2edgee[left][right] = rightv;
                face2edgeb[right][left] = leftv;
                face2edgee[right][left] = rightv;
                face2edgew[left][right] = it->second.ang_d;
                face2edgew[right][left] = it->second.ang_d;
            }
        }
        avgAng_d /= adjacentN;
        avgGeo_d /= adjacentN;
        float delta = 0.5;
        for(unsigned int i=0;i<edge2face->size();i++){
            for(map<unsigned int, Edge>::iterator it=edge2face->at(i).begin();it != edge2face->at(i).end();it++){
                unsigned int left = it->second.left;
                unsigned int right = it->second.right;
                weights[left][right] = (1-delta)*it->second.ang_d/avgAng_d+delta*it->second.geo_d/avgGeo_d;
                weights[right][left] = weights[left][right];
            }
        }
    }
    void calcWeights(){
        unsigned int N = this->faces.size();
        decompositionMachine.floyd(weights, paths, N);
        // for(unsigned int i=0;i<N;i++){
        //     for(unsigned int j=0;j<N;j++){
        //         cout<<weights[i][j]<<";"<<paths[i][j]<<",";
        //     }
        //     cout<<endl;
        // }
    }
    // constructor
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures, vector<Face> faces)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        this->faces = faces;

        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setupMesh();
    }

    // render the mesh
    void Draw(Shader &shader) 
    {
        // bind appropriate textures
        unsigned int diffuseNr  = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr   = 1;
        unsigned int heightNr   = 1;
        for(unsigned int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
            // retrieve texture number (the N in diffuse_textureN)
            string number;
            string name = textures[i].type;
            if(name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if(name == "texture_specular")
                number = std::to_string(specularNr++); // transfer unsigned int to string
            else if(name == "texture_normal")
                number = std::to_string(normalNr++); // transfer unsigned int to string
             else if(name == "texture_height")
                number = std::to_string(heightNr++); // transfer unsigned int to string

            // now set the sampler to the correct texture unit
            glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
            // and finally bind the texture
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        
        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);
    }
    void initProbs(unsigned int typen=2){
        unsigned int N = this->faces.size();
        int maxi=0;
        int maxj = 0;
        float length =0;
        probs = new float*[typen];
        for(unsigned int i=0;i<typen;i++){
            probs[i] = new float[N];
        }
        for(unsigned int i=0;i<N;i++){
            for(unsigned int j=i;j<N;j++){
                if(weights[i][j]>length){
                    length = weights[i][j];
                    maxi = i;
                    maxj = j;
                }
            }
        }
        typeindex = new int[typen];
        typeindex[0] = maxi;
        typeindex[1] = maxj;
        // cout<<maxi<<" "<<maxj<<endl;
        for(unsigned int i=0;i<N;i++){
            float probsi_total =0;
            int maxtype = 0;
            float maxprob = 0;
            for(unsigned int j=0;j<typen;j++){
                probsi_total += 1/weights[typeindex[j]][i];
                if((1/weights[typeindex[j]][i])>maxprob){
                    maxprob = 1/weights[typeindex[j]][i];
                    maxtype = j;
                }
            }
            faces[i].type = maxtype;
            for(unsigned int j=0;j<typen;j++){
                probs[j][i] = 1/weights[typeindex[j]][i]/probsi_total;
                // cout<< weights[typeindex[j]][i]<<" ";
            }
            // cout<<maxtype<<endl;
        }
    }
    void prob(float** probs,unsigned int typen){
        unsigned int N = this->faces.size();
        for(unsigned int i=0;i<N;i++){
            float probsi_total =0;
            int maxtype = 0;
            float maxprob = 0;
            for(unsigned int j=0;j<typen;j++){
                probsi_total += 1/weights[typeindex[j]][i];
                if((1/weights[typeindex[j]][i])>maxprob){
                    maxprob = 1/weights[typeindex[j]][i];
                    maxtype = j;
                }
            }
            faces[i].type = maxtype;
            for(unsigned int j=0;j<typen;j++){
                probs[j][i] = 1/weights[typeindex[j]][i]/probsi_total;
                // cout<< weights[typeindex[j]][i]<<" ";
            }
            // cout<<maxtype<<endl;
        }
    }
    void rep(float** probs, unsigned int typen, unsigned int times=0){
        unsigned int N = this->faces.size();
        float delta = 0.2;
        unsigned int* tmptypeindex= new unsigned int[typen];
        float* tmpweights= new float[typen];
        // move the seed
        for(unsigned int t=0;t<1000;t++){
            for(unsigned int j=0;j<typen;j++){
                tmptypeindex[j] = typeindex[j];
                tmpweights[j] = 1e10;
                for(unsigned int i=0;i<N;i++){
                    if(probs[j][i]>0.5+delta){
                        float w = weightsProb(i,j);
                        if(w<tmpweights[j]){
                            tmpweights[j] = w;
                            tmptypeindex[j] = i;
                        }
                    }
                }
            }
            unsigned int difference = 0;
            for(unsigned int j=0;j<typen;j++){
                if(tmptypeindex[j]!=typeindex[j]){
                    difference += 1;
                    typeindex[j] = tmptypeindex[j];
                }
            }
            if(difference>0){
                prob(probs,typen);
            }else{
                break;
            }
        }
        if(times==1000){
            cout<<"iteration limited"<<endl;
            return;
        }
    }
    float weightsProb(unsigned int facei, unsigned int typei){
        unsigned int N = this->faces.size();
        float w = 0;
        for(unsigned int i=0;i<N;i++){
            w += probs[typei][i]*weights[facei][i];
        }
        return w;
    }
    void fuzzyConstruct(float delta=0.1){
        unsigned int N = this->faces.size();
        set<unsigned int> As;
        set<unsigned int> Bs;
        
        for(unsigned int i=0;i<N;i++){
            if(probs[0][i]>(0.5-delta)&&probs[0][i]<(0.5+delta)){
                C.push_back(i);
                for(unsigned int j=0;j<3;j++){
                    unsigned int begin = indices[i*3+j];
                    unsigned int end = indices[i*3+(j+1)%3];
                    bool findleft = true;
                    if (begin>end){
                        begin,end = end, begin;
                        findleft = false;
                    }
                    unsigned int left;
                    if(edge2face->at(begin).count(end)>0){
                        if(findleft){
                            left = edge2face->at(begin)[end].left;
                        }else{
                            left = edge2face->at(begin)[end].right;
                        }
                        if(probs[0][left]>0.5+delta){
                            As.insert(left);
                        }else if(probs[0][left]<0.5-delta){
                            Bs.insert(left);
                        }
                    }
                }
            }
        }
        total.push_back(-1);
        for(set<unsigned int>::iterator it=As.begin();it!=As.end();it++){
            A.push_back(*it);
            total.push_back(*it);
        }
        for(unsigned int j=0;j<C.size();j++){
            total.push_back(C.at(j));
        }
        for(set<unsigned int>::iterator it=Bs.begin();it!=Bs.end();it++){
            B.push_back(*it);
            total.push_back(*it);
        }
        int fuzzysize = A.size()+B.size()+C.size()+2;
        fuzzycap = new float*[fuzzysize];
        for(unsigned int i=0;i<fuzzysize;i++){
            fuzzycap[i] = new float[fuzzysize];
            for(unsigned int j=0;j<fuzzysize;j++){
                fuzzycap[i][j] = 0;
            }
        }
        total.push_back(-2);
        for(unsigned int i=0;i<(A.size());i++){
            // 0 is the source
            fuzzycap[0][i+1] = 1e10;
            fuzzycap[i+1][0] = 1e10;
            for(unsigned int j=0;j<C.size();j++){
                if(face2edgeb[A.at(i)][C.at(j)]!=-1){
                    fuzzycap[i+1][A.size()+1+j] = 1/(1+face2edgew[A.at(i)][C.at(j)]/avgAng_d);
                    fuzzycap[A.size()+1+j][i+1] = fuzzycap[i+1][A.size()+1+j];
                }
            }
        }
        unsigned int Boffset = A.size()+C.size();
        unsigned int Coffset = A.size();
        for(unsigned int i=0;i<(B.size());i++){
            // fuzzysize-1 is the dst
            fuzzycap[fuzzysize-1][Boffset+i+1] = 1e10;
            fuzzycap[Boffset+i+1][fuzzysize-1] = 1e10;
            for(unsigned int j=0;j<C.size();j++){
                if(face2edgeb[B.at(i)][C.at(j)]!=-1){
                    fuzzycap[Boffset+i+1][Coffset+1+j] = 1/(1+face2edgew[B.at(i)][C.at(j)]/avgAng_d);
                    fuzzycap[Coffset+1+j][Boffset+i+1] = fuzzycap[Boffset+i+1][Coffset+1+j];
                }
            }
        }
        for(unsigned int i=0;i<C.size();i++){
            for(unsigned int j=i+1;j<C.size();j++){
                if(face2edgeb[C.at(i)][C.at(j)]!=-1){
                    fuzzycap[Coffset+i+1][Coffset+1+j] = 1/(1+face2edgew[C.at(i)][C.at(j)]/avgAng_d);
                    fuzzycap[Coffset+1+j][Coffset+i+1] = fuzzycap[Coffset+i+1][Coffset+1+j];
                }
            }
        }
    }
    void saveFuzzy(string output){
        fuzzyConstruct(0.1);
        vector<int> cut = fordFulkson(fuzzycap, A.size()+B.size()+C.size()+2);
        for(int i=1;i<total.size()-1;i++){
            faces[total.at(i)].type = 1;
        }
        for(int i=0;i<cut.size();i++){
            faces[total.at(cut.at(i))].type = 0;
        }
        vector<glm::vec3> colors;
        colors.push_back(glm::vec3(1,0,0));
        colors.push_back(glm::vec3(0,0,1));

        ofstream fopt(output);
        // store vertice of faces and color
        unsigned int N = this->faces.size();
        for(unsigned int i=0;i<N;i++){
            glm::vec3 color =  colors[faces[i].type];
            for(unsigned int j=0;j<3;j++){
                glm::vec3 pos = vertices[indices[i*3+j]].Position;
                fopt<<"v "<<pos.x<<" "<<pos.y<<" "<<pos.z<<" "<<color.x<<" "<<color.y<<" "<<color.z<<endl;
            }
        }
        for(unsigned int i=0;i<N;i++){
            // fopt<<"f "<<indices[i*3]<<" "<<indices[i*3+1]<<" "<<indices[i*3+2]<<endl;
            fopt<<"f "<<i*3+1<<" "<<i*3+2<<" "<<i*3+3<<endl;

        }

    }
    void saveAs(string output){
        initProbs(2);
        rep(probs,2);
        vector<glm::vec3> colors;
        colors.push_back(glm::vec3(1,0,0));
        colors.push_back(glm::vec3(0,0,1));

        ofstream fopt(output);
        // store vertice of faces and color
        unsigned int N = this->faces.size();
        for(unsigned int i=0;i<N;i++){
            glm::vec3 color =  colors[faces[i].type];
            for(unsigned int j=0;j<3;j++){
                glm::vec3 pos = vertices[indices[i*3+j]].Position;
                fopt<<"v "<<pos.x<<" "<<pos.y<<" "<<pos.z<<" "<<color.x<<" "<<color.y<<" "<<color.z<<endl;
            }
        }
        for(unsigned int i=0;i<N;i++){
            // fopt<<"f "<<indices[i*3]<<" "<<indices[i*3+1]<<" "<<indices[i*3+2]<<endl;
            fopt<<"f "<<i*3+1<<" "<<i*3+2<<" "<<i*3+3<<endl;

        }
        // store faces
    }
private:
    // render data 
    unsigned int VBO, EBO;

    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
		// ids
		glEnableVertexAttribArray(5);
		glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));

		// weights
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
        glBindVertexArray(0);
    }
};
#endif

