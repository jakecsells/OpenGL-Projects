#include <GL/glew.h> // glew must be included before the main gl libs
#include <GL/glut.h> // doing otherwise causes compiler shouting
#include <iostream>
#include <chrono>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> // Makes passing matrices to shaders easier


//--Data types
//This object will define the attributes of a vertex(position, color, etc...)
struct Vertex
{
    GLfloat position[3];
    GLfloat color[3];
};

//--Evil Global variables
//Just for this example!
int w = 640, h = 480;// Window size
GLuint program;// The GLSL program handle
GLuint vbo_geometry;// VBO handle for our geometry
const char* VERTEX_SHADER = "../bin/assets/shader.vert";
const char* FRAGMENT_SHADER = "../bin/assets/shader.frag";
bool PLANET_ROTATE_FLAG = true;
bool MOON_ROTATE_FLAG = true;
float PLANET_ROTATE_SPEED = 100.0f;
float MOON_ROTATE_SPEED = 50.0f;


//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader

//attribute locations
GLint loc_position;
GLint loc_color;

//transform matrices
glm::mat4 modelPlanet;//obj->world each object should have its own model matrix
glm::mat4 modelMoon;//obj->world moon model, which is just a cube
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 mvpPlanet;//premultiplied planet modelviewprojection
glm::mat4 mvpMoon;//premultiplied moon modelviewprojection

//--GLUT Callbacks
void render();
void update();
void reshape(int n_w, int n_h);
void keyInput(unsigned char key, int x_pos, int y_pos);
void mouseInput(int button, int state, int x, int y);
void arrowInput(int key, int x_pos, int y_pos);

//--Function Prototypes
char* loadShader( const char* filename );
void rotateMenu( int selection );

//--Resource management
bool initialize();
void cleanUp();

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;


//--Main
int main(int argc, char **argv)
{
    // Initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(w, h);
    // Name and create the Window
    glutCreateWindow("Matrix Example");

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
    glutKeyboardFunc(keyInput);// Called if there is keyboard input
    glutSpecialFunc(arrowInput);// Called if there is arrow input
    glutMouseFunc(mouseInput);// Called if there is mouse input

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
    mvpPlanet = projection * view * modelPlanet;
    mvpMoon = projection * view * modelMoon;

    //enable the shader program
    glUseProgram(program);

    //planet model loading
    //upload the matrix to the shader
    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvpPlanet));

    //set up the Vertex Buffer Object so it can be drawn
    glEnableVertexAttribArray(loc_position);
    glEnableVertexAttribArray(loc_color);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
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

    glDrawArrays(GL_TRIANGLES, 0, 36);//mode, starting index, count

    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_color);

    //moon model loading, I will make a complete model loader later
    //upload the matrix to the shader
    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvpMoon));

    //set up the Vertex Buffer Object so it can be drawn
    glEnableVertexAttribArray(loc_position);
    glEnableVertexAttribArray(loc_color);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
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

    glDrawArrays(GL_TRIANGLES, 0, 36);//mode, starting index, count

    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_color);
                           
    //swap the buffers
    glutSwapBuffers();
}

void update()
{
    //total time
    static float anglePlanet = 0.0;
    static float rotatePlanet = 0.0;
    static float angleMoon = 0.0;
    static float rotateMoon = 0.0;
    float dt = getDT();// if you have anything moving, use dt.

    //planet updating
    anglePlanet += dt * M_PI/2; //move through 90 degrees a second
    angleMoon += dt * M_PI; //have the moon move through 180 degrees a second

    //check if the planet is rotating, step rotation
    if( PLANET_ROTATE_FLAG )
        rotatePlanet += dt * PLANET_ROTATE_SPEED;

    //check if the moon is rotating, step rotation
    if( MOON_ROTATE_FLAG )
        rotateMoon += dt * MOON_ROTATE_SPEED;

    //translate the moon to orbit
    modelPlanet = glm::translate( glm::mat4(1.0f), glm::vec3(4.0 * sin(anglePlanet), 0.0, 4.0 * cos(anglePlanet)));

    //translate the moon before the planet rotation
    glm::mat4 translateMatrix = glm::translate( modelPlanet, glm::vec3( 4.0 * sin(angleMoon), 0.0, 4.0 * cos(angleMoon) ));

    //now we rotate the planet
    modelPlanet = glm::rotate( modelPlanet, rotatePlanet, glm::vec3(0.0,1.0,0.0) );

    //scale the moon
    glm::mat4 scaleMatrix = glm::scale( glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f) );

    //multiply the translate matrix with the scale to get it to scale
    //and translate correctly
    modelMoon = translateMatrix * scaleMatrix;

    //rotation
    modelMoon = glm::rotate( modelMoon, rotateMoon, glm::vec3(0.0,1.0,0.0) );
    

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

