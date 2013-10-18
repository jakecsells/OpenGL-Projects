#include <GL/glew.h> // glew must be included before the main gl libs
#include <GL/glut.h> // doing otherwise causes compiler shouting
#include <iostream>
#include <chrono>
#include <fstream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp> //Makes passing matrices to shaders easier

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include <time.h>

//Including texture stuff
#include <IL/il.h>
#include <map>


using namespace std;


//--Data types
//This object will define the attributes of a vertex(position, color, etc...)
struct Vertex
{
    GLfloat position[3];
    GLfloat color[3];
    GLfloat uv[2];
};

//--Evil Global variables
//Just for this example!
int w = 640, h = 480;// Window size
GLuint program;// The GLSL program handle
GLuint vbo_geometry;// VBO handle for our geometry
const char* VERTEX_SHADER = "../bin/shader.vert";
const char* FRAGMENT_SHADER = "../bin/shader.frag";
float factor;
GLuint* textureId;

std::vector<float> g_vp, g_vt, g_vn;

int g_point_count = 0;
//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader

//attribute locations
GLint loc_position;
GLint loc_color;
GLint loc_uv;
GLint gSampler;

//transform matrices
glm::mat4 model;//obj->world each object should have its own model matrix
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 mvp;//premultiplied modelviewprojection

//--GLUT Callbacks
void render();
void update();
void reshape(int n_w, int n_h);
void keyboard(unsigned char key, int x_pos, int y_pos);
void arrows(int key, int x_pos, int y_pos);

//--Function Prototypes
char* loadShader( const char* filename );
bool loadOBJ(char*, Vertex*&);
bool loadTexture(char * );
unsigned int geometrySize;
char* filename;
char* textName;
bool quad = false;
float scaler = 1.;

//--Resource management
bool initialize();
void cleanUp();

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;


//--Main
int main(int argc, char **argv)
{

    if(argc !=4){
      cout << "ERROR: Input to command line incorrect. Aborting." << endl;
      return 0;
    }
    filename = argv[argc-3];
    textName = argv[argc-2];
    factor = atof(argv[argc-1]);

    // Initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(w, h);
    // Name and create the Window
    glutCreateWindow("Our Beautiful Program");

    // Now that the window is created the GL context is fully set up
    // Because of that we can now initialize GLEW to prepare work with shaders
    GLenum status = glewInit();
    if( status != GLEW_OK)
    {
        std::cerr << "[F] GLEW NOT INITIALIZED: ";
        std::cerr << glewGetErrorString(status) << std::endl;
        return -1;
    }

    // Set all of the callbacks to GLUT that we need
    glutDisplayFunc(render);// Called when its time to display
    glutReshapeFunc(reshape);// Called if the window is resized
    glutIdleFunc(update);// Called if there is nothing else to do
    glutKeyboardFunc(keyboard);// Called if there is keyboard input

    // Initialize all of our resources(shaders, geometry)
    bool init = initialize();
    if(init)
    {
        t1 = std::chrono::high_resolution_clock::now();
        glutMainLoop();
    }

    // Clean up after ourselves
    cleanUp();
    return 0;
}

//--Implementations
void render()
{
    //--Render the scene

    //clear the screen
    glClearColor(0.0, 0.0, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //premultiply the matrix for this example
    mvp = projection * view * model;

    // //enable the shader program
    glUseProgram(program);

    // //upload the matrix to the shader
    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp));

    //set up the Vertex Buffer Object so it can be drawn
    glEnableVertexAttribArray(loc_position);
    glEnableVertexAttribArray(loc_uv);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    //set pointers into the vbo for each of the attributes(position and color)
    glVertexAttribPointer( loc_position,//location of attribute
                           3,//number of elements
                           GL_FLOAT,//type
                           GL_FALSE,//normalized?
                           sizeof(Vertex),//stride
                           0);//offset

    glVertexAttribPointer( loc_uv,
                           2,
                           GL_FLOAT,
                           GL_FALSE,
                           sizeof(Vertex),
                           (void*)offsetof(Vertex,uv));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *textureId);
    glDrawArrays(GL_TRIANGLES, 0, geometrySize);//mode, starting index, count

    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_uv);
                           
    //swap the buffers
    glutSwapBuffers();
}

