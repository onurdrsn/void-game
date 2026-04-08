// platform.cpp — VOID Horror FPS
// Cross-platform: Linux (X11+GLX), Windows (Win32+WGL)
#include "platform.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ── GL function pointer definitions (both platforms) ─────────────────────────
PFNVOID_CREATESHADERFN             glCreateShader          = nullptr;
PFNVOID_DELETESHADERFN             glDeleteShader          = nullptr;
PFNVOID_SHADERSOURCEFN             glShaderSource          = nullptr;
PFNVOID_COMPILESHADERFN            glCompileShader         = nullptr;
PFNVOID_GETSHADERIVFN              glGetShaderiv           = nullptr;
PFNVOID_GETSHADERINFOLOGFN         glGetShaderInfoLog      = nullptr;
PFNVOID_CREATEPROGRAMFN            glCreateProgram         = nullptr;
PFNVOID_DELETEPROGRAMFN            glDeleteProgram         = nullptr;
PFNVOID_ATTACHSHADERFN             glAttachShader          = nullptr;
PFNVOID_LINKPROGRAMFN              glLinkProgram           = nullptr;
PFNVOID_GETPROGRAMIVFN             glGetProgramiv          = nullptr;
PFNVOID_GETPROGRAMINFOLOGFN        glGetProgramInfoLog     = nullptr;
PFNVOID_USEPROGRAMFN               glUseProgram            = nullptr;
PFNVOID_GETUNIFORMLOCATIONFN       glGetUniformLocation    = nullptr;
PFNVOID_UNIFORM1IFN                glUniform1i             = nullptr;
PFNVOID_UNIFORM1FFN                glUniform1f             = nullptr;
PFNVOID_UNIFORM2FFN                glUniform2f             = nullptr;
PFNVOID_UNIFORM3FFN                glUniform3f             = nullptr;
PFNVOID_UNIFORM4FFN                glUniform4f             = nullptr;
PFNVOID_UNIFORM3FVFN               glUniform3fv            = nullptr;
PFNVOID_UNIFORM1FVFN               glUniform1fv            = nullptr;
PFNVOID_UNIFORMMATRIX4FVFN         glUniformMatrix4fv      = nullptr;
PFNVOID_GENVERTEXARRAYSFN          glGenVertexArrays       = nullptr;
PFNVOID_DELETEVERTEXARRAYSFN       glDeleteVertexArrays    = nullptr;
PFNVOID_BINDVERTEXARRAYFN          glBindVertexArray       = nullptr;
PFNVOID_GENBUFFERSFN               glGenBuffers            = nullptr;
PFNVOID_DELETEBUFFERSFN            glDeleteBuffers         = nullptr;
PFNVOID_BINDBUFFERFN               glBindBuffer            = nullptr;
PFNVOID_BUFFERDATAFN               glBufferData            = nullptr;
PFNVOID_BUFFERSUBDATAFN            glBufferSubData         = nullptr;
PFNVOID_ENABLEVERTEXATTRIBARRAYFN  glEnableVertexAttribArray  = nullptr;
PFNVOID_DISABLEVERTEXATTRIBARRAYFN glDisableVertexAttribArray = nullptr;
PFNVOID_VERTEXATTRIBPOINTERFN      glVertexAttribPointer   = nullptr;
PFNVOID_ACTIVETEXTUREFN            glActiveTexture         = nullptr;
PFNVOID_GENERATEMIPMAPFN           glGenerateMipmap        = nullptr;
PFNVOID_GENFRAMEBUFFERSFN          glGenFramebuffers       = nullptr;
PFNVOID_DELETEFRAMEBUFFERSFN       glDeleteFramebuffers    = nullptr;
PFNVOID_BINDFRAMEBUFFERFN          glBindFramebuffer       = nullptr;
PFNVOID_FRAMEBUFFERTEXTURE2DFN     glFramebufferTexture2D  = nullptr;
PFNVOID_GENRENDERBUFFERSFN         glGenRenderbuffers      = nullptr;
PFNVOID_DELETERENDERBUFFERSFN      glDeleteRenderbuffers   = nullptr;
PFNVOID_BINDRENDERBUFFERFN         glBindRenderbuffer      = nullptr;
PFNVOID_RENDERBUFFERSTORAGEFN      glRenderbufferStorage   = nullptr;
PFNVOID_FRAMEBUFFERRENDERBUFFERFN  glFramebufferRenderbuffer  = nullptr;
PFNVOID_CHECKFRAMEBUFFERSTATUSFN   glCheckFramebufferStatus   = nullptr;
PFNVOID_DRAWARRAYSFN               glDrawArrays            = nullptr;
PFNVOID_DRAWELEMENTSFN             glDrawElements          = nullptr;

