// 실제 앱의 고DPI 표시·CSS 입력 좌표·같은 미션의 크기/DPR 전환을 검사한다. 목표 수행/승리는 실행하지 않는다.
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { installAllyAi } = require('./ally-ai-test-control');

// 검사 전용 고정 시각을 원래 앱 프레임으로 진행한다. 게임 상태/gate/승패 값을 쓰지 않는다.
async function step(page, frames = 2) { await page.evaluate((count) => window.netstorm.stepFixedFrames(count), frames); }

// 현재 실제 단추의 논리 제어점을 CSS 좌표로 변환해 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  const point = await page.evaluate((text) => {
    // 실제 메뉴/Canvas의 같은 배치 정보다. DPR을 CSS 좌표에 다시 곱하지 않는다.
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    if (!logical) throw new Error(`현재 실제 단추 없음: ${text}`);
    return { x: rect.left + logical.x * rect.width / app._layout.logicalWidth, y: rect.top + logical.y * rect.height / app._layout.logicalHeight };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 }); await step(page);
}

// 같은 현재 색 번호/팔레트를 독립 전체 변환해 실제 앱의 부분 표시 결과와 비교한다.
async function inspect(page, name, rows) {
  const row = await page.evaluate(async () => {
    // 새 표시기는 네 인자의 전체 표시 경로만 사용하며 게임 Canvas/표면을 쓰지 않는다.
    const { IndexedPresenter } = await import('/render/presenter.js');
    const app = window.netstorm, layout = app._layout, canvas = document.createElement('canvas');
    canvas.width = layout.canvasWidth; canvas.height = layout.canvasHeight;
    const reference = new IndexedPresenter(document, app._presenter.scaling); reference.lookup = app._presenter.lookup.slice();
    try {
      reference.present(canvas.getContext('2d'), app._surface, layout, true);
      // 실제 출력과 전체 기대 그림의 모든 RGBA 픽셀이다.
      const actual = app.canvas.getContext('2d').getImageData(0, 0, canvas.width, canvas.height).data;
      const expected = canvas.getContext('2d').getImageData(0, 0, canvas.width, canvas.height).data;
      const a = new Uint32Array(actual.buffer), b = new Uint32Array(expected.buffer); let differences = 0;
      // 일부 샘플이 아니라 화면 전체를 읽는다.
      for (let index = 0; index < a.length; index += 1) if (a[index] !== b[index]) differences += 1;
      const rect = app.canvas.getBoundingClientRect(), hud = app._campaignProduction?.hud;
      return { layout: { ...layout }, dpr: devicePixelRatio, canvas: [app.canvas.width, app.canvas.height], rect: { left: rect.left, top: rect.top, width: rect.width, height: rect.height },
        differences, hud: hud ? hud.report().map : null, worldPreserved: !window.netstormDpiReference || window.netstormDpiReference.world.deref() === app._campaignWorld,
        sessionPreserved: !window.netstormDpiReference || window.netstormDpiReference.session.deref() === app._campaignWorld.session() };
    } finally { reference.release(); canvas.width = 1; canvas.height = 1; }
  });
  row.name = name; rows.push(row);
  expect(row.differences, `${name} 실제 Canvas와 전체 변환`).toBe(0);
  expect(row.layout.devicePixelRatio).toBe(row.dpr); expect(row.canvas).toEqual([row.layout.canvasWidth, row.layout.canvasHeight]);
  // DOM의 분수 CSS 픽셀 양자화만 허용한다. 실제 Canvas 픽셀 비교는 허용 차이0을 유지한다.
  expect(Math.abs(row.rect.width - row.layout.cssWidth)).toBeLessThanOrEqual(1 / 64); expect(Math.abs(row.rect.height - row.layout.cssHeight)).toBeLessThanOrEqual(1 / 64);
  expect(row.worldPreserved).toBe(true); expect(row.sessionPreserved).toBe(true);
  if (row.hud) expect(row.hud).toEqual({ left: 85, top: 0, right: row.layout.logicalWidth, bottom: row.layout.logicalHeight });
  return row;
}