void update()
{
    //total time
    static float rotate = 0.0;
    float dt = getDT();// if you have anything moving, use dt.

    rotate += dt * 90;

    model = glm::rotate( glm::mat4(1.f), rotate, glm::vec3(1,1,1) );
    model = glm::scale(model, glm::vec3(factor, factor, factor));
    
    // Update the state of the scene
    glutPostRedisplay();//call the display callback
}


void reshape(int n_w, int n_h)
{
    w = n_w;
    h = n_h;
    //Change the viewport to be correct
    glViewport( 0, 0, w, h);
    //Update the projection matrix as well
    //See the init function for an explaination
    projection = glm::perspective(45.0f, float(w)/float(h), 0.01f, 100.0f);

}

void keyboard(unsigned char key, int x_pos, int y_pos)
{
    // Handle keyboard input
    if(key == 27)//ESC
    {
        exit(0);
    }
}

bool initialize()
{
    // Initialize basic geometry and shaders for this example


    // Create a Vertex Buffer object to store this vertex info on the GPU
    Vertex *geometry;
    bool test = loadOBJ(filename, geometry);

    if(test == false){
      cout << "Object Loader failed. Aborting" << endl;
      return 0;
    }

    test = loadTexture(textName);
    if(test == false){
      cout << "Texture loader failed. Aborting" << endl;
      return 0;
    }

    glGenBuffers(1, &vbo_geometry);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*geometrySize, geometry, GL_STATIC_DRAW);

    //--Geometry done

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    //Shader Sources
    // Put these into files and write a loader in the future
    // Note the added uniform!

    //Load Vertex Shader
    const char* vs = loadShader(VERTEX_SHADER);
    const char* fs = loadShader(FRAGMENT_SHADER);

    //compile the shaders
    GLint shader_status;

    // Vertex shader first
    glShaderSource(vertex_shader, 1, &vs, NULL);
    glCompileShader(vertex_shader);
    //check the compile status
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] FAILED TO COMPILE VERTEX SHADER!" << std::endl;
        return false;
    }

    // Now the Fragment shader
    glShaderSource(fragment_shader, 1, &fs, NULL);
    glCompileShader(fragment_shader);
    //check the compile status
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] FAILED TO COMPILE FRAGMENT SHADER!" << std::endl;
        return false;
    }

    //Now we link the 2 shader objects into a program
    //This program is what is run on the GPU
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    //check if everything linked ok
    glGetProgramiv(program, GL_LINK_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] THE SHADER PROGRAM FAILED TO LINK" << std::endl;
        return false;
    }

    //Now we set the locations of the attributes and uniforms
    //this allows us to access them easily while rendering
    loc_position = glGetAttribLocation(program,
                    const_cast<const char*>("v_position"));
    if(loc_position == -1)
    {
        std::cerr << "[F] POSITION NOT FOUND" << std::endl;
        return false;
    }

    loc_uv = glGetAttribLocation(program,
                    const_cast<const char*>("v_uv"));
    if(loc_color == -1)
    {
        std::cerr << "[F] V_COLOR NOT FOUND" << std::endl;
        return false;
    }

    loc_mvpmat = glGetUniformLocation(program,
                    const_cast<const char*>("mvpMatrix"));
    if(loc_mvpmat == -1)
    {
        std::cerr << "[F] MVPMATRIX NOT FOUND" << std::endl;
        return false;
    }

    gSampler = glGetUniformLocation(program,
                    const_cast<const char*>("gSampler"));
    if(gSampler == -1){
        std::cerr << "[F] LMAG NOT FOUND" << std::endl;
        return false;
    }

    
    //--Init the view and projection matrices
    //  if you will be having a moving camera the view matrix will need to more dynamic
    //  ...Like you should update it before you render more dynamic 
    //  for this project having them static will be fine
    view = glm::lookAt( glm::vec3(0.0, 8.0, -16.0), //Eye Position
                        glm::vec3(0.0, 0.0, 0.0), //Focus point
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up

    projection = glm::perspective( 45.0f, //the FoV typically 90 degrees is good which is what this is set to
                                   float(w)/float(h), //Aspect Ratio, so Circles stay Circular
                                   0.01f, //Distance to the near plane, normally a small value like this
                                   100.0f); //Distance to the far plane, 

    //enable depth testing
    glUniform1i(gSampler, 0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    //and its done
    return true;
}

void cleanUp()
{
    // Clean up, Clean up
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo_geometry);
}

