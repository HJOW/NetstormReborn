// Tutorial5 단계 A에서 안내·스톤 타워 생산 등록·배치·대기를 같은 프레임의 cpppj 오라클과 대조한다.
// steps.json·oracle.json은 225553df 기준선 자료이며 피격 관문 이후·특정 적 공격·포획·희생·승리는 실행하지 않는다. (WB04-E1339)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { loadOracle, oracleFolder, runL3 } = require('./l3-common');

// 이 단위에서 새로 수집한 단계 A 오라클의 폴더와 검사 파일 목록이다.
const ORACLE = oracleFolder('tutorial5-l3');
const FILES = ['a100.png', 'a100.tsv', 'a140.png', 'a200.png', 'a200.tsv', 'production295.png', 'production295.tsv', 'cursor305.png', 'tower500.png', 'tower500.tsv', 'a1000.png', 'a1000.tsv'];

// 실제 메뉴 입력으로 생산을 등록하고 스톤 타워가 완공된 상태까지 검사한다.
test('L3 Tutorial5: 단계 A의 스톤 타워 생산·배치·대기 화면과 덤프가 cpppj와 같다', async ({ browser }) => {
  test.setTimeout(600000);
  const { plan } = loadOracle(ORACLE);
  expect(plan.mission).toBe('Tutorial5');
  // 선언한 체크포인트 자료가 빠지지 않았는지 먼저 확인한다.
  for (const file of FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  const { results } = await runL3({
    browser, plan, oracleDir: ORACLE,
    inspect: async (page, frame) => {
      if (frame !== 1000) return;
      // 이번 단계표는 피격으로 B로 전환되기 전의 A 상태까지가 완료 범위다.
      expect(await page.evaluate(() => window.netstorm._clientGuide.current().runtime.gate.stage)).toBe(0x41);
    },
  });
  // 커서용 미완성 객체가 아니라 같은 SID의 실제 소유자1 스톤 타워가 있어야 한다.
  const rows = results['session-dump:a1000'].split(/\r?\n/).map((line) => line.split('\t'));
  const tower = rows.find((row) => row[0] === 'object' && row[1] === '15060');
  expect(tower?.slice(2, 5)).toEqual(['78', '0', '1']);
  expect(tower?.[8]).toBe('2000');
  expect(rows.find((row) => row[0] === 'totalmade' && row[1] === '78')?.slice(2)).toEqual(['1', '1', '5']);
});
