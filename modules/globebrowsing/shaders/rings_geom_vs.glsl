/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2018                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#version __CONTEXT__

#include "PowerScaling/powerScaling_vs.hglsl"

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_st;

out vec2 vs_st;
out float vs_screenSpaceDepth;
out vec4 vs_positionViewSpace;

uniform dmat4 modelMatrix;
uniform dmat4 modelViewMatrix;
uniform dmat4 projectionMatrix;

void main() {
    vs_st = in_st;

    dvec4 positionViewSpace = modelViewMatrix * dvec4(in_position.xy, 1.0, 1.0);
    vec4 positionClipSpace = vec4(projectionMatrix * positionViewSpace);
    vec4 positionClipSpaceZNorm = z_normalization(positionClipSpace);
    
    // if (positionClipSpaceZNorm.z > 1.0) {
    //     positionClipSpaceZNorm.z = float(double(positionClipSpaceZNorm.z) / double(1E30));
    // } else {
    //     positionClipSpaceZNorm.z = positionClipSpaceZNorm.z - 1.0;
    // }


    vs_screenSpaceDepth  = positionClipSpaceZNorm.w;
    vs_positionViewSpace = vec4(positionViewSpace);
    
    gl_Position = positionClipSpaceZNorm;
}