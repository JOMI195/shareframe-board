import { describe, expect, it, vi } from 'vitest';
import { fireEvent, screen, waitFor } from '@testing-library/react';
import PictureUploadDialog from '@/main/pictureUpload/uploadDialog';
import { renderWithProviders } from '@tests/helpers/renderWithProviders';
import { signedInState } from '@tests/helpers/preloadedState';
import { renderEpaper } from '@/main/pictureUpload/epaper/renderEpaper';
import { pixelsToBlob } from '@/common/utils/canvas/pixels';
import { adjustImage } from '@/main/pictureUpload/imageAdjust/adjustImage';
import { getCroppedImg } from '@/main/pictureUpload/imageCropping/cropper/utils';

// jsdom has no layout, so react-easy-crop never reports a crop area, and no canvas, so the
// real getCroppedImg cannot run. Both are unit-tested separately.
vi.mock('react-easy-crop', async () => {
  const { useEffect } = await import('react');
  const area = { x: 0, y: 0, width: 100, height: 100 };

  type MockProps = {
    crop: { x: number; y: number };
    onCropChange: (crop: { x: number; y: number }) => void;
    onCropAreaChange: (a: unknown, b: unknown) => void;
  };

  const MockCropper = ({ crop, onCropChange, onCropAreaChange }: MockProps) => {
    useEffect(() => onCropAreaChange(area, area), [onCropAreaChange]);
    return (
      <div data-testid="cropper" data-crop={JSON.stringify(crop)}>
        <button type="button" onClick={() => onCropChange({ x: 5, y: 7 })}>Ausschnitt verschieben</button>
      </div>
    );
  };

  return { default: MockCropper };
});

vi.mock('@/main/pictureUpload/imageCropping/cropper/utils', async (importOriginal) => ({
  ...(await importOriginal<typeof import('@/main/pictureUpload/imageCropping/cropper/utils')>()),
  getCroppedImg: vi.fn(async () => new Blob(['cropped'], { type: 'image/jpeg' })),
}));

vi.mock('@/main/pictureUpload/useLoadedImage', () => import('@tests/mocks/loadedImage'));

// No canvas in jsdom either; the dithering and the adjustment math are unit-tested.
const epaperBlob = new Blob(['epaper'], { type: 'image/png' });
vi.mock('@/main/pictureUpload/epaper/renderEpaper', () => ({
  renderEpaper: vi.fn(async () => epaperBlob),
}));
vi.mock('@/main/pictureUpload/imageAdjust/adjustImage', () => ({
  adjustImage: vi.fn(async () => new Blob(['adjusted'], { type: 'image/jpeg' })),
}));
vi.mock('@/common/utils/canvas/pixels', () => ({
  readPixels: (_source: unknown, width: number, height: number) => ({
    data: new Uint8ClampedArray(width * height * 4).fill(100),
    width,
    height,
  }),
  pixelsToBlob: vi.fn(async () => new Blob(['preview'], { type: 'image/png' })),
}));

const photo = (name: string, lastModified = 1) =>
  new File(['x'.repeat(64)], name, { type: 'image/jpeg', lastModified });

const render = ({ maxFiles = 5, accept = true } = {}) => {
  const onUpload = vi.fn(async (file: File) => { void file; return accept; });
  const onClose = vi.fn();
  const view = renderWithProviders(
    <PictureUploadDialog open onClose={onClose} onUpload={onUpload} maxFiles={maxFiles} uploading={false} />,
    { preloadedState: signedInState() },
  );
  return { ...view, onUpload, onClose };
};

const alerts = (store: ReturnType<typeof render>['store']) =>
  store.getState().snackbars.snackbars.alerts.map((alert) => alert.message).join('\n');

const fileInput = () => document.querySelector('input[type="file"]') as HTMLInputElement;

const tab = (name: 'Zuschnitt' | 'Anpassen') => screen.getByRole('tab', { name });

const adjustOpen = () => screen.findByRole('slider', { name: 'Kontrast' });

const goToCropping = async (photos = [photo('urlaub.jpg')], options = {}) => {
  const view = render(options);
  await view.user.upload(fileInput(), photos);
  await screen.findByTestId('cropper');
  await waitFor(() => expect(screen.getByRole('button', { name: 'Weiter zur Vorschau' })).toBeEnabled());
  return view;
};

