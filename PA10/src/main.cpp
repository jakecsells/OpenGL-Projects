#include <GL/glew.h> // glew must be included before the main gl libs
#include <GL/glut.h> // doing otherwise causes compiler shouting
#include <iostream>
#include <chrono>
#include <fstream>
#include <vector>
#include <ctime>
#include <cstdlib>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "IL/il.h"

#include "bullet/btBulletCollisionCommon.h"
#include "bullet/btBulletDynamicsCommon.h"
#include <time.h>

using namespace std;

#define EASY 1
#define MEDIUM 2
#define HARD 3

struct LightInfo {
    glm::vec4 Position; // Light position in eye coords.
    glm::vec3 La; // Ambient light intensity
    glm::vec3 Ld; // Diffuse light intensity
    glm::vec3 Ls; // Specular light intensity
};

//--Data types
//This object will define the attributes of a vertex(position, color, etc...)
struct Vertex
{
    GLfloat position[3];
    GLfloat uv[2];
    GLfloat normal[3];
};

struct Mesh
{
    Vertex* geometry;
    unsigned int numFaces;
    unsigned int numVertices;
    GLuint vbo_geometry;
    GLuint texture;
    bool hasNormals;
};

struct Object
{
    Mesh* mesh;
    glm::mat4 modelMatrix;
    unsigned int numMeshes;
    btRigidBody *rigidBody;
    GLuint texture;
};

// Game stat variables
int scoreP1 = 0;
int scoreP2 = 0;
bool startGame = false;
bool paused = false;
bool aiToggle = false;
int aiLevel = 1;
int scorePlayer1 = 0;
int scorePlayer2 = 0;

// View Vars
int rotating = 1;
int ambient = 1;
int distant = 1;
int point = 1;
int spot = 1;
GLfloat light_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };


// GLOBAL GAME OBJECTS
Object lightObj;

// GLOBAL LIGHTS
LightInfo pointLight;
GLint loc_pointLight;

//--Evil Global variables
//Just for this example!
int windowWidth = 1366, windowHeight = 768;// Window size
GLuint program;// The GLSL program handle
const char* VERTEX_SHADER = "../bin/assets/shader.vert";
const char* FRAGMENT_SHADER = "../bin/assets/shader.frag";

//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader
GLint loc_modelviewmat;
GLint loc_bools;
// GLint loc_projmat;

//attribute locations
GLint loc_position;
GLint loc_uv;
GLint loc_normal;
GLint gSampler;
GLfloat lmodel_ambient[] = {0.2, 0.2, 0.2, 1.0};

//transform matrices
glm::mat4 model;//obj->world each object should have its own model matrix
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 modelView;
glm::mat4 mvp;//premultiplied modelviewprojection
int toggles[4];

//--GLUT Callbacks
void render();
void update();
void reshape(int n_w, int n_h);
void mouseClick(int button, int state, int x, int y);
void mousePassive(int x, int y);
void keyboard(unsigned char key, int x_pos, int y_pos);
void menu(int selection);
void menuWrapper(int id){
    menu(id);
}
void restart_game();
void initLighting();

//--Function Prototypes
char* loadShader(const char*);
bool loadOBJ(const char*, const char*, Object &);
void renderOBJ(Object &obj);
void update(Object &obj);
void printText(float x, float y, char* text);

//--Resource management
bool initialize();
void manageMenu();
void cleanUp();

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;


