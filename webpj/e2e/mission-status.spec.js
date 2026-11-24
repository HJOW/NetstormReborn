// 실제 로컬 송금 수신→현재 MessageBoard 전달·미션 억제·후속 프레임을 확인한다. 자금과 수신 패킷은 명시한 검사 입력이다. (WB04-E1326)
const { test, expect } = require('@playwright/test');

test('미션 SP 수신: 자금 증가 뒤 상태 안내 미연결 예외가 없고 원본 미션 억제를 유지한다', async ({ page }) => {
  test.setTimeout(180000); const errors = [];
  page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/debug.html?fixed=25&aspect=4:3&title=1');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.evaluate(() => window.netstorm._prepareCampaignAudio());
  // 실제 Tutorial3의 시작 그림과 MORE/OK를 진행한다.
  while (await page.evaluate(() => window.netstorm.state !== 'mainMenu')) await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await page.evaluate(() => window.netstorm.stepFixedFrames(1)); await page.evaluate(() => window.netstorm._beginCampaignFromDialog('Tutorial3')); await page.evaluate(() => window.netstorm.stepFixedFrames(120));
  for (let index = 0; index < 6; index += 1) {
    const point = await page.evaluate(() => window.netstorm._mainMenu.controlPoint('MORE') ?? window.netstorm._mainMenu.controlPoint('OK'));
    if (!point) break; await page.mouse.click(point.x, point.y); await page.evaluate(() => window.netstorm.stepFixedFrames(5));
  }
  const result = await page.evaluate(() => {
    const app = window.netstorm, session = app._campaignWorld.session(), messages = app._campaignProduction.ui.warnings(), calls = [];
    const status = messages.battleStatus.bind(messages); messages.battleStatus = (...args) => { calls.push(args); return status(...args); };
    // 송신자 자금2000·수신125 SP는 검사 입력이다. 자연 AI 송금이나 자연 승리를 주장하지 않는다.
    const bits = new DataView(new ArrayBuffer(4)); bits.setFloat32(0, 2000, true); session.spStorage().set(2, bits.getUint32(0, true));
    const before = session.sp(1), packet = Uint8Array.of(1, 0x6c, 11, 0, 1, 2, 0, 125, 0, 0, 0);
    session.sendLocalPacket(9, packet); session.pumpLocalConnection();
    const after = session.sp(1), suppressed = messages.report(), received = calls.slice();
    // 종류16은 미션에서도 허용되는 실제 Game Time 안내다. 미션 플래그를 바꾸지 않고 같은 MessageBoard의 양성 경로를 확인한다.
    app._campaignLoginServices().status(16, 0, 0); const visible = messages.report(); messages.remove(16, 0, 0);
    const frame = app._gameClock.time().number;
    return { before, after, received, suppressed, visible, frame, mission: Boolean(app._clientGameplay.world.missionPost) };
  });
  expect(result.after - result.before).toBe(125); expect(result.received).toEqual([[4, 2, 125]]); expect(result.mission).toBe(true);
  expect(result.suppressed).toContain('warning_count\t0'); expect(result.visible).toContain('warning_count\t1'); expect(result.visible).toContain('Game Time');
  await page.evaluate(() => window.netstorm.stepFixedFrames(10));
  expect(await page.evaluate(() => window.netstorm._gameClock.time().number)).toBe(result.frame + 10);
  const missing = await page.evaluate(() => {
    const app = window.netstorm, current = app._campaignProduction; app._campaignProduction = null;
    try { app._campaignLoginServices().status(4, 2, 125); return null; } catch (error) { return error.message; } finally { app._campaignProduction = current; }
  });
  expect(missing).toMatch(/실제 MessageBoard 미연결/); expect(errors).toEqual([]);
});
