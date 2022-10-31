
#include <GL/glut.h>


#ifndef _WIN32
#define __stdcall
#endif

#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdlib.h>
#include <time.h>

#include "euler_operation.h"

using std::stringstream;
using std::cout;
using std::cerr;
using std::endl;
using std::ends;

#ifndef CALLBACK
#define CALLBACK
#endif

// CALLBACK functions for GLU_TESS ////////////////////////////////////////////
// NOTE: must be declared with CALLBACK directive
void CALLBACK tessBeginCB(GLenum which);
void CALLBACK tessEndCB();
void CALLBACK tessErrorCB(GLenum errorCode);
void CALLBACK tessVertexCB(const GLvoid* data);
void CALLBACK tessVertexCB2(const GLvoid* data);
void CALLBACK tessCombineCB(const GLdouble newVertex[3], const GLdouble* neighborVertex[4],
    const GLfloat neighborWeight[4], GLdouble** outData);

// GLUT CALLBACK functions ////////////////////////////////////////////////////
void displayCB();
void reshapeCB(int w, int h);
void timerCB(int millisec);
void idleCB();
void keyboardCB(unsigned char key, int x, int y);
void mouseCB(int button, int stat, int x, int y);
void mouseMotionCB(int x, int y);

// function declarations //////////////////////////////////////////////////////
void initGL();
int  initGLUT(int argc, char** argv);
bool initSharedMem();
void clearSharedMem();
void initLights();
void setCamera(float posX, float posY, float posZ, float targetX, float targetY, float targetZ);
void drawString(const char* str, int x, int y, float color[4], void* font);
void drawString3D(const char* str, float pos[3], float color[4], void* font);
void showInfo();
const char* getPrimitiveType(GLenum type);
GLuint drawSolid(Solid* s);
void drawFace(Face* f, GLUtesselator* tess);
Solid* createSolid();

// global variables ///////////////////////////////////////////////////////////
void* font = GLUT_BITMAP_8_BY_13;
bool mouseLeftDown;
bool mouseRightDown;
float mouseX, mouseY;
float cameraAngleX;
float cameraAngleY;
float cameraDistance;
int drawMode = 0;
GLuint listId;
GLdouble vertices[64][6];               // arrary to store newly created vertices (x,y,z,r,g,b) by combine callback
int vertexIndex = 0;                    // array index for above array incremented inside combine callback

// DEBUG //
stringstream ss;

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    initSharedMem();

    // init GLUT and GL
    initGLUT(argc, argv);
    initGL();

    Solid* s = createSolid();

    // perform tessellation and compile into display lists

    listId = drawSolid(s);
    // the last GLUT call (LOOP)
    // window will be shown and display callback is triggered by events
    // NOTE: this call never return main().
    glutMainLoop(); /* Start GLUT event-processing loop */

    return 0;
}

// use euler operations to create solid
Solid* createSolid()
{
    Point p0(-5, 6, -5), p1(-5, 0, -5), p2(5, 0, -5), p3(5, 6, -5); // out edge
    Point t4(-3, 2, -5), t5(-1, 2, -5), t6(-2, 4, -5); // inner triangle
    Point t7(3, 2, -5), t8(3, 4, -5), t9(1, 4, -5), t10(1, 2, -5); // inner quad

    Vertex* v0;
    Solid* s = EulerOperation::mvsf(p0, v0);
    Face* nowf = s->sfaces[0];
    Loop* downloop = nowf->floops[0];
    HalfEdge* he01 = EulerOperation::mev(v0, p1, downloop);
    HalfEdge* he12 = EulerOperation::mev(he01->endv, p2, downloop);
    HalfEdge* he23 = EulerOperation::mev(he12->endv, p3, downloop);
    Loop* uploop = EulerOperation::mef(v0, he23->endv, downloop);

    HalfEdge* he14 = EulerOperation::mev(he01->endv, t4, uploop);
    HalfEdge* he45 = EulerOperation::mev(he14->endv, t5, uploop);
    HalfEdge* he56 = EulerOperation::mev(he45->endv, t6, uploop);

    Loop* innerloop1 = EulerOperation::mef(he56->endv, he14->endv, uploop);
    Loop* newloop1 = EulerOperation::kemr(he14->startv, he14->endv, uploop);

    HalfEdge* he27 = EulerOperation::mev(he12->endv, t7, uploop);
    HalfEdge* he78 = EulerOperation::mev(he27->endv, t8, uploop);
    HalfEdge* he89 = EulerOperation::mev(he78->endv, t9, uploop);
    HalfEdge* he910 = EulerOperation::mev(he89->endv, t10, uploop);

    Loop* innerloop2 = EulerOperation::mef(he27->endv, he910->endv, uploop);
    Loop* newloop2 = EulerOperation::kemr(he27->startv, he27->endv, uploop);

    nowf = uploop->lface;
    s= EulerOperation::sweep(nowf, Point(0, 0, 1), 2);

    s->printSolid();

    return s;
}

