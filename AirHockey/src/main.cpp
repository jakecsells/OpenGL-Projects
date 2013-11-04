
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

using namespace std;

#define EASY 1
#define MEDIUM 2
#define HARD 3

//--Data types
//This object will define the attributes of a vertex(position, color, etc...)
struct Vertex
{
    GLfloat position[3];
    GLfloat uv[2];
};

struct Mesh
{
    Vertex* geometry;
    unsigned int numFaces;
    unsigned int numVertices;
    GLuint vbo_geometry;
    GLuint texture;
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


// GLOBAL GAME OBJECTS
Object table, paddleComputer, paddlePlayer, puck;

//--Evil Global variables
//Just for this example!
int windowWidth = 1366, windowHeight = 768;// Window size
GLuint program;// The GLSL program handle
const char* VERTEX_SHADER = "../bin/assets/shader.vert";
const char* FRAGMENT_SHADER = "../bin/assets/shader.frag";

//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader

//attribute locations
GLint loc_position;
GLint loc_uv;
GLint gSampler;

//transform matrices
glm::mat4 model;//obj->world each object should have its own model matrix
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 mvp;//premultiplied modelviewprojection
glm::vec2 mousPos;
glm::vec2 oldMousePos;

//physics globals
btBroadphaseInterface* broadphase;
btDefaultCollisionConfiguration* collisionConfiguration;
btCollisionDispatcher* dispatcher;
btSequentialImpulseConstraintSolver* solver;
btDiscreteDynamicsWorld* dynamicsWorld;


//--GLUT Callbacks
void render();
void update();
void reshape(int n_w, int n_h);
void mouseClick(int button, int state, int x, int y);
void mousePassive(int x, int y);
void keyboard(unsigned char key, int x_pos, int y_pos);
void arrowkey(int key, int x_pos, int y_pos);
void menu(int selection);
void menuWrapper(int id){
    menu(id);
}
void restart_game();

//--Function Prototypes
char* loadShader(const char*);
bool loadOBJ(const char*, const char*, Object &);
void renderOBJ(Object &obj);
void initPhysics();
void update(Object &obj);
void aiUpdate(Object &obj, float dtime);
void easyAI(Object &obj, float dtime);
void midAI(Object &obj);
void hardAI(Object &obj);
void printText(float x, float y, char* text);

//--Resource management
bool initialize();
void manageMenu();
void cleanUp();
bool gameStart = false;

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
    glutSpecialFunc(arrowkey);// Called if there is arrowkey input
    glutMouseFunc(mouseClick);// Called if there is mouse input
    glutPassiveMotionFunc(mousePassive);// Called all the time for mouse movement.
    
    glutCreateMenu(menuWrapper);
    glutAddMenuEntry("Start", 1);
    glutAddMenuEntry("Pause/Resume", 2);
    glutAddMenuEntry("Camera: Top View", 3);
    glutAddMenuEntry("Camera: Player Perspective", 4);
    glutAddMenuEntry("Camera: Angled Side View", 5);
    glutAddMenuEntry("Turn AI On/Off",6);
    glutAddMenuEntry("Restart Game",7);
    glutAddMenuEntry("Quit", 8);
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
    if(startGame == true){
        //enable the shader program
        glUseProgram(program);

        //enable
        glUniform1i(gSampler, 0);
        glEnableVertexAttribArray(loc_position);
        glEnableVertexAttribArray(loc_uv);

        // render table
        renderOBJ(table);
        // render cube
        renderOBJ(paddleComputer);
        // render sphere
        renderOBJ(paddlePlayer);
        // render cylindar
        renderOBJ(puck);

        //clean up
        glDisableVertexAttribArray(loc_position);
        glDisableVertexAttribArray(loc_uv);
    }
                   
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
        glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp));
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
        glVertexAttribPointer( loc_uv,
                               2,
                               GL_FLOAT,
                               GL_FALSE,
                               sizeof(Vertex),
                               (void*)offsetof(Vertex,uv));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, obj.texture);
        glDrawArrays(GL_TRIANGLES, 0, obj.mesh[i].numFaces * 3);
    }
}

