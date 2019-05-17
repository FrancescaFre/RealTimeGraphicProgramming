#pragma once //Serve per farlo caricare da uno solo questo file

using namespace std;

#include <glad/glad.h>

//header per il sistema di metrica, glm serve per specificare mesh usando la stessa convenzione di metriche
#include <glm/glm.hpp>
//assimp è una lib opensource per caricare mesh dal disco, fornisce anche tools per fare operazioni sulla struttura dati caricata da disco
#include <assimp/importer.hpp>
#include <assimp/scene.hpp>
#include <assimp/postprocess.hpp>

#include <utils/mesh.h>

class Model
{
    public:
        vector<Mesh> meshes;

        //const con il path da cui devo caricare il dato
        Model(const string& path);
        {
            this->loadModel(path);
        }

        void Draw(Shader shader)
        {
            for(GLuint i=0; i<this->meshes.size(); i++)
                this->mesh[i].Draw(shader);
        }

        virtual ~Model() //tilde alt+126 //copia da michael
        {
            //il metodo viene chiamato automaticamente dopo la fine dell'applicazione
            for(GLuint i=0; i<this->meshes.size(); i++)
                this->mesh[i].Delete();
        }

    private:
        void loadModel(string path)
        {
            Assimp::importer importer;
            const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JonIdenticalVertices |
                                                    aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace)
                                                    //quelli dopo path sono flags
            //Triangulate: se la mash è composta da quad, li converte in triangoli
            //JonIdenticalVertices: se per qualche ragione la mseh ha dei vertici doppi (due con la stessa posizione) si mergiano
            //FlipUVs: librarie differenti gestiscono in moodo diverso le convenizoni delle UV
            //GenSmoothNormals: per avere le normali che siano coerenti con quelle vicine (sopratutto dopo i processi di triangulation)
            //CalcTangentSpace: calcola tangent e cotangent per ogni vertice. Sono calcolati usando le coordinate UV per determinare
              //la direzione del tangent e cotangen, bisogna fare attenzione perchè non si può accedere al tangent e cotangent perchè
              // non vengono calcolate se le UV coordinate non erano nella mesh originale (non avvisa in caso di fallimento nel calcolo

            //ora dentro scene ho il modello

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
              cout<- "ASSIMP ERROR: "<<importer.GetErrorString()<<endl;
              return;
            }

            //se invece tutto va bene, ora si fanno coseh
            this-> processNode(scene->mRootNode, scene);
          }

      void processNode(aiNode*node, const aiScene* scene)
      {
        for (GLuint i=0; i < node->mNumMaeshes; i++)
        {
            //per ogni nodo all'interno della struttura dati
            aiMesh*mesh =scene->mMeshes[node->mMeshes[i]];
            //in scene ho un array di mmeshes che contiene ogni meshes
            //in node ...boh

            this->meshes.push_back(this->processMesh(mesh, scene)); //dove meshes è un vettore di mesh
            //process mesh mi restituirà l'istanza della mesh e questa verrà messa dentro il vettore di meshes
        }
        for (GLuint i=0; i<node->mNumChildren; i++)
            //chiamata ricorsiva
            this->processNode(node->mChildren[i], scene)

      }

      Mesh processMesh(aiMesh* mesh, const aiScene*scene)
      {
          //si creano due vettori
          vector <Vertex> verticies;
          vector <GLuint> indices;

        for (GLuint i=0; i<mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector //mi serve per assegnare per ogni compontene del vettore di assimp a quello di glm
                            //è il problema delle lib a basso livello che usano strutture dati differenti, quindi sono da convertire

            //conversione per la posizione dei vertici
            vector.x=mesh->mVertices[i].x;
            vector.y=mesh->mVertices[i].y;
            vector.z=mesh->mVertices[i].z;
            vertex.Position = vector;

            //conversione per il valore delle normali
            vector.x=mesh->mNormals[i].x;
            vector.y=mesh->mNormals[i].y;
            vector.z=mesh->mNormals[i].z;
            vertex.Normal = vector;

            if(mesh->mTextureCoords[0]) //assumo in questo caso di aver una sola mesh, quindi controllo che esiste
            {                           //se non vengono calcolate le tangent e bitangent da errore
              glm::vec2 vec;
              vec.x = mesh->mTextureCoords[0][i].x;
              vec.x = mesh->mTextureCoords[0][i].y;
              vertex.TexCoords = vec;

              vector.x=mesh->mTangents[i].x;
              vector.y=mesh->mTangents[i].y;
              vector.z=mesh->mTangents[i].z;
              vertex.Tangent = vector;

              vector.x=mesh->mBiangents[i].x;
              vector.y=mesh->mBitangents[i].y;
              vector.z=mesh->mBitangents[i].z;
              vertex.Bitangent = vector;
            }
            else
            {
              //non voglio terminare l'applicazione, ma inizializzo le texture cooridnate a zero e invio un messaggio
              vertex.TexCoords = glm::vec2(0.0f, 0.0f);
              cout << "WARNING NO UV for tangent calc"<<endl;
            }
            verticies.push_back(vertex);
        }

        //La conversione è terminata da ssimp a glm
        //ora necessito di una indicizzazione delle face de
        for(GLuint i=0; i<mesh->mNumFaces; i++)
        {
          aiFace face = mesh ->mFaces[i];
          for(GLuint j=0; j<face->nNumIndices; j++)
            indices.push_back(face.mIndices[j]);
        }

        //ho creato due vettori, uno con i vertici e uno con le facce ideale per i buffer di OpenGL
        return Mesh(vertices, indices);
      }
}