//--Main
int main(int argc, char **argv)
{
    // if(argc !=2){
    //   cout << "ERROR: Input to command line incorrect. Aborting." << endl;
    //   return 0;
    // }
    // OBJPath = argv[argc-1];
    // Initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    // Name and create the Window
    glutCreateWindow("Air Hockey");

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
    glutMouseFunc(mouseClick);// Called if there is mouse input
    
    glutCreateMenu(menuWrapper);
    glutAddMenuEntry("Toggle Rotate", 1);
    glutAddMenuEntry("Toggle Ambient", 2);
    glutAddMenuEntry("Toggle Distant", 3);
    glutAddMenuEntry("Toggle Point", 4);
    glutAddMenuEntry("Toggle Spot", 5);
    glutAddMenuEntry("Camera: Top View", 6);
    glutAddMenuEntry("Camera: Player Perspective", 7);
    glutAddMenuEntry("Camera: Angled Side View", 8);
    glutAddMenuEntry("Quit", 9);
    glutAttachMenu(GLUT_RIGHT_BUTTON);        

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
void render() {
    //clear the screen
    glClearColor(0.0, 0.0, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //enable the shader program
    glUseProgram(program);

    //enable
    glUniform1i(gSampler, 0);
    glEnableVertexAttribArray(loc_position);
    glEnableVertexAttribArray(loc_normal);
    glEnableVertexAttribArray(loc_uv);

    // render table
    renderOBJ(lightObj);

    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_normal);
    glDisableVertexAttribArray(loc_uv);
                   
    char buff[10];
    sprintf(buff, "P1:%d P2:%d\n" ,scorePlayer1, scorePlayer2);
    printText(-0.95f, 0.9f, buff);        
    //swap the buffers
    glutSwapBuffers();
}

void renderOBJ(Object &obj) {
    // //send to shader

    for(unsigned int i=0; i<obj.numMeshes; i++) {

        mvp = projection * view * obj.modelMatrix;
        modelView = view * obj.modelMatrix;
        glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(loc_modelviewmat, 1, GL_FALSE, glm::value_ptr(modelView));
        glUniform4i(loc_bools, toggles[0], toggles[1], toggles[2], toggles[3]);
        // glUniformMatrix4fv(loc_projmat, 1, GL_FALSE, glm::value_ptr(projection));
        //set up the Vertex Buffer Object so it can be drawn

        glBindBuffer(GL_ARRAY_BUFFER, obj.mesh[i].vbo_geometry);
        //set pointers into the vbo for each of the attributes(position and color)
        glVertexAttribPointer( loc_position,//location of attribute
                               3,//number of elements
                               GL_FLOAT,//type
                               GL_FALSE,//normalized?
                               sizeof(Vertex),//stride
                               0);//offset
        // load in uv texture stuff
        glVertexAttribPointer(loc_normal, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex,normal));
        glVertexAttribPointer(loc_uv, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,uv));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, obj.texture);
        glDrawArrays(GL_TRIANGLES, 0, obj.mesh[i].numFaces * 3);

    }
}

void update()
{
    //total time
    //static float rotate = 0.;
    // float dt = getDT();// if you have anything moving, use dt.
    // static float rotAngle = 0.0;
    // glm::vec3 yRotate = glm::vec3(0,-1,0);

    // rotAngle = dt * 90;
    // if(rotating > 0)
    //     lightObj.modelMatrix = glm::rotate(lightObj.modelMatrix, rotAngle, yRotate);

    // Update the state of the scene
    glutPostRedisplay();//call the display callback
}

void reshape(int n_w, int n_h)
{
    windowWidth = n_w;
    windowHeight = n_h;
    //Change the viewport to be correct
    glViewport( 0, 0, windowWidth, windowHeight);
    //Update the projection matrix as well
    //See the init function for an explaination
    projection = glm::perspective(45.0f, float(windowWidth)/float(windowHeight), 0.01f, 100.0f);

}

