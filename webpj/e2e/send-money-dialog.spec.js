// Players ▸ Send Money ▸ Send X 의 숫자 입력 창(Send X Storm Power)이 실제 Chromium 에서 열리고, 숫자만 받으며, OK/Enter 가 송금하고 Cancel 이 닫는지 확인한다. (2026-11-22 WB04-E1168)
// 원본은 편집용 이름 입력과 같은 모달 대화창(SaveFortNameDialog::ShowMoney)을 쓴다. 웹 앱은 Gump 트리에 장식 틀·입력창·돌 버튼을 만들어 모달로 올린다.
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => { const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect(); return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height }; }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}
// 입력 큐에 명령을 올리고 잠시 기다린다.
const push = async (page, code, x, y) => { await page.evaluate(({ code, x, y }) => window.netstorm._clientInputQueue.pushCommand(code, x, y), { code, x, y }); await page.waitForTimeout(600); };
// 입력 큐 머리의 명령 번호다(비어 있으면 null, 남아 있으면 입력이 먹통이다).
const queueHead = (page) => page.evaluate(() => window.netstorm._clientInputQueue.peek()?.code ?? null);
// 현재 로컬 SP(float)와 AI 지갑 잔액이다.
const money = (page) => page.evaluate(() => window.netstorm._campaignWorld.session().moneyInputHooks().money());
const wallet = (page, owner) => page.evaluate((who) => window.netstorm._campaignWorld.session().aiMoney?.(who) ?? null, owner);
// 열린 송금 창의 상태다: 토큰(없으면 0), 대상·명령·입력 문자열·글자 수 한도, 두 단추와 입력창의 화면 좌표(브라우저 픽셀)다.
const dialogState = (page) => page.evaluate(() => {
  const app = window.netstorm, dialog = app._campaignProduction.ui.sendMoney(), token = dialog.current(); if (!token) return { token: 0 };
  const root = app._clientGump, node = root.node(token), entry = root.node(node.entry), rect = app.canvas.getBoundingClientRect();
  const point = (child) => { const b = root.collection().bounds(child); return { x: rect.left + (b.left + b.right) / 2 * rect.width / app._layout.logicalWidth, y: rect.top + (b.top + b.bottom) / 2 * rect.height / app._layout.logicalHeight }; };
  return { token, mode: node.fortMode, command: node.command, maxLength: entry.maxLength, text: dialog.text(), ok: point(node.ok), cancel: point(node.cancel), entry: point(node.entry), bounds: root.collection().bounds(token) };
});
// 창이 닫힐 때까지(지연 삭제 포함) 기다린다.
const closed = (page) => page.waitForFunction(() => window.netstorm._campaignProduction.ui.sendMoney().current() === 0, null, { timeout: 10000 });

