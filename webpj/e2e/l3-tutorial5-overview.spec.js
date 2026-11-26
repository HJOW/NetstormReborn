// Tutorial5 C의 안내·미니맵 이동·훨리베이스 선택·대기를 D 안내 진입까지만 대조한다.
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { loadOracle, oracleFolder, runL3 } = require('./l3-common');

// 특정 적을 공격하는 D 단계 입력은 포함하지 않은 별도 기준선 폴더다. (WB04-E1342)
const ORACLE = oracleFolder('tutorial5-overview-l3');
// 같은 단계표의 출처와 모든 화면·덤프를 명시한다.
const FILES = ['steps.json', 'oracle.json', 'c2400.png', 'airdefense2440.png', 'enemyview.png', 'aviaryselected.png', 'aviaryselected.tsv', 'd2800.png', 'd2800.tsv'];

// 읽기 전용 선택과 대기만으로 D 안내가 열리고 더 진행하지 않는지 확인한다.
test('L3 Tutorial5: C 안내와 훨리베이스 선택 뒤 D 안내까지 cpppj와 같다', async ({ browser }) => {
  test.setTimeout(900000);
  const { plan } = loadOracle(ORACLE);
  // 누락된 비교 자료로 허용 범위를 잘못 판정하지 않도록 먼저 확인한다.
  for (const file of FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  await runL3({ browser, plan, oracleDir: ORACLE,
    inspect: async (page, frame) => {
      if (frame === 2800) expect(await page.evaluate(() => window.netstorm._clientGuide.current().runtime.gate.stage)).toBe(0x44);
    },
  });
});
