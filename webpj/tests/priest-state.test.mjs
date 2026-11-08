// 사제 상태 raw-priest-state.js(RawPriestState·PriestHitPointMode)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: prieststate-x86.tsv 10.78(originals) 6752행(Ground 2720·Wrapper 2720·Set 1312: 최대 HP·1/4 모드·genus·현재 HP·extra·spot·좌표 → 이동 불가 반환 또는 HP setter 의 Unpop/Repop 기록과 슬롯 50바이트).
//    CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: priest_state_short_circuits_unused_coordinates_and_preserves_raw·priest_state_setter_rejects_invalid_bindings_before_hp_write·PriestState_SharedAuthority*·
//    PriestState_SharedPlayerRecords*(HP setter 1312행을 값형 배열과 현재 Player 9 기록으로 다시 재생, 원문 전체 DWORD 와 Unpop 전 결정, 사용하지 않는 경로의 기록 폭 비검사)의 10.78 부분.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E62: 사제 상태 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { PRIEST_TYPE, PRIEST_VTABLE } from '../www/core/raw-priest-owner.js';
import { RawPriestState, PriestHitPointMode } from '../www/core/raw-priest-state.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E62)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'prieststate-x86.tsv'))?.filter((row) => row[1] === 'originals') ?? null;

// 관찰 SID, 공통 가상 표(비사제), 패치 Player 기록의 폭과 신전 필드 위치이다. (2026-11-08 WB04-E62)
const SOURCE = 50;
const COMMON_VTABLE = 0x11010000;
const STRIDE = 0xb4;
const FIELD = 0x7c;

// 예외가 나는지 본다. (2026-11-08 WB04-E62)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E62)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// client SID 50 을 확보하며 HP 조회만으로 SP 저장소가 초기화되지 않는지도 확인하는 장면이다. (2026-11-08 WB04-E62)
function stateScene() {
    const pool = new SidPool(24000, { server: false });
    // 지정된 원본 관찰 번호까지 실제 풀에 할당한다.
    for (let id = 5; id <= SOURCE; id += 1) assert.equal(pool.allocate(2), id);
    const store = new ScrambledSpStore(new GameRandom(), () => 1);
    const scene = { pool, store, book: new SquidPostPopState(), hpMode: new SquidRewardState(), types: Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 })), spots: new Uint8Array(65536), spotKey: -1, hp: null, state: null, mode: new PriestHitPointMode(), events: '', after: '-' };
    // 입력만 공급한다. 원본 반환값/HP 변경 결과를 준비 코드에서 계산하지 않는다.
    scene.prepare = (row) => {
        assert.equal(row.length, 18);
        scene.events = '';
        scene.after = row[14];
        const type = scene.types[PRIEST_TYPE];
        type.maxHitPoints = Number(row[2]) | 0;
        type.flags1 = 0x69012;
        type.flags2 = Number(row[4]) >>> 0;
        scene.hpMode.priestQuarterHp = Number(row[3]) !== 0;
        scene.hp = new SquidReward(pool, scene.types, scene.book, scene.hpMode, store);
        const spot = Number(row[7]);
        // 체커보드는 두 좌표의 절삭/거의 올림 혼동을 드러내는 입력이다(spot 값이 같으면 다시 채우지 않는다).
        if (scene.spotKey !== spot) {
            for (let y = 0; y < 256; y += 1) for (let x = 0; x < 256; x += 1) scene.spots[y * 256 + x] = spot ^ ((x + y) % 2 ? 6 : 0);
            scene.spotKey = spot;
        }
        const raw = pool.allocated(SOURCE);
        raw.fill(0xab);
        put(raw, 0, PRIEST_VTABLE);
        raw[10] = PRIEST_TYPE;
        raw[11] = 0;
        raw[40] = Number(row[6]);
        raw[34] = Number(row[13]);
        put(raw, 14, Number(row[8]));
        put(raw, 18, Number(row[9]));
        put(raw, 26, Number(row[5]));
        scene.mode.authority = Number(row[11]) !== 0;
        scene.mode.recoveryAllowed.fill(false);
        scene.mode.recoveryAllowed[Number(row[13])] = Number(row[12]) !== 0;
        scene.state = new RawPriestState(pool, scene.hp, scene.types, scene.spots);
    };
    // 원본의 두 가상 공간 대체처럼 호출 시점의 HP/좌표를 기록하고 Unpop 변경 입력을 적용한다.
    scene.record = (marker, sid, flags) => {
        const raw = pool.slot(sid);
        scene.events += `${scene.events ? ';' : ''}${marker}:${sid}:${flags}:${scene.state.currentHitPoints(sid)}:${get(raw, 14)}:${get(raw, 18)}`;
        if (marker === 'U' && scene.after !== '-') {
            const target = pool.allocated(sid);
            put(target, 26, Number(scene.after) >>> 0);
            put(target, 14, floatToBits(30.75));
            put(target, 18, floatToBits(31.9));
        }
    };
    scene.hooks = () => ({ unpop: (sid, flags) => scene.record('U', sid, flags), repop: (sid, flags) => scene.record('R', sid, flags) });
    return scene;
}