void mouseClick(int button, int state, int x, int y) {
    
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
    //initialize devil-based image loading
    ilInit();

    // initialize lighting
    initLighting();
    for(int i = 0; i < 4; i++)
        toggles[i] = 1;


    //Create a Vertex Buffer object to store this vertex info on the GPU
    bool ModelSuccess = loadOBJ("../bin/assets/paddle.obj", "../bin/assets/uv.jpg", lightObj);
    if(ModelSuccess == false){
      cout << "Object Loader failed. Aborting" << endl;
      return 0;
    }
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
    loc_position = glGetAttribLocation(program, const_cast<const char*>("v_position"));
    if(loc_position == -1)
    {
        std::cerr << "[F] POSITION NOT FOUND" << std::endl;
        return false;
    }

    loc_uv = glGetAttribLocation(program, const_cast<const char*>("v_uv"));
    if(loc_uv == -1)
    {
        std::cerr << "[F] V_UV NOT FOUND" << std::endl;
        return false;
    }

    loc_normal = glGetAttribLocation(program, const_cast<const char*>("v_normal"));
    if(loc_normal == -1)
    {
        std::cerr << "[F] NORMAL NOT FOUND" << std::endl;
        return false;
    }

    loc_bools = glGetUniformLocation(program, const_cast<const char*>("toggles"));
    if(loc_bools == -1)
    {
        std::cerr << "[F] TOGGLES NOT FOUND" << std::endl;
        return false;
    }

    loc_mvpmat = glGetUniformLocation(program, "mvpMatrix");
    if(loc_mvpmat == -1)
    {
        std::cerr << "[F] MVPMATRIX NOT FOUND" << std::endl;
        return false;
    }
    loc_modelviewmat = glGetUniformLocation(program, const_cast<const char*>("modelViewMatrix"));
    if(loc_modelviewmat == -1)
    {
        std::cerr << "[F] MODELVIEW MATRIX NOT FOUND" << std::endl;
        return false;
    }
    // loc_projmat = glGetUniformLocation(program, const_cast<const char*>("projectionMatrix"));
    // if(loc_projmat == -1)
    // {
    //     std::cerr << "[F] PROJECTION MATRIX NOT FOUND" << std::endl;
    //     return false;
    // }
    gSampler = glGetUniformLocation(program, const_cast<const char*>("gSampler"));
    if(gSampler == -1){
        std::cerr << "[F] LMAG NOT FOUND" << std::endl;
        return false;
    }
    
    //--Init the view and projection matrices
    //  if you will be having a moving camera the view matrix will need to more dynamic
    //  ...Like you should update it before you render more dynamic 
    //  for this project having them static will be fine
    view = glm::lookAt( glm::vec3(0, 1.0, -2.0), //Eye Position
                        glm::vec3(0.0, 0.0, 0.0), //Focus point
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up

    projection = glm::perspective( 45.0f, //the FoV typically 90 degrees is good which is what this is set to
                                   float(windowWidth)/float(windowHeight), //Aspect Ratio, so Circles stay Circular
                                   0.01f, //Distance to the near plane, normally a small value like this
                                   100.0f); //Distance to the far plane, 

    //enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    // Enable lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    //and its done
    return true;
}

void initLighting(){
    //init global lighting
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    pointLight.Position = glm::vec4(0.0,10.0,10.0,1.0);
    pointLight.La = glm::vec3(0.5,0.5,0.5);
    pointLight.Ld = glm::vec3(0.5,0.5,0.5);
    pointLight.Ls = glm::vec3(0.5,0.5,0.5);
}

void cleanUp()
{
    // Clean up, Clean up
    glDeleteProgram(program);

    // Clean up Bullet Stuff
    // delete dynamicsWorld;
    // delete solver;
    // delete dispatcher;
    // delete collisionConfiguration;
    // delete broadphase;
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


bool loadOBJ(const char* filePathOBJ, const char* filePathTex, Object &obj){
    //read file
    srand(time(NULL));
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filePathOBJ, aiProcess_Triangulate);
    //return false if there is a scene error
    if(scene == NULL){
        cout << "Error: Unable to read in " << filePathOBJ << endl;
        const char* msg = importer.GetErrorString();
        cout << msg << endl;
        return false;
    }

    // Get Materials (textures)
    bool success;
    success = ilLoadImage((const ILstring)filePathTex);
    if(!success) {
        cout << "Error: Unable to load texture " << filePathTex << endl;
        return false;
    }
    else {
        success = ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);
        if(!success) {
            cout << "Error: Unable to convert image " << filePathTex << endl;
            return false;
        }
        glGenTextures(1, &obj.texture);
        glBindTexture(GL_TEXTURE_2D, obj.texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH),
            ilGetInteger(IL_IMAGE_HEIGHT), 0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE,
            ilGetData());
    }

    // Get mesh
    obj.numMeshes = scene->mNumMeshes;
    obj.mesh = new Mesh[obj.numMeshes];

    for( unsigned int i=0; i<scene->mNumMeshes; i++ ) {
        aiMesh *tmpMesh = scene->mMeshes[i]; //aiMesh temporary to give to struct
        tmpMesh = scene->mMeshes[i];
        obj.mesh[i].geometry = new Vertex[tmpMesh->mNumVertices]; //3 vertex for each face
        obj.mesh[i].numVertices = tmpMesh->mNumVertices;
        obj.mesh[i].numFaces = tmpMesh->mNumFaces;

        for( unsigned int j=0; j<tmpMesh->mNumFaces; j++ ) {
            aiFace &tmpFace = tmpMesh->mFaces[j];
            for( unsigned int k=0; k<3; k++ ) {
                unsigned int tmpVertex = tmpFace.mIndices[k];
                //load vertex positions
                obj.mesh[i].geometry[3*j+k].position[0] = tmpMesh->mVertices[tmpVertex].x;
                obj.mesh[i].geometry[3*j+k].position[1] = tmpMesh->mVertices[tmpVertex].y;
                obj.mesh[i].geometry[3*j+k].position[2] = tmpMesh->mVertices[tmpVertex].z;
                //load vertex uv
                obj.mesh[i].geometry[3*j+k].uv[0] = tmpMesh->mTextureCoords[0][tmpVertex].x;
                obj.mesh[i].geometry[3*j+k].uv[1] = tmpMesh->mTextureCoords[0][tmpVertex].y;
                //load vertex normal
                obj.mesh[i].hasNormals = tmpMesh->HasNormals();
                if(tmpMesh->HasNormals()) {
                    obj.mesh[i].geometry[3*j+k].normal[0] = tmpMesh->mNormals[tmpVertex].x;
                    obj.mesh[i].geometry[3*j+k].normal[1] = tmpMesh->mNormals[tmpVertex].y;
                    obj.mesh[i].geometry[3*j+k].normal[2] = tmpMesh->mNormals[tmpVertex].z;
                    // cout << "<" << obj.mesh[i].geometry[3*j+k].normal[0] << ", " <<
                    //     obj.mesh[i].geometry[3*j+k].normal[1] << ", " <<
                    //     obj.mesh[i].geometry[3*j+k].normal[2] << ">" << endl;
                }
            }
        }
    }

    //Create Vertex Array Object for each mesh
    for(unsigned int i=0; i<scene->mNumMeshes; i++ ) {
        glGenBuffers(1, &(obj.mesh[i].vbo_geometry));
        glBindBuffer(GL_ARRAY_BUFFER, obj.mesh[i].vbo_geometry);
        glBufferData(GL_ARRAY_BUFFER, obj.mesh[i].numFaces * 3 * sizeof(Vertex), &(obj.mesh[i].geometry[0]), GL_STATIC_DRAW);
    }

    return true;
}    

