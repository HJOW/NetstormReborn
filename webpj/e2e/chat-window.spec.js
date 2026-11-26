// 미션 화면 아래 채팅 창(F3)이 실제 Gump 트리에 만들어지고, F3 로 보이며, 로그 줄이 색과 함께 그려지는지 실제 Chromium 에서 확인한다. (2026-11-22 WB04-E1169)
// 원본 `ChatWindow`(0x13c root)의 도우미·배경·입력줄(Console)·로그(ScrollingText) 네 자식을 같은 Client 수명에 조립하고 공유 로그 버퍼(0x548758)를 로그 자식이 빌려 쓴다.
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 90000 });
  const point = await page.evaluate((text) => { const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect(); return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height }; }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}
// 현재 채팅 창 진단과 지도 영역 아래쪽, 공유 로그 줄 수이다.
const state = (page) => page.evaluate(() => { const app = window.netstorm, chat = app._campaignProduction.ui.chat(); return { report: chat.report(), mapBottom: app._clientGump.mapView().bottom, lines: app._clientChatWindows.log.lineCount, current: app._clientChatWindows.window.current }; });
// 논리 화면의 사각형 안에서 가장 많은 색이 아닌 픽셀 수와 빨강 계열 픽셀 수이다(글자가 그려졌는지).
const ink = (page, rect) => page.evaluate((box) => {
  const app = window.netstorm, canvas = app.canvas, context = canvas.getContext('2d'), scale = canvas.width / app._layout.logicalWidth;
  const data = context.getImageData(Math.round(box.left * scale), Math.round(box.top * scale), Math.round((box.right - box.left) * scale), Math.round((box.bottom - box.top) * scale)).data; const counts = new Map(); let red = 0;
  for (let index = 0; index < data.length; index += 4) { const key = data[index] << 16 | data[index + 1] << 8 | data[index + 2]; counts.set(key, (counts.get(key) ?? 0) + 1); if (data[index] > 170 && data[index + 1] < 70 && data[index + 2] < 70) red += 1; }
  const total = data.length / 4, background = Math.max(...counts.values()); return { total, other: total - background, red };
}, rect);

test('채팅 창: 숨긴 채 만들어지고 F3 로 보이며 로그가 색과 함께 그려진다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type()) && !/willReadFrequently/u.test(message.text())) errors.push(message.text()); }); // 시험이 픽셀을 읽을 때 브라우저가 내는 Canvas2D 성능 안내는 게임 오류가 아니다.
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(2000);

  // 시작 때는 숨김이다: 채팅 root 는 화면 아래(768 이후)에 있고 지도 영역은 전체 높이이며 도우미는 보이지 않는다.
  const hidden = await state(page); expect(hidden.report, '채팅 창').not.toBeNull(); expect(hidden.current).toBe(hidden.report.token);
  expect(hidden.report.bounds.top).toBeGreaterThanOrEqual(768); expect(hidden.mapBottom).toBe(768); expect(hidden.report.helperVisible).toBe(0);
  // 숨긴 동안에도 로그는 쌓인다(원본처럼 같은 공유 버퍼에 추가).
  await page.evaluate(() => window.netstorm._campaignProduction.ui.chat().appendLog('Our Priest has been Immobilized')); expect((await state(page)).lines).toBe(1);

  // F3: 지도 영역 아래쪽이 줄고(768 → 672) 채팅 영역(96픽셀)이 위로 올라오며 도우미 표시 비트가 켜진다. 앞서 쌓은 로그 한 줄이 보인다.
  await page.keyboard.press('F3'); await page.waitForTimeout(800);
  const shown = await state(page); expect(shown.mapBottom).toBe(672); expect(shown.report.bounds.top).toBeLessThan(700); expect(shown.report.bounds.bottom).toBe(768); expect(shown.report.helperVisible).toBe(1);
  expect(shown.report.background.bounds.right - shown.report.background.bounds.left).toBeGreaterThan(800);
  const firstLine = await ink(page, { left: 90, top: 680, right: 420, bottom: 693 }); expect(firstLine.other, '첫 로그 줄 글자').toBeGreaterThan(60);

  // 색 표기: 빨강 글자 접두(`~r`)와 되돌림(`~n`)이 줄 안에서 색을 바꾼다. 줄이 늘면 새 줄이 아래에 그려진다.
  await page.evaluate(() => window.netstorm._campaignProduction.ui.chat().appendLog('~rRed~n line two')); await page.waitForTimeout(500);
  expect((await state(page)).lines).toBe(2);
  const second = await ink(page, { left: 90, top: 694, right: 220, bottom: 706 }); expect(second.other, '둘째 로그 줄 글자').toBeGreaterThan(30); expect(second.red, '빨강 글자').toBeGreaterThan(5);

  // 다시 F3: 숨긴다(지도 영역이 전체 높이로 돌아가고 로그는 남는다).
  await page.keyboard.press('F3'); await page.waitForTimeout(600);
  const again = await state(page); expect(again.mapBottom).toBe(768); expect(again.report.bounds.top).toBeGreaterThanOrEqual(768); expect(again.lines).toBe(2); expect(again.report.helperVisible).toBe(0);
  expect(errors).toEqual([]);
});

