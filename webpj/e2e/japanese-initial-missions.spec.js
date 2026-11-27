// 실제운영번들의일본어31미션진입/첫안내만확인한다. Play/Start/Next/유닛명령은실행하지않는다.
const { test, expect } = require('@playwright/test');
// 첫안내31개상태를보고서와독립된JSON에도보존한다.
const fs = require('node:fs');
// 첫방문자동언어감지도검증한다. 명시적저장/URL언어는넣지않는다.
test.use({ locale: 'ja-JP' });
// 공식31캠페인대본이다. 데모자동플레이/완료값합성/자연승리는이검사의대상이아니다.
const MISSIONS = ['Tutorial1','Tutorial2','Tutorial3','Tutorial4','Tutorial5','Tutorial6','TheWarBegins','MasterOfWhirligigs','SaveTheIsland','FragileFortune','ThunderingPower','DissolvedAlliance','TheNoose','RainVsRain','RunForIt','GuardMyBack','Surrounded','BreakingThrough','ToTheRescue','Vicious','EnemyTerritory','FinalConfrontation','bc1menu76','bc2bridging76','bc3acid_bar76','bc4bow_rush76','bc5walkers76','bc6mows76','bc7sun_bars76','bc8fury76','bc9ssun_bars76'];
// 실제글자배치검토용대표창만스크린샷을남긴다.31개전체상태는JSON으로보존한다.
const CAPTURES = new Set(['Tutorial1','TheWarBegins','TheNoose','FinalConfrontation','bc6mows76']);

test('일본어31캠페인: 실제번들진입과첫안내의창/줄폭', async ({ page }, info) => {
  test.skip(process.env.NETSTORM_E2E_JAPANESE !== '1', '실제CDN/31미션진입은 NETSTORM_E2E_JAPANESE=1에서실행한다.');
  test.setTimeout(600000);
  // 오류와첫안내상태만수집한다. 목표/체력/자원/완료값은쓰지않는다.
  const errors = [], rows = [];
  page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 });
  expect(await page.evaluate(() => window.netstorm.options.language)).toBe('japanese');
  await page.locator('canvas').click(); await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  // 같은앱에서미션을반납하고실제원래진입경로로다음대본을적재한다. 잠금/진행저장은변경하지않는다.
  for (const mission of MISSIONS) {
    await page.evaluate(() => window.netstorm._leaveCampaignMission());
    await page.waitForFunction(() => !window.netstorm._campaignWorld && !window.netstorm._clientGuide.current());
    await page.evaluate((name) => window.netstorm._beginCampaignFromDialog(name), mission);
    await page.waitForFunction((name) => {
      const app = window.netstorm, current = app._clientGuide.current();
      return current?.script.name.toLowerCase() === name.toLowerCase() && Boolean(app._campaignWorld?.session().localConnectionReady()) && Boolean(app._mainMenu.dialog);
    }, mission, { timeout: 60000 });
    // 첫안내가보이는즉시읽는다. 안내의다음/시작단추는누르지않는다.
    const state = await page.evaluate(() => {
      const app = window.netstorm, current = app._clientGuide.current(), dialog = app._mainMenu.dialog, menu = document.querySelector('[data-netstorm="game-menu-button"]');
      return { mission: current.script.name, title: current.script.get('title'), language: app._gameData.language, x: dialog.x, width: dialog.width, logicalWidth: app._layout.logicalWidth, maxLine: dialog.body.maximumWidth(), bodyWidth: dialog.body.baseWidth, buttons: dialog.buttons.map((button) => button.label), menuFont: menu ? getComputedStyle(menu).fontFamily : null, time: app._gameClock.gameSeconds() };
    });
    rows.push(state); console.log(JSON.stringify(state));
    expect(state.language, mission).toBe('japanese'); expect(state.title, mission).toMatch(/[\u3040-\u30ff\u4e00-\u9fff]/u);
    expect(state.x, mission).toBeGreaterThanOrEqual(0); expect(state.x + state.width, mission).toBeLessThanOrEqual(state.logicalWidth);
    expect(state.maxLine, mission).toBeLessThanOrEqual(state.bodyWidth); if (state.menuFont !== null) expect(state.menuFont, mission).toContain('Noto Sans JP');
    expect(errors).toEqual([]);
    if (CAPTURES.has(mission)) await page.screenshot({ path: info.outputPath(mission + '.png') });
  }
  expect(rows.length).toBe(31); expect(rows.some((row) => row.menuFont?.includes('Noto Sans JP'))).toBe(true);
  const statePath = info.outputPath('initial-missions.json'); fs.writeFileSync(statePath, JSON.stringify({ scope: '진입/첫안내만;미션시작/목표/승리검사아님', rows, errors }, null, 2));
  await info.attach('initial-missions', { path: statePath, contentType: 'application/json' });
});
