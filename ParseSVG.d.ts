export function artboard_size(svg: string): [number, number];
/**
 * Provided arrays are reused internally
 */
export function process_svg(
    svg: string,
    callback: (segments: number[], qbeziers: number[]) => void,
    precision?: number
): void;