test('채팅 창: 미션을 떠나도 같은 로그·색 조각이 유지되고 앱 재시작에서 초기화된다', async ({ page }) => {
  test.setTimeout(450000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type()) && !/willReadFrequently/u.test(message.text())) errors.push(message.text()); }); // 시험이 픽셀을 읽을 때 브라우저가 내는 Canvas2D 성능 안내는 게임 오류가 아니다.
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  const start = async () => { await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
    await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 }); await clickLabel(page, 'Go!'); await page.waitForTimeout(1500); };
  await start(); const first = await state(page); expect(first.report).not.toBeNull();
  // 새 창이 같은 Client 소유 로그·스타일 조각을 다시 빌려야 한다. 줄 수만 복사해서는 충분하지 않다.
  await page.evaluate(() => { const app = window.netstorm, globals = app._clientChatWindows;
    app._campaignProduction.ui.chat().appendLog('Persist ~rRed~n line');
    window.__chatLifetime = { globals, log: globals.log, owner: globals.uiOwner, heads: [...globals.log.hardLines] }; });
  await page.keyboard.press('F3'); await page.waitForTimeout(600);
  expect((await ink(page, { left: 90, top: 680, right: 420, bottom: 693 })).red).toBeGreaterThan(5);
  await page.evaluate(() => window.netstorm._leaveCampaignMission()); await page.waitForFunction(() => !window.netstorm._campaignDisplayActive(), null, { timeout: 30000 });
  // 전체 State 부모는 이전 Runtime을 반납하기 전에 World 없는 새 HUD를 만든다. 같은 로그를 빌리되 이전 Session은 보유하지 않는다.
  expect(await page.evaluate(() => { const app = window.netstorm, production = app._campaignProduction;
    return { lines: app._clientChatWindows.log.lineCount, world: app._campaignWorld, borrowed: app._clientGameplay.borrowed(), session: production.session,
      bound: production.worldBound, chat: app._clientChatWindows.window.current === production.ui.chat().current(), owner: app._clientChatWindows.uiOwner === window.__chatLifetime.owner }; })).toEqual({ lines: 1, world: null, borrowed: false, session: null, bound: false, chat: true, owner: true });
  await start(); const second = await state(page); expect(second.report, '다음 미션의 채팅 창').not.toBeNull(); expect(second.report.bounds.top).toBeGreaterThanOrEqual(768);
  expect(second.lines).toBe(1);
  expect(await page.evaluate(() => { const globals = window.netstorm._clientChatWindows, saved = window.__chatLifetime;
    return globals === saved.globals && globals.log === saved.log && globals.uiOwner === saved.owner && saved.heads.every((head, index) => head === globals.log.hardLines[index]); })).toBe(true);
  await page.keyboard.press('F3'); await page.waitForTimeout(600);
  expect((await ink(page, { left: 90, top: 680, right: 420, bottom: 693 })).red, '두 번째 창의 같은 빨강 글자').toBeGreaterThan(5);
  await page.evaluate(() => window.netstorm._campaignProduction.ui.chat().appendLog('second mission')); expect((await state(page)).lines).toBe(2);
  // 페이지 재시작은 Client 수명의 끝이다. localStorage에 로그를 저장하지 않는다.
  await page.reload(); await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 }); await start(); expect((await state(page)).lines).toBe(0);
  expect(errors).toEqual([]);
});

