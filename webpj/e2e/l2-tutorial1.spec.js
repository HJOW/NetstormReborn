// L2 대조: 실제 Chromium 에서 Tutorial1 을 시작한 직후의 Session 덤프를 cpppj 기준선(225553df)의 같은 시점 덤프(`load.tsv`)와 행별로 비교한다. (2026-11-21 WB04-E1129)
// 웹은 실제 시각으로 돌아 시계·난수·정기 프로세스 시각·풀 해시와 애니메이션 프레임 같은 시간 의존 값이 다르다. 그 밖의 모든 행(동맹·Graph·장부·Kernel·슬롯 원문)이 같음을 확인한다.
// 허용하는 차이(기준선 덤프에 대한 웹 덤프, 같은 세 번 이상 실행에서 같은 모양으로 재현됨):
//  ① 행 frame·random·regular:*·pool(시계·난수·풀 해시)
//  ② SP 행의 분산 오프셋(셋째 칸)과 소유자 0 값(넷째 칸) — 둘 다 실행마다 난수이다. 초기화 여부와 소유자 1~8 값은 같아야 한다.
//  ③ Player 기록 9개의 앞 4바이트(주소 잔재처럼 환경마다 다른 값)와 Player 1 의 +0x68·+0x69(원인 미분류).
//  ④ 슬롯 +36..39(애니메이션 프레임)·+33(위상)·+31(앰비언트 객체 나이)·+4·+5(해시 체인 연결)·SID 15001 의 +42(시간에 따라 오르는 카운터)와 앰비언트 바람 객체(SID 0 의 개수 바이트, 15·16·17: 존재·내용 모두 허용).
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 15000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 줄바꿈과 탭이 든 덤프 텍스트에서 지정한 종류의 행 표(번호 → 마지막 칸)를 읽는다.
function rowsOf(text, kind) {
  const rows = new Map();
  for (const line of text.split('\n')) { const cells = line.split('\t'); if (cells[0] === kind) rows.set(Number(cells[1]), cells[2]); }
  return rows;
}

