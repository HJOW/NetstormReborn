// L3 대조(Tutorial4): cpppj 오라클과 같은 입력 단계표(`tests/fixtures/cpppj/candidate-225553df/tutorial4-l3/steps.json`)를 고정 시각 웹에 적용해 체크포인트의 논리 화면과 Session 덤프를 비교한다.
// 적용·비교 본문은 `e2e/l3-common.js` 이다. Tutorial4 는 TacticalCombat(발사기·제너레이터 생산→대각 중앙 회랑 다리로 적 Sun Cannon 2기 파괴→단계 C 'Upgrading Workshops, Part 1'→Sun Workshop Level II 업그레이드·Sun Cannon(131,124)으로 적 발사기 6기 파괴→단계 E1 'Sacrificing Priests, Part 1') 구간을 L3 대조한다. (2026-11-25 WB04-E1334 추가, 2026-11-25 WB04-E1335 단계 C~E1 확장)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { oracleFolder, loadOracle, runL3 } = require('./l3-common');

// 오라클 폴더 경로이다. (2026-11-25 WB04-E1334 추가)
const ORACLE = oracleFolder('tutorial4-l3');

// 덤프와 화면 체크포인트 파일 목록이다(a100~e7240 단계 E1 진입 안내 창). (2026-11-25 WB04-E1334 추가, 2026-11-25 WB04-E1335 갱신)
const EXPECTED_FILES = [
  'a100.tsv', 'a100.png', 'a155.png', 'a210.png', 'a220.png',
  'b290.tsv', 'b290.png', 'b325.png', 'b345.tsv', 'b345.png',
  'b440.tsv', 'b440.png', 'b835.tsv', 'b835.png', 'b1120.tsv', 'b1120.png',
  'b1550.tsv', 'b1550.png', 'b2100.tsv', 'b2100.png', 'b2750.tsv', 'b2750.png',
  'c3500.tsv', 'c3500.png', 'd4755.tsv', 'd4755.png', 'd5400.tsv', 'd5400.png',
  'e7200.tsv', 'e7200.png', 'e7240.tsv', 'e7240.png',
];
// Session 덤프 대조 라벨 순서이다. (2026-11-25 WB04-E1334 추가, 2026-11-25 WB04-E1335 갱신)
const EXPECTED_DUMPS = [
  'a100', 'b290', 'b345', 'b440', 'b835', 'b1120', 'b1550', 'b2100', 'b2750', 'c3500', 'd4755', 'd5400', 'e7200', 'e7240',
];
// 화면 스냅샷 대조 라벨 순서이다. (2026-11-25 WB04-E1334 추가, 2026-11-25 WB04-E1335 갱신)
const EXPECTED_SNAPSHOTS = [
  'a100', 'a155', 'a210', 'a220', 'b290', 'b325', 'b345', 'b440', 'b835', 'b1120', 'b1550', 'b2100', 'b2750', 'c3500', 'd4755', 'd5400', 'e7200', 'e7240',
];

// Tutorial4 단계 A→C→E1('Sacrificing Priests, Part 1')까지의 L3 일치 여부를 검증한다. (2026-11-25 WB04-E1334 추가, 2026-11-25 WB04-E1335 갱신)
test('L3 Tutorial4: 단계 A→E1(Sun Disc Thrower 2기·워크샾 업그레이드·Sun Cannon으로 적 발사기 파괴→단계 E1 안내 창)의 체크포인트 화면과 덤프가 cpppj 와 같다', async ({ browser }) => {
  test.setTimeout(900000);
  const { plan } = loadOracle(ORACLE);
  expect(plan.mission).toBe('Tutorial4');
  // 단계표 체크포인트마다 오라클 파일이 있는지 확인한다. (2026-11-25 WB04-E1334 추가)
  for (const file of EXPECTED_FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  expect(plan.steps.filter(([, op]) => op === 'session-dump').map(([, , label]) => label)).toEqual(EXPECTED_DUMPS);
  expect(plan.steps.filter(([, op]) => op === 'snapshot').map(([, , label]) => label)).toEqual(EXPECTED_SNAPSHOTS);
  await runL3({ browser, plan, oracleDir: ORACLE });
});
