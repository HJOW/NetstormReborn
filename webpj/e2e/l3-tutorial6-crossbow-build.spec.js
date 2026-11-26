// Tutorial6 D의 크로스보우 건설을 E 안내까지 실제 입력으로 대조한다.
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { loadOracle, oracleFolder, runL3 } = require('./l3-common');

// 특정 적 공격 목표를 시작하기 전까지의 독립 C++ 자료다. (WB04-E1346)
const ORACLE = oracleFolder('tutorial6-crossbow-build-l3');
// 비교 자료와 출처를 모두 소비하기 위한 파일 목록이다.
const FILES = ['steps.json', 'oracle.json', 'd5600.png', 'd5600.tsv', 'sidebar5640.png', 'crossbowcursor5690.png', 'e6100.png', 'e6100.tsv'];

// 생산 격자 선택·우클릭 회전·실제 배치·대기만으로 E 안내와 완공 객체를 확인한다.
test('L3 Tutorial6: 실제 크로스보우 건설과 E 바리케이드 안내가 cpppj와 같다', async ({ browser }) => {
  test.setTimeout(1500000);
  const { plan } = loadOracle(ORACLE);
  // 파일 부재로 검증 구간이 줄지 않도록 먼저 확인한다.
  for (const file of FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  const { results } = await runL3({ browser, plan, oracleDir: ORACLE,
    inspect: async (page, frame) => {
      if (frame === 6100) expect(await page.evaluate(() => window.netstorm._clientGuide.current().runtime.gate.stage)).toBe(0x45);
    },
  });
  // 생산 미리보기와 구별해 실제 소유자1의 크로스보우(TYPE140)가 살아 있어야 한다.
  const rows = results['session-dump:e6100'].split(/\r?\n/).map((line) => line.split('\t'));
  expect(rows.filter((row) => row[0] === 'object' && row[2] === '140' && row[4] === '1' && Number(row[8]) > 0)).toHaveLength(1);
});