//returns the time delta
float getDT()
{
    float ret;
    t2 = std::chrono::high_resolution_clock::now();
    ret = std::chrono::duration_cast< std::chrono::duration<float> >(t2-t1).count();
    t1 = std::chrono::high_resolution_clock::now();
    return ret;
}  

char* loadShader( const char* filename ) {
    //declare temporary variables
    char* temp;
    std::ifstream input;
    int length;

    //return the loaded temporary file
    input.open(filename); //open the file
    input.seekg(0, input.end); //go to end 9of file
    length = input.tellg(); //get end of file
    input.seekg(0, input.beg); //go to beginning
    temp = new char[length]; //allocate memory
    input.read(temp, length); //read into memory
    input.close(); //close file
    temp[length] = '\0'; //add null terminator
    return temp; //return
}


bool loadOBJ(char * obj, Vertex * &geometry){
  vector<unsigned int> temp_faces, temp_normal, temp_indices;
  vector <glm::vec3> temp_vertices, temp_vertnorms;
  srand(time(NULL));
  Assimp::Importer importer;

  // Load all assimp files
  const aiScene *scene = importer.ReadFile(obj, aiProcess_Triangulate | aiProcess_GenSmoothNormals 
                                                | aiProcess_FlipUVs );
  
  if(scene == NULL){
    const char* msg = importer.GetErrorString();
    cout << msg << endl;
    return false;
  }

  aiMesh **mesh = new aiMesh*[scene->mNumMeshes];
  int meshSize = 0;

  
  for(unsigned int i = 0; i < scene->mNumMeshes; i++){
     cout << "Meshes: " << scene->mNumMeshes << endl;
     mesh[i] = scene->mMeshes[i];
     meshSize += mesh[i]->mNumFaces;
     
    for(unsigned int j = 0; j < mesh[i]->mNumFaces;j++){
        // for each face, add the 3 points
        const aiFace& face = mesh[i]->mFaces[j];
        temp_indices.push_back(face.mIndices[0]);
        temp_indices.push_back(face.mIndices[1]);
        temp_indices.push_back(face.mIndices[2]);
    }    
  }

 // declare geometry
geometry = new Vertex[meshSize*3];

int offset = 0;
int index = 0;
int counter = 0;

for(unsigned int j = 0; j < scene->mNumMeshes; j++){
    for(unsigned int i = 0; i < mesh[j]->mNumFaces*3;i++){
        index = mesh[j]->mNumFaces*3;
        counter++;
        
        // import faces
        aiVector3D *pos= &(mesh[j]->mVertices[temp_indices[i]]);
        geometry[i+offset].position[0] = GLfloat(pos->x);
        geometry[i+offset].position[1] = GLfloat(pos->y);
        geometry[i+offset].position[2] = GLfloat(pos->z);

        if(mesh[j]->HasTextureCoords(0)){
            const aiVector3D* vt = &(mesh[j]->mTextureCoords[0][temp_indices[i]]);
            geometry[i+offset].uv[0] = GLfloat(vt->x);
            geometry[i+offset].uv[1] = GLfloat(vt->y);
        }
        else{
            geometry[i+offset].uv[0] = 0;
            geometry[i+offset].uv[1] = 0;            
        }

    }
    offset += index;
    cout << offset << endl;
}
  geometrySize = meshSize*3;

  return true;
}


bool loadTexture(char * name)
{
    ILuint texture;
    bool test;

    ilInit();
    ilGenImages(1, &texture);
    ilBindImage(texture);
    test = ilLoadImage((const ILstring)name);
    if(test == false){
        cout << "Error: there was a problem loading the texture" << endl;
        return false;
    }
    else{
        test = ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE); 
        if(test == false){
            cout << "Error: There was a problem with the conversion" << endl;
        }
            /* Create and load textures to OpenGL */
        textureId = new GLuint;
        glGenTextures(1, textureId);
        glBindTexture(GL_TEXTURE_2D, *textureId); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH),
            ilGetInteger(IL_IMAGE_HEIGHT), 0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE,
            ilGetData());
    }

    ilDeleteImages(1, textureId);
    return true;
}