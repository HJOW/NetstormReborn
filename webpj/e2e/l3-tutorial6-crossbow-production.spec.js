// Tutorial6 C의 실제 워크샾 업그레이드·크로스보우 생산 등록을 독립 오라클과 대조한다.
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { loadOracle, oracleFolder, runL3 } = require('./l3-common');

// C 안내부터 D 안내까지 공격 없이 진행한 원본 입력과 관측 자료다. (WB04-E1345)
const ORACLE = oracleFolder('tutorial6-crossbow-production-l3');
// 출처·화면·덤프를 빠짐없이 소비하도록 필요한 파일을 열거한다.
const FILES = ['steps.json', 'oracle.json', 'c4300.png', 'c4300.tsv', 'workshop4370.png', 'upgrade4390.png', 'upgraded5000.png', 'upgraded5000.tsv', 'knowledge5530.png', 'd5600.png', 'd5600.tsv'];

// 실제 업그레이드 비용을 지불하고 생산 메뉴에 크로스보우를 등록하면 D 안내가 열려야 한다.
test('L3 Tutorial6: 워크샾 업그레이드와 크로스보우 생산 등록이 cpppj와 같다', async ({ browser }) => {
  test.setTimeout(1500000);
  const { plan } = loadOracle(ORACLE);
  // 누락된 비교 자료를 조용히 건너뛰지 않게 사전에 확인한다.
  for (const file of FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  await runL3({ browser, plan, oracleDir: ORACLE,
    // E390의 명시 인접 DWORD=1과 C++ OS 힙 인접값=0이 갈리는 네 입자만 구별한다.
    screenRegions: { upgrade4390: [
      { rect: [302, 386, 303, 387], pixels: 1, reason: 'E390 건설 색 표 끝의 웹 명시 인접 DWORD1·C++ OS 힙0' },
      { rect: [307, 421, 308, 422], pixels: 1, reason: 'E390 건설 색 표 끝의 웹 명시 인접 DWORD1·C++ OS 힙0' },
      { rect: [287, 443, 288, 444], pixels: 1, reason: 'E390 건설 색 표 끝의 웹 명시 인접 DWORD1·C++ OS 힙0' },
      { rect: [388, 444, 389, 445], pixels: 1, reason: 'E390 건설 색 표 끝의 웹 명시 인접 DWORD1·C++ OS 힙0' },
    ],
      // E1336의 마나 그림 보충으로 생산 슬롯 그림과 팝업 폭이 달라지는 범위다.
      knowledge5530: [{ rect: [548, 347, 755, 451], pixels: 5632, reason: 'E1336 마나 그림 보충에 따른 생산 슬롯 그림·팝업 폭 변경' }],
    },
    inspect: async (page, frame) => {
      // 조회만으로 실제 게임 관문의 C→D 전환을 확인한다.
      if (frame === 4300 || frame === 5600) expect(await page.evaluate(() => window.netstorm._clientGuide.current().runtime.gate.stage)).toBe(frame === 4300 ? 0x43 : 0x44);
      if (frame === 4390) expect(await page.evaluate(() => [[302, 386], [307, 421], [287, 443], [388, 444]].map(([x, y]) => window.netstorm._surface.pixels[y * 1024 + x]))).toEqual([1, 1, 1, 1]);
    },
  });
});
