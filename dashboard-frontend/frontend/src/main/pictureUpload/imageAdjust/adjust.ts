export const ADJUST_RANGE = 100;

export interface Adjustment {
    brightness: number;
    contrast: number;
}

export const NEUTRAL_ADJUSTMENT: Adjustment = { brightness: 0, contrast: 0 };

export const isNeutral = ({ brightness, contrast }: Adjustment) => brightness === 0 && contrast === 0;

// Contrast scales around mid gray from flat (factor 0) to factor 4, brightness then shifts by up to half the range.
export const adjustPixels = (rgba: Uint8ClampedArray, { brightness, contrast }: Adjustment): Uint8ClampedArray => {
    const factor = ((ADJUST_RANGE + contrast) / ADJUST_RANGE) ** 2;
    const offset = (brightness / ADJUST_RANGE) * 127.5;
    const lookup = new Uint8ClampedArray(256);
    for (let value = 0; value < 256; value++) {
        lookup[value] = (value - 127.5) * factor + 127.5 + offset;
    }

    const out = new Uint8ClampedArray(rgba.length);
    for (let i = 0; i < rgba.length; i += 4) {
        out[i] = lookup[rgba[i]];
        out[i + 1] = lookup[rgba[i + 1]];
        out[i + 2] = lookup[rgba[i + 2]];
        out[i + 3] = rgba[i + 3];
    }
    return out;
};
