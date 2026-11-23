// 단독 미션의 내부 전송이 Client 분배기로 실제로 주는 패킷 종류를 실제 Chromium 에서 확인한다. cpppj `ClientCampaignTransport` 의 Client 기본 등록 18종(`CLIENT_PACKET_DEFAULTS`: 빈 C3 수신기·길이 검증·관찰기)은
// 서버에서 Client 로 오는 패킷의 검증·진단 장치인데, 웹 단독 미션(스커미시 진행 40초)에서 그 종류가 한 번도 도착하지 않아 앱에 설치하지 않아도 효과가 없다. 이 성질이 깨지면(기본 종류 패킷이 도착하면) 이 시험이 먼저 알려 준다. (2026-11-23 WB04-E1274)
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

test('단독 미션: Client 기본 등록 18종의 패킷은 내부 전송으로 도착하지 않는다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/debug.html?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  // 분배기의 dispatch 를 감싸 도착한 패킷 종류(헤더 둘째 바이트)를 센다. 미션 시작 전에 감싸 로그인 때 도착한 패킷도 센다.
  await page.evaluate(async () => {
    const dispatchModule = await import('/core/raw-packet-dispatch.js'), proto = dispatchModule.RawPacketDispatch.prototype, original = proto.dispatch;
    window.__arrived = {};
    proto.dispatch = function (connection, packet) { window.__arrived[packet[1]] = (window.__arrived[packet[1]] ?? 0) + 1; return original.call(this, connection, packet); };
  });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await page.waitForTimeout(40000);
  const { arrived, defaults } = await page.evaluate(async () => {
    const transport = await import('/core/client-campaign-transport.js');
    return { arrived: window.__arrived, defaults: transport.CLIENT_PACKET_DEFAULTS.map((entry) => entry.type) };
  });
  expect(defaults.length, 'Client 기본 등록 종류 수').toBe(18);
  expect(Object.keys(arrived).length, '감싼 분배기가 패킷을 실제로 센다(로그인 때 종류 1 등)').toBeGreaterThan(0);
  expect(Object.keys(arrived).map(Number).filter((type) => defaults.includes(type)), '기본 등록 종류의 패킷 도착').toEqual([]);
  expect(errors).toEqual([]);
});
