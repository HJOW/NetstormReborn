// 공통 계약을 실제 공개 런타임에 적용한다. 게임 구현을 흉내 내는 원시 주소/슬롯 fixture를 만들지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { ModRuntime, ModError, MOD_API_VERSION, MOD_ROOT_NAMES, MOD_COMMAND_NAMES, MOD_EVENT_NAMES, MOD_KEY_NAMES, MOD_ERROR_CODES, MOD_LIMITS, modRandomSeed, emptyModSnapshot, copyModJson, validateModSnapshot } from '../www/modding/mod-api.js';

// null prototype 경계 사본은 JSON 값으로 비교한다.
const plain = (value) => JSON.parse(JSON.stringify(value));
// 실제 호스트가 만들어야 하는 최소 공개 상태다.
function snapshot(seconds = 0) {
    return { state: { sessionId: 'session-1', mission: 'firstblood', gameSeconds: seconds, localOwner: 1, paused: false, logicalSize: { width: 1024, height: 768 }, mapRect: { left: 85, top: 0, right: 1024, bottom: 768 } }, players: [{ owner: 1, stormPower: 1000, allies: [2] }, { owner: 2, stormPower: 500, allies: [1] }], selection: { typeId: 163, owner: 1, health: 100 } };
}
// 같은 상태 경계의 시작 또는 tick 사건을 만든다.
function event(name = 'sessionStart', sequence = 1, seconds = 0) {
    return { name, sequence, sessionId: 'session-1', gameSeconds: seconds, ...(name === 'sessionStart' ? { mission: 'firstblood' } : {}) };
}
// 오류 코드를 판별해 엔진마다 다른 스택 문자열에 기대지 않는다.
const code = (expected) => (error) => error instanceof ModError && error.code === expected;

test('공개 이름·버전·한도가 공통 JSON과 정확히 같다', async () => {
    const contract = JSON.parse(await readFile(new URL('../../shared/modding/v0/contract.json', import.meta.url)));
    assert.equal(MOD_API_VERSION, contract.apiVersion);
    // 각각 독립한 목록이므로 누락/임의 별칭을 모두 잡는다.
    for (const [actual, key] of [[MOD_ROOT_NAMES, 'rootNames'], [MOD_COMMAND_NAMES, 'commandNames'], [MOD_EVENT_NAMES, 'eventNames'], [MOD_KEY_NAMES, 'keyNames'], [MOD_ERROR_CODES, 'errorCodes']]) assert.deepEqual(actual, contract[key]);
    assert.deepEqual(MOD_LIMITS, contract.limits);
    const runtime = new ModRuntime({ modId: 'test.names' });
    runtime.register((api) => { assert.deepEqual(Object.keys(api), contract.rootNames); assert.deepEqual(Object.keys(api.commands), contract.commandNames); assert.ok(Object.isFrozen(api)); assert.ok(Object.isFrozen(api.commands)); });
});

test('새 등록은 미션 없는 사본만 읽고 게임 명령을 실행하지 않는다', () => {
    const runtime = new ModRuntime({ modId: 'test.initial' });
    const output = runtime.register((api) => {
        assert.deepEqual(plain(api.getState()), emptyModSnapshot().state); assert.deepEqual(api.getPlayers(), []); assert.equal(api.getSelection(), null);
        assert.throws(() => api.commands.selectPriest(), code('noSession'));
    });
    assert.deepEqual(plain(output), { eventSequence: 0, commands: [], logs: [] });
});

test('조회와 사건의 수정이 호스트·다음 콜백·다음 조회에 전파되지 않는다', () => {
    const input = snapshot(), runtime = new ModRuntime({ modId: 'test.copies' });
    runtime.register((api) => {
        api.on('sessionStart', (message) => { message.mission = 'changed'; api.getPlayers()[0].stormPower = 9999; api.getState().logicalSize.width = 1; api.getSelection().health = 0; });
        api.on('sessionStart', (message) => { assert.equal(message.mission, 'firstblood'); assert.equal(api.getPlayers()[0].stormPower, 1000); assert.equal(api.getState().logicalSize.width, 1024); assert.equal(api.getSelection().health, 100); });
    });
    runtime.dispatch(event(), input); assert.equal(input.players[0].stormPower, 1000); assert.equal(input.selection.health, 100);
});

