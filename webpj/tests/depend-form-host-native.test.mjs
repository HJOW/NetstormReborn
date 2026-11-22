// DependFormHost 의 Native(원문 PE) 진단 모드를 확인한다: 제거 인자 이상·통지 뒤 watchedFid·FormFinder 시작 진단을 표시한 뒤 계속한다. (2026-11-22 WB04-E1217)
// `bindNativeFind` 를 연결하지 않은 독립 입력은 기존 오류 계약을 유지한다(tests/form-hosts*.test.mjs 가 원본 관측과 대조).
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { DependFormHost } from '../www/core/depend-form-host.js';

// 자산 TYPE(70 이상) 슬롯 두 개와 Depend Form TYPE(1) 조건을 갖춘 풀·호스트를 만든다.
function scene() {
    const pool = new SidPool(32768, { server: true });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, cost: 0, name: 'T' }));
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    const events = []; const host = new DependFormHost(pool, destroy, { notice: () => {}, fallback: {}, transmit: null });
    const watcher = pool.allocate(2); const watched = pool.allocate(2); pool.physical(watcher)[10] = 100; pool.physical(watched)[10] = 101;
    return { pool, host, watcher, watched, events };
}
const file = String.fromCharCode(92) + 'Ns' + String.fromCharCode(92) + 'O' + String.fromCharCode(92) + 'Squid.cpp';

test('DependFormHost Native: 제거의 인자 이상 다섯 진단은 표시한 뒤 계속하고 FormFinder 시작 진단을 부른다', () => {
    const { pool, host, watcher, watched, events } = scene();
    host.bindCreation({ pool }, () => 1, (...args) => events.push(args)); host.bindNativeFind((parent) => events.push(['find', parent]));
    // 정상 인자: 진단 없이 시작 진단만 부른다.
    host.remove(watcher, watched, 0); assert.deepEqual(events, [['find', watched]]);
    // watcher 가 Depend TYPE(1)이거나 watched 가 Form(TYPE<70)이거나 같은 객체이면 해당 진단을 표시하고도 계속한다.
    events.length = 0; pool.physical(watcher)[10] = 1; host.remove(watcher, watched, 0); assert.deepEqual(events, [['watcher.getType() != dependFormType', file, 0xb4b], ['find', watched]]);
    events.length = 0; pool.physical(watcher)[10] = 100; pool.physical(watched)[10] = 5; host.remove(watcher, watched, 0); assert.deepEqual(events, [['!watched.isForm()', file, 0xb4c], ['find', watched]]);
    events.length = 0; pool.physical(watched)[10] = 100; host.remove(watched, watched, 0); assert.deepEqual(events, [['watcherSid != watchedSid', file, 0xb4e], ['find', watched]]);
    // 이미 dead 인 watched 는 진단 없이 바로 돌아온다.
    events.length = 0; pool.physical(watched)[11] |= 2; host.remove(watcher, watched, 0); assert.deepEqual(events, []);
});

test('DependFormHost Native: 독립 입력(진단 미연결)은 기존 오류 계약을 유지한다', () => {
    const { pool, host, watcher, watched } = scene();
    host.bindCreation({ pool }, () => 1, null);
    pool.physical(watcher)[10] = 1; assert.throws(() => host.remove(watcher, watched, 0), /감시 대상 타입 오류/);
    assert.throws(() => host.remove(watcher, 0, 0), /제거 부모 번호 오류/); assert.throws(() => host.has(0, watcher), /조회 부모 번호 오류/);
});

test('DependFormHost Native: 조회는 범위 확인 대신 FormFinder 시작 진단을 먼저 부른다', () => {
    const { pool, host, watcher, watched, events } = scene();
    host.bindCreation({ pool }, () => 1, (...args) => events.push(['assert', ...args])); host.bindNativeFind((parent) => events.push(['find', parent]));
    assert.equal(host.has(watched, watcher), false); assert.deepEqual(events, [['find', watched]]);
});