// 입력줄: Enter 로 열고 문자를 쓰고 제출한다. 문자 키가 게임 단축키로 새지 않고, 단독 미션에서는 보낼 manager 가 없어 제출한 줄이 비워지며 빈 Enter 가 입력줄을 숨긴다. (2026-11-22 WB04-E1170)
test('채팅 입력줄: Enter 로 열리고 문자만 입력줄로 가며 빈 Enter 로 닫힌다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type()) && !/willReadFrequently/u.test(message.text())) errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(2000); await page.locator('canvas').focus();
  const input = () => page.evaluate(() => { const chat = window.netstorm._campaignProduction.ui.chat(); return { visible: chat.inputVisible(), text: chat.inputText(), mapBottom: window.netstorm._clientGump.mapView().bottom, queue: window.netstorm._clientInputQueue.peek()?.code ?? null }; });

  // 처음에는 입력줄이 숨어 있고 채팅 구분선이 화면 아래이다.
  expect(await input()).toMatchObject({ visible: false, text: '', mapBottom: 768 });
  // Enter: 입력줄이 보이고 채팅 구분선이 올라온다(지도 영역 아래쪽 768 → 672).
  await page.keyboard.press('Enter'); await page.waitForTimeout(600);
  expect(await input()).toMatchObject({ visible: true, mapBottom: 672, queue: null });
  // 문자는 입력줄로만 간다: q·w·a 는 평소 생산 단축키, 숫자는 화면 위치 저장 키이지만 입력줄에 글자로 들어간다.
  const camera = await page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().data.getUint32(0, true));
  await page.keyboard.type('qwa1 hello'); await page.waitForTimeout(400);
  const typed = await input(); expect(typed.text).toBe('qwa1 hello'); expect(typed.queue).toBeNull();
  expect(await page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().data.getUint32(0, true))).toBe(camera);
  // Backspace 와 방향 키 편집이 된다.
  await page.keyboard.press('Backspace'); await page.keyboard.press('Backspace'); expect((await input()).text).toBe('qwa1 hel');
  // Enter 제출: 단독 미션은 보낼 manager 가 없어 줄이 비워지고 입력줄은 남는다.
  await page.keyboard.press('Enter'); await page.waitForTimeout(500);
  expect(await input()).toMatchObject({ visible: true, text: '' });
  // 빈 Enter 는 입력줄을 숨긴다.
  await page.keyboard.press('Enter'); await page.waitForTimeout(500);
  expect((await input()).visible).toBe(false);
  // 입력줄이 보이는 동안에도 ESC 는 게임 메뉴막대를 연다(문자 키만 입력줄로 간다).
  await page.keyboard.press('Enter'); await page.waitForTimeout(400); expect((await input()).visible).toBe(true);
  await page.keyboard.press('Escape'); await page.waitForTimeout(500);
  expect(await page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().isOpen())).toBe(true);
  expect(errors).toEqual([]);
});

