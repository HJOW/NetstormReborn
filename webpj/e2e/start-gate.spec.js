// 처음 방문(캐시 없음)의 시작 화면 안정성을 실제 Chromium 에서 확인한다. (2026-11-24 WB04-E1300, 사용자 요구)
//  1) 메뉴·언어 자료는 다 받았어도 뒤에서 받는 메뉴 음악이 끝나기 전에는 '클릭하여 시작'이 열리지 않고 클릭도 받지 않는다(끝나면 열린다).
//  2) 정적 파일 요청 몇 개가 일시적으로 실패(503·연결 끊김)해도 다시 받아 한국어로 시작한다(영어 대체·오류 화면이 되지 않는다).
const { test, expect } = require('@playwright/test');

// 한국어 브라우저(선호 언어 ko-KR)에서 처음 방문하는 상황으로 시험한다: 주소의 ?lang 이 없어도 선호 언어로 한국어를 탐지해야 한다.
test.use({ locale: 'ko-KR' });

// 저장된 설정 없이(처음 방문) 한국어 브라우저 언어로 연다.
async function openFirstVisit(page, url) {
  await page.addInitScript(() => { try { localStorage.clear(); } catch { /* 저장소가 막혀도 계속한다 */ } });
  await page.setViewportSize({ width: 1024, height: 768 });
  await page.goto(url);
}

test('메뉴 음악을 다 받기 전에는 시작 클릭이 열리지 않고 다 받으면 열린다', async ({ page }) => {
  test.setTimeout(120000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  let release; const gate = new Promise((resolve) => { release = resolve; });
  // 음악 파일만 붙잡아 둔다(원본 파일은 그대로 이어 받는다).
  await page.route('**/data/music/*.mus', async (route) => { await gate; await route.continue(); });
  await openFirstVisit(page, '/?lang=ko');
  // 메뉴 묶음·한국어 묶음이 끝나 자원이 만들어질 때까지 기다린다.
  await page.waitForFunction(() => Boolean(window.netstorm?._resources) && window.netstorm._musicStatus === 'loading', null, { timeout: 60000 });
  await page.waitForTimeout(1500);
  expect(await page.evaluate(() => window.netstorm.state), '음악을 받는 동안에는 로딩 화면').toBe('loading');
  // 이 상태의 클릭은 아무 일도 하지 않는다.
  await page.locator('canvas').click(); await page.waitForTimeout(300);
  expect(await page.evaluate(() => window.netstorm.state), '클릭을 받지 않는다').toBe('loading');
  release();
  await page.waitForFunction(() => window.netstorm.state === 'clickToStart', null, { timeout: 60000 });
  await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm.state === 'mainMenu' || window.netstorm.state === 'title', null, { timeout: 30000 });
  expect(errors).toEqual([]);
});

test('정적 파일 요청이 일시적으로 실패해도 다시 받아 한국어로 시작한다', async ({ page }) => {
  test.setTimeout(120000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  const hits = { manifest: 0, tell: 0, font: 0, xlat: 0 };
  // 매니페스트는 503 한 번, 한국어 글꼴은 연결 끊김 한 번, 번역표는 429 한 번, 한국어 Tell 은 503 두 번 뒤에 정상이다.
  await page.route('**/data/manifest.json', async (route) => { hits.manifest += 1; if (hits.manifest === 1) await route.fulfill({ status: 503, body: 'busy' }); else await route.continue(); });
  await page.route('**/data/fonts/Galmuri11.woff2', async (route) => { hits.font += 1; if (hits.font === 1) await route.abort('connectionreset'); else await route.continue(); });
  await page.route('**/data/d/xlat.korean', async (route) => { hits.xlat += 1; if (hits.xlat === 1) await route.fulfill({ status: 429, body: 'slow down' }); else await route.continue(); });
  await page.route('**/data/d/tell.korean', async (route) => { hits.tell += 1; if (hits.tell <= 2) await route.fulfill({ status: 503, body: 'busy' }); else await route.continue(); });
  await openFirstVisit(page, '/?lang=ko');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  const result = await page.evaluate(() => ({ language: window.netstorm.options.language, campaign: window.netstorm._resources.translate('Campaign'), unicode: window.netstorm._resources.fonts.unicode !== null }));
  expect(result, '영어로 대체되지 않고 한국어 번역·글꼴이 준비됨').toEqual({ language: 'korean', campaign: '캠페인', unicode: true });
  expect(hits.manifest, '매니페스트 재시도').toBeGreaterThanOrEqual(2); expect(hits.font, '글꼴 재시도').toBeGreaterThanOrEqual(2);
  expect(hits.xlat, '번역표 재시도').toBeGreaterThanOrEqual(2); expect(hits.tell, 'Tell 재시도').toBe(3);
  expect(errors).toEqual([]);
});

// 번들 스크립트 자체(3MB 넘음)를 받다가 끊겨도 index.html 의 작은 로더가 다시 받아 시작하고, 계속 실패하면 빈 화면이 아니라 안내 문구를 보인다. (2026-11-24 WB04-E1300)
test('번들 스크립트 내려받기가 한 번 끊겨도 다시 받아 시작한다', async ({ page }) => {
  test.setTimeout(120000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  let hits = 0;
  await page.route('**/dist/netstorm.bundle.js', async (route) => { hits += 1; if (hits === 1) await route.abort('connectionreset'); else await route.continue(); });
  await openFirstVisit(page, '/?lang=ko');
  // 받는 동안과 실패 직후에는 한국어 안내 문구가 보인다.
  await expect(page.locator('#netstorm_boot')).toHaveText('불러오는 중...');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  expect(hits, '한 번 끊기고 다시 받음').toBe(2);
  expect(await page.locator('#netstorm_boot').count(), '앱이 시작되면 안내 요소를 지운다').toBe(0);
  expect(errors).toEqual([]);
});

test('번들 스크립트를 계속 받지 못하면 빈 화면 대신 안내 문구를 보인다', async ({ page }) => {
  test.setTimeout(120000);
  let hits = 0;
  await page.route('**/dist/netstorm.bundle.js', async (route) => { hits += 1; await route.abort('connectionreset'); });
  await openFirstVisit(page, '/?lang=ko');
  await expect(page.locator('#netstorm_boot')).toHaveText('게임 스크립트를 불러오지 못했습니다. 페이지를 새로고침하세요.', { timeout: 30000 });
  expect(hits, '처음 한 번과 재시도 세 번').toBe(4);
});

// ?lang 없이 브라우저 선호 언어만으로 언어를 탐지한다(한국어 선호는 한국어, 영어 선호는 영어). 처음 방문에서 탐지가 영어로 빗나가던 경우를 고정한다. (2026-11-24 WB04-E1300)
test('?lang 없이 브라우저 선호 언어로 한국어·영어를 탐지한다', async ({ page, browser }) => {
  test.setTimeout(120000);
  await openFirstVisit(page, '/');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  expect(await page.evaluate(() => ({ language: window.netstorm.options.language, campaign: window.netstorm._resources.translate('Campaign') })), '한국어 브라우저').toEqual({ language: 'korean', campaign: '캠페인' });
  const english = await browser.newContext({ locale: 'en-US' }), other = await english.newPage();
  await openFirstVisit(other, '/');
  await other.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  expect(await other.evaluate(() => window.netstorm.options.language), '영어 브라우저').toBe('english');
  await english.close();
});
