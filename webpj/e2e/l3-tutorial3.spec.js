// L3 대조(Tutorial3): cpppj 오라클과 같은 입력 단계표(`tests/fixtures/cpppj/candidate-225553df/tutorial3-l3/steps.json`)를 고정 시각 웹에 적용해 체크포인트의 논리 화면과 Session 덤프를 비교한다.
// 적용·비교 본문은 `e2e/l3-common.js` 이다(Tutorial1·2 와 같은 입력 프레임 규칙). Tutorial3 은 Capture The Priest(골렘·다리로 간헐천·적 섬 잇기·발사기·제너레이터·적 사제 포획)이며, 단계 A(북서쪽 간헐천 직결·골렘 배치·수확·신전 배달)부터 단계 B(Capturing Enemy Priests Part 1·2 안내 창 및 자유 진행)까지 대조한다. (2026-11-24 WB04-E1309, WB04-E1329 확장)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { oracleFolder, loadOracle, runL3 } = require('./l3-common');

// 오라클 폴더이다.
const ORACLE = oracleFolder('tutorial3-l3');

// 덤프와 화면 체크포인트 목록이다(a100~d2360 + d2384·e2435·e2460 Wind Generator 생산·단계 E + e2510·e3000·e3240 에너지 확장·적 섬 발사기 배치 + f3420·f3460 단계 F 진입). (2026-11-24 WB04-E1332)
const EXPECTED_FILES = [
  'a100.tsv', 'a100.png', 'a160.png', 'a240.tsv', 'a240.png',
  'b340.tsv', 'b340.png', 'b900.tsv', 'b900.png', 'b1200.tsv', 'b1200.png',
  'b1600.tsv', 'b1600.png', 'b1640.png', 'b1700.tsv', 'b1700.png',
  'b1950.tsv', 'b1950.png', 'c2160.tsv', 'c2160.png', 'c2200.tsv', 'c2200.png',
  'c2214.png', 'c2224.png', 'd2280.tsv', 'd2280.png', 'd2320.png', 'd2360.tsv', 'd2360.png',
  'd2384.png', 'e2435.tsv', 'e2435.png', 'e2460.png', 'e2510.tsv', 'e2510.png',
  'e3000.tsv', 'e3000.png', 'e3240.png', 'f3420.tsv', 'f3420.png', 'f3460.tsv', 'f3460.png',
];
const EXPECTED_DUMPS = [
  'a100', 'a240', 'b340', 'b900', 'b1200', 'b1600', 'b1700', 'b1950', 'c2160', 'c2200',
  'd2280', 'd2360', 'e2435', 'e2510', 'e3000', 'f3420', 'f3460',
];
const EXPECTED_SNAPSHOTS = [
  'a100', 'a160', 'a240', 'b340', 'b900', 'b1200', 'b1600', 'b1640', 'b1700', 'b1950', 'c2160', 'c2200',
  'c2214', 'c2224', 'd2280', 'd2320', 'd2360', 'd2384', 'e2435', 'e2460', 'e2510', 'e3000', 'e3240', 'f3420', 'f3460',
];

test('L3 Tutorial3: 단계 A→B→C→D~E(Wind Generator·적 섬 에너지 확장·발사기 배치)→F(Incapacitating the Enemy Priest)의 체크포인트 화면과 덤프가 cpppj 와 같다', async ({ browser }) => {
  test.setTimeout(900000);
  const { plan } = loadOracle(ORACLE);
  expect(plan.mission).toBe('Tutorial3');
  // 단계표의 체크포인트 이름마다 오라클의 덤프·화면 파일이 있어야 한다.
  for (const file of EXPECTED_FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  expect(plan.steps.filter(([, op]) => op === 'session-dump').map(([, , label]) => label)).toEqual(EXPECTED_DUMPS);
  expect(plan.steps.filter(([, op]) => op === 'snapshot').map(([, , label]) => label)).toEqual(EXPECTED_SNAPSHOTS);
  await runL3({ browser, plan, oracleDir: ORACLE });
});