void drawFace(Face* f, GLUtesselator* tess)
{
    gluTessBeginPolygon(tess, 0);                       // with NULL data
    glColor3f(0.6, rand() / double(RAND_MAX) / 2.0 + 0.3, rand() / double(RAND_MAX));

    vector<Vertex*> vlist;
    vector<pair<int, int> >conts;
    int startCont = 0;
    GLdouble quad[100][3];
    for (int lIndex = 0; lIndex < f->floops.size(); lIndex++) {
        HalfEdge* he = f->floops[lIndex]->ledge;
        Vertex* startv = he->startv;
        vlist.push_back(startv);
        he = he->next;
        for (Vertex* tmpv = he->startv; tmpv != startv; he = he->next) {
            vlist.push_back(tmpv);
            tmpv = he->next->startv;
        }
        conts.push_back(make_pair(startCont, vlist.size()));
        startCont = vlist.size();
    }

    for (int i = 0; i < vlist.size(); i++) {
        quad[i][0] = vlist[i]->p.x;
        quad[i][1] = vlist[i]->p.y;
        quad[i][2] = vlist[i]->p.z;
    }

    for (int lIndex = 0; lIndex < conts.size(); lIndex++) {
        gluTessBeginContour(tess);                      // outer quad
        for (int i = conts[lIndex].first; i < conts[lIndex].second; i++) {
            gluTessVertex(tess, quad[i], quad[i]);
        }
        gluTessEndContour(tess);
    }

    gluTessEndPolygon(tess);
}

GLuint drawSolid(Solid* s)
{
    srand((unsigned)time(NULL));
    GLuint id = glGenLists(1);  // create a display list
    if (!id) return id;          // failed to create a list, return 0

    GLUtesselator* tess = gluNewTess(); // create a tessellator
    if (!tess) return 0;         // failed to create tessellation object, return 0
    
    // register callback functions  
    gluTessCallback(tess, GLU_TESS_BEGIN, (void(__stdcall*)(void))tessBeginCB);
    gluTessCallback(tess, GLU_TESS_END, (void(__stdcall*)(void))tessEndCB);
    gluTessCallback(tess, GLU_TESS_ERROR, (void(__stdcall*)(void))tessErrorCB);
    gluTessCallback(tess, GLU_TESS_VERTEX, (void(__stdcall*)())tessVertexCB);

    // tessellate and compile a concave quad into display list

    glNewList(id, GL_COMPILE);

    for (int index = 0; index < s->sfaces.size(); index++) {

        Face* nowf = s->sfaces[index];
        drawFace(nowf, tess);
    }

    glEndList();
    gluDeleteTess(tess);        // delete after tessellation

    return id;      // return handle ID of a display list
}


///////////////////////////////////////////////////////////////////////////////
// convert enum of OpenGL primitive type to a string(char*)
// OpenGL supports only 10 primitive types.
///////////////////////////////////////////////////////////////////////////////
const char* getPrimitiveType(GLenum type)
{
    switch (type)
    {
    case 0x0000:
        return "GL_POINTS";
        break;
    case 0x0001:
        return "GL_LINES";
        break;
    case 0x0002:
        return "GL_LINE_LOOP";
        break;
    case 0x0003:
        return "GL_LINE_STRIP";
        break;
    case 0x0004:
        return "GL_TRIANGLES";
        break;
    case 0x0005:
        return "GL_TRIANGLE_STRIP";
        break;
    case 0x0006:
        return "GL_TRIANGLE_FAN";
        break;
    case 0x0007:
        return "GL_QUADS";
        break;
    case 0x0008:
        return "GL_QUAD_STRIP";
        break;
    case 0x0009:
        return "GL_POLYGON";
        break;
    }
}


