// L3 대조(Tutorial2): cpppj 오라클과 같은 입력 단계표(`tests/fixtures/cpppj/candidate-225553df/tutorial2-l3/steps.json`)를 고정 시각 웹에 적용해 체크포인트의 논리 화면과 Session 덤프를 비교한다.
// 적용·비교 본문은 Tutorial1 과 같은 `e2e/l3-common.js` 이다(입력 프레임 규칙도 같다). Tutorial2 는 건설·생산(신전·워크샾·발사기)을 다룬다. (2026-11-24 WB04-E1305)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { oracleFolder, loadOracle, runL3 } = require('./l3-common');

// 오라클 폴더이다.
const ORACLE = oracleFolder('tutorial2-l3');

test('L3 Tutorial2: Buildings 안내 창→사제 우클릭 메뉴(Construct→Temple→Build Wind Temple)→신전 놓기→건설→신전 완공 안내 창→워크샾 건설(Construct→Workshop→Build Sun Workshop)→Production 안내 창→워크샾 지식 생산 투입→발사기 집어 놓기→Weapon Displays 안내 창→두 번째 발사기·Energy 안내 창→신전 에너지 범위 표시→Energy Part 3 안내 창→세·네 번째 발사기→Energy Part 4·Salvaging 안내 창→발사기 회수→미션 완료 창의 체크포인트 화면과 덤프가 cpppj 와 같다', async ({ browser }) => {
  test.setTimeout(900000);
  const { plan } = loadOracle(ORACLE);
  expect(plan.mission).toBe('Tutorial2');
  // 단계표의 체크포인트 이름마다 오라클의 덤프·화면 파일이 있어야 한다(덤프 c400·c1200·c1520·c2600·c2800·c3400·c3700·c4200·c4800·c4900·c5100·c5400, 화면 m190·m210·m230·g270·c400·c1200·w1360·w1460·c1520·c2600·k2710·k2730·c2800·t3000·c3400·e3500·p3580·c3700·e3960·c4200·p4340·p4400·c4800·c4900·s5000·c5100·c5400).
  for (const file of ['m190.png', 'm210.png', 'm230.png', 'g270.png', 'c400.tsv', 'c400.png', 'c1200.tsv', 'c1200.png', 'w1360.png', 'w1460.png', 'c1520.tsv', 'c1520.png', 'c2600.tsv', 'c2600.png', 'k2710.png', 'k2730.png', 'c2800.tsv', 'c2800.png', 't3000.png', 'c3400.tsv', 'c3400.png', 'e3500.png', 'p3580.png', 'c3700.tsv', 'c3700.png', 'e3960.png', 'c4200.tsv', 'c4200.png', 'p4340.png', 'p4400.png', 'c4800.tsv', 'c4800.png', 'c4900.tsv', 'c4900.png', 's5000.png', 'c5100.tsv', 'c5100.png', 'c5400.tsv', 'c5400.png']) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  expect(plan.steps.filter(([, op]) => op === 'session-dump').map(([, , label]) => label)).toEqual(['c400', 'c1200', 'c1520', 'c2600', 'c2800', 'c3400', 'c3700', 'c4200', 'c4800', 'c4900', 'c5100', 'c5400']);
  expect(plan.steps.filter(([, op]) => op === 'snapshot').map(([, , label]) => label)).toEqual(['m190', 'm210', 'm230', 'g270', 'c400', 'c1200', 'w1360', 'w1460', 'c1520', 'c2600', 'k2710', 'k2730', 'c2800', 't3000', 'c3400', 'e3500', 'p3580', 'c3700', 'e3960', 'c4200', 'p4340', 'p4400', 'c4800', 'c4900', 's5000', 'c5100', 'c5400']);
  await runL3({ browser, plan, oracleDir: ORACLE });
});
