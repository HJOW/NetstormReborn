// 실행 결과 화면을 저장할 파일 API와 실제 Chromium 검사 도구다.
const fs = require('node:fs');
const { chromium } = require('@playwright/test');
const { installAllyAi } = require('../e2e/ally-ai-test-control');
// 사용자가 허용한 미션의 실제 아군 AI를 관찰할 프레임 상한이다.
const mission = process.argv[2] ?? 'thewarbegins', limit = Number(process.argv[3] ?? 36000);
// 게임 상태는 바꾸지 않고 실제 판정 호출 결과와 희생 종료 호출만 관찰한다.
async function run() {
  const browser = await chromium.launch({ headless: true });
  try {
    const page = await browser.newPage({ viewport: { width: 1024, height: 768 }, locale: 'en-US' }), errors = [];
    page.on('pageerror', error => errors.push(error.stack ?? error.message));
    page.on('console', message => { if (message.type() === 'error') errors.push(message.text()); });
    await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
    await page.goto('http://localhost:9803/debug.html?fixed=25&aspect=4:3&title=1');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 });
    await page.locator('canvas').click(); await page.evaluate(() => window.netstorm._prepareCampaignAudio());
    // 시작 그림과 메뉴를 원래 프레임 순서로 통과한다.
    while (await page.evaluate(() => window.netstorm.state !== 'mainMenu')) await page.evaluate(() => window.netstorm.stepFixedFrames(1));
    await page.evaluate(() => window.netstorm.stepFixedFrames(1));
    await installAllyAi(page, mission);
    await page.evaluate(async () => {
      const { RawMissionOutcome } = await import('./core/raw-mission-outcome.js');
      const { RawDaisRegular } = await import('./core/raw-dais-regular.js');
      window.allyObservation = { events: [], captured: {}, dead: {} };
      // 실제 원래 조회 결과가 바뀌는 시점만 기록한다.
      for (const [name, key] of [['priestCaptured', 'captured'], ['priestDead', 'dead']]) {
        const original = RawMissionOutcome.prototype[name];
        RawMissionOutcome.prototype[name] = function (owner) {
          const result = original.call(this, owner), state = window.allyObservation;
          if (state[key][owner] !== result) { state[key][owner] = result; state.events.push({ name, owner, result, time: window.netstorm._gameClock.gameSeconds() }); }
          return result;
        };
      }
      // 자연 AI에 의한 원래 희생 완료를 관찰하며 호출 인자·결과·효과를 그대로 유지한다.
      const finish = RawDaisRegular.prototype.finishSacrifice;
      RawDaisRegular.prototype.finishSacrifice = function (...args) { window.allyObservation.events.push({ name: 'finishSacrifice', args, time: window.netstorm._gameClock.gameSeconds() }); return finish.apply(this, args); };
    });
    await page.evaluate(name => window.netstorm._beginCampaignFromDialog(name), mission);
    await page.evaluate(() => window.netstorm.stepFixedFrames(120));
    const point = await page.evaluate(() => { const app = window.netstorm, p = app._mainMenu.controlPoint('Play Mission'), r = app.canvas.getBoundingClientRect(); return { x: r.left+p.x*r.width/1024, y: r.top+p.y*r.height/768 }; });
    await page.mouse.click(point.x, point.y);
    //100개 호출로 묶어도 실제 Kernel·렌더러는 모든 프레임을 돈다.
    for (let frame = 0; frame <= limit; frame += 100) {
      if (frame) await page.evaluate(() => window.netstorm.stepFixedFrames(100));
      if (frame % 2000 !== 0 && frame !== limit) continue;
      const current = await page.evaluate(() => {
        const app = window.netstorm, session = app._campaignWorld.session(), pool = session.core().pool, types = {}, priests = [];
        // 실제 할당 객체의 종류·소유자와 사제 HP·수납 부모를 읽는다.
        for (let sid = 1; sid < pool.capacity; sid += 1) {
          const raw = pool.slot(sid); if (raw[11] & 1) continue;
          const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength), key = raw[34] + ':' + raw[10];
          types[key] = (types[key] ?? 0) + 1;
          if (raw[10] === 158) priests.push({ sid, owner: raw[34], hp: view.getInt32(26, true), flags: raw[11], bytes: Array.from(raw.slice(0, 90)) });
        }
        return { time: app._gameClock.gameSeconds(), state: app.state, sp: session.sp(1), types, priests, observation: window.allyObservation, buttons: app._mainMenu.dialog?.buttons?.map(button => button.label) ?? [], gate: app._clientGuide.current()?.runtime.gate };
      });
      console.log(JSON.stringify({ mission, frame, ...current, errors }));
      if (errors.length) throw new Error('실제 프레임 오류 발생');
      // 실제 완료 단추가 뜨면 자동 진행을 멈추고 그 화면을 보존한다. 완료값을 직접 쓰지 않는다.
      if(current.buttons.some(label=>/^(Next Mission|Leave Mission)$/i.test(label)))break;
    }
    await page.screenshot({ path: 'extracted/ally-observe-'+mission+'.png' });
  } finally { await browser.close(); }
}
run().catch(error => { console.error(error); process.exitCode = 1; });