// 송수신: 입력줄 제출이 원본 파이프라인(대상 해석→점 명령 분배→say→41h 패킷)을 타고 로컬 서버(9)를 거쳐 되돌아와 `You> 본문` 줄이 되고, `.clear` 가 로그를 비운다. (2026-11-22 WB04-E1172)
test('채팅 송수신: 제출한 줄이 You> 로그가 되고 점 명령 .clear 가 비운다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type()) && !/willReadFrequently/u.test(message.text())) errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(2000); await page.locator('canvas').focus();
  const chat = () => page.evaluate(() => { const app = window.netstorm, ui = app._campaignProduction.ui.chat(); return { lines: app._clientChatWindows.log.lineCount, visible: ui.inputVisible(), input: ui.inputText(), manager: app._clientChatManager.current().value, mapBottom: app._clientGump.mapView().bottom }; });
  expect((await chat()).manager, 'chat manager 초기화').toBeGreaterThan(0); expect((await chat()).lines).toBe(0);

  // 제출: Enter 로 열고 쓰고 Enter. 로컬 서버를 거쳐 한 줄이 로그에 쌓인다(입력줄은 남고 입력은 비워진다).
  await page.keyboard.press('Enter'); await page.waitForTimeout(400); await page.keyboard.type('hello team'); await page.keyboard.press('Enter'); await page.waitForTimeout(1000);
  expect(await chat()).toMatchObject({ lines: 1, visible: true, input: '', mapBottom: 672 });
  // 로그 줄이 실제로 그려진다(첫 줄 영역에 글자 픽셀).
  expect((await ink(page, { left: 90, top: 680, right: 300, bottom: 693 })).other, '`You> hello team` 줄').toBeGreaterThan(60);
  // 두 번째 줄도 쌓인다.
  await page.keyboard.type('second line'); await page.keyboard.press('Enter'); await page.waitForTimeout(800); expect((await chat()).lines).toBe(2);
  // 점 명령 .clear: 로그가 비워진다(제출한 점 명령 자체는 로그에 남지 않는다).
  await page.keyboard.type('.clear'); await page.keyboard.press('Enter'); await page.waitForTimeout(800); expect(await chat()).toMatchObject({ lines: 0, visible: true, input: '' });
  // 빈 Enter 는 입력줄을 닫는다.
  await page.keyboard.press('Enter'); await page.waitForTimeout(500); expect((await chat()).visible).toBe(false);
  expect(errors).toEqual([]);
});

// 채널 접두·서식 점 명령·도우미 메뉴: `EVERYONE: team` 은 접두 `EVERYONE:` 를 입력줄에 남기고 본문만 로그가 되며, `.front ~g` 는 줄을 만들지 않는 점 명령이고, 도우미 `>` 를 누르면 원본 Chat Helper 메뉴가 뜬다. (2026-11-22 WB04-E1172)
test('채팅: 채널 접두가 남고 서식 점 명령은 줄을 만들지 않으며 도우미 메뉴가 열린다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type()) && !/willReadFrequently/u.test(message.text())) errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(2000); await page.locator('canvas').focus();
  const chat = () => page.evaluate(() => { const app = window.netstorm, ui = app._campaignProduction.ui.chat(); return { lines: app._clientChatWindows.log.lineCount, input: ui.inputText(), popup: app._clientGump.blockingPopups() }; });
  await page.keyboard.press('Enter'); await page.waitForTimeout(400);
  // 채널 접두: 본문만 로그가 되고 접두는 입력줄에 남는다.
  await page.keyboard.type('EVERYONE: team'); await page.keyboard.press('Enter'); await page.waitForTimeout(900); expect(await chat()).toMatchObject({ lines: 1, input: 'EVERYONE:' });
  // 서식 점 명령은 줄을 만들지 않고 다음 본문부터 서식이 적용된다.
  await page.keyboard.type('.front ~g'); await page.keyboard.press('Enter'); await page.waitForTimeout(700); expect((await chat()).lines).toBe(1);
  await page.keyboard.type('green text'); await page.keyboard.press('Enter'); await page.waitForTimeout(700); expect((await chat()).lines).toBe(2);
  // issue 점 명령 `.flash`(번개 섬광+소리)는 줄을 만들지 않고 오류 없이 처리된다(섬광은 원본 창 모드와 같은 무동작, E1163 판정).
  await page.keyboard.type('.flash 50'); await page.keyboard.press('Enter'); await page.waitForTimeout(700); expect((await chat()).lines).toBe(2);
  // 도우미 `>` 를 누르면 Chat Helper 메뉴(Gump 팝업)가 열린다.
  const helper = await page.evaluate(() => { const app = window.netstorm, b = app._campaignProduction.ui.chat().report().helper.bounds, rect = app.canvas.getBoundingClientRect(); return { x: rect.left + (b.left + b.right) / 2 * rect.width / app._layout.logicalWidth, y: rect.top + (b.top + b.bottom) / 2 * rect.height / app._layout.logicalHeight }; });
  expect((await chat()).popup).toBe(false);
  await page.mouse.move(helper.x, helper.y); await page.mouse.down(); await page.waitForTimeout(100); await page.mouse.up(); await page.waitForTimeout(800);
  expect((await chat()).popup, 'Chat Helper 메뉴').toBe(true);
  // 메뉴 밖을 누르면 닫힌다.
  await page.mouse.click(600, 300); await page.waitForTimeout(600); expect((await chat()).popup).toBe(false);
  expect(errors).toEqual([]);
});

