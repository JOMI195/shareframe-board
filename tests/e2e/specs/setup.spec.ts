import { existsSync } from 'node:fs';
import {
  attachPanel, DASHBOARD_PASSWORD, expectShimCall, Frame, retryFlag, setWifiMode, showsApSetup, showsDefault,
  showsRegistration, showsStatus, waitForPanel,
} from '../support/board';
import { card, expect, test } from '../support/fixtures';

// The hotspot fallback: no internet, the owner joins the board's AP, signs in with the
// device password and sets up Wi-Fi in the regular dashboard.

const signInOnHotspot = async (page: import('@playwright/test').Page) => {
  await setWifiMode('ap');
  await page.goto('/');
  await expect(page).toHaveURL(/\/auth\/sign-in$/);
  await expect(page.getByText(/stellt den Hotspot „shareframe-e2e“ bereit/)).toBeVisible();
  await expect(page.getByRole('button', { name: 'Mit OTP anmelden' })).toHaveCount(0);
  await page.getByLabel(/Passwort/).fill(DASHBOARD_PASSWORD);
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page).toHaveURL(/\/network\/$/);
};

test('on the hotspot the owner signs in with the device password and lands on the network settings', async ({ page }) => {
  await signInOnHotspot(page);

  await expect(page.getByLabel('Verbindung: Hotspot')).toBeVisible();
  await expect(card(page, 'Hotspot aktiv')).toBeVisible();
});

test('on the hotspot a saved network is only tried on request', async ({ page }) => {
  await signInOnHotspot(page);

  await page.getByRole('button', { name: 'Netzwerk hinzufügen' }).click();
  const add = page.getByRole('dialog');
  await add.getByLabel(/Netzwerkname/).fill('HomeNet');
  await add.getByLabel(/^Passwort/).fill('homepass1');
  await add.getByRole('button', { name: 'Hinzufügen' }).click();
  await expectShimCall('shareframe-wifi add HomeNet homepass1');
  expect(existsSync(retryFlag), 'saving alone switched the board over').toBe(false);

  await card(page, 'Hotspot aktiv').getByRole('button', { name: 'Jetzt verbinden' }).click();
  const connect = page.getByRole('dialog');
  await expect(connect.getByText('http://shareframe-e2e.local')).toBeVisible();
  await connect.getByRole('button', { name: 'Verbinden' }).click();

  await expect(card(page, 'Hotspot aktiv').getByText(/Der Rahmen verbindet sich/)).toBeVisible();
  await expect.poll(() => existsSync(retryFlag), { message: 'retry requested from the Wi-Fi daemon' }).toBe(true);
});

test('the hotspot says when the last attempt failed', async ({ page }) => {
  await signInOnHotspot(page);
  await setWifiMode('ap', { last_failed_at: Math.floor(Date.now() / 1000) - 120 });

  await expect(card(page, 'Hotspot aktiv').getByText(/Letzter Verbindungsversuch vor 2 Minuten fehlgeschlagen/))
    .toBeVisible({ timeout: 15_000 });
});

test('the Wi-Fi endpoints need a session, also on the hotspot', async ({ request, board }) => {
  const connect = () => request.post('/api/connection/connect', { data: { ssid: 'HomeNet', password: 'homepass1' } });
  expect((await connect()).status()).toBe(401);
  await setWifiMode('ap');
  expect((await connect()).status()).toBe(401);
  expect((await request.post('/api/connection/apply')).status()).toBe(401);

  await setWifiMode('connected');
  expect((await board.postRaw('/api/connection/apply')).status()).toBe(409);
});

test('on the hotspot a visitor can reset the frame to factory defaults from the sign-in page', async ({ page, request }) => {
  expect((await request.post('/api/system/factory-reset')).status()).toBe(401);

  await setWifiMode('ap');
  await page.goto('/auth/sign-in');
  await page.getByRole('button', { name: /Auf Werkseinstellungen zurücksetzen/ }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Zurücksetzen' }).click();
  await expectShimCall('shareframe-factory-reset --yes');
  await waitForPanel(showsStatus('factory_reset'), 'factory reset status screen');
});

test('the hotspot password is only shown to hotspot clients and signed-in owners', async ({ request, board }) => {
  const mode = async () => (await (await request.get('/api/connection/mode')).json()).data;
  expect((await mode()).ap_password).toBe('');
  expect((await board.get('/api/connection/mode')).ap_password).toBe('hotspot123');

  await setWifiMode('ap');
  expect((await mode()).ap_password).toBe('hotspot123');
});

test('a frame booting without Wi-Fi shows the setup QR until it connects', async ({ board }, testInfo) => {
  await setWifiMode('ap');
  await board.restartService('display');
  await waitForPanel(showsApSetup, 'Wi-Fi setup screen');

  const frame = await Frame.read();
  await attachPanel(testInfo, 'wifi-setup');
  // Only this area shows through the passepartout; see ApNetworkSetupScreenRenderer::SAFE_AREA.
  const safe = { x0: 82, y0: 44, x1: 718, y1: 436 };
  expect(frame.blackRatio(safe.x0, safe.y0, 382, safe.y1), 'QR code').toBeGreaterThan(0.2);
  expect(frame.blackRatio(402, safe.y0, safe.x1, safe.y1), 'instructions').toBeGreaterThan(0);
  for (const [x0, y0, x1, y1] of [[0, 0, 800, safe.y0], [0, safe.y1, 800, 480], [0, 0, safe.x0, 480], [safe.x1, 0, 800, 480]])
    expect(frame.blackRatio(x0, y0, x1, y1), `ink behind the passepartout at ${x0},${y0}`).toBe(0);

  await setWifiMode('connected');
  await waitForPanel(showsDefault, 'slideshow back after connecting');
});

test('a frame nobody owns asks to be registered until the owner does', async ({ board, server }, testInfo) => {
  test.setTimeout(120_000);
  await board.restartService('display');
  await waitForPanel(showsDefault, 'slideshow after the display restart');
  await server.unregistered(true);
  await board.restartService('websocket');

  await waitForPanel(showsRegistration, 'registration screen', 30_000);
  await attachPanel(testInfo, 'registration');
  const frame = await Frame.read();
  expect(frame.blackRatio(82, 44, 382, 436), 'QR code').toBeGreaterThan(0.2);

  await server.unregistered(false);
  await waitForPanel(showsDefault, 'slideshow once the frame is registered', 60_000);
});
