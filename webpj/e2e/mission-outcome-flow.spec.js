// 미션 승패 안내와 그 뒤 흐름(실패 → Continue → 다시 시도 묻기 → Replay Mission, 성공 → Leave Missions/Next Mission, 진행 저장과 다음 미션 해제)을
// 실제 Chromium 에서 확인한다. (2026-11-21 WB04-E1152)
// 승패 조건은 실제 판정기(`RawMissionOutcome.check`)가 읽는 Player 활성 플래그를 바꿔 만든다(로컬 번호를 비활성으로 하면 사제 사망 → Failed, 적 번호를 비활성으로 하면 적 팀 전멸 → BadTeamDead).
// 진짜 사제 사망·포획은 적 AI 가 건너와야 해 수십 분이 걸리므로 판정의 입력만 바꾸고, 판정 → 대본 구역 안내 창 → 단추 → 미션 전환·진행 저장은 실제 경로를 그대로 쓴다.
const { test, expect } = require('@playwright/test');

// 화면비를 환경 변수 NETSTORM_E2E_ASPECT('4:3' 기본, '16:9', '16:10')로 고른다.
const ASPECT = process.env.NETSTORM_E2E_ASPECT ?? '4:3';
const VIEWPORTS = { '4:3': { width: 1024, height: 768 }, '16:9': { width: 1280, height: 720 }, '16:10': { width: 1280, height: 800 } };
if (!VIEWPORTS[ASPECT]) throw new Error(`지원하지 않는 화면비 ${ASPECT}`);

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 새 페이지의 오류·경고를 모은다.
function collectErrors(page) { const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); }); return errors; }

// The War Begins! 를 시작하고 안내 창을 모두 닫아 시계가 흐르게 한다.
async function startWarBegins(page) {
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6']) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  });
  await page.setViewportSize(VIEWPORTS[ASPECT]); await page.goto(`/?aspect=${ASPECT}`);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await closeDialogs(page);
}

// 열려 있는 안내 창을 마지막(주) 단추로 닫고, 창이 없고 게임 시계가 흐를 때까지 반복한다(안내 창 사이의 짧은 빈 순간에 끝나지 않도록 시계도 확인한다).
async function closeDialogs(page) {
  for (let index = 0; index < 40; index += 1) {
    const state = await page.evaluate(() => ({ button: window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null, paused: window.netstorm._gameClock.paused() }));
    if (state.button !== null) { await clickLabel(page, state.button); await page.waitForTimeout(500); continue; }
    if (!state.paused) { await page.waitForTimeout(400); if (!(await page.evaluate(() => window.netstorm._gameClock.paused() || Boolean(window.netstorm._mainMenu.dialog)))) return; continue; }
    await page.waitForTimeout(500);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 20000 });
}

// 현재 안내 창의 단추 이름 목록이다(창이 없으면 null).
const buttons = (page) => page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.map((button) => button.label) ?? null);

// 지정한 단추 목록의 안내 창이 열릴 때까지 기다린다.
async function waitButtons(page, expected, message) {
  await expect.poll(() => buttons(page), { timeout: 15000, message }).toEqual(expected);
}

// 플레이어 번호의 활성 플래그를 지워 판정기가 그 플레이어를 전멸로 보게 한다(로컬 1 → 사제 사망·실패, 적 2 → 적 팀 전멸·성공).
const knockOut = (page, owner) => page.evaluate((target) => { const records = window.netstorm._campaignWorld.session().playerRecords(); records[target * 180 + 0x50] = 0; }, owner);

// 진행 저장소에 저장된 값 중 `Done` 으로 시작하는 키 이름(소문자)이다.
const savedDone = (page) => page.evaluate(() => { try { return Object.keys(JSON.parse(localStorage.getItem('netstorm.progress') ?? '{}').values ?? {}).filter((key) => /^done/i.test(key)).map((key) => key.toLowerCase()); } catch { return null; } });