test('PriestState: 원본 10.78 6752행(이동 불가 조회·HP setter)의 반환/사건/슬롯이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = stateScene();
    let count = 0;
    // 각 입력은 이전 HP 변경의 영향을 받지 않는다.
    for (const row of rows) {
        scene.prepare(row);
        if (row[0] === 'Set') scene.state.setHitPoints(SOURCE, Number(row[10]) | 0, scene.mode, scene.hooks());
        else assert.equal(row[0] === 'Ground' ? scene.state.groundImmobile(SOURCE) : scene.state.immobile(SOURCE), row[15] === '1', `행 ${count}`);
        assert.equal(scene.events === '' ? '-' : scene.events, row[16], `행 ${count}`);
        assert.equal(hex(scene.pool.slot(SOURCE)), row[17], `행 ${count}`);
        count += 1;
    }
    assert.equal(count, 6752);
    assert.ok(!scene.store.initialized);
});

test('PriestState: 강제 비트/비사제 genus/저HP 는 나쁜 좌표를 조회하지 않고 필요한 지면 경로만 거부한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = stateScene();
    const row = [...rows[0]];
    row[2] = '200';
    row[3] = '0';
    row[4] = '2162688';
    row[5] = '20';
    row[6] = '0';
    row[8] = String(0x7fc00000);
    scene.prepare(row);
    const before = hex(scene.pool.slot(SOURCE));
    assert.ok(scene.state.groundImmobile(SOURCE));
    assert.equal(hex(scene.pool.slot(SOURCE)), before);
    row[5] = '200';
    row[6] = '32';
    scene.prepare(row);
    assert.ok(scene.state.immobile(SOURCE));
    row[6] = '0';
    scene.prepare(row);
    assert.ok(throws(() => scene.state.groundImmobile(SOURCE)));
    row[4] = '0';
    scene.prepare(row);
    assert.ok(!scene.state.groundImmobile(SOURCE));
    assert.ok(throws(() => scene.state.immobile(0)));
    assert.ok(throws(() => new RawPriestState(scene.pool, scene.hp, scene.types, new Uint8Array(0))));
    assert.ok(throws(() => scene.state.bindCurrentTypes(scene.types.slice(0, 10))), '잘못된 대여는 기존 연결을 바꾸기 전에 거부');
});

test('PriestState: 전환에 필요한 효과/소유자/가상 표가 빠지면 HP 쓰기 전에 거부하며 전환 없는 변경은 효과를 요구하지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = stateScene();
    const row = [...rows[0]];
    row[2] = '200';
    row[3] = '0';
    row[4] = '2162688';
    row[5] = '150';
    scene.prepare(row);
    scene.mode.authority = true;
    const before = hex(scene.pool.slot(SOURCE));
    assert.ok(throws(() => scene.state.setHitPoints(SOURCE, 20, scene.mode, {})));
    assert.equal(hex(scene.pool.slot(SOURCE)), before);
    scene.state.setHitPoints(SOURCE, 140, scene.mode, {});
    assert.equal(scene.state.currentHitPoints(SOURCE), 140);
    const raw = scene.pool.allocated(SOURCE);
    put(raw, 26, 20);
    raw[34] = 255;
    const invalidOwner = hex(raw);
    assert.ok(throws(() => scene.state.setHitPoints(SOURCE, 150, scene.mode, {})));
    assert.equal(hex(raw), invalidOwner);
    raw[34] = 1;
    put(raw, 0, COMMON_VTABLE);
    const invalidVtable = hex(raw);
    assert.ok(throws(() => scene.state.setHitPoints(SOURCE, 30, scene.mode, {})));
    assert.equal(hex(raw), invalidVtable);
});

