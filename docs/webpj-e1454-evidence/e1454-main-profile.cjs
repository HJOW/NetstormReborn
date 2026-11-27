// 실제1-1 아군AI의 정상rAF/표시상한/시계/메모리와 원래 반납을 관찰한다.
// 사용: 서버를 켠 뒤 webpj에서 node tools/menu-frame-profile.cjs. 결과는 extracted/webpj-e1454-main-profile.json이다.
'use strict';
// 실제 Chromium과 파일/경로/단언 API다.
const { chromium } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
// 실제 실행 시점의 본 소스 바이트를 기록한다.
const {createHash}=require('node:crypto');
// 실제 허용1-1의 아군AI 활성화기다.
const {installAllyAi}=require('../e2e/ally-ai-test-control');
// 현재 프로젝트와 이 관찰만 쓰는 출력 경로다.
const root = path.resolve(__dirname, '..');
const resultPath = path.join(root, 'extracted/webpj-e1454-main-profile.json');

// 원래 메뉴가 제공하는 논리 좌표를 실제 마우스로 누른다.
async function clickLabel(page, label) {
    await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
    // 논리 위치를 현재 브라우저의 실제 Canvas 영역으로 변환한다.
    const point = await page.evaluate((text) => {
        // 같은 현재 앱/제어점/화면 영역이며 게임 값을 쓰지 않는다.
        const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
        return { x: rect.left + logical.x * rect.width / app._layout.logicalWidth, y: rect.top + logical.y * rect.height / app._layout.logicalHeight };
    }, label);
    await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 실제 rAF 관측과 같은 게임 시계의 경과를 한 구간에서 비교한다. 프레임/시각 공급원은 대체하지 않는다.
async function sample(page, durationMs) {
    return page.evaluate(async (duration) => {
        // 현재 앱과 관찰 전용 rAF 횟수/예약 번호다.
        const app = window.netstorm;
        let rafCount = 0, handle, observing = true;
        // 원래 게임과 별개인 읽기 전용 rAF 관찰 예약이다.
        const observe = () => { rafCount += 1; if (observing) handle = requestAnimationFrame(observe); };
        // 현재 호스트 시간과 원본 게임 시계/처리 프레임을 같은 순서로 읽는다.
        const read = () => ({ monotonicMs: performance.now(), game: app._gameClock.gameSeconds(), wall: app._gameClock.wallSeconds(), frame: app._gameClock.time().number, paused: app._gameClock.paused(), maxFPS: app._settings.get('maxFPS'), state: app.state, hidden: document.hidden, mission: app._clientGuide.current()?.script.name??null, brain: app._campaignWorld?.session().participation().brain(1)??0, layout:{...app._layout} });
        const before = read();
        handle = requestAnimationFrame(observe);
        try { await new Promise((resolve) => setTimeout(resolve, duration)); }
        finally { observing = false; cancelAnimationFrame(handle); }
        // 동일한 실제 호스트에서 경과한 시간이다.
        const after = read(), seconds = (after.monotonicMs - before.monotonicMs) / 1000;
        return { before, after, rafCount, seconds, processedFrames: after.frame - before.frame, processedFps: (after.frame - before.frame) / seconds, rafHz: rafCount / seconds, gameDelta: after.game - before.game, wallDelta: after.wall - before.wall };
    }, durationMs);
}

async function clickMenuRow(page, token, index) {
    // 현재 메뉴의 실제 배치와 기존 자식 토큰만 읽는다.
    const point = await page.evaluate(({ current, row }) => {
        // 게임 메뉴 원본 노드와 같은 화면/생성 저장소다.
        const app = window.netstorm, root = app._clientGump, node = root.node(current), bounds = root.collection().bounds(current), rect = app.canvas.getBoundingClientRect();
        if (!node || row < 0 || row >= node.items.length) throw new Error('현재 게임 메뉴 행 범위 오류');
        // 원래0x40000 가로 메뉴 플래그와 항목별 세로 오프셋을 사용한다.
        const horizontal = Boolean(node.menuFlags & 0x40000), x = horizontal ? node.itemWidth * (row + 0.5) : 20, y = horizontal ? (bounds.bottom - bounds.top) / 2 : node.offsets[row] + node.heights[row] / 2;
        return { child: node.items[row].childId ? root.createState().entries[node.items[row].childId] : 0, x: rect.left + (bounds.left + x) * rect.width / app._layout.logicalWidth, y: rect.top + (bounds.top + y) * rect.height / app._layout.logicalHeight };
    }, { current: token, row: index });
    await page.mouse.move(point.x, point.y); await page.mouse.click(point.x, point.y, { delay: 60 });
    await page.waitForTimeout(300);
    return point.child;
}

// 실제 Game → Leave Mission → ABORT 첫 단추로 나가 전체 입력/State 반납 경로를 실행한다.
async function leave(page) {
    await page.locator('[data-netstorm="game-menu-button"]').click();
    await page.waitForTimeout(300);
    // 실제 막대와 그 Game 자식의 현재 항목이다.
    const bar = await page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().token());
    const game = await clickMenuRow(page, bar, 0);
    const labels = await page.evaluate((token) => window.netstorm._campaignProduction.ui.gameMenu().labels(token), game);
    await clickMenuRow(page, game, labels.indexOf('Leave Mission'));
    // 원본 ABORT의 첫 단추가 메인 메뉴 복귀 경로다.
    const confirm = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons[0]?.label ?? null);
    assert.equal(typeof confirm, 'string'); await clickLabel(page, confirm);
    // 실제 큐/State가 이탈을 마칠 때까지만 원래 프레임을 진행한다.
    for (let frame = 0; frame < 200; frame += 1) {
        if (await page.evaluate(() => !window.netstorm._campaignWorld && !window.netstorm._clientGuide.current())) return true;
        await page.waitForTimeout(25);
    }
    throw new Error('실제 Leave Mission 반납 상한');
}


