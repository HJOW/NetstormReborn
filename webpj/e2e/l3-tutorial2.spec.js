// L3 대조(Tutorial2): cpppj 오라클과 같은 입력 단계표(`tests/fixtures/cpppj/candidate-225553df/tutorial2-l3/steps.json`)를 고정 시각 웹에 적용해 체크포인트의 논리 화면과 Session 덤프를 비교한다.
// 적용·비교 본문은 Tutorial1 과 같은 `e2e/l3-common.js` 이다(입력 프레임 규칙도 같다). Tutorial2 는 건설·생산(신전·워크샾·발사기)을 다룬다. (2026-11-24 WB04-E1305)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { oracleFolder, loadOracle, runL3 } = require('./l3-common');

// 오라클 폴더이다.
const ORACLE = oracleFolder('tutorial2-l3');

test('L3 Tutorial2: Buildings 안내 창→사제 우클릭 메뉴(Construct→Temple→Build Wind Temple)→신전 놓기→건설→신전 완공 안내 창의 체크포인트 화면과 덤프가 cpppj 와 같다', async ({ browser }) => {
  test.setTimeout(900000);
  const { plan } = loadOracle(ORACLE);
  expect(plan.mission).toBe('Tutorial2');
  // 단계표의 체크포인트 이름마다 오라클의 덤프·화면 파일이 있어야 한다(덤프 c400·c1200, 화면 m190·m210·m230·g270·c400·c1200).
  for (const file of ['m190.png', 'm210.png', 'm230.png', 'g270.png', 'c400.tsv', 'c400.png', 'c1200.tsv', 'c1200.png']) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  expect(plan.steps.filter(([, op]) => op === 'session-dump').map(([, , label]) => label)).toEqual(['c400', 'c1200']);
  expect(plan.steps.filter(([, op]) => op === 'snapshot').map(([, , label]) => label)).toEqual(['m190', 'm210', 'm230', 'g270', 'c400', 'c1200']);
  await runL3({ browser, plan, oracleDir: ORACLE });
});
