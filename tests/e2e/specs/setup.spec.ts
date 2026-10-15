import { expectShimCall, setWifiMode } from '../support/board';
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

test('the hotspot password is only shown to hotspot clients and signed-in owners', async ({ request, board }) => {
  const mode = async () => (await (await request.get('/api/connection/mode')).json()).data;
  expect((await mode()).ap_password).toBe('');
  expect((await board.get('/api/connection/mode')).ap_password).toBe('hotspot123');

  await setWifiMode('ap');
  expect((await mode()).ap_password).toBe('hotspot123');
});
