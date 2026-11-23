// 미션을 떠날 때 원본 State 월드 정리의 두 효과를 실제 Chromium 에서 확인한다: 점유된 채팅 채널 기록 정리(ChatChannels::ResetOccupied)와 시간 동기화 세 DWORD 초기화(ResetClientClockSynchronization).
// 시간 동기화의 응답 방식(replyMode)은 미션 로그인 중에 1 이 되고(자연 값), 채팅 기록은 점유 기록을 직접 넣어 정리 연결을 확인한다. (2026-11-23 WB04-E1268)
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

// 채팅 기록 100개 중 Subscriber(+44)가 0 이 아닌 기록 수와 시간 동기화 세 DWORD 이다.
const occupied = (page) => page.evaluate(() => {
  const records = window.netstorm._clientChatRecords; let count = 0;
  for (let index = 0; index < 100; index += 1) { const at = index * 48 + 44; if (((records[at] | (records[at + 1] << 8) | (records[at + 2] << 16) | (records[at + 3] << 24)) >>> 0) !== 0) count += 1; }
  return count;
});
const synchronization = (page) => page.evaluate(() => { const state = window.netstorm._clientLocal.clockSynchronization(); return [state.connection, state.replyMode, state.gameTicks]; });

test('스커미시 미션을 떠나면 점유된 채팅 채널 기록과 시간 동기화가 처음 상태로 돌아간다', async ({ page }) => {
  test.setTimeout(240000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  expect([await occupied(page), await synchronization(page)], '메뉴 처음 상태').toEqual([0, [0, 0, 0]]);
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  // 로그인 중 시간 응답 방식이 1 이 된다(자연 값). 점유 기록 하나를 직접 넣는다(Subscriber 7·이름 `Bob`).
  expect((await synchronization(page))[1], '미션 중 응답 방식').toBe(1);
  await page.evaluate(() => { const records = window.netstorm._clientChatRecords, at = 3 * 48; records[at] = 0x42; records[at + 1] = 0x6f; records[at + 2] = 0x62; records[at + 3] = 0; records[at + 44] = 7; });
  expect(await occupied(page), '점유 기록').toBe(1);
  await page.evaluate(() => window.netstorm._leaveCampaignMission()); await page.waitForTimeout(800);
  expect([await occupied(page), await synchronization(page)], '미션을 떠난 뒤').toEqual([0, [0, 0, 0]]);
  expect(await page.evaluate(() => Array.from(window.netstorm._clientChatRecords.subarray(3 * 48, 3 * 48 + 48)).every((byte) => byte === 0)), '기록 48바이트 전부 0').toBe(true);
  expect(errors).toEqual([]);
});
