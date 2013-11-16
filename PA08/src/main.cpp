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

#include "bullet/btBulletCollisionCommon.h"
#include "bullet/btBulletDynamicsCommon.h"


using namespace std;


//--Data types
//This object will define the attributes of a vertex(position, color, etc...)
struct Vertex
{
    GLfloat position[3];
    GLfloat color[3];
};

struct Mesh
{
    Vertex* geometry;
    unsigned int numFaces;
    unsigned int numVertices;
    GLuint vbo_geometry;
};

struct Object
{
    Mesh* mesh;
    glm::mat4 modelMatrix;
    unsigned int numMeshes;
    btRigidBody *rigidBody;
};

// GLOBAL GAME OBJECTS
Object board, cube, sphere, cylinder;

//--Evil Global variables
//Just for this example!
int w = 640, h = 480;// Window size
GLuint program;// The GLSL program handle
const char* VERTEX_SHADER = "../bin/assets/shader.vert";
const char* FRAGMENT_SHADER = "../bin/assets/shader.frag";

//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader

//attribute locations
GLint loc_position;
GLint loc_color;

//transform matrices
glm::mat4 model;//obj->world each object should have its own model matrix
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 mvp;//premultiplied modelviewprojection

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
void keyboard(unsigned char key, int x_pos, int y_pos);
void arrowkey(int key, int x_pos, int y_pos);

//--Function Prototypes
char* loadShader(const char*);
bool loadOBJ(const char*, Object &);
void renderOBJ(Object &obj);
void initPhysics();
void update(Object &obj);

//--Resource management
bool initialize();
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
    glutInitWindowSize(w, h);
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
    glEnableVertexAttribArray(loc_position);
    glEnableVertexAttribArray(loc_color);

    // render board
    renderOBJ(board);
    // render cube
    renderOBJ(cube);
    // render sphere
    renderOBJ(cylinder);
    // render cylindar
    renderOBJ(sphere);

    

    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_color);
                           
    //swap the buffers
    glutSwapBuffers();
}

void update()
{
    //total time
    //static float rotate = 0.;
    float dt = getDT();// if you have anything moving, use dt.
    dynamicsWorld->stepSimulation(dt,10);

    //update each model separately
    update(sphere);
    update(cylinder);
    update(cube);
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
    if( key == 32 ) { // SPACE
        sphere.rigidBody->applyCentralImpulse( btVector3(0,10,0) );
        sphere.rigidBody->clearForces();
    }
    if( key == 119 ) { // W
        sphere.rigidBody->applyCentralImpulse( btVector3(0,0,10) );
        sphere.rigidBody->clearForces();
    }
    if( key == 115 ) { // A
        sphere.rigidBody->applyCentralImpulse( btVector3(0,0,-10) );
        sphere.rigidBody->clearForces();
    }
    if( key == 97 ) { // S
        sphere.rigidBody->applyCentralImpulse( btVector3(10,0,0) );
        sphere.rigidBody->clearForces();
    }
    if( key == 100 ) { // D
        sphere.rigidBody->applyCentralImpulse( btVector3(-10,0,0) );
        sphere.rigidBody->clearForces();
    }
}

void arrowkey(int key, int x_pos, int y_pos) {

    if( key == GLUT_KEY_LEFT ) {
        //~ paddle.rb->applyForce(btVector3(200,0,0),btVector3(0,0,0));
        cylinder.rigidBody->applyCentralImpulse(btVector3(10,0,0));
    }
    
    if( key == GLUT_KEY_RIGHT ) {
        //~ paddle.rb->applyForce(btVector3(-200,0,0),btVector3(0,0,0));
        cylinder.rigidBody->applyCentralImpulse(btVector3(-10,0,0));
    }
    
    if( key == GLUT_KEY_DOWN ) {
        //~ paddle.rb->applyForce(btVector3(0,0,-200),btVector3(0,0,0));
        cylinder.rigidBody->applyCentralImpulse(btVector3(0,0,-10));
    }
    
    if( key == GLUT_KEY_UP ) {
        //~ paddle.rb->applyForce(btVector3(0,0,200),btVector3(0,0,0));
        cylinder.rigidBody -> applyCentralImpulse( btVector3(0,0,10) );
    }
}