// Tab 이름 완성: 동맹 AI 는 미션 참가 때 채팅 채널 기록(100행)에 등록되므로 그 이름의 앞부분을 치고 Tab 을 누르면 이름이 완성된다. (2026-11-22 WB04-E1173)
test('채팅: 동맹 AI 이름이 Tab 으로 완성된다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type()) && !/willReadFrequently/u.test(message.text())) errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: 3, skirmishAi2Ally: 1, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(2000); await page.locator('canvas').focus();
  const input = () => page.evaluate(() => window.netstorm._campaignProduction.ui.chat().inputText());
  await page.keyboard.press('Enter'); await page.waitForTimeout(400); await page.keyboard.type('Com'); await page.waitForTimeout(400); expect(await input()).toBe('Com');
  await page.keyboard.press('Tab'); await page.waitForTimeout(500); expect(await input()).toMatch(/^Computer/u);
  expect(errors).toEqual([]);
});

// 붙여넣기: 채팅 입력줄이 활성일 때 클립보드 본문이 확정 문자로 입력줄에 들어가고(줄바꿈은 공백), 입력줄이 없으면 기본 동작에 맡긴다. (2026-11-22 WB04-E1175)
test('채팅: 입력줄에 붙여넣기가 들어간다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type()) && !/willReadFrequently/u.test(message.text())) errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(2000); await page.locator('canvas').focus();
  const input = () => page.evaluate(() => window.netstorm._campaignProduction.ui.chat().inputText());
  const paste = (text) => page.evaluate((value) => { const data = new DataTransfer(); data.setData('text/plain', value); const event = new ClipboardEvent('paste', { clipboardData: data, bubbles: true, cancelable: true }); document.dispatchEvent(event); return event.defaultPrevented; }, text);
  // 입력줄이 숨어 있으면 붙여넣기를 가로채지 않는다(기본 동작에 맡김).
  expect(await paste('ignored')).toBe(false);
  await page.keyboard.press('Enter'); await page.waitForTimeout(400); await page.keyboard.type('Hi '); await page.waitForTimeout(300);
  expect(await paste('pasted\r\ntext')).toBe(true); await page.waitForTimeout(500); expect(await input()).toBe('Hi pasted text');
  expect(errors).toEqual([]);
});