// ── Global state ─────────────────────────────────────────────────────────────
PlatformState g_platform = {};

void platform_fatal(const char* msg) {
    fprintf(stderr, "[VOID FATAL] %s\n", msg);
#ifdef _WIN32
    MessageBoxA(nullptr, msg, "VOID Fatal Error", MB_ICONERROR | MB_OK);
#endif
    exit(1);
}

bool key_down(Key k)          { return g_platform.keys[k]; }
bool key_just_pressed(Key k)  { return  g_platform.keys[k] && !g_platform.keys_prev[k]; }
bool key_just_released(Key k) { return !g_platform.keys[k] &&  g_platform.keys_prev[k]; }

// =============================================================================
// LINUX (X11 + GLX)
// =============================================================================
#if defined(__linux__)

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <time.h>

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
typedef void      (*glXSwapIntervalEXTProc)(Display*, GLXDrawable, int);

static Display*   s_dpy      = nullptr;
static Window     s_win      = 0;
static GLXContext s_ctx      = 0;
static Atom       s_wm_del   = 0;
static bool       s_captured = false;
static Cursor     s_blank    = 0;
static struct timespec s_t0;

// Helper: cast glXGetProcAddressARB result to void* safely
static void* glx_get_proc(const char* n) {
    return (void*)glXGetProcAddressARB((const GLubyte*)n);
}

static Key map_keysym(KeySym ks) {
    switch(ks) {
        case XK_w: case XK_W: return KEY_W;
        case XK_a: case XK_A: return KEY_A;
        case XK_s: case XK_S: return KEY_S;
        case XK_d: case XK_D: return KEY_D;
        case XK_space:                         return KEY_SPACE;
        case XK_Shift_L:   case XK_Shift_R:   return KEY_SHIFT;
        case XK_Control_L: case XK_Control_R: return KEY_CTRL;
        case XK_r: case XK_R: return KEY_R;
        case XK_e: case XK_E: return KEY_E;
        case XK_f: case XK_F: return KEY_F;
        case XK_Tab:    return KEY_TAB;
        case XK_Escape: return KEY_ESCAPE;
        case XK_Return: return KEY_ENTER;
        case XK_1:      return KEY_1;
        case XK_2:      return KEY_2;
        case XK_3:      return KEY_3;
        case XK_4:      return KEY_4;
        default:        return KEY_COUNT;
    }
}

static Cursor make_blank_cursor(Display* d, Window w) {
    static char data[1] = {0};
    Pixmap p = XCreateBitmapFromData(d, w, data, 1, 1);
    XColor c; c.red = c.green = c.blue = 0;
    Cursor cur = XCreatePixmapCursor(d, p, p, &c, &c, 0, 0);
    XFreePixmap(d, p);
    return cur;
}

