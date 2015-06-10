//
//  GpuRegistration.cpp
//  example_registration
//
//  Created by Yuya Hanai on 2015/06/11.
//
//

#include "GpuRegistration.h"

#define STRINGIFY(x) #x

void GpuRegistration::setup(ofProtonect2* protonect, float scale) {
    ldepth_index = 0;
    depth = protonect->getIrCameraParams();
    color = protonect->getColorCameraParams();
    int w = scale * 512;
    int h = scale * 424;
    depth_to_color_map.allocate(w, h, 3);
    {
        ofFbo::Settings settings;
        settings.width = w;
        settings.height = h;
        settings.minFilter = GL_NEAREST;
        settings.maxFilter = GL_NEAREST;
        settings.colorFormats.push_back(GL_RGB);
        settings.colorFormats.push_back(GL_RGB32F_ARB);
        settings.depthStencilAsTexture = true;
        settings.useDepth = true;
        settings.useStencil = true;
        regist_fbo.allocate(settings);
    }
    masked_regist_fbo.allocate(w, h, GL_RGB);
    
    const float mask_scale = 3.0;
    
    ldepth_work_fbo.allocate(1920 / mask_scale, 1080 / mask_scale, GL_RGB32F_ARB);
    ldepth_work_fbo.getTextureReference().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    for (ofFbo& ldepth_fbo : ldepth_fbos) {
        ldepth_fbo.allocate(1920 / mask_scale, 1080 / mask_scale, GL_RGB32F_ARB);
        ldepth_fbo.getTextureReference().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    }
    
    float mx, my;
    float rx, ry;
    int ptr = 0;
    float inv_sc = 1.0f / scale;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++, ptr+=3) {
            undistort_depth(inv_sc*x,inv_sc*y,mx,my);
            depth_to_color(mx,my,rx,ry);
            depth_to_color_map[ptr]   = rx;
            depth_to_color_map[ptr+1] = ry;
            depth_to_color_map[ptr+2] = 0;
            
            m.addVertex(ofVec3f(x, y, 0));
            m.addTexCoord(ofVec2f(x,y));
        }
    
    m.setMode(OF_PRIMITIVE_POINTS);
    m.enableTextures();
    depth_to_color_map_tex.loadData(depth_to_color_map);
    
    static string cshader_vert_str =
    STRINGIFY(
              uniform sampler2DRect tex; // cmap
              uniform float inv_mask_scale;
              void main()
              {
                  vec4 cmap = texture2DRect(tex, gl_MultiTexCoord0.xy);
                  float dz = cmap.z;
                  if (dz == 0.0) {
                      dz = 65535.0;
                  }
                  vec4 worldpos = vec4(cmap.x * inv_mask_scale,
                                       cmap.y * inv_mask_scale, -dz / 65535.0, 1.0);
                  gl_Position = gl_ModelViewProjectionMatrix * worldpos;
                  gl_FrontColor = cmap;
                  gl_TexCoord[0] = gl_MultiTexCoord0;
              }
              );
    
    static string cshader_frag_str =
    STRINGIFY(
              uniform sampler2DRect tex; // cmap
              void main()
              {
                  gl_FragColor = texture2DRect(tex, gl_TexCoord[0].xy);
              }
              );
    
    static string rshader_str =
    STRINGIFY(
              uniform sampler2DRect depth;
              uniform sampler2DRect color;
              uniform sampler2DRect tex; // map
              uniform float inv_scale;
              uniform float color_shift_m;
              uniform float color_shift_md;
              uniform float color_fx;
              uniform float color_fy;
              uniform float color_cx;
              uniform float color_cy;
              
              void main()
              {
                  vec4 map_xy = texture2DRect(tex, gl_TexCoord[0].xy);
                  float rx = map_xy.x;
                  float ry = map_xy.y;
                  
                  float dz = texture2DRect(depth, gl_TexCoord[0].xy * inv_scale).r;
                  if (dz == 0.0) {
                      gl_FragData[0] = vec4(0.0);
                      gl_FragData[1] = vec4(0.0);
                      return;
                  }
                  
                  rx += (color_shift_m / dz) - color_shift_md;
                  
                  float cx = rx * color_fx + color_cx;
                  float cy = ry * color_fy + color_cy;
                  vec2 cxy = vec2(cx, cy);

                  gl_FragData[1] = vec4(cx, cy, dz, 1.0);
                  if (any(notEqual(clamp(cxy, vec2(0.0, 0.0), vec2(1920.0, 1080.0)) - cxy, vec2(0.0, 0.0))))
                  {
                      gl_FragData[0] = vec4(0.0, 0.0, 0.0, 1.0);
                  } else {
                      gl_FragData[0] = texture2DRect(color, cxy);
                  }
                  
              }
              );
    
    static string fillhole_shader_str =
    STRINGIFY(
              uniform sampler2DRect tex; // regist fbo #1
              
              void main()
              {
                  vec2 co = gl_TexCoord[0].xy;
                  float step = 2.0;
                  float dz_color = texture2DRect(tex, co).z;
                  float dz_color_1 = texture2DRect(tex, co + vec2(step, 0.0)).z;
                  float dz_color_2 = texture2DRect(tex, co + vec2(-step, 0.0)).z;
                  float dz_color_3 = texture2DRect(tex, co + vec2(0.0, -step)).z;
                  float dz_color_4 = texture2DRect(tex, co + vec2(0.0, step)).z;
                  float dz_color_5 = texture2DRect(tex, co + vec2(step, step)).z;
                  float dz_color_6 = texture2DRect(tex, co + vec2(-step, step)).z;
                  float dz_color_7 = texture2DRect(tex, co + vec2(step, -step)).z;
                  float dz_color_8 = texture2DRect(tex, co + vec2(-step, -step)).z;
                  dz_color = max(dz_color, dz_color_1);
                  dz_color = max(dz_color, dz_color_2);
                  dz_color = max(dz_color, dz_color_3);
                  dz_color = max(dz_color, dz_color_4);
                  dz_color = max(dz_color, dz_color_5);
                  dz_color = max(dz_color, dz_color_6);
                  dz_color = max(dz_color, dz_color_7);
                  dz_color = max(dz_color, dz_color_8);
                  
                  vec4 frag = texture2DRect(tex, co);
                  frag.z = dz_color;
                  gl_FragColor = frag;
              }
              );
    
    static string mask_shader_str =
    STRINGIFY(
              uniform sampler2DRect color;
              uniform sampler2DRect color_depth;
              uniform sampler2DRect color_depth_ref;
              uniform sampler2DRect tex; // regist fbo #1
              uniform float inv_mask_scale;
              
              void main()
              {
                  vec4 c = texture2DRect(tex, gl_TexCoord[0].xy);
                  float dz = c.z;
                  float cx = c.x;
                  float cy = c.y;
                  vec2 cxy = vec2(cx, cy);
                  if (any(notEqual(clamp(cxy, vec2(0.0, 0.0), vec2(1920.0, 1080.0)) - cxy, vec2(0.0, 0.0))))
                  {
                      gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
                      return;
                  }
                  
                  vec2 co = vec2(cx, cy) * inv_mask_scale;
                  float step = 2.0;
                  float dz_color = texture2DRect(color_depth, co).z;
                  float dz_color_ref = texture2DRect(color_depth_ref, co).z;
                  dz_color = max(dz_color, dz_color_ref);
                  
                  if (dz < dz_color + 50.0) {
                      gl_FragColor = texture2DRect(color, vec2(cx, cy));
                  } else {
                      gl_FragColor = vec4(0.0);
                  }
              }
              );
    
    regist_shader.setupShaderFromSource(GL_FRAGMENT_SHADER, rshader_str);
    regist_shader.linkProgram();
    regist_shader.begin();
    regist_shader.setUniform1f("inv_scale", 1.0 / scale);
    regist_shader.setUniform1f("color_shift_m", color.shift_m);
    regist_shader.setUniform1f("color_shift_md", color.shift_m / color.shift_d);
    regist_shader.setUniform1f("color_fx", color.fx);
    regist_shader.setUniform1f("color_fy", color.fy);
    regist_shader.setUniform1f("color_cx", color.cx);
    regist_shader.setUniform1f("color_cy", color.cy);
    regist_shader.end();
    
    c_vert_shader.setupShaderFromSource(GL_VERTEX_SHADER, cshader_vert_str);
    c_vert_shader.setupShaderFromSource(GL_FRAGMENT_SHADER, cshader_frag_str);
    c_vert_shader.linkProgram();
    
    c_vert_shader.begin();
    c_vert_shader.setUniform1f("inv_mask_scale", 1.0 / mask_scale);
    c_vert_shader.end();
    
    c_mask_shader.setupShaderFromSource(GL_FRAGMENT_SHADER, mask_shader_str);
    c_mask_shader.linkProgram();
    
    c_mask_shader.begin();
    c_mask_shader.setUniform1f("inv_mask_scale", 1.0 / mask_scale);
    c_mask_shader.end();
    
    depth_fill_hole_shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fillhole_shader_str);
    depth_fill_hole_shader.linkProgram();
}

