// 하위 경로에서 실제 브라우저 모듈과 자산을 적재하고 새로고침의 설정 보존을 확인한다. (2026-11-19 WB04-E945)
const { test, expect } = require('@playwright/test');
const { once } = require('node:events');
const { createStaticServer } = require('../server.js');

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
  page.on('pageerror', (error) => errors.push(error.message));
  page.on('request', (request) => {
    const url = new URL(request.url());
    if (url.origin === baseUrl.origin && url.pathname.startsWith(mount)) files.add(url.pathname);
    else escapedRequests.push(url.href);
  });

  await page.goto(baseUrl.href);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  // 설정 변경은 앱의 구성 쓰기 경로를 통해 저장한다.
  await page.evaluate(() => window.netstorm._gameData.set('soundVolume', 3));
  await page.reload();
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  expect(await page.evaluate(() => window.netstorm._settings.get('soundVolume'))).toBe(3);
  expect(files.has(`${mount}netstorm.js`)).toBe(true);
  expect(files.has(`${mount}data/manifest.json`)).toBe(true);
  expect(escapedRequests).toEqual([]);
  expect(errors).toEqual([]);
});
