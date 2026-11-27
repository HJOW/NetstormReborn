// 하위 경로에서 실제 브라우저 모듈과 자산을 적재하고 새로고침의 설정 보존을 확인한다. (2026-11-19 WB04-E945)
const { test, expect } = require('@playwright/test');
const { once } = require('node:events');
const { createStaticServer } = require('../server.js');
// 물리 음향 장치에 의존하지 않는 GUI 검사 설정이며 기본 검사는 원래 장치를 사용한다.
const { installSilentAudioSink } = require('./silent-audio-sink');

// 개발 서버와 동일한 정적 장치를 하위 경로와 임시 포트에서 제공한다.
const mount = '/preview/netstorm/';
let server;
let baseUrl;

test.beforeAll(async () => {
  server = createStaticServer(mount);
  server.listen(0, '127.0.0.1');
  await once(server, 'listening');
  baseUrl = new URL(mount, `http://127.0.0.1:${server.address().port}`);
});

test.afterAll(async () => {
  if (server?.listening) await new Promise((resolve, reject) => server.close((error) => error ? reject(error) : resolve()));
});

test('하위 경로에서 시작하고 새로고침 뒤 사용자 설정을 읽는다', async ({ page }) => {
  const files = new Set();
  const escapedRequests = [];
  const errors = [];
  await installSilentAudioSink(page);
  // 브라우저가 합성한 캐시 응답 대신 서버가 실제 보낸 상태와 조건부 요청을 보존한다.
  const wire = [];
  const observe = (req, res) => res.on('finish', () => wire.push({ path: req.url.split('?')[0], status: res.statusCode, tag: req.headers['if-none-match'] }));
  server.on('request', observe);
  page.on('pageerror', (error) => errors.push(error.message));
  page.on('request', (request) => {
    const url = new URL(request.url());
    if (url.origin === baseUrl.origin && url.pathname.startsWith(mount)) files.add(url.pathname);
    else escapedRequests.push(url.href);
  });

  try {
    // GUI/저장 검사에서만 기존mute 옵션을 고를 수 있다. 기본 음향 경로의 검증 정책은 유지한다.
    const entry = new URL(baseUrl.href);
    if (process.env.NETSTORM_E2E_MUTE === '1') entry.searchParams.set('mute', '1');
    await page.goto(entry.href);
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
    // 설정 변경은 앱의 구성 쓰기 경로를 통해 저장한다.
    await page.evaluate(() => window.netstorm._gameData.set('soundVolume', 3));
    await page.reload();
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
    expect(await page.evaluate(() => window.netstorm._settings.get('soundVolume'))).toBe(3);
    expect(files.has(`${mount}dist/netstorm.bundle.js`)).toBe(true); // 모든 스크립트는 webpack 번들 하나로 불린다. (2026-11-24 WB04-E1293)
    expect(files.has(`${mount}data/manifest.json`)).toBe(true);
    expect(escapedRequests).toEqual([]);
    expect(errors).toEqual([]);
    // 번들과 매니페스트는 처음200으로 받고 새로고침 때 같은 파일의304로 재사용한다.
    for (const relative of ['dist/netstorm.bundle.js', 'data/manifest.json']) {
      const responses = wire.filter((row) => row.path === mount + relative);
      expect(responses.some((row) => row.status === 200)).toBe(true);
      expect(responses.some((row) => row.status === 304 && row.tag)).toBe(true);
    }
  } finally { server.off('request', observe); }
});
