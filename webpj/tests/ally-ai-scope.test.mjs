// 최신 사용자 검사 범위가 공용 활성화기와 승리 CLI에서 브라우저 실행 전에 적용되는지 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import scope from '../e2e/ally-ai-scope.js';
import control from '../e2e/ally-ai-test-control.js';

// 허용된 정확한 여섯 이름만 대소문자 정규화 뒤 실제 페이지 평가에 전달한다.
test('아군 AI 범위: 캠페인 1-1~1-5와 스커미시 허용', async () => {
  // 평가 본문을 실행하지 않고 전달 인자만 관찰한다. 실제 활성화·진행은 Chromium 검사에서 확인한다.
  for (const mission of ['TheWarBegins', 'MasterOfWhirligigs', 'SaveTheIsland', 'FragileFortune', 'ThunderingPower', 'Skirmish']) {
    let received = null;
    const page = { async evaluate(_body, input) { received = input; } };
    assert.equal(scope.assertAllyAiMission(mission), mission.toLowerCase());
    await control.installAllyAi(page, mission); assert.equal(received.name, mission.toLowerCase());
  }
});

// 제한 밖 목표 수행은 페이지 평가와 Chromium 생성 전에 거부한다.
test('아군 AI 범위: 1-6·튜토리얼·후반 캠페인의 활성화와 승리 CLI 거부', async () => {
  const page = { evaluate() { throw new Error('금지 미션의 페이지 접근'); } };
  // 실제 CLI의 실패 문구를 확인해 구문 오류와 범위 거부를 구별한다.
  for (const mission of ['DissolvedAlliance', 'Tutorial1', 'Tutorial6', 'TheNoose', 'unknown']) {
    await assert.rejects(control.installAllyAi(page, mission), /허용 미션/);
    const result = spawnSync(process.execPath, ['webpj/tools/ally-ai-campaign-flow.cjs', mission, '100'], { cwd: new URL('../../', import.meta.url), encoding: 'utf8' });
    assert.equal(result.status, 1); assert.match(result.stderr, /캠페인 1-1~1-5와 스커미시만 허용/);
  }
  await assert.rejects(control.installAllyAi(page, null), /문자열/);
});

// 재현 가능한 시작 대기와 원래 사제 채취 능력 전략만 허용하고 잘못된 조건은 실행 전에 거부한다.
test('아군 AI 조건: 메뉴 대기 범위와 사제 보호 전략의 전달·거부', async () => {
  // 실제 메뉴에서 기다릴 프레임은 문자열 CLI와 직접 호출에서 같은 정수로 정리한다.
  for (const frames of [0, 40, 400, 2400]) assert.deepEqual(scope.allyAiCondition(String(frames), 'guard-priest'), { menuFrames: frames, profile: 'guard-priest' });
  let received = null;
  await control.installAllyAi({ async evaluate(_body, input) { received = input; } }, 'SaveTheIsland', { profile: 'guard-priest' });
  assert.equal(received.profile, 'guard-priest');
  const page = { evaluate() { throw new Error('잘못된 전략에서 페이지 접근'); } };
  await assert.rejects(control.installAllyAi(page, 'SaveTheIsland', { profile: '../invalid' }), /검사 전략/);
  // 오류가 있는 조건은 Chromium 실행/파일명 작성에 도달하지 않아야 한다.
  for (const [delay, profile] of [['-1', 'default'], ['2401', 'default'], ['1.5', 'default'], ['0', '../invalid']]) {
    const result = spawnSync(process.execPath, ['webpj/tools/ally-ai-campaign-flow.cjs', 'savetheisland', '100', delay, profile], { cwd: new URL('../../', import.meta.url), encoding: 'utf8' });
    assert.equal(result.status, 1); assert.match(result.stderr, /아군 AI (메뉴 대기|검사 전략)/);
  }
});
