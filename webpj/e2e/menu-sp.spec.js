// 실제 ESC 메뉴의 원본 SP 회전/검산 상자와 화면·Session을 C++ 입력 오라클로 함께 대조한다. (WB04-E1327)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs'), path = require('node:path');
const { oracleFolder, loadOracle, runL3 } = require('./l3-common');
// 같은 Tutorial3의 C++ 실행 자료와 체크포인트별 예상 열림/offset이다.
const ORACLE = oracleFolder('tutorial3-menu-sp');
const POINTS = [{ frame: 240, name: 'before240', open: false, offset: 103 }, { frame: 280, name: 'open280', open: true, offset: 106 }, { frame: 320, name: 'closed320', open: false, offset: 106 }, { frame: 360, name: 'open360', open: true, offset: 109 }];

// 원본 SP 행의 초기화/offset/플레이어1..8을 보존하고 미초기화 owner0 한 칸만 제외한다.
function spRow(dump) {
  const row = dump.split(/\r?\n/).find((line) => line.startsWith('sp\t')).split('\t');
  expect(row.length).toBe(12); return [...row.slice(0, 3), ...row.slice(4)];
}

test('실제 ESC 메뉴: 열 때 SP 비트와 같은 검산값을3 늘리고 닫을 때 유지하며 C++ 화면/상태와 같다', async ({ browser }) => {
  test.setTimeout(300000);
  // 모든 fixture를 실제 L3 대조와 아래 SP 열 비교가 소비한다.
  for (const file of ['steps.json', 'oracle.json', 'before240.tsv', 'open280.png', 'open280.tsv', 'closed320.png', 'closed320.tsv', 'open360.png', 'open360.tsv']) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  const { plan } = loadOracle(ORACLE), observations = [];
  const { results } = await runL3({ browser, plan, oracleDir: ORACLE, inspect: async (page, frame) => {
    const point = POINTS.find((item) => item.frame === frame); if (!point) return;
    const current = await page.evaluate(() => {
      const app = window.netstorm, globals = app._clientSp, store = globals.store(), session = app._campaignWorld.session();
      return { offset: store.offset, tracking: globals.tracking().value, shared: session.spStorage() === store && store.trackingOwner === globals.tracking(),
        open: app._campaignProduction.ui.gameMenu().isOpen(), ownerBits: Array.from({ length: 8 }, (_, index) => store.get(index + 1)) };
    });
    expect(current).toMatchObject({ offset: point.offset, tracking: point.offset, shared: true, open: point.open }); observations.push(current);
  } });
  // 일반 L3의 sp 행 전체 허용보다 좁게 검사한다. offset이나 플레이어 금액 변화는 허용하지 않는다.
  for (const point of POINTS) expect(spRow(results[`session-dump:${point.name}`]), point.name).toEqual(spRow(fs.readFileSync(path.join(ORACLE, `${point.name}.tsv`), 'utf8')));
  for (const current of observations) expect(current.ownerBits).toEqual(observations[0].ownerBits);
});