void update()
{
    //total time
    //static float rotate = 0.;
    float dt = getDT();// if you have anything moving, use dt.
    dynamicsWorld->stepSimulation(dt,10);

    //update each model separately
    if(aiToggle == true){
            aiUpdate(paddleComputer, dt);
    }
    else
        update(paddleComputer);
    update(paddlePlayer);
    update(puck);

    //dont go over the line
    btTransform transform;
    paddleComputer.rigidBody->getMotionState()->getWorldTransform(transform);
    btVector3 pos = transform.getOrigin();
    //cout << "position: <" << pos.x() << ", " << pos.y() << ", " << pos.z() << ">" << endl;
    if(pos.z() < 0) {
        paddleComputer.rigidBody->applyCentralImpulse(btVector3(0,0,10));
        paddleComputer.rigidBody->clearForces();
    }

    paddlePlayer.rigidBody->getMotionState()->getWorldTransform(transform);
    pos = transform.getOrigin();
    //cout << "position: <" << pos.x() << ", " << pos.y() << ", " << pos.z() << ">" << endl;
    if(pos.z() > 0) {
        paddlePlayer.rigidBody->applyCentralImpulse(btVector3(0,0,-10));
        paddlePlayer.rigidBody->clearForces();
    }

    puck.rigidBody->getMotionState()->getWorldTransform(transform);
    pos = transform.getOrigin();
    if(pos.z() >= 0.9 && gameStart) {
        if(pos.x() <= 0) {
            puck.rigidBody->applyCentralImpulse(btVector3(.5,2,-.5));
            puck.rigidBody->clearForces();
        }
        else {
            puck.rigidBody->applyCentralImpulse(btVector3(-.5,2,-.5));
            puck.rigidBody->clearForces();
        }
        if(pos.x() <= .1 && pos.x() >= -.1) {
            scorePlayer1 += 1;
        }
    }
    if(pos.z() <= -0.9 && gameStart) {
        if(pos.x() <= 0) {
            puck.rigidBody->applyCentralImpulse(btVector3(.5,2,.5));
            puck.rigidBody->clearForces();
        }
        else {
            puck.rigidBody->applyCentralImpulse(btVector3(-.5,2,.5));
            puck.rigidBody->clearForces();
        }
        if(pos.x() <= .1 && pos.x() >= -.1) {
            scorePlayer2 += 1;
        }
    }

    // Update the state of the scene
    glutPostRedisplay();//call the display callback
}

void update(Object &obj) {
    btTransform trans;
    btScalar buffer[16];
    trans = obj.rigidBody -> getCenterOfMassTransform();
    trans.getOpenGLMatrix(buffer);
    glm::mat4 modelPosition = glm::make_mat4(buffer);
    obj.modelMatrix = modelPosition;
}

void aiUpdate(Object &obj, float dtime){
    switch(aiLevel){
        case 1:
            easyAI(obj, dtime);
            break;
        case 2:
            midAI(obj);
            break;
        case 3:
            hardAI(obj);
            break;
    }
}

void easyAI(Object &obj, float dtime){
    srand(time(NULL));
    int sign = 1;
    if(rand()% 10 > 5)
        sign = -1;
    float x = sign*rand()%100/10;
    float y = sign*rand()%100/10;

        btTransform trans;
    btScalar buffer[16];
    trans = obj.rigidBody -> getCenterOfMassTransform();
    trans.getOpenGLMatrix(buffer);
    glm::mat4 modelPosition = glm::make_mat4(buffer);
    obj.modelMatrix = modelPosition;
    if((int)dtime %3 == 0)
        obj.rigidBody->applyCentralImpulse(btVector3(x/5,0,y/5));
    //obj.rigidBody->clearForces();
}
void midAI(Object &obj){
    // btTransform transform;
    // paddlePlayer.rigidBody->getMotionState()->getWorldTransform(transform);
    // btVector3 pos = transform.getOrigin();
    // transform.setOrigin(btVector3(pos.x(), pos.y(), -pos.z()));
    // paddleComputer.rigidBody->getMotionState()->setWorldTransform(transform);
    // cout << "THIS IS MID AI";
}