// 도우미 메뉴 명령: 삽입(0x02000115)은 입력줄에 글을 넣고, 설정 대입(0x0200006d)은 채팅 옵션을 바꾸며, Edit Format(0x02000105)은 큐를 막지 않고 소비된다. 이전에는 이 명령들이 미구현 사건으로 큐를 3초 막고 오류와 함께 버려졌다. (2026-11-22 WB04-E1177)
test('채팅 도우미 명령: 삽입·설정 대입이 처리되고 입력 큐를 막지 않는다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type()) && !/willReadFrequently/u.test(message.text())) errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(2000); await page.locator('canvas').focus();
  const send = (code, text) => page.evaluate(({ code, text }) => { const app = window.netstorm; app._clientInputQueue.pushCommand(code, 0, text === null ? 0 : app._clientChatWindows.store(text)); }, { code, text });
  const state = () => page.evaluate(() => { const app = window.netstorm; return { input: app._campaignProduction.ui.chat().inputText(), option: app._chatOption('ShowChatWindow'), head: app._clientInputQueue.peek()?.code ?? null }; });
  await page.keyboard.press('Enter'); await page.waitForTimeout(400); await page.keyboard.type('Hi '); await page.waitForTimeout(300);
  expect((await state()).option).toBe(1);
  // 삽입: 입력줄 글 뒤에 문자열이 붙는다.
  await send(0x02000115, '~g'); await page.waitForTimeout(500); expect(await state()).toMatchObject({ input: 'Hi ~g', head: null });
  // 설정 대입: 키=값 문자열이 채팅 옵션을 바꾼다.
  await send(0x0200006d, 'ShowChatWindow=0'); await page.waitForTimeout(500); expect(await state()).toMatchObject({ option: 0, head: null });
  await send(0x0200006d, 'ShowChatWindow=1'); await page.waitForTimeout(500); expect(await state()).toMatchObject({ option: 1, head: null });
  // 뒤따르는 키 입력이 평소처럼 입력줄로 간다.
  await page.keyboard.type('ok'); await page.waitForTimeout(400); expect((await state()).input).toBe('Hi ~gok');
  expect(errors).toEqual([]);
});

// 복사·잘라내기: 입력줄에서 Ctrl+Insert(원본 편집기의 복사 키)로 복사하면 앱 클립보드 사본에 본문이 들어가고, Shift+Home 선택 뒤 Shift+Delete 는 잘라내며, 도우미 `Paste` 삽입이 사본을 입력줄에 넣는다. (2026-11-22 WB04-E1178)
test('채팅: 입력줄 복사·잘라내기와 도우미 Paste 가 클립보드 사본을 쓴다', async ({ page, context }) => {
  test.setTimeout(300000);
  await context.grantPermissions(['clipboard-read', 'clipboard-write']);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type()) && !/willReadFrequently/u.test(message.text())) errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(2000); await page.locator('canvas').focus();
  const state = () => page.evaluate(() => ({ input: window.netstorm._campaignProduction.ui.chat().inputText(), clip: window.netstorm._chatClipboard ?? null }));
  await page.keyboard.press('Enter'); await page.waitForTimeout(400); await page.keyboard.type('copy me'); await page.waitForTimeout(300);
  // 원본 편집기의 복사 키 Ctrl+Insert(선택이 없으면 전체): 입력줄은 그대로이고 사본에 본문이 들어간다.
  await page.keyboard.press('Control+Insert'); await page.waitForTimeout(500);
  expect(await state()).toEqual({ input: 'copy me', clip: 'copy me' });
  expect(await page.evaluate(() => navigator.clipboard.readText())).toBe('copy me');
  // Shift+Home 으로 줄 처음까지 고른 뒤 Shift+Delete 로 잘라내기: 입력줄이 비고 사본은 유지된다.
  await page.keyboard.press('Shift+Home'); await page.keyboard.press('Shift+Delete'); await page.waitForTimeout(500); expect(await state()).toEqual({ input: '', clip: 'copy me' });
  // 도우미 Paste 삽입: 사본이 입력줄에 들어간다.
  await page.evaluate(() => { const app = window.netstorm; app._clientInputQueue.pushCommand(0x02000115, 0, app._clientChatWindows.store('paste')); }); await page.waitForTimeout(500);
  expect(await state()).toEqual({ input: 'copy me', clip: 'copy me' });
  expect(errors).toEqual([]);
});

