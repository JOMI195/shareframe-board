import { describe, expect, it, vi } from 'vitest';
import { fireEvent, screen, waitFor, within } from '@testing-library/react';
import * as api from '@/assets/endpoints/api/frame';
import { renderRoute } from '@tests/helpers/renderRoute';
import { signedInState } from '@tests/helpers/preloadedState';
import { makeDefaultImages } from '@tests/fixtures';
import { failsWith, ok, server, setBoard, spyOnRequests, unreachable } from '@tests/mocks';

// jsdom has no layout, so react-easy-crop never reports a crop area, and no canvas for the
// crop, the adjustment or the e-paper view. Those are unit-tested.
vi.mock('react-easy-crop', async () => {
  const { useEffect } = await import('react');
  const area = { x: 0, y: 0, width: 100, height: 100 };
  const MockCropper = ({ onCropAreaChange }: { onCropAreaChange: (a: unknown, b: unknown) => void }) => {
    useEffect(() => onCropAreaChange(area, area), [onCropAreaChange]);
    return <div data-testid="cropper" />;
  };
  return { default: MockCropper };
});
vi.mock('@/main/pictureUpload/imageCropping/cropper/utils', async (importOriginal) => ({
  ...(await importOriginal<typeof import('@/main/pictureUpload/imageCropping/cropper/utils')>()),
  getCroppedImg: vi.fn(async () => new Blob(['cropped'], { type: 'image/jpeg' })),
}));
vi.mock('@/main/pictureUpload/epaper/renderEpaper', () => ({
  renderEpaper: vi.fn(async () => new Blob(['epaper'], { type: 'image/png' })),
}));
vi.mock('@/main/pictureUpload/useLoadedImage', () => import('@tests/mocks/loadedImage'));

const openPage = async () => {
  const view = renderRoute('/default-images/', { preloadedState: signedInState() });
  // The page is a lazy route chunk, which takes a moment to load on the first test.
  await screen.findByRole('img', { name: 'Standardbild 1' }, { timeout: 5000 });
  return view;
};

// The preview's actions sit on top of the page; the confirmation comes on top of the preview.
const askToRemove = async (user: Awaited<ReturnType<typeof openPage>>['user'], index: number) => {
  await user.click(screen.getByRole('listitem', { name: `Standardbild ${index}` }));
  const remove = await within(await screen.findByRole('dialog')).findByRole('button', { name: 'Entfernen' });
  await user.click(remove);
  return screen.findByRole('dialog', { name: 'Standardbild entfernen?' });
};

const photo = (name: string) => new File(['x'.repeat(64)], name, { type: 'image/jpeg', lastModified: 1 });

const pickAndPreview = async (user: Awaited<ReturnType<typeof openPage>>['user'], name: string) => {
  await user.click(screen.getAllByRole('button', { name: 'Bild hinzufügen' })[0]);
  const dialog = await screen.findByRole('dialog');
  await user.upload(dialog.parentElement!.querySelector('input[type="file"]') as HTMLInputElement, photo(name));
  const next = await within(dialog).findByRole('button', { name: 'Weiter zur Vorschau' });
  // Enabled once the cropper has measured the crop.
  await waitFor(() => expect(next).toBeEnabled());
  await user.click(next);
  return dialog;
};

