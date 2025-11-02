RWBuffer<uint> screen_buf : register(u0);
RWBuffer<min16float> z_buf : register(u1);
StructuredBuffer<float> p : register(t0);
StructuredBuffer<float> c : register(t1);
StructuredBuffer<uint> misc : register(t2);
StructuredBuffer<float> tri_meta : register(t3);
StructuredBuffer<float> uniforms : register(t4);
StructuredBuffer<uint> texture_custom : register(t5);
groupshared half gs_tri[9];
groupshared half col_tri[13];
groupshared half tri_meta_[5];

[numthreads(64, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
    uint i = threadID.x;
    uint local = i % 64;
    uint t = i * rcp(64.0f);
    uint qx = local % 8;
    uint qy = local * rcp(8.0f);
    uint pb = t * 9;
    uint cb = t * 13;
    uint tb = t * 5;
    
    if (local < 9) {
        gs_tri[local] = p[pb + local];
    }
    
    if (local < 13) {
        col_tri[local] = c[cb + local];
    }
    
    if (local < 5) {
        tri_meta_[local] = tri_meta[tb + local];
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if (i < (misc[2] * 64)) {
        int m0 = misc[0];
        int m1 = misc[1];
        int m3 = misc[3];
        int m4 = misc[4];
        min16float2 v1 = min16float2(gs_tri[0], gs_tri[1]);
        min16float2 v2 = min16float2(gs_tri[3], gs_tri[4]);
        min16float2 v3 = min16float2(gs_tri[6], gs_tri[7]);
        min16float3 zs = min16float3(gs_tri[2], gs_tri[5], gs_tri[8]);
        min16float3 c1 = min16float3(col_tri[0], col_tri[1], col_tri[2]);
        min16float3 c2 = min16float3(col_tri[4], col_tri[5], col_tri[6]);
        min16float3 c3 = min16float3(col_tri[8], col_tri[9], col_tri[10]);
        min16float3 as = min16float3(col_tri[3], col_tri[7], col_tri[11]);
        min16float l1, l2, l3;
        int hit = 0;
        
        min16float t5 = tri_meta_[0];
        min16float t51 = tri_meta_[1];
        min16float t54 = tri_meta_[4];
        min16float t52 = tri_meta_[2];
        min16float t53 = tri_meta_[3];
        
        int width = int(t52) - int(t5);
        int height = int(t53) - int(t51);
        
        int minx = clamp(int(t5) + floor((min16float(qx) * rcp(8.0f)) * min16float(width)), 0, m0);
        int miny = clamp(int(t51) + floor((min16float(qy) * rcp(8.0f)) * min16float(height)), 0, m1);
        int maxx = clamp(minx + ceil(min16float(width) * rcp(8.0f)), 0, m0);
        int maxy = clamp(miny + ceil(min16float(height) * rcp(8.0f)), 0, m1);
        
        min16float v3ymv2y = v3.y - v2.y;
        min16float v1ymv3y = v1.y - v3.y;
        min16float v3xmv2x = v3.x - v2.x;
        min16float v1xmv3x = v1.x - v3.x;
        min16float l1_xstep = (v3.y - v2.y) * t54; // change in l1 per +1 x
        min16float l1_ystep = -(v3.x - v2.x) * t54; // change in l1 per +1 y
        min16float l2_xstep = (v1.y - v3.y) * t54;
        min16float l2_ystep = -(v1.x - v3.x) * t54;
        
        l1 = ((min16float(minx) - v2.x) * v3ymv2y - (min16float(miny) - v2.y) * v3xmv2x) * t54;
        l2 = ((min16float(minx) - v3.x) * v1ymv3y - (min16float(miny) - v3.y) * v1xmv3x) * t54;
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        l1 = ((min16float(maxx) - v2.x) * v3ymv2y - (min16float(miny) - v2.y) * v3xmv2x) * t54;
        l2 = ((min16float(maxx) - v3.x) * v1ymv3y - (min16float(miny) - v3.y) * v1xmv3x) * t54;
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        l1 = ((min16float(minx) - v2.x) * v3ymv2y - (min16float(maxy) - v2.y) * v3xmv2x) * t54;
        l2 = ((min16float(minx) - v3.x) * v1ymv3y - (min16float(maxy) - v3.y) * v1xmv3x) * t54;
        l3 = 1.0f - l2 - l1;
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        l1 = ((min16float(maxx) - v2.x) * v3ymv2y - (min16float(maxy) - v2.y) * v3xmv2x) * t54;
        l2 = ((min16float(maxx) - v3.x) * v1ymv3y - (min16float(maxy) - v3.y) * v1xmv3x) * t54;
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        if (hit == 0)
            return;
        
        hit = 4;
        
        if ((l1 * zs.x + l2 * zs.y + l3 * zs.z) > z_buf[m0 * maxy + maxx]) // maxx maxy
            hit--;
        
        l1 -= l1_ystep * (maxy - miny);
        l2 -= l2_ystep * (maxy - miny);
        l3 = 1.0f - l2 - l1; // maxx miny
        
        if ((l1 * zs.x + l2 * zs.y + l3 * zs.z) > z_buf[m0 * miny + maxx])
            hit--;
        
        l1 += l1_ystep * (maxy - miny);
        l2 += l2_ystep * (maxy - miny);
        l1 -= l1_xstep * (maxx - minx);
        l2 -= l2_xstep * (maxx - minx); // maxx miny
        l3 = 1.0f - l2 - l1; // minx maxy
        
        if ((l1 * zs.x + l2 * zs.y + l3 * zs.z) > z_buf[m0 * maxy + minx])
            hit--;
        
        l1 -= l1_ystep * (maxy - miny);
        l2 -= l2_ystep * (maxy - miny);
        l3 = 1.0f - l2 - l1;
        
        if ((l1 * zs.x + l2 * zs.y + l3 * zs.z) > z_buf[m0 * miny + minx])
            hit--;
    
        if (hit == 0)
            return;
        
        if (c[t * 13 + 12] == 1.0f) {
            for (int x = minx; x < maxx; x++) {
                for (int y = miny; y < maxy; y++) {
                    l1 += l1_ystep;
                    l2 += l2_ystep;
                    l3 = 1.0f - l2 - l1;
                    
                    int idx = m0 * y + x;

                    min16float depth = l1 * zs.x + l2 * zs.y + l3 * zs.z;
                    
                    if ((l1 >= 0.0f) && (l2 >= 0.0f) && (l3 >= 0.0f)) {
                        if (z_buf[idx] > depth) {
                            screen_buf[idx] = int(saturate(l1 * c1.r + l2 * c2.r + l3 * c3.r) * 255u) << 16 |
                                              int(saturate(l1 * c1.g + l2 * c2.g + l3 * c3.g) * 255u) << 8 |
                                              int(saturate(l1 * c1.b + l2 * c2.b + l3 * c3.b) * 255u);
                            z_buf[idx] = depth;
                        }
                    }
                }
                
                l1 += l1_xstep;
                l2 += l2_xstep;
                l1 -= (maxy - miny) * l1_ystep;
                l2 -= (maxy - miny) * l2_ystep;
            }
        } else if (c[t * 13 + 12] == 0.0f) {
            for (int x = minx; x < maxx; x++) {
                for (int y = miny; y < maxy; y++) {
                    l1 += l1_ystep;
                    l2 += l2_ystep;
                    l3 = 1.0f - l2 - l1;
                    
                    int idx = m0 * y + x;

                    min16float depth = l1 * zs.x + l2 * zs.y + l3 * zs.z;
                    
                    if ((l1 >= 0.0f) && (l2 >= 0.0f) && (l3 >= 0.0f)) {
                        if (z_buf[idx] > depth) {
                            uint u = round((l1 * c1.r + l2 * c2.r + l3 * c3.r) * (m3 - 1));
                            uint v = round((l1 * c1.g + l2 * c2.g + l3 * c3.g) * (m4 - 1));
                            uint color = texture_custom[u + v * m3];
                            uint r = ((color >> 24) & 0xFF);
                            uint g = ((color >> 16) & 0xFF);
                            uint b = ((color >> 8) & 0xFF);
                            screen_buf[x + y * m0] = (r << 16) | (g << 8) | b;
                            z_buf[idx] = depth;
                        }
                    }
                }
                
                l1 += l1_xstep;
                l2 += l2_xstep;
                l1 -= (maxy - miny) * l1_ystep;
                l2 -= (maxy - miny) * l2_ystep;
            }
        }
    }
}   
