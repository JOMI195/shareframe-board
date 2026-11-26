import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { getCroppedImg } from '@/main/pictureUpload/imageCropping/cropper/utils';
import { renderEpaper } from '@/main/pictureUpload/epaper/renderEpaper';
import { adjustImage } from '@/main/pictureUpload/imageAdjust/adjustImage';
import { blobToPixels } from '@/common/utils/canvas/pixels';

// jsdom has no canvas; this stand-in keeps the pixels a canvas would hold, so the
// pipeline around it runs for real.
const exported: { type: string; width: number; height: number; data: Uint8ClampedArray }[] = [];

const fakeContext = (canvas: HTMLCanvasElement) => {
  let pixels = new Uint8ClampedArray(canvas.width * canvas.height * 4).fill(60);
  return {
    fillRect: vi.fn(), scale: vi.fn(), translate: vi.fn(), rotate: vi.fn(), drawImage: vi.fn(),
    fillStyle: '', imageSmoothingEnabled: false, imageSmoothingQuality: 'low',
    getImageData: (_x: number, _y: number, width: number, height: number) => ({ data: new Uint8ClampedArray(pixels), width, height }),
    createImageData: (width: number, height: number) => ({ data: new Uint8ClampedArray(width * height * 4), width, height }),
    putImageData: (image: { data: Uint8ClampedArray }) => { pixels = new Uint8ClampedArray(image.data); },
    exportedPixels: () => pixels,
  };
};

beforeEach(() => {
  exported.length = 0;
  const contexts = new WeakMap<HTMLCanvasElement, ReturnType<typeof fakeContext>>();
  vi.spyOn(HTMLCanvasElement.prototype, 'getContext').mockImplementation(function (this: HTMLCanvasElement) {
    if (!contexts.has(this)) contexts.set(this, fakeContext(this));
    return contexts.get(this) as unknown as CanvasRenderingContext2D;
  });
  vi.spyOn(HTMLCanvasElement.prototype, 'toBlob').mockImplementation(function (this: HTMLCanvasElement, callback, type) {
    exported.push({ type: type ?? 'image/png', width: this.width, height: this.height, data: contexts.get(this)!.exportedPixels() });
    callback(new Blob(['x'], { type }));
  });
  vi.stubGlobal('createImageBitmap', vi.fn(async () => ({ width: 1200, height: 900, close: vi.fn() })));
  vi.stubGlobal('Image', class {
    width = 6000;
    height = 4000;
    onload: (() => void) | null = null;
    set src(_url: string) { queueMicrotask(() => this.onload?.()); }
  });
});

afterEach(() => vi.unstubAllGlobals());

describe('picture pipeline', () => {
  // The board takes at most a few MB per picture; a full-size phone photo would be far over.
  it('exports a large crop as a JPEG of at most 1600 px on the long side', async () => {
    await getCroppedImg(new File(['x'], 'gross.jpg'), { x: 0, y: 0, width: 5000, height: 3000 });

    expect(exported).toHaveLength(1);
    expect(exported[0]).toMatchObject({ type: 'image/jpeg', width: 1600, height: 960 });
  });

  it('renders the e-paper view at panel size in pure black and white', async () => {
    await renderEpaper(new Blob(['x']));

    const [png] = exported;
    expect(png).toMatchObject({ type: 'image/png', width: 800, height: 480 });
    expect(png.data.every((value, index) => index % 4 === 3 || value === 0 || value === 255)).toBe(true);
  });

  it('keeps the photo size when adjusting it', async () => {
    await adjustImage(new Blob(['x']), { brightness: 20, contrast: 0 });

    expect(exported[0]).toMatchObject({ type: 'image/jpeg', width: 1200, height: 900 });
    expect(exported[0].data[0]).toBeGreaterThan(60);
  });

  it('explains a photo the browser cannot draw', async () => {
    vi.mocked(HTMLCanvasElement.prototype.getContext).mockReturnValue(null);

    await expect(blobToPixels(new Blob(['x']))).rejects.toThrow('Das Foto ist zu groß zum Bearbeiten.');
  });
});
