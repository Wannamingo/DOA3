// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <GLES3/gl3.h>
#include <cstdio>

// One dispatch table for presenter and buffer cache. The default entries use
// Android GLES; an imported EGL context needs its matching GLES entry points.
#define DOA3_GLES_FUNCTIONS(X) \
    X(glActiveTexture) \
    X(glAttachShader) \
    X(glBindBuffer) \
    X(glBindBufferBase) \
    X(glBindFramebuffer) \
    X(glBindRenderbuffer) \
    X(glBindTexture) \
    X(glBindVertexArray) \
    X(glBlendColor) \
    X(glBlendEquation) \
    X(glBlendFunc) \
    X(glBlitFramebuffer) \
    X(glBufferData) \
    X(glBufferSubData) \
    X(glCheckFramebufferStatus) \
    X(glClear) \
    X(glClearColor) \
    X(glClearDepthf) \
    X(glClearStencil) \
    X(glColorMask) \
    X(glCompileShader) \
    X(glCreateProgram) \
    X(glCreateShader) \
    X(glCullFace) \
    X(glDeleteBuffers) \
    X(glDeleteFramebuffers) \
    X(glDeleteProgram) \
    X(glDeleteRenderbuffers) \
    X(glDeleteShader) \
    X(glDeleteTextures) \
    X(glDeleteVertexArrays) \
    X(glDepthFunc) \
    X(glDepthMask) \
    X(glDisable) \
    X(glDisableVertexAttribArray) \
    X(glDrawArrays) \
    X(glDrawElements) \
    X(glEnable) \
    X(glEnableVertexAttribArray) \
    X(glFramebufferRenderbuffer) \
    X(glFramebufferTexture2D) \
    X(glFrontFace) \
    X(glGenBuffers) \
    X(glGenFramebuffers) \
    X(glGenRenderbuffers) \
    X(glGenTextures) \
    X(glGenVertexArrays) \
    X(glGetActiveUniformBlockiv) \
    X(glGetError) \
    X(glGetIntegerv) \
    X(glGetProgramInfoLog) \
    X(glGetProgramiv) \
    X(glGetShaderInfoLog) \
    X(glGetShaderiv) \
    X(glGetString) \
    X(glGetStringi) \
    X(glGetUniformBlockIndex) \
    X(glGetUniformLocation) \
    X(glLinkProgram) \
    X(glPixelStorei) \
    X(glRenderbufferStorage) \
    X(glShaderSource) \
    X(glStencilFunc) \
    X(glStencilMask) \
    X(glStencilOp) \
    X(glTexImage2D) \
    X(glTexParameterfv) \
    X(glTexParameteri) \
    X(glTexStorage2D) \
    X(glTexSubImage2D) \
    X(glUniform1i) \
    X(glUniformBlockBinding) \
    X(glUseProgram) \
    X(glVertexAttribPointer) \
    X(glViewport)
namespace doa3_gl {
struct Api {
#define DOA3_GL_ENTRY(name) decltype(&::name) name = &::name;
    DOA3_GLES_FUNCTIONS(DOA3_GL_ENTRY)
#undef DOA3_GL_ENTRY

    template<class Lookup>
    bool load(Lookup lookup) {
        Api candidate;
#define DOA3_GL_LOAD(name) \
        candidate.name = reinterpret_cast<decltype(candidate.name)>(lookup(#name)); \
        if (!candidate.name) { std::fprintf(stderr, "ANGLE missing GLES symbol: %s\n", #name); return false; }
        DOA3_GLES_FUNCTIONS(DOA3_GL_LOAD)
#undef DOA3_GL_LOAD
        *this = candidate;
        return true;
    }
};
// Calls are on the render thread, including destruction. Standalone GPU
// tests intentionally keep the system dispatch table.
inline thread_local Api api;
}
#undef DOA3_GLES_FUNCTIONS
