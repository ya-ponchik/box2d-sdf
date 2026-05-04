#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <algorithm>
#include <span>

namespace SDF
{
namespace detail
{
// 2D vector cross
inline double cro(glm::dvec2 a, glm::dvec2 b) { return a.x*b.y-a.y*b.x; }
} // namespace detail

// https://iquilezles.org/articles/distfunctions2d/ -> https://www.shadertoy.com/view/MlKcDD
// carefully translated to c++ and doubles, omited all #if 0
inline double quadratic_bezier(glm::dvec2 pos, glm::dvec2 A, glm::dvec2 B, glm::dvec2 C)
{
    using namespace detail;

    auto const a = B - A;
    auto const b = A - 2.0*B + C;
    auto const c = a * 2.0;
    auto const d = A - pos;

    // cubic to be solved (kx*=3 and ky*=3)
    auto const kk = 1.0/glm::dot(b,b);
    auto const kx = kk * glm::dot(a,b);
    auto const ky = kk * (2.0*glm::dot(a,a)+glm::dot(d,b))/3.0;
    auto const kz = kk * glm::dot(d,a);      

    auto res = 0.0;
    auto sgn = 0.0;

    auto const p  = ky - kx*kx;
    auto const q  = kx*(2.0*kx*kx - 3.0*ky) + kz;
    auto const p3 = p*p*p;
    auto const q2 = q*q;
    auto h  = q2 + 4.0*p3;


    if( h>=0.0 ) 
    {   // 1 root
        h = std::sqrt(h);
        
            h = (q<0.0) ? h : -h; // copysign()
            auto const x = (h-q)/2.0;
            auto const v = glm::sign(x)*std::pow(std::abs(x),1.0/3.0);
            auto t = v - p/v;


		// from NinjaKoala - single newton iteration to account for cancellation
        t -= (t*(t*t+3.0*p)+q)/(3.0*t*t+3.0*p);
        
        t = std::clamp( t-kx, 0.0, 1.0 );
        auto const  w = d+(c+b*t)*t;
        res = glm::dot(w,w);
    	sgn = cro(c+2.0*b*t,w);
    }
    else 
    {   // 3 roots
        auto const z = std::sqrt(-p);

        auto x = q/(p*z*2.0);
        // https://www.shadertoy.com/view/WltSD7
        x=std::sqrt(0.5+0.5*x);
        auto const m = x*(x*(x*(x*-0.008972+0.039071)-0.107074)+0.576975)+0.5;
        auto n = std::sqrt(1.0-m*m);
  
        n *= std::sqrt(3.0);
        auto const  t = glm::clamp( glm::dvec3(m+m,-n-m,n-m)*z-kx, 0.0, 1.0 );
        auto const  qx=d+(c+b*t.x)*t.x;
        auto const dx=glm::dot(qx,qx);
        auto const sx=cro(a+b*t.x,qx);
        auto const  qy=d+(c+b*t.y)*t.y;
        auto const dy=glm::dot(qy,qy);
        auto const sy=cro(a+b*t.y,qy);
        if( dx<dy ) {
            res=dx;
            sgn=sx;
        } else {
            res=dy;
            sgn=sy;
        }
    }
    
    return std::sqrt( res )*glm::sign(sgn);
}

// https://iquilezles.org/articles/distfunctions2d/ -> https://www.shadertoy.com/view/3tdSDj
// carefully translated to c++ and doubles
// no r argument (you can use opRound)
inline double segment(glm::dvec2 p, glm::dvec2 a, glm::dvec2 b)
{
    auto const ba = b-a;
    auto const pa = p-a;
    auto const h = std::clamp( glm::dot(pa,ba)/glm::dot(ba,ba), 0.0, 1.0 );
    return glm::length(pa-h*ba);
}

// no r argument (you can use opRound)
inline double point(glm::dvec2 p, glm::dvec2 a)
{
    return glm::length(p - a);
}

// https://iquilezles.org/articles/distfunctions/
// carefully translated to c++ and doubles
inline double smooth_union(double a, double b, double k)
{
    k *= 4.0;
    auto const h = std::max(k-std::abs(a-b),0.0);
    return std::min(a, b) - h*h*0.25/k;
}

// https://iquilezles.org/articles/distfunctions/
// carefully translated to c++ and doubles
inline double smooth_subtraction(double a, double b, double k)
{
    return -smooth_union(a,-b,k);
}

struct Segment {
    glm::dvec2 a;
    glm::dvec2 b;
};

struct QBezier {
    glm::dvec2 a;
    glm::dvec2 b;
    glm::dvec2 c;
};

// Part of the shader on which SDF::svg is based
inline double winding_sign(glm::dvec2 p, glm::dvec2 a, glm::dvec2 b)
{
    // Source: https://www.shadertoy.com/view/wdBXRW
    auto const e = b - a;
    auto const w = p - a;
    // winding number from http://geomalgorithms.com/a03-_inclusion.html
    auto const cond = glm::bvec3(p.y >= a.y, 
                       p.y < b.y, 
                       e.x*w.y > e.y*w.x);
    return (glm::all(cond) || glm::all(glm::not_(cond))) ? -1.0 : 1.0;
}

// https://www.shadertoy.com/view/dls3Wr
// carefully translated to c++ and doubles
//
// I’ve been testing various doodles and Inkscape features. It works surprisingly well.
// I rarely saw incorrect distances at primitive junctions, but the image didn't explode.
// Preprocessing could be the culprit here. Or sdBezier was incorrectly ported (some parts omitted).
// The shapes don't look great artistically in those glitched spots anyway.
//
// even odd fill rule
inline double svg(glm::dvec2 p, std::span<Segment const> segments, std::span<QBezier const> beziers, auto&& optimization_condition)
{
    using namespace detail;

    auto d = 1e10;
    auto winding = 1.0;
    for (auto const [a, b, c] : beziers) {
        // Experiment: return false if unknown.
        if (optimization_condition(p, a, b, c)) {
            winding *= winding_sign(p, a, c);
            continue;
        }
        // NOTE: I use a different Bezier function than the original shader
        auto const sd = quadratic_bezier(p, a, b, c);
        d = std::min(d, std::abs(sd));
        if (sd > 0.0 == cro(b - c, b - a) < 0.0) {
            winding *= winding_sign(p, a, b);
            winding *= winding_sign(p, b, c);
        } else {
            winding *= winding_sign(p, a, c);
        }
    }
    for (auto const [a, b] : segments) {
        d = std::min(d, segment(p, a, b));
        winding *= winding_sign(p, a, b);
    }
    return d * winding;
}

// Encoding signed distance to a 1-byte texture
// 1. Clamp to the min/max range
// 2. Map to the byte range (0-255)
// 3. Round to the nearest integer (improves precision)
inline uint8_t encode(double v, double min, double max)
{
    auto const scale = 255.0 / (max - min);
    auto const clamped = std::clamp(v, min, max);
    return static_cast<uint8_t>((clamped - min) * scale + 0.5);
}
} // namespace SDF