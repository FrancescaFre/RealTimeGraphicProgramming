
#pragma once

using namespace std;

//standard lib per la gestione del testo e da file esterni
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

//lib che serve per la gestione del profile dell'applicazione e caricare l'estensionc he abbiamo bisogno (?????) 
//quindi prendere le lib che sono utili e non tutte
#include <glad/glad.h>

class Shader
{
    //costruttore, responsabile del caricamento e della creazione del programma di shading (compile) 
    public: 
        GLuint Program;
        
        //costruttore che indica dove trovare e caricare le informazioni per lo shader
        Shader(const GLchar* vertexPath, const GLchar* fragmentPath)
        {   
            //indicano dove verrà salvato il codice dello shader
            string vertexCode; 
            string fragmentCode;

            //leggo da un file, converto questo in string, poi in string GL --> puntatore a char per un array di char == stringa 
            ifstream vShaderFile; 
            ifstream fShaderFile; 
        
            //per l'uso delle classi di stream devo usare una specie di try-catch per gli errori, es file corrotto o fallimento della lettura del file
            vShaderFile.exceptions(ifstream::failbit | ifstream::badbit)
            fShaderFile.exceptions(ifstream::failbit | ifstream::badbit)
            try
            {
                vShaderFile.open(vertexPath);
                fShaderFile.open(fragmentPath);
                
                //creo due var local stream string 
                strinstream vShaderStream, fShaderStream;
                vShaderStream << vShaderFile.rdnuf();
                fShaderStream << fShaderFile.rdnuf();
                
                //ora che abbiamo letto i contenuti dei file e salvati, si chiudono gli stream dei file
                vShaderFile.close();
                fShaderFile.close();
                
                //converto in stringa
                vertexCode = vShaderStream.str();
                fragmentCode = fShaderStream.srt();
            }
            catch (ifstream::failure e)
            {
                //in caso andasse male qualche procedura
                cout<<"++++ERROR! nella lettura dei file++++"<< endl; //endl: end-line 
            }
            
            //convertire in puntatore GLchar da stringa 
            const GLChar* vShaderCode = vertexCode.c_str();
            const GLChar* fShaderCode = fragmentCode.c_str();
            
            //----------------- Fine caricamento da disco
            
            //------------------ Si comincia con gli shaders
            GLuint vertex, fragment;
           
            vertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex, 1, &vShaderCode, NULL);     
            glCompileShader(vertex);  //compila a runtime lo shader
       
            //faccio un metodo per checkare i compile errors perchè è da fare 3 volte: per fragmen, shader e programma
            checkCompileErrors(vertex, "VERTEX");
            
            //########### Ripeto per il fragment
            fragment = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment, 1, &fShaderCode, NULL);     
            glCompileShader(fragmen
            t);  //compila a runtime lo shader
            checkCompileErrors(fragment, "FRAGMENT");
        
            //########## Ripeto per il PROGRAM
            //linking shaders
            this->Program = glCreateProgram();
            glAttachShader( this->Program, vertex);
            glAttachShader( this->Program, fragment);
            glLinkProgram( this->Program);
            checkCompileErrors(this->Program, "PROGRAM");
            
            //cancello i fragment e vertex perchè dopo la compilazione è già tutto caricato e ora si elimina il resto 
            //perchè non sono più necessari
            glDeleteShader(vertex);
            glDeleteShader(fragment);

            //---------------TERMINATO IL COSTRUTTORE
        }    
        
        void Use()
        {
            //con questa chiamata, ogni cosa reinderizzata con un determinato shader 
            glUseProgram(this-> Program);
            //da quello che ho capito, PROGRAM è la variabile che contiene tutte le info di vertex e fragment COMPILATE, 
            //quindi OGNI VOLTA CHE SI USNO METODI PER DISEGNARE A SCHERMO si usano quelli
        }
        
        void Delete()
        {
            glDeleteProgram(this->Program);
        }

    private: 
        void checkCompileErorrs(GLuint shader, string type)
        {
            GLint success; 
            GLchar infoLog[1024];
            
            if(type!= "PROGRAM")
                    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); 
                    if (!success)
                    {
                        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                        std::cout << "ERROR::SHADER-COMPILATION_ERROR_of_type: " << type << "\n"<< infoLog << "\n" << std::endl;
                    }
            
            else{
                //glGetProgramiv: si chiama questo metodo per vedere se il linking è avvenuto con successo, è legato al link status
                 glGetProgramiv (shader, GL_LINK_STATUS, &success); 
                    if (!success)
                    {
                        glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                        std::cout << "ERROR::PROGRAM-LINKING_ERROR_of_type: " << type << "\n"<< infoLog << "\n" << std::endl;
                    }
            }
        }
    
    

}


