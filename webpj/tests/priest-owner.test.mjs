// 사제 소유자 raw-priest-owner.js(RawPriestOwner·PriestOwnerState·makePriestOwnerDispatch)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestowner-x86.tsv 10.78(originals) 3528행(모드·로딩·요청·원래 단어·현재 소유자·상태/extra → 알림/공통 지정 호출 순서와 슬롯 전체)과
//    priest-owner-current-local-x86.tsv 10.78 72행(진단·알림이 바꾼 현재 모드/WORD와 공통 부모 중단·TYPE/free 변경 뒤 후단의 현재 값 재조회). CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: priest_owner_rejects_missing_hooks_and_keeps_original_partial_effects·priest_owner_real_factory_load_and_battle_ownership_sequence·priest_owner_dispatch_composes_with_island_and_common_owners 의 10.78 부분.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E62: 사제 소유자 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { BridgeConnectState, makeOwnerDispatch } from '../www/core/raw-bridge-connect.js';
import { RawPriestOwner, PriestOwnerState, makePriestOwnerDispatch, PRIEST_TYPE, PRIEST_VTABLE } from '../www/core/raw-priest-owner.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E62)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const ownerRows = readRows(path.join(FIXTURES, 'priestowner-x86.tsv'))?.filter((row) => row[1] === 'originals') ?? null;
const currentRows = readRows(path.join(FIXTURES, 'priest-owner-current-local-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 관찰 SID, 판본당 관찰 행 수, 공통 가상 표(관찰용)이다. (2026-11-08 WB04-E62)
const SOURCE = 50;
const PATCH_ROWS = 3528;
const COMMON_VTABLE = 0x11010000;

// 예외가 나는지 본다. (2026-11-08 WB04-E62)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E62)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 관찰 SID 50 을 준비하고 실제 공통 소유자 지정에 사제 genus 를 공급한 장면이다. (2026-11-08 WB04-E62)
function priestScene() {
    const pool = new SidPool(24000, { server: false });
    // client 영역의 앞 번호를 차례대로 확보한다.
    for (let id = 5; id <= SOURCE; id += 1) assert.equal(pool.allocate(2), id);
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
    types[PRIEST_TYPE].flags2 = 0x00210000;
    const mode = new SquidOwnerMode();
    const bookkeeping = new SquidPostPopState();
    const scene = { pool, types, mode, state: new PriestOwnerState(), bookkeeping, base: new SquidOwner(pool, types, bookkeeping, mode), events: [] };
    // 현재 단어/소유자 필드를 관찰 순서로 기록한다.
    scene.fields = (sid) => `${get(pool.slot(sid), 12, 2)}:${pool.slot(sid)[34]}`;
    // 입력 행을 raw 슬롯/모드에 넣는다. 기대 결과나 호출 여부는 계산하지 않는다.
    scene.prepare = (row) => {
        scene.events = [];
        mode.fort = Number(row[2]) !== 0;
        mode.battle = Number(row[3]) !== 0;
        mode.challenge = false;
        scene.state.loadingDepth = Number(row[4]);
        scene.state.fortPlayer = Number(row[5]);
        scene.state.battlePlayer = Number(row[6]);
        const raw = pool.allocated(SOURCE);
        raw.fill(0);
        put(raw, 0, PRIEST_VTABLE);
        raw[10] = PRIEST_TYPE;
        raw[11] = Number(row[10]);
        put(raw, 12, Number(row[7]), 2);
        raw[34] = Number(row[8]);
        raw[40] = Number(row[11]);
        put(raw, 14, 0x41a60000);
        put(raw, 18, 0x41af3333);
    };
    // 공통 몸체는 실제 SquidOwner.set 을 사용하고 무상태 알림만 호출 당시 필드를 기록한다.
    scene.hooks = () => ({
        base: (sid, player) => { scene.events.push(`B:${sid}:${player}:${scene.fields(sid)}`); scene.base.set(sid, player); },
        notify: (sid) => scene.events.push(`N:${sid}:${scene.fields(sid)}`)
    });
    scene.eventText = () => (scene.events.length === 0 ? '-' : scene.events.join(';'));
    return scene;
}

test('PriestOwner: 원본 10.78 3528행의 알림/공통 지정 호출 순서와 슬롯 전체가 같다', { skip: !ownerRows && '원본 fixture 없음' }, () => {
    const scene = priestScene();
    const priest = new RawPriestOwner(scene.pool, scene.mode, scene.state, scene.hooks());
    let count = 0;
    // 모드·로딩·요청·원래 단어·현재 소유자·상태/extra 의 모든 관찰을 재생한다.
    for (const row of ownerRows) {
        assert.ok(row.length === 14 && row[0] === 'Owner');
        scene.prepare(row);
        priest.set(SOURCE, Number(row[9]));
        assert.equal(scene.eventText(), row[12], `행 ${count}`);
        assert.equal(hex(scene.pool.slot(SOURCE)), row[13], `행 ${count}`);
        count += 1;
    }
    assert.equal(count, PATCH_ROWS);
    assert.ok(scene.bookkeeping.providers.count === 0 && scene.bookkeeping.factories.count === 0 && scene.bookkeeping.totalCost === 0);
});

test('PriestOwnerCurrent: 진단·알림이 바꾼 현재 모드/WORD 와 공통 부모 중단·TYPE/free 변경이 후단에 반영된다(10.78 72행)', { skip: !currentRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 실제 게임/통신/포획 없이 사제 부모만 실행한다.
    for (const row of currentRows) {
        assert.equal(row.length, 6);
        const args = row[1].split(',').map(Number);
        assert.equal(args.length, 4);
        const [battle, loading, player, profile] = args;
        const pool = new SidPool(32768, { server: true });
        const raw = pool.physical(5);
        // Native 와 같은 입력만 구성하며 기대 결과는 읽기만 한다.
        for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 29 + 7) & 0xff;
        put(raw, 0, PRIEST_VTABLE);
        raw[10] = 158;
        raw[11] = 4;
        put(raw, 12, 0xab83, 2);
        raw[34] = 3;
        const mode = new SquidOwnerMode();
        mode.fort = true;
        mode.battle = battle !== 0;
        const state = new PriestOwnerState({ loadingDepth: loading, fortPlayer: 2, battlePlayer: 3 });
        const events = [];
        let cause = 'returned';
        let notices = 0;
        const event = (value) => events.push(value);
        const hooks = {
            assertion: (expression, file, line) => {
                event(`D${expression}|${file}|${line}`);
                if (profile === 1) {
                    mode.battle = true;
                    mode.fort = false;
                    state.loadingDepth = 0;
                    state.battlePlayer = 0;
                    put(raw, 12, 0xcc81, 2);
                }
            },
            notify: (sid) => {
                event(`N${sid},${get(raw, 12, 2)},${raw[34]}`);
                notices += 1;
                if (profile === 4) throw new Error('notify');
                if (profile === 2 && notices === 1) {
                    mode.battle = false;
                    mode.fort = false;
                    state.loadingDepth = 1;
                    put(raw, 12, 0xbb07, 2);
                }
            },
            base: (sid, owner) => {
                event(`B${owner},${get(raw, 12, 2)},${raw[34]}`);
                if (profile === 5) throw new Error('base');
                raw[34] = owner;
                if (profile === 3) {
                    raw[11] |= 1;
                    raw[10] = 255;
                    put(raw, 12, 0xcc81, 2);
                    mode.battle = false;
                    mode.fort = true;
                    state.loadingDepth = 0;
                    state.fortPlayer = 2;
                }
            }
        };
        try { new RawPriestOwner(pool, mode, state, hooks).set(5, player); } catch (error) { cause = error.message; }
        const current = `${Number(mode.fort)},${Number(mode.battle)},${state.loadingDepth},${state.fortPlayer},${state.battlePlayer}`;
        assert.equal(cause, row[2], `행 ${count}`);
        assert.equal(events.length === 0 ? '-' : events.join(';'), row[3], `행 ${count}`);
        assert.equal(current, row[4], `행 ${count}`);
        assert.equal(hex(raw), row[5], `행 ${count}`);
        count += 1;
    }
    assert.equal(count, 72);
});

test('PriestOwner: 필수 장치/외국 VT 는 거부하고 요청 0 진단 위치에서 앞선 WORD/알림 효과를 보존한다', () => {
    const scene = priestScene();
    const row = ['Owner', '', '1', '0', '1', '3', '1', '43781', '2', '3', '0', '0', '-', '-'];
    scene.prepare(row);
    assert.ok(throws(() => new RawPriestOwner(scene.pool, scene.mode, scene.state, {})));
    assert.ok(throws(() => new RawPriestOwner(scene.pool, scene.mode, scene.state, { ...scene.hooks(), notify: undefined })));
    const priest = new RawPriestOwner(scene.pool, scene.mode, scene.state, scene.hooks());
    // 실패가 전체 raw 단어/소유자와 호출 기록을 보존하는지 확인한다.
    const reject = (sid, player) => {
        const before = hex(scene.pool.slot(SOURCE));
        assert.ok(throws(() => priest.set(sid, player)));
        assert.ok(scene.events.length === 0 && hex(scene.pool.slot(SOURCE)) === before);
    };
    reject(SOURCE, 0);
    reject(0, 3);
    reject(51, 3);
    // 가상 몸체는 현재 TYPE 를 사제 번호로 선행 강제하지 않는다.
    scene.pool.allocated(SOURCE)[10] = 82;
    priest.set(SOURCE, 3);
    assert.ok(scene.pool.slot(SOURCE)[10] === 82 && scene.pool.slot(SOURCE)[34] === 3);
    scene.prepare(row);
    put(scene.pool.allocated(SOURCE), 0, COMMON_VTABLE);
    reject(SOURCE, 3);
    put(scene.pool.allocated(SOURCE), 0, PRIEST_VTABLE);
    // challenge 혼합 사례는 공통 진단 이전의 실제 부모 진입을 남긴다.
    scene.mode.battle = true;
    scene.mode.challenge = true;
    scene.state.loadingDepth = 0;
    put(scene.pool.allocated(SOURCE), 12, 0, 2);
    const before = hex(scene.pool.slot(SOURCE));
    assert.ok(throws(() => priest.set(SOURCE, 3)));
    assert.ok(hex(scene.pool.slot(SOURCE)) === before && scene.events.length === 1 && scene.events[0].startsWith('B:'));
    assert.ok(throws(() => makePriestOwnerDispatch(priest, undefined)));
});

test('PriestOwner: 실제 사제 생성자·공통 소유자와 순차 로딩/전투 상태 전환을 연결한다', () => {
    const pool = new SidPool(24000, { server: true });
    const bookkeeping = new SquidPostPopState();
    const mode = new SquidOwnerMode();
    mode.battle = true;
    const state = new PriestOwnerState({ loadingDepth: 1, fortPlayer: 3, battlePlayer: 1 });
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
    types[PRIEST_TYPE].constructorAddress = typeConstructorAddress(PRIEST_TYPE);
    types[PRIEST_TYPE].flags2 = 0x00210000;
    const factory = new SquidFactory(pool, types);
    const base = new SquidOwner(pool, types, bookkeeping, mode);
    const sid = factory.create(PRIEST_TYPE);
    put(pool.allocated(sid), 12, 0xab00, 2);
    const order = [];
    const priest = new RawPriestOwner(pool, mode, state, {
        base: (object, player) => { order.push('base'); base.set(object, player); },
        notify: (object) => { assert.equal(object, sid); order.push('notify'); }
    });
    assert.ok(priest.handles(sid));
    priest.set(sid, 3);
    assert.ok(pool.slot(sid)[34] === 3 && get(pool.slot(sid), 12, 2) === 0xab03);
    assert.deepEqual(order, ['notify', 'base', 'notify']);
    // 로딩이 끝난 전투에서는 요청값 1 을 무시하고 원래 소유자 3 을 유지한다.
    state.loadingDepth = 0;
    order.length = 0;
    priest.set(sid, 1);
    assert.ok(pool.slot(sid)[34] === 3 && get(pool.slot(sid), 12, 2) === 0xab03);
    assert.deepEqual(order, ['base', 'notify']);
    // 현재 전투 플레이어가 원래 소유자일 때만 표식 비트 7 을 켠다.
    state.battlePlayer = 3;
    order.length = 0;
    priest.set(sid, 0xffffffff);
    assert.ok(pool.slot(sid)[34] === 3 && get(pool.slot(sid), 12, 2) === 0xab83);
    assert.deepEqual(order, ['base', 'notify']);
    // 로딩 중 비전투는 원래 소유자만 바꾸고 표식 비트는 그대로 둔다.
    mode.battle = false;
    mode.fort = true;
    state.loadingDepth = 1;
    order.length = 0;
    priest.set(sid, 1);
    assert.ok(pool.slot(sid)[34] === 1 && get(pool.slot(sid), 12, 2) === 0xab81);
    assert.deepEqual(order, ['notify', 'base']);
    state.loadingDepth = 0;
    order.length = 0;
    priest.set(sid, 3);
    assert.ok(pool.slot(sid)[34] === 3 && get(pool.slot(sid), 12, 2) === 0xab83);
    assert.deepEqual(order, ['notify', 'base', 'notify']);
});

test('PriestOwner: 사제 분배기는 섬/종유석 분배기와 공통 소유자 fallback 을 합성한다', () => {
    const scene = priestScene();
    const links = new BridgeConnectState();
    links.battle = true;
    scene.prepare(['Owner', '', '0', '1', '0', '3', '1', '43779', '2', '1', '0', '0', '-', '-']);
    const priest = new RawPriestOwner(scene.pool, scene.mode, scene.state, scene.hooks());
    const dispatch = makeOwnerDispatch(scene.pool, links, makePriestOwnerDispatch(priest, (sid, owner) => scene.base.set(sid, owner)));
    dispatch(SOURCE, 1);
    assert.ok(scene.pool.slot(SOURCE)[34] === 3 && scene.events.length === 2);
    // 공통 가상 표는 원래 단어를 바꾸지 않고 요청한 소유자를 그대로 받는다.
    scene.events = [];
    put(scene.pool.allocated(SOURCE), 0, COMMON_VTABLE);
    dispatch(SOURCE, 1);
    assert.ok(scene.pool.slot(SOURCE)[34] === 1 && get(scene.pool.slot(SOURCE), 12, 2) === 0xab03 && scene.events.length === 0);
    // 섬 전용 가상 표는 원래 단어를 보존하고 소유자 색 프레임을 바꾼다.
    put(scene.pool.allocated(SOURCE), 0, 0x506d98);
    dispatch(SOURCE, 3);
    assert.ok(scene.pool.slot(SOURCE)[34] === 3 && get(scene.pool.slot(SOURCE), 36) === 2 && get(scene.pool.slot(SOURCE), 12, 2) === 0xab03);
});
