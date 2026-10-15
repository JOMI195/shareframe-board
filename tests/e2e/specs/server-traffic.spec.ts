import { attachPanel, Frame, isCleared, readPanel, showsDefault, showsImage, waitForPanel } from '../support/board';
import { expect, test } from '../support/fixtures';

// Everything the Shareframe server can send over the frames WebSocket, and how the
// frame recovers when the connection is closed or dropped.

test.describe('pictures', () => {
  test('without user pictures the frame rotates its default images', async ({ board }) => {
    const first = await waitForPanel(showsDefault, 'default image shown');
    await board.post('/api/frame/slideshow/skip');
    const next = await waitForPanel((p) => showsDefault(p) && p.seq > first.seq, 'next default image');
    expect(next.source).not.toBe(first.source);
  });

  test('a pushed picture is rendered right side up and in the right polarity', async ({ server }, testInfo) => {
    const id = await server.sendPicture('left-black.jpg');
    await waitForPanel(showsImage(id), `picture ${id} on panel`);
    await attachPanel(testInfo, 'left-black');

    const frame = await Frame.read();
    expect(frame.blackRatio(0, 0, 390, 480)).toBeGreaterThan(0.98);
    expect(frame.blackRatio(410, 0, 800, 480)).toBeLessThan(0.02);
  });

  test('grey levels are dithered, not thresholded', async ({ server }, testInfo) => {
    const id = await server.sendPicture('gradient.jpg');
    await waitForPanel(showsImage(id), `picture ${id} on panel`);
    await attachPanel(testInfo, 'gradient');

    const frame = await Frame.read();
    const stripes = Array.from({ length: 8 }, (_, i) => frame.blackRatio(i * 100, 0, (i + 1) * 100, 480));
    expect(stripes[0]).toBeGreaterThan(0.85);
    expect(stripes[7]).toBeLessThan(0.15);
    for (let i = 1; i < stripes.length; i++) expect(stripes[i]).toBeLessThan(stripes[i - 1]);
    expect(stripes[3]).toBeGreaterThan(0.3);
    expect(stripes[4]).toBeLessThan(0.7);
  });

  test('pictures rotate in id order and wrap around', async ({ server, board }) => {
    const a = await server.sendPicture('left-black.jpg');
    await waitForPanel(showsImage(a), `picture ${a} on panel`);
    const b = await server.sendPicture('top-black.jpg');
    await expect.poll(async () => (await board.slideshowStatus()).image_count).toBe(2);

    await board.post('/api/frame/slideshow/skip');
    await waitForPanel(showsImage(b), `skip to ${b}`);
    const frame = await Frame.read();
    expect(frame.blackRatio(0, 0, 800, 230)).toBeGreaterThan(0.98);
    expect(frame.blackRatio(0, 250, 800, 480)).toBeLessThan(0.02);

    await board.post('/api/frame/slideshow/skip');
    await waitForPanel(showsImage(a), `wrap around to ${a}`);
  });

  test('a duplicate delivery neither repaints nor replaces the picture', async ({ server, board }) => {
    const id = await server.sendPicture('left-black.jpg');
    const shown = await waitForPanel(showsImage(id), `picture ${id} on panel`);

    await server.sendPicture('top-black.jpg', { id });
    await new Promise((r) => setTimeout(r, 2000));
    expect((await readPanel())?.seq).toBe(shown.seq);
    expect((await board.slideshowStatus()).image_count).toBe(1);
  });

  test('a picture with undecodable data is dropped without disturbing the slideshow', async ({ server, board }) => {
    await server.sendPicture('', { data: '%%% not base64 %%%' });
    const good = await server.sendPicture('left-black.jpg');
    await waitForPanel(showsImage(good), `picture ${good} on panel`);
    expect((await board.slideshowStatus()).image_count).toBe(1);
  });
});

test.describe('removal', () => {
  test('a picture removed by the server leaves the panel', async ({ server, board }) => {
    const id = await server.sendPicture('left-black.jpg');
    await waitForPanel(showsImage(id), `picture ${id} on panel`);

    await server.clearImages([id]);
    await waitForPanel(showsDefault, 'back to default images');
    expect((await board.slideshowStatus()).image_count).toBe(0);
  });

  test('removing one of several pictures keeps the others', async ({ server, board }) => {
    const a = await server.sendPicture('left-black.jpg');
    await waitForPanel(showsImage(a), `picture ${a} on panel`);
    const b = await server.sendPicture('top-black.jpg');

    await server.clearImages([a]);
    await waitForPanel(showsImage(b), `moved on to ${b}`);
    expect((await board.slideshowStatus()).image_count).toBe(1);
  });

  test('an expired picture is cleaned up by the frame itself', async ({ server, board }) => {
    const id = await server.sendPicture('top-black.jpg', { expiresInSecs: 4 });
    await waitForPanel(showsImage(id), `picture ${id} on panel`);

    await expect.poll(async () => (await board.slideshowStatus()).image_count, { timeout: 20_000 }).toBe(0);
    await board.post('/api/frame/slideshow/skip');
    await waitForPanel(showsDefault, 'back to default images');
  });

  test('a clear-display command blanks the panel', async ({ server }) => {
    await server.send({ type: 'clear_display' });
    await waitForPanel(isCleared, 'panel cleared by the server');
  });
});

test.describe('protocol robustness', () => {
  test('garbage and unknown messages are ignored', async ({ server }) => {
    await server.sendRaw('not json at all');
    await server.send({ no_type: true });
    await server.send({ type: 'from_the_future', payload: 1 });
    const id = await server.sendPicture('left-black.jpg');
    await waitForPanel(showsImage(id), 'picture after garbage');
    expect((await server.state()).connectedFrames).toBe(1);
  });
});

test.describe('connection', () => {
  test('after connecting, the frame reports the pictures it holds', async ({ server, board }) => {
    const id = await server.sendPicture('left-black.jpg');
    await waitForPanel(showsImage(id), `picture ${id} on panel`);

    await board.restartService('websocket');
    await expect
      .poll(async () => (await server.state()).frameMessages.find((m) => m.type === 'check_missing_images'))
      .toMatchObject({ sent_image_ids: [id] });
  });

  test('a dropped connection is re-established and resynced', async ({ server, board }) => {
    const id = await server.sendPicture('left-black.jpg');
    await waitForPanel(showsImage(id), `picture ${id} on panel`);

    await server.disconnect();
    await expect
      .poll(async () => (await server.state()).frameMessages.filter((m) => m.type === 'check_missing_images').length, { timeout: 20_000 })
      .toBeGreaterThan(0);
    await server.sendPicture('top-black.jpg');
    await expect.poll(async () => (await board.slideshowStatus()).image_count).toBe(2);
  });

  test('a rejected token is replaced before reconnecting', async ({ server }) => {
    const before = (await server.state()).tokenRequests;
    await server.disconnect(4001);

    await expect.poll(async () => (await server.state()).tokenRequests, { timeout: 20_000 }).toBeGreaterThan(before);
    await expect.poll(async () => (await server.state()).connectedFrames, { timeout: 20_000 }).toBe(1);
    const id = await server.sendPicture('left-black.jpg');
    await waitForPanel(showsImage(id), 'picture over the new connection');
  });
});
