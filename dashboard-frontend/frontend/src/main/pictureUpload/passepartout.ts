import { PaletteMode } from '@mui/material';

// Display active area and the window the passepartout leaves open, both centered.
const PANEL_MM = { width: 163.2, height: 97.92 };
const WINDOW_MM = { width: 140, height: 90 };

export const PASSEPARTOUT = {
    x: (PANEL_MM.width - WINDOW_MM.width) / 2 / PANEL_MM.width,
    y: (PANEL_MM.height - WINDOW_MM.height) / 2 / PANEL_MM.height,
};

export interface Bounds {
    x: number;
    y: number;
    width: number;
    height: number;
}

export const safeArea = (width: number, height: number): Bounds => ({
    x: width * PASSEPARTOUT.x,
    y: height * PASSEPARTOUT.y,
    width: width * (1 - 2 * PASSEPARTOUT.x),
    height: height * (1 - 2 * PASSEPARTOUT.y),
});

export const passepartoutShade = (mode: PaletteMode) =>
    mode === 'light' ? 'rgba(255, 255, 255, 0.7)' : 'rgba(0, 0, 0, 0.65)';

export const passepartoutBackground = (color: string) => {
    const band = `linear-gradient(${color}, ${color})`;
    const x = `${PASSEPARTOUT.x * 100}%`;
    const y = `${PASSEPARTOUT.y * 100}%`;
    const middle = `calc(100% - 2 * ${y})`;
    return [
        `${band} top / 100% ${y} no-repeat`,
        `${band} bottom / 100% ${y} no-repeat`,
        `${band} left center / ${x} ${middle} no-repeat`,
        `${band} right center / ${x} ${middle} no-repeat`,
    ].join(', ');
};