test('The War Begins!: 실패 → Continue → 다시 시도 묻기 → Replay Mission 으로 미션이 다시 시작된다', async ({ page }) => {
  test.setTimeout(300000); const errors = collectErrors(page);
  await startWarBegins(page);
  await knockOut(page, 1);
  // 대본 [Failed] 구역: 한 단추(Continue). 시계는 멈춘다. 다음은 TryAgain 의 두 단추이다.
  await waitButtons(page, ['Continue'], '실패 안내(Continue)'); expect(await page.evaluate(() => window.netstorm._gameClock.paused()), '안내 중 시계 정지').toBe(true);
  await clickLabel(page, 'Continue'); await waitButtons(page, ['Replay Mission', 'Leave Missions'], '다시 시도 묻기');
  expect(await savedDone(page), '실패는 진행에 기록되지 않는다').not.toContain('donethewarbegins');
  // Replay Mission → 같은 미션의 브리핑(Review Knowledge · Play Mission) → Play Mission 으로 새로 시작.
  await clickLabel(page, 'Replay Mission'); await waitButtons(page, ['Review Knowledge', 'Play Mission'], '다시 시작한 브리핑');
  await clickLabel(page, 'Play Mission'); await closeDialogs(page);
  expect(await page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(); return { name: app._clientGuide.current().script.name, dead: session.missionPriestDead(1), priest: session.findPriestParticipation(session.localOwner()) > 0, paused: app._gameClock.paused() }; }), '다시 시작한 미션')
    .toEqual({ name: 'TheWarBegins', dead: false, priest: true, paused: false });
  // 실패 안내가 다시 뜨지 않고 게임이 계속 흐른다.
  await page.waitForTimeout(2500); expect(await buttons(page), '다시 시작 뒤 안내 창 없음').toBeNull();
  expect(errors).toEqual([]);
});

test('The War Begins!: 성공 → Leave Missions 로 나가면 진행이 저장되고 캠페인 목록에 체크 표시와 다음 미션 해제가 나온다', async ({ page }) => {
  test.setTimeout(300000); const errors = collectErrors(page);
  await startWarBegins(page);
  await knockOut(page, 2);
  await waitButtons(page, ['Leave Missions', 'Next Mission'], '성공 안내'); expect(await savedDone(page), '성공은 진행에 기록된다').toContain('donethewarbegins');
  await clickLabel(page, 'Leave Missions');
  await page.waitForFunction(() => !window.netstorm._campaignDisplayActive() && !window.netstorm._mainMenu.dialog, null, { timeout: 15000 });
  // 목록을 다시 열면 1번은 체크, 2번(Master of Whirligigs)은 풀려 있다.
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom'); await page.waitForTimeout(500);
  const rows = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.slice(0, 3).map((item) => ({ label: item.label, checked: item.checked, disabled: item.disabled })));
  expect(rows).toEqual([{ label: '1 The War Begins!', checked: true, disabled: false }, { label: '2 Master of Whirligigs', checked: false, disabled: false }, { label: '3 Save the Island!', checked: false, disabled: true }]);
  expect(errors).toEqual([]);
});

test('The War Begins!: 성공 → Next Mission 으로 다음 미션(Master of Whirligigs)이 시작된다', async ({ page }) => {
  test.setTimeout(300000); const errors = collectErrors(page);
  await startWarBegins(page);
  await knockOut(page, 2); await waitButtons(page, ['Leave Missions', 'Next Mission'], '성공 안내');
  await clickLabel(page, 'Next Mission'); await waitButtons(page, ['Review Knowledge', 'Play Mission'], '다음 미션 브리핑');
  expect(await page.evaluate(() => window.netstorm._clientGuide.current().script.name), '다음 미션 이름').toBe('MasterOfWhirligigs');
  await clickLabel(page, 'Play Mission'); await closeDialogs(page);
  expect(await page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(); return { name: app._clientGuide.current().script.name, dead: session.missionPriestDead(1), paused: app._gameClock.paused() }; }), '다음 미션 진행').toEqual({ name: 'MasterOfWhirligigs', dead: false, paused: false });
  expect(errors).toEqual([]);
});