///////////////////////////////////////////////////////////////////////////////
// initialize GLUT for windowing
///////////////////////////////////////////////////////////////////////////////
int initGLUT(int argc, char** argv)
{
    // GLUT stuff for windowing
    // initialization openGL window.
    // it is called before any other GLUT routine
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);   // display mode

    glutInitWindowSize(800, 600);               // window size

    glutInitWindowPosition(100, 100);           // window location

    // finally, create a window with openGL context
    // Window will not displayed until glutMainLoop() is called
    // it returns a unique ID
    int handle = glutCreateWindow(argv[0]);     // param is the title of window

    // register GLUT callback functions
    glutDisplayFunc(displayCB);
    glutTimerFunc(33, timerCB, 33);             // redraw only every given millisec
    //glutIdleFunc(idleCB);                       // redraw only every given millisec
    glutReshapeFunc(reshapeCB);
    glutKeyboardFunc(keyboardCB);
    glutMouseFunc(mouseCB);
    glutMotionFunc(mouseMotionCB);

    return handle;
}


///////////////////////////////////////////////////////////////////////////////
// initialize OpenGL
// disable unused features
///////////////////////////////////////////////////////////////////////////////
void initGL()
{
    glShadeModel(GL_SMOOTH);                    // shading mathod: GL_SMOOTH or GL_FLAT
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);      // 4-byte pixel alignment

    // enable /disable features
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    //glEnable(GL_CULL_FACE);

    // track material ambient and diffuse from surface color, call it before glEnable(GL_COLOR_MATERIAL)
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    glClearColor(0.2, 0.2, 0.2, 0);                   // background color
    glClearStencil(0);                          // clear stencil buffer
    glClearDepth(1.0f);                         // 0 is near, 1 is far
    glDepthFunc(GL_LEQUAL);

    initLights();
    setCamera(0, 3, 10, 0, 0, 0);
}


///////////////////////////////////////////////////////////////////////////////
// write 2d text using GLUT
// The projection matrix must be set to orthogonal before call this function.
///////////////////////////////////////////////////////////////////////////////
void drawString(const char* str, int x, int y, float color[4], void* font)
{
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); // lighting and color mask
    glDisable(GL_LIGHTING);     // need to disable lighting for proper text color

    glColor4fv(color);          // set text color
    glRasterPos2i(x, y);        // place text position

    // loop all characters in the string
    while (*str)
    {
        glutBitmapCharacter(font, *str);
        ++str;
    }

    glEnable(GL_LIGHTING);
    glPopAttrib();
}


///////////////////////////////////////////////////////////////////////////////
// draw a string in 3D space
///////////////////////////////////////////////////////////////////////////////
void drawString3D(const char* str, float pos[3], float color[4], void* font)
{
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); // lighting and color mask
    glDisable(GL_LIGHTING);     // need to disable lighting for proper text color

    glColor4fv(color);          // set text color
    glRasterPos3fv(pos);        // place text position

    // loop all characters in the string
    while (*str)
    {
        glutBitmapCharacter(font, *str);
        ++str;
    }

    glEnable(GL_LIGHTING);
    glPopAttrib();
}



///////////////////////////////////////////////////////////////////////////////
// initialize global variables
///////////////////////////////////////////////////////////////////////////////
bool initSharedMem()
{
    mouseLeftDown = mouseRightDown = false;
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// clean up shared memory
///////////////////////////////////////////////////////////////////////////////
void clearSharedMem()
{
}



///////////////////////////////////////////////////////////////////////////////
// initialize lights
///////////////////////////////////////////////////////////////////////////////
void initLights()
{
    // set up light colors (ambient, diffuse, specular)
    GLfloat lightKa[] = { .2f, .2f, .2f, 1.0f };  // ambient light
    GLfloat lightKd[] = { .7f, .7f, .7f, 1.0f };  // diffuse light
    GLfloat lightKs[] = { 1, 1, 1, 1 };           // specular light
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightKa);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightKd);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightKs);

    // position the light
    float lightPos[4] = { 0, 0, 20, 1 }; // positional light
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glEnable(GL_LIGHT0);                        // MUST enable each light source after configuration
}



///////////////////////////////////////////////////////////////////////////////
// set camera position and lookat direction
///////////////////////////////////////////////////////////////////////////////
void setCamera(float posX, float posY, float posZ, float targetX, float targetY, float targetZ)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(posX, posY, posZ, targetX, targetY, targetZ, 0, 1, 0); // eye(x,y,z), focal(x,y,z), up(x,y,z)
}



