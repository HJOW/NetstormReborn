// Factory의 현재 World 대여/장기 공간 조회와 호출 전체 pin을 실제 Pool/Form 부모로 검증한다(WB04-E592).
// 225553df SquidFactory.cpp235..278의 소스 계약 대조이며 새 원본 관측/게임 실행이 아니다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { PATCH_TYPE_CONSTRUCTORS } from '../www/core/squid-constructors.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { GameAssets } from '../www/assets/game-assets.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawSquidContainedUnpop, commonContainedPrevious, commonContainedSetPrevious, commonContainedParent } from '../www/core/squid-contained-unpop.js';
import { put, get } from './raw-support.mjs';

// 실제 생성자 표의 같은 Pool/공장/공간 부모와 현재 진단 입력이다.
function scene() {
    const pool = new SidPool(32768);
    const types = PATCH_TYPE_CONSTRUCTORS.map((address) => ({ constructorAddress: address }));
    const factory = new SquidFactory(pool, types);
    factory.bindDiagnostics({ types });
    const unpop = new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536));
    return { pool, types, factory, unpop };
}
// 검사에서 선택하지 않아야 하는 장치는 빈 성공 대신 실패한다.
function forbidden() { throw new Error('선택하지 않은 장치'); }
// 실제 부모를 쓰되 이번에 선택하지 않는 가상 장치는 명시 실패인 대여이다.
function runtime(s, changes = {}) { return { unpop: s.unpop, formDetach: forbidden, contentType: () => 6, formTransmit: forbidden, ...changes }; }
// 실제 공통 Form을 부모에 붙인다. 0x10은 전송을 억제하며 이후 부모/분리 검사를 할 수 있다.
function attached(s) {
    const parent = s.factory.create(74, 2); const form = s.factory.createGumpForm(2);
    s.factory.attachForm(form, parent, 0x10);
    return { parent, form };
}

// Form Unpop 접두가 실행된 뒤 실제 종속 분리만 실행한다. 공통 Unpop을 재귀 호출하지 않는다.
function detach(s, sid) {
    new RawSquidContainedUnpop(s.pool, { contentFormType: () => 6, gumpFormType: () => 3, uiEvent: () => 0 }, {
        parent: (form) => commonContainedParent(s.pool, form), previous: (form) => commonContainedPrevious(s.pool, form),
        setPrevious: (form, previous) => commonContainedSetPrevious(s.pool, form, previous), parentNotice: forbidden,
    }).detach(sid);
}

test('Factory Runtime: 같은 Pool/필수 장치/동일 대여 관문은 원문 상태를 쓰지 않는다', () => {
    const s = scene(); const r = runtime(s); const other = scene(); const bytes = s.pool.bytes();
    // 각 거부 입력은 Pool을 변경하지 않아야 한다.
    for (const input of [null, {}, runtime(s, { unpop: other.unpop }), runtime(s, { formDetach: null }), runtime(s, { contentType: null }), runtime(s, { formTransmit: null })]) {
        assert.throws(() => s.factory.bindRuntime(input), /실제 World/);
    }
    const use = s.factory.bindRuntime(r); assert.throws(() => s.factory.bindRuntime(runtime(s)), /실제 World/);
    assert.deepEqual(s.pool.bytes(), bytes); use.release(); use.release();
});

test('Factory Runtime: 같은 대여 재연결의 오래된 반납은 새 연결을 지우지 않는다', () => {
    const s = scene(); let pins = 0; const r = runtime(s, { pin: () => { pins += 1; } });
    const first = s.factory.bindRuntime(r); const second = s.factory.bindRuntime(r); first.release();
    s.factory.take(3, 15100); assert.equal(pins, 1); second.release();
    s.factory.take(3, 15101); assert.equal(pins, 1);
});

test('Factory Runtime: 장기 Unpop 조회는 선택된 자산+48에서만 호출한다', () => {
    const s = scene(); let reads = 0;
    assert.throws(() => s.factory.bindUnpop(null), /조회기/);
    s.factory.bindUnpop(() => { reads += 1; return s.unpop; });
    s.factory.take(3, 15100); assert.equal(reads, 0);
    const asset = s.factory.create(74, 0); const free = s.pool.freeCount;
    s.factory.take(74, asset); assert.deepEqual([reads, s.pool.freeCount], [1, free]);
});

test('Factory Runtime: Take는 진단이 대여를 교체해도 처음 선택한 공간 부모를 유지한다', () => {
    const s = scene(); const target = s.factory.create(74, 0); s.pool.physical(target)[10] = 75;
    const calls = []; const oldUnpop = { pool: s.pool, unpop: (...args) => { calls.push('old'); s.unpop.unpop(...args); } };
    const nextUnpop = { pool: s.pool, unpop: (...args) => { calls.push('new'); s.unpop.unpop(...args); } };
    const old = runtime(s, { unpop: oldUnpop, pin: () => { calls.push('pin'); return { current: true }; } });
    const use = s.factory.bindRuntime(old); let replacement;
    s.factory.bindDiagnostics({ types: s.types, detail: () => { use.release(); replacement = runtime(s, { unpop: nextUnpop }); s.factory.bindRuntime(replacement); }, assertion: () => calls.push('assert') });
    s.factory.take(74, target); assert.deepEqual(calls, ['pin', 'assert', 'old']);
    s.factory.take(74, target); assert.deepEqual(calls, ['pin', 'assert', 'old', 'new']);
});