const goToAdjust = async (photos?: File[], options = {}) => {
  const view = await goToCropping(photos, options);
  await view.user.click(tab('Anpassen'));
  await adjustOpen();
  return view;
};

const goToPreview = async (photos?: File[], options = {}) => {
  const view = await goToCropping(photos, options);
  await view.user.click(screen.getByRole('button', { name: 'Weiter zur Vorschau' }));
  await screen.findByRole('button', { name: 'Hochladen' });
  return view;
};

const objectUrlOf = (blob: Blob) => {
  const { calls, results } = vi.mocked(URL.createObjectURL).mock;
  const index = calls.map(([arg]) => arg).lastIndexOf(blob);
  return results[index]?.value;
};

const uploaded = async (onUpload: ReturnType<typeof render>['onUpload']) => {
  await waitFor(() => expect(onUpload).toHaveBeenCalledOnce());
  return onUpload.mock.calls[0][0];
};

describe('PictureUploadDialog selection', () => {
  it('starts with the picker and no editor', () => {
    render();

    expect(screen.getByRole('button', { name: 'Fotos auswählen' })).toBeInTheDocument();
    expect(screen.queryByTestId('cropper')).not.toBeInTheDocument();
  });

  it('opens the first photo for cropping right after picking', async () => {
    const { user } = render();

    await user.upload(fileInput(), [photo('a.jpg'), photo('b.jpg', 2)]);

    expect(await screen.findByTestId('cropper')).toBeInTheDocument();
    expect(screen.getByRole('heading', { name: 'Foto 1 von 2' })).toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'a.jpg' })).toHaveAttribute('aria-current', 'true');
  });

  // The set on the board has room for only so many more pictures.
  it('takes no more photos than there is room for', async () => {
    const { user, store } = render({ maxFiles: 2 });

    await user.upload(fileInput(), [photo('a.jpg'), photo('b.jpg', 2), photo('c.jpg', 3)]);

    expect(await screen.findByRole('button', { name: 'b.jpg' })).toBeInTheDocument();
    expect(screen.queryByRole('button', { name: 'c.jpg' })).not.toBeInTheDocument();
    expect(alerts(store)).toMatch(/c\.jpg: Es passen nur noch 2 Fotos dazu/);
  });

  it('refuses a file it cannot edit', async () => {
    const { store } = render();

    // Pickers and drops do not always honour the accept list.
    fireEvent.change(fileInput(), { target: { files: [new File(['x'], 'notiz.txt', { type: 'text/plain' })] } });

    await waitFor(() => expect(alerts(store)).toMatch(/notiz\.txt: Falsches Dateiformat/));
    expect(screen.getByRole('button', { name: 'Fotos auswählen' })).toBeInTheDocument();
  });

  it('switches to another photo from the strip with a fresh crop', async () => {
    const { user } = await goToCropping([photo('a.jpg'), photo('b.jpg', 2)]);
    await user.click(screen.getByRole('button', { name: 'Ausschnitt verschieben' }));

    await user.click(screen.getByRole('button', { name: 'b.jpg' }));

    expect(await screen.findByTestId('cropper')).toHaveAttribute('data-crop', JSON.stringify({ x: 0, y: 0 }));
    expect(screen.getByRole('heading', { name: 'Foto 2 von 2' })).toBeInTheDocument();
  });

  it('opens the next photo when the current one is removed', async () => {
    const { user } = await goToCropping([photo('a.jpg'), photo('b.jpg', 2)]);

    await user.click(screen.getByRole('button', { name: 'a.jpg entfernen' }));
    await user.click(await screen.findByRole('button', { name: 'Entfernen' }));

    expect(screen.queryByRole('button', { name: 'a.jpg' })).not.toBeInTheDocument();
    expect(await screen.findByRole('button', { name: 'b.jpg' })).toHaveAttribute('aria-current', 'true');
  });

  it('returns to the picker when the last photo is removed', async () => {
    const { user } = await goToCropping();

    await user.click(screen.getByRole('button', { name: 'urlaub.jpg entfernen' }));
    await user.click(await screen.findByRole('button', { name: 'Entfernen' }));

    expect(await screen.findByRole('button', { name: 'Fotos auswählen' })).toBeInTheDocument();
  });
});

