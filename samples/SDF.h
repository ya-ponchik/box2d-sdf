#pragma once

// In production it is good to replace GLSL functions with Box2D for determinism.

#ifdef INSIDE_CPP
// https://registry.khronos.org/OpenGL-Refpages/gl4/html/mix.xhtml
float mix(float x, float y, float a) { return x * (1.0f - a) + y * a; }

// https://registry.khronos.org/OpenGL-Refpages/gl4/html/mod.xhtml
// https://stackoverflow.com/questions/5122993/floor-int-function-implementaton/26091248#26091248
float mod(float x, float y)
{
    float div = x / y;
    int xi = int(div);
    float floor = div < xi ? xi - 1 : xi;
    return x - y * floor;
}

// https://registry.khronos.org/OpenGL-Refpages/gl4/html/fract.xhtml
// https://stackoverflow.com/questions/5122993/floor-int-function-implementaton/26091248#26091248
float fract(float x)
{
    int xi = int(x);
    float floor = x < xi ? xi - 1 : xi;
    return x - floor;
}

float calc_sin(float x) { return b2ComputeCosSin(x).sine; }
float calc_cos(float x) { return b2ComputeCosSin(x).cosine; }
#else
#define b2Vec2 vec2
#define b2Length length
#define b2ClampFloat clamp
#define b2Abs abs
#define b2Max max
#define b2MaxFloat max
#define b2MinFloat min
#define calc_sin sin
#define calc_cos cos
#define b2AbsFloat abs
#define B2_ASSERT(x)
#endif

// https://iquilezles.org/articles/distfunctions2d/
float sdCircle( b2Vec2 p, float r )
{
    return b2Length(p) - r;
}

// https://iquilezles.org/articles/distfunctions/
float opSmoothUnion( float d1, float d2, float k )
{
    float h = b2ClampFloat( 0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f );
    return mix( d2, d1, h ) - k*h*(1.0f-h);
}

// https://iquilezles.org/articles/distfunctions/
float opSmoothSubtraction( float d1, float d2, float k )
{
    float h = b2ClampFloat( 0.5f - 0.5f*(d2+d1)/k, 0.0f, 1.0f );
    return mix( d2, -d1, h ) + k*h*(1.0f-h);
}

// https://iquilezles.org/articles/distfunctions2d/
float sdBox( b2Vec2 p, b2Vec2 b )
{
    b2Vec2 d = b2Abs(p)-b;
    return b2Length(b2Max(d,b2Vec2(0.0f, 0.0f))) + b2MinFloat(b2MaxFloat(d.x,d.y),0.0f);
}

// https://iquilezles.org/articles/distfunctions2d/
float sdCircleWave( b2Vec2 p, float tb, float ra )
{
    tb = 3.1415927f*5.0f/6.0f*b2MaxFloat(tb,0.0001f);
    b2Vec2 co = ra*b2Vec2(calc_sin(tb),calc_cos(tb));
    p.x = b2AbsFloat(mod(p.x,co.x*4.0f)-co.x*2.0f);
    b2Vec2  p1 = p;
    b2Vec2  p2 = b2Vec2(b2AbsFloat(p.x-2.0f*co.x),-p.y+2.0f*co.y);
    float d1 = ((co.y*p1.x>co.x*p1.y) ? b2Length(p1-co) : b2AbsFloat(b2Length(p1)-ra));
    float d2 = ((co.y*p2.x>co.x*p2.y) ? b2Length(p2-co) : b2AbsFloat(b2Length(p2)-ra));
    return b2MinFloat(d1, d2); 
}

// https://iquilezles.org/articles/distfunctions2d/
float sdArc( b2Vec2 p, b2Vec2 sc, float ra, float rb )
{
    // sc is the sin/cos of the arc's aperture
    p.x = b2AbsFloat(p.x);
    return ((sc.y*p.x>sc.x*p.y) ? b2Length(p-sc*ra) : 
                                  b2AbsFloat(b2Length(p)-ra)) - rb;
}

b2Vec2 rot(b2Vec2 p, float a)
{
#ifdef INSIDE_CPP
    return b2RotateVector(b2MakeRot(a), p);
#else
    float c = cos(a);
    float s = sin(a);
    return mat2(c, s, -s, c) * p;
#endif
}

float circleGrid(b2Vec2 p, float a, float s, float s2)
{
    b2Vec2 rp = rot(p, a);
    b2Vec2 tmp_1 = s2 * rp;
    b2Vec2 tmp_2 = b2Vec2(fract(tmp_1.x), fract(tmp_1.y));
    float cg = b2Length(tmp_2-b2Vec2(0.5f, 0.5f))-(b2MinFloat(-p.y*0.3f, s));
    float d = b2MaxFloat(cg, p.y);
    // Less space dilation that way.
    return d / s2;
}