static void load_gl_functions() {
    // Cast via (void*) first, then to the function pointer type.
    // This avoids any conflict with system-declared GL function names
    // because our PFNVOID_ typedefs are completely independent.
#define GL(type, name) \
    name = (type)glx_get_proc(#name); \
    if (!name) platform_fatal("GL missing: " #name);

    GL(PFNVOID_CREATESHADERFN,             glCreateShader)
    GL(PFNVOID_DELETESHADERFN,             glDeleteShader)
    GL(PFNVOID_SHADERSOURCEFN,             glShaderSource)
    GL(PFNVOID_COMPILESHADERFN,            glCompileShader)
    GL(PFNVOID_GETSHADERIVFN,              glGetShaderiv)
    GL(PFNVOID_GETSHADERINFOLOGFN,         glGetShaderInfoLog)
    GL(PFNVOID_CREATEPROGRAMFN,            glCreateProgram)
    GL(PFNVOID_DELETEPROGRAMFN,            glDeleteProgram)
    GL(PFNVOID_ATTACHSHADERFN,             glAttachShader)
    GL(PFNVOID_LINKPROGRAMFN,              glLinkProgram)
    GL(PFNVOID_GETPROGRAMIVFN,             glGetProgramiv)
    GL(PFNVOID_GETPROGRAMINFOLOGFN,        glGetProgramInfoLog)
    GL(PFNVOID_USEPROGRAMFN,               glUseProgram)
    GL(PFNVOID_GETUNIFORMLOCATIONFN,       glGetUniformLocation)
    GL(PFNVOID_UNIFORM1IFN,                glUniform1i)
    GL(PFNVOID_UNIFORM1FFN,                glUniform1f)
    GL(PFNVOID_UNIFORM2FFN,                glUniform2f)
    GL(PFNVOID_UNIFORM3FFN,                glUniform3f)
    GL(PFNVOID_UNIFORM4FFN,                glUniform4f)
    GL(PFNVOID_UNIFORM3FVFN,               glUniform3fv)
    GL(PFNVOID_UNIFORM1FVFN,               glUniform1fv)
    GL(PFNVOID_UNIFORMMATRIX4FVFN,         glUniformMatrix4fv)
    GL(PFNVOID_GENVERTEXARRAYSFN,          glGenVertexArrays)
    GL(PFNVOID_DELETEVERTEXARRAYSFN,       glDeleteVertexArrays)
    GL(PFNVOID_BINDVERTEXARRAYFN,          glBindVertexArray)
    GL(PFNVOID_GENBUFFERSFN,               glGenBuffers)
    GL(PFNVOID_DELETEBUFFERSFN,            glDeleteBuffers)
    GL(PFNVOID_BINDBUFFERFN,               glBindBuffer)
    GL(PFNVOID_BUFFERDATAFN,               glBufferData)
    GL(PFNVOID_BUFFERSUBDATAFN,            glBufferSubData)
    GL(PFNVOID_ENABLEVERTEXATTRIBARRAYFN,  glEnableVertexAttribArray)
    GL(PFNVOID_DISABLEVERTEXATTRIBARRAYFN, glDisableVertexAttribArray)
    GL(PFNVOID_VERTEXATTRIBPOINTERFN,      glVertexAttribPointer)
    GL(PFNVOID_ACTIVETEXTUREFN,            glActiveTexture)
    GL(PFNVOID_GENERATEMIPMAPFN,           glGenerateMipmap)
    GL(PFNVOID_GENFRAMEBUFFERSFN,          glGenFramebuffers)
    GL(PFNVOID_DELETEFRAMEBUFFERSFN,       glDeleteFramebuffers)
    GL(PFNVOID_BINDFRAMEBUFFERFN,          glBindFramebuffer)
    GL(PFNVOID_FRAMEBUFFERTEXTURE2DFN,     glFramebufferTexture2D)
    GL(PFNVOID_GENRENDERBUFFERSFN,         glGenRenderbuffers)
    GL(PFNVOID_DELETERENDERBUFFERSFN,      glDeleteRenderbuffers)
    GL(PFNVOID_BINDRENDERBUFFERFN,         glBindRenderbuffer)
    GL(PFNVOID_RENDERBUFFERSTORAGEFN,      glRenderbufferStorage)
    GL(PFNVOID_FRAMEBUFFERRENDERBUFFERFN,  glFramebufferRenderbuffer)
    GL(PFNVOID_CHECKFRAMEBUFFERSTATUSFN,   glCheckFramebufferStatus)
    // glDrawArrays / glDrawElements are GL 1.1 and always present in libGL.
    // glXGetProcAddressARB is guaranteed to return them on any compliant driver.
    GL(PFNVOID_DRAWARRAYSFN,               glDrawArrays)
    GL(PFNVOID_DRAWELEMENTSFN,             glDrawElements)
#undef GL
}

bool platform_init(const char* title, int w, int h) {
    clock_gettime(CLOCK_MONOTONIC, &s_t0);
    g_platform.width=w; g_platform.height=h; g_platform.running=true;

    s_dpy = XOpenDisplay(nullptr);
    if (!s_dpy) {
        fprintf(stderr,"[VOID] Cannot open display. Set DISPLAY env var.\n");
        fprintf(stderr,"  WSLg:   export DISPLAY=:0\n");
        fprintf(stderr,"  VcXsrv: export DISPLAY=$(grep nameserver /etc/resolv.conf|awk '{print $2}'):0.0\n");
        return false;
    }
    int scr = DefaultScreen(s_dpy);

    static const int fb_attr[] = {
        GLX_X_RENDERABLE,True, GLX_DRAWABLE_TYPE,GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,GLX_RGBA_BIT, GLX_X_VISUAL_TYPE,GLX_TRUE_COLOR,
        GLX_RED_SIZE,8, GLX_GREEN_SIZE,8, GLX_BLUE_SIZE,8, GLX_ALPHA_SIZE,8,
        GLX_DEPTH_SIZE,24, GLX_STENCIL_SIZE,8, GLX_DOUBLEBUFFER,True, None
    };
    int n=0;
    GLXFBConfig* fbc = glXChooseFBConfig(s_dpy, scr, fb_attr, &n);
    if (!fbc || n==0) platform_fatal("glXChooseFBConfig failed");
    GLXFBConfig best = fbc[0];
    XFree(fbc);

    XVisualInfo* vi = glXGetVisualFromFBConfig(s_dpy, best);
    if (!vi) platform_fatal("glXGetVisualFromFBConfig failed");

    XSetWindowAttributes swa;
    swa.colormap   = XCreateColormap(s_dpy, RootWindow(s_dpy,vi->screen), vi->visual, AllocNone);
    swa.event_mask = KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|
                     PointerMotionMask|StructureNotifyMask|FocusChangeMask;

    s_win = XCreateWindow(s_dpy, RootWindow(s_dpy,vi->screen),
                          0,0,w,h,0, vi->depth, InputOutput, vi->visual,
                          CWColormap|CWEventMask, &swa);
    XFree(vi);
    if (!s_win) platform_fatal("XCreateWindow failed");

    XStoreName(s_dpy, s_win, title);
    s_wm_del = XInternAtom(s_dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(s_dpy, s_win, &s_wm_del, 1);

    glXCreateContextAttribsARBProc ctxfn =
        (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
    if (!ctxfn) platform_fatal("glXCreateContextAttribsARB not found");

    static const int ctx_attr[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB,3, GLX_CONTEXT_MINOR_VERSION_ARB,3,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, None
    };
    s_ctx = ctxfn(s_dpy, best, nullptr, True, ctx_attr);
    if (!s_ctx) platform_fatal("Failed to create OpenGL 3.3 Core context");

    XMapWindow(s_dpy, s_win);
    glXMakeCurrent(s_dpy, s_win, s_ctx);
    XSync(s_dpy, False);

    glXSwapIntervalEXTProc swap_fn =
        (glXSwapIntervalEXTProc)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalEXT");
    if (swap_fn) swap_fn(s_dpy, s_win, 1);

    s_blank = make_blank_cursor(s_dpy, s_win);
    load_gl_functions();
    return true;
}

void platform_poll_events() {
    for(int i=0;i<KEY_COUNT;i++) g_platform.keys_prev[i]=g_platform.keys[i];
    g_platform.mouse_dx=0; g_platform.mouse_dy=0;

    while (XPending(s_dpy)>0) {
        XEvent ev; XNextEvent(s_dpy,&ev);
        switch(ev.type) {
        case KeyPress: {
            KeySym ks=XLookupKeysym(&ev.xkey,0);
            Key k=map_keysym(ks); if(k!=KEY_COUNT) g_platform.keys[k]=true; break; }
        case KeyRelease: {
            // Filter out auto-repeat
            if(XPending(s_dpy)>0){
                XEvent nx; XPeekEvent(s_dpy,&nx);
                if(nx.type==KeyPress && nx.xkey.keycode==ev.xkey.keycode &&
                   nx.xkey.time==ev.xkey.time){ XNextEvent(s_dpy,&nx); break; }
            }
            KeySym ks=XLookupKeysym(&ev.xkey,0);
            Key k=map_keysym(ks); if(k!=KEY_COUNT) g_platform.keys[k]=false; break; }
        case ButtonPress:
            if(ev.xbutton.button==Button1) g_platform.mouse_buttons[0]=true;
            if(ev.xbutton.button==Button3) g_platform.mouse_buttons[1]=true;
            if(ev.xbutton.button==Button2) g_platform.mouse_buttons[2]=true;
            break;
        case ButtonRelease:
            if(ev.xbutton.button==Button1) g_platform.mouse_buttons[0]=false;
            if(ev.xbutton.button==Button3) g_platform.mouse_buttons[1]=false;
            if(ev.xbutton.button==Button2) g_platform.mouse_buttons[2]=false;
            break;
        case MotionNotify: {
            float mx=(float)ev.xmotion.x, my=(float)ev.xmotion.y;
            if(s_captured){
                g_platform.mouse_dx += mx - g_platform.width*0.5f;
                g_platform.mouse_dy += my - g_platform.height*0.5f;
            } else {
                g_platform.mouse_dx = mx - g_platform.mouse_x;
                g_platform.mouse_dy = my - g_platform.mouse_y;
                g_platform.mouse_x=mx; g_platform.mouse_y=my;
            }
            break; }
        case ConfigureNotify:
            g_platform.width  = ev.xconfigure.width;
            g_platform.height = ev.xconfigure.height; break;
        case FocusIn:  g_platform.focused=true;  break;
        case FocusOut: g_platform.focused=false; break;
        case ClientMessage:
            if((Atom)ev.xclient.data.l[0]==s_wm_del) g_platform.running=false;
            break;
        }
    }
    if(s_captured){
        XWarpPointer(s_dpy,None,s_win,0,0,0,0,g_platform.width/2,g_platform.height/2);
        XFlush(s_dpy);
    }
    struct timespec now; clock_gettime(CLOCK_MONOTONIC,&now);
    double t=(now.tv_sec-s_t0.tv_sec)+(now.tv_nsec-s_t0.tv_nsec)*1e-9;
    g_platform.dt=t-g_platform.time; g_platform.time=t;
}

void platform_swap_buffers()            { glXSwapBuffers(s_dpy,s_win); }

double platform_get_time() {
    struct timespec n; clock_gettime(CLOCK_MONOTONIC,&n);
    return (n.tv_sec-s_t0.tv_sec)+(n.tv_nsec-s_t0.tv_nsec)*1e-9;
}

void platform_set_title(const char* t)  { XStoreName(s_dpy,s_win,t); }

void platform_capture_mouse(bool cap) {
    s_captured=cap;
    if(cap){
        if(s_blank) {
            XDefineCursor(s_dpy,s_win,s_blank);
            XGrabPointer(s_dpy,s_win,True,
                         PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
                         GrabModeAsync,GrabModeAsync,s_win,s_blank,CurrentTime);
        } else {
            fprintf(stderr, "[VOID] Warning: s_blank cursor not created\n");
        }
    } else {
        XUngrabPointer(s_dpy,CurrentTime);
        if(s_blank) XUndefineCursor(s_dpy,s_win);
    }
    XFlush(s_dpy);
}

void platform_shutdown() {
    if(s_captured) XUngrabPointer(s_dpy,CurrentTime);
    if(s_blank)    XFreeCursor(s_dpy,s_blank);
    if(s_ctx)    { glXMakeCurrent(s_dpy,None,nullptr); glXDestroyContext(s_dpy,s_ctx); }
    if(s_win)      XDestroyWindow(s_dpy,s_win);
    if(s_dpy)      XCloseDisplay(s_dpy);
}

// =============================================================================
// WINDOWS (Win32 + WGL)
// =============================================================================
#elif defined(_WIN32)

#include <mmsystem.h>

#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_ACCELERATION_ARB              0x2003
#define WGL_FULL_ACCELERATION_ARB         0x2027
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022

typedef HGLRC (WINAPI* wglCreateCtxAttribsARB_t)(HDC, HGLRC, const int*);
typedef BOOL  (WINAPI* wglChoosePixelFmtARB_t)(HDC,const int*,const float*,UINT,int*,UINT*);
typedef BOOL  (WINAPI* wglSwapIntervalEXT_t)(int);

static HWND   s_hwnd     = nullptr;
static HDC    s_hdc      = nullptr;
static HGLRC  s_hglrc    = nullptr;
static bool   s_captured = false;
static LARGE_INTEGER s_freq, s_t0;

static void* wgl_get_proc(const char* name) {
    void* p = (void*)wglGetProcAddress(name);
    if (!p) {
        HMODULE m = GetModuleHandleA("opengl32.dll");
        if (m) p = (void*)GetProcAddress(m, name);
    }
    return p;
}

static void load_gl_functions() {
#define GL(type,name) \
    name=(type)wgl_get_proc(#name); \
    if(!name) platform_fatal("GL missing: " #name);

    GL(PFNVOID_CREATESHADERFN,             glCreateShader)
    GL(PFNVOID_DELETESHADERFN,             glDeleteShader)
    GL(PFNVOID_SHADERSOURCEFN,             glShaderSource)
    GL(PFNVOID_COMPILESHADERFN,            glCompileShader)
    GL(PFNVOID_GETSHADERIVFN,              glGetShaderiv)
    GL(PFNVOID_GETSHADERINFOLOGFN,         glGetShaderInfoLog)
    GL(PFNVOID_CREATEPROGRAMFN,            glCreateProgram)
    GL(PFNVOID_DELETEPROGRAMFN,            glDeleteProgram)
    GL(PFNVOID_ATTACHSHADERFN,             glAttachShader)
    GL(PFNVOID_LINKPROGRAMFN,              glLinkProgram)
    GL(PFNVOID_GETPROGRAMIVFN,             glGetProgramiv)
    GL(PFNVOID_GETPROGRAMINFOLOGFN,        glGetProgramInfoLog)
    GL(PFNVOID_USEPROGRAMFN,               glUseProgram)
    GL(PFNVOID_GETUNIFORMLOCATIONFN,       glGetUniformLocation)
    GL(PFNVOID_UNIFORM1IFN,                glUniform1i)
    GL(PFNVOID_UNIFORM1FFN,                glUniform1f)
    GL(PFNVOID_UNIFORM2FFN,                glUniform2f)
    GL(PFNVOID_UNIFORM3FFN,                glUniform3f)
    GL(PFNVOID_UNIFORM4FFN,                glUniform4f)
    GL(PFNVOID_UNIFORM3FVFN,               glUniform3fv)
    GL(PFNVOID_UNIFORM1FVFN,               glUniform1fv)
    GL(PFNVOID_UNIFORMMATRIX4FVFN,         glUniformMatrix4fv)
    GL(PFNVOID_GENVERTEXARRAYSFN,          glGenVertexArrays)
    GL(PFNVOID_DELETEVERTEXARRAYSFN,       glDeleteVertexArrays)
    GL(PFNVOID_BINDVERTEXARRAYFN,          glBindVertexArray)
    GL(PFNVOID_GENBUFFERSFN,               glGenBuffers)
    GL(PFNVOID_DELETEBUFFERSFN,            glDeleteBuffers)
    GL(PFNVOID_BINDBUFFERFN,               glBindBuffer)
    GL(PFNVOID_BUFFERDATAFN,               glBufferData)
    GL(PFNVOID_BUFFERSUBDATAFN,            glBufferSubData)
    GL(PFNVOID_ENABLEVERTEXATTRIBARRAYFN,  glEnableVertexAttribArray)
    GL(PFNVOID_DISABLEVERTEXATTRIBARRAYFN, glDisableVertexAttribArray)
    GL(PFNVOID_VERTEXATTRIBPOINTERFN,      glVertexAttribPointer)
    GL(PFNVOID_ACTIVETEXTUREFN,            glActiveTexture)
    GL(PFNVOID_GENERATEMIPMAPFN,           glGenerateMipmap)
    GL(PFNVOID_GENFRAMEBUFFERSFN,          glGenFramebuffers)
    GL(PFNVOID_DELETEFRAMEBUFFERSFN,       glDeleteFramebuffers)
    GL(PFNVOID_BINDFRAMEBUFFERFN,          glBindFramebuffer)
    GL(PFNVOID_FRAMEBUFFERTEXTURE2DFN,     glFramebufferTexture2D)
    GL(PFNVOID_GENRENDERBUFFERSFN,         glGenRenderbuffers)
    GL(PFNVOID_DELETERENDERBUFFERSFN,      glDeleteRenderbuffers)
    GL(PFNVOID_BINDRENDERBUFFERFN,         glBindRenderbuffer)
    GL(PFNVOID_RENDERBUFFERSTORAGEFN,      glRenderbufferStorage)
    GL(PFNVOID_FRAMEBUFFERRENDERBUFFERFN,  glFramebufferRenderbuffer)
    GL(PFNVOID_CHECKFRAMEBUFFERSTATUSFN,   glCheckFramebufferStatus)
    GL(PFNVOID_DRAWARRAYSFN,               glDrawArrays)
    GL(PFNVOID_DRAWELEMENTSFN,             glDrawElements)
#undef GL
}

static Key vk_to_key(WPARAM vk) {
    switch(vk){
        case 'W': return KEY_W; case 'A': return KEY_A;
        case 'S': return KEY_S; case 'D': return KEY_D;
        case VK_SPACE:   return KEY_SPACE;
        case VK_SHIFT:   return KEY_SHIFT;
        case VK_CONTROL: return KEY_CTRL;
        case 'R': return KEY_R; case 'E': return KEY_E;
        case 'F': return KEY_F; case VK_TAB: return KEY_TAB;
        case VK_ESCAPE:  return KEY_ESCAPE;
        case VK_RETURN:  return KEY_ENTER;
        case '1': return KEY_1; case '2': return KEY_2;
        case '3': return KEY_3; case '4': return KEY_4;
        default:  return KEY_COUNT;
    }
}

static RAWINPUTDEVICE s_rid;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
    case WM_KEYDOWN: case WM_SYSKEYDOWN: {
        Key k=vk_to_key(wp); if(k!=KEY_COUNT) g_platform.keys[k]=true; return 0; }
    case WM_KEYUP: case WM_SYSKEYUP: {
        Key k=vk_to_key(wp); if(k!=KEY_COUNT) g_platform.keys[k]=false; return 0; }
    case WM_LBUTTONDOWN: g_platform.mouse_buttons[0]=true;  return 0;
    case WM_LBUTTONUP:   g_platform.mouse_buttons[0]=false; return 0;
    case WM_RBUTTONDOWN: g_platform.mouse_buttons[1]=true;  return 0;
    case WM_RBUTTONUP:   g_platform.mouse_buttons[1]=false; return 0;
    case WM_MBUTTONDOWN: g_platform.mouse_buttons[2]=true;  return 0;
    case WM_MBUTTONUP:   g_platform.mouse_buttons[2]=false; return 0;
    case WM_INPUT: {
        UINT sz=0;
        GetRawInputData((HRAWINPUT)lp,RID_INPUT,nullptr,&sz,sizeof(RAWINPUTHEADER));
        static unsigned char buf[128];
        if(sz<=sizeof(buf)){
            GetRawInputData((HRAWINPUT)lp,RID_INPUT,buf,&sz,sizeof(RAWINPUTHEADER));
            RAWINPUT* ri=(RAWINPUT*)buf;
            if(ri->header.dwType==RIM_TYPEMOUSE){
                g_platform.mouse_dx+=(float)ri->data.mouse.lLastX;
                g_platform.mouse_dy+=(float)ri->data.mouse.lLastY;
            }
        }
        return 0; }
    case WM_SIZE:
        g_platform.width  = LOWORD(lp);
        g_platform.height = HIWORD(lp); return 0;
    case WM_SETFOCUS:  g_platform.focused=true;  return 0;
    case WM_KILLFOCUS: g_platform.focused=false; return 0;
    case WM_DESTROY:   g_platform.running=false; return 0;
    case WM_CLOSE:     g_platform.running=false; return 0;
    default: return DefWindowProcA(hwnd,msg,wp,lp);
    }
}

bool platform_init(const char* title, int w, int h) {
    QueryPerformanceFrequency(&s_freq);
    QueryPerformanceCounter(&s_t0);
    timeBeginPeriod(1);

    g_platform.width=w; g_platform.height=h; g_platform.running=true;

    WNDCLASSEXA wc={};
    wc.cbSize=sizeof(wc); wc.style=CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc=WndProc; wc.hInstance=GetModuleHandleA(nullptr);
    wc.hCursor=LoadCursorA(nullptr,IDC_ARROW);
    wc.lpszClassName="VOID_WND";
    if(!RegisterClassExA(&wc)) platform_fatal("RegisterClassEx failed");

    RECT rc={0,0,w,h}; AdjustWindowRect(&rc,WS_OVERLAPPEDWINDOW,FALSE);
    s_hwnd=CreateWindowExA(WS_EX_APPWINDOW,"VOID_WND",title,
                            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                            CW_USEDEFAULT,CW_USEDEFAULT,
                            rc.right-rc.left, rc.bottom-rc.top,
                            nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);
    if(!s_hwnd) platform_fatal("CreateWindowEx failed");

    s_hdc = GetDC(s_hwnd);

    // Dummy context to load WGL extensions
    PIXELFORMATDESCRIPTOR pfd={};
    pfd.nSize=sizeof(pfd); pfd.nVersion=1;
    pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.iPixelType=PFD_TYPE_RGBA; pfd.cColorBits=32; pfd.cDepthBits=24;
    int fmt=ChoosePixelFormat(s_hdc,&pfd);
    SetPixelFormat(s_hdc,fmt,&pfd);
    HGLRC dummy=wglCreateContext(s_hdc);
    wglMakeCurrent(s_hdc,dummy);

    wglChoosePixelFmtARB_t    wglChoosePixelFormatARB =
        (wglChoosePixelFmtARB_t)   wglGetProcAddress("wglChoosePixelFormatARB");
    wglCreateCtxAttribsARB_t  wglCreateContextAttribsARB =
        (wglCreateCtxAttribsARB_t) wglGetProcAddress("wglCreateContextAttribsARB");
    wglSwapIntervalEXT_t      wglSwapIntervalEXT =
        (wglSwapIntervalEXT_t)     wglGetProcAddress("wglSwapIntervalEXT");

    wglMakeCurrent(nullptr,nullptr);
    wglDeleteContext(dummy);

    if(!wglChoosePixelFormatARB || !wglCreateContextAttribsARB)
        platform_fatal("WGL extensions not found");

    const int pfa[]={
        WGL_DRAW_TO_WINDOW_ARB,1, WGL_SUPPORT_OPENGL_ARB,1,
        WGL_DOUBLE_BUFFER_ARB,1,  WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
        WGL_PIXEL_TYPE_ARB,WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,32, WGL_DEPTH_BITS_ARB,24, 0
    };
    int pfmt=0; UINT nfmt=0;
    wglChoosePixelFormatARB(s_hdc,pfa,nullptr,1,&pfmt,&nfmt);
    if(!nfmt) platform_fatal("wglChoosePixelFormatARB failed");
    SetPixelFormat(s_hdc,pfmt,&pfd);

    const int ctx_attr[]={
        WGL_CONTEXT_MAJOR_VERSION_ARB,3,
        WGL_CONTEXT_MINOR_VERSION_ARB,3,
        WGL_CONTEXT_PROFILE_MASK_ARB,WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };
    s_hglrc=wglCreateContextAttribsARB(s_hdc,nullptr,ctx_attr);
    if(!s_hglrc) platform_fatal("Failed to create OpenGL 3.3 Core context");
    wglMakeCurrent(s_hdc,s_hglrc);
    if(wglSwapIntervalEXT) wglSwapIntervalEXT(1);

    s_rid.usUsagePage=0x01; s_rid.usUsage=0x02;
    s_rid.dwFlags=0; s_rid.hwndTarget=s_hwnd;
    RegisterRawInputDevices(&s_rid,1,sizeof(s_rid));

    load_gl_functions();
    return true;
}

void platform_poll_events() {
    for(int i=0;i<KEY_COUNT;i++) g_platform.keys_prev[i]=g_platform.keys[i];
    g_platform.mouse_dx=0; g_platform.mouse_dy=0;

    MSG msg;
    while(PeekMessageA(&msg,nullptr,0,0,PM_REMOVE)){
        TranslateMessage(&msg); DispatchMessageA(&msg);
    }

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double t=(double)(now.QuadPart-s_t0.QuadPart)/(double)s_freq.QuadPart;
    g_platform.dt=t-g_platform.time; g_platform.time=t;
}

void platform_swap_buffers()            { SwapBuffers(s_hdc); }

double platform_get_time() {
    LARGE_INTEGER n; QueryPerformanceCounter(&n);
    return (double)(n.QuadPart-s_t0.QuadPart)/(double)s_freq.QuadPart;
}

void platform_set_title(const char* t)  { SetWindowTextA(s_hwnd,t); }

void platform_capture_mouse(bool cap) {
    s_captured=cap;
    ShowCursor(cap ? FALSE : TRUE);
    if(cap){
        RECT rc; GetClientRect(s_hwnd,&rc);
        MapWindowPoints(s_hwnd,nullptr,(POINT*)&rc,2);
        ClipCursor(&rc);
    } else {
        ClipCursor(nullptr);
    }
}

void platform_shutdown() {
    if(s_captured){ ClipCursor(nullptr); ShowCursor(TRUE); }
    if(s_hglrc){ wglMakeCurrent(nullptr,nullptr); wglDeleteContext(s_hglrc); }
    if(s_hdc && s_hwnd) ReleaseDC(s_hwnd,s_hdc);
    if(s_hwnd) DestroyWindow(s_hwnd);
    timeEndPeriod(1);
}

// =============================================================================
// macOS — stub
// =============================================================================
#elif defined(__APPLE__)
#error "macOS: platform.cpp icin Cocoa+NSOpenGL implementasyonu gereklidir."
#endif // platform switch