test('구독을 콜백 안에서 바꿔도 사건 시작 목록으로 전달한다', () => {
    const calls = [], runtime = new ModRuntime({ modId: 'test.events' });
    runtime.register((api) => {
        let second;
        api.on('tick', () => { calls.push('a'); if (second) { assert.equal(api.off(second), true); second = 0; api.on('tick', () => calls.push('c')); } });
        second = api.on('tick', () => calls.push('b'));
    });
    runtime.dispatch(event(), snapshot()); runtime.dispatch(event('tick', 2, 1), snapshot(1)); runtime.dispatch(event('tick', 3, 2), snapshot(2));
    assert.deepEqual(calls, ['a', 'b', 'a', 'c']);
});

test('저장한 API는 동기 실행 바깥에서 조회·로그·명령을 거절한다', () => {
    let saved; const runtime = new ModRuntime({ modId: 'test.lifetime' }); runtime.register((api) => { saved = api; });
    // 호출할 수 있는 모든 종류의 함수에서 같은 경계를 확인한다.
    for (const action of [() => saved.getState(), () => saved.getPlayers(), () => saved.getSelection(), () => saved.randomInt(10), () => saved.log('info', 'x'), () => saved.on('tick', () => {}), () => saved.off(1), () => saved.commands.selectPriest(), () => saved.commands.nextUnit(5), () => saved.commands.produceBridge(99)]) assert.throws(action, code('permissionDenied'));
    runtime.dispose(); assert.throws(() => saved.getState(), code('disposed'));
});

test('RNG는 Python 독립 golden과 같고 모드 사이에서 섞이지 않는다', async () => {
    const golden = JSON.parse(await readFile(new URL('../../shared/modding/v0/conformance/rng-vectors.json', import.meta.url)));
    // 각 벡터는 동일 모드의 두 독립 런타임에서도 일치해야 한다.
    for (const vector of golden.vectors) {
        assert.equal(modRandomSeed(vector.sessionSeed, vector.modId), vector.initialState);
        const a = new ModRuntime(vector), b = new ModRuntime(vector), results = [];
        a.register((api) => { for (const item of vector.samples) results.push(api.randomInt(item.limit)); });
        b.register((api) => { assert.deepEqual(vector.samples.map((item) => api.randomInt(item.limit)), results); });
        assert.deepEqual(results, vector.samples.map((item) => item.value));
    }
});

test('난수의 잘못된 인수는 상태를 전진시키지 않는다', () => {
    const a = new ModRuntime({ modId: 'test.random' }), b = new ModRuntime({ modId: 'test.random' }); let actual;
    a.register((api) => { for (const limit of [0, -1, 1.5, 0x100000000, NaN]) assert.throws(() => api.randomInt(limit), code('invalidArgument')); actual = api.randomInt(100); });
    b.register((api) => assert.equal(api.randomInt(100), actual));
});

test('일곱 공개 명령은 원래 클릭/키 요청으로만 정규화된다', () => {
    const runtime = new ModRuntime({ modId: 'test.commands' });
    runtime.register((api) => api.on('sessionStart', () => {
        assert.equal(api.commands.click(100, 200, 'right', { control: true }), 1); api.commands.pressKey('ArrowLeft'); api.commands.selectPriest(); api.commands.nextUnit(true); api.commands.produceBridge(5); api.commands.repeatProduction(); api.commands.rotatePiece();
    }));
    const output = plain(runtime.dispatch(event(), snapshot()));
    assert.equal(output.commands.length, 7); assert.deepEqual(output.commands[0], { commandId: 1, sessionId: 'session-1', name: 'click', args: { modifiers: { shift: false, control: true }, x: 100, y: 200, button: 'right' } });
    assert.deepEqual(output.commands.slice(1).map((item) => item.args.key), ['ArrowLeft', 'p', 'n', 'x', 'd', 'c']);
    assert.equal(output.commands[3].args.modifiers.shift, true); assert.ok(output.commands.every((item) => ['click', 'pressKey'].includes(item.name)));
});