test('Factory Runtime: Take pin 실패는 범위 진단/기존 객체 쓰기보다 먼저 중단한다', () => {
    const s = scene(); const bytes = s.pool.bytes(); const error = new Error('pin 중단');
    const r = runtime(s, { pin: () => { throw error; } }); s.factory.bindRuntime(r);
    assert.throws(() => s.factory.take(3, 0), (actual) => actual === error);
    assert.deepEqual(s.pool.bytes(), bytes);
});

test('Factory Runtime: 실제 Pop 전송은 현재 대여/명시 인수/반납 뒤 기본 장치를 선택한다', () => {
    const s = scene(); const depth = { value: 7 }; s.factory.bindFormPopDepth(depth); const calls = [];
    s.factory.bindFormAttach(null, (sid, flags) => calls.push(['default', sid, flags, depth.value]));
    const r = runtime(s, { pin: () => calls.push('pin'), formTransmit: (sid, flags) => calls.push(['current', sid, flags, depth.value]) });
    const use = s.factory.bindRuntime(r); const one = s.factory.createGumpForm(0); s.factory.popForm(one, 5);
    const two = s.factory.createGumpForm(0); s.factory.popForm(two, 9, (sid, flags) => calls.push(['explicit', sid, flags, depth.value]));
    use.release(); const three = s.factory.createGumpForm(0); s.factory.popForm(three, 5);
    assert.deepEqual(calls, ['pin', ['current', one, 13, 7], 'pin', ['explicit', two, 9, 7], ['default', three, 13, 7]]);
    assert.equal(depth.value, 7);
});

test('Factory Runtime: Attach는 현재 contentType을 선택하고 원문 링크 쓰기 뒤 진단한다', () => {
    const s = scene(); const lines = []; const calls = [];
    s.factory.bindFormAttach(() => 69, forbidden);
    s.factory.bindDiagnostics({ types: s.types, assertion: (expression, file, line) => lines.push(line) });
    const r = runtime(s, { contentType: () => { calls.push('content'); return 3; }, pin: () => calls.push('pin') });
    const use = s.factory.bindRuntime(r); const { parent, form } = attached(s);
    assert.deepEqual([lines, calls, commonContainedParent(s.pool, form), get(s.pool.slot(parent), 6, 2)], [[0x3e0], ['pin', 'content'], parent, form]);
    use.release(); const second = attached(s); assert.deepEqual(lines, [0x3e0]);
    assert.equal(commonContainedParent(s.pool, second.form), second.parent);
});

test('Factory Runtime: Unpop의 pin 중 반납은 선택한 분리를 유지하며 다음 호출부터 기본 분리다', () => {
    const s = scene(); const first = attached(s); const second = attached(s); const calls = [];
    s.factory.bindFormDetach((sid) => { calls.push(['default', sid, s.pool.slot(sid)[11]]); detach(s, sid); });
    let use; const r = runtime(s, { pin: () => { calls.push('pin'); use.release(); }, formDetach: (sid) => { calls.push(['current', sid, s.pool.slot(sid)[11]]); detach(s, sid); } });
    use = s.factory.bindRuntime(r); s.factory.unpopForm(first.form); s.factory.unpopForm(second.form);
    assert.deepEqual(calls, ['pin', ['current', first.form, 12], ['default', second.form, 12]]);
    assert.equal(get(s.pool.slot(first.parent), 6, 2), 0); assert.equal(get(s.pool.slot(second.parent), 6, 2), 0);
});

test('Factory Runtime: Form Take는 진단 뒤 현재 UnpopForm 대여를 새로 조회한다', () => {
    const s = scene(); const { parent, form } = attached(s); s.pool.physical(form)[10] = 7;
    s.types[7].constructorAddress = s.types[3].constructorAddress; const calls = []; let replacement;
    const first = runtime(s, { pin: () => calls.push('outer') }); const use = s.factory.bindRuntime(first);
    s.factory.bindDiagnostics({ types: s.types, detail: () => {
        use.release(); replacement = runtime(s, { pin: () => calls.push('inner'), formDetach: (sid) => { calls.push('detach'); detach(s, sid); } }); s.factory.bindRuntime(replacement);
    }, assertion: () => calls.push('assert') });
    s.factory.take(3, form); assert.deepEqual(calls, ['outer', 'assert', 'assert', 'inner', 'detach']);
    assert.deepEqual([s.pool.slot(form)[10], s.pool.slot(form)[11], get(s.pool.slot(parent), 6, 2)], [3, 4, 0]);
});

test('Factory Runtime: Form 진입 세 곳의 pin 예외는 어떤 prefix도 쓰지 않는다', () => {
    const s = scene(); const { parent, form } = attached(s); const bytes = s.pool.bytes(); const error = new Error('Form pin 중단');
    const r = runtime(s, { pin: () => { throw error; } }); s.factory.bindRuntime(r);
    // 세 진입점 모두 pin이 첫 효과인지 대조한다.
    for (const call of [() => s.factory.popForm(form, 5), () => s.factory.attachForm(form, parent, 0x10), () => s.factory.unpopForm(form)]) {
        assert.throws(call, (actual) => actual === error); assert.deepEqual(s.pool.bytes(), bytes);
    }
});

test('Factory Runtime: 실제 World 종료 뒤 같은 장기 공장의 기본 공간 부모로 돌아간다', () => {
    const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database);
    const o = new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList() });
    const factory = o.core.factory; const target = o.world.create(75, 0); factory.take(75, target); o.dispose();
    assert.equal(factory, o.world.storage.factory);
    assert.equal(factory.take(75, target), target);
});