// CDP의 실제 renderer target 지표를 이름별로 읽는다.
async function metrics(cdp) { return Object.fromEntries((await cdp.send('Performance.getMetrics')).metrics.map((metric) => [metric.name, metric.value])); }

// 같은 앱에서 두 표시상한의 허용 미션을 진행하고 제품 파일을 바꾸지 않는다.
async function run() {
    // 실제 Chromium/새 저장소/메뉴 관찰 결과다.
    const browser = await chromium.launch({ headless: true });
    const result = { unit: 'WB04-E1454', product: 'E1454 성능4모듈 본 소스·대체0', node: process.version, browser: browser.version(), platform: process.platform, scope: '실제 아군AI1-1·정상rAF·1920x1080·실제메뉴60/120각30초두표본·원래Leave UI', sourceSha256:Object.fromEntries(['www/netstorm.js','www/render/presenter.js','www/render/screen.js','www/core/raw-surface-world.js'].map(file=>[file,createHash('sha256').update(fs.readFileSync(path.join(root,file))).digest('hex')])),samples: [], errors: [] };
    try {
        // 이 관찰만 소유하는 페이지다. 사용자의 저장소와 분리한다.
        const page = await browser.newPage({ viewport: { width: 1920, height: 1080 }, locale: 'en-US' });
        page.on('pageerror', (error) => result.errors.push(error.stack ?? error.message));
        page.on('console', (message) => { if (message.type() === 'error') result.errors.push(message.text()); });
        await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
        await page.goto('http://localhost:9803/debug.html?aspect=16:9&title=1&lang=english');
        await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 });
        await page.locator('canvas').click();
        await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 120000 });
        await page.bringToFront();
        await installAllyAi(page,'thewarbegins');
        // 실제 renderer의 CDP 수집기를 켠다. V8 회수 후의 표본과 일반 표본을 구별한다.
        const cdp = await page.context().newCDPSession(page);
        await cdp.send('Performance.enable');
        // 동일 앱에서60/120을 두 번씩 선택하며 장시간/미션 누수 검사라고 주장하지 않는다.
        for (const rate of [60, 120]) {
            await clickLabel(page, 'Options'); await clickLabel(page, 'Frame Rate'); await clickLabel(page, `${rate} FPS`);
            await page.waitForFunction((value) => window.netstorm._settings.get('maxFPS') === value && !window.netstorm._mainMenu.popup, rate, { timeout: 30000 });
            // 원래 미션 진입/Play Mission 단추 뒤 실제 아군AI 초기화만 기다린다.
            await page.evaluate(()=>{delete window.netstormAllyTestState;window.netstorm._beginCampaignFromDialog('TheWarBegins');});
            await page.waitForFunction(()=>Boolean(window.netstorm._mainMenu.controlPoint('Play Mission'))&&Boolean(window.netstorm._campaignWorld?.session().localConnectionReady()),null,{timeout:120000});
            await clickLabel(page,'Play Mission');
            await page.waitForFunction(()=>window.netstorm._clientGuide.current()?.runtime.gate.started===1&&!window.netstorm._gameClock.paused()&&Boolean(window.netstormAllyTestState?.brain),null,{timeout:60000});
            const activation=await page.evaluate(()=>window.netstormAllyTestState);for(const key of ['money','allies','name','technology'])assert.deepEqual(activation[key],activation.before[key]);
            // 두30초 구간을 따로 보존해 실제시간 한도와 관찰 시점을 구별한다.
            for(let interval=0;interval<2;interval++){
            await cdp.send('HeapProfiler.collectGarbage');
            // 같은 구간 전후의 V8/외부 backing storage와 renderer 작업 시간이다.
            const heapBefore = await cdp.send('Runtime.getHeapUsage'), metricBefore = await metrics(cdp);
            const observed = await sample(page, 30000);
            const metricAfter = await metrics(cdp), heapAfter = await cdp.send('Runtime.getHeapUsage');
            await cdp.send('HeapProfiler.collectGarbage');
            const heapAfterGc = await cdp.send('Runtime.getHeapUsage');
            const row = { rate, interval, activation, ...observed, heapBefore, heapAfter, heapAfterGc, rendererTaskSeconds: metricAfter.TaskDuration - metricBefore.TaskDuration, rendererMetricsBefore: metricBefore, rendererMetricsAfter: metricAfter };
            assert.equal(row.before.mission.toLowerCase(),'thewarbegins');assert.equal(row.after.mission.toLowerCase(),'thewarbegins');assert.ok(row.before.brain);assert.ok(row.after.brain);
            assert.equal(row.before.paused, false); assert.equal(row.after.paused, false);
            assert.equal(row.before.hidden, false); assert.equal(row.after.hidden, false);
            assert.equal(row.after.maxFPS, rate); assert.ok(row.processedFrames > 10);
            assert.ok(Math.abs(row.gameDelta - row.seconds) < 0.05, JSON.stringify(row));
            assert.ok(Math.abs(row.wallDelta - row.seconds) < 0.05, JSON.stringify(row));
            result.samples.push(row);
            fs.writeFileSync(resultPath, JSON.stringify(result, null, 2));
            console.log(JSON.stringify({ rate, processedFps: row.processedFps, rafHz: row.rafHz, seconds: row.seconds, gameDelta: row.gameDelta, heapAfterGc }));
            }
            await page.keyboard.press('F5');await page.screenshot({path:path.join(root,'extracted/e1454-main-mission-'+rate+'.png')});
            await leave(page);await page.waitForFunction(()=>!window.netstorm._campaignWorld&&!window.netstorm._clientGuide.current());
        }
        assert.deepEqual(result.errors, []);
        result.storage = await page.evaluate(() => localStorage.getItem('netstorm.settings'));
        result.complete = true;
    } finally {
        fs.writeFileSync(resultPath, JSON.stringify(result, null, 2));
        await browser.close();
    }
}
run().catch((error) => { console.error(error); process.exitCode = 1; });
