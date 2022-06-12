#pragma once
#include "../external/NiceMath.h"

inline float frand(int* seed)
{
    union
    {
        float fres;
        unsigned int ires;
    };

    seed[0] *= 16807;
    ires = (((unsigned int)(seed[0]) >> 9) | 0x3f800000);
    return fres - 1.0f;
}

// returns random float between 0 and 1
static float randf()
{
    static int seed = 15677;
    return frand(&seed);
}

inline nm::float3 random_in_unit_sphere()
{
    nm::float3 result;
    do
    {
        // Randf generates floating point values betweem 0 and 1.0f. So converting them between -1.0f and 1.0f
        result = (2.0f * nm::float3{ randf(), randf() , randf() }) - nm::float3(1.0f, 1.0f, 1.0f);
    } while (nm::lengthsq(result) >= 1.0f);

    return result;
}

inline nm::float3 random_in_unit_disk()
{
    nm::float3 result;
    do
    {
        result = (2.0f * nm::float3{ randf(), randf() , 0.0f }) - nm::float3(1.0f, 1.0f, 0.0f);
    } while (nm::lengthsq(result) >= 1.0f);

    return result;
}

inline double random_double() 
{
    // Returns a random real in [0,1).
    return rand() / (RAND_MAX + 1.0);
}

inline double random_double(double min, double max) 
{
    // Returns a random real in [min,max).
    return min + (max - min) * random_double();
}

inline nm::float3 random(double min, double max)
{
    return nm::float3((float)random_double(min, max), 
        (float)random_double(min, max), 
        (float)random_double(min, max));
    // Returns a random real in [min,max).
    //return min + (max - min) * random_double();
}