void GpuRegistration::update(const ofFloatPixels& depth_pix, const ofPixels& color_pix, bool process_occlusion_mask) {
    depth_tex.loadData(depth_pix);
    color_tex.loadData(color_pix);
    update(depth_tex, color_tex, process_occlusion_mask);
}

void GpuRegistration::update(ofTexture& depth_tex, ofTexture& color_tex, bool process_occlusion_mask) {
    regist_fbo.begin();
    regist_fbo.activateAllDrawBuffers();
    {
        ofClear(0);
        regist_shader.begin();
        regist_shader.setUniformTexture("depth", depth_tex, 1);
        regist_shader.setUniformTexture("color", color_tex, 2);
        depth_to_color_map_tex.draw(0, 0);
        regist_shader.end();
    }
    regist_fbo.end();
    
    if (!process_occlusion_mask) {
        return;
    }
    
    ldepth_work_fbo.begin();
    {
        ofClear(0);
        ofPushStyle();
        ofEnableDepthTest();
        glPointSize(10);
        glDisable(GL_POINT_SMOOTH);
        ofSetupScreenOrtho();
        c_vert_shader.begin();
        c_vert_shader.setUniformTexture("tex", regist_fbo.getTextureReference(1), 0);
        m.draw();
        c_vert_shader.end();
        ofDisableDepthTest();
        ofPopStyle();
    }
    ldepth_work_fbo.end();
    
    ofFbo& ldepth_fbo = ldepth_fbos[ldepth_index];
    ldepth_fbo.begin();
    {
        depth_fill_hole_shader.begin();
        ldepth_work_fbo.draw(0, 0);
        depth_fill_hole_shader.end();
    }
    ldepth_fbo.end();
    
    masked_regist_fbo.begin();
    {
        ofClear(0);
        c_mask_shader.begin();
        c_mask_shader.setUniformTexture("color_depth", ldepth_fbo.getTextureReference(), 1);
        c_mask_shader.setUniformTexture("color_depth_ref", ldepth_fbos[1-ldepth_index].getTextureReference(), 2);
        c_mask_shader.setUniformTexture("color", color_tex, 3);
        regist_fbo.getTextureReference(1).draw(0, 0);
        c_mask_shader.end();
    }
    masked_regist_fbo.end();
    ldepth_index = 1 - ldepth_index;
}