test('PriestState: 복사된 HP 모드도 원본 boss 주소를 다시 읽고 권한 없는 절반 HP 전환과 실제 공간 전환을 구분한다', () => {
    const scene = stateScene();
    const row = new Array(18).fill('0');
    row[2] = '200';
    row[5] = '200';
    row[7] = '4';
    row[8] = String(floatToBits(30));
    row[9] = String(floatToBits(31));
    row[11] = '1';
    row[12] = '1';
    row[13] = '1';
    row[14] = '-';
    scene.prepare(row);
    let authority = false;
    scene.mode.sharedAuthority = () => authority;
    const copied = Object.assign(new PriestHitPointMode(), scene.mode);
    let unpops = 0;
    let repops = 0;
    const hooks = { unpop: (sid, flags) => { unpops += 1; scene.record('U', sid, flags); }, repop: (sid, flags) => { repops += 1; scene.record('P', sid, flags); } };
    scene.state.setHitPoints(SOURCE, 1, copied, hooks);
    assert.ok(scene.state.currentHitPoints(SOURCE) === 100 && unpops === 0 && repops === 0);
    authority = true;
    scene.mode.authority = false;
    scene.state.setHitPoints(SOURCE, 1, copied, hooks);
    assert.ok(scene.state.currentHitPoints(SOURCE) === 1 && unpops === 1 && repops === 1 && copied.sharedAuthority !== null);
});

test('PriestState: 원본 HP setter 1312행을 값형 배열과 현재 Player 9 기록으로 다시 재생해도 같고 기록은 변하지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = stateScene();
    const players = new Uint8Array(9 * STRIDE);
    let count = 0;
    // 기대 HP·전체 raw·공간 호출 순서는 저장된 원본 관찰에서만 읽는다.
    for (const row of rows) {
        if (row[0] !== 'Set') continue;
        scene.prepare(row);
        players.fill(0xa5);
        const owner = Number(row[13]);
        put(players, owner * STRIDE + FIELD, Number(row[12]));
        scene.mode.recoveryAllowed.fill(Number(row[12]) === 0);
        scene.mode.playerRecords = players;
        const before = players.slice();
        scene.state.setHitPoints(SOURCE, Number(row[10]) | 0, scene.mode, scene.hooks());
        assert.equal(scene.events === '' ? '-' : scene.events, row[16], `행 ${count}`);
        assert.equal(hex(scene.pool.slot(SOURCE)), row[17], `행 ${count}`);
        assert.deepEqual(players, before);
        count += 1;
    }
    assert.equal(count, 1312);
    assert.ok(!scene.store.initialized);
});

