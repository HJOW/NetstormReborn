// 실제 공통 API 런타임을 가진 유계 Worker 장치로 사건 순서/시계/반납과 저장 실패 경계를 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { ModRuntime } from '../www/modding/mod-api.js';
import { ModManager } from '../www/modding/mod-manager.js';
import { ModTimeSource } from '../www/modding/mod-time-source.js';
import { ClientTimeSource } from '../www/core/client-time-source.js';
import { ClientFrameClock } from '../www/core/client-frame-clock.js';
// 공통 상태 벡터를 원본 그대로 읽고 각 테스트에서 사본만 변경한다.
const trace = JSON.parse(fs.readFileSync(new URL('../../shared/modding/v0/conformance/core-trace.json', import.meta.url)));
// 비동기 대기를 시험이 명시 해제하므로 실제 임의 sleep/타이머가 필요 없다.
function deferred() { let resolve; const promise = new Promise((done) => { resolve = done; }); return { promise, resolve }; }
// 내부 Session 대신 공개 값 사본만 제공하는 장치다. ModRuntime의 본 검증은 생략하지 않는다.
function fixture({ ids = ['test.a', 'test.b'], handlers = {}, forcedOff = false, failSave = false, createGate } = {}) {
    let raw = 1000000, snapshot = structuredClone(trace.cases[0].snapshot), storeDisposed = false;
    const native = new ClientTimeSource({ hostMilliseconds: () => raw, hostUnixSeconds: () => 1000 }), source = new ModTimeSource(native), clock = new ClientFrameClock(source, { value: 0 });
    const events = [], commits = [], reports = [], workers = [], saves = [];
    const entries = ids.map((id) => ({ manifest: { id }, enabled: true }));
    const store = { async load() {}, list: () => entries, getPackage: (id) => ({ manifest: { id } }), async setEnabled(id, enabled, options) { if (storeDisposed) throw new Error('disposed'); saves.push({ id, enabled, options }); if (failSave) throw new Error('quota'); }, dispose() { storeDisposed = true; } };
    const host = { snapshot: () => structuredClone(snapshot), sessionSeed: () => 123, dispose() { this.disposed = true; }, commit(output, context) { commits.push({ output, context }); return output.commands.map((command) => ({ eventSequence: output.eventSequence, commandId: command.commandId, status: 'queued', code: null })); } };
    // register/dispatch는 제품의 ModRuntime을 그대로 호출하고 Worker 수명만 신뢰 장치로 대신한다.
    const createWorker = async () => {
        if (createGate) await createGate.promise;
        const worker = { runtime: null, disposed: false, async register(pkg, seed) { this.id = pkg.manifest.id; this.runtime = new ModRuntime({ modId: this.id, sessionSeed: seed }); return this.runtime.register(handlers[this.id] ?? (() => {})); }, async dispatch(event, state) { events.push({ id: this.id, event: structuredClone(event), snapshot: structuredClone(state) }); return this.runtime.dispatch(event, state); }, dispose() { this.disposed = true; this.runtime?.dispose(); } };
        workers.push(worker); return worker;
    };
    const manager = new ModManager({ host, source, store, createWorker, forcedOff, report: (kind, detail) => reports.push({ kind, detail }) });
    return { manager, source, clock, host, store, entries, events, commits, reports, workers, saves, setTime(value) { raw = 1000000 + value; }, setSnapshot(value) { snapshot = structuredClone(value); }, advanceGame(seconds) { snapshot.state.gameSeconds = seconds; } };
}
test('고정 오라클과 설치0개는 저장 읽기/Worker/시계 장벽을 만들지 않는다', async () => {
    // 두 비활성 경로는 각각 다른 이유로 실행을 건너뛴다.
    for (const options of [{ forcedOff: true }, { ids: [] }]) { const f = fixture(options); await f.manager.load(); f.manager.afterFrame(); f.advanceGame(20); f.manager.afterFrame(); assert.equal(f.manager.pending, false); assert.equal(f.workers.length, 0); f.setTime(1000); assert.equal(f.clock.wallSeconds(), 1); await f.manager.dispose(); }
});
test('Worker 대기의 game/wall 동시 정지 뒤 원래 경과만 복귀한다', async () => {
    const gate = deferred(), f = fixture({ createGate: gate }); await f.manager.load(); f.setTime(500); f.clock.captureFrame(); f.manager.afterFrame();
    assert.equal(f.manager.pending, true); f.setTime(1500); assert.equal(f.clock.gameSeconds(), .5); assert.equal(f.clock.wallSeconds(), .5);
    gate.resolve(); await f.manager.whenIdle(); assert.equal(f.manager.pending, false); assert.equal(f.clock.wallSeconds(), .5); f.setTime(1525); assert.equal(f.clock.captureFrame().game, 525 * .001); await f.manager.dispose();
});
test('사건 phase→id와 모드별 sequence를 지키며 입력을 다음 원래 경계까지 미룬다', async () => {
    const f = fixture({ ids: ['test.b', 'test.a'], handlers: { 'test.a': (api) => { api.on('sessionStart', () => api.commands.selectPriest()); api.on('commandResult', () => api.commands.nextUnit()); api.on('tick', () => api.commands.produceBridge(0)); }, 'test.b': (api) => api.on('tick', () => api.commands.rotatePiece()) } });
    await f.manager.load(); f.manager.afterFrame(); await f.manager.whenIdle(); assert.deepEqual(f.events.map((item) => item.id), ['test.a', 'test.b']); assert.equal(f.commits.length, 0);
    f.manager.beforeInput(); assert.equal(f.commits.length, 1); f.advanceGame(1); f.manager.afterFrame(); await f.manager.whenIdle();
    assert.deepEqual(f.events.slice(2).map(({ id, event }) => [id, event.name, event.sequence]), [['test.a', 'commandResult', 2], ['test.a', 'tick', 3], ['test.b', 'tick', 2]]);
    assert.equal(f.commits.length, 1); f.manager.beforeInput(); assert.deepEqual(f.commits.slice(1).map((item) => item.output.eventSequence), [2, 3, 2]);
    f.manager.afterFrame(); await f.manager.whenIdle(); assert.deepEqual(f.events.slice(-2).map(({ id, event }) => [id, event.name, event.sequence]), [['test.a', 'commandResult', 4], ['test.b', 'commandResult', 3]]); await f.manager.dispose();
});
test('같은 경계의 결과8+tick8은 다음 결과사건16개 하나에 머문다', async () => {
    // 최대8명령을 각 콜백에서 내도 결과사건이 자기증폭으로 분할되지 않아야 한다.
    const commands = (api) => { for (let index = 0; index < 8; index++) api.commands.selectPriest(); };
    const f = fixture({ ids: ['test.a'], handlers: { 'test.a': (api) => { api.on('sessionStart', () => commands(api)); api.on('commandResult', () => commands(api)); api.on('tick', () => commands(api)); } } });
    await f.manager.load(); f.manager.afterFrame(); await f.manager.whenIdle(); f.manager.beforeInput(); f.advanceGame(10); f.manager.afterFrame(); await f.manager.whenIdle(); f.manager.beforeInput(); f.manager.afterFrame(); await f.manager.whenIdle();
    assert.equal(f.events.filter((item) => item.event.name === 'tick').length, 1); const last = f.events.at(-1).event; assert.equal(last.name, 'commandResult'); assert.equal(last.results.length, 16); assert.equal(f.saves.length, 0); await f.manager.dispose();
});
test('한 모드 예외/저장 실패는 해당 실행만 끄고 정상 모드는 계속한다', async () => {
    const f = fixture({ failSave: true, handlers: { 'test.a': (api) => api.on('sessionStart', () => { api.commands.selectPriest(); throw new Error('bad'); }), 'test.b': (api) => api.on('tick', () => api.commands.nextUnit()) } });
    await f.manager.load(); f.manager.afterFrame(); await f.manager.whenIdle(); f.manager.beforeInput(); assert.equal(f.commits.length, 0); assert.equal(f.workers[0].disposed, true); assert.equal(f.saves[0].id, 'test.a'); assert.equal(f.saves[0].enabled, false); assert.match(f.manager.error, /quota/u);
    f.advanceGame(1); f.manager.afterFrame(); await f.manager.whenIdle(); f.manager.beforeInput(); assert.equal(f.commits.length, 1); assert.equal(f.events.at(-1).id, 'test.b'); await f.manager.dispose();
});
test('실행 중 설정 변경은 다음 미션에 반영하고 종료는 마지막 사본/새 VM을 사용한다', async () => {
    const f = fixture({ handlers: { 'test.a': (api) => api.on('sessionEnd', (event) => api.log('info', event.reason)) } });
    await f.manager.load(); f.manager.afterFrame(); await f.manager.whenIdle(); f.entries[1].enabled = false; f.advanceGame(1); f.manager.afterFrame(); await f.manager.whenIdle(); assert.ok(f.events.some((item) => item.id === 'test.b' && item.event.name === 'tick'));
    f.manager.end('leave'); const empty = structuredClone(trace.cases[0].snapshot); empty.state.sessionId = 'session-2'; f.setSnapshot(empty); await f.manager.whenIdle(); assert.ok(f.workers.every((item) => item.disposed)); assert.equal(f.events.at(-1).event.sessionId, trace.cases[0].snapshot.state.sessionId); assert.ok(f.reports.some((item) => item.kind === 'log' && item.detail.text === 'leave'));
    f.manager.afterFrame(); await f.manager.whenIdle(); assert.equal(f.workers.length, 3); assert.equal(f.events.at(-1).event.sequence, 1); assert.equal(f.events.at(-1).id, 'test.a'); await f.manager.dispose();
});
test('앱 정리는 대기 중에도 host/시계를 즉시 놓고 늦은 Worker를 반납한다', async () => {
    const gate = deferred(), f = fixture({ createGate: gate }); await f.manager.load(); f.manager.afterFrame(); await Promise.resolve(); const closing = f.manager.dispose();
    assert.equal(f.host.disposed, true); assert.equal(f.manager.pending, false); f.setTime(1000); assert.equal(f.clock.wallSeconds(), 1); gate.resolve(); await closing;
    assert.ok(f.workers.every((item) => item.disposed)); assert.equal(f.saves.length, 0); assert.equal(f.events.length, 0);
});

