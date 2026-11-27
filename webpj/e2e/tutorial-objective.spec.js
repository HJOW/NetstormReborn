// 실제 F4 입력의 단계 전환과 F8/ESC의 가림·복원을 검증한다. 포획·희생·특정 적 공격·승리는 실행하지 않는다.
const { test, expect } = require('@playwright/test');
const { step, dismissTell, closeHelp, objective, startTutorial } = require('./tutorial-objective-control');

test('튜토리얼 안내: 실제 F4 단계 전환·F8 재열기·게임 메뉴 가림과 복원', async ({ page }) => {
  test.setTimeout(180000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await startTutorial(page);
  await page.keyboard.press('F4'); await step(page, 40);
  expect((await objective(page)).gate.stage).toBe(66); expect((await objective(page)).text).toBe('');
  await dismissTell(page); await step(page, 30);
  let current = await objective(page, 'Place a total of 8 bridges.'); expect(current.text).toBe(current.expected);
  await page.keyboard.press('F8'); await step(page, 2); expect((await objective(page)).dialog).toBe(true); expect((await objective(page)).text).toBe('');
  await dismissTell(page); await step(page, 15); current = await objective(page, 'Place a total of 8 bridges.'); expect(current.text).toBe(current.expected);
  await page.keyboard.press('Escape'); await step(page, 2); expect((await objective(page)).text).toBe('');
  await page.keyboard.press('Escape'); await step(page, 2); current = await objective(page, 'Place a total of 8 bridges.'); expect(current.text).toBe(current.expected);
  await page.screenshot({ path: test.info().outputPath('tutorial1-stage-b.png') });
  expect(errors).toEqual([]);
});

// 실제 글꼴·팔레트·지도 노드를 사용한 진단이다. 표시기 내부 문자열을 기대 위치로 복제하지 않는다.
async function labelsAndInput(page) {
  return page.evaluate(() => {
    const app = window.netstorm, root = app._clientGump, view = app._campaignWorld.mapView(), colors = app._clientPaletteScreen.colors();
    const labels = [...root.createState().entries].filter(Boolean).flatMap((token) => {
      const node = root.node(token), bounds = root.collection().bounds(token);
      if (node.command !== 45 || !(node.flags & 1) || (node.flags & 0x10008) || node.textContext[0] !== 2 || node.textContext[1] !== 0 || node.textContext[2] !== colors.basic[7]
        || bounds.left < view.right - 460 || bounds.top < view.bottom - 100 || bounds.bottom > view.bottom) return [];
      const point = { x: Math.floor((bounds.left + bounds.right) / 2), y: Math.floor((bounds.top + bounds.bottom) / 2) };
      return [{ token, bounds, picked: root.events().pick(point, 0), help: root.events().pick(point, 0x200), top: root.events().pickTopLevel(point), point }];
    });
    return { view: { ...view }, labels, width: app._layout.logicalWidth, height: app._layout.logicalHeight };
  });
}

// 표시와 같은 실제 화면 좌표로 입력한다. 가로세로비 변환과 DPI는 현재 Canvas 크기에서 역변환한다.
async function logicalPoint(page, point) {
  return page.evaluate((at) => { const app = window.netstorm, rect = app.canvas.getBoundingClientRect(); return { x: rect.left + at.x * rect.width / app._layout.logicalWidth, y: rect.top + at.y * rect.height / app._layout.logicalHeight }; }, point);
}

// 각 언어의 세 화면비를 실제 브라우저로 확인하며 같은 실행에서 다른 화면비로도 전환한다.
const MODES = {
  '4:3': { width: 1024, height: 768 },
  '16:9': { width: 1366, height: 768 },
  '16:10': { width: 1280, height: 800 },
};
for (const language of ['english', 'korean']) for (const [aspect, viewport] of Object.entries(MODES)) {
  test(`튜토리얼 안내 화면: ${language} ${aspect} 도움말·지식·입력·리사이즈`, async ({ page }) => {
    test.setTimeout(180000);
    const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
    await startTutorial(page, { language, aspect, viewport });
    await page.keyboard.press('F4'); await step(page, 40); await dismissTell(page); await step(page, 20);
    let current = await objective(page, 'Place a total of 8 bridges.'); expect(current.text).toBe(current.expected);
    if (language === 'korean') expect(current.text).toMatch(/다리/);
    const initial = await labelsAndInput(page); expect(initial.labels.length).toBeGreaterThan(0);
    const tokens = initial.labels.map((label) => label.token);
    for (const label of initial.labels) {
      expect(label.bounds.left).toBeGreaterThanOrEqual(initial.view.left + 8); expect(label.bounds.right).toBeLessThanOrEqual(initial.view.right - 7);
      expect(tokens).not.toContain(label.picked); expect(tokens).not.toContain(label.help); expect(tokens).not.toContain(label.top);
    }
    // 글자 위에 포인터를 둔 키 입력도 실제 카메라로 전달돼32픽셀 이동한다.
    const point = await logicalPoint(page, initial.labels[0].point); await page.mouse.move(point.x, point.y);
    const beforeCamera = await page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().pixels.x);
    await page.keyboard.press('ArrowRight'); await step(page, 2);
    expect(await page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().pixels.x)).toBe(beforeCamera + 32);
    await page.keyboard.press('F1'); await step(page, 2); expect((await objective(page)).dialog).toBe(true); expect((await objective(page)).text).toBe('');
    await closeHelp(page); expect((await objective(page)).text).toBe(current.expected);
    await page.keyboard.press('F6'); await step(page, 2);
    expect(await page.evaluate(() => window.netstorm._campaignProduction.ui.knowledge().isOpen())).toBe(true); expect((await objective(page)).text).toBe('');
    await page.keyboard.press('Escape'); await step(page, 2); expect((await objective(page)).text).toBe(current.expected);
    await page.screenshot({ path: test.info().outputPath(`${language}-${aspect.replace(':', 'x')}.png`) });
    const resizedAspect = aspect === '16:9' ? '16:10' : '16:9', resized = MODES[resizedAspect];
    await page.setViewportSize(resized); await page.evaluate((value) => window.netstorm.setAspect(value), resizedAspect); await step(page, 2);
    await page.waitForFunction((size) => window.netstorm._layout.logicalWidth === size.width && window.netstorm._layout.logicalHeight === size.height, resized);
    const after = await labelsAndInput(page); expect(after.labels.length).toBeGreaterThan(0);
    expect(after.labels.every((label) => label.bounds.right <= after.view.right - 7 && label.bounds.bottom <= after.view.bottom - 7)).toBe(true);
    expect((await objective(page)).text).toBe(current.expected); expect((await objective(page)).gate.stage).toBe(66);
    await page.screenshot({ path: test.info().outputPath(`${language}-resized-${resizedAspect.replace(':', 'x')}.png`) });
    expect(errors).toEqual([]);
  });
}

// 정규 미션 제어 경로로 재시작/전환/이탈한다. 새 상태에 옛 문구/노드가 남지 않아야 한다.
test('튜토리얼 안내 수명: 재시작·미션 전환·일반 캠페인·이탈·앱 반납', async ({ page }) => {
  test.setTimeout(240000); const errors = [];
  page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await startTutorial(page);
  const clear = await page.evaluate(() => { const app = window.netstorm, old = app._campaignTutorialObjective; app._restartCampaignMission(); return { old: old.text(), current: app._campaignTutorialObjective }; });
  expect(clear).toEqual({ old: '', current: null }); await step(page, 90); await dismissTell(page); await step(page, 20);
  let current = await objective(page, 'Press F4 to return to your island, or place a bridge.'); expect(current.text).toBe(current.expected);
  expect(await page.evaluate(() => { const app = window.netstorm, old = app._campaignTutorialObjective; app._beginCampaignFromDialog('Tutorial2'); return old.text(); })).toBe('');
  await step(page, 90); await dismissTell(page); await step(page, 20);
  current = await objective(page, 'Finish building a Temple.'); expect(current.text).toBe(current.expected); expect(current.mission).toBe('Tutorial2');
  await page.evaluate(() => window.netstorm._beginCampaignFromDialog('TheWarBegins')); await step(page, 90); await dismissTell(page); await step(page, 20);
  expect((await objective(page)).text).toBe(''); expect((await objective(page)).mission).toBe('TheWarBegins');
  await page.evaluate(() => window.netstorm._leaveCampaignMission()); await step(page, 2); expect((await objective(page)).text).toBe(''); expect((await objective(page)).mission).toBeNull();
  await page.evaluate(() => window.netstorm._beginCampaignFromDialog('Tutorial1')); await step(page, 90); await dismissTell(page); await step(page, 20);
  current = await objective(page, 'Press F4 to return to your island, or place a bridge.'); expect(current.text).toBe(current.expected);
  expect(await page.evaluate(() => { const app = window.netstorm, old = app._campaignTutorialObjective; app.destroy(); return { old: old.text(), current: app._campaignTutorialObjective, root: app._clientGump }; })).toEqual({ old: '', current: null, root: null });
  expect(errors).toEqual([]);
});

// 후반 목표를 수행하거나 gate를 주입하지 않고 표시 부품만 별도 루트에서 검사한다. 실제 브라우저 갈무리/원본 글꼴로39키의 긴 문장도 모두 출력한다.
for (const language of ['english', 'korean']) {
  test(`튜토리얼 안내 글꼴 부품: ${language} 실제39키·3크기·긴 문장 줄바꿈`, async ({ page }) => {
    test.setTimeout(180000); await startTutorial(page, { language });
    const result = await page.evaluate(async () => {
      const { ClientGumpRoot } = await import('/ui/client-gump-root.js');
      const { TutorialObjectiveGump } = await import('/ui/tutorial-objective-gump.js');
      const { TUTORIAL_OBJECTIVES, TUTORIAL_WAITING } = await import('/core/tutorial-objective.js');
      const { GumpDrawState, GumpPixelTarget, setGumpDrawClip } = await import('/render/gump-draw.js');
      const app = window.netstorm, before = app.sessionDump(), colors = app._clientPaletteScreen.colors();
      const keys = [...new Set([...TUTORIAL_OBJECTIVES.map((entry) => entry.english), TUTORIAL_WAITING])], modes = [];
      // 게임의 실제 루트/Guide에는 표시를 추가하지 않는다. 독립 소유 루트에서 같은 실제 글꼴/8비트 출력만 실행한다.
      for (const [width, height] of [[1024, 768], [1366, 768], [1280, 800]]) {
        const root = new ClientGumpRoot(width, height, () => 0, () => {}), view = { left: 85, top: 0, right: width, bottom: height };
        const owner = new TutorialObjectiveGump(root, app._resources.fonts); let maximumLines = 0, maximumUnwrappedWidth = 0, ink = 0;
        try {
          for (const key of keys) {
            const translated = app._resources.translate(key), bytes = app._resources.fonts.encode(translated), font = app._resources.fonts.measureFont({ font: 0, style: 0 }, bytes);
            // 줄바꿈 함수와 독립적으로 현재 글리프 전진 폭을 합산한다. 한국어 문구가 한 줄에 들어오면 강제로 두 줄을 요구하지 않는다.
            const unwrappedWidth = [...bytes].reduce((sum, code) => sum + font.glyph(code).advance, 0); maximumUnwrappedWidth = Math.max(maximumUnwrappedWidth, unwrappedWidth);
            owner.sync(translated, view, colors.basic[7], colors.basic[0]);
            const tokens = [...root.createState().entries].filter(Boolean).filter((token) => root.node(token).command === 45);
            if (unwrappedWidth > 440 ? tokens.length < 2 : tokens.length !== 1) throw new Error(`실제 글리프 폭과 줄바꿈 불일치: ${key}`);
            maximumLines = Math.max(maximumLines, tokens.length);
            const state = new GumpDrawState(), output = new Uint8Array(width * height).fill(157); setGumpDrawClip(state, { left: 0, top: 0, right: width, bottom: height });
            const target = new GumpPixelTarget(output, width, height, state);
            for (const token of tokens) {
              const bounds = root.collection().bounds(token), point = { x: Math.floor((bounds.left + bounds.right) / 2), y: Math.floor((bounds.top + bounds.bottom) / 2) };
              if (bounds.left < view.left + 8 || bounds.right > width - 7 || bounds.bottom > height - 7 || bounds.right - bounds.left > 443) throw new Error(`실제 번역 글꼴 줄 범위 초과: ${key}`);
              if (root.events().pick(point, 0) || root.events().pickTopLevel(point)) throw new Error(`표시 글자의 입력 소비: ${key}`);
              root.draw(token, state, target);
            }
            // 배경색157은 손대지 않고 실제 글리프의 흰색/검은 그림자만 쓴다.
            for (const pixel of output) { if (pixel === 157) continue; if (pixel !== colors.basic[7] && pixel !== colors.basic[0]) throw new Error('안내에 글리프/그림자 이외 색 출력'); ink += 1; }
          }
          modes.push({ width, height, keys: keys.length, maximumLines, maximumUnwrappedWidth, ink });
        } finally { owner.clear(); root.dispose(); }
      }
      return { modes, sameGame: before === app.sessionDump() };
    });
    expect(result.sameGame).toBe(true);
    for (const mode of result.modes) { expect(mode.keys).toBe(39); expect(mode.maximumLines).toBeGreaterThanOrEqual(1); expect(mode.ink).toBeGreaterThan(10000); }
    await test.info().attach(`${language}-39-keys.json`, { body: JSON.stringify(result, null, 2), contentType: 'application/json' });
    console.log('실제 글꼴39키', language, JSON.stringify(result));
  });
}
