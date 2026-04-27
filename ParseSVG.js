import { SVGPathData as PathData } from 'svg-pathdata'
import c2q from 'cubic2quad'
export let artboard_size = svg => svg.match(/viewBox="([^"]+)"/)[1].split(' ').slice(2).map(Number) 
// "SDF of a cubic bezier involves solving a quintic, so it's not analytic. There are approximations"
// Our engine only supports quadratic beziers (which are analytic) for simplicity
// An analytical distance to an ellipse exists, meaning exact SVG arcs are possible
// I don't have arcs in my levels yet. If they're urgently needed, I have a file with an arcToQuadraticBeziers function
// If we could render cubic beziers, we could convert arcs to cubics with very high precision (.aToC())
// and convert quadratic beziers to cubics with zero loss (.qtToC())
// The content of the path element (sub-paths) is rendered using the SDF::svg function
// Individual paths are combined using a boolean union operation (std::min(d1, d2))
// (note that the above can result in an incorrect SDF interior if the paths intersect)
export let process_svg = (svg, callback, precision = 0.1) => {
    let x, y, segments = [], qbeziers = []
    for (const [, d] of svg.matchAll(/<path[^>]*\bd="([^"]+)"/g)) {
        segments.length = 0; qbeziers.length = 0;
        // We'll trust Inkscape to ensure Optimized SVG doesn't export any "garbage"
        // like zero-radius arcs, zero-length segments, or collinear segments
        for (let c of /*REMOVE_COLLINEAR*/new PathData(d).toAbs().normalizeHVZ().normalizeST()/*.sanitize*/.commands) {
            switch (c.type) {
            case PathData.MOVE_TO: x = c.x; y = c.y; break
            case PathData.LINE_TO: segments.push(x, y, x = c.x, y = c.y); break
            case PathData.QUAD_TO: qbeziers.push(x, y, c.x1, c.y1, x = c.x, y = c.y); break
            case PathData.ARC: throw new Error('arcs are not implemented'); break
            case PathData.CURVE_TO:
                for (let i = 2, q = c2q(x, y, c.x1, c.y1, c.x2, c.y2, x = c.x, y = c.y, precision); i < q.length; i += 4)
                    qbeziers.push(q[i - 2], q[i - 1], q[i], q[i + 1], q[i + 2], q[i + 3])
                break
            }
        }
        callback(segments, qbeziers)
    }
}