// 실제 CSS 포인터 사건의 논리 역변환을 검사하며 유닛/건물/목표 값을 직접 바꾸지 않는다.
async function pointer(page) {
  const point = await page.evaluate(() => {
    const app = window.netstorm, rect = app.canvas.getBoundingClientRect(), x = Math.floor(app._layout.logicalWidth / 2), y = Math.floor(app._layout.logicalHeight / 2);
    return { x, y, cssX: rect.left + x * rect.width / app._layout.logicalWidth, cssY: rect.top + y * rect.height / app._layout.logicalHeight };
  });
  await page.mouse.move(point.cssX, point.cssY); await step(page);
  const actual = await page.evaluate(() => ({ ...window.netstorm._input.pointer }));
  expect(actual.inside).toBe(true); expect(Math.abs(actual.x - point.x)).toBeLessThanOrEqual(1); expect(Math.abs(actual.y - point.y)).toBeLessThanOrEqual(1);
  return { requested: point, actual };
}

// 원래 가로/세로 게임 메뉴 행을 현재 배치에서 찾아 CSS 좌표로 누른다.
async function menuRow(page, token, row) {
  const point = await page.evaluate(({ current, index }) => {
    const app = window.netstorm, root = app._clientGump, node = root.node(current), bounds = root.collection().bounds(current), rect = app.canvas.getBoundingClientRect();
    if (!node || index < 0 || index >= node.items.length) throw new Error('실제 게임 메뉴 행 범위');
    const horizontal = Boolean(node.menuFlags & 0x40000), x = horizontal ? node.itemWidth * (index + 0.5) : 20, y = horizontal ? (bounds.bottom - bounds.top) / 2 : node.offsets[index] + node.heights[index] / 2;
    return { child: node.items[index].childId ? root.createState().entries[node.items[index].childId] : 0,
      x: rect.left + (bounds.left + x) * rect.width / app._layout.logicalWidth, y: rect.top + (bounds.top + y) * rect.height / app._layout.logicalHeight };
  }, { current: token, index: row });
  await page.mouse.click(point.x, point.y, { delay: 60 }); await step(page); return point.child;
}

// 원래 Game/Leave/ABORT 입력 경로로 현재 미션을 반납한다.
async function leave(page) {
  await page.locator('[data-netstorm="game-menu-button"]').click(); await step(page);
  const bar = await page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().token());
  const game = await menuRow(page, bar, 0), labels = await page.evaluate((token) => window.netstorm._campaignProduction.ui.gameMenu().labels(token), game);
  await menuRow(page, game, labels.indexOf('Leave Mission'));
  const abort = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons[0]?.label); expect(typeof abort).toBe('string'); await clickLabel(page, abort); await step(page, 3);
  expect(await page.evaluate(() => !window.netstorm._campaignWorld && !window.netstorm._clientGuide.current())).toBe(true);
}

