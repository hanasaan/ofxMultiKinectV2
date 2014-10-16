#version 120
#extension GL_EXT_gpu_shader4 : enable

struct Parameters
{
  float ab_multiplier;
  vec3 ab_multiplier_per_frq;
  float ab_output_multiplier;
  
  vec3 phase_in_rad;
  
  float joint_bilateral_ab_threshold;
  float joint_bilateral_max_edge;
  float joint_bilateral_exp;
  mat3 gaussian_kernel;
  
  float phase_offset;
  float unambigious_dist;
  float individual_ab_threshold;
  float ab_threshold;
  float ab_confidence_slope;
  float ab_confidence_offset;
  float min_dealias_confidence;
  float max_dealias_confidence;
  
  float edge_ab_avg_min_value;
  float edge_ab_std_dev_threshold;
  float edge_close_delta_threshold;
  float edge_far_delta_threshold;
  float edge_max_delta_threshold;
  float edge_avg_delta_threshold;
  float max_edge_count;
  
  float min_depth;
  float max_depth;
};

uniform sampler2DRect P0Table0;
uniform sampler2DRect P0Table1;
uniform sampler2DRect P0Table2;
uniform sampler2DRect Lut11to16;
uniform sampler2DRect Data1;
uniform sampler2DRect Data2;
uniform sampler2DRect ZTable;

uniform Parameters Params;

#define M_PI 3.1415926535897932384626433832795

int data(ivec2 uv)
{
    if (uv.y >= 2120) {
        uv.y -= 2120;
        return int(texelFetch2DRect(Data2, uv).x * 65536.0);
    } else {
        return int(texelFetch2DRect(Data1, uv).x * 65536.0);
    }
}

float decode_data(ivec2 uv, int sub)
{
//  int row_idx = 424 * sub + (uv.y < 212 ? (uv.y + 212) : (423 - uv.y));
    int row_idx = 424 * sub + (uv.y >= 212 ? (635 - uv.y) : (uv.y));

  int m = int(0xffffffff);
  int bitmask = (((1 << 2) - 1) << 7) & m;
  int idx = (((uv.x >> 2) + ((uv.x << 7) & bitmask)) * 11) & m;

  int col_idx = idx >> 4;
  int upper_bytes = idx & 15;
  int lower_bytes = 16 - upper_bytes;

  ivec2 data_idx0 = ivec2(col_idx, row_idx);
  ivec2 data_idx1 = ivec2(col_idx + 1, row_idx);

  int lut_idx = (uv.x < 1 || 510 < uv.x || col_idx > 352) ? 0 : (((data(data_idx0) >> upper_bytes) | (data(data_idx1) << lower_bytes)) & 2047);
    
    return float(int(texelFetch2DRect(Lut11to16, ivec2(int(lut_idx), 0)).x * 65536.0) - 32768);
}

vec2 processMeasurementTriple(in ivec2 uv, in sampler2DRect p0table, in int offset, in float ab_multiplier_per_frq, inout bool saturated)
{
    float p0 = -float(texelFetch2DRect(p0table, uv).x) * 2.0 * M_PI;
  
  vec3 v = vec3(decode_data(uv, offset + 0), decode_data(uv, offset + 1), decode_data(uv, offset + 2));
  
  saturated = saturated && any(greaterThan(v, vec3(32767.0)));
  
  float a = dot(v, cos( p0 + Params.phase_in_rad)) * ab_multiplier_per_frq;
  float b = dot(v, sin(-p0 - Params.phase_in_rad)) * ab_multiplier_per_frq;
  
  return vec2(a, b);
}

void main(void)
{
  ivec2 uv = ivec2(gl_TexCoord[0].x, gl_TexCoord[0].y);
    
  bool valid_pixel = 0.0 < texelFetch2DRect(ZTable, uv).x;
  bvec3 saturated = bvec3(valid_pixel);
  
  vec2 ab0 = processMeasurementTriple(uv, P0Table0, 0, Params.ab_multiplier_per_frq.x, saturated.x);
  vec2 ab1 = processMeasurementTriple(uv, P0Table1, 3, Params.ab_multiplier_per_frq.y, saturated.y);
  vec2 ab2 = processMeasurementTriple(uv, P0Table2, 6, Params.ab_multiplier_per_frq.z, saturated.z);
  
  bvec3 invalid_pixel = bvec3(!valid_pixel);
    vec3 invalid_pixelf = vec3(invalid_pixel);
  vec3 A    = mix(vec3(ab0.x, ab1.x, ab2.x), vec3(0.0), invalid_pixelf);
  vec3 B    = mix(vec3(ab0.y, ab1.y, ab2.y), vec3(0.0), invalid_pixelf);
  vec3 Norm = sqrt(A * A + B * B);
  
    vec3 saturatedf = vec3(saturated);
  A = mix(A, vec3(0.0), saturatedf);
  B = mix(B, vec3(0.0), saturatedf);
  
    float Infrared = min(dot(mix(Norm, vec3(65536.0), saturatedf), vec3(0.333333333  * Params.ab_multiplier * Params.ab_output_multiplier)), 65536.0);

    gl_FragData[0] = vec4(A, 1.0);
    gl_FragData[1] = vec4(B, 1.0);
    gl_FragData[2] = vec4(Norm, 1.0);
    gl_FragData[3] = vec4(Infrared/65536.0);
}