test('PriestState: 중립·첫·마지막 owner 와 하위 WORD 가 0 인 신전 DWORD 도 원문대로 현재 전체 값을 읽는다', () => {
    // 원문은 owner0 도 허용하며 owner8 뒤를 읽지 않는다.
    for (const owner of [0, 1, 8]) {
        const scene = stateScene();
        const row = new Array(18).fill('0');
        row[2] = '200';
        row[5] = '20';
        row[7] = '6';
        row[8] = String(floatToBits(20));
        row[9] = String(floatToBits(21));
        row[11] = '1';
        row[13] = String(owner);
        row[14] = '-';
        scene.prepare(row);
        const players = new Uint8Array(9 * STRIDE).fill(0xa5);
        put(players, owner * STRIDE + FIELD, 0);
        scene.mode.playerRecords = players;
        const copied = Object.assign(new PriestHitPointMode(), scene.mode);
        copied.recoveryAllowed.fill(true);
        let spaces = 0;
        const hooks = { unpop: () => { spaces += 1; }, repop: () => { spaces += 1; } };
        scene.state.setHitPoints(SOURCE, 150, copied, hooks);
        assert.ok(scene.state.currentHitPoints(SOURCE) === 20 && spaces === 0);
        put(players, owner * STRIDE + FIELD, 0x10000);
        copied.recoveryAllowed.fill(false);
        const before = players.slice();
        scene.state.setHitPoints(SOURCE, 150, copied, hooks);
        assert.ok(scene.state.currentHitPoints(SOURCE) === 150 && spaces === 2 && players.every((value, index) => value === before[index]) && copied.playerRecords === players);
        scene.state.setHitPoints(SOURCE, 20, copied, hooks);
        put(players, owner * STRIDE + FIELD, 0);
        copied.recoveryAllowed.fill(true);
        scene.state.setHitPoints(SOURCE, 150, copied, hooks);
        assert.ok(scene.state.currentHitPoints(SOURCE) === 20 && spaces === 4);
    }
});

test('PriestState: 권한 없는 전환·저HP 로의 전환·전환 없음은 Player 허용 필드를 조회하지 않고 잘못된 기록 폭은 필요한 경로에서만 거부한다', () => {
    const scene = stateScene();
    const row = new Array(18).fill('0');
    row[2] = '200';
    row[5] = '200';
    row[7] = '6';
    row[8] = String(floatToBits(20));
    row[9] = String(floatToBits(21));
    row[11] = '1';
    row[13] = '1';
    row[14] = '-';
    scene.prepare(row);
    scene.mode.playerRecords = new Uint8Array(1);
    const hooks = { unpop: () => {}, repop: () => {} };
    scene.state.setHitPoints(SOURCE, 150, scene.mode, {});
    scene.state.setHitPoints(SOURCE, 20, scene.mode, hooks);
    const before = hex(scene.pool.slot(SOURCE));
    assert.ok(throws(() => scene.state.setHitPoints(SOURCE, 150, scene.mode, hooks)) && hex(scene.pool.slot(SOURCE)) === before);
    scene.mode.authority = false;
    scene.state.setHitPoints(SOURCE, 150, scene.mode, {});
    assert.equal(scene.state.currentHitPoints(SOURCE), 99);
    scene.mode.playerRecords = new Uint8Array(0);
    scene.mode.authority = true;
    scene.mode.recoveryAllowed[1] = true;
    scene.state.setHitPoints(SOURCE, 150, scene.mode, hooks);
    assert.ok(scene.state.currentHitPoints(SOURCE) === 150 && throws(() => scene.mode.isRecoveryAllowed(9)));
});

test('PriestState: 회복 허용은 Unpop 전에 결정되므로 Unpop 이 신전 필드를 지워도 진행 중인 Repop 은 수행하고 다음 회복부터 새 값을 반영한다', () => {
    const scene = stateScene();
    const row = new Array(18).fill('0');
    row[2] = '200';
    row[5] = '20';
    row[7] = '6';
    row[8] = String(floatToBits(20));
    row[9] = String(floatToBits(21));
    row[11] = '1';
    row[13] = '1';
    row[14] = '-';
    scene.prepare(row);
    const players = new Uint8Array(9 * STRIDE);
    put(players, STRIDE + FIELD, 0x80000000);
    scene.mode.playerRecords = players;
    let unpops = 0;
    let repops = 0;
    const hooks = { unpop: () => { unpops += 1; put(players, STRIDE + FIELD, 0); }, repop: () => { repops += 1; } };
    scene.state.setHitPoints(SOURCE, 150, scene.mode, hooks);
    assert.ok(scene.state.currentHitPoints(SOURCE) === 150 && unpops === 1 && repops === 1 && get(players, STRIDE + FIELD) === 0);
    scene.state.setHitPoints(SOURCE, 20, scene.mode, hooks);
    scene.state.setHitPoints(SOURCE, 150, scene.mode, hooks);
    assert.ok(scene.state.currentHitPoints(SOURCE) === 20 && unpops === 2 && repops === 2);
});