describe('PictureUploadDialog closing', () => {
  it('closes from the header on the picker', async () => {
    const { user, onClose } = render();

    await user.click(screen.getByRole('button', { name: 'Schließen' }));

    expect(onClose).toHaveBeenCalledOnce();
  });

  it('asks before discarding a photo in edit', async () => {
    const { user, onClose } = await goToCropping();

    await user.click(screen.getByRole('button', { name: 'Schließen' }));
    await user.click(await screen.findByRole('button', { name: 'Weiter bearbeiten' }));
    await waitFor(() => expect(screen.queryByText('Bearbeitung verwerfen?')).not.toBeInTheDocument());
    expect(onClose).not.toHaveBeenCalled();

    await user.click(screen.getByRole('button', { name: 'Schließen' }));
    await user.click(await screen.findByRole('button', { name: 'Verwerfen' }));
    expect(onClose).toHaveBeenCalledOnce();
  });

  it('steps back from the preview instead of closing', async () => {
    const { user, onClose } = await goToPreview();

    await user.click(screen.getByRole('button', { name: 'Zurück zum Bearbeiten' }));

    expect(await screen.findByRole('button', { name: 'Weiter zur Vorschau' })).toBeInTheDocument();
    expect(onClose).not.toHaveBeenCalled();
  });
});

describe('PictureUploadDialog crop and adjust', () => {
  it('keeps the crop when coming back from the adjust step', async () => {
    const { user } = await goToCropping();
    await user.click(screen.getByRole('button', { name: 'Ausschnitt verschieben' }));
    await user.click(tab('Anpassen'));
    await adjustOpen();

    await user.click(tab('Zuschnitt'));

    expect(await screen.findByTestId('cropper')).toHaveAttribute('data-crop', JSON.stringify({ x: 5, y: 7 }));
  });

  it('crops again after a rotation and with the chosen border colour', async () => {
    const { user } = await goToAdjust();
    expect(vi.mocked(getCroppedImg).mock.lastCall![3]).toBe('black');
    await user.click(tab('Zuschnitt'));
    await screen.findByTestId('cropper');

    fireEvent.change(screen.getByRole('slider', { name: 'Drehen' }), { target: { value: 90 } });
    await user.click(screen.getByRole('button', { name: 'Weiß' }));
    await user.click(tab('Anpassen'));
    await adjustOpen();

    expect(getCroppedImg).toHaveBeenCalledTimes(2);
    expect(vi.mocked(getCroppedImg).mock.lastCall![2]).toBe(90);
    expect(vi.mocked(getCroppedImg).mock.lastCall![3]).toBe('white');
  });

  it('moves, zooms, turns and resets the crop with the buttons', async () => {
    const { user } = await goToCropping();
    const cropper = () => JSON.parse(screen.getByTestId('cropper').getAttribute('data-crop')!);
    const value = (name: string) => Number((screen.getByRole('slider', { name }) as HTMLInputElement).value);

    await user.click(screen.getByRole('button', { name: 'Nach rechts verschieben' }));
    await user.click(screen.getByRole('button', { name: 'Nach unten verschieben' }));
    expect(cropper()).toEqual({ x: 5, y: 5 });
    await user.click(screen.getByRole('button', { name: 'Nach links verschieben' }));
    await user.click(screen.getByRole('button', { name: 'Nach oben verschieben' }));
    expect(cropper()).toEqual({ x: 0, y: 0 });

    await user.click(screen.getByRole('button', { name: 'Vergrößern' }));
    expect(value('Zoom')).toBeGreaterThan(1);
    await user.click(screen.getByRole('button', { name: 'Verkleinern' }));
    expect(value('Zoom')).toBeCloseTo(1);

    await user.click(screen.getByRole('button', { name: 'Nach rechts drehen' }));
    expect(value('Drehen')).toBe(22.5);
    await user.click(screen.getByRole('button', { name: 'Nach links drehen' }));
    expect(value('Drehen')).toBe(0);

    await user.click(screen.getByRole('button', { name: 'Ausschnitt verschieben' }));
    await user.click(screen.getByRole('button', { name: 'Zentrieren' }));
    expect(cropper()).toEqual({ x: 0, y: 0 });
    await user.click(screen.getByRole('button', { name: 'Nach rechts drehen' }));
    await user.click(screen.getByRole('button', { name: 'Zurücksetzen' }));
    expect(value('Drehen')).toBe(0);
  });

  it('repaints the adjust preview as the contrast changes and as the panel shows it', async () => {
    const painted = () => vi.mocked(pixelsToBlob).mock.lastCall![0];
    const { user } = await goToAdjust();
    await waitFor(() => expect(painted().data[0]).toBe(100));

    fireEvent.change(screen.getByRole('slider', { name: 'Kontrast' }), { target: { value: 50 } });
    await waitFor(() => expect(painted().data[0]).toBeLessThan(100));

    await user.click(screen.getByRole('button', { name: 'E-Paper' }));
    await waitFor(() => {
      expect(painted().width).toBe(800);
      expect(painted().data.every((value) => value === 0 || value === 255)).toBe(true);
    });
  });

  it('resets both values at once', async () => {
    const { user } = await goToAdjust();
    fireEvent.change(screen.getByRole('slider', { name: 'Helligkeit' }), { target: { value: -20 } });
    fireEvent.change(screen.getByRole('slider', { name: 'Kontrast' }), { target: { value: 40 } });

    await user.click(screen.getByRole('button', { name: 'Zurücksetzen' }));

    expect(screen.getByRole('slider', { name: 'Helligkeit' })).toHaveValue('0');
    expect(screen.getByRole('slider', { name: 'Kontrast' })).toHaveValue('0');
  });
});

