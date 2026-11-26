// 캠페인1-2의 브리핑·시작 단추·대기 허용 구간을 같은 프레임의 C++ 자료와 대조한다.
const { test, expect } = require('@playwright/test');
const fs = require('node:fs'), path = require('node:path');
const { oracleFolder, loadOracle, runL3 } = require('./l3-common');
// 아군 AI를 켜지 않고 수동 공격·포획·희생·승리를 요구하지 않는1200프레임 자료다.
const ORACLE = oracleFolder('campaign1-2-wait-l3');
// 실제 비교하는 모든 출처·화면·상태 파일이다.
const FILES = ['steps.json', 'oracle.json', 'brief100.png', 'brief100.tsv', 'wait400.png', 'wait400.tsv', 'wait1200.png', 'wait1200.tsv'];

// 독립 C++ 덤프의 소유자1 실제 금액이다. owner0 미초기화 허용과 분리한다.
const EXPECTED_SP = {"400":2000,"1200":2000};

// 브리핑의 실제 Play Mission 단추 뒤 자연 대기만 입력하고 사람이 조작하는 최초 진행을 확인한다.
test('L3 캠페인1-2: 브리핑·시작·대기 화면과 상태가 cpppj와 같다', async ({ browser }) => {
  test.setTimeout(600000);
  const { plan } = loadOracle(ORACLE);
  // 자료 누락으로 비교 지점이 빠지는 것을 막는다.
  for (const file of FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  await runL3({ browser, plan, oracleDir: ORACLE, inspect: async (page, frame) => {
    if (frame !== 400 && frame !== 1200) return;
    // 다른 검사의 아군 AI 설정이 이 독립 미션에 남지 않아야 한다.
    const current = await page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(); return { started: app._clientGuide.current().runtime.gate.started, brain: session.participation().brain(1), sp: session.sp(1) }; });
    expect(current).toEqual({ started: 1, brain: 0, sp: EXPECTED_SP[frame] });
  } });
});