test('Tutorial1 시작 직후 Session 덤프가 cpppj 기준선과 알려진 시간 의존 값만 다르다', async ({ page }) => {
  test.setTimeout(120000);
  const { compareSessionDumps } = await import('../tools/session-dump-compare.mjs');
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/debug.html?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Early Missions');
  const first = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial1')?.label);
  // 미션 진행 창("Starting Mission...")이 뜨는 순간의 위치를 기록해 둔다: cpppj State7 처럼 왼쪽 85px 를 뺀 영역 가운데여야 한다. (2026-11-21 WB04-E1131)
  await page.evaluate(() => {
    const app = window.netstorm, original = app._showMissionLoading.bind(app); window.__loadingShown = [];
    app._showMissionLoading = (message, view) => { original(message, view); const dialog = app._missionLoadingWindow; window.__loadingShown.push({ message, left: view?.left ?? 0, right: view?.right ?? 1024, x: dialog.x, width: dialog.baseWidth }); };
  });
  await clickLabel(page, first);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 60000 });
  // 첫 안내 창("NetStorm!")은 cpppj 기준 화면(load.png)처럼 왼쪽 HUD(85px)를 뺀 지도 영역의 가운데에 놓인다: 창 왼쪽 x 가 85 + (939 - 폭) / 2 이다. (2026-11-21 WB04-E1130)
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.content), null, { timeout: 15000 });
  const placed = await page.evaluate(() => { const dialog = window.netstorm._mainMenu.dialog; return { x: dialog.x, width: dialog.baseWidth }; });
  expect(Math.abs(placed.x - (85 + (939 - placed.width) / 2))).toBeLessThan(1);
  // 시작 안내(`startingProgress`)는 영역 {85,0,W,H} 이고, 로그인 진행 문구(`progress`)는 그 시점의 Gump 지도 영역(HUD 도킹 전 전체 화면)이다. 어느 쪽이든 창은 자기 영역 가운데에 있어야 하고 시작 안내는 한 번 이상 있어야 한다.
  const loading = await page.evaluate(() => window.__loadingShown);
  expect(loading.some((entry) => entry.left === 85)).toBe(true);
  for (const entry of loading) expect(Math.abs(entry.x - (entry.left + (entry.right - entry.left - entry.width) / 2))).toBeLessThan(1);
  const web = (await page.evaluate(() => window.netstorm.sessionDump())).replaceAll('\r\n', '\n');
  const reference = fs.readFileSync(path.join(__dirname, '..', 'tests', 'fixtures', 'cpppj', 'candidate-225553df', 'tutorial1', 'oracle', 'tutorial1', 'load.tsv'), 'utf8').replaceAll('\r\n', '\n');
  // 행 단위: 허용한 행 밖의 차이·누락·추가가 없다.
  const { differences } = compareSessionDumps(reference, web);
  const allowedRows = new Set(['frame', 'random', 'pool', 'sp']);
  const unexpected = differences.filter((item) => !(allowedRows.has(item.key) || ['regular:', 'slot:', 'object:', 'player:'].some((prefix) => item.key.startsWith(prefix))));
  expect(unexpected.map((item) => `${item.kind}:${item.key}`)).toEqual([]);
  // SP: 초기화 여부와 소유자 1~8 값은 같고 오프셋(셋째 칸)·소유자 0(넷째 칸)만 다르다.
  const spRow = (text) => text.split('\n').find((line) => line.startsWith('sp\t')).split('\t');
  const referenceSp = spRow(reference), webSp = spRow(web);
  expect(webSp[1]).toBe(referenceSp[1]); expect(webSp.slice(4)).toEqual(referenceSp.slice(4));
  // Player 기록 9개: 앞 4바이트와 Player 1 의 +0x68·+0x69 만 다르다.
  const referencePlayers = rowsOf(reference, 'player'), webPlayers = rowsOf(web, 'player'), playerProblems = [];
  for (const [number, hex] of referencePlayers) for (let index = 0; index < hex.length; index += 2) {
    const offset = index / 2;
    if (hex.slice(index, index + 2) === webPlayers.get(number).slice(index, index + 2) || offset < 4 || (number === 1 && (offset === 104 || offset === 105))) continue;
    playerProblems.push(`player ${number}+${offset}`);
  }
  expect(playerProblems).toEqual([]);
  // 슬롯 원문: 허용 바이트 밖의 차이와 허용 SID 밖의 누락·추가가 없다.
  const expectedSlots = rowsOf(reference, 'slot'), actualSlots = rowsOf(web, 'slot'), ambient = new Set([0, 15, 16, 17]), problems = [];
  for (const [sid, hex] of expectedSlots) {
    const other = actualSlots.get(sid);
    if (ambient.has(sid)) continue;
    if (other === undefined) { problems.push(`누락 ${sid}`); continue; }
    for (let index = 0; index < hex.length; index += 2) {
      const offset = index / 2; if (hex.slice(index, index + 2) === other.slice(index, index + 2)) continue;
      if ((offset >= 36 && offset < 40) || offset === 33 || offset === 31 || offset === 4 || offset === 5 || (sid === 15001 && offset === 42)) continue;
      problems.push(`${sid}+${offset} ${hex.slice(index, index + 2)} → ${other.slice(index, index + 2)}`);
    }
  }
  for (const sid of actualSlots.keys()) if (!expectedSlots.has(sid)) problems.push(`추가 ${sid}`);
  expect(problems).toEqual([]);
  // 기준선과 같은 월드 규모이다(슬롯 수는 앰비언트 객체 몇 개 차이만 허용).
  expect(Math.abs(expectedSlots.size - actualSlots.size)).toBeLessThanOrEqual(3);
  expect(errors).toEqual([]);
});
