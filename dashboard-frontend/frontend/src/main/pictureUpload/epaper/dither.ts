export const EPAPER_WIDTH = 800;
export const EPAPER_HEIGHT = 480;

const THRESHOLD = 127.5;
const STRETCH_LOW_PERCENTILE = 0.01;
const STRETCH_HIGH_PERCENTILE = 0.99;
const STRETCH_MIN_SPREAD = 32;
const GAMMA = 0.9;

// The steps below mirror the board's photo renderer, so the preview shows what the panel paints.

// Stretches the 1st..99th luma percentile to full range, skipped for near-flat
// images where it would only amplify noise, then lifts the midtones slightly.
export const applyToneCurve = (gray: Float32Array): void => {
    const histogram = new Array<number>(256).fill(0);
    for (const value of gray) {
        histogram[Math.min(255, Math.max(0, Math.trunc(value)))]++;
    }

    const percentile = (p: number) => {
        const target = Math.trunc(p * gray.length);
        let count = 0;
        for (let value = 0; value < 256; value++) {
            count += histogram[value];
            if (count > target) return value;
        }
        return 255;
    };
    const low = percentile(STRETCH_LOW_PERCENTILE);
    const high = percentile(STRETCH_HIGH_PERCENTILE);
    const stretch = high - low >= STRETCH_MIN_SPREAD;

    for (let i = 0; i < gray.length; i++) {
        const t = stretch ? (gray[i] - low) / (high - low) : gray[i] / 255;
        gray[i] = Math.pow(Math.min(1, Math.max(0, t)), GAMMA) * 255;
    }
};

// Serpentine Floyd–Steinberg: odd rows run right to left with the kernel mirrored.
export const ditherSerpentine = (gray: Float32Array, width: number, height: number): void => {
    for (let y = 0; y < height; y++) {
        const reverse = y % 2 === 1;
        const dir = reverse ? -1 : 1;
        for (let i = 0; i < width; i++) {
            const x = reverse ? width - 1 - i : i;
            const index = y * width + x;
            const value = gray[index] > THRESHOLD ? 255 : 0;
            const error = gray[index] - value;
            gray[index] = value;

            const distribute = (dx: number, dy: number, factor: number) => {
                const nx = x + dx * dir;
                const ny = y + dy;
                if (nx >= 0 && nx < width && ny < height) {
                    gray[ny * width + nx] += error * factor;
                }
            };
            distribute(1, 0, 7 / 16);
            distribute(-1, 1, 3 / 16);
            distribute(0, 1, 5 / 16);
            distribute(1, 1, 1 / 16);
        }
    }
};

export const ditherToEpaper = (rgba: Uint8ClampedArray, width: number, height: number): Uint8ClampedArray => {
    const gray = new Float32Array(width * height);
    for (let i = 0; i < gray.length; i++) {
        gray[i] = 0.299 * rgba[i * 4] + 0.587 * rgba[i * 4 + 1] + 0.114 * rgba[i * 4 + 2];
    }

    applyToneCurve(gray);
    ditherSerpentine(gray, width, height);

    const out = new Uint8ClampedArray(width * height * 4);
    for (let i = 0; i < gray.length; i++) {
        const value = gray[i] > THRESHOLD ? 255 : 0;
        out[i * 4] = value;
        out[i * 4 + 1] = value;
        out[i * 4 + 2] = value;
        out[i * 4 + 3] = 255;
    }
    return out;
};
