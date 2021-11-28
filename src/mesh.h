

#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.h"
#include "decomposition.h"
#include <string>
#include <vector>
#include <map>
using namespace std;

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    // position
    glm::vec3 Position;
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
    float ** weights;
    int** paths;
    float sum_angD, sum_geoD;
    Decomposition decompositionMachine = Decomposition(0.2,1.0);
    void initWeights(){
        unsigned int N = this->faces.size();
        weights = new float*[N];
        paths = new int*[N];
        for(unsigned int i=0;i<N;i++){
            weights[i] = new float[N];
            paths[i] = new int[N];
        }
        sum_angD = 0;
        sum_geoD = 0;
        for(unsigned int i=0;i<N;i++){
            this->weights[i][i] = 0;
            paths[i][i] = i;
            for(unsigned int j=i+1;j<N;j++){
                this->weights[i][j] = -1;
                this->weights[j][i] = -1;
                this->paths[i][j] = j;
                this->paths[j][i] = i;
            }
        }
        for(unsigned int i=0;i<edge2face->size();i++){
            for(map<unsigned int, Edge>::iterator it=edge2face->at(i).begin();it != edge2face->at(i).end();it++){
                unsigned int j = it->first;
                unsigned int left = it->second.left;
                unsigned int right = it->second.right;
                unsigned int leftv = it->second.leftv;
                unsigned int rightv = it->second.rightv;
                it->second.ang_d = decompositionMachine.calcAngDistance(faces[left].Normal,faces[right].Normal,vertices[i].Position,vertices[j].Position,vertices[left].Position,vertices[right].Position);
                it->second.geo_d = decompositionMachine.calcGeoDistance(vertices[i].Position,vertices[j].Position,vertices[leftv].Position,vertices[rightv].Position);
                weights[left][right] = it->second.ang_d+it->second.geo_d;
                weights[right][left] = weights[left][right];
            }
        }
    }
    void calcWeights(){
        decompositionMachine.floyd(weights, paths, this->vertices.size());
        // unsigned int N = this->faces.size();
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