test('Send X 숫자 창: 숫자만 받고 OK/Enter 로 송금하며 Cancel 은 닫기만 한다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type())) errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(2500);

  // Send X(0x02000100) 명령은 대상 owner 2 로 숫자 창을 연다. 처음 입력은 선택된 `0` 이고, 창은 화면 가운데(200×104)에 놓인다.
  await push(page, 0x02000100, 0, 2);
  const opened = await dialogState(page);
  expect(opened.token, '송금 숫자 창').not.toBe(0); expect(opened.command).toBe(68); expect(opened.mode).toBe(2); expect(opened.maxLength).toBe(10); expect(opened.text).toBe('0');
  expect(opened.bounds).toEqual({ left: 412, top: 332, right: 612, bottom: 436 }); expect(await queueHead(page)).toBeNull();
  // 이미 열려 있으면 같은 명령이 두 번째 창을 만들지 않는다.
  await push(page, 0x02000100, 0, 3); expect((await dialogState(page)).token).toBe(opened.token); expect((await dialogState(page)).mode).toBe(2);

  // 숫자만 받는다: 선택된 0 이 첫 숫자로 바뀌고 문자·기호·공백은 무시된다. 아홉 문자를 넘는 입력도 무시된다.
  await page.keyboard.type('5q0-0 '); expect((await dialogState(page)).text).toBe('500');
  await page.keyboard.press('Backspace'); await page.keyboard.type('01234567890123'); expect((await dialogState(page)).text).toBe('500123456');
  // Backspace 를 모두 눌러 비우면 확정해도 송금되지 않는다(양수만 송금). 창은 닫힌다(원문은 잘못된 금액도 1로 닫는다).
  const before = await money(page), walletBefore = await wallet(page, 2);
  for (let index = 0; index < 12; index += 1) await page.keyboard.press('Backspace');
  expect((await dialogState(page)).text).toBe(''); await page.keyboard.press('Enter'); await closed(page);
  expect(await money(page)).toBe(before); expect(await queueHead(page)).toBeNull();

  // Enter 로 확정: 500 이 로컬 SP 에서 빠지고 AI 지갑에 더해진다. 창이 닫히고 입력이 이어진다. 게임 단축키(숫자 화면 저장 등)로 새지 않는다.
  await push(page, 0x02000100, 0, 2); await page.keyboard.type('500'); await page.keyboard.press('Enter'); await closed(page);
  const sent = before - await money(page); expect(sent).toBeGreaterThan(480); expect(sent).toBeLessThan(520);
  if (walletBefore !== null) expect(await wallet(page, 2) - walletBefore).toBe(500);
  expect(await queueHead(page)).toBeNull();

  // OK 단추(마우스)로 확정: 이번에는 300 이다.
  const afterEnter = await money(page), walletMid = await wallet(page, 2);
  await push(page, 0x02000100, 0, 2); await page.keyboard.type('300'); const ok = (await dialogState(page)).ok;
  await page.mouse.move(ok.x, ok.y); await page.mouse.down(); await page.waitForTimeout(100); await page.mouse.up(); await closed(page);
  const sentOk = afterEnter - await money(page); expect(sentOk).toBeGreaterThan(280); expect(sentOk).toBeLessThan(320);
  if (walletMid !== null) expect(await wallet(page, 2) - walletMid).toBe(300);

  // Cancel 단추: 입력한 값을 보내지 않고 닫는다.
  const afterOk = await money(page), walletEnd = await wallet(page, 2);
  await push(page, 0x02000100, 0, 2); await page.keyboard.type('700'); const cancel = (await dialogState(page)).cancel;
  await page.mouse.move(cancel.x, cancel.y); await page.mouse.down(); await page.waitForTimeout(100); await page.mouse.up(); await closed(page);
  expect(Math.abs(afterOk - await money(page))).toBeLessThan(25); // 시간에 따른 SP 증가분만 있을 수 있다.
  if (walletEnd !== null) expect(await wallet(page, 2)).toBe(walletEnd);

  // 모달이 열린 동안 화면 아무 곳을 눌러도 월드로 새지 않는다: 닫힌 뒤 입력 큐는 비어 있고 같은 명령으로 다시 열 수 있다.
  await push(page, 0x02000100, 0, 4); expect((await dialogState(page)).mode).toBe(4);
  await page.mouse.click(800, 600); await page.waitForTimeout(300); expect((await dialogState(page)).token).not.toBe(0); expect(await queueHead(page)).toBeNull();
  const final = await dialogState(page); await page.mouse.move(final.cancel.x, final.cancel.y); await page.mouse.down(); await page.waitForTimeout(100); await page.mouse.up(); await closed(page);
  expect(errors).toEqual([]);
});

// 한국어 모드: OK/Cancel 돌 버튼 라벨이 게임 표시 부호화(선두 0x1f 네 바이트)로 `확인`·`취소` 가 되고 혼합 글자로 측정된다(폭 0 이 아님). (2026-11-22 WB04-E1168)
test('Send X 숫자 창: 한국어 모드의 두 단추 라벨이 확인·취소로 부호화·측정된다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type())) errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, language: 2, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=ko&mute');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, '캠페인');
  const items = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.map((item) => item.label)); await clickLabel(page, items.find((label) => /스커미시/u.test(label)));
  const buttons = await page.evaluate(() => window.netstorm._mainMenu.dialog.buttons.map((button) => button.label)); await clickLabel(page, buttons.find((label) => /시작/u.test(label)));
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  for (let quiet = 0; quiet < 6;) { const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null); if (label === null) { quiet += 1; await page.waitForTimeout(500); continue; } quiet = 0; await clickLabel(page, label); await page.waitForTimeout(400); }
  await page.waitForTimeout(1500); await push(page, 0x02000100, 0, 2);
  const labels = await page.evaluate(async () => {
    const { displayTextCodec } = await import('/assets/display-text.js'), app = window.netstorm, root = app._clientGump, dialog = app._campaignProduction.ui.sendMoney(), node = root.node(dialog.current());
    const read = (token) => { const stone = root.node(token), end = stone.label.indexOf(0); return { text: displayTextCodec.decode(stone.label.subarray(0, end)), bytes: end, width: stone.textWidth, height: stone.textHeight, extended: stone.label[0] === 0x1f }; };
    return { ok: read(node.ok), cancel: read(node.cancel) };
  });
  expect(labels.ok).toMatchObject({ text: '확인', extended: true }); expect(labels.cancel).toMatchObject({ text: '취소', extended: true });
  expect(labels.ok.width).toBeGreaterThan(10); expect(labels.ok.height).toBeGreaterThan(5); expect(labels.cancel.width).toBeGreaterThan(10);
  expect(errors).toEqual([]);
});