// 비정수125%와 정수200%에서 같은 실제 앱을 실행한다. 프로젝트 설정의 Chromium/Chrome/Edge/Firefox를 그대로 사용한다.
for (const dpr of [1.25, 2]) test.describe(`고DPI ${dpr}`, () => {
  test.use({ viewport: { width: 1024, height: 768 }, deviceScaleFactor: dpr });
  test('메뉴·실제 아군AI 미션·리사이즈·DPR 변경의 픽셀/입력/수명', async ({ page, browserName, browser }) => {
    test.setTimeout(300000);
    page.setDefaultTimeout(30000);
    const evidence = { dpr, browser: browser.version(), browserName, rows: [], pointers: [], errors: [], dynamicDpr: [] };
    page.on('pageerror', (error) => evidence.errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') evidence.errors.push(message.text()); });
    try {
      await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
      await page.goto('http://localhost:9803/debug.html?fixed=25&title=1&aspect=auto&lang=english');
      await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 }); await page.locator('canvas').click();
      await page.evaluate(() => window.netstorm._prepareCampaignAudio());
      // 실제 시작 그림을 원래 프레임으로 끝내고 메뉴에 도달한다.
      for (let frames = 0; frames < 1200 && await page.evaluate(() => window.netstorm.state !== 'mainMenu'); frames += 20) await step(page, 20);
      expect(await page.evaluate(() => window.netstorm.state)).toBe('mainMenu'); await step(page);
      expect(await page.evaluate(() => devicePixelRatio)).toBe(dpr); await inspect(page, 'menu', evidence.rows);
      await clickLabel(page, 'Options'); await clickLabel(page, 'Frame Rate'); await clickLabel(page, '60 FPS');
      expect(await page.evaluate(() => window.netstorm._settings.get('maxFPS'))).toBe(60); await inspect(page, 'option-input', evidence.rows);
      await installAllyAi(page, 'thewarbegins'); await page.evaluate(() => window.netstorm._beginCampaignFromDialog('TheWarBegins'));
      // 안내의 Play Mission을 누르기 전까지만 로그인/파일 읽기를 기다린다.
      for (let frames = 0; frames < 1500; frames += 5) {
        if (await page.evaluate(() => Boolean(window.netstorm._mainMenu.controlPoint('Play Mission')) && Boolean(window.netstorm._campaignWorld?.session().localConnectionReady()))) break;
        await step(page, 5);
      }
      await clickLabel(page, 'Play Mission'); await step(page, 4);
      evidence.activation = await page.evaluate(() => window.netstormAllyTestState); expect(evidence.activation.brain).toBeTruthy();
      // 활성화에서 보존해야 하는 사용자 값 네 개를 확인한다.
      for (const key of ['money', 'allies', 'name', 'technology']) expect(evidence.activation[key]).toEqual(evidence.activation.before[key]);
      await page.evaluate(() => { const app = window.netstorm; window.netstormDpiReference = { world: new WeakRef(app._campaignWorld), session: new WeakRef(app._campaignWorld.session()) }; });
      evidence.pointers.push(await pointer(page)); await inspect(page, 'mission', evidence.rows);
      await page.keyboard.press('Escape'); await step(page); await inspect(page, 'game-menu', evidence.rows); await page.keyboard.press('Escape'); await step(page);
      // 같은 미션에서 실제 창 크기만 바꾸고 화면비/입력/전체 출력을 다시 확인한다.
      for (const viewport of [{ width: 1366, height: 768 }, { width: 1280, height: 800 }]) {
        await page.setViewportSize(viewport); await page.waitForFunction((size) => window.netstorm._layout.logicalWidth === size.width && window.netstorm._layout.logicalHeight === size.height, viewport); await step(page);
        evidence.pointers.push(await pointer(page)); await inspect(page, `resize-${viewport.width}`, evidence.rows);
      }
      if (dpr === 2 && browserName === 'chromium') {
        const cdp = await page.context().newCDPSession(page);
        // CSS 크기를 유지한 실제 브라우저 DPR 변경이다. 앱 속성/resize 함수를 직접 호출하지 않는다.
        for (const next of [1.25, 2, 1.5, 1]) {
          await cdp.send('Emulation.setDeviceMetricsOverride', { width: 1280, height: 800, deviceScaleFactor: next, mobile: false });
          // 고정 시각 앱도 다음 실제 표시 프레임에서 배율을 확인하게 한다. 배치 함수를 직접 호출하지 않는다.
          await step(page);
          await page.waitForFunction((value) => devicePixelRatio === value && window.netstorm._layout.devicePixelRatio === value, next); await step(page);
          evidence.pointers.push(await pointer(page)); await inspect(page, `dynamic-dpr-${next}`, evidence.rows); evidence.dynamicDpr.push(next);
        }
      }
      await page.screenshot({ path: test.info().outputPath('high-dpi-mission.png') }); await leave(page); evidence.left = true;
      if (dpr === 2 && browserName === 'chromium') {
        // 운영 번들의 일반 rAF에서도 알림 없는 DPR 변경을 확인한다. 미션은 다시 시작하지 않는다.
        const cdp = await page.context().newCDPSession(page);
        await cdp.send('Emulation.setDeviceMetricsOverride', { width: 1280, height: 800, deviceScaleFactor: 2, mobile: false });
        await page.goto('http://localhost:9803/?aspect=auto&lang=english');
        await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 }); await page.locator('canvas').click();
        await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 120000 });
        evidence.normalBefore = await page.evaluate(() => ({ dpr: devicePixelRatio, layoutDpr: window.netstorm._layout.devicePixelRatio, fixed: window.netstorm.options.fixedFrameMs }));
        expect(evidence.normalBefore).toEqual({ dpr: 2, layoutDpr: 2, fixed: 0 });
        await cdp.send('Emulation.setDeviceMetricsOverride', { width: 1280, height: 800, deviceScaleFactor: 1.25, mobile: false });
        await page.waitForFunction(() => devicePixelRatio === 1.25 && window.netstorm._layout.devicePixelRatio === 1.25);
        await inspect(page, 'normal-bundle-dpr-no-notice', evidence.rows);
      }
      expect(evidence.errors).toEqual([]);
    } finally {
      const output = test.info().outputPath('high-dpi.json'); fs.mkdirSync(path.dirname(output), { recursive: true }); fs.writeFileSync(output, JSON.stringify(evidence, null, 2) + '\n');
    }
  });
});