describe('default images', () => {
  it('keeps the last three pictures from being removed', async () => {
    const { user } = await openPage();
    expect(screen.getByText('3 von höchstens 10 Standardbildern')).toBeInTheDocument();

    await user.click(screen.getByRole('listitem', { name: 'Standardbild 2' }));

    const preview = await screen.findByRole('dialog');
    expect(within(preview).getByText('Position 2 von 3')).toBeInTheDocument();
    expect(within(preview).getByRole('button', { name: 'Entfernen' })).toBeDisabled();
  });

  it('uploads the edited photo as a JPEG and shows it in the set', async () => {
    const upload = spyOnRequests('post', api.getDefaultImagesUrl(), () => ok(makeDefaultImages(4)));
    server.use(upload.handler);
    const { user } = await openPage();

    const dialog = await pickAndPreview(user, 'meer.png');
    await user.click(await within(dialog).findByRole('button', { name: 'Hochladen' }));

    await waitFor(() => expect(upload.calls).toHaveLength(1));
    expect(upload.calls[0].headers.get('Content-Type')).toBe('image/jpeg');
    expect(await upload.calls[0].text()).toBe('cropped');
    await waitFor(() => expect(screen.queryByRole('dialog')).not.toBeInTheDocument());
    expect(await screen.findByRole('img', { name: 'Standardbild 4' })).toBeInTheDocument();
  });

  it('a photo the board finds too similar stays in the editor with the reason', async () => {
    server.use(...failsWith('post', api.getDefaultImagesUrl(), 409, 'Picture too similar'));
    const { user } = await openPage();

    const dialog = await pickAndPreview(user, 'fast-gleich.jpg');
    await user.click(await within(dialog).findByRole('button', { name: 'Hochladen' }));

    expect(await screen.findByText(/fast genauso aus wie ein vorhandenes Standardbild/)).toBeInTheDocument();
    expect(within(dialog).getByRole('button', { name: 'Hochladen' })).toBeEnabled();
    expect(screen.queryByRole('img', { name: 'Standardbild 4' })).not.toBeInTheDocument();
  });

  it('a full set offers no upload', async () => {
    setBoard({ defaultImages: makeDefaultImages(10) });
    await openPage();

    for (const button of screen.getAllByRole('button', { name: 'Bild hinzufügen' }))
      expect(button).toBeDisabled();
  });

  it('removes a picture above the minimum after asking', async () => {
    setBoard({ defaultImages: makeDefaultImages(4) });
    const remove = spyOnRequests('post', api.getRemoveDefaultImageUrl(), () => ok(makeDefaultImages(3)));
    server.use(remove.handler);
    const { user } = await openPage();

    const confirm = await askToRemove(user, 2);
    await user.click(within(confirm).getByRole('button', { name: 'Entfernen' }));

    await waitFor(() => expect(screen.queryByRole('dialog')).not.toBeInTheDocument());
    expect(await remove.calls[0].json()).toEqual({ name: makeDefaultImages(4).images[1].name });
    expect(screen.queryByRole('img', { name: 'Standardbild 4' })).not.toBeInTheDocument();
  });

  it('offers a retry when the set cannot be loaded', async () => {
    server.use(...failsWith('get', api.getDefaultImagesUrl(), 503, 'Fehler'));
    const { user } = renderRoute('/default-images/', { preloadedState: signedInState() });

    const retry = await screen.findByRole('button', { name: 'Erneut versuchen' }, { timeout: 5000 });
    server.resetHandlers();
    await user.click(retry);

    expect(await screen.findByRole('img', { name: 'Standardbild 1' })).toBeInTheDocument();
  });

  it('keeps the set when the board cannot be reached for a removal', async () => {
    server.use(...unreachable('post', api.getRemoveDefaultImageUrl()));
    setBoard({ defaultImages: makeDefaultImages(4) });
    const { user } = await openPage();

    const confirm = await askToRemove(user, 2);
    await user.click(within(confirm).getByRole('button', { name: 'Entfernen' }));

    expect(await screen.findByText(/nicht erreichbar|offline/)).toBeInTheDocument();
    expect(screen.getByText('4 von höchstens 10 Standardbildern')).toBeInTheDocument();
  });

  it('downloads a picture from its preview', async () => {
    const click = vi.spyOn(HTMLAnchorElement.prototype, 'click').mockImplementation(() => undefined);
    const { user } = await openPage();

    await user.click(screen.getByRole('listitem', { name: 'Standardbild 2' }));
    await within(await screen.findByRole('dialog')).findByRole('button', { name: 'Entfernen' });
    await user.hover(screen.getByRole('button', { name: 'Aktionen' }));
    const label = await screen.findByText('Bild herunterladen');
    // In jsdom the open speed dial's actions still report pointer-events: none.
    fireEvent.click(within(label.closest('.MuiSpeedDialAction-staticTooltip') as HTMLElement).getByRole('menuitem'));

    await waitFor(() => expect(click).toHaveBeenCalledTimes(1));
    expect((click.mock.instances[0] as unknown as HTMLAnchorElement).download).toBe('standardbild-2.jpg');
  });
});
