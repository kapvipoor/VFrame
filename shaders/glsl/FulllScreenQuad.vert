#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

layout (location = 0) out vec2 outUV;

out gl_PerVertex { vec4 gl_Position; };

void main() 
{
	//See: https://web.archive.org/web/20140719063725/http://www.altdev.co/2011/08/08/interesting-vertex-shader-trick/
    //Also: https://michaldrobot.com/2014/04/01/gcn-execution-patterns-in-full-screen-passes/
    //
    // |.
    // |_`.  
    // |  |`.
    // '--'--` 
    //
    //       1  
    //    ( 0, 2)
    //    [-1, 3]   [ 3, 3]
    //        .
    //        |`.
    //        |  `.  
    //        |    `.
    //        '------`
    //       0         2
    //    ( 0, 0)   ( 2, 0)
    //    [-1,-1]   [ 3,-1]
    //
    //    ID=0 -> Pos=[-1,-1], Tex=(0,0)
    //    ID=1 -> Pos=[-1, 3], Tex=(0,2)
    //    ID=2 -> Pos=[ 3,-1], Tex=(2,0)

    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);
}