bool initialize()
{
    // Initialize basic geometry and shaders for this example

    //this defines a cube, this is why a model loader is nice
    //you can also do this with a draw elements and indices, try to get that working
    Vertex geometry[] = { {{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}},
                          {{-1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}},
                          {{-1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}},

                          {{1.0, 1.0, -1.0}, {1.0, 1.0, 0.0}},
                          {{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}},
                          {{-1.0, 1.0, -1.0}, {0.0, 1.0, 0.0}},
                          
                          {{1.0, -1.0, 1.0}, {1.0, 0.0, 1.0}},
                          {{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}},
                          {{1.0, -1.0, -1.0}, {1.0, 0.0, 0.0}},
                          
                          {{1.0, 1.0, -1.0}, {1.0, 1.0, 0.0}},
                          {{1.0, -1.0, -1.0}, {1.0, 0.0, 0.0}},
                          {{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}},

                          {{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}},
                          {{-1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}},
                          {{-1.0, 1.0, -1.0}, {0.0, 1.0, 0.0}},

                          {{1.0, -1.0, 1.0}, {1.0, 0.0, 1.0}},
                          {{-1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}},
                          {{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}},

                          {{-1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}},
                          {{-1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}},
                          {{1.0, -1.0, 1.0}, {1.0, 0.0, 1.0}},
                          
                          {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},
                          {{1.0, -1.0, -1.0}, {1.0, 0.0, 0.0}},
                          {{1.0, 1.0, -1.0}, {1.0, 1.0, 0.0}},

                          {{1.0, -1.0, -1.0}, {1.0, 0.0, 0.0}},
                          {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},
                          {{1.0, -1.0, 1.0}, {1.0, 0.0, 1.0}},

                          {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},
                          {{1.0, 1.0, -1.0}, {1.0, 1.0, 0.0}},
                          {{-1.0, 1.0, -1.0}, {0.0, 1.0, 0.0}},

                          {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},
                          {{-1.0, 1.0, -1.0}, {0.0, 1.0, 0.0}},
                          {{-1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}},

                          {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},
                          {{-1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}},
                          {{1.0, -1.0, 1.0}, {1.0, 0.0, 1.0}}
                        };
    // Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(1, &vbo_geometry);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    glBufferData(GL_ARRAY_BUFFER, sizeof(geometry), geometry, GL_STATIC_DRAW);

    //--Geometry done

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    //Shader Sources
    // Put these into files and write a loader in the future
    // Note the added uniform!

    //Load Vertex Shader
    const char* vs = loadShader(VERTEX_SHADER);
    const char* fs = loadShader(FRAGMENT_SHADER);

    //Initialize Cube Rotation Menu
    glutCreateMenu( rotateMenu );
    glutAddMenuEntry( "Quit", 1);
    glutAddMenuEntry( "Stop Rotation", 2 );
    glutAddMenuEntry( "Start Rotation", 3 );
    glutAttachMenu( GLUT_RIGHT_BUTTON );

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
    view = glm::lookAt( glm::vec3(0.0, 8.0, -16.0), //Eye Position
                        glm::vec3(0.0, 0.0, 0.0), //Focus point
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up

    projection = glm::perspective( 45.0f, //the FoV typically 90 degrees is good which is what this is set to
                                   float(w)/float(h), //Aspect Ratio, so Circles stay Circular
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

void rotateMenu( int selection )
{
    switch( selection )
    {
        //exit program
        case 1:
            exit(0);
            break;
        //stop planet rotation
        case 2:
            PLANET_ROTATE_FLAG = false;
            break;
        //start planet rotation
        case 3:
            PLANET_ROTATE_FLAG = true;
            break;
    }
    glutPostRedisplay();
}

void arrowInput(int key, int x_pos, int y_pos)
{
    //increase planet rotation speed
    if( key == GLUT_KEY_UP && PLANET_ROTATE_FLAG )
        PLANET_ROTATE_SPEED += 10.0;

    //decrease planet rotation speed, stop if at 0.0 rotation
    if( key== GLUT_KEY_DOWN && PLANET_ROTATE_FLAG && PLANET_ROTATE_SPEED != 0.0 )
        PLANET_ROTATE_SPEED -= 10.0;

    //change planet rotation counter-clockwise
    if( key == GLUT_KEY_LEFT && PLANET_ROTATE_FLAG && PLANET_ROTATE_SPEED < 0.0 )
        PLANET_ROTATE_SPEED *= -1.0;

    //change planet rotation clockwise
    if( key == GLUT_KEY_RIGHT && PLANET_ROTATE_FLAG && PLANET_ROTATE_SPEED > 0.0 )
        PLANET_ROTATE_SPEED *= -1.0;
}

void mouseInput(int button, int state, int x, int y)
{
    //menu is connected to right button
    if( button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN )
        exit(0);
    //toggle rotation on left-click
    if( button == GLUT_LEFT_BUTTON && state == GLUT_DOWN )
    {
        if( PLANET_ROTATE_FLAG == true )
            PLANET_ROTATE_FLAG = false;
        else
            PLANET_ROTATE_FLAG = true;
    }
}

void keyInput(unsigned char key, int x_pos, int y_pos)
{
    //exit program if escape key
    if(key == 27)
        exit(0);

    //increase moon rotation speed
    if( key == 'w' && MOON_ROTATE_FLAG )
        MOON_ROTATE_SPEED += 10.0;

    //decrease moon rotation speed, stop if at 0.0 rotation
    if(key == 's' && MOON_ROTATE_FLAG && MOON_ROTATE_SPEED != 0.0 )
        MOON_ROTATE_SPEED -= 10.0;

    //change moon rotation counter-clockwise
    if( key == 'a' && MOON_ROTATE_FLAG && MOON_ROTATE_SPEED < 0.0 )
        MOON_ROTATE_SPEED *= -1.0;

    //change moon rotation clockwise
    if( key == 'd' && MOON_ROTATE_FLAG && MOON_ROTATE_SPEED > 0.0 )
        MOON_ROTATE_SPEED *= -1.0;
}
