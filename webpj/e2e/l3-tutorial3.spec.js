// L3 대조(Tutorial3): cpppj 오라클과 같은 입력 단계표(`tests/fixtures/cpppj/candidate-225553df/tutorial3-l3/steps.json`)를 고정 시각 웹에 적용해 체크포인트의 논리 화면과 Session 덤프를 비교한다.
// 적용·비교 본문은 `e2e/l3-common.js` 이다(Tutorial1·2 와 같은 입력 프레임 규칙). Tutorial3 은 Capture The Priest(골렘·다리로 간헐천·적 섬 잇기·발사기·제너레이터·적 사제 포획)이며, 단계 A~B~C~D~E~F~G~H(미션 완료 'Mission Accomplished!') 전 구간을 대조한다. (2026-11-24 WB04-E1309~E1333 확장)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { oracleFolder, loadOracle, runL3 } = require('./l3-common');

// 오라클 폴더이다.
const ORACLE = oracleFolder('tutorial3-l3');

// 덤프와 화면 체크포인트 목록이다(a100~f3460 + f3620 두 번째 발사기·g4160 단계 G 안내 창·g4185 Drop Storm Crystal 메뉴·g5120 골렘 다리 이동·g5140 골렘 선택·h6200 미션 완료). (2026-11-24 WB04-E1333)
const EXPECTED_FILES = [
  'a100.tsv', 'a100.png', 'a160.png', 'a240.tsv', 'a240.png',
  'b340.tsv', 'b340.png', 'b900.tsv', 'b900.png', 'b1200.tsv', 'b1200.png',
  'b1600.tsv', 'b1600.png', 'b1640.png', 'b1700.tsv', 'b1700.png',
  'b1950.tsv', 'b1950.png', 'c2160.tsv', 'c2160.png', 'c2200.tsv', 'c2200.png',
  'c2214.png', 'c2224.png', 'd2280.tsv', 'd2280.png', 'd2320.png', 'd2360.tsv', 'd2360.png',
  'd2384.png', 'e2435.tsv', 'e2435.png', 'e2460.png', 'e2510.tsv', 'e2510.png',
  'e3000.tsv', 'e3000.png', 'e3240.png', 'f3420.tsv', 'f3420.png', 'f3460.tsv', 'f3460.png',
  'f3620.png', 'g4160.tsv', 'g4160.png', 'g4185.png', 'g5120.tsv', 'g5120.png', 'g5140.png', 'h6200.tsv', 'h6200.png',
];
const EXPECTED_DUMPS = [
  'a100', 'a240', 'b340', 'b900', 'b1200', 'b1600', 'b1700', 'b1950', 'c2160', 'c2200',
  'd2280', 'd2360', 'e2435', 'e2510', 'e3000', 'f3420', 'f3460',
  'g4160', 'g5120', 'h6200',
];
const EXPECTED_SNAPSHOTS = [
  'a100', 'a160', 'a240', 'b340', 'b900', 'b1200', 'b1600', 'b1640', 'b1700', 'b1950', 'c2160', 'c2200',
  'c2214', 'c2224', 'd2280', 'd2320', 'd2360', 'd2384', 'e2435', 'e2460', 'e2510', 'e3000', 'e3240', 'f3420', 'f3460',
  'f3620', 'g4160', 'g4185', 'g5120', 'g5140', 'h6200',
];

test('L3 Tutorial3: 단계 A→B→C→D~E→F~H(적 사제 무력화·골렘 Drop Storm Crystal·포획·Mission Accomplished! 및 Tutorial4 해제)의 체크포인트 화면과 덤프가 cpppj 와 같다', async ({ browser }) => {
  test.setTimeout(900000);
  const { plan } = loadOracle(ORACLE);
  expect(plan.mission).toBe('Tutorial3');
  // 단계표의 체크포인트 이름마다 오라클의 덤프·화면 파일이 있어야 한다.
  for (const file of EXPECTED_FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  expect(plan.steps.filter(([, op]) => op === 'session-dump').map(([, , label]) => label)).toEqual(EXPECTED_DUMPS);
  expect(plan.steps.filter(([, op]) => op === 'snapshot').map(([, , label]) => label)).toEqual(EXPECTED_SNAPSHOTS);
  // h6200 체크포인트에서 진행 저장(DoneTutorial3=1)과 다음 튜토리얼(Tutorial4) 해제·전환을 함께 확인한다. (2026-11-24 WB04-E1333)
  let progressedToNext = false;
  await runL3({
    browser, plan, oracleDir: ORACLE,
    inspect: async (page, frame) => {
      if (frame !== 6200) return;
      const done = await page.evaluate(() => JSON.parse(localStorage.getItem('netstorm.progress') ?? '{}').values ?? {});
      expect(String(done.DoneTutorial3 ?? done.donetutorial3 ?? '')).toBe('1');
      const nextBtn = await page.evaluate(() => {
        const app = window.netstorm, dlg = app._mainMenu?.dialog, btn = dlg?.buttons?.[1];
        if (!btn) return null;
        const point = app._mainMenu.controlPoint(btn.label), rect = app.canvas.getBoundingClientRect();
        return point ? { label: btn.label, x: rect.left + point.x * rect.width / app._layout.logicalWidth, y: rect.top + point.y * rect.height / app._layout.logicalHeight } : null;
      });
      expect(nextBtn?.label).toBe('Next Tutorial');
      await page.mouse.click(nextBtn.x, nextBtn.y, { delay: 60 });
      for (let tick = 0; tick < 120; tick += 1) {
        const ready = await page.evaluate(() => window.netstorm._lastCampaignMission?.name === 'Tutorial4' && Boolean(window.netstorm._campaignWorld) && Boolean(window.netstorm._clientGuide?.current()));
        if (ready) { progressedToNext = true; break; }
        await page.evaluate(() => window.netstorm.stepFixedFrames(1));
      }
      expect(await page.evaluate(() => window.netstorm._lastCampaignMission?.name)).toBe('Tutorial4');
    },
  });
  expect(progressedToNext).toBe(true);
});
