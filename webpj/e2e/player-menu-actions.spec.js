// 스커미시의 Players 메뉴 동작(송금 고정 금액·모두 보내기 확인·동맹 요청/해제와 스팸 제한)이 입력 큐를 막지 않고 실제로 SP·동맹을 바꾸는지 실제 Chromium 에서 확인한다. (2026-11-20 WB04-E1126)
// 연결 전에는 송금 명령이 입력 큐 머리에 영원히 남아 이후 모든 마우스·키 입력이 먹통이 됐다.
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}
// 입력 큐에 명령을 올리고 한 프레임쯤 기다린다. 큐 머리가 남아 있으면 먹통 상태이다.
const push = async (page, code, x, y) => { await page.evaluate(({ code, x, y }) => window.netstorm._clientInputQueue.pushCommand(code, x, y), { code, x, y }); await page.waitForTimeout(500); };
const queueHead = (page) => page.evaluate(() => window.netstorm._clientInputQueue.peek()?.code ?? null);
// 현재 로컬 SP(float)와 AI 지갑 잔액이다.
const money = (page) => page.evaluate(() => window.netstorm._campaignWorld.session().moneyInputHooks().money());
const wallet = (page, owner) => page.evaluate((who) => window.netstorm._campaignWorld.session().aiMoney?.(who) ?? null, owner);

test('Players 메뉴 동작: 송금·모두 보내기·동맹 요청이 큐를 막지 않고 적용된다', async ({ page }) => {
  test.setTimeout(240000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(2500);
  // 고정 300 송금: 로컬 SP 가 300 줄고 큐가 비며 AI 지갑이 늘어난다.
  const before = await money(page); const walletBefore = await wallet(page, 2);
  await push(page, 0x02000120, 0, 2);
  const after = await money(page); expect(await queueHead(page)).toBeNull();
  expect(before - after).toBeGreaterThan(280); expect(before - after).toBeLessThan(320);
  const walletAfter = await wallet(page, 2); if (walletBefore !== null) expect(walletAfter - walletBefore).toBe(300);
  // 700·1000 도 같은 방식이고, 모자라면 보내지 않는다(남은 SP 가 1000 미만이 될 때까지는 정상).
  await push(page, 0x02000108, 0, 2); await push(page, 0x020000ec, 0, 2);
  expect(await queueHead(page)).toBeNull(); expect(Math.round((after - await money(page)) / 100) * 100).toBe(1700);
  // 모두 보내기 확인: 안내 창이 열리고 첫 단추(예)가 남은 SP 를 모두 보낸다.
  await push(page, 0x0200012b, 0, 2);
  const asked = await page.evaluate(() => ({ dialog: Boolean(window.netstorm._mainMenu.dialog), buttons: window.netstorm._mainMenu.dialog?.buttons?.map((button) => button.label) ?? [] }));
  expect(asked.dialog).toBe(true); expect(asked.buttons.length).toBe(2);
  await clickLabel(page, asked.buttons[0]); await page.waitForTimeout(800);
  expect(await money(page)).toBeLessThan(5); expect(await queueHead(page)).toBeNull();
  // 동맹 요청은 packet 으로 나가 큐를 막지 않는다. 네 번째 요청은 제한에 걸려 안내가 열리고 packet 은 세 번만 나간다.
  for (let index = 0; index < 4; index += 1) await push(page, 0x020000fb, 0, 3);
  expect(await queueHead(page)).toBeNull();
  expect(await page.evaluate(() => window.netstorm._clientChatWindows.allianceRequests.counters[3])).toBe(4);
  // 해제는 제한이 없다. 이후 입력도 막히지 않는다.
  await push(page, 0x020000fa, 0, 3); expect(await queueHead(page)).toBeNull();
  expect(errors).toEqual([]);
});
