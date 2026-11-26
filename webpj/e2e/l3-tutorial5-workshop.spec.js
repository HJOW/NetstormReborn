// Tutorial5 B의 추가 워크샾 건설과 실제 생산 등록을 C 안내까지 원본 기준선과 대조한다.
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { loadOracle, oracleFolder, runL3 } = require('./l3-common');

// 실제 사제 건설·생산 등록 단계표와 출처를 보존한 독립 오라클 폴더다. (WB04-E1341)
const ORACLE = oracleFolder('tutorial5-workshop-l3');
// manifest에 등록한 화면·덤프의 존재와 출처 파일을 검사한다.
const FILES = ['steps.json', 'oracle.json', 'priestmenu.png', 'workshopmenu.png', 'workshopcursor.png', 'building1600.png', 'built2180.tsv', 'knowledge2.png', 'oldknowledge.png', 'c2400.png', 'c2400.tsv'];

// 실제 추가 워크샾의 완공·SP 차감·기술 등록 뒤 안내 창을 검사한다.
test('L3 Tutorial5: 추가 워크샾과 세 기술 등록으로 C 안내까지 cpppj와 같다', async ({ browser }) => {
  test.setTimeout(900000);
  const { plan } = loadOracle(ORACLE);
  // 등록된 각 자료를 같은 폴더에서 읽어 누락된 체크포인트를 거부한다.
  for (const file of FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  const { results } = await runL3({ browser, plan, oracleDir: ORACLE,
    // E1336에서 복구한 생산 슬롯의 마나 그림으로 달라진 메뉴 영역만 정확한 픽셀 수로 검사한다.
    screenRegions: {
      knowledge2: [{ rect: [643, 367, 850, 488], pixels: 6215, reason: 'E1336 생산 슬롯 마나 그림 보충과 메뉴 폭·높이 변경' }],
      oldknowledge: [{ rect: [738, 244, 945, 348], pixels: 5640, reason: 'E1336 생산 슬롯 마나 그림 보충과 메뉴 폭·높이 변경' }],
    },
    inspect: async (page, frame) => {
      if (frame === 2400) expect(await page.evaluate(() => window.netstorm._clientGuide.current().runtime.gate.stage)).toBe(0x43);
    },
  });
  // 초기 워크샾과 별개로 같은 소유자의 새 워크샾이 살아 있어야 한다.
  const rows = results['session-dump:c2400'].split(/\r?\n/).map((line) => line.split('\t'));
  const workshops = rows.filter((row) => row[0] === 'object' && row[2] === '85' && row[4] === '1' && Number(row[8]) > 0);
  expect(workshops).toHaveLength(2);
});
