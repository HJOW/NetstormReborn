// L3 대조: cpppj 오라클과 같은 입력 단계표(`tests/fixtures/cpppj/candidate-225553df/tutorial1-l3/steps.json`)를 고정 시각 웹에 적용해 체크포인트 프레임의 논리 화면과 Session 덤프를 비교한다. (2026-11-23 WB04-E1284)
// 단계표의 한 단계는 [프레임, 동작, 인자]이고 동작 이름은 cpppj `--ui-script` 와 같다(`click-if` 라벨 클릭, `click`/`right` 화면 좌표 클릭, `key`, `session-dump`, `snapshot`).
// 입력 프레임 규칙(실측): cpppj 는 단계 프레임 N 의 시작에 입력을 큐에 넣어 그 프레임에서 처리한다. 웹은 DOM 사건이라 주입 위치가 동작마다 다르다.
//  - 안내 창 단추 클릭(click-if): 웹은 즉시 처리하지만 cpppj 는 창 닫힘·관문 재개가 한 프레임 뒤에 이어져 웹을 N+1 에 넣어야 같다.
//  - 키(key)와 화면 좌표 클릭(click): 웹 입력 큐는 다음 프레임 처리라 N-1 에 넣어야 cpppj 의 프레임 N 처리와 같다. 화면 좌표 클릭(생산 창 조각 집기·월드 놓기)은 처음에 N+1 로 두었으나 그러면 웹이 cpppj 보다 2프레임 늦게 놓아
//    세 번째 조각 뒤 안내 단계(`builtForGenus > 7`)와 그 잠금 해제가 2프레임 밀려 530프레임에서 게임 시계가 어긋났다. N-1 에서는 498~530프레임의 덤프가 모두 같다(`click-if` 는 +1 그대로). (2026-11-24 WB04-E1299)
// 체크포인트(session-dump·snapshot)는 N 프레임을 마친 뒤에 찍는다. 시작 순서는 고정 시각 L2 와 같다(메뉴 1프레임 → 미션 시작).
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const { runL3 } = require('./l3-common');

// 오라클 폴더와 단계표 파일이다.
const ORACLE = path.join(__dirname, '..', 'tests', 'fixtures', 'cpppj', 'candidate-225553df', 'tutorial1-l3');

test('L3 Tutorial1: MORE→OK→스크롤 연습→F4→Bridges OK→다리 조각 집기·놓기(다리 생성)→두·세 번째 조각 이어 놓기→안내 창→카메라 이동→네·다섯 번째 조각 놓기(간헐천 섬·단계 D 안내 창)→Part 2 안내 창→여섯·일곱 번째 조각 놓기(회전·간헐천 연결, 단계 E 안내 창)→사제 선택·간헐천으로 걷기→크리스털 배달·단계 F 안내 창→수확 이어 가기·SP 600 미션 완료 창의 체크포인트 화면과 덤프가 cpppj 와 같다', async ({ browser }) => {
  test.setTimeout(600000);
  const plan = JSON.parse(fs.readFileSync(path.join(ORACLE, 'steps.json'), 'utf8')), meta = JSON.parse(fs.readFileSync(path.join(ORACLE, 'oracle.json'), 'utf8'));
  expect(meta.cpppjCommit, '오라클 기준선 커밋').toBe('225553df'); expect(plan.screen).toEqual({ width: 1024, height: 768 });
  // 단계표의 체크포인트 이름마다 오라클의 덤프·화면 파일이 있어야 한다(덤프 c200·c300·c400·c540·c680·c760·c880·c1000·c1200·c1400·c1700·c2000·c2600·c3200·c3800·c4200, 화면 c200·c300·p345·d364·c400·e470·c540·c680·c760·c880·c1000·p1070·c1200·c1400·c1700·c2000·c2600·c3200·c3800·c4200).
  for (const file of ['c200.tsv', 'c200.png', 'c300.tsv', 'c300.png', 'c400.tsv', 'c400.png', 'p345.png', 'd364.png', 'e470.png', 'c540.tsv', 'c540.png', 'c680.tsv', 'c680.png', 'c760.tsv', 'c760.png', 'c880.tsv', 'c880.png', 'c1000.tsv', 'c1000.png', 'p1070.png', 'c1200.tsv', 'c1200.png', 'c1400.tsv', 'c1400.png', 'c1700.tsv', 'c1700.png', 'c2000.tsv', 'c2000.png', 'c2600.tsv', 'c2600.png', 'c3200.tsv', 'c3200.png', 'c3800.tsv', 'c3800.png', 'c4200.tsv', 'c4200.png']) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  expect(plan.steps.filter(([, op]) => op === 'session-dump').map(([, , label]) => label)).toEqual(['c200', 'c300', 'c400', 'c540', 'c680', 'c760', 'c880', 'c1000', 'c1200', 'c1400', 'c1700', 'c2000', 'c2600', 'c3200', 'c3800', 'c4200']);
  expect(plan.steps.filter(([, op]) => op === 'snapshot').map(([, , label]) => label)).toEqual(['c200', 'c300', 'p345', 'd364', 'c400', 'e470', 'c540', 'c680', 'c760', 'c880', 'c1000', 'p1070', 'c1200', 'c1400', 'c1700', 'c2000', 'c2600', 'c3200', 'c3800', 'c4200']);
  await runL3({ browser, plan, oracleDir: ORACLE });
});
