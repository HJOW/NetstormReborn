// Tutorial6 A에서 실제 워크샾·제네레이터·에어 쉽을 건설하고 B 안내까지 기준선과 대조한다.
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { loadOracle, oracleFolder, runL3 } = require('./l3-common');

// 공격 목표를 시작하지 않고 생산·건설 입력만 보존한 기준선 폴더다. (WB04-E1343)
const ORACLE = oracleFolder('tutorial6-build-l3');
// 출처와 실제 비교하는 화면·덤프를 목록으로 확인한다.
const FILES = ['steps.json', 'oracle.json', 'a100.png', 'energy180.png', 'workshopcursor.png', 'workshop1600.png', 'workshop1600.tsv', 'production1730.png', 'generators2400.png', 'generators2400.tsv', 'airshipcursor.png', 'b3200.png', 'b3200.tsv'];

// 에너지 공급원 둘을 완공하고 에어 쉽을 실제로 만든 뒤 B 관문을 확인한다.
test('L3 Tutorial6: 윈드 워크샾과 제네레이터 둘·에어 쉽 생산이 cpppj와 같다', async ({ browser }) => {
  test.setTimeout(900000);
  const { plan } = loadOracle(ORACLE);
  // 파일 누락 때문에 필요한 비교 구간이 빠지지 않게 먼저 검사한다.
  for (const file of FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  const { results } = await runL3({ browser, plan, oracleDir: ORACLE,
    // E1336의 마나 그림 복구·별칭 교정으로 에너지 안내 창만 다시 배치된다. 지도는 계속 0픽셀이어야 한다.
    screenRegions: { energy180: [{ rect: [254, 217, 857, 552], pixels: 26846, reason: 'E1336 마나 그림·imana 별칭 보충에 따른 에너지 안내 그림과 줄 나눔 변경' }] },
    inspect: async (page, frame) => {
      if (frame === 3200) expect(await page.evaluate(() => window.netstorm._clientGuide.current().runtime.gate.stage)).toBe(0x42);
    },
  });
  // 커서 미리보기와 구별해 살아 있는 소유자1의 완공 객체를 확인한다.
  const rows = results['session-dump:b3200'].split(/\r?\n/).map((line) => line.split('\t'));
  const live = rows.filter((row) => row[0] === 'object' && row[4] === '1' && Number(row[8]) > 0);
  expect(live.filter((row) => row[2] === '86')).toHaveLength(1);
  expect(live.filter((row) => row[2] === '75')).toHaveLength(2);
  expect(live.filter((row) => row[2] === '141')).toHaveLength(1);
});
