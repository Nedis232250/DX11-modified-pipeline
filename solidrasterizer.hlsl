RWTexture2D<uint> screen_buf : register(u0);
RWTexture2D<uint> z_buf : register(u1);
StructuredBuffer<float> p : register(t0);
StructuredBuffer<float> c : register(t1);
StructuredBuffer<uint> misc : register(t2);
StructuredBuffer<float> tri_meta : register(t3);
StructuredBuffer<uint> texture_custom : register(t5);
groupshared float gs_tri[9];
groupshared float col_tri[12];
groupshared float tri_meta_[13];
groupshared int misc_[5];

// Variables = registers rap3d
#define xby 8.0f
#define yby 8.0f
#define boxes xby * yby
#define rcp3 rcp(3.0f)
#define bias 0x179000
#define bias2 0x170000
#define max_percision_float32 16777216.0f

float mean2f(float a, float b) {
    return (a + b) * rcp3;
}


[numthreads(uint(xby), uint(yby), 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint3 groupID : SV_GroupThreadID, uint3 group_num : SV_GroupID) {
    uint i = group_num.x * uint(boxes) + (groupID.y * uint(xby) + groupID.x);
    uint local = i % uint(boxes);
    uint t = group_num.x;
    uint qx = groupID.x;
    uint qy = groupID.y;
    uint pb = t * 9;
    uint cbtb = t * 13;
    
    if (local == 0) {
        gs_tri[0] = p[pb];
        gs_tri[1] = p[pb + 1];
        gs_tri[2] = p[pb + 2];
        gs_tri[3] = p[pb + 3];
        gs_tri[4] = p[pb + 4];
        gs_tri[5] = p[pb + 5];
        gs_tri[6] = p[pb + 6];
        gs_tri[7] = p[pb + 7];
        gs_tri[8] = p[pb + 8];
    }
    
    if (local == 1) {
        col_tri[0] = c[cbtb];
        col_tri[1] = c[cbtb + 1];
        col_tri[2] = c[cbtb + 2];
        col_tri[3] = c[cbtb + 3];
        col_tri[4] = c[cbtb + 4];
        col_tri[5] = c[cbtb + 5];
        col_tri[6] = c[cbtb + 6];
        col_tri[7] = c[cbtb + 7];
        col_tri[8] = c[cbtb + 8];
        col_tri[9] = c[cbtb + 9];
        col_tri[10] = c[cbtb + 19];
        col_tri[11] = c[cbtb + 11];
    } 
    
    if (local == 2) {
        tri_meta_[0] = tri_meta[cbtb];
        tri_meta_[1] = tri_meta[cbtb + 1];
        tri_meta_[2] = tri_meta[cbtb + 2];
        tri_meta_[3] = tri_meta[cbtb + 3];
        tri_meta_[4] = tri_meta[cbtb + 4];
        tri_meta_[5] = tri_meta[cbtb + 5];
        tri_meta_[6] = tri_meta[cbtb + 6];
        tri_meta_[7] = tri_meta[cbtb + 7];
        tri_meta_[8] = tri_meta[cbtb + 8];
        tri_meta_[9] = tri_meta[cbtb + 9];
        tri_meta_[10] = tri_meta[cbtb + 10];
        tri_meta_[11] = tri_meta[cbtb + 11];
        tri_meta_[12] = tri_meta[cbtb + 12];
    }
    
    if (local == 3) {
        misc_[0] = misc[0];
        misc_[1] = misc[1];
        misc_[2] = misc[2];
        misc_[3] = misc[3];
        misc_[4] = misc[4];
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    int m0 = misc_[0];
    int m1 = misc_[1];
    int m3 = misc_[3];
    int m4 = misc_[4];
    if (i < (misc_[2] * uint(boxes))) {
        float2 v1 = float2(gs_tri[0], gs_tri[1]);
        float2 v2 = float2(gs_tri[3], gs_tri[4]);
        float2 v3 = float2(gs_tri[6], gs_tri[7]);
        float3 zs = float3(gs_tri[2], gs_tri[5], gs_tri[8]);
        float t52 = tri_meta_[2];
        float t53 = tri_meta_[3];
        float t5 = tri_meta_[0];
        float t51 = tri_meta_[1];
        
        int width = int(t52) - int(t5);
        int height = int(t53) - int(t51);
        
        int minx = int(t5) + floor((float(qx) * rcp(xby)) * float(width));
        int miny = int(t51) + floor((float(qy) * rcp(yby)) * float(height));
        int maxx = minx + ceil(float(width) * rcp(xby));
        int maxy = miny + ceil(float(height) * rcp(yby));
        
        if (minx >= m0 || miny >= m1 || maxy < 0 || maxx < 0)
            return; // completely off-screen
        
        float t54 = tri_meta_[4];
        
        if (rcp(t54) < 0.01) { // degenerate triangles
            return;
        }
        
        uint hit = 0;
        float l1, l2, l3;
        float v3ymv2y = tri_meta_[5];
        float v1ymv3y = tri_meta_[6];
        float v3xmv2x = tri_meta_[7];
        float v1xmv3x = tri_meta_[8];
        uint aabb_width = maxx - minx;
        uint aabb_height = maxy - miny;
        float l1_xstep = tri_meta_[9]; // change in l1 per +1 x
        float l1_ystep = tri_meta_[10]; // change in l1 per +1 y
        float l2_xstep = tri_meta_[11];
        float l2_ystep = tri_meta_[12];
        float mmm_l1_xstep = aabb_width * l1_xstep;
        float mmm_l2_xstep = aabb_width * l2_xstep;
        float mmm_l1_ystep = aabb_height * l1_xstep;
        float mmm_l2_ystep = aabb_height * l2_xstep;
        /*
        int guard = max(aabb_width, aabb_height);
        bool middle = maxx < m0 - guard && maxy < m1 - guard && minx > guard && miny > guard;
        if (middle) {
            if (z_buf[v1] + bias < (max_percision_float32 * zs.x))
                hit++;
            if (z_buf[v2] + bias < (max_percision_float32 * zs.y))
                hit++;
            if (z_buf[v3] + bias < (max_percision_float32 * zs.z))
                hit++;
        
            if (hit > 2) {
                return;
            }
            
            hit = 0;
        
            l1 = ((maxx - v2.x) * v3ymv2y - (maxy - v2.y) * v3xmv2x) * t54;
            l2 = ((maxx - v3.x) * v1ymv3y - (maxy - v3.y) * v1xmv3x) * t54;
            l3 = 1.0f - l2 - l1;
        
            if (z_buf[uint2(maxx, maxy)] + bias2 < (max_percision_float32 * (l1 * zs.x + l2 * zs.y + l3 * zs.z)))
                hit++;
        
            //
            
            l1 -= mmm_l1_xstep;
            l2 -= mmm_l2_xstep;
            l3 = 1.0f - l1 - l2;
        
            if (z_buf[uint2(minx, maxy)] + bias2 < (max_percision_float32 * (l1 * zs.x + l2 * zs.y + l3 * zs.z)))
                hit++;
            
            //
        
            l1 += mmm_l1_xstep;
            l2 += mmm_l2_xstep;
            l1 -= mmm_l1_ystep;
            l2 -= mmm_l2_ystep;
            l3 = 1.0f - l1 - l2;
        
            if (z_buf[uint2(maxx, miny)] + bias2 < (max_percision_float32 * (l1 * zs.x + l2 * zs.y + l3 * zs.z)))
                hit++;
            
            //
        
            uint2 middle_coord = uint2(mean2f(maxx, minx), mean2f(maxy, miny));
            
            l1 -= mmm_l1_xstep * 0.5;
            l2 -= mmm_l2_xstep * 0.5;
            l1 += mmm_l1_ystep * 0.5;
            l2 += mmm_l2_ystep * 0.5;
            l3 = 1.0f - l1 - l2;
        
            if (z_buf[middle_coord] + bias2 < (max_percision_float32 * (l1 * zs.x + l2 * zs.y + l3 * zs.z)))
                hit++;
            
            l1 = ((minx - v2.x) * v3ymv2y - (miny - v2.y) * v3xmv2x) * t54;
            l2 = ((minx - v3.x) * v1ymv3y - (miny - v3.y) * v1xmv3x) * t54;
            l3 = 1.0f - l2 - l1;
        
            if (z_buf[uint2(minx, miny)] + bias2 < (max_percision_float32 * (l1 * zs.x + l2 * zs.y + l3 * zs.z)))
                hit++;
            
            if (hit > 4) {
                return;
            }
        } else {
            l1 = ((minx - v2.x) * v3ymv2y - (miny - v2.y) * v3xmv2x) * t54;
            l2 = ((minx - v3.x) * v1ymv3y - (miny - v3.y) * v1xmv3x) * t54;
            l3 = 1.0f - l2 - l1;
        }*/
        
        l1 = ((minx - v2.x) * v3ymv2y - (miny - v2.y) * v3xmv2x) * t54;
        l2 = ((minx - v3.x) * v1ymv3y - (miny - v3.y) * v1xmv3x) * t54;
        l3 = 1.0f - l2 - l1;
        
        float3 c1 = float3(col_tri[0], col_tri[1], col_tri[2]);
        float3 c2 = float3(col_tri[4], col_tri[5], col_tri[6]);
        float3 c3 = float3(col_tri[8], col_tri[9], col_tri[10]);
        float3 as = float3(col_tri[3], col_tri[7], col_tri[11]);
        //half a;
        uint /*u, v, _1ma,*/ old, depth = 0;
        //uint m3m1 = m3 - 1;
        //uint m4m1 = m4 - 1;
        
        //
        //
        //
        
        // What the fuck? 67? actually no, its ~ the max full percision float32
                    
        depth = (l1 * zs.x + l2 * zs.y + l3 * zs.z) * max_percision_float32;
        float depth_increment_x = max_percision_float32 * (l1_xstep * zs.x + l2_xstep * zs.y - (l1_xstep + l2_xstep) * zs.z);
        
        //if (c[t * 13 + 12] == 1.0f) {
        for (uint y = miny; y < maxy; y++) {
            for (uint x = minx; x < maxx; x++) {
                l1 += l1_xstep;
                l2 += l2_xstep;
                l3 -= l1_xstep;
                l3 -= l2_xstep;
                depth += depth_increment_x;
                    
                if ((l1 >= 0.0f) && (l2 >= 0.0f) && (l3 >= 0.0f)) {
                    if (z_buf[uint2(x, y)] > depth) {
                        InterlockedMin(z_buf[uint2(x, y)], depth, old);
                        if (old > depth) {
                            screen_buf[uint2(x, y)] = uint((l1 * c1.r + l2 * c2.r + l3 * c3.r) * 255u) << 16 |
                                                      uint((l1 * c1.g + l2 * c2.g + l3 * c3.g) * 255u) << 8 |
                                                      uint((l1 * c1.b + l2 * c2.b + l3 * c3.b) * 255u);
                        }
                    }
                }
            }
                
            l1 += l1_ystep;
            l2 += l2_ystep;
            
            // Goofy ass variable names
            l1 -= mmm_l1_xstep;
            l2 -= mmm_l2_xstep;
            l3 = 1.0f - l1 - l2;
            depth = uint(max_percision_float32 * (l1 * zs.x + l2 * zs.y + l3 * zs.z));
        }
        /*} if (1 == 2) {
            for (uint y = miny; y < maxy; y++) {
                for (uint x = minx; x < maxx; x++) {
                    l1 += l1_xstep;
                    l2 += l2_xstep;
                    l3 = 1.0f - l2 - l1;
                    
                    if ((l1 >= 0.0f) && (l2 >= 0.0f) && (l3 >= 0.0f)) {
                        depth = l1 * zs.x + l2 * zs.y + l3 * zs.z;
                        if (z_buf[uint2(x, y)] > depth) {
                            u = min(floor((l1 * c1.r + l2 * c2.r + l3 * c3.r) * m3m1), m3m1);
                            v = min(floor((l1 * c1.g + l2 * c2.g + l3 * c3.g) * m4m1), m4m1);
                            uint color = texture_custom[u + v * m3];
                            uint og_col = screen_buf[uint2(x, y)];
                            a = (color & 0xFF) * rcp(255.0f) * (as.x * l1 + as.y * l2 + as.z * l3);
                            _1ma = 1 - a;
                            screen_buf[uint2(x, y)] = int(((color >> 24) & 0xFF) * a + ((og_col >> 16) & 0xFF) * (_1ma)) << 16 |
                                                      int(((color >> 16) & 0xFF) * a + ((og_col >> 8) & 0xFF) * (_1ma)) << 8 |
                                                      int(((color >> 8) & 0xFF) * a + (og_col & 0xFF) * (_1ma));
                            z_buf[uint2(x, y)] = depth;
                        }
                    }
                }
                
                l1 += l1_ystep;
                l2 += l2_ystep;
                l1 -= mmm_l1_xstep;
                l2 -= mmm_l2_xstep;
            }
        }*/
    }
}
