// L3 대조(Tutorial3): cpppj 오라클과 같은 입력 단계표(`tests/fixtures/cpppj/candidate-225553df/tutorial3-l3/steps.json`)를 고정 시각 웹에 적용해 체크포인트의 논리 화면과 Session 덤프를 비교한다.
// 적용·비교 본문은 `e2e/l3-common.js` 이다(Tutorial1·2 와 같은 입력 프레임 규칙). Tutorial3 은 Capture The Priest(골렘·다리로 적 섬 잇기·발사기·제너레이터·적 사제 포획)이다. (2026-11-24 WB04-E1309)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { oracleFolder, loadOracle, runL3 } = require('./l3-common');

// 오라클 폴더이다.
const ORACLE = oracleFolder('tutorial3-l3');

test('L3 Tutorial3: Capture The Priest 안내 창→Build Golems 안내 창의 체크포인트 화면과 덤프가 cpppj 와 같다', async ({ browser }) => {
  test.setTimeout(900000);
  const { plan } = loadOracle(ORACLE);
  expect(plan.mission).toBe('Tutorial3');
  // 단계표의 체크포인트 이름마다 오라클의 덤프·화면 파일이 있어야 한다(덤프 a100·a240, 화면 a100·a160·a240).
  for (const file of ['a100.tsv', 'a100.png', 'a160.png', 'a240.tsv', 'a240.png']) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  expect(plan.steps.filter(([, op]) => op === 'session-dump').map(([, , label]) => label)).toEqual(['a100', 'a240']);
  expect(plan.steps.filter(([, op]) => op === 'snapshot').map(([, , label]) => label)).toEqual(['a100', 'a160', 'a240']);
  await runL3({ browser, plan, oracleDir: ORACLE });
});
