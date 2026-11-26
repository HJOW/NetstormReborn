// Tutorial5의 실제 스톤 타워 건설·피격으로 B 안내 창이 열리는 과정을 기준선과 대조한다.
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { loadOracle, oracleFolder, runL3 } = require('./l3-common');

// 기존 A1 자료를 보존하기 위해 피격 관문 오라클은 별도 폴더에서 읽는다. (WB04-E1340)
const ORACLE = oracleFolder('tutorial5-defense-l3');
// manifest 소비 검사가 실제로 읽는 파일을 명시하며 단계표와 메타도 같은 폴더에서 사용한다.
const FILES = ['steps.json', 'oracle.json', 'a100.png', 'a100.tsv', 'a140.png', 'a200.png', 'a200.tsv', 'production295.png', 'production295.tsv', 'cursor305.png', 'defense500.png', 'defense500.tsv', 'defense700.png', 'defense700.tsv', 'defense800.png', 'defense800.tsv', 'defense1000.png', 'defense1000.tsv'];

// 피해·단계 주입 없이 실제 메뉴·건설·대기만으로 관문을 통과해야 한다.
test('L3 Tutorial5: 스톤 타워 피격과 B 안내 창의 화면·덤프가 cpppj와 같다', async ({ browser }) => {
  test.setTimeout(600000);
  const { plan } = loadOracle(ORACLE);
  // 명시한 자료 파일이 모두 있어야 같은 출처의 비교라고 판정한다.
  for (const file of FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  const { results } = await runL3({ browser, plan, oracleDir: ORACLE,
    inspect: async (page, frame) => {
      if (frame !== 1000) return;
      expect(await page.evaluate(() => window.netstorm._clientGuide.current().runtime.gate.stage)).toBe(0x42);
    },
  });
  // B 안내로 정지한 시점에 같은 타워가 완공되어 실제 피해를 입었는지 확인한다.
  const rows = results['session-dump:defense1000'].split(/\r?\n/).map((line) => line.split('\t'));
  const tower = rows.find((row) => row[0] === 'object' && row[1] === '15060');
  expect(tower?.slice(2, 5)).toEqual(['78', '0', '1']);
  expect(Number(tower?.[8])).toBeGreaterThan(0);
  expect(Number(tower?.[8])).toBeLessThan(2000);
});