void GpuRegistration::undistort_depth(float x, float y, float& mx, float& my)
{
    float dx = (x - depth.cx) / depth.fx;
    float dy = (y - depth.cy) / depth.fy;
    
    float ps = (dx * dx) + (dy * dy);
    float qs = ((ps * depth.k3 + depth.k2) * ps + depth.k1) * ps + 1.0;
    for (int i = 0; i < 9; i++) {
        float qd = ps / (qs * qs);
        qs = ((qd * depth.k3 + depth.k2) * qd + depth.k1) * qd + 1.0;
    }
    
    mx = dx / qs;
    my = dy / qs;
}

void GpuRegistration::depth_to_color(float mx, float my, float& rx, float& ry)
{
    static const float depth_q = 0.01;
    static const float color_q = 0.002199;
    
    mx *= depth.fx * depth_q;
    my *= depth.fy * depth_q;
    
    float wx =
    (mx * mx * mx * color.mx_x3y0) + (my * my * my * color.mx_x0y3) +
    (mx * mx * my * color.mx_x2y1) + (my * my * mx * color.mx_x1y2) +
    (mx * mx * color.mx_x2y0) + (my * my * color.mx_x0y2) + (mx * my * color.mx_x1y1) +
    (mx * color.mx_x1y0) + (my * color.mx_x0y1) + (color.mx_x0y0);
    
    float wy =
    (mx * mx * mx * color.my_x3y0) + (my * my * my * color.my_x0y3) +
    (mx * mx * my * color.my_x2y1) + (my * my * mx * color.my_x1y2) +
    (mx * mx * color.my_x2y0) + (my * my * color.my_x0y2) + (mx * my * color.my_x1y1) +
    (mx * color.my_x1y0) + (my * color.my_x0y1) + (color.my_x0y0);
    
    rx = wx / (color.fx * color_q);
    ry = wy / (color.fx * color_q);
}



