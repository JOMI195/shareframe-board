// Stand-in for the Shareframe web server: frame auth, OTP, heartbeat, releases
// and the frames WebSocket. Tests drive it through /__test/*.
import http from 'node:http';
import { createHash, randomUUID } from 'node:crypto';
import { WebSocketServer } from 'ws';

const PORT = Number(process.env.PORT ?? 9000);
const VALID_OTP = '123456';

// Tokens survive a reset so connected frames keep their session.
const tokens = new Set();
let tokenRequests = 0;
// Every picture pushed so far; a reset removes them from the frame again.
const pushedIds = new Set();
let nextImageId = 1000;
const state = freshState();

function freshState() {
  return {
    heartbeats: [],
    frameMessages: [],
    release: null,
    bundles: new Map(),
    bundleDownloads: [],
  };
}

const sockets = new Set();
let outage = false;

function json(res, status, body) {
  res.writeHead(status, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify(body));
}

async function readBody(req) {
  const chunks = [];
  for await (const c of req) chunks.push(c);
  const text = Buffer.concat(chunks).toString();
  return text ? JSON.parse(text) : {};
}

function tokenFrom(req) {
  const auth = req.headers.authorization ?? '';
  return auth.startsWith('Frame-Access-Token ') ? auth.slice(19) : null;
}

function broadcast(msg) {
  const data = JSON.stringify(msg);
  for (const ws of sockets) ws.send(data);
  return sockets.size;
}

const routes = {
  'POST /api/frames/obtain-frame-token/': (req, res) => {
    if (!(req.headers.authorization ?? '').startsWith('Ed25519-Sig ') || !req.headers['x-frame-id'])
      return json(res, 401, { detail: 'missing signature' });
    const token = randomUUID();
    tokens.add(token);
    tokenRequests++;
    json(res, 200, { access_token: token, expires_at: '2099-01-01T00:00:00' });
  },
  'POST /api/frames/verify-frame-auth-token/': async (req, res) => {
    const { access_token } = await readBody(req);
    json(res, tokens.has(access_token) ? 200 : 401, {});
  },
  'POST /api/frames/verify-frame-otp/': async (req, res) => {
    if (!tokens.has(tokenFrom(req))) return json(res, 401, {});
    const { otp } = await readBody(req);
    json(res, 200, { valid: otp === VALID_OTP });
  },
  'POST /api/frames/frame-hearbeat/': async (req, res) => {
    if (!tokens.has(tokenFrom(req))) return json(res, 401, {});
    state.heartbeats.push({ at: Date.now(), ...(await readBody(req)) });
    json(res, 200, {});
  },
  'GET /bundle': (req, res) => {
    const name = new URL(req.url, 'http://x').searchParams.get('name');
    state.bundleDownloads.push({ name, authorized: tokens.has(tokenFrom(req)) });
    if (!tokens.has(tokenFrom(req))) return json(res, 401, {});
    const bundle = state.bundles.get(name);
    if (!bundle) return json(res, 404, {});
    res.writeHead(200, { 'Content-Type': 'application/octet-stream', 'Content-Length': bundle.length });
    res.end(bundle);
  },
  'GET /api/frame-updates/latest/': (req, res) => {
    if (!tokens.has(tokenFrom(req))) return json(res, 401, {});
    if (!state.release) return json(res, 404, { detail: 'no release' });
    json(res, 200, state.release);
  },

  'GET /__test/state': (req, res) =>
    json(res, 200, {
      connectedFrames: sockets.size,
      tokenRequests,
      heartbeats: state.heartbeats,
      frameMessages: state.frameMessages,
      bundleDownloads: state.bundleDownloads,
    }),
  'POST /__test/reset': (req, res) => {
    if (pushedIds.size > 0) broadcast({ type: 'clear_specific_sent_images', sent_image_ids: [...pushedIds] });
    pushedIds.clear();
    Object.assign(state, freshState());
    json(res, 200, {});
  },
  'POST /__test/picture': async (req, res) => {
    const { data, sender = 'e2e', expiresInSecs = 3600, id = nextImageId++ } = await readBody(req);
    pushedIds.add(id);
    const delivered = broadcast({
      type: 'picture',
      sent_image_id: id,
      sender,
      data,
      expiry_unix_timestamp: Math.floor(Date.now() / 1000) + expiresInSecs,
    });
    json(res, 200, { id, delivered });
  },
  'POST /__test/send': async (req, res) => json(res, 200, { delivered: broadcast(await readBody(req)) }),
  'POST /__test/send-raw': async (req, res) => {
    const chunks = [];
    for await (const c of req) chunks.push(c);
    const text = Buffer.concat(chunks).toString();
    for (const ws of sockets) ws.send(text);
    json(res, 200, { delivered: sockets.size });
  },
  // Offers a release whose bundle the frame can download; `corrupt` breaks its checksum.
  'POST /__test/release': async (req, res) => {
    const { bundle = 'e2e bundle', corrupt = false, ...release } = await readBody(req);
    const name = `shareframe-${release.version}.raucb`;
    const bytes = Buffer.from(bundle);
    state.bundles.set(name, bytes);
    const checksum = createHash('sha256').update(corrupt ? Buffer.from('other') : bytes).digest('hex');
    state.release = { download_url: `http://fake-server:${PORT}/bundle?name=${name}`, checksum, ...release };
    json(res, 200, state.release);
  },
  // Ends the frame's socket: with a close code like the server would, or abruptly.
  'POST /__test/disconnect': async (req, res) => {
    const { code } = await readBody(req);
    for (const ws of sockets) code ? ws.close(code, 'e2e') : ws.terminate();
    json(res, 200, {});
  },
  // Server down for good: drops the frame and turns its reconnects away until switched off.
  'POST /__test/outage': async (req, res) => {
    ({ on: outage } = await readBody(req));
    if (outage) for (const ws of sockets) ws.terminate();
    json(res, 200, {});
  },
};

const server = http.createServer(async (req, res) => {
  const handler = routes[`${req.method} ${req.url.split('?')[0]}`];
  if (!handler) return json(res, 404, { detail: 'not found' });
  try {
    await handler(req, res);
  } catch (e) {
    json(res, 500, { detail: String(e) });
  }
});

const wss = new WebSocketServer({ noServer: true });
server.on('upgrade', (req, socket, head) => {
  if (outage) {
    socket.end('HTTP/1.1 503 Service Unavailable\r\n\r\n');
    return;
  }
  if (!req.url.startsWith('/ws/frames/') || !tokens.has(tokenFrom(req))) {
    socket.end('HTTP/1.1 401 Unauthorized\r\n\r\n');
    return;
  }
  wss.handleUpgrade(req, socket, head, (ws) => {
    sockets.add(ws);
    ws.on('message', (data) => state.frameMessages.push(JSON.parse(data.toString())));
    ws.on('close', () => sockets.delete(ws));
  });
});

// Outlive the runner's idle keep-alive sockets; the 5 s default races its polling.
server.keepAliveTimeout = 120_000;
server.headersTimeout = 121_000;
server.listen(PORT, () => console.log(`fake shareframe server on :${PORT}`));
