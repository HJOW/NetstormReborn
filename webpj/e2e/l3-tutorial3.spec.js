// L3 대조(Tutorial3): cpppj 오라클과 같은 입력 단계표(`tests/fixtures/cpppj/candidate-225553df/tutorial3-l3/steps.json`)를 고정 시각 웹에 적용해 체크포인트의 논리 화면과 Session 덤프를 비교한다.
// 적용·비교 본문은 `e2e/l3-common.js` 이다(Tutorial1·2 와 같은 입력 프레임 규칙). Tutorial3 은 Capture The Priest(골렘·다리로 간헐천·적 섬 잇기·발사기·제너레이터·적 사제 포획)이며, 단계 A(북서쪽 간헐천 직결·골렘 배치·수확·신전 배달)부터 단계 B(Capturing Enemy Priests Part 1·2 안내 창 및 자유 진행)까지 대조한다. (2026-11-24 WB04-E1309, WB04-E1329 확장)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { oracleFolder, loadOracle, runL3 } = require('./l3-common');

// 오라클 폴더이다.
const ORACLE = oracleFolder('tutorial3-l3');

// 덤프와 화면 체크포인트 목록이다(a100·a160·a240 도입 + b340 다리 직결 + b900 골렘 완성 + b1200 크리스털 운반 + b1600·b1640·b1700 단계 B 진입 + b1950 남쪽 다리 연결 + c2160·c2200 적 섬 직결 및 단계 C 진입). (2026-11-24 WB04-E1330)
const EXPECTED_FILES = [
  'a100.tsv', 'a100.png', 'a160.png', 'a240.tsv', 'a240.png',
  'b340.tsv', 'b340.png', 'b900.tsv', 'b900.png', 'b1200.tsv', 'b1200.png',
  'b1600.tsv', 'b1600.png', 'b1640.png', 'b1700.tsv', 'b1700.png',
  'b1950.tsv', 'b1950.png', 'c2160.tsv', 'c2160.png', 'c2200.tsv', 'c2200.png',
];
const EXPECTED_DUMPS = ['a100', 'a240', 'b340', 'b900', 'b1200', 'b1600', 'b1700', 'b1950', 'c2160', 'c2200'];
const EXPECTED_SNAPSHOTS = ['a100', 'a160', 'a240', 'b340', 'b900', 'b1200', 'b1600', 'b1640', 'b1700', 'b1950', 'c2160', 'c2200'];

test('L3 Tutorial3: 단계 A(간헐천 다리 직결·골렘 생산·크리스털 수확)→단계 B(남쪽 적 섬 다리 직결)→단계 C(Build Sun Disc Throwers)의 체크포인트 화면과 덤프가 cpppj 와 같다', async ({ browser }) => {
  test.setTimeout(900000);
  const { plan } = loadOracle(ORACLE);
  expect(plan.mission).toBe('Tutorial3');
  // 단계표의 체크포인트 이름마다 오라클의 덤프·화면 파일이 있어야 한다.
  for (const file of EXPECTED_FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  expect(plan.steps.filter(([, op]) => op === 'session-dump').map(([, , label]) => label)).toEqual(EXPECTED_DUMPS);
  expect(plan.steps.filter(([, op]) => op === 'snapshot').map(([, , label]) => label)).toEqual(EXPECTED_SNAPSHOTS);
  await runL3({ browser, plan, oracleDir: ORACLE });
});