///////////////////////////////////////////////////////////////////////////////
// display info messages
///////////////////////////////////////////////////////////////////////////////
void showInfo()
{
    // backup current model-view matrix
    glPushMatrix();                     // save current modelview matrix
    glLoadIdentity();                   // reset modelview matrix

    // set to 2D orthogonal projection
    glMatrixMode(GL_PROJECTION);     // switch to projection matrix
    glPushMatrix();                  // save current projection matrix
    glLoadIdentity();                // reset projection matrix
    gluOrtho2D(0, 400, 0, 200);  // set to orthogonal projection

    float color[4] = { 1, 1, 1, 1 };

    stringstream ss;
    ss << std::fixed << std::setprecision(3);

    if (drawMode == 0)
        ss << "Draw Mode: Fill" << ends;
    else if (drawMode == 1)
        ss << "Draw Mode: Wireframe" << ends;
    else
        ss << "Draw Mode: Points" << ends;
    drawString(ss.str().c_str(), 1, 186, color, font);
    ss.str("");

    ss << "Press 'D' to switch drawing mode." << ends;
    drawString(ss.str().c_str(), 1, 2, color, font);
    ss.str("");

    // unset floating format
    ss << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);

    // restore projection matrix
    glPopMatrix();                   // restore to previous projection matrix

    // restore modelview matrix
    glMatrixMode(GL_MODELVIEW);      // switch to modelview matrix
    glPopMatrix();                   // restore to previous modelview matrix
}


//=============================================================================
// CALLBACKS
//=============================================================================


void displayCB()
{
    // clear buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // save the initial ModelView matrix before modifying ModelView matrix
    glPushMatrix();

    // tramsform camera
    glTranslatef(0, 0, cameraDistance);
    glRotatef(cameraAngleX, 1, 0, 0);   // pitch
    glRotatef(cameraAngleY, 0, 1, 0);   // heading

    // draw meshes
    glTranslatef(-4, -1.5f, 0);


    glTranslatef(4, -1, 0);
    //Cube();
    glCallList(listId);


    // draw info messages
    showInfo();

    glPopMatrix();

    glutSwapBuffers();
}


void reshapeCB(int w, int h)
{
    // set viewport to be the entire window
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);

    // set perspective viewing frustum
    float aspectRatio = (float)w / h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //glFrustum(-aspectRatio, aspectRatio, -1, 1, 1, 100);
    gluPerspective(60.0f, (float)(w) / h, 1.0f, 1000.0f); // FOV, AspectRatio, NearClip, FarClip

    // switch to modelview matrix in order to set scene
    glMatrixMode(GL_MODELVIEW);
}


void timerCB(int millisec)
{
    glutTimerFunc(millisec, timerCB, millisec);
    glutPostRedisplay();
}


void idleCB()
{
    glutPostRedisplay();
}


void keyboardCB(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 27: // ESCAPE
        clearSharedMem();
        exit(0);
        break;

    case ' ':
        break;

    case 'd': // switch rendering modes (fill -> wire -> point)
    case 'D':
        drawMode = ++drawMode % 3;
        if (drawMode == 0)        // fill mode
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_DEPTH_TEST);
            //glEnable(GL_CULL_FACE);
        }
        else if (drawMode == 1)  // wireframe mode
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDisable(GL_DEPTH_TEST);
            //glDisable(GL_CULL_FACE);
        }
        else                    // point mode
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            glDisable(GL_DEPTH_TEST);
            //glDisable(GL_CULL_FACE);
        }
        break;

    default:
        ;
    }

    glutPostRedisplay();
}


void mouseCB(int button, int state, int x, int y)
{
    mouseX = x;
    mouseY = y;

    if (button == GLUT_LEFT_BUTTON)
    {
        if (state == GLUT_DOWN)
        {
            mouseLeftDown = true;
        }
        else if (state == GLUT_UP)
            mouseLeftDown = false;
    }

    else if (button == GLUT_RIGHT_BUTTON)
    {
        if (state == GLUT_DOWN)
        {
            mouseRightDown = true;
        }
        else if (state == GLUT_UP)
            mouseRightDown = false;
    }
}