bool initialize()
{
    //initialize bullet-based physics
    initPhysics();


    // Create a Vertex Buffer object to store this vertex info on the GPU
    bool ModelSuccess = loadOBJ("../bin/assets/board.obj", board);
    if(ModelSuccess == false){
      cout << "Object Loader failed. Aborting" << endl;
      return 0;
    }
    ModelSuccess = loadOBJ("../bin/assets/cube.obj", cube);
    if(ModelSuccess == false){
      cout << "Object Loader failed. Aborting" << endl;
      return 0;
    }
    ModelSuccess = loadOBJ("../bin/assets/cylinder.obj", cylinder);
    if(ModelSuccess == false){
      cout << "Object Loader failed. Aborting" << endl;
      return 0;
    }
    ModelSuccess = loadOBJ("../bin/assets/sphere.obj", sphere);
    if(ModelSuccess == false){
      cout << "Object Loader failed. Aborting" << endl;
      return 0;
    }

    board.modelMatrix = glm::scale(board.modelMatrix, glm::vec3(10.0, 10.0, 10.0));
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
    loc_position = glGetAttribLocation(program,
                    const_cast<const char*>("v_position"));
    if(loc_position == -1)
    {
        std::cerr << "[F] POSITION NOT FOUND" << std::endl;
        return false;
    }

    loc_color = glGetAttribLocation(program,
                    const_cast<const char*>("v_color"));
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
    
    //--Init the view and projection matrices
    //  if you will be having a moving camera the view matrix will need to more dynamic
    //  ...Like you should update it before you render more dynamic 
    //  for this project having them static will be fine
    view = glm::lookAt( glm::vec3(0.0, 10.0, -15.0), //Eye Position
                        glm::vec3(0.0, 0.0, 0.0), //Focus point
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up

    projection = glm::perspective( 45.0f, //the FoV typically 90 degrees is good which is what this is set to
                                   float(w)/float(h), //Aspect Ratio, so Circles stay Circular
                                   0.01f, //Distance to the near plane, normally a small value like this
                                   100.0f); //Distance to the far plane, 

    //enable depth testing
    glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LESS);

    //and its done
    return true;
}

void cleanUp()
{
    // Clean up, Clean up
    glDeleteProgram(program);
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


bool loadOBJ(const char * filePath, Object &obj){
    //read file
    srand(time(NULL));
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filePath, aiProcess_Triangulate);
    //return false if there is a scene error
    if(scene == NULL){
        cout << "Error: Error in reading object file!" << endl;
        const char* msg = importer.GetErrorString();
        cout << msg << endl;
        return false;
    }
    obj.numMeshes = scene->mNumMeshes;
    obj.mesh = new Mesh[obj.numMeshes];

    for( unsigned int i=0; i<scene->mNumMeshes; i++ ) {
        aiMesh *tmpMesh = scene->mMeshes[i]; //aiMesh temporary to give to struct
        tmpMesh = scene->mMeshes[i];
        obj.mesh[i].geometry = new Vertex[tmpMesh->mNumVertices]; //3 vertex for each face
        obj.mesh[i].numVertices = tmpMesh->mNumVertices;
        obj.mesh[i].numFaces = tmpMesh->mNumFaces;
        // cout << "||||| Mesh: " << i << "| Vertices: " << obj.mesh[i].numVertices << 
        //     " | Faces: " << obj.mesh[i].numFaces << endl;

        for( unsigned int j=0; j<tmpMesh->mNumFaces; j++ ) {
            aiFace &tmpFace = tmpMesh->mFaces[j];
            for( unsigned int k=0; k<3; k++ ) {
                unsigned int tmpVertex = tmpFace.mIndices[k];
                obj.mesh[i].geometry[3*j+k].position[0] = tmpMesh->mVertices[tmpVertex].x;
                obj.mesh[i].geometry[3*j+k].position[1] = tmpMesh->mVertices[tmpVertex].y;
                obj.mesh[i].geometry[3*j+k].position[2] = tmpMesh->mVertices[tmpVertex].z;

                obj.mesh[i].geometry[3*j+k].color[0] = GLfloat((float)rand()/(float)RAND_MAX);
                obj.mesh[i].geometry[3*j+k].color[1] = GLfloat((float)rand()/(float)RAND_MAX);
                obj.mesh[i].geometry[3*j+k].color[2] = GLfloat((float)rand()/(float)RAND_MAX);
            }
            // cout << "Vertex: " << j << " | (" <<
            //     obj.mesh[i].geometry[j].position[0] << ", " <<
            //     obj.mesh[i].geometry[j].position[0] << ", " <<
            //     obj.mesh[i].geometry[j].position[0] << ")" << endl;
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

        glVertexAttribPointer( loc_color,
                               3,
                               GL_FLOAT,
                               GL_FALSE,
                               sizeof(Vertex),
                               (void*)offsetof(Vertex,color));
        //TEXTURE STUFF: DO LATER
        // glVertexAttribPointer( loc_uv,
        //                        2,
        //                        GL_FLOAT,
        //                        GL_FALSE,
        //                        sizeof(Vertex),
        //                        (void*)offsetof(Vertex,uv));
        // GLuint num = m.meshes[i].faces[j].texIndex;
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, num);
        glDrawArrays(GL_TRIANGLES, 0, obj.mesh[i].numFaces * 3);
    }
}

