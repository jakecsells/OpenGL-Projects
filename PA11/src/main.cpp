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


//--Data types
//This object will define the attributes of a vertex(position, color, etc...)
struct Vertex
{
    GLfloat position[3];
    GLfloat uv[2];
    GLfloat normal[3];
};

//Mesh will hold the geometry of each mesh loaded
struct Mesh
{
    Vertex* geometry;
    unsigned int numFaces;
    unsigned int numVertices;
    GLuint vbo_geometry;
    GLuint texture;
    GLuint _texture;
    bool hasNormals;
};

//Objects can hold one or more meshes
struct Object
{
    Mesh* mesh;
    glm::mat4 modelMatrix;
    unsigned int numMeshes;
    btRigidBody *rigidBody;
    btTriangleMesh *btMesh;
    GLuint texture;
    GLuint _texture;
};

// View Vars
int rotating = 1;
int ambient = 1;
int distant = 1;
int point = 1;
int spot = 1;

// GLOBAL GAME OBJECTS
Object maze01;
Object ball01;
Object mazeDEMO;
Object ballDEMO;

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
GLint loc_RAVEMODE;
// GLint loc_projmat;

//attribute locations
GLint loc_position;
GLint loc_uv;
GLint loc_normal;
GLint gSampler;
GLfloat lmodel_ambient[] = {0.2, 0.2, 0.2, 1.0};
GLfloat RAVEMODE[3] = {0.,0.,0.};
int counter;
bool textureToggle;
int mode = 0;


//transform matrices
glm::mat4 model;//obj->world each object should have its own model matrix
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 modelView;
glm::mat4 mvp;//premultiplied modelviewprojection
int toggles[4];

//Physics Globals
btBroadphaseInterface* broadphase;
btDefaultCollisionConfiguration* collisionConfiguration;
btCollisionDispatcher* dispatcher;
btSequentialImpulseConstraintSolver* solver;
btDiscreteDynamicsWorld* dynamicsWorld;

//maze transforms
float yaw, pitch, roll;
float pitchDEMO, rollDEMO;
glm::vec2 oldMousePos;

//game control globals
bool paused;
bool followBall;
bool mouseControl;
bool gameWon = false;
float gameWinTime = 0.0;


//--GLUT Callbacks
void render();
void update();
void update(Object &obj);
void reshape(int n_w, int n_h);
void mouseClick(int button, int state, int x, int y);
void mousePassive(int x, int y);
void keyboard(unsigned char key, int x_pos, int y_pos);
void menu(int selection);
void menuWrapper(int id){
    menu(id);
}

//--Function Prototypes
char* loadShader(const char*);
bool loadOBJ(const char*, const char*, const char*, Object &);
void renderOBJ(Object &obj);
void update(Object &obj);
void printText(float x, float y, char* text);
void initPhysics();

//--Resource management
bool initialize();
void manageMenu();
void cleanUp();

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2, gameTime, step;
float timeRet = 0.0;