test('좌표·키·슬롯·보조 키 오류는 명령 번호를 소비하지 않는다', () => {
    const runtime = new ModRuntime({ modId: 'test.arguments' });
    runtime.register((api) => api.on('sessionStart', () => {
        // 현재 논리 화면과 고정 허용 목록을 벗어난 입력을 전부 검사한다.
        for (const action of [() => api.commands.click(-1, 0), () => api.commands.click(1024, 0), () => api.commands.click(10.5, 1), () => api.commands.click(1, 1, 'middle'), () => api.commands.pressKey('Escape'), () => api.commands.pressKey('p', { alt: false }), () => api.commands.pressKey('p', { shift: 1 }), () => api.commands.produceBridge(6), () => api.commands.nextUnit(1)]) assert.throws(action, code('invalidArgument'));
        assert.equal(api.commands.selectPriest(), 1);
    }));
    assert.equal(runtime.dispatch(event(), snapshot()).commands.length, 1);
});

test('처리한 명령/로그 한도 오류는 정상 앞 요청을 유지한다', () => {
    const runtime = new ModRuntime({ modId: 'test.quotas' });
    runtime.register((api) => api.on('sessionStart', () => {
        // 계약 한도까지는 기록하고 초과 건은 부분 기록 없이 거절한다.
        for (let index = 0; index < 8; index += 1) { api.commands.selectPriest(); api.log('info', String(index)); }
        assert.throws(() => api.commands.selectPriest(), code('quotaExceeded')); assert.throws(() => api.log('info', '초과'), code('quotaExceeded'));
    }));
    const result = runtime.dispatch(event(), snapshot()); assert.equal(result.commands.length, 8); assert.equal(result.logs.length, 8); assert.equal(runtime.disabled, false);
});

test('구독 한도를 처리해도 다음 정상 사건이 실행된다', () => {
    const runtime = new ModRuntime({ modId: 'test.handler-quota' }); let called = 0;
    runtime.register((api) => { for (let index = 0; index < 32; index += 1) api.on('tick', () => { called += 1; }); assert.throws(() => api.on('tick', () => {}), code('quotaExceeded')); });
    runtime.dispatch(event(), snapshot()); runtime.dispatch(event('tick', 2, 1), snapshot(1)); assert.equal(called, 32);
});

test('한 모드의 예외는 그 사건의 미반영 요청을 폐기하고 다른 모드는 계속한다', () => {
    const bad = new ModRuntime({ modId: 'test.bad' }), good = new ModRuntime({ modId: 'test.good' });
    bad.register((api) => api.on('sessionStart', () => { api.commands.selectPriest(); api.log('info', '버릴 로그'); throw new Error('의도한 오류'); }));
    good.register((api) => api.on('sessionStart', () => api.log('info', '계속')));
    assert.throws(() => bad.dispatch(event(), snapshot()), code('scriptError')); assert.equal(bad.disabled, true); assert.throws(() => bad.dispatch(event('tick', 2, 1), snapshot(1)), code('disposed'));
    assert.equal(good.dispatch(event(), snapshot()).logs[0].text, '계속'); assert.equal(good.disabled, false);
});

test('비동기 엔트리와 핸들러 반환은 계약 위반이다', () => {
    const a = new ModRuntime({ modId: 'test.async-entry' }); assert.throws(() => a.register(async () => {}), code('scriptError')); assert.equal(a.disabled, true);
    const b = new ModRuntime({ modId: 'test.async-event' }); b.register((api) => api.on('sessionStart', async () => {})); assert.throws(() => b.dispatch(event(), snapshot()), code('scriptError')); assert.equal(b.disabled, true);
});

test('종료는 마지막 상태를 읽되 명령을 거절하고 컨텍스트를 반납한다', () => {
    const runtime = new ModRuntime({ modId: 'test.end' }); let saved;
    runtime.register((api) => { saved = api; api.on('sessionEnd', () => { assert.equal(api.getState().gameSeconds, 2); assert.throws(() => api.commands.selectPriest(), code('noSession')); api.log('info', '끝'); }); });
    runtime.dispatch(event(), snapshot());
    const result = runtime.dispatch({ name: 'sessionEnd', sequence: 2, sessionId: 'session-1', mission: 'firstblood', gameSeconds: 2, reason: 'leave' }, snapshot(2));
    assert.equal(result.logs[0].text, '끝'); assert.equal(runtime.disabled, true); assert.throws(() => saved.getState(), code('disposed'));
});

