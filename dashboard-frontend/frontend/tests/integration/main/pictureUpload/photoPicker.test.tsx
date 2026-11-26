import { describe, expect, it, vi } from 'vitest';
import { fireEvent, waitFor } from '@testing-library/react';
import StartScreen from '@/main/pictureUpload/startScreen';
import { usePhotoPicker } from '@/main/pictureUpload/usePhotoPicker';
import type { ImageStatus } from '@/main/pictureUpload/useUploadQueue';
import { renderWithProviders } from '@tests/helpers/renderWithProviders';

const file = (name: string, type: string, sizeMb = 1) =>
  new File([new Uint8Array(Math.round(sizeMb * 1024 * 1024))], name, { type });

const Picker = ({ addImages, maxFiles }: { addImages: (files: File[]) => void; maxFiles: number }) => {
  const imageStatuses: ImageStatus[] = [];
  const { inputProps, openPicker, selectFiles, isPreparing } = usePhotoPicker(imageStatuses, addImages, maxFiles);
  return (
    <>
      <input {...inputProps} />
      <StartScreen openPicker={openPicker} selectFiles={selectFiles} isPreparing={isPreparing} showCamera maxFiles={maxFiles} />
    </>
  );
};

const setup = (maxFiles = 5) => {
  const addImages = vi.fn();
  const rendered = renderWithProviders(<Picker addImages={addImages} maxFiles={maxFiles} />);
  const input = rendered.container.querySelector('input[type="file"]') as HTMLInputElement;
  const alerts = () => rendered.store.getState().snackbars.snackbars.alerts.map((alert) => alert.message).join('\n');
  return { ...rendered, addImages, input, alerts };
};

describe('photo picker', () => {
  it('picks one photo at a time when only one more fits', () => {
    expect(setup(1).input).not.toHaveAttribute('multiple');
    expect(setup(3).input).toHaveAttribute('multiple');
  });

  it('rejects an oversized file and keeps the valid ones', async () => {
    const { user, addImages, input, alerts } = setup();

    await user.upload(input, [file('gut.jpg', 'image/jpeg'), file('gross.jpg', 'image/jpeg', 31)]);

    await waitFor(() => expect(addImages).toHaveBeenCalledOnce());
    expect(addImages.mock.calls[0][0].map((f: File) => f.name)).toEqual(['gut.jpg']);
    expect(alerts()).toContain('gross.jpg: Datei ist zu groß (maximal 30 MB)');
  });

  // Android Chrome releases the picker's staged file once the input is touched again.
  it('hands the parent a detached copy, not the picked file', async () => {
    const { user, addImages, input } = setup();
    const picked = file('a.jpg', 'image/jpeg');

    await user.upload(input, [picked]);

    await waitFor(() => expect(addImages).toHaveBeenCalledOnce());
    const forwarded: File = addImages.mock.calls[0][0][0];
    expect(forwarded).not.toBe(picked);
    expect(await forwarded.arrayBuffer()).toEqual(await picked.arrayBuffer());
  });

  it('reports a file whose bytes cannot be read instead of failing silently', async () => {
    const { user, addImages, input, alerts } = setup();
    const unreadable = file('kaputt.jpg', 'image/jpeg');
    vi.spyOn(unreadable, 'arrayBuffer').mockRejectedValue(
      Object.assign(new Error('could not be read'), { name: 'NotReadableError' }),
    );

    await user.upload(input, [unreadable]);

    await waitFor(() => expect(alerts()).toContain('kaputt.jpg: NotReadableError'));
    expect(addImages).not.toHaveBeenCalled();
  });

  it('takes photos dropped on the start screen', async () => {
    const { addImages, getByText } = setup();
    const target = getByText('Welche Fotos möchtest du hochladen?').closest('div')!.parentElement!;

    fireEvent.dragOver(target);
    fireEvent.drop(target, { dataTransfer: { files: [file('gezogen.png', 'image/png')] } });

    await waitFor(() => expect(addImages).toHaveBeenCalledOnce());
    expect(addImages.mock.calls[0][0][0].name).toBe('gezogen.png');
  });

  it('asks for the rear camera only when taking a photo', async () => {
    const { user, input, getByRole } = setup();
    vi.spyOn(input, 'click').mockImplementation(() => {});

    await user.click(getByRole('button', { name: 'Foto aufnehmen' }));
    expect(input).toHaveAttribute('capture', 'environment');

    await user.click(getByRole('button', { name: 'Fotos auswählen' }));
    expect(input).not.toHaveAttribute('capture');
  });
});
