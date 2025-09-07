RWBuffer<uint> screen_buf : register(u0);
RWBuffer<float> z_buf : register(u1);
StructuredBuffer<float> p : register(t0);
StructuredBuffer<float> c : register(t1);
StructuredBuffer<uint> misc : register(t2);
StructuredBuffer<float> tri_meta : register(t3);
StructuredBuffer<uint> indices : register(t4);
groupshared float gs_tri[9];
groupshared float col_tri[13];
groupshared float tri_meta_[5];

[numthreads(144, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
    uint i = threadID.x;
    uint local = i % 144;
    uint t = i * rcp(144.0f);
    uint qx = local % 12;
    uint qy = local * rcp(12.0f);
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
    
    if (i < (misc[2] * 144)) {
        uint m0 = misc[0];
        uint m1 = misc[1];
        float2 v1 = float2(gs_tri[0], gs_tri[1]);
        float2 v2 = float2(gs_tri[3], gs_tri[4]);
        float2 v3 = float2(gs_tri[6], gs_tri[7]);
        float3 zs = float3(gs_tri[2], gs_tri[5], gs_tri[8]);
        float3 c1 = float3(col_tri[0], col_tri[1], col_tri[2]);
        float3 c2 = float3(col_tri[4], col_tri[5], col_tri[6]);
        float3 c3 = float3(col_tri[8], col_tri[9], col_tri[10]);
        float l1, l2, l3;
        uint hit = 0;
        
        float t5 = tri_meta_[0];
        float t51 = tri_meta_[1];
        float t54 = tri_meta_[4];
        float t52 = tri_meta_[2];
        float t53 = tri_meta_[3];
        
        uint width = uint(t52) - uint(t5);
        uint height = uint(t53) - uint(t51);
        
        uint minx = clamp(uint(t5) + floor((float(qx) * rcp(12.0f)) * float(width)), 0u, m0);
        uint miny = clamp(uint(t51) + floor((float(qy) * rcp(12.0f)) * float(height)), 0u, m1);
        uint maxx = clamp(minx + ceil(float(width) * rcp(12.0f)), 0u, m0);
        uint maxy = clamp(miny + ceil(float(height) * rcp(12.0f)), 0u, m1);
        
        float v3ymv2y = v3.y - v2.y;
        float v1ymv3y = v1.y - v3.y;
        float v3xmv2x = v3.x - v2.x;
        float v1xmv3x = v1.x - v3.x;
        float l1_xstep = (v3.y - v2.y) * t54; // change in l1 per +1 x
        float l1_ystep = -(v3.x - v2.x) * t54; // change in l1 per +1 y
        float l2_xstep = (v1.y - v3.y) * t54;
        float l2_ystep = -(v1.x - v3.x) * t54;
        
        l1 = ((float(minx) - v2.x) * v3ymv2y - (float(miny) - v2.y) * v3xmv2x) * t54;
        l2 = ((float(minx) - v3.x) * v1ymv3y - (float(miny) - v3.y) * v1xmv3x) * t54;
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        l1 += l1_xstep * (maxx - minx);
        l2 += l2_xstep * (maxx - minx); // maxx miny
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        l1 -= l1_xstep * (maxx - minx);
        l2 -= l2_xstep * (maxx - minx);
        l1 += l1_ystep * (maxy - miny);
        l2 += l2_ystep * (maxy - miny); // minx maxy
        l3 = 1.0f - l2 - l1;
        
        if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f)
            hit++;
        
        l1 += l1_xstep * (maxx - minx);
        l2 += l2_xstep * (maxx - minx); // maxx maxy
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
            for (uint x = minx; x < maxx; x++) {
                for (uint y = miny; y < maxy; y++) {
                    l1 += l1_ystep;
                    l2 += l2_ystep;
                    l3 = 1.0f - l2 - l1;
                    
                    uint idx = m0 * y + x;
                    
                    if (l1 >= 0.0f && l2 >= 0.0f && l3 >= 0.0f && z_buf[idx] > (l1 * zs.x + l2 * zs.y + l3 * zs.z)) {
                        uint color = uint(saturate(l1 * c1.r + l2 * c2.r + l3 * c3.r) * 255u) << 16 |
                                     uint(saturate(l1 * c1.g + l2 * c2.g + l3 * c3.g) * 255u) << 8 |
                                     uint(saturate(l1 * c1.b + l2 * c2.b + l3 * c3.b) * 255u);
                        float depth = l1 * zs.x + l2 * zs.y + l3 * zs.z;
                        
                        screen_buf[idx] = color;
                        z_buf[idx] = depth;
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