void initPhysics() {
    ///collision configuration contains default setup for memory, collision setup
    collisionConfiguration = new btDefaultCollisionConfiguration();
    //m_collisionConfiguration->setConvexConvexMultipointIterations();

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
    btCollisionShape* boardBottom = new btStaticPlaneShape(btVector3(0,1,0),1);
    btDefaultMotionState* boardBottomMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));
    btRigidBody::btRigidBodyConstructionInfo boardBottomRigidBodyCI(0,boardBottomMotionState,boardBottom,btVector3(0,0,0));
    btRigidBody* boardBottomRigidBody = new btRigidBody(boardBottomRigidBodyCI);
    dynamicsWorld->addRigidBody(boardBottomRigidBody);

    //BOARD POSITIVE X
    btCollisionShape* boardPosX = new btStaticPlaneShape(btVector3(0,0,1),-4.8);
    btDefaultMotionState* boardPosXMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));
    btRigidBody::btRigidBodyConstructionInfo boardPosXRigidBodyCI(0,boardPosXMotionState,boardPosX,btVector3(0,0,0));
    btRigidBody* boardPosXRigidBody = new btRigidBody(boardPosXRigidBodyCI);
    dynamicsWorld->addRigidBody(boardPosXRigidBody);

    //BOARD NEGATIVE X
    btCollisionShape* boardNegX = new btStaticPlaneShape(btVector3(0,0,-1),-4.8);
    btDefaultMotionState* boardNegXMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));
    btRigidBody::btRigidBodyConstructionInfo boardNegXRigidBodyCI(0,boardNegXMotionState,boardNegX,btVector3(0,0,0));
    btRigidBody* boardNegXRigidBody = new btRigidBody(boardNegXRigidBodyCI);
    dynamicsWorld->addRigidBody(boardNegXRigidBody);

    //BOARD POSITIVE Y
    btCollisionShape* boardPosY = new btStaticPlaneShape(btVector3(-1,0,0),-9.3);
    btDefaultMotionState* boardPosYMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));
    btRigidBody::btRigidBodyConstructionInfo boardPosYRigidBodyCI(0,boardPosYMotionState,boardPosY,btVector3(0,0,0));
    btRigidBody* boardPosYRigidBody = new btRigidBody(boardPosYRigidBodyCI);
    dynamicsWorld->addRigidBody(boardPosYRigidBody);

    //BOARD NEGATIVE X
    btCollisionShape* boardNegY = new btStaticPlaneShape(btVector3(1,0,0),-9.3);
    btDefaultMotionState* boardNegYMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));
    btRigidBody::btRigidBodyConstructionInfo boardNegYRigidBodyCI(0,boardNegYMotionState,boardNegY,btVector3(0,0,0));
    btRigidBody* boardNegYRigidBody = new btRigidBody(boardNegYRigidBodyCI);
    dynamicsWorld->addRigidBody(boardNegYRigidBody);

    //SPHERE
    btCollisionShape* sphereShape = new btSphereShape( 0.3 );
    btDefaultMotionState* sphereMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(-1,15,3)) );
    btRigidBody::btRigidBodyConstructionInfo sphereRigidBodyCI( 2,sphereMotionState,sphereShape,btVector3(0,0,0) );
    sphere.rigidBody = new btRigidBody( sphereRigidBodyCI );
    sphere.rigidBody->setActivationState(DISABLE_DEACTIVATION);
    dynamicsWorld->addRigidBody( sphere.rigidBody );

    //CUBE
    btCollisionShape* cubeShape = new btBoxShape( btVector3(.3,.3,.3));
    btDefaultMotionState* cubeMotionShape = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(-1,15,3)) );
    btRigidBody::btRigidBodyConstructionInfo cubeRigidBodyCI( 2,cubeMotionShape,cubeShape,btVector3(0,0,0) );
    cube.rigidBody = new btRigidBody( cubeRigidBodyCI );
    cube.rigidBody->setActivationState(DISABLE_DEACTIVATION);
    dynamicsWorld->addRigidBody( cube.rigidBody );

    //CYLINDER
    btCollisionShape* cylinderShape = new btCylinderShape( btVector3(1,.2,10) );
    btDefaultMotionState* cylinderMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(-1,15,3)) );
    btRigidBody::btRigidBodyConstructionInfo cylinderRigidBodyCI( 2,cylinderMotionState,cylinderShape,btVector3(0,0,0) );
    cylinder.rigidBody = new btRigidBody( cylinderRigidBodyCI );
    cylinder.rigidBody->setActivationState(DISABLE_DEACTIVATION);
    dynamicsWorld->addRigidBody( cylinder.rigidBody );

}