// 미션 중 만료된 안내(Game Time 등)가 죽은 Gump 정리 단계에서 등록표와 Kill 큐 밖으로 나가는지 실제 Chromium 에서 확인한다. (2026-11-21 WB04-E1134)
// 원본 메인 루프는 Gump Idle 앞에서 죽은 Gump 정리를 매 프레임 실행한다. 웹에 이 단계가 없던 때는 안내가 Kill 큐와 등록표에 계속 쌓여 3분쯤 뒤 "Gump 원본 개수 한도 초과" 오류가 났다(몽키 시험의 Vicious 에서 발견).
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

test('만료된 안내 Gump 는 정리 단계에서 등록표와 Kill 큐 밖으로 나간다', async ({ page }) => {
  test.setTimeout(180000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { DoneTutorial1: '1', DoneTutorial6: '1' } }));
  });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  // The War Begins! 를 시작하고 안내 창을 모두 닫는다.
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  for (let index = 0; index < 6; index += 1) {
    const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
    if (button === null) break; await clickLabel(page, button); await page.waitForTimeout(400);
  }
  // 등록 Gump 수·그중 안내(WarningNode) 수·Kill 큐 길이를 읽는다. 시작 직후에는 HUD·미니맵 Gump 가 더 만들어질 수 있어 전체 수 대신 안내 수와 증가량으로 판정한다.
  const gumps = () => page.evaluate(() => {
    const root = window.netstorm._clientGump, state = root.createState(); let warnings = 0;
    for (const token of state.entries) if (token && root.node(token)?.constructor?.name === 'WarningNode') warnings += 1;
    return { count: state.count, warnings, kill: root.lifetimeState().killCount };
  });
  await page.waitForTimeout(1500);
  // T 키로 게임 시간 표시를 켜면 원본처럼 매 프레임 0.1초 기한의 Game Time 안내(종류 16)를 요청한다. 정리 단계가 없으면 만료된 안내가 Kill 큐와 등록표에 쌓인다(켠 채 3분쯤 두면 한도 초과 오류).
  await page.keyboard.press('t'); await page.waitForTimeout(3000); const early = await gumps();
  // 정리 단계가 있으면 안내가 만들어지고 지워지는 평형(십여 개)에 머문다. 없으면 초당 약 10개씩 계속 늘어난다.
  await page.waitForTimeout(9000); const late = await gumps();
  expect(late.warnings - early.warnings, '안내 Gump 가 계속 늘면 안 된다').toBeLessThanOrEqual(4); expect(late.count - early.count, '등록 Gump 가 계속 늘면 안 된다').toBeLessThanOrEqual(4); expect(late.kill, 'Kill 큐가 쌓이면 안 된다').toBeLessThanOrEqual(5);
  // 표시를 끄면 남은 안내도 기한 뒤 모두 사라지고 Kill 큐도 빈다.
  await page.keyboard.press('t'); await page.waitForTimeout(1500);
  const after = await gumps(); expect(after.warnings).toBe(0); expect(after.kill).toBe(0);
  expect(errors).toEqual([]);
});