//--Main
int main(int argc, char **argv) {
    // if(argc !=2){
    //   cout << "ERROR: Input to command line incorrect. Aborting." << endl;
    //   return 0;
    // }
    // OBJPath = argv[argc-1];
    // Initialize glut
    srand(time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    // Name and create the Window
    glutCreateWindow("Labyrinth");

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
    glutPassiveMotionFunc(mousePassive);
    
    glutCreateMenu(menuWrapper);
    glutAddMenuEntry("Play Game", 1);
    glutAddMenuEntry("Play Tutorial", 2);
    glutAddMenuEntry("Restart", 3);
    glutAddMenuEntry("Pause/Resume", 4);
    glutAddMenuEntry("Enable/Disable Mouse Control", 5);
    glutAddMenuEntry("Toggle Point", 6);
    glutAddMenuEntry("RAVE MODE!", 7);
    glutAddMenuEntry("Camera: Front View", 8);
    glutAddMenuEntry("Camera: Top View", 9);
    glutAddMenuEntry("Camera: Follow the Ball", 10);
    glutAddMenuEntry("Change Texture", 11);
    glutAddMenuEntry("Quit", 12);
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
    if(toggles[3]==1) {
        int tmp = rand() % 3;
        switch(tmp) {
            case 1:
                glClearColor(1.0, 0.0, 0.0, 1.0);
                break;
            case 2:
                glClearColor(0.0, 1.0, 0.0, 1.0);
                break;
            case 3:
                glClearColor(0.0, 0.0, 1.0, 1.0);
                break;
        }
    }
    else
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
    if(mode == 1 || 2){
        renderOBJ(mazeDEMO);
        renderOBJ(ballDEMO);
    }
    if(mode == 2){
        renderOBJ(maze01);
        renderOBJ(ball01);
    }

    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_normal);
    glDisableVertexAttribArray(loc_uv);
    
    // update timer
    if(mode == 2){
        step = std::chrono::high_resolution_clock::now();
        timeRet = std::chrono::duration_cast< std::chrono::duration<float> >(step-gameTime).count();
        char buff[40];
        int minutes = (int)(timeRet/60);
        float seconds = timeRet - 60*minutes;
        if(gameWon) {
            if(timeRet < gameWinTime || gameWinTime == 0.0) {
                gameWinTime = timeRet;
            }
            minutes = (int)(gameWinTime/60);
            seconds = gameWinTime - 60*minutes;
            sprintf(buff, "YOU WIN!!! Official Time:   %d:%f\n", minutes, seconds);
        }
        else
            sprintf(buff, "Time:   %d:%f\n", minutes, seconds);
        printText(-0.95f, 0.9f, buff);
    }

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
        glUniform3f(loc_RAVEMODE, RAVEMODE[0], RAVEMODE[1], RAVEMODE[2]);
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
        if(textureToggle == 0)
            glBindTexture(GL_TEXTURE_2D, obj.texture);
        else
            glBindTexture(GL_TEXTURE_2D, obj._texture);
        glDrawArrays(GL_TRIANGLES, 0, obj.mesh[i].numFaces * 3);

    }
}

