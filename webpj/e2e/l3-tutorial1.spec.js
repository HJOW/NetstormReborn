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

// 오라클 폴더와 단계표 파일이다.
const ORACLE = path.join(__dirname, '..', 'tests', 'fixtures', 'cpppj', 'candidate-225553df', 'tutorial1-l3');

// 단계 동작마다 웹에서 입력을 넣을 프레임 보정(단계 프레임에 더한다)이다. 위 규칙을 값으로 옮겼다.
const INJECT_SHIFT = { 'click-if': 1, click: -1, right: 1, key: -1 };

test('L3 Tutorial1: MORE→OK→스크롤 연습→F4→Bridges OK→다리 조각 집기·놓기(다리 생성)→두·세 번째 조각 이어 놓기→안내 창→카메라 이동→네 번째 조각 놓기의 체크포인트 화면과 덤프가 cpppj 와 같다', async ({ browser }) => {
  test.setTimeout(600000);
  const plan = JSON.parse(fs.readFileSync(path.join(ORACLE, 'steps.json'), 'utf8')), meta = JSON.parse(fs.readFileSync(path.join(ORACLE, 'oracle.json'), 'utf8'));
  expect(meta.cpppjCommit, '오라클 기준선 커밋').toBe('225553df'); expect(plan.screen).toEqual({ width: 1024, height: 768 });
  // 단계표의 체크포인트 이름마다 오라클의 덤프·화면 파일이 있어야 한다(덤프 c200·c300·c400·c540·c680, 화면 c200·c300·p345·d364·c400·e470·c540·c680).
  for (const file of ['c200.tsv', 'c200.png', 'c300.tsv', 'c300.png', 'c400.tsv', 'c400.png', 'p345.png', 'd364.png', 'e470.png', 'c540.tsv', 'c540.png', 'c680.tsv', 'c680.png']) expect(fs.existsSync(path.join(ORACLE, file)), file).toBe(true);
  expect(plan.steps.filter(([, op]) => op === 'session-dump').map(([, , label]) => label)).toEqual(['c200', 'c300', 'c400', 'c540', 'c680']);
  expect(plan.steps.filter(([, op]) => op === 'snapshot').map(([, , label]) => label)).toEqual(['c200', 'c300', 'p345', 'd364', 'c400', 'e470', 'c540', 'c680']);
  const context = await browser.newContext({ viewport: { width: plan.screen.width, height: plan.screen.height }, locale: 'en-US' }), page = await context.newPage();
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0 } })));
  await page.goto('/debug.html?fixed=25&aspect=4:3&title=1');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  await page.locator('canvas').click();
  await page.evaluate(() => window.netstorm._prepareCampaignAudio());
  while ((await page.evaluate(() => window.netstorm.state)) !== 'mainMenu') await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await page.evaluate((mission) => window.netstorm._beginCampaignFromDialog(mission), plan.mission);
  // 논리 좌표를 브라우저 클라이언트 좌표로 바꾼다.
  const toClient = (logical) => page.evaluate((point) => { const app = window.netstorm, rect = app.canvas.getBoundingClientRect(); return { x: rect.left + point.x * rect.width / app._layout.logicalWidth, y: rect.top + point.y * rect.height / app._layout.logicalHeight }; }, logical);
  // 웹에서 입력을 넣을 프레임별 단계와 체크포인트(프레임을 마친 뒤)로 나눈다.
  const inputs = plan.steps.filter(([, op]) => op in INJECT_SHIFT).map(([frame, op, arg]) => ({ at: frame + INJECT_SHIFT[op], op, arg }));
  const checkpoints = plan.steps.filter(([, op]) => op === 'session-dump' || op === 'snapshot');
  const last = Math.max(...checkpoints.map(([frame]) => frame)), results = {};
  // 주 루프 프레임 번호 n(메뉴 1프레임 포함)을 1씩 올리며 입력과 체크포인트를 처리한다.
  for (let n = 1; n <= last; n += 1) {
    for (const input of inputs.filter((item) => item.at === n)) {
      if (input.op === 'click-if') {
        const logical = await page.evaluate((label) => { const point = window.netstorm._mainMenu?.controlPoint(label); return point ? { x: point.x, y: point.y } : null; }, input.arg);
        if (logical) { const point = await toClient(logical); await page.mouse.click(point.x, point.y, { delay: 0 }); }
      } else if (input.op === 'click' || input.op === 'right') {
        const [x, y] = input.arg.replace('screen:', '').split(',').map(Number), point = await toClient({ x, y });
        await page.mouse.click(point.x, point.y, { delay: 0, button: input.op === 'right' ? 'right' : 'left' });
      } else if (input.op === 'key') await page.keyboard.press(input.arg.length === 2 && input.arg[0] === 'f' ? input.arg.toUpperCase() : input.arg);
    }
    for (const [frame, op, name] of checkpoints.filter(([frame]) => frame === n)) {
      results[`${op}:${name}`] = op === 'session-dump'
        ? await page.evaluate(() => window.netstorm.sessionDump())
        : await page.evaluate(() => { const app = window.netstorm, surface = app._surface; return { width: surface.width, height: surface.height, pixels: Array.from(surface.pixels), palette: Array.from(app._clientPaletteScreen.palette()) }; });
    }
    if (n < last) await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  }
  await context.close();
  // 어긋남을 조사할 때 웹 쪽 덤프를 파일로 남긴다: 환경 변수 L3_DUMP_DIR 가 있으면 체크포인트마다 `<이름>.web.tsv` 를 쓴다(오라클 `<이름>.tsv` 와 비교해 첫 어긋난 행을 찾는다). (2026-11-24 WB04-E1299)
  if (process.env.L3_DUMP_DIR) for (const [key, value] of Object.entries(results)) if (key.startsWith('session-dump:')) fs.writeFileSync(path.join(process.env.L3_DUMP_DIR, `${key.slice(13)}.web.tsv`), value);
  expect(errors).toEqual([]);
  const { readPng, indexedToRgb } = await import('../tools/image-files.mjs'), { compareImages } = await import('../tools/screen-compare.mjs'), { compareSessionDumps } = await import('../tools/session-dump-compare.mjs');
  const normalize = (text) => text.split(/\r?\n/).join('\n');
  // 화면 체크포인트마다 논리 화면이 0픽셀 다르다(다리 조각을 집은 직후 p345 의 커서 조각 표시와 놓은 직후 d364·400 의 새 다리 포함).
  for (const name of checkpoints.filter(([, op]) => op === 'snapshot').map(([, , label]) => label)) {
    const shot = results[`snapshot:${name}`], palette = new Uint8Array(768);
    // BGRX → RGB 로 바꾼다.
    for (let index = 0; index < 256; index += 1) { palette[index * 3] = shot.palette[index * 4 + 2]; palette[index * 3 + 1] = shot.palette[index * 4 + 1]; palette[index * 3 + 2] = shot.palette[index * 4]; }
    const web = indexedToRgb({ width: shot.width, height: shot.height, pixels: Uint8Array.from(shot.pixels) }, palette);
    expect(compareImages(readPng(fs.readFileSync(path.join(ORACLE, `${name}.png`))), web, []).total, `${name} 화면의 다른 픽셀 수`).toBe(0);
  }
  // 덤프 체크포인트마다 `sp`(소유자 0 미초기화 난수)·`player:*`(주소 잔재) 외에는 차이가 없다.
  for (const name of checkpoints.filter(([, op]) => op === 'session-dump').map(([, , label]) => label)) {
    const differences = compareSessionDumps(normalize(fs.readFileSync(path.join(ORACLE, `${name}.tsv`), 'utf8')), normalize(results[`session-dump:${name}`])).differences;
    expect(differences.filter((item) => !(item.key === 'sp' || item.key.startsWith('player:'))).map((item) => `${item.kind}:${item.key}`), `${name} 허용 밖 덤프 차이`).toEqual([]);
  }
});
