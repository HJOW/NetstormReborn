// L3 공통 보조: cpppj 오라클과 같은 입력 단계표(steps.json)를 고정 시각 웹에 적용해 체크포인트 프레임의 논리 화면과 Session 덤프를 오라클 폴더의 파일과 비교한다.
// Tutorial1(`l3-tutorial1.spec.js`)과 Tutorial2(`l3-tutorial2.spec.js`)가 같은 틀을 쓰도록 `l3-tutorial1.spec.js` 에서 옮겨 왔다. (2026-11-24 WB04-E1305)
// 단계표의 한 단계는 [프레임, 동작, 인자]이고 동작 이름은 cpppj `--ui-script` 와 같다(`click-if` 라벨 클릭, `click`/`right` 화면 좌표 클릭, `key`, `pointer` 좌표 폴링, `session-dump`, `snapshot`).
// 입력 프레임 규칙(실측): cpppj 는 단계 프레임 N 의 시작에 입력을 큐에 넣어 그 프레임에서 처리한다. 웹은 DOM 사건이라 주입 위치가 동작마다 다르다.
//  - 안내 창 단추 클릭(click-if): 웹은 즉시 처리하지만 cpppj 는 창 닫힘·관문 재개가 한 프레임 뒤에 이어져 웹을 N+1 에 넣어야 같다.
//  - 키(key)와 화면 좌표 좌/우 클릭(click/right): 웹 입력 큐는 다음 프레임 처리라 N-1 에 넣어야 cpppj 의 프레임 N 처리와 같다(Tutorial3 g4185 이동 중 골렘 우클릭 위치 0픽셀 일치에서 right -1 도 동일 규약으로 확인, 세부 근거는 docs/webpj-l3-tutorial1-20261123.md).
//  - 좌표 폴링(pointer)도 N-1에 옮기면 프레임 N의 실제 입력 폴링이 C++와 같다(E1366의 가장자리 시작 시각16.6·좌표·화면으로 확인).
// 체크포인트(session-dump·snapshot)는 N 프레임을 마친 뒤에 찍는다. 시작 순서는 고정 시각 L2 와 같다(메뉴 1프레임 → 미션 시작).
const { expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');

// 단계 동작마다 웹에서 입력을 넣을 프레임 보정(단계 프레임에 더한다)이다. 위 규칙을 값으로 옮겼다. (2026-11-24 WB04-E1305, E1333 right -1 정리)
const INJECT_SHIFT = { 'click-if': 1, click: -1, right: -1, key: -1, esc: -1, pointer: -1 };

// 이름이 `name`(예: 'tutorial1-l3')인 cpppj 기준선 오라클 폴더의 절대 경로를 돌려준다. (2026-11-24 WB04-E1305)
function oracleFolder(name) {
  return path.join(__dirname, '..', 'tests', 'fixtures', 'cpppj', 'candidate-225553df', name);
}

// 오라클 폴더의 단계표·메타를 읽고 기준선을 확인한다. 반환 { plan, meta }. (2026-11-24 WB04-E1305)
function loadOracle(oracleDir) {
  const plan = JSON.parse(fs.readFileSync(path.join(oracleDir, 'steps.json'), 'utf8')), meta = JSON.parse(fs.readFileSync(path.join(oracleDir, 'oracle.json'), 'utf8'));
  expect(meta.cpppjCommit, '오라클 기준선 커밋').toBe('225553df'); expect(plan.screen).toEqual({ width: 1024, height: 768 });
  return { plan, meta };
}

// 단계표를 웹에 적용해 체크포인트를 모두 오라클과 비교한다(화면 0픽셀·덤프는 `sp`·`player:*` 외 차이 없음). (2026-11-24 WB04-E1305: l3-tutorial1.spec.js 에서 이동)
// compare 를 false 로 주면 비교 없이 체크포인트 결과만 돌려준다(어긋남 조사용 임시 단계표를 `L3_DUMP_DIR` 로 덤프할 때). 반환 { results, errors }.
// inspect가 있으면 체크포인트 직후의 실제 앱을 추가 조회할 수 있다(읽기 전용 검사 용도). (WB04-E1327)
// screenRegions는 기준선 뒤 의도된 UI 변경만 사유·사각형·정확한 차이 픽셀 수로 명시한다. 영역 밖은 계속 0픽셀이어야 한다. (WB04-E1337)
// snapshotPhase='presented'는 기존 마지막 표시 화면, 'frameEnd'는 C++ Capture와 같은 카메라 적용 뒤 DIB를 읽는다. frameEnd는 별도 합성 Tell 창이 없는 장면에 쓴다.
// prepareMission은 검사 전용 실제 아군 AI 설치 등에 쓰며 sessionDumpOptions로 선택 AI 계측을 켠다. batchIdleFrames는 호출만 묶고 내부의 모든 프레임은 유지한다.
async function runL3({ browser, plan, oracleDir, compare = true, inspect = null, screenRegions = {}, snapshotPhase = 'presented', prepareMission = null, sessionDumpOptions = {}, batchIdleFrames = false }) {
  if (inspect !== null && typeof inspect !== 'function') throw new TypeError('L3 추가 조회는 함수여야 합니다.');
  if (snapshotPhase !== 'presented' && snapshotPhase !== 'frameEnd') throw new TypeError('L3 화면 관측 시점은 presented 또는 frameEnd여야 합니다.');
  if (prepareMission !== null && typeof prepareMission !== 'function') throw new TypeError('L3 미션 준비는 함수여야 합니다.');
  if (typeof batchIdleFrames !== 'boolean') throw new TypeError('L3 호출 묶음 선택은 불리언이어야 합니다.');
  const context = await browser.newContext({ viewport: { width: plan.screen.width, height: plan.screen.height }, locale: 'en-US' }), page = await context.newPage();
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0 } })));
  await page.goto('/debug.html?fixed=25&originalTutorialPresentation=1&aspect=4:3&title=1');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  await page.locator('canvas').click();
  await page.evaluate(() => window.netstorm._prepareCampaignAudio());
  while ((await page.evaluate(() => window.netstorm.state)) !== 'mainMenu') await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  if (prepareMission) await prepareMission(page);
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
      } else if (input.op === 'pointer') {
        // C++ inspectPoll의 좌버튼 뗌 좌표만 바꾼다. 마우스 버튼 사건을 만들지 않으며 다른 raw 폴링 코드는 명시 거부한다.
        const fields = input.arg.split(',').map(Number), [code, x, y] = fields;
        if (fields.length !== 3 || !fields.every(Number.isInteger) || code !== 0x80010000) throw new Error('L3 pointer는 좌버튼 뗌 code2147549184,x,y만 지원합니다.');
        const point = await toClient({ x, y }); await page.mouse.move(point.x, point.y);
      } else if (input.op === 'key') await page.keyboard.press(input.arg.length === 2 && input.arg[0] === 'f' ? input.arg.toUpperCase() : input.arg);
      // C++ 입력 단계표의 esc도 같은 가상 키 누름으로 보낸다. (WB04-E1327)
      else if (input.op === 'esc') await page.keyboard.press('Escape');
    }
    for (const [frame, op, name] of checkpoints.filter(([frame]) => frame === n)) {
      results[`${op}:${name}`] = op === 'session-dump'
        ? await page.evaluate((options) => window.netstorm.sessionDump(options), sessionDumpOptions)
        : await page.evaluate((phase) => {
          const app = window.netstorm, surface = app._surface, screen = app._clientPaletteScreen;
          // 기존 체크포인트는 마지막 표시 화면을 계속 읽는다. 정지 카메라에서는 프레임 말미 DIB와도 같다.
          if (phase === 'presented') return { width: surface.width, height: surface.height, pixels: Array.from(surface.pixels), palette: Array.from(screen.palette()) };
          if (app._mainMenu?.dialog) throw new Error('L3 frameEnd는 별도 Tell 합성 창이 닫힌 장면에 사용하십시오.');
          const raw = screen.lock(), pixels = new Uint8Array(surface.width * surface.height);
          try {
            // DIB 행 패딩을 제외한 논리 픽셀만 읽고 잠금 깊이는 원래 값으로 돌려놓는다.
            for (let row = 0; row < surface.height; row += 1) pixels.set(raw.subarray(row * screen.pitch, row * screen.pitch + surface.width), row * surface.width);
          } finally { screen.unlock(); }
          return { width: surface.width, height: surface.height, pixels: Array.from(pixels), palette: Array.from(screen.palette()) };
        }, snapshotPhase);
    }
    if (inspect && checkpoints.some(([frame]) => frame === n)) await inspect(page, n, results);
    if (n < last) {
      // 기본 단일 호출은 그대로 두며, 선택 묶음은 다음 실제 입력/관측 프레임까지만 전진한다.
      const next = batchIdleFrames ? Math.min(last, ...inputs.filter((item) => item.at > n).map((item) => item.at), ...checkpoints.filter(([frame]) => frame > n).map(([frame]) => frame)) : n + 1;
      await page.evaluate((count) => window.netstorm.stepFixedFrames(count), next - n); n = next - 1;
    }
  }
  await context.close();
  // 어긋남을 조사할 때 웹 쪽 덤프를 파일로 남긴다: 환경 변수 L3_DUMP_DIR 가 있으면 체크포인트마다 `<이름>.web.tsv` 를 쓴다(오라클 `<이름>.tsv` 와 비교해 첫 어긋난 행을 찾는다). (2026-11-24 WB04-E1299)
  if (process.env.L3_DUMP_DIR) for (const [key, value] of Object.entries(results)) if (key.startsWith('session-dump:')) fs.writeFileSync(path.join(process.env.L3_DUMP_DIR, `${key.slice(13)}.web.tsv`), value);
  if (!compare) return { results, errors };
  expect(errors).toEqual([]);
  const { readPng, indexedToRgb, encodeIndexedPng } = await import('../tools/image-files.mjs'), { compareImages } = await import('../tools/screen-compare.mjs'), { compareSessionDumps } = await import('../tools/session-dump-compare.mjs');
  const normalize = (text) => text.split(/\r?\n/).join('\n');
  // 화면 체크포인트마다 논리 화면이 0픽셀 다르다(다리 조각을 집은 직후 p345 의 커서 조각 표시와 놓은 직후 d364·400 의 새 다리 포함).
  for (const name of checkpoints.filter(([, op]) => op === 'snapshot').map(([, , label]) => label)) {
    const shot = results[`snapshot:${name}`], palette = new Uint8Array(768);
    // BGRX → RGB 로 바꾼다.
    for (let index = 0; index < 256; index += 1) { palette[index * 3] = shot.palette[index * 4 + 2]; palette[index * 3 + 1] = shot.palette[index * 4 + 1]; palette[index * 3 + 2] = shot.palette[index * 4]; }
    const web = indexedToRgb({ width: shot.width, height: shot.height, pixels: Uint8Array.from(shot.pixels) }, palette);
    const regions = screenRegions[name] ?? [];
    const difference = compareImages(readPng(fs.readFileSync(path.join(oracleDir, `${name}.png`))), web, regions);
    // 조사 폴더를 지정한 실행만 웹 논리 화면도 보존해 기존 오라클을 바꾸지 않고 차이 위치를 확인한다. (WB04-E1341)
    if (process.env.L3_DUMP_DIR) fs.writeFileSync(path.join(process.env.L3_DUMP_DIR, `${name}.web.png`), encodeIndexedPng(web));
    expect(difference.outside, `${name} 화면의 설명된 영역 밖 차이 ${JSON.stringify(difference.outsideBox)}`).toBe(0);
    // 변경 영역도 사유와 정확한 픽셀 수를 고정해 다른 결함이 섞이지 않게 한다.
    for (let index = 0; index < regions.length; index += 1) {
      expect(regions[index].reason, `${name} 변경 사유`).toBeTruthy();
      expect(difference.regions[index], `${name} ${regions[index].reason}`).toBe(regions[index].pixels);
    }
  }
  // 덤프 체크포인트마다 `sp`(소유자 0 미초기화 난수)·`player:*`(주소 잔재) 외에는 차이가 없다.
  for (const name of checkpoints.filter(([, op]) => op === 'session-dump').map(([, , label]) => label)) {
    const differences = compareSessionDumps(normalize(fs.readFileSync(path.join(oracleDir, `${name}.tsv`), 'utf8')), normalize(results[`session-dump:${name}`])).differences;
    expect(differences.filter((item) => !(item.key === 'sp' || item.key.startsWith('player:'))).map((item) => `${item.kind}:${item.key}`), `${name} 허용 밖 덤프 차이`).toEqual([]);
  }
  // 같은 대조 결과를 SP 원문 열 등 추가 불변식 검사에도 제공한다. (WB04-E1327)
  return { results, errors };
}

module.exports = { INJECT_SHIFT, oracleFolder, loadOracle, runL3 };