// Union produces correct exterior but incorrect interior
// Subtract produces incorrect exterior but correct interior
// Smoothing dilates space
// If the ground SDF is animated, it neither wakes bodies nor gives them the correct push velocity
float sdf_sample_1(b2Vec2 p, b2Vec2 center, b2Vec2 half_size)
{
	float c1 = sdCircle(p - b2Vec2(0.0f, -22.0f), 25.0f);
    float c2 = sdCircle(p - b2Vec2(-42.0f, -7.0f), 25.0f);
    float c3 = sdCircle(p - b2Vec2(42.0f, -7.0f), 25.0f);
    float ground = opSmoothUnion(c1, c2, 10.0f);
    ground = opSmoothUnion(ground, c3, 10.0f);

    const float min_carve_r = 2.5f;
    const float max_carve_r = 7.5f;
    const float anim_speed_multiplier = 0.5f;
    float carve_r = min_carve_r + b2AbsFloat(calc_sin(test_time * anim_speed_multiplier)) * (max_carve_r - min_carve_r);
	float carve = sdCircle(p - b2Vec2(-35.0f, 15.0f), carve_r);
    
	return opSmoothSubtraction(carve, ground, 2.5f);
}

// Exact circle
float sdf_sample_2(b2Vec2 p, b2Vec2 center, b2Vec2 half_size)
{
    B2_ASSERT(half_size.x == half_size.y);
	return sdCircle(p - center, half_size.x);
}

// Exact box
float sdf_sample_3(b2Vec2 p, b2Vec2 center, b2Vec2 half_size)
{
	return sdBox(p - center, half_size);
}

// Exact infinite wave (but Box2D broad phase tree is not infinite...)
float sdf_sample_4(b2Vec2 p, b2Vec2 center, b2Vec2 half_size)
{
	return sdCircleWave(p - b2Vec2(0.0f, 30.0f), 10.0f, 25.0f) - 1.0f;
}

// Union produces correct exterior but incorrect interior
// Smoothing dilates space
float sdf_sample_5(b2Vec2 p, b2Vec2 center, b2Vec2 half_size)
{
	float c1 = sdCircle(p - center - b2Vec2(15.0f, 0.0f), 15.0f);
    float c2 = sdCircle(p - center + b2Vec2(15.0f, 0.0f), 15.0f);
    return opSmoothUnion(c1, c2, 15.0f * (calc_sin(test_time * 0.5f) + 1.0f));
}

// Exact arc
float sdf_sample_6(b2Vec2 p, b2Vec2 center, b2Vec2 half_size)
{
    const float arc_radius = 4.0f;
    const float outline_radius = 0.7f;
    // slightly less than half of pi
	const float tb = 1.4f;
    p -= center;
    // 180 deg rotation
    p = -p;
    return sdArc(p, b2Vec2(calc_sin(tb), calc_cos(tb)), arc_radius, outline_radius);
}

// An infinite (if you enable it below) procedural level
// The code taken from https://obelex.itch.io/put-them-back
float sdf_sample_7(b2Vec2 p, b2Vec2 center, b2Vec2 half_size)
{
    const float scale = 0.1f;
    const bool is_infinite = false;

    p *= scale;

    float d = opSmoothSubtraction(-opSmoothUnion(circleGrid(b2Vec2(p.x * 0.6f, p.y), 1.0f, 0.3f, 0.3f), circleGrid(p, 1.4f, 0.18f, 0.2f), 0.2f), -circleGrid(b2Vec2(p.x * 0.5f, p.y * 0.8f), 2.0f, 0.3f, 0.5f), 0.2f);
    if (!is_infinite)
        d = opSmoothUnion(opSmoothUnion(opSmoothSubtraction(-p.y, -(b2AbsFloat(p.x) - 1.0f + p.y * 0.2f), 0.5f), d, 0.5f), p.y + 100.0f, 0.5f);

    return d / scale;
}

const b2Vec2 sample_2_center = b2Vec2(0.0f, 15.0f);
const b2Vec2 sample_3_center = b2Vec2(20.0f, 20.0f);
const b2Vec2 sample_5_center = b2Vec2(0.0f, 70.0f);
const b2Vec2 sample_6_center = b2Vec2(-43.0f, 37.0f);

const b2Vec2 sample_2_half_size = b2Vec2(0.2f, 0.2f);
const b2Vec2 sample_3_half_size = b2Vec2(6.0f, 6.0f);

float calc_sdf(b2Vec2 p)
{
    const b2Vec2 none = b2Vec2(0.0f, 0.0f);

    if (test_mode == 1)
        return sdf_sample_7(p, none, none);

    // NOTE: Samples influence each other visually, but not physically.
    float d = sdf_sample_1(p, none, none);
    d = b2MinFloat(d, sdf_sample_2(p, sample_2_center, sample_2_half_size));
    d = b2MinFloat(d, sdf_sample_3(p, sample_3_center, sample_3_half_size));
    d = b2MinFloat(d, sdf_sample_4(p, none, none));
    d = b2MinFloat(d, sdf_sample_5(p, sample_5_center, none));
    d = b2MinFloat(d, sdf_sample_6(p, sample_6_center, none));
    return d;
}