// Edit Back/Front Format: 도우미 명령(0x02000105/106)이 서식 입력 창을 열고, 글자는 창의 입력창으로만 가며(채팅 입력줄은 그대로), Enter 가 `<구독자>chatFormat`·`chatFormat2` 설정에 저장하고 창을 닫으며 다시 열면 저장값이 초기 문자열이다. (2026-11-22 WB04-E1179)
test('채팅: Edit Back/Front Format 입력 창이 열리고 확정하면 서식 설정이 저장된다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type()) && !/willReadFrequently/u.test(message.text())) errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(2000); await page.locator('canvas').focus();
  const send = (code) => page.evaluate((value) => { window.netstorm._clientInputQueue.pushCommand(value, 0, 0); }, code);
  const state = () => page.evaluate(() => {
    const app = window.netstorm, ui = app._campaignProduction.ui, token = ui.chatFormat().current(), subscriber = app._clientSubscriber.mySubscriber().value | 0;
    const rect = token ? app._clientGump.collection().bounds(token) : null;
    return { open: token !== 0, text: ui.chatFormat().text(), chat: ui.chat().inputText(), back: app._gameData.get(`${subscriber}chatFormat`) ?? null, front: app._gameData.get(`${subscriber}chatFormat2`) ?? null, head: app._clientInputQueue.peek()?.code ?? null, rect };
  });
  await page.keyboard.press('Enter'); await page.waitForTimeout(400); await page.keyboard.type('Hi'); await page.waitForTimeout(300);
  expect(await state()).toMatchObject({ open: false, chat: 'Hi', back: null, front: null });
  // Edit Back Format: 창이 화면 가운데에 열리고 초기 문자열은 빈 설정이다. 글자는 창으로만 간다.
  await send(0x02000105); await page.waitForTimeout(600);
  let current = await state(); expect(current).toMatchObject({ open: true, text: '', chat: 'Hi', head: null });
  expect(current.rect.left).toBeGreaterThan(200); expect(current.rect.right).toBeLessThan(824); expect(current.rect.bottom - current.rect.top).toBeGreaterThan(80);
  await page.keyboard.type('abc ~g'); await page.waitForTimeout(500); expect(await state()).toMatchObject({ open: true, text: 'abc ~g', chat: 'Hi' });
  await page.keyboard.press('Enter'); await page.waitForTimeout(700);
  expect(await state()).toMatchObject({ open: false, chat: 'Hi', back: 'abc ~g', front: null });
  // Edit Front Format 은 `chatFormat2` 에 저장하고, 다시 연 Back 창의 초기 문자열은 저장값이다.
  await send(0x02000106); await page.waitForTimeout(600); expect(await state()).toMatchObject({ open: true, text: '', chat: 'Hi' });
  await page.keyboard.type('[F]'); await page.waitForTimeout(400); await page.keyboard.press('Enter'); await page.waitForTimeout(700);
  expect(await state()).toMatchObject({ open: false, back: 'abc ~g', front: '[F]' });
  await send(0x02000105); await page.waitForTimeout(600); expect(await state()).toMatchObject({ open: true, text: 'abc ~g' });
  await page.keyboard.press('Enter'); await page.waitForTimeout(700); expect((await state()).open).toBe(false);
  // 닫힌 뒤 키 입력은 채팅 입력줄로 돌아간다.
  await page.keyboard.type('!'); await page.waitForTimeout(400); expect((await state()).chat).toBe('Hi!');
  expect(errors).toEqual([]);
});
