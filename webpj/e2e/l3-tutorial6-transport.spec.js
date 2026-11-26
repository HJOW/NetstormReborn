// Tutorial6 B에서 에어 쉽의 실제 수확·귀환·납품으로 C 안내까지 이어지는 입력을 대조한다.
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { loadOracle, oracleFolder, runL3 } = require('./l3-common');

// 결과·SP·경로를 주입하지 않은 실제 운반 입력의 독립 오라클이다. (WB04-E1344)
const ORACLE = oracleFolder('tutorial6-transport-l3');
// 실제 비교 자료와 출처 파일을 명시한다.
const FILES = ['steps.json', 'oracle.json', 'airship3240.png', 'airship3240.tsv', 'flight3400.png', 'delivery4300.png', 'delivery4300.tsv'];

// 선택·간헐천 클릭·대기만으로 200 SP가 납품되고 C 안내가 열려야 한다.
test('L3 Tutorial6: 에어 쉽의 실제 자원 납품과 C 안내가 cpppj와 같다', async ({ browser }) => {
  test.setTimeout(1200000);
  const { plan } = loadOracle(ORACLE);
  // 일부 체크포인트가 빠진 실행을 완료로 보지 않도록 파일을 먼저 확인한다.
  for (const file of FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  await runL3({ browser, plan, oracleDir: ORACLE,
    inspect: async (page, frame) => {
      if (frame !== 3240 && frame !== 4300) return;
      // 저장소의 float 비트 대신 공개 SP 조회로 실제 금액을 읽는다.
      const state = await page.evaluate(() => ({ stage: window.netstorm._clientGuide.current().runtime.gate.stage, sp: window.netstorm._campaignWorld.session().sp(1) }));
      expect(state).toEqual(frame === 3240 ? { stage: 0x42, sp: 4500 } : { stage: 0x43, sp: 4700 });
    },
  });
});
