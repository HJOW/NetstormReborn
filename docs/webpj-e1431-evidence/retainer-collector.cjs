// 실제 아군 AI의1-1 정상 rAF 진행과 원래 나가기 경로를 최소30분 반복해 World/메모리 반납을 관찰한다.
// 사용: 서버를 켠 뒤 webpj에서 node tools/mission-memory-profile.cjs. 결과는 extracted/webpj-e1431-long-memory.json이다.
// 선택적 반복 상한은 첫 인자20~100(기본100)이며 NETSTORM_MEMORY_HEAP=1이면 마지막 메뉴의 heap snapshot도 남긴다.
'use strict';
// 실제 Chromium, 파일/경로/단언과 사용자 허용 범위의 AI 제어기다.
const { chromium } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
const { installAllyAi } = require('../e2e/ally-ai-test-control');
// 프로젝트와 이 계측의 원문 출력 파일이다.
const root = path.resolve(__dirname, '..');
const output = path.join(root, 'extracted/webpj-e1431-long-memory.json');
// 반복 수와 큰 heap 원문 저장은 계측 실행에만 영향을 주는 선택 사항이다.
const cycleCount = Number(process.argv[2] ?? 100);
// 실제 rAF 앱에서 최소30분·20회 진입/반납을 관찰한다. 반복 수는 상한이다.
const minimumCycles = 20, minimumWallMs = 30 * 60 * 1000;
// 실제 호스트 경과에는 시각 보정 영향을 받지 않는 단조 시계를 사용한다.
const { performance: taskPerformance } = require('node:perf_hooks');
const captureHeap = process.env.NETSTORM_MEMORY_HEAP === '1';
if (!Number.isInteger(cycleCount) || cycleCount < minimumCycles || cycleCount > 100) throw new Error('장기 메모리 계측 반복 상한은20~100 정수여야 합니다.');

// 같은 원래 Client 프레임을 진행하며 실제 미션 준비를 기다린다. 준비 상태 자체는 쓰지 않는다.
async function settle(page, stage) {
    // 실제 비동기 적재/시작에 필요한 프레임만 진행하며 무한 대기를 제한한다.
    for (let frame = 0; frame < 1000; frame += 1) {
        // 현재 World/대본/실제 UI의 준비 상태다.
        const ready = await page.evaluate((mode) => {
            // 매번 현재 앱과 같은 원래 미션 관문을 읽는다.
            const app = window.netstorm, current = app._clientGuide.current();
            if (current?.script.name.toLowerCase() !== 'thewarbegins' || !app._campaignWorld?.session().localConnectionReady()) return false;
            if (mode === 'briefing') return Boolean(app._mainMenu.controlPoint('Play Mission'));
            return current.runtime.gate.started === 1 && !app._gameClock.paused() && Boolean(window.netstormAllyTestState?.brain);
        }, stage);
        if (ready) return frame;
        await page.waitForTimeout(25);
    }
    throw new Error(`실제1-1 준비 상한: ${stage}`);
}

// 실제 Tell 단추를 누르고 원래 입력 프레임을 진행한다.
async function clickLabel(page, label) {
    // 원래 제어점에서 구한 Canvas의 실제 마우스 위치다.
    const point = await page.evaluate((text) => {
        // 현재 앱/제어점/화면 영역을 읽는다.
        const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
        if (!logical) throw new Error(`현재 Tell 단추 없음: ${text}`);
        return { x: rect.left + logical.x * rect.width / app._layout.logicalWidth, y: rect.top + logical.y * rect.height / app._layout.logicalHeight };
    }, label);
    await page.mouse.click(point.x, point.y);
    await page.waitForTimeout(50);
}

// 원래 가로/세로 Gump 메뉴의 행을 실제 마우스로 누르고 자식 메뉴의 애니메이션을 진행한다.
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

