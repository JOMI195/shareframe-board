import { attachPanel, expectShimCall, Frame, setWifiMode, showsApSetup, showsDefault, showsStatus, waitForPanel } from '../support/board';
import { expect, test } from '../support/fixtures';

// The hotspot fallback: no internet, the owner joins the board's AP and sets up Wi-Fi
// without signing in.

test('in hotspot mode a visitor can set up Wi-Fi without signing in', async ({ page }) => {
  await setWifiMode('ap');
  await page.goto('/auth/sign-in');
  await page.getByRole('button', { name: 'WLAN einrichten' }).click();
  await expect(page).toHaveURL(/\/setup$/);

  await page.getByLabel('Netzwerkname (SSID)').fill('HomeNet');
  await page.getByLabel('Passwort', { exact: true }).fill('homepass1');
  await page.getByRole('button', { name: 'Verbinden' }).click();

  await expect(page.getByText('WLAN gespeichert')).toBeVisible();
  await expectShimCall('shareframe-wifi add HomeNet homepass1');
});

test('outside hotspot mode the Wi-Fi endpoints need a session', async ({ request }) => {
  const connect = () => request.post('/api/connection/connect', { data: { ssid: 'HomeNet', password: 'homepass1' } });
  expect((await connect()).status()).toBe(401);

  await setWifiMode('ap');
  expect((await connect()).status()).toBe(200);
});

test('in hotspot mode a visitor can reset the frame to factory defaults', async ({ page, request }) => {
  expect((await request.post('/api/system/factory-reset')).status()).toBe(401);

  await setWifiMode('ap');
  await page.goto('/setup');
  await page.getByRole('button', { name: 'Auf Werkseinstellungen zurücksetzen' }).click();
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