describe('PictureUploadDialog upload', () => {
  it('hands over the plain crop as a JPEG and closes once everything is done', async () => {
    const { user, onUpload, onClose } = await goToPreview();

    await user.click(screen.getByRole('button', { name: 'Hochladen' }));

    const file = await uploaded(onUpload);
    expect(await file.text()).toBe('cropped');
    expect(file.type).toBe('image/jpeg');
    expect(file.name).toBe('urlaub.jpg');
    expect(adjustImage).not.toHaveBeenCalled();
    await waitFor(() => expect(onClose).toHaveBeenCalledOnce());
  });

  it('hands over the adjusted photo', async () => {
    const { user, onUpload } = await goToAdjust();
    fireEvent.change(screen.getByRole('slider', { name: 'Helligkeit' }), { target: { value: 30 } });

    await user.click(screen.getByRole('button', { name: 'Weiter zur Vorschau' }));
    await user.click(await screen.findByRole('button', { name: 'Hochladen' }));

    expect(await (await uploaded(onUpload)).text()).toBe('adjusted');
    expect(vi.mocked(adjustImage).mock.calls[0][1]).toEqual({ brightness: 30, contrast: 0 });
  });

  it('shows the e-paper view in the preview and hands over the original anyway', async () => {
    const { user, onUpload } = await goToPreview();
    const preview = () => screen.getByRole('img', { name: 'Vorschau von urlaub.jpg' });

    await user.click(screen.getByRole('button', { name: 'E-Paper' }));

    await waitFor(() => expect(preview()).toHaveAttribute('src', objectUrlOf(epaperBlob)));
    expect(await vi.mocked(renderEpaper).mock.calls[0][0].text()).toBe('cropped');
    await user.click(screen.getByRole('button', { name: 'Hochladen' }));
    expect(await (await uploaded(onUpload)).text()).toBe('cropped');
  });

  it('keeps the photo in the preview when it was refused', async () => {
    const { user, onUpload, onClose } = await goToPreview(undefined, { accept: false });

    await user.click(screen.getByRole('button', { name: 'Hochladen' }));

    await uploaded(onUpload);
    expect(screen.getByRole('button', { name: 'Hochladen' })).toBeInTheDocument();
    expect(onClose).not.toHaveBeenCalled();
  });

  it('continues with a fresh crop of the next photo after an upload', async () => {
    const { user, onClose } = await goToCropping([photo('a.jpg'), photo('b.jpg', 2)]);
    await user.click(screen.getByRole('button', { name: 'Ausschnitt verschieben' }));
    await user.click(screen.getByRole('button', { name: 'Weiter zur Vorschau' }));
    await user.click(await screen.findByRole('button', { name: 'Hochladen' }));

    expect(await screen.findByRole('heading', { name: 'Foto 2 von 2' })).toBeInTheDocument();
    expect(await screen.findByTestId('cropper')).toHaveAttribute('data-crop', JSON.stringify({ x: 0, y: 0 }));
    expect(onClose).not.toHaveBeenCalled();
  });
});