test('등록 예외의 로그/명령은 폐기하고 다른 모드의 시작을 계속한다', async () => {
    const f = fixture({ handlers: { 'test.a': (api) => { api.log('info', 'discard'); throw new Error('registration'); } } });
    await f.manager.load(); f.manager.afterFrame(); await f.manager.whenIdle(); assert.equal(f.workers[0].disposed, true); assert.equal(f.saves[0].id, 'test.a'); assert.equal(f.reports.some((item) => item.kind === 'log'), false); assert.deepEqual(f.events.map((item) => item.id), ['test.b']); await f.manager.dispose();
});

test('사건 대기 중 이탈은 늦은 명령을 폐기하고 같은 VM에 마지막 사본 종료를 보낸다', async () => {
    const gate = deferred(), f = fixture({ ids: ['test.a'], handlers: { 'test.a': (api) => api.on('tick', () => api.commands.selectPriest()) } });
    await f.manager.load(); f.manager.afterFrame(); await f.manager.whenIdle();
    const worker = f.workers[0], dispatch = worker.dispatch.bind(worker);
    worker.dispatch = async (event, snapshot) => { if (event.name === 'tick') await gate.promise; return dispatch(event, snapshot); };
    f.advanceGame(1); f.manager.afterFrame(); await Promise.resolve(); f.manager.end('leave');
    const newer = structuredClone(trace.cases[0].snapshot); newer.state.sessionId = 'session-9'; f.setSnapshot(newer); gate.resolve(); await f.manager.whenIdle(); f.manager.beforeInput();
    assert.equal(f.commits.length, 0); assert.equal(f.events.at(-1).event.name, 'sessionEnd'); assert.equal(f.events.at(-1).event.sequence, 3); assert.equal(f.events.at(-1).snapshot.state.gameSeconds, 1); assert.equal(worker.disposed, true); assert.equal(f.saves.length, 0); await f.manager.dispose();
});

test('미션 이탈은 아직 부팅 중인 Worker를 취소하고 오류 활성 저장을 바꾸지 않는다', async () => {
    const f = fixture({ ids: ['test.a'] }), entered = deferred();
    const manager = new ModManager({ host: f.host, source: f.source, store: f.store, createWorker: ({ signal }) => new Promise((resolve, reject) => { entered.resolve(); signal.addEventListener('abort', () => reject(new Error('부팅 취소')), { once: true }); }) });
    await manager.load(); manager.afterFrame(); await entered.promise; manager.end('leave'); await manager.whenIdle();
    assert.equal(manager.pending, false); assert.equal(f.saves.length, 0); await manager.dispose();
});
