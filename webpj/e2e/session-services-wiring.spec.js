// RawCampaignSession 공개 경계 감사에서 찾은 앱 서비스 5건(kencloudGlobals·htmlWindowGlobals·factoryHelp·removeConnectionStatus·templateStateBytes)의 실제 미션 연결을 확인한다. (2026-11-24 WB04-E1328)
const { test, expect } = require('@playwright/test');

test('실제 미션 서비스 연결: 구름 redraw·HTML 전역·Factory 연구 도움말·연결 상태 삭제·빈 State 바이트', async ({ page }) => {
  test.setTimeout(180000);
  const errors = [];
  page.on('pageerror', (error) => errors.push(error.message));
  page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 });
  await page.goto('/debug.html?fixed=25&aspect=4:3&title=1');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  await page.locator('canvas').click();
  await page.evaluate(() => window.netstorm._prepareCampaignAudio());
  // 타이틀 화면을 지나 메인 메뉴까지 고정 프레임으로 전진한다.
  while (await page.evaluate(() => window.netstorm.state !== 'mainMenu')) await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  // 워크샾이 시작부터 놓여 있는 Tutorial3을 시작하고 첫 안내 창을 닫는다.
  await page.evaluate(() => window.netstorm._beginCampaignFromDialog('Tutorial3'));
  await page.evaluate(() => window.netstorm.stepFixedFrames(120));
  // 시작 브리핑/안내 창을 닫고 실제 미션 플레이 상태로 들어간다.
  for (let index = 0; index < 6; index += 1) {
    const point = await page.evaluate(() => window.netstorm._mainMenu.controlPoint('MORE') ?? window.netstorm._mainMenu.controlPoint('OK'));
    if (!point) break;
    await page.mouse.click(point.x, point.y);
    await page.evaluate(() => window.netstorm.stepFixedFrames(5));
  }

  const check = await page.evaluate(() => {
    const app = window.netstorm;
    const session = app._campaignWorld.session();
    const shared = app._campaignSharedServices();
    const gameplay = app._campaignGameplayServices();
    const core = session.core();

    // 1. kencloudGlobals·htmlWindowGlobals 공유 확인 및 worldRedraw 전파(0x0059a8b0) 확인.
    const sameKencloud = shared.kencloudGlobals === app._clientKencloud;
    const sameHtml = shared.htmlWindowGlobals === app._clientHtml;
    const redrawBox = app._clientKencloud.worldRedraw();
    redrawBox.value = 0;
    const priestSid = session.findPriestParticipation(1) & 0xffff;
    const home = { x: 0, y: 0, width: 1024, height: 768 };
    // centerCameraOn(00433aa0)이 세션 내부 worldRedraw를 증가시켜 실제 ClientKencloudGlobals.worldRedraw()에 반영되는지 확인한다.
    session.centerCameraOn(priestSid, home, () => {});
    const redrawAfterCenter = redrawBox.value;
    // commitRequestedCamera가 전달하는 worldRedraw 상자도 같은 ClientKencloudGlobals 소유 상자인지 확인한다.
    let commitSameBox = false;
    const origCommit = app._commitCampaignCamera.bind(app);
    app._commitCampaignCamera = (context) => {
      commitSameBox = context.worldRedraw === redrawBox;
      return origCommit(context);
    };
    try {
      session.commitRequestedCamera(1024, 768);
    } finally {
      app._commitCampaignCamera = origCommit;
    }

    // 2. removeConnectionStatus 연결 확인: Game Time(종류 16) 표시 뒤 removeConnectionStatus(16, 0)으로 제거한다.
    const messages = app._campaignProduction.ui.warnings();
    app._campaignLoginServices().status(16, 0, 0);
    const beforeRemove = messages.report();
    gameplay.removeConnectionStatus(16, 0);
    const afterRemove = messages.report();

    // 3. templateStateBytes 연결 확인: 28바이트 빈 버퍼 호출이 정상 반환되는지 확인한다.
    const stateSample = new Uint8Array(28);
    let templateStateOk = false;
    if (typeof gameplay.templateStateBytes === 'function') {
      gameplay.templateStateBytes(stateSample);
      templateStateOk = true;
    }

    // 4. factoryHelp 연결 확인: 실제 Factory 슬롯을 찾아 0x03000080(kind=0) 호출 시 researchHelp 도움말 창이 열리는지 확인한다.
    let factorySid = 0;
    // 풀에서 아군 일반 Factory(VT 0x509148) 하나를 찾는다.
    for (let sid = 1; sid < core.pool.capacity; sid += 1) {
      const raw = core.pool.slot(sid);
      if (raw[11] & 1) continue;
      const vt = new DataView(raw.buffer, raw.byteOffset, 4).getUint32(0, true);
      if (vt === 0x509148 && raw[34] === 1) { factorySid = sid; break; }
    }
    let helpOpened = false;
    let helpTopic = null;
    if (factorySid) {
      session.commandFactory(factorySid, 0x03000080, factorySid);
      const dialog = app._mainMenu?.dialog;
      helpOpened = Boolean(dialog && !dialog.closed);
      helpTopic = dialog?.currentTopic ?? dialog?.topic ?? null;
      if (dialog && typeof dialog.close === 'function') {
        dialog.close();
        app._mainMenu.setDialog(null);
      }
    }
    return { sameKencloud, sameHtml, priestSid, redrawAfterCenter, commitSameBox, beforeRemove, afterRemove, templateStateOk, factorySid, helpOpened, helpTopic };
  });

  expect(check.sameKencloud).toBe(true);
  expect(check.sameHtml).toBe(true);
  expect(check.priestSid).toBeGreaterThan(0);
  expect(check.redrawAfterCenter).toBeGreaterThan(0);
  expect(check.commitSameBox).toBe(true);
  expect(check.beforeRemove).toContain('warning_count\t1');
  expect(check.afterRemove).toContain('warning_count\t0');
  expect(check.templateStateOk).toBe(true);
  expect(check.factorySid).toBeGreaterThan(0);
  expect(check.helpOpened).toBe(true);

  // 닫은 뒤 후속 10프레임이 오류 없이 진행되고 렌더러가 worldRedraw를 0으로 소비하는지 확인한다.
  const frameBefore = await page.evaluate(() => window.netstorm._gameClock.time().number);
  await page.evaluate(() => window.netstorm.stepFixedFrames(10));
  const after = await page.evaluate(() => ({ frame: window.netstorm._gameClock.time().number, redraw: window.netstorm._clientKencloud.worldRedraw().value }));
  expect(after.frame).toBe(frameBefore + 10);
  expect(after.redraw).toBe(0);
  expect(errors).toEqual([]);
});
