// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the presenter's private resize path against real GLES allocations.
// Unused SDL/guest entry points are discarded from this standalone GPU test.
#include "d3d_presenter_gles.cpp"
#include <EGL/egl.h>
#define CHECK(c) do { if(!(c)){std::fprintf(stderr,"Live display check failed at line %d: %s\n",__LINE__,#c);return 1;} } while(0)
static auto read_pixels=&glReadPixels;
static bool pixel(const Surface &s,unsigned r,unsigned g,unsigned b){
    doa3_gl::api.glBindFramebuffer(GL_FRAMEBUFFER,s.fbo);unsigned char rgba[4]{};
    read_pixels(s.pixel_width/2,s.pixel_height/2,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
    return rgba[0]==r&&rgba[1]==g&&rgba[2]==b&&rgba[3]==255&&doa3_gl::api.glGetError()==GL_NO_ERROR;
}
int main(int argc,char **argv){
    CHECK(argc==1||argc==2);
    void *gles_library=nullptr,*egl_library=nullptr;
    if(argc==2){
        gles_library=dlopen((std::string(argv[1])+"/libGLESv2_angle.so").c_str(),RTLD_NOW|RTLD_GLOBAL);CHECK(gles_library);
        egl_library=dlopen((std::string(argv[1])+"/libEGL_angle.so").c_str(),RTLD_NOW|RTLD_LOCAL);CHECK(egl_library);
        CHECK(doa3_gl::api.load([&](const char *name){return dlsym(gles_library,name);}));
        read_pixels=reinterpret_cast<decltype(read_pixels)>(dlsym(gles_library,"glReadPixels"));CHECK(read_pixels);
    }
#define EGL_ENTRY(name) const auto name##_fn=egl_library?reinterpret_cast<decltype(&name)>(dlsym(egl_library,#name)):&name;CHECK(name##_fn)
    EGL_ENTRY(eglGetDisplay);EGL_ENTRY(eglInitialize);EGL_ENTRY(eglChooseConfig);
    EGL_ENTRY(eglCreateContext);EGL_ENTRY(eglCreatePbufferSurface);EGL_ENTRY(eglMakeCurrent);
    EGL_ENTRY(eglDestroySurface);EGL_ENTRY(eglDestroyContext);EGL_ENTRY(eglTerminate);
#undef EGL_ENTRY
    EGLDisplay display=eglGetDisplay_fn(EGL_DEFAULT_DISPLAY);CHECK(eglInitialize_fn(display,nullptr,nullptr));
    const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
    EGLConfig config;EGLint count;CHECK(eglChooseConfig_fn(display,attributes,&config,1,&count)&&count);
    const EGLint context_attributes[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    EGLContext context=eglCreateContext_fn(display,config,EGL_NO_CONTEXT,context_attributes);CHECK(context!=EGL_NO_CONTEXT);
    const EGLint surface_attributes[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};
    EGLSurface window=eglCreatePbufferSurface_fn(display,config,surface_attributes);CHECK(window!=EGL_NO_SURFACE);CHECK(eglMakeCurrent_fn(display,window,window,context));
    const auto *renderer=reinterpret_cast<const char *>(doa3_gl::api.glGetString(GL_RENDERER));
    CHECK(renderer&&(argc==1||std::strstr(renderer,"ANGLE")));
    std::printf("Test renderer: %s\n",renderer);
    RecompD3dPresenter p;CHECK(surface(p.main,720,480,1));doa3_gl::api.glClearColor(1,0,0,1);doa3_gl::api.glClear(GL_COLOR_BUFFER_BIT);
    Surface offscreen;CHECK(surface(offscreen,64,32,1));doa3_gl::api.glClearColor(0,0,1,1);doa3_gl::api.glClear(GL_COLOR_BUFFER_BIT);p.targets.emplace(4096,offscreen);p.target_bytes=64*32*4;
    CHECK(surface(p.feedback,64,32,1));Depth depth;doa3_gl::api.glGenRenderbuffers(1,&depth.buffer);p.depths.emplace(0,depth);p.current=&p.main;
    for(unsigned scale:{3u,1u,2u,3u,1u}){
        CHECK(resize_surfaces(p,scale));CHECK(p.render_scale==scale&&p.main.pixel_width==720*scale&&p.main.pixel_height==480*scale);
        CHECK(p.targets.at(4096).pixel_width==64*scale&&p.targets.at(4096).pixel_height==32*scale);
        CHECK(p.target_bytes==64*32*4*scale*scale&&p.current==nullptr&&p.depths.empty()&&!p.feedback.fbo);
        CHECK(pixel(p.main,255,0,0)&&pixel(p.targets.at(4096),0,0,255));
    }
    GLuint previous=p.main.fbo,old_target=p.targets.at(4096).fbo;
    CHECK(!resize_surfaces(p,0));while(doa3_gl::api.glGetError()!=GL_NO_ERROR){}
    CHECK(p.main.fbo==previous&&p.targets.at(4096).fbo==old_target&&p.render_scale==1);
    CHECK(pixel(p.main,255,0,0)&&pixel(p.targets.at(4096),0,0,255));
    destroy(p.main);destroy(p.targets.at(4096));p.targets.clear();
    eglMakeCurrent_fn(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroySurface_fn(display,window);eglDestroyContext_fn(display,context);eglTerminate_fn(display);
    if(egl_library)dlclose(egl_library);
    if(gles_library)dlclose(gles_library);
    std::puts("Live display: real GPU 1x/2x/3x transitions preserve main/offscreen pixels; failed allocation retains old targets.");return 0;
}