test('다른 미션·중복/이전 사건·미등록 전달은 효과 전에 거절한다', () => {
    const runtime = new ModRuntime({ modId: 'test.order' }); assert.throws(() => runtime.dispatch(event(), snapshot()), code('invalidArgument'));
    runtime.register(() => {}); runtime.dispatch(event(), snapshot());
    assert.throws(() => runtime.dispatch(event(), snapshot()), code('invalidArgument'));
    assert.throws(() => runtime.dispatch(event('tick', 3, 2), snapshot(2)), code('invalidArgument'));
    const other = snapshot(); other.state.sessionId = 'session-2';
    assert.throws(() => runtime.dispatch({ ...event('tick', 2), sessionId: 'session-2' }, other), code('invalidArgument')); assert.equal(runtime.disabled, false);
    runtime.dispatch(event('tick', 2, 1), snapshot(1)); assert.throws(() => runtime.dispatch(event('tick', 3, 0), snapshot(0)), code('invalidArgument'));
});

test('명령 결과는 알려진 상태/오류 코드만 통과한다', () => {
    const runtime = new ModRuntime({ modId: 'test.results' }); let results;
    runtime.register((api) => api.on('commandResult', (message) => { results = message.results; })); runtime.dispatch(event(), snapshot());
    const valid = { name: 'commandResult', sequence: 2, sessionId: 'session-1', results: [{ eventSequence: 1, commandId: 1, status: 'queued', code: null }, { eventSequence: 1, commandId: 2, status: 'rejected', code: 'queueFull' }] };
    runtime.dispatch(valid, snapshot()); assert.deepEqual(plain(results), valid.results);
    assert.throws(() => runtime.dispatch({ ...valid, sequence: 3, results: [{ eventSequence: 1, commandId: 1, status: 'queued', code: 'queueFull' }] }, snapshot()), code('invalidArgument'));
});

test('데이터 경계는 getter·심볼·빈 배열칸·비유한 값·과대/순환 구조를 실행하지 않는다', () => {
    let read = 0; const accessor = Object.defineProperty({}, 'x', { enumerable: true, get() { read += 1; return 1; } });
    const arrayGetter = [0]; Object.defineProperty(arrayGetter, '0', { enumerable: true, get() { read += 1; return 1; } });
    const cycle = {}; cycle.self = cycle;
    // 한 사례가 실패해도 다음 검증 비용이나 전역 상태에 영향이 없어야 한다.
    for (const invalid of [accessor, arrayGetter, { [Symbol('x')]: 1 }, Array(2), { x: Infinity }, { x: 1n }, new Date(), cycle, Array(4097).fill(0), '가'.repeat(30000)]) assert.throws(() => copyModJson(invalid), code('invalidArgument'));
    assert.equal(read, 0);
    const pollution = copyModJson(JSON.parse('{"__proto__":{"changed":true}}')); assert.equal(Object.getPrototypeOf(pollution), null); assert.equal({}.changed, undefined);
});

test('호스트 상태의 내부 필드 노출·플레이어 순서·잘못된 동맹을 거절한다', () => {
    // 매번 독립 상태를 변경해 어떤 검증이 실패했는지 구분한다.
    for (const mutate of [(input) => { input.state.raw = {}; }, (input) => { input.selection.sid = 100; }, (input) => { input.players.reverse(); }, (input) => { input.players[0].allies = [1]; }, (input) => { input.state.gameSeconds = NaN; }, (input) => { input.state.mapRect.right = 2000; }]) { const input = snapshot(); mutate(input); assert.throws(() => validateModSnapshot(input), code('invalidArgument')); }
    assert.deepEqual(plain(validateModSnapshot(emptyModSnapshot())), emptyModSnapshot());
});

test('공통 패키지 하나가 다섯 경계의 WB 기준 출력과 일치한다', async () => {
    const directory = new URL('../../shared/modding/v0/conformance/', import.meta.url);
    const pkg = JSON.parse(await readFile(new URL('core-probe.nsmod.json', directory))), trace = JSON.parse(await readFile(new URL('core-trace.json', directory)));
    // 저장소에 고정한 신뢰 예제만 직접 import한다. 제품의 모드 로더를 대신하는 경로가 아니다.
    const entry = await import('data:text/javascript;base64,' + Buffer.from(pkg.modules[pkg.manifest.entry]).toString('base64'));
    const runtime = new ModRuntime({ modId: pkg.manifest.id, sessionSeed: trace.sessionSeed });
    assert.deepEqual(plain(runtime.register(entry.default)), trace.registration);
    const actual = trace.cases.map(({ event, snapshot }) => plain(runtime.dispatch(event, snapshot)));
    assert.deepEqual(actual, trace.expected); assert.equal(runtime.disabled, true);
});

