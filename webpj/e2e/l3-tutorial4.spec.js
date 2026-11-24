// L3 대조(Tutorial4): cpppj 오라클과 같은 입력 단계표(`tests/fixtures/cpppj/candidate-225553df/tutorial4-l3/steps.json`)를 고정 시각 웹에 적용해 체크포인트의 논리 화면과 Session 덤프를 비교한다.
// 적용·비교 본문은 `e2e/l3-common.js` 이다. Tutorial4 는 TacticalCombat(발사기·제너레이터 생산→대각 중앙 회랑 다리로 적 Sun Cannon 2기 파괴→단계 C~E1 Sun Cannon(131,124)으로 적 발사기 6기 파괴→남쪽 상륙·Wind Gen #3+Thrower 로 적 사제 무력화→골렘 포획·단계 F→본섬 복귀·Altar 건설·단계 G→결박·단계 H→5룬 제물 의식·단계 I→Mission Accomplished! 및 Tutorial5 해제) 전 구간을 L3 대조한다. (2026-11-25 WB04-E1334 추가, 2026-11-25 WB04-E1335 C~E1 확장, 2026-11-25 WB04-E1336 E1~I1 완주 확장)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { oracleFolder, loadOracle, runL3 } = require('./l3-common');

// 오라클 폴더 경로이다. (2026-11-25 WB04-E1334 추가)
const ORACLE = oracleFolder('tutorial4-l3');

// 덤프와 화면 체크포인트 파일 목록이다(a100~i1_18100 단계 A~I1 전 구간). (2026-11-25 WB04-E1334 추가, 2026-11-25 WB04-E1336 E1~I1 완주 갱신)
const EXPECTED_FILES = [
  'a100.tsv', 'a100.png', 'a155.png', 'a210.png', 'a220.png',
  'b290.tsv', 'b290.png', 'b325.png', 'b345.tsv', 'b345.png',
  'b440.tsv', 'b440.png', 'b835.tsv', 'b835.png', 'b1120.tsv', 'b1120.png',
  'b1550.tsv', 'b1550.png', 'b2100.tsv', 'b2100.png', 'b2750.tsv', 'b2750.png',
  'c3500.tsv', 'c3500.png', 'd4755.tsv', 'd4755.png', 'd5400.tsv', 'd5400.png',
  'e7200.tsv', 'e7200.png', 'e7240.tsv', 'e7240.png',
  'e7276.tsv', 'e7276.png', 'e7625.tsv', 'e7625.png', 'f10000.tsv', 'f10000.png',
  'g13500.tsv', 'g13500.png', 'h14000.tsv', 'h14000.png', 'h16000.tsv', 'h16000.png',
  'i18000.tsv', 'i18000.png', 'i1_18100.tsv', 'i1_18100.png',
];
// Session 덤프 대조 라벨 순서이다. (2026-11-25 WB04-E1334 추가, 2026-11-25 WB04-E1336 E1~I1 완주 갱신)
const EXPECTED_DUMPS = [
  'a100', 'b290', 'b345', 'b440', 'b835', 'b1120', 'b1550', 'b2100', 'b2750', 'c3500', 'd4755', 'd5400', 'e7200', 'e7240',
  'e7276', 'e7625', 'f10000', 'g13500', 'h14000', 'h16000', 'i18000', 'i1_18100',
];
// 화면 스냅샷 대조 라벨 순서이다. (2026-11-25 WB04-E1334 추가, 2026-11-25 WB04-E1336 E1~I1 완주 갱신)
const EXPECTED_SNAPSHOTS = [
  'a100', 'a155', 'a210', 'a220', 'b290', 'b325', 'b345', 'b440', 'b835', 'b1120', 'b1550', 'b2100', 'b2750', 'c3500', 'd4755', 'd5400', 'e7200', 'e7240',
  'e7276', 'e7625', 'f10000', 'g13500', 'h14000', 'h16000', 'i18000', 'i1_18100',
];

// Tutorial4 단계 A→E1→I1('Mission Accomplished!') 전 구간의 L3 일치와 Tutorial5 해제를 검증한다. (2026-11-25 WB04-E1334 추가, 2026-11-25 WB04-E1336 E1~I1 완주 확장)
test('L3 Tutorial4: 단계 A→E1→I1(적 사제 포획·제단 건설·제물 의식→Mission Accomplished! 및 Tutorial5 해제) 전 구간의 체크포인트 화면과 덤프가 cpppj 와 같다', async ({ browser }) => {
  test.setTimeout(900000);
  const { plan } = loadOracle(ORACLE);
  expect(plan.mission).toBe('Tutorial4');
  // 단계표 체크포인트마다 오라클 파일이 있는지 확인한다. (2026-11-25 WB04-E1334 추가)
  for (const file of EXPECTED_FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  expect(plan.steps.filter(([, op]) => op === 'session-dump').map(([, , label]) => label)).toEqual(EXPECTED_DUMPS);
  expect(plan.steps.filter(([, op]) => op === 'snapshot').map(([, , label]) => label)).toEqual(EXPECTED_SNAPSHOTS);
  // i1_18100 체크포인트에서 진행 저장(DoneTutorial4=1)과 다음 튜토리얼(Tutorial5) 해제·전환을 함께 확인한다. (2026-11-25 WB04-E1336)
  let progressedToNext = false;
  await runL3({
    browser, plan, oracleDir: ORACLE,
    inspect: async (page, frame) => {
      if (frame !== 18100) return;
      const done = await page.evaluate(() => JSON.parse(localStorage.getItem('netstorm.progress') ?? '{}').values ?? {});
      expect(String(done.DoneTutorial4 ?? done.donetutorial4 ?? '')).toBe('1');
      const nextBtn = await page.evaluate(() => {
        const app = window.netstorm, dlg = app._mainMenu?.dialog;
        const btn = dlg?.buttons?.find((candidate) => candidate.label === 'Next Tutorial');
        if (!btn) return null;
        const point = app._mainMenu.controlPoint(btn.label), rect = app.canvas.getBoundingClientRect();
        return point ? { label: btn.label, x: rect.left + point.x * rect.width / app._layout.logicalWidth, y: rect.top + point.y * rect.height / app._layout.logicalHeight } : null;
      });
      expect(nextBtn?.label).toBe('Next Tutorial');
      await page.mouse.click(nextBtn.x, nextBtn.y, { delay: 60 });
      // Next Tutorial 클릭 뒤 Tutorial5 월드·단계 A 안내 창이 열릴 때까지 프레임을 진행한다. (2026-11-25 WB04-E1336)
      for (let tick = 0; tick < 120; tick += 1) {
        const ready = await page.evaluate(() => window.netstorm._lastCampaignMission?.name === 'Tutorial5' && Boolean(window.netstorm._campaignWorld) && Boolean(window.netstorm._clientGuide?.current()));
        if (ready) { progressedToNext = true; break; }
        await page.evaluate(() => window.netstorm.stepFixedFrames(1));
      }
      expect(await page.evaluate(() => window.netstorm._lastCampaignMission?.name)).toBe('Tutorial5');
    },
  });
  expect(progressedToNext).toBe(true);
});