void hardAI(Object &obj){
    
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

void mousePassive(int x, int y) {
    float xNorm = ((float)x/(float)windowWidth - 0.5f) * 2.0f;
    float yNorm = ((float)y/(float)windowHeight - 0.5f) * 2.0f;

    float xDiff = xNorm - oldMousePos.x;
    float yDiff = yNorm - oldMousePos.y;

    paddlePlayer.rigidBody->applyCentralImpulse(btVector3(xDiff * -20, 0.0, yDiff * -20));
    paddlePlayer.rigidBody->clearForces();

    oldMousePos.x = xNorm;
    oldMousePos.y = yNorm;

    if(aiLevel == MEDIUM && aiToggle) {
        paddleComputer.rigidBody->applyCentralImpulse(btVector3(0.0, 0.0, -10.0));
        paddleComputer.rigidBody->clearForces();
        cout <<  "Computer Paddle: " << xDiff * -20 << " " << 0.0 << " " << yDiff * 20 << endl;
    }
}

void keyboard(unsigned char key, int x_pos, int y_pos)
{
    // Handle keyboard input
    if(key == 27)//ESC
    {
        exit(0);
    }
    if( key == 32 ) { // SPACE
        paddleComputer.rigidBody->applyCentralImpulse( btVector3(0,3,0) );
        paddleComputer.rigidBody->clearForces();
    }

    if(aiToggle == false && paused == false){
        if( key == 119 ) { // W
            paddleComputer.rigidBody->applyCentralImpulse( btVector3(0,0,3) );
            paddleComputer.rigidBody->clearForces();
        }
        if( key == 115 ) { // A
            paddleComputer.rigidBody->applyCentralImpulse( btVector3(0,0,-3) );
            paddleComputer.rigidBody->clearForces();
        }
        if( key == 97 ) { // S
            paddleComputer.rigidBody->applyCentralImpulse( btVector3(3,0,0) );
            paddleComputer.rigidBody->clearForces();
        }
        if( key == 100 ) { // D
            paddleComputer.rigidBody->applyCentralImpulse( btVector3(-3,0,0) );
            paddleComputer.rigidBody->clearForces();
        }
    }
}

void arrowkey(int key, int x_pos, int y_pos) {
    if(paused == false){
        if( key == GLUT_KEY_LEFT ) {
            paddlePlayer.rigidBody->applyCentralImpulse(btVector3(1,0,0));
        }
        
        if( key == GLUT_KEY_RIGHT ) {
            paddlePlayer.rigidBody->applyCentralImpulse(btVector3(-1,0,0));
        }
        
        if( key == GLUT_KEY_DOWN ) {
            paddlePlayer.rigidBody->applyCentralImpulse(btVector3(0,0,-1));
        }
        
        if( key == GLUT_KEY_UP ) {
            paddlePlayer.rigidBody -> applyCentralImpulse( btVector3(0,0,1) );
        }
    }
}

bool initialize()
{
    //initialize bullet-based physics
    initPhysics();

    //initialize devil-based image loading
    ilInit();


    //Create a Vertex Buffer object to store this vertex info on the GPU
    bool ModelSuccess = loadOBJ("../bin/assets/table.obj", "../bin/assets/tableUV.tga", table);
    if(ModelSuccess == false){
      cout << "Object Loader failed. Aborting" << endl;
      return 0;
    }
    ModelSuccess = loadOBJ("../bin/assets/paddle.obj", "../bin/assets/paddleUV.tga", paddlePlayer);
    if(ModelSuccess == false){
      cout << "Object Loader failed. Aborting" << endl;
      return 0;
    }
    ModelSuccess = loadOBJ("../bin/assets/paddle.obj", "../bin/assets/paddleUV.tga", paddleComputer);
    if(ModelSuccess == false){
      cout << "Object Loader failed. Aborting" << endl;
      return 0;
    }
    ModelSuccess = loadOBJ("../bin/assets/paddle.obj", "../bin/assets/paddleUV.tga", puck);
    if(ModelSuccess == false){
      cout << "Object Loader failed. Aborting" << endl;
      return 0;
    }

    table.modelMatrix = glm::rotate(glm::mat4(1.f),90.f,glm::vec3(0,1,0));
    // cylinder.modelMatrix = glm::scale(cylinder.modelMatrix, glm::vec3(4.0, 4.0, 4.0));
    // cube.modelMatrix = glm::scale(cube.modelMatrix, glm::vec3(2.0, 2.0, 2.0));
    // sphere.modelMatrix = glm::scale(sphere.modelMatrix, glm::vec3(5.0, 5.0, 5.0));

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

    loc_mvpmat = glGetUniformLocation(program, const_cast<const char*>("mvpMatrix"));
    if(loc_mvpmat == -1)
    {
        std::cerr << "[F] MVPMATRIX NOT FOUND" << std::endl;
        return false;
    }
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

    //and its done
    return true;
}

void cleanUp()
{
    // Clean up, Clean up
    glDeleteProgram(program);

    // Clean up Bullet Stuff
    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;
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

void initPhysics() {
    ///collision configuration contains default setup for memory, collision setup
    collisionConfiguration = new btDefaultCollisionConfiguration();
    collisionConfiguration->setConvexConvexMultipointIterations();

    ///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
    dispatcher = new  btCollisionDispatcher(collisionConfiguration);

    broadphase = new btDbvtBroadphase();

    ///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
    btSequentialImpulseConstraintSolver* sol = new btSequentialImpulseConstraintSolver;
    solver = sol;

    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration);

    dynamicsWorld->setGravity(btVector3(0,-10,0));
    dynamicsWorld->setForceUpdateAllAabbs(false);
 
    //BOARD BOTTOM
    btCollisionShape* boardBottom = new btStaticPlaneShape(btVector3(0,1,0),1.2);
    btDefaultMotionState* boardBottomMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));
    btRigidBody::btRigidBodyConstructionInfo boardBottomRigidBodyCI(0,boardBottomMotionState,boardBottom,btVector3(0,0,0));
    btRigidBody* boardBottomRigidBody = new btRigidBody(boardBottomRigidBodyCI);
    dynamicsWorld->addRigidBody(boardBottomRigidBody);

    //BOARD POSITIVE X
    btCollisionShape* boardPosX = new btStaticPlaneShape(btVector3(0,0,1),-1.15);
    btDefaultMotionState* boardPosXMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));
    btRigidBody::btRigidBodyConstructionInfo boardPosXRigidBodyCI(0,boardPosXMotionState,boardPosX,btVector3(0,0,0));
    btRigidBody* boardPosXRigidBody = new btRigidBody(boardPosXRigidBodyCI);
    dynamicsWorld->addRigidBody(boardPosXRigidBody);

    //BOARD NEGATIVE X
    btCollisionShape* boardNegX = new btStaticPlaneShape(btVector3(0,0,-1),-1.15);
    btDefaultMotionState* boardNegXMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));
    btRigidBody::btRigidBodyConstructionInfo boardNegXRigidBodyCI(0,boardNegXMotionState,boardNegX,btVector3(0,0,0));
    btRigidBody* boardNegXRigidBody = new btRigidBody(boardNegXRigidBodyCI);
    dynamicsWorld->addRigidBody(boardNegXRigidBody);

    //BOARD POSITIVE Y
    btCollisionShape* boardPosY = new btStaticPlaneShape(btVector3(-1,0,0),-0.65);
    btDefaultMotionState* boardPosYMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));
    btRigidBody::btRigidBodyConstructionInfo boardPosYRigidBodyCI(0,boardPosYMotionState,boardPosY,btVector3(0,0,0));
    btRigidBody* boardPosYRigidBody = new btRigidBody(boardPosYRigidBodyCI);
    dynamicsWorld->addRigidBody(boardPosYRigidBody);

    //BOARD NEGATIVE X
    btCollisionShape* boardNegY = new btStaticPlaneShape(btVector3(1,0,0),-0.65);
    btDefaultMotionState* boardNegYMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));
    btRigidBody::btRigidBodyConstructionInfo boardNegYRigidBodyCI(0,boardNegYMotionState,boardNegY,btVector3(0,0,0));
    btRigidBody* boardNegYRigidBody = new btRigidBody(boardNegYRigidBodyCI);
    dynamicsWorld->addRigidBody(boardNegYRigidBody);

    //puck
    btCollisionShape* puckShape = new btCylinderShape(btVector3(.2,.1,.2));
    btDefaultMotionState* puckMotionShape = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(-1,15,3)));
    btRigidBody::btRigidBodyConstructionInfo puckRigidBodyCI(2,puckMotionShape,puckShape,btVector3(0,0,0) );
    puckRigidBodyCI.m_friction = 0.1;
    puck.rigidBody = new btRigidBody(puckRigidBodyCI);
    puck.rigidBody->setActivationState(DISABLE_DEACTIVATION);
    dynamicsWorld->addRigidBody(puck.rigidBody);

    //paddleComputer
    btCollisionShape* paddleComputerShape = new btCylinderShape(btVector3(.2,.1,.2));
    btDefaultMotionState* paddleComputerMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(-1,15,3)));
    btRigidBody::btRigidBodyConstructionInfo paddleComputerRigidBodyCI( 2,paddleComputerMotionState,paddleComputerShape,btVector3(0,0,0));
    paddleComputer.rigidBody = new btRigidBody(paddleComputerRigidBodyCI);
    paddleComputer.rigidBody->setActivationState(DISABLE_DEACTIVATION);
    dynamicsWorld->addRigidBody(paddleComputer.rigidBody);

    //paddlePlayaer
    btCollisionShape* paddlePlayerShape = new btCylinderShape(btVector3(.2,.1,.2));
    btDefaultMotionState* paddlePlayerMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(-1,15,3)));
    btRigidBody::btRigidBodyConstructionInfo paddlePlayerRigidBodyCI(2,paddlePlayerMotionState,paddlePlayerShape,btVector3(0,0,0) );
    paddlePlayer.rigidBody = new btRigidBody(paddlePlayerRigidBodyCI);
    paddlePlayer.rigidBody->setActivationState(DISABLE_DEACTIVATION);
    dynamicsWorld->addRigidBody(paddlePlayer.rigidBody);
}

void menu(int selection){
    //make decision based on menu choice ---------------------
    float x, y, z;
    switch (selection) {
            //start
    case 1:
        gameStart = true;
        x=0;
        y=1.0;
        z=-2;
        view = glm::lookAt( glm::vec3(x, y, z), //Eye Position
                            glm::vec3(0.0, 0.0, 0.0), //Focus point
                            glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up
        restart_game();
        paused = true;
        glutSetCursor(GLUT_CURSOR_NONE);
        // render();

    //pause/resume game
    case 2:
        if(paused)
            paused = false;
        else
            paused = true;
        break;

            //camera: top view
    case 3:
        x=0.0;
        y=3.5;
        z=-1.;
        view = glm::lookAt( glm::vec3(x, y, z ), //Eye Position
                            glm::vec3(0.0, 0.0, 0.0), //Focus point
                            glm::vec3(0.0, 1.0, 0.0)); //y is up
        break;

            //player one's view
    case 4:
        x=0.;
        y=1.;
        z=-2.;
        view = glm::lookAt( glm::vec3(x, y, z ), //Eye Position
                            glm::vec3(0.0, 0.0, 0.0), //Focus point
                            glm::vec3(0.0, 1.0, 0.0)); //y is up
        render();

        break;

            //player one's view from top
    case 5:
        // sideCamera = false;
        x=-2;
        y=3;
        z=-2;
        view = glm::lookAt( glm::vec3(x, y, z ), //Eye Position
                            glm::vec3(0.0, 0.0, 0.0), //Focus point
                            glm::vec3(0.0, 1.0, 0.0)); //y is up
        render();

        break;

        // turn AI on/off
    case 6:
        if(aiToggle)
           aiToggle = false;
        else
           aiToggle = true;
        break;
        
        // restart game
    case 7:
        restart_game();
        break;

        //quit
    case 8:
        exit(0);
        break;
    }


    glutPostRedisplay();
}

void restart_game(){
    scorePlayer1 = 0;
    scorePlayer2 = 0;
    startGame=true;
    glutPostRedisplay();
}

void printText(float x, float y, char* text) {
    glUseProgram(0);

    float r,g,b;
    r = 1.;
    g = 0.;
    b = 0.;

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    bool blending = false;

    if(glIsEnabled(GL_BLEND)) 
    blending = true;

    glEnable(GL_BLEND);
    glColor3f(r,g,b);
    glRasterPos2f(x,y);

    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *text);
        text++;
    }

    if(!blending) 
    glDisable(GL_BLEND);
}
