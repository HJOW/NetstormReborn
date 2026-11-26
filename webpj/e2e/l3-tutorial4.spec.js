// Tutorial4의 수집 자료를 보존하면서 자동 실행 범위를 생산 등록·안내 창까지 제한한다.
// 특정 적 공격과 사제 포획·희생은 2026-11-26 사용자 지시에 따라 이 스테이지에서 실행하지 않는다. (WB04-E1337)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { oracleFolder, loadOracle, runL3 } = require('./l3-common');

// 기존 steps.json·oracle.json과 오라클을 읽는 폴더다. 보존 자료는 덮어쓰지 않는다.
const ORACLE = oracleFolder('tutorial4-l3');
// 중단본의 포획·희생 자료를 포함한 파일 목록이다. 실행 없이 보존 여부만 확인한다.
const PRESERVED_FILES = [
  'a100.tsv', 'a100.png', 'a155.png', 'a210.png', 'a220.png',
  'b290.tsv', 'b290.png', 'b325.png', 'b345.tsv', 'b345.png',
  'b440.tsv', 'b440.png', 'b835.tsv', 'b835.png', 'b1120.tsv', 'b1120.png',
  'b1550.tsv', 'b1550.png', 'b2100.tsv', 'b2100.png', 'b2750.tsv', 'b2750.png',
  'c3500.tsv', 'c3500.png', 'd4755.tsv', 'd4755.png', 'd5400.tsv', 'd5400.png',
  'e7200.tsv', 'e7200.png', 'e7240.tsv', 'e7240.png',
  'e7276.tsv', 'e7276.png', 'e7625.tsv', 'e7625.png', 'f10000.tsv', 'f10000.png',
  'g13500.tsv', 'g13500.png', 'h14000.tsv', 'h14000.png', 'h16000.tsv', 'h16000.png',
  'i18000.tsv', 'i18000.png', 'i1_18100.tsv', 'i1_18100.png',
];
// 이전 E1335에서 검증한 E1 진입까지의 덤프·화면 라벨이다.
const DUMPS = ['a100', 'b290', 'b345', 'b440', 'b835', 'b1120', 'b1550', 'b2100', 'b2750', 'c3500', 'd4755', 'd5400', 'e7200', 'e7240'];
const SHOTS = ['a100', 'a155', 'a210', 'a220', 'b290', 'b325', 'b345', 'b440', 'b835', 'b1120', 'b1550', 'b2100', 'b2750', 'c3500', 'd4755', 'd5400', 'e7200', 'e7240'];

// E1 진입까지의 자료 무결성과 실행 범위를 검사한다. 게임은 시작하지 않는다.
test('L3 Tutorial4: E1 진입까지의 단계표와 보존 오라클이 있다', () => {
  const { plan } = loadOracle(ORACLE);
  expect(plan.mission).toBe('Tutorial4');
  // 중단본 전체 단계표는 보존하고 E1335의 마지막 체크포인트로 검사 범위를 자른다.
  const steps = plan.steps.filter(([frame]) => frame <= 7240);
  // 기존 수집 파일은 사용자 수동 검증에 사용할 수 있게 유지한다.
  for (const file of PRESERVED_FILES) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  expect(steps.filter(([, op]) => op === 'session-dump').map(([, , label]) => label)).toEqual(DUMPS);
  expect(steps.filter(([, op]) => op === 'snapshot').map(([, , label]) => label)).toEqual(SHOTS);
  // 각 체크포인트의 독립 수집 파일 존재를 확인한다.
  for (const label of DUMPS) expect(fs.existsSync(path.join(ORACLE, `${label}.tsv`)), label).toBe(true);
  for (const label of SHOTS) expect(fs.existsSync(path.join(ORACLE, `${label}.png`)), label).toBe(true);
});

// 생산 등록과 단계 B 안내 창까지는 적 공격 없이 재현 가능한 허용 구간이다.
test('L3 Tutorial4: 생산 등록과 단계 B 안내까지 화면·덤프가 cpppj와 같다', async ({ browser }) => {
  test.setTimeout(180000);
  const { plan } = loadOracle(ORACLE);
  await runL3({
    browser, plan: { ...plan, steps: plan.steps.filter(([frame]) => frame <= 345) }, oracleDir: ORACLE,
    // E1336의 CD 마나 그림 보충으로 생산 슬롯 그림이 생겨 메뉴 폭이 바뀐다. 그 밖은 기존 기준선과 같아야 한다.
    screenRegions: { a220: [{ rect: [802, 392, 1024, 513], pixels: 8656, reason: 'E1336 생산 슬롯 마나 그림 보충과 메뉴 재배치' }] },
  });
});

// E1 진입 뒤 포획·희생 및 그 앞의 특정 적 공격은 자동 재현하지 않는다. 이전 검증은 단위 문서에 남긴다.
test.skip('L3 Tutorial4: 특정 적 공격·포획·희생·승리는 사용자 직접 확인 범위', () => {});
