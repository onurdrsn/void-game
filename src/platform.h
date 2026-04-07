// platform.h — VOID Horror FPS
#pragma once
#include "vmath.h"

// ── Key codes (platform-independent) ─────────────────────────────────────────
enum Key {
    KEY_W=0, KEY_A, KEY_S, KEY_D,
    KEY_SPACE, KEY_SHIFT, KEY_CTRL,
    KEY_R, KEY_E, KEY_F, KEY_TAB,
    KEY_ESCAPE, KEY_ENTER,
    KEY_1, KEY_2, KEY_3, KEY_4,
    KEY_COUNT
};

struct PlatformState {
    int    width, height;
    bool   running;
    bool   keys[KEY_COUNT];
    bool   keys_prev[KEY_COUNT];
    bool   mouse_buttons[3];      // left, right, middle
    float  mouse_dx, mouse_dy;    // delta since last frame
    float  mouse_x, mouse_y;      // absolute position
    bool   focused;
    double time;
    double dt;
};

extern PlatformState g_platform;

// ── Platform GL headers ───────────────────────────────────────────────────────
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <GL/gl.h>
  // WGL extension typedefs (used only in platform.cpp)
  typedef HGLRC (WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
  typedef BOOL  (WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);
  typedef BOOL  (WINAPI* PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC, const int*, const float*, unsigned int, int*, unsigned int*);
#else
  // Linux/Mac: include GL WITHOUT GL_GLEXT_PROTOTYPES.
  // We load every GL 3.3+ function ourselves via glXGetProcAddressARB.
  // GL_GLEXT_PROTOTYPES would pre-declare them as real functions, which
  // conflicts with our function-pointer approach.
  // Temporarily rename 1.1/1.3 functions to avoid conflicts with our pointers.
  #define glActiveTexture sys_glActiveTexture
  #define glDrawArrays sys_glDrawArrays
  #define glDrawElements sys_glDrawElements
  #include <GL/gl.h>
  #include <GL/glx.h>
  #undef glActiveTexture
  #undef glDrawArrays
  #undef glDrawElements
#endif

// ── GL 3.3 function pointer typedefs (both platforms) ────────────────────────
// We define our own typedefs so the same names work on Windows and Linux.
// On Windows these shadow the ones in <GL/glext.h> (which we don't include).
// On Linux GL_GLEXT_PROTOTYPES is NOT defined so no conflict exists.

// glCreateShader returns GLuint — use a unique name to avoid any system typedef clash
typedef unsigned int  (*PFNVOID_CREATESHADERFN)(unsigned int);
typedef void          (*PFNVOID_DELETESHADERFN)(unsigned int);
typedef void          (*PFNVOID_SHADERSOURCEFN)(unsigned int, int, const char* const*, const int*);
typedef void          (*PFNVOID_COMPILESHADERFN)(unsigned int);
typedef void          (*PFNVOID_GETSHADERIVFN)(unsigned int, unsigned int, int*);
typedef void          (*PFNVOID_GETSHADERINFOLOGFN)(unsigned int, int, int*, char*);
typedef unsigned int  (*PFNVOID_CREATEPROGRAMFN)(void);
typedef void          (*PFNVOID_DELETEPROGRAMFN)(unsigned int);
typedef void          (*PFNVOID_ATTACHSHADERFN)(unsigned int, unsigned int);
typedef void          (*PFNVOID_LINKPROGRAMFN)(unsigned int);
typedef void          (*PFNVOID_GETPROGRAMIVFN)(unsigned int, unsigned int, int*);
typedef void          (*PFNVOID_GETPROGRAMINFOLOGFN)(unsigned int, int, int*, char*);
typedef void          (*PFNVOID_USEPROGRAMFN)(unsigned int);
typedef int           (*PFNVOID_GETUNIFORMLOCATIONFN)(unsigned int, const char*);
typedef void          (*PFNVOID_UNIFORM1IFN)(int, int);
typedef void          (*PFNVOID_UNIFORM1FFN)(int, float);
typedef void          (*PFNVOID_UNIFORM2FFN)(int, float, float);
typedef void          (*PFNVOID_UNIFORM3FFN)(int, float, float, float);
typedef void          (*PFNVOID_UNIFORM4FFN)(int, float, float, float, float);
typedef void          (*PFNVOID_UNIFORM3FVFN)(int, int, const float*);
typedef void          (*PFNVOID_UNIFORM1FVFN)(int, int, const float*);
typedef void          (*PFNVOID_UNIFORMMATRIX4FVFN)(int, int, unsigned char, const float*);
typedef void          (*PFNVOID_GENVERTEXARRAYSFN)(int, unsigned int*);
typedef void          (*PFNVOID_DELETEVERTEXARRAYSFN)(int, const unsigned int*);
typedef void          (*PFNVOID_BINDVERTEXARRAYFN)(unsigned int);
typedef void          (*PFNVOID_GENBUFFERSFN)(int, unsigned int*);
typedef void          (*PFNVOID_DELETEBUFFERSFN)(int, const unsigned int*);
typedef void          (*PFNVOID_BINDBUFFERFN)(unsigned int, unsigned int);
typedef void          (*PFNVOID_BUFFERDATAFN)(unsigned int, long long, const void*, unsigned int);
typedef void          (*PFNVOID_BUFFERSUBDATAFN)(unsigned int, long long, long long, const void*);
typedef void          (*PFNVOID_ENABLEVERTEXATTRIBARRAYFN)(unsigned int);
typedef void          (*PFNVOID_DISABLEVERTEXATTRIBARRAYFN)(unsigned int);
typedef void          (*PFNVOID_VERTEXATTRIBPOINTERFN)(unsigned int, int, unsigned int, unsigned char, int, const void*);
typedef void          (*PFNVOID_ACTIVETEXTUREFN)(unsigned int);
typedef void          (*PFNVOID_GENERATEMIPMAPFN)(unsigned int);
typedef void          (*PFNVOID_GENFRAMEBUFFERSFN)(int, unsigned int*);
typedef void          (*PFNVOID_DELETEFRAMEBUFFERSFN)(int, const unsigned int*);
typedef void          (*PFNVOID_BINDFRAMEBUFFERFN)(unsigned int, unsigned int);
typedef void          (*PFNVOID_FRAMEBUFFERTEXTURE2DFN)(unsigned int, unsigned int, unsigned int, unsigned int, int);
typedef void          (*PFNVOID_GENRENDERBUFFERSFN)(int, unsigned int*);
typedef void          (*PFNVOID_DELETERENDERBUFFERSFN)(int, const unsigned int*);
typedef void          (*PFNVOID_BINDRENDERBUFFERFN)(unsigned int, unsigned int);
typedef void          (*PFNVOID_RENDERBUFFERSTORAGEFN)(unsigned int, unsigned int, int, int);
typedef void          (*PFNVOID_FRAMEBUFFERRENDERBUFFERFN)(unsigned int, unsigned int, unsigned int, unsigned int);
typedef unsigned int  (*PFNVOID_CHECKFRAMEBUFFERSTATUSFN)(unsigned int);
// glDrawArrays / glDrawElements — GL 1.1 on Windows, but we load them as
// pointers on both platforms for a uniform call pattern.
typedef void          (*PFNVOID_DRAWARRAYSFN)(unsigned int, int, int);
typedef void          (*PFNVOID_DRAWELEMENTSFN)(unsigned int, int, unsigned int, const void*);

// ── GL function pointer externs (defined in platform.cpp) ────────────────────
extern PFNVOID_CREATESHADERFN             glCreateShader;
extern PFNVOID_DELETESHADERFN             glDeleteShader;
extern PFNVOID_SHADERSOURCEFN             glShaderSource;
extern PFNVOID_COMPILESHADERFN            glCompileShader;
extern PFNVOID_GETSHADERIVFN              glGetShaderiv;
extern PFNVOID_GETSHADERINFOLOGFN         glGetShaderInfoLog;
extern PFNVOID_CREATEPROGRAMFN            glCreateProgram;
extern PFNVOID_DELETEPROGRAMFN            glDeleteProgram;
extern PFNVOID_ATTACHSHADERFN             glAttachShader;
extern PFNVOID_LINKPROGRAMFN              glLinkProgram;
extern PFNVOID_GETPROGRAMIVFN             glGetProgramiv;
extern PFNVOID_GETPROGRAMINFOLOGFN        glGetProgramInfoLog;
extern PFNVOID_USEPROGRAMFN               glUseProgram;
extern PFNVOID_GETUNIFORMLOCATIONFN       glGetUniformLocation;
extern PFNVOID_UNIFORM1IFN                glUniform1i;
extern PFNVOID_UNIFORM1FFN                glUniform1f;
extern PFNVOID_UNIFORM2FFN                glUniform2f;
extern PFNVOID_UNIFORM3FFN                glUniform3f;
extern PFNVOID_UNIFORM4FFN                glUniform4f;
extern PFNVOID_UNIFORM3FVFN               glUniform3fv;
extern PFNVOID_UNIFORM1FVFN               glUniform1fv;
extern PFNVOID_UNIFORMMATRIX4FVFN         glUniformMatrix4fv;
extern PFNVOID_GENVERTEXARRAYSFN          glGenVertexArrays;
extern PFNVOID_DELETEVERTEXARRAYSFN       glDeleteVertexArrays;
extern PFNVOID_BINDVERTEXARRAYFN          glBindVertexArray;
extern PFNVOID_GENBUFFERSFN               glGenBuffers;
extern PFNVOID_DELETEBUFFERSFN            glDeleteBuffers;
extern PFNVOID_BINDBUFFERFN               glBindBuffer;
extern PFNVOID_BUFFERDATAFN               glBufferData;
extern PFNVOID_BUFFERSUBDATAFN            glBufferSubData;
extern PFNVOID_ENABLEVERTEXATTRIBARRAYFN  glEnableVertexAttribArray;
extern PFNVOID_DISABLEVERTEXATTRIBARRAYFN glDisableVertexAttribArray;
extern PFNVOID_VERTEXATTRIBPOINTERFN      glVertexAttribPointer;
extern PFNVOID_ACTIVETEXTUREFN            glActiveTexture;
extern PFNVOID_GENERATEMIPMAPFN           glGenerateMipmap;
extern PFNVOID_GENFRAMEBUFFERSFN          glGenFramebuffers;
extern PFNVOID_DELETEFRAMEBUFFERSFN       glDeleteFramebuffers;
extern PFNVOID_BINDFRAMEBUFFERFN          glBindFramebuffer;
extern PFNVOID_FRAMEBUFFERTEXTURE2DFN     glFramebufferTexture2D;
extern PFNVOID_GENRENDERBUFFERSFN         glGenRenderbuffers;
extern PFNVOID_DELETERENDERBUFFERSFN      glDeleteRenderbuffers;
extern PFNVOID_BINDRENDERBUFFERFN         glBindRenderbuffer;
extern PFNVOID_RENDERBUFFERSTORAGEFN      glRenderbufferStorage;
extern PFNVOID_FRAMEBUFFERRENDERBUFFERFN  glFramebufferRenderbuffer;
extern PFNVOID_CHECKFRAMEBUFFERSTATUSFN   glCheckFramebufferStatus;
extern PFNVOID_DRAWARRAYSFN               glDrawArrays;
extern PFNVOID_DRAWELEMENTSFN             glDrawElements;

// ── GL constants not in old gl.h headers ─────────────────────────────────────
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88B4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_FRAMEBUFFER                    0x8D40
#define GL_RENDERBUFFER                   0x8D41
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_RGBA8                          0x8058
#define GL_MULTISAMPLE                    0x809D
#endif

// ── Platform API ─────────────────────────────────────────────────────────────
bool   platform_init(const char* title, int width, int height);
void   platform_poll_events();
void   platform_swap_buffers();
void   platform_capture_mouse(bool capture);
void   platform_set_title(const char* title);
void   platform_shutdown();
void   platform_fatal(const char* msg);
double platform_get_time();

bool key_down(Key k);
bool key_just_pressed(Key k);
bool key_just_released(Key k);