// World를 강하게 보관하지 않고 실제 현재 상태와 이전 WeakRef 생존 여부만 반환한다.
async function state(page) {
    return page.evaluate(() => {
        // 현재 앱과 현재 Session이며 반환 객체에는 해당 객체를 넣지 않는다.
        const app = window.netstorm, session = app._campaignWorld?.session();
        // 실제 현재 로컬 AI의 초기화/중지 DWORD를 원래 저장소에서 읽는다.
        const brain = session?.participation().brain(session.localOwner()) ?? 0;
        let ai = null;
        if (brain) {
            // 같은 현재 AI 몸체의 읽기 전용 물리 뷰다.
            const bytes = session.aiMemory().bytes(brain), view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
            ai = { initialized: view.getUint32(0x258, true), stopped: view.getUint32(0x254, true) };
        }
        return { mission: app._clientGuide.current()?.script.name ?? null, world: Boolean(app._campaignWorld), production: Boolean(app._campaignProduction), productionSession: Boolean(app._campaignProduction?.session), productionWorldBound: Boolean(app._campaignProduction?.worldBound), productionMaintenance: Boolean(app._campaignProduction?.maintenance), display: app._campaignDisplayActive(), time: app._gameClock.gameSeconds(), paused: app._gameClock.paused(), brain, ai, worldRefsAlive: window.memoryProbeWorlds.map((reference) => Boolean(reference.deref())), sessionRefsAlive: window.memoryProbeSessions.map((reference) => Boolean(reference.deref())) };
    });
}

// 현재 V8/외부 저장소를 GC 후 읽는다. GC는 게임의 프레임/시각 공급원을 전진시키지 않는다.
async function collected(cdp) { await cdp.send('HeapProfiler.collectGarbage'); return cdp.send('Runtime.getHeapUsage'); }