test('DN에 전달할 V8 수학 격자가 double 비트·부호0·비유한 분류를 보존한다', async () => {
    const golden = JSON.parse(await readFile(new URL('../../shared/modding/v0/conformance/math-vectors.json', import.meta.url)));
    // 허용 오차나 float32 반올림으로 double 차이를 숨기지 않는다.
    for (const vector of golden.vectors) {
        const result = Math[vector.name](...vector.args.map(Number)), view = new DataView(new ArrayBuffer(8)); view.setFloat64(0, result, false);
        const bits = Number.isNaN(result) ? 'NaN' : view.getBigUint64(0, false).toString(16).padStart(16, '0');
        assert.equal(bits, vector.expected, `${vector.name}(${vector.args})`);
    }
});

test('두 요청 사건의 16개 결과를 한 번에 받고 다음 출력은 여전히8개로 제한한다', () => {
    const runtime = new ModRuntime({ modId: 'test.result-bound' }); let count = 0;
    runtime.register((api) => {
        api.on('sessionStart', () => { for (let index = 0; index < 8; index += 1) api.commands.selectPriest(); });
        api.on('tick', () => { for (let index = 0; index < 8; index += 1) api.commands.nextUnit(); });
        api.on('commandResult', (event) => { count = event.results.length; for (let index = 0; index < 8; index += 1) api.commands.selectPriest(); assert.throws(() => api.commands.selectPriest(), code('quotaExceeded')); });
    });
    runtime.dispatch(event(), snapshot()); runtime.dispatch(event('tick', 2, 1), snapshot(1));
    const results = [1, 2].flatMap((eventSequence) => Array.from({ length: 8 }, (_, index) => ({ eventSequence, commandId: index + 1, status: 'queued', code: null })));
    const output = runtime.dispatch({ name: 'commandResult', sequence: 3, sessionId: 'session-1', results }, snapshot(1)); assert.equal(count, 16); assert.equal(output.commands.length, 8);
});

test('결과17개·중복·역순은 콜백 전에 거절한다', () => {
    const runtime = new ModRuntime({ modId: 'test.result-invalid' }); let calls = 0;
    runtime.register((api) => api.on('commandResult', () => { calls += 1; })); runtime.dispatch(event(), snapshot()); runtime.dispatch(event('tick', 2, 1), snapshot(1));
    const result = (commandId) => ({ eventSequence: 1, commandId, status: 'queued', code: null });
    // 각 잘못된 전송은 같은 다음 sequence를 소비하지 않는다.
    for (const results of [Array(17).fill(result(1)), [result(1), result(1)], [result(2), result(1)]]) assert.throws(() => runtime.dispatch({ name: 'commandResult', sequence: 3, sessionId: 'session-1', results }, snapshot(1)), code('invalidArgument'));
    assert.equal(calls, 0); runtime.dispatch({ name: 'commandResult', sequence: 3, sessionId: 'session-1', results: [result(1)] }, snapshot(1)); assert.equal(calls, 1);
});

test('결과 사건을 받은 모드와 받지 않은 모드의 sequence는 각각 증가한다', () => {
    const a = new ModRuntime({ modId: 'test.sequence-a' }), b = new ModRuntime({ modId: 'test.sequence-b' });
    a.register((api) => { api.on('sessionStart', () => api.commands.selectPriest()); api.on('tick', (event) => api.log('info', String(event.sequence))); });
    b.register((api) => api.on('tick', (event) => api.log('info', String(event.sequence))));
    a.dispatch(event(), snapshot()); b.dispatch(event(), snapshot()); a.dispatch({ name: 'commandResult', sequence: 2, sessionId: 'session-1', results: [{ eventSequence: 1, commandId: 1, status: 'queued', code: null }] }, snapshot());
    assert.equal(a.dispatch(event('tick', 3, 1), snapshot(1)).logs[0].text, '3'); assert.equal(b.dispatch(event('tick', 2, 1), snapshot(1)).logs[0].text, '2');
});