void mouseMotionCB(int x, int y)
{
    if (mouseLeftDown)
    {
        cameraAngleY += (x - mouseX);
        cameraAngleX += (y - mouseY);
        mouseX = x;
        mouseY = y;
    }
    if (mouseRightDown)
    {
        cameraDistance += (y - mouseY) * 0.2f;
        mouseY = y;
    }

    //glutPostRedisplay();
}



///////////////////////////////////////////////////////////////////////////////
// GLU_TESS CALLBACKS
///////////////////////////////////////////////////////////////////////////////
void CALLBACK tessBeginCB(GLenum which)
{
    glBegin(which);

    // DEBUG //
    ss << "glBegin(" << getPrimitiveType(which) << ");\n";
}



void CALLBACK tessEndCB()
{
    glEnd();

    // DEBUG //
    ss << "glEnd();\n";
}



void CALLBACK tessVertexCB(const GLvoid* data)
{
    // cast back to double type
    const GLdouble* ptr = (const GLdouble*)data;

    glVertex3dv(ptr);

    // DEBUG //
    ss << "  glVertex3d(" << *ptr << ", " << *(ptr + 1) << ", " << *(ptr + 2) << ");\n";
}



///////////////////////////////////////////////////////////////////////////////
// draw a vertex with color
///////////////////////////////////////////////////////////////////////////////
void CALLBACK tessVertexCB2(const GLvoid* data)
{
    // cast back to double type
    const GLdouble* ptr = (const GLdouble*)data;

    glColor3dv(ptr + 3);
    glVertex3dv(ptr);

    // DEBUG //
    ss << "  glColor3d(" << *(ptr + 3) << ", " << *(ptr + 4) << ", " << *(ptr + 5) << ");\n";
    ss << "  glVertex3d(" << *ptr << ", " << *(ptr + 1) << ", " << *(ptr + 2) << ");\n";
}



///////////////////////////////////////////////////////////////////////////////
// Combine callback is used to create a new vertex where edges intersect.
// In this function, copy the vertex data into local array and compute the
// color of the vertex. And send it back to tessellator, so tessellator pass it
// to vertex callback function.
//
// newVertex: the intersect point which tessellator creates for us
// neighborVertex[4]: 4 neighbor vertices to cause intersection (given from 3rd param of gluTessVertex()
// neighborWeight[4]: 4 interpolation weights of 4 neighbor vertices
// outData: the vertex data to return to tessellator
///////////////////////////////////////////////////////////////////////////////
void CALLBACK tessCombineCB(const GLdouble newVertex[3], const GLdouble* neighborVertex[4],
    const GLfloat neighborWeight[4], GLdouble** outData)
{
    // copy new intersect vertex to local array
    // Because newVertex is temporal and cannot be hold by tessellator until next
    // vertex callback called, it must be copied to the safe place in the app.
    // Once gluTessEndPolygon() called, then you can safly deallocate the array.
    vertices[vertexIndex][0] = newVertex[0];
    vertices[vertexIndex][1] = newVertex[1];
    vertices[vertexIndex][2] = newVertex[2];

    // compute vertex color with given weights and colors of 4 neighbors
    // the neighborVertex[4] must hold required info, in this case, color.
    // neighborVertex was actually the third param of gluTessVertex() and is
    // passed into here to compute the color of the intersect vertex.
    vertices[vertexIndex][3] = neighborWeight[0] * neighborVertex[0][3] +   // red
        neighborWeight[1] * neighborVertex[1][3] +
        neighborWeight[2] * neighborVertex[2][3] +
        neighborWeight[3] * neighborVertex[3][3];
    vertices[vertexIndex][4] = neighborWeight[0] * neighborVertex[0][4] +   // green
        neighborWeight[1] * neighborVertex[1][4] +
        neighborWeight[2] * neighborVertex[2][4] +
        neighborWeight[3] * neighborVertex[3][4];
    vertices[vertexIndex][5] = neighborWeight[0] * neighborVertex[0][5] +   // blue
        neighborWeight[1] * neighborVertex[1][5] +
        neighborWeight[2] * neighborVertex[2][5] +
        neighborWeight[3] * neighborVertex[3][5];


    // return output data (vertex coords and others)
    *outData = vertices[vertexIndex];   // assign the address of new intersect vertex

    ++vertexIndex;  // increase index for next vertex
}



void CALLBACK tessErrorCB(GLenum errorCode)
{
    const GLubyte* errorStr;

    errorStr = gluErrorString(errorCode);
    cerr << "[ERROR]: " << errorStr << endl;
}
