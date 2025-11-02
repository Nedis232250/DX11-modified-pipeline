RWBuffer<uint> screen_buf : register(u0);
RWBuffer<min16float> z_buf : register(u1);

[numthreads(1024, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
    uint i = threadID.x;
    
    if (i < 1920 * 1080) {
        screen_buf[i] = 0;
        z_buf[i] = 1e30;
    }
}