// 한 앱에서 동일한 허용 미션을 실제30분 이상 시작/반납한다. SP/HP/기술/승패는 변경하지 않는다.
async function run() {
    // 독립 Chromium과 이 관찰의 결과만 보관한다.
    const browser = await chromium.launch({ headless: true });
    const result = { unit: 'WB04-E1431', product: 'E1416 게임 소스/E1427 검사 범위', node: process.version, platform: process.platform, browser: browser.version(), scope: '1-1 실제 아군AI·정상 rAF 60초씩·실제 Leave Mission UI·같은 앱 최소30분/20회·물리120Hz검사아님', requestedCycles: cycleCount, cycles: [], errors: [] };
    try {
        // 같은 페이지를 유지해 앱 전역의 누적 여부도 관찰한다.
        const page = await browser.newPage({ viewport: { width: 1024, height: 768 }, locale: 'en-US' });
        page.on('pageerror', (error) => result.errors.push(error.stack ?? error.message));
        page.on('console', (message) => { if (message.type() === 'error') result.errors.push(message.text()); });
        await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
        await page.goto('http://localhost:9803/debug.html?aspect=4:3&title=1&lang=english');
        await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 });
        await page.locator('canvas').click(); await page.evaluate(() => window.netstorm._prepareCampaignAudio());
        // 시작 그림과 메뉴의 실제 프레임을 모두 실행한다.
        while (await page.evaluate(() => window.netstorm.state !== 'mainMenu')) await page.waitForTimeout(25);
        await page.waitForTimeout(25);
        await installAllyAi(page, 'thewarbegins');
        await page.evaluate(() => { window.memoryProbeWorlds = []; window.memoryProbeSessions = []; });
        // 이 페이지 renderer의 실제 V8 계측 통로다.
        const cdp = await page.context().newCDPSession(page);
        result.initialMenu = { state: await state(page), heap: await collected(cdp) };
        // 처음 메뉴 준비 이후의 실제 앱 관찰 시간을 잰다.
        const wallStart = taskPerformance.now(); result.minimumWallMs = minimumWallMs; result.minimumCycles = minimumCycles; result.startedAt = new Date().toISOString();
        // 원본 미션 진입/UI·실제 AI 진행·원래 반납을 반복하며 각 결과를 즉시 보존한다.
        for (let cycle = 1; cycle <= cycleCount; cycle += 1) {
            await page.evaluate(() => { delete window.netstormAllyTestState; if (!window.netstorm._beginCampaignFromDialog('TheWarBegins')) throw new Error('실제1-1 요청 거부'); });
            await settle(page, 'briefing'); await clickLabel(page, 'Play Mission'); await settle(page, 'running');
            // WeakRef는 이전 World를 테스트 자체가 붙잡지 않도록 사용한다.
            await page.evaluate(() => { window.memoryProbeWorlds.push(new WeakRef(window.netstorm._campaignWorld)); window.memoryProbeSessions.push(new WeakRef(window.netstorm._campaignWorld.session())); });
            // 현재 시작 직후의 검사 관측값과 원래 시계다.
            const activation = await page.evaluate(() => window.netstormAllyTestState), before = await state(page);
            assert.equal(activation.money, activation.before.money); assert.equal(activation.allies, activation.before.allies);
            assert.deepEqual(activation.name, activation.before.name); assert.deepEqual(activation.technology, activation.before.technology);
            // 정상 rAF에서 실제1분을 기다린다. 게임/목표/HP/SP를 직접 쓰지 않는다.
            const activeWallStart = taskPerformance.now();
            await page.waitForTimeout(60000);
            const activeWallMs = taskPerformance.now() - activeWallStart;
            // 실제1분 rAF AI 진행 뒤 World가 살아 있는 상태와 메모리다.
            const active = await state(page), activeHeap = await collected(cdp);
            assert.equal(active.mission.toLowerCase(), 'thewarbegins'); assert.ok(active.brain); assert.equal(active.paused, false);
            assert.deepEqual(active.ai, { initialized: 1, stopped: 0 });
            // 실제 게임 경과는 관측값으로 저장한다. 고정 시계의15초 기대값을 사용하지 않는다.
            const activeGameSeconds = active.time - before.time; assert.ok(activeGameSeconds > 0);
            // 실제 메뉴 확인을 포함한 원래 나가기 경로다. 개별 World/Session을 임의 삭제하지 않는다.
            const left = await leave(page);
            assert.equal(left, true);
            await page.waitForTimeout(250);
            await collected(cdp);
            // 다음 job에서 두 번째 GC 후 읽어 WeakRef 조회 자체의 임시 생존 영향을 피한다.
            let releasedHeap = await collected(cdp), released = await state(page);
            // WeakRef 조회 job의 일시 생존과 지속적인 반납 실패를 구분한다.
            const earlyReleased = released;
            if (released.worldRefsAlive.some(Boolean) || released.sessionRefsAlive.some(Boolean)) {
                await page.waitForTimeout(100); await collected(cdp); await page.waitForTimeout(0); releasedHeap = await collected(cdp); released = await state(page);
            }
            // 장시간 누수의 결론과 구분해 표본/실제 약한 참조 결과를 그대로 보존한다.
            const row = { cycle, activation, before, active, activeHeap, earlyReleased, released, releasedHeap, activeWallMs, activeGameSeconds, gameToWallRatio: activeGameSeconds / (activeWallMs / 1000), wallElapsedMs: taskPerformance.now() - wallStart };
            result.cycles.push(row);
            fs.writeFileSync(output, JSON.stringify(result, null, 2));
            console.log(JSON.stringify({ cycle, activeHeap, releasedHeap, worldRefsAlive: released.worldRefsAlive, sessionRefsAlive: released.sessionRefsAlive, errors: result.errors }));
            assert.equal(released.world, false); assert.equal(released.display, false); assert.equal(released.mission, null);
            assert.equal(released.productionSession, false); assert.equal(released.productionWorldBound, false); assert.equal(released.productionMaintenance, false);
            assert.deepEqual(result.errors, []);
            // 최초 회수 실패의 실제 strong root를 잃지 않도록 단언 전에 원시 heap과 추가 task 경과를 남긴다.
            if (released.worldRefsAlive.some(Boolean) || released.sessionRefsAlive.some(Boolean)) {
                const heapPath = path.join(root, 'extracted/webpj-e1431-retained-cycle-' + cycle + '.heapsnapshot');
                const descriptor = fs.openSync(heapPath, 'w');
                const onChunk = ({chunk}) => fs.writeSync(descriptor,chunk);
                cdp.on('HeapProfiler.addHeapSnapshotChunk',onChunk);
                try { await cdp.send('HeapProfiler.takeHeapSnapshot',{reportProgress:false}); }
                finally { cdp.off('HeapProfiler.addHeapSnapshotChunk',onChunk);fs.closeSync(descriptor); }
                row.retainedHeap={path:heapPath,bytes:fs.statSync(heapPath).size};
                // 실제 다음 task에서1초를 더 기다린 뒤 GC하고 원래 약한 참조를 다시 읽는다. 최초 실패는 덮지 않는다.
                await page.waitForTimeout(1000);await collected(cdp);await page.waitForTimeout(0);
                row.afterAdditionalSecond={heap:await collected(cdp),state:await state(page)};
                console.log(JSON.stringify({cycle,retainedHeap:row.retainedHeap,afterAdditionalSecond:row.afterAdditionalSecond}));
            }
            assert.ok(released.worldRefsAlive.every((alive) => !alive)); assert.ok(released.sessionRefsAlive.every((alive) => !alive));
            result.wallElapsedMs = row.wallElapsedMs; result.actualCycles = result.cycles.length;
            if (cycle >= minimumCycles && row.wallElapsedMs >= minimumWallMs) break;
        }
        // WeakRef의 job 중 임시 생존과 늦은 비동기 반납을 구분하도록 실제 다음 task까지 기다린다.
        await page.evaluate(() => new Promise((resolve) => setTimeout(resolve, 100)));
        await collected(cdp);
        await page.evaluate(() => new Promise((resolve) => setTimeout(resolve, 0)));
        result.settled = { heap: await collected(cdp), state: await state(page) };
        console.log(JSON.stringify({ settled: result.settled }));
        if (captureHeap) {
            // 최종 WeakRef 조회의 임시 생존을 지난 task에서 해제한 뒤 실제 heap을 수집한다.
            await page.evaluate(() => new Promise((resolve) => setTimeout(resolve, 100)));
            await collected(cdp);
            // 큰 원문은 Git 제외 조사 경로에만 저장하며 각 chunk를 파일에 바로 쓴다.
            const heapPath = path.join(root, 'extracted/webpj-e1431-long-memory.heapsnapshot');
            const descriptor = fs.openSync(heapPath, 'w');
            const onChunk = ({ chunk }) => fs.writeSync(descriptor, chunk);
            cdp.on('HeapProfiler.addHeapSnapshotChunk', onChunk);
            try { await cdp.send('HeapProfiler.takeHeapSnapshot', { reportProgress: false }); }
            finally { cdp.off('HeapProfiler.addHeapSnapshotChunk', onChunk); fs.closeSync(descriptor); }
            result.heapSnapshot = { path: heapPath, bytes: fs.statSync(heapPath).size };
        }
        result.progress = await page.evaluate(() => localStorage.getItem('netstorm.progress'));
        result.wallElapsedMs = taskPerformance.now() - wallStart; result.actualCycles = result.cycles.length;
        assert.ok(result.actualCycles >= minimumCycles && result.wallElapsedMs >= minimumWallMs);
        result.complete = true;
    } finally { fs.writeFileSync(output, JSON.stringify(result, null, 2)); await browser.close(); }
}
run().catch((error) => { console.error(error); process.exitCode = 1; });