void menu(int selection){
    //make decision based on menu choice ---------------------
    float x, y, z;
    switch (selection) {
            //start
    case 1:
        rotating *= -1;
        break;

    // toggle ambient
    case 2:
        if(toggles[0] == 0)
            toggles[0] = 1;
        else
            toggles[0] = 0;
        break;
    // toggle distant
    case 3:
        if(toggles[1] == 0)
            toggles[1] = 1;
        else
            toggles[1] = 0;
        break;

    // toggle point
    case 4:
        if(toggles[2] == 0)
            toggles[2] = 1;
        else
            toggles[2] = 0;
        break;

    //toggle spot
    case 5:
        if(toggles[3] == 0)
            toggles[3] = 1;
        else
            toggles[3] = 0;
        break;

    //camera: top view
    case 6:
        x=0.;
        y=.5;
        z=-.1;
        view = glm::lookAt( glm::vec3(x, y, z ), //Eye Position
                            glm::vec3(0.0, 0.0, 0.0), //Focus point
                            glm::vec3(0.0, 1.0, 0.0)); //y is up
        break;

     //player one's view
    case 7:
        x=1.;
        y=.1;
        z=-.1;
        view = glm::lookAt( glm::vec3(x, y, z ), //Eye Position
                            glm::vec3(0.0, 0.0, 0.0), //Focus point
                            glm::vec3(0.0, 1.0, 0.0)); //y is up
        render();

        break;

            //player one's view from top
    case 8:
        // sideCamera = false;
        x=0.0;
        y=.2;
        z=-.2;
        view = glm::lookAt( glm::vec3(x, y, z ), //Eye Position
                            glm::vec3(0.0, 0.0, 0.0), //Focus point
                            glm::vec3(0.0, 1.0, 0.0)); //y is up
        render();

        break;

    // quit
    case 9:
        exit(0);
        break;
    }


    glutPostRedisplay();
}

void printText(float x, float y, char* text) {
    // glUseProgram(0);

    // float r,g,b;
    // r = 1.;
    // g = 0.;
    // b = 0.;

    // glDisable(GL_TEXTURE_2D);
    // glDisable(GL_LIGHTING);

    // bool blending = false;

    // if(glIsEnabled(GL_BLEND)) 
    // blending = true;

    // glEnable(GL_BLEND);
    // glColor3f(r,g,b);
    // glRasterPos2f(x,y);

    // while (*text) {
    //     glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *text);
    //     text++;
    // }

    // if(!blending) 
    // glDisable(GL_BLEND);
}