void update() {

    //update physics stuff
    float dt = getDT();
    float high = .99;
    float low = .25;
    int tmp = rand() % 3;

    if(!paused) {
        dynamicsWorld->stepSimulation(dt,10);
    }

    update(ball01);
    update(maze01);
    update(ballDEMO);
    update(mazeDEMO);

    // IMPORTANT RAVEMODE THINGS
    counter++;
    if(counter % 5 == 0){
        counter = 0;
        if(tmp == 0){
            RAVEMODE[0] = high;
            RAVEMODE[1] = low;
            RAVEMODE[2] = low;
        }
        if(tmp == 1){
            RAVEMODE[0] = low;
            RAVEMODE[1] = high;
            RAVEMODE[2] = low;
        }
        if(tmp == 2){
            RAVEMODE[0] = low;
            RAVEMODE[1] = low;
            RAVEMODE[2] = high;
        }
    }
    btTransform trans;
    trans.setIdentity();
    btQuaternion quat;
    quat.setEuler(yaw, pitch, roll);
    trans.setRotation(quat);
    maze01.rigidBody->setCenterOfMassTransform(trans);

    btTransform transform;
    ball01.rigidBody->getMotionState()->getWorldTransform(transform);
    btVector3 pos = transform.getOrigin();

    if(followBall) {
        view = glm::lookAt( glm::vec3(0.0, 15.0, 0.0 ), //Eye Position
                            glm::vec3(pos.x(), pos.y(), pos.z()), //Focus point
                            glm::vec3(0.0, 0.0, 1.0)); //y is up
    }

    //YOU WIN FUNCTION
    if(pos.x() < -4.2 && pos.z() > 4.2) {
        gameWon = true;
    }

    btTransform transDEMO;
    transDEMO.setIdentity();
    btQuaternion quatDEMO;
    quatDEMO.setEuler(0.0,pitchDEMO, rollDEMO);
    transDEMO.setRotation(quatDEMO);
    transDEMO.setOrigin(btVector3(50.0,0.0,0.0));
    mazeDEMO.rigidBody->setCenterOfMassTransform(transDEMO);

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

void reshape(int n_w, int n_h) {
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

void keyboard(unsigned char key, int x_pos, int y_pos) {
    // Handle keyboard input
    if(key == 27)//ESC
    {
        exit(0);
    }
    //for main maze
    if(key == 119 && !paused && mode == 2) { // W
        pitch += 0.005;
    }
    if(key == 115 && !paused && mode == 2) { // A
        pitch -= 0.005;
    }
    if(key == 97 && !paused && mode == 2) { // S
        roll -= 0.005;
    }
    if(key == 100 && !paused && mode == 2) { // D
        roll += 0.005;
    }
    //for demo maze
    if(key == 119 && !paused && mode == 1) { // W
        pitchDEMO += 0.005;
    }
    if(key == 115 && !paused && mode == 1) { // A
        pitchDEMO -= 0.005;
    }
    if(key == 97 && !paused && mode == 1) { // S
        rollDEMO -= 0.005;
    }
    if(key == 100 && !paused && mode == 1) { // D
        rollDEMO += 0.005;
    }
}

void mousePassive(int x, int y) {
    if(mouseControl) {
        float xNorm = ((float)x/(float)windowWidth - 0.5f) * 2.0f;
        float yNorm = ((float)y/(float)windowHeight - 0.5f) * 2.0f;

        float xDiff = xNorm - oldMousePos.x;
        float yDiff = yNorm - oldMousePos.y;

        roll += xDiff*0.5;
        pitch -= yDiff*0.5;

        oldMousePos.x = xNorm;
        oldMousePos.y = yNorm;
    }
}

bool initialize() {
    // Set timer for game
    //initialize rotations
    yaw = 0.0;
    pitch = 0.0;
    roll = 0.0;

    //intialize control globals
    paused = false;
    followBall = false;
    mouseControl = false;

    //initialize devil-based image loading
    ilInit();

    //initialize lighting toggles
    for(int i = 0; i < 3; i++)
        toggles[i] = 1;
    toggles[3] = 0; 

    //Create a Vertex Buffer object to store this vertex info on the GPU
    bool ModelSuccess = loadOBJ("../bin/assets/maze1.obj", "../bin/assets/wood.jpg",
                                         "../bin/assets/neonBoard.jpg", maze01);
    if(ModelSuccess == false){
      cout << "Object Loader failed. Aborting" << endl;
      return 0;
    }
    ModelSuccess = loadOBJ("../bin/assets/ball1.obj", "../bin/assets/marble.jpg",
                                        "../bin/assets/neonBall.jpg", ball01);
    if(ModelSuccess == false){
      cout << "Object Loader failed. Aborting" << endl;
      return 0;
    }    
    //Create a Vertex Buffer object to store this vertex info on the GPU
    ModelSuccess = loadOBJ("../bin/assets/maze2.obj", "../bin/assets/wood.jpg",
                                         "../bin/assets/neonBoard.jpg", mazeDEMO);
    if(ModelSuccess == false){
      cout << "Object Loader failed. Aborting" << endl;
      return 0;
    }
    ModelSuccess = loadOBJ("../bin/assets/ball1.obj", "../bin/assets/marble.jpg",
                                        "../bin/assets/neonBall.jpg", ballDEMO);
    if(ModelSuccess == false){
      cout << "Object Loader failed. Aborting" << endl;
      return 0;
    }
    //--Geometry done

    //initialize physics
    initPhysics();

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
    loc_RAVEMODE = glGetUniformLocation(program, const_cast<const char*>("RAVEMODE"));
    if(loc_RAVEMODE == -1){
        std::cerr << "[F] ERROR WITH RAVEMODE!!!!" << std::endl;
        return false;
    }
    //--Init the view and projection matrices
    //  if you will be having a moving camera the view matrix will need to more dynamic
    //  ...Like you should update it before you render more dynamic 
    //  for this project having them static will be fine
    view = glm::lookAt( glm::vec3(0, 10.0, -10.0), //Eye Position
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

    //and its done
    return true;
}

void cleanUp() {
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
float getDT() {
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


bool loadOBJ(const char* filePathOBJ, const char* filePathTex,const char* filePathTex2, Object &obj) {
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

    // Get Materials (textures)
    success = ilLoadImage((const ILstring)filePathTex2);
    if(!success) {
        cout << "Error: Unable to load _texture " << filePathTex2 << endl;
        return false;
    }
    else {
        success = ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);
        if(!success) {
            cout << "Error: Unable to convert image " << filePathTex << endl;
            return false;
        }
        glGenTextures(1, &obj._texture);
        glBindTexture(GL_TEXTURE_2D, obj._texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH),
            ilGetInteger(IL_IMAGE_HEIGHT), 0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE,
            ilGetData());
    }

    // Create bullet mesh for collision
    obj.btMesh = new btTriangleMesh();
    btVector3 v0, v1, v2;

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
            //make position vectors for each vertex on face.
            v0 = btVector3(obj.mesh[i].geometry[3*j+0].position[0],
                           obj.mesh[i].geometry[3*j+0].position[1],
                           obj.mesh[i].geometry[3*j+0].position[2]);
            v1 = btVector3(obj.mesh[i].geometry[3*j+1].position[0],
                           obj.mesh[i].geometry[3*j+1].position[1],
                           obj.mesh[i].geometry[3*j+1].position[2]);
            v2 = btVector3(obj.mesh[i].geometry[3*j+2].position[0],
                           obj.mesh[i].geometry[3*j+2].position[1],
                           obj.mesh[i].geometry[3*j+2].position[2]);

            //add face to btmesh
            obj.btMesh->addTriangle(v0, v1, v2, false);
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

void menu(int selection) {
    //make decision based on menu choice ---------------------
    float x, y, z;
    btTransform transform;
    btVector3 pos;
    switch (selection) {
    case 1: // play game
        mode = 2;
        gameTime = std::chrono::high_resolution_clock::now();
        //reset camera position
        view = glm::lookAt( glm::vec3(0, 10.0, -10.0), //Eye Position
                            glm::vec3(0.0, 0.0, 0.0), //Focus point
                            glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up
        break;

    case 2: // play demo 
        mode = 1;
        //set camera to position
        view = glm::lookAt( glm::vec3(50.0, 10.0, -10.0 ), //Eye Position
                            glm::vec3(50.0, 0.0, 0.0), //Focus point
                            glm::vec3(0.0, 1.0, 0.0)); //y is up
        break;

    //restart
    case 3:
        //restart ball
        ball01.rigidBody->getMotionState()->getWorldTransform(transform);
        pos = btVector3(4.5,5.0,-4.2);
        transform.setOrigin(pos);
        ball01.rigidBody->setCenterOfMassTransform(transform);
        ball01.rigidBody->setLinearVelocity(btVector3(0.0,0.0,0.0));
        gameTime = std::chrono::high_resolution_clock::now();
        gameWon = false;
        
        //reset board
        pitch = 0.0;
        roll = 0.0;
        render();
        break;

    // pause/unpause the game
    case 4:
        if(!paused) {
            paused = true;
        }
        else {
            paused = false;
        }
        break;
    // toggle distant
    case 5:
        if(!mouseControl)
            mouseControl = true;
        else
            mouseControl = false;
        break;

    // toggle point
    case 6:
        if(toggles[2] == 0)
            toggles[2] = 1;
        else
            toggles[2] = 0;
        break;

    //toggle spot
    case 7:
        if(toggles[3] == 0)
            toggles[3] = 1;
        else
            toggles[3] = 0;
        break;

    //camera: front view
    case 8:
        x=0.0;
        y=10;
        z=-10;
        view = glm::lookAt( glm::vec3(x, y, z ), //Eye Position
                            glm::vec3(0.0, 0.0, 0.0), //Focus point
                            glm::vec3(0.0, 1.0, 0.0)); //y is up
        followBall = false;
        break;

    //camera: top view
    case 9:
        x=0.0;
        y=15.0;
        z=0.0;
        view = glm::lookAt( glm::vec3(x, y, z ), //Eye Position
                            glm::vec3(0.0, 0.0, 0.0), //Focus point
                            glm::vec3(0.0, 0.0, 1.0)); //y is up
        render();
        followBall = false;
        break;

    //camera: follow the ball
    case 10:
        x=0.0;
        y=15.0;
        z=0.0;
        view = glm::lookAt( glm::vec3(x, y, z ), //Eye Position
                            glm::vec3(0.0, 0.0, 0.0), //Focus point
                            glm::vec3(0.0, 1.0, 0.0)); //y is up
        render();
        followBall = true;
        break;

    // quit
    case 11:
        if(textureToggle == 0)
            textureToggle = 1;
        else
            textureToggle = 0;
        break;
    // quit
    case 12:
        exit(0);
        break;
    }

    glutPostRedisplay();
}

void printText(float x, float y, char* text) {
    glUseProgram(0);

    float r,g,b;
    r = 1.;
    g = .5;
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

    dynamicsWorld->setGravity(btVector3(0,-5.0,0));
    // dynamicsWorld->setForceUpdateAllAabbs(false);

    //maze01
    btCollisionShape* maze01Shape = new btBvhTriangleMeshShape(maze01.btMesh, true);
    btDefaultMotionState* maze01MotionShape = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,0,0)));
    btRigidBody::btRigidBodyConstructionInfo maze01RigidBodyCI(0,maze01MotionShape,maze01Shape,btVector3(0,0,0));
    maze01.rigidBody = new btRigidBody(maze01RigidBodyCI);
    // maze01.rigidBody->setActivationState(DISABLE_DEACTIVATION);
    dynamicsWorld->addRigidBody(maze01.rigidBody);

    //maze02
    btCollisionShape* mazeDEMOShape = new btBvhTriangleMeshShape(mazeDEMO.btMesh, true);
    btDefaultMotionState* mazeDEMOMotionShape = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(50,0.0,0.0)));
    btRigidBody::btRigidBodyConstructionInfo mazeDEMORigidBodyCI(0,mazeDEMOMotionShape,mazeDEMOShape,btVector3(0,0,0));
    mazeDEMO.rigidBody = new btRigidBody(mazeDEMORigidBodyCI);
    // maze01.rigidBody->setActivationState(DISABLE_DEACTIVATION);
    dynamicsWorld->addRigidBody(mazeDEMO.rigidBody);


    //Ball01
    btCollisionShape* ball01Shape = new btSphereShape(0.15);   
    btDefaultMotionState* ball01MotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(4.5,5.0,-4.2 )));
    btRigidBody::btRigidBodyConstructionInfo ball01RigidBodyCI(2,ball01MotionState,ball01Shape,btVector3(0,0,0) );
    ball01RigidBodyCI.m_friction = 0.01;
    ball01RigidBodyCI.m_restitution = 1.0;
    ball01.rigidBody = new btRigidBody(ball01RigidBodyCI);
    ball01.rigidBody->setActivationState(DISABLE_DEACTIVATION);
    dynamicsWorld->addRigidBody(ball01.rigidBody);

    //ballDEMO
    btCollisionShape* ballDEMOShape = new btSphereShape(0.15);   
    btDefaultMotionState* ballDEMOMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(50.0,5.0,0.0)));
    btRigidBody::btRigidBodyConstructionInfo ballDEMORigidBodyCI(2,ballDEMOMotionState,ballDEMOShape,btVector3(0,0,0) );
    ballDEMORigidBodyCI.m_friction = 0.01;
    ballDEMORigidBodyCI.m_restitution = 1.0;
    ballDEMO.rigidBody = new btRigidBody(ballDEMORigidBodyCI);
    ballDEMO.rigidBody->setActivationState(DISABLE_DEACTIVATION);
    dynamicsWorld->addRigidBody(ballDEMO.rigidBody);
}