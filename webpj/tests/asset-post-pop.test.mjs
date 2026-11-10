// 초기 요새 자산 후처리 분배 본체 raw-asset-post-pop.js(RawAssetPostPop·makeAssetPostPopFinder)를 원본 10.78 관측과 cpppj `AssetPostPopTests.cpp` 계약으로 대조한다(WB04-E254).
//  - assetpostpop-x86.tsv `originals` 818행: 열두 가상 표(제네레이터·Gun 넷·장식·거주 구역·바리케이드·간헐천·신전 셋)의 사건 순서(A 확보·R Regular·P GunProcess·M 애니메이션 제거·F 각도·Q 그래프·O 소유자·N 표면 통지·
//    K 차단 시작·G 간헐천 등록·U/V Unpop/Repop·E 음향·C 배치 취소·I 섬 등록·X 삭제·S slot 생성·T 생산 갱신·D Damageable)와 슬롯 50 전체·세 목록(꼬리 포함)·Player 1 신전 네 필드·신전 개수·두 이웃 프레임.
//    같은 818행을 독립 호환 진입(activate)·공유 권한(반대 fallback)·반대 전투 표시·공유 Player 9기록·실제 가상 진입(postPopVirtual)으로 다시 재생한다.
//  - asset-player-x86.tsv `originals` 288행: 콜백 뒤 owner 변경과 현재 신전 DWORD 를 Player 9기록 전체 바이트로. gun-postpop-checking-x86.tsv 1,344행: 네 Gun 의 checking·로딩·편집 진단 순서와 복귀.
//    vortex-current-x86.tsv 936행: 신전 진단·WORD·중복 없는 목록과 9기록 Adler. arc-spire·acid-fence 의 P 100행, ice-tower 의 P 212행: 펜스/성장형 아이스 부모 예약.
//  - 계약: 각 몸체가 도달한 장치만 요구하고(없으면 그 위치에서 거부, 앞선 효과 유지) 효과 뒤 현재 raw/전역을 다시 읽는다는 cpppj 의 개별 시험 전부(10.78 분).
// CD 10.72(originalCD) 행과 CD 전용 assert 는 웹 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E254)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { RawAssetPostPop, makeAssetPostPopFinder } from '../www/core/raw-asset-post-pop.js';
import { AssetPostPopState } from '../www/core/raw-asset-post-pop-state.js';
import { CarrierPostPopState, RawCarrierPostPop } from '../www/core/raw-carrier-post-pop.js';
import { postPopGrowingIce } from '../www/core/raw-ice-tower.js';
import { RawNuggetPostPop } from '../www/core/raw-nugget-post-pop.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { REPO_ROOT } from './helpers.mjs';
import { adler32, floatToBits, get, put, readRows } from './raw-support.mjs';

// 실제 +0x20 이 각 시험 몸체로 연결되는 10.78 가상 표다(fixture 의 body 번호 순서). 명령 실행용 주소가 아니다. (2026-11-10 WB04-E254)
const PATCH_ASSET_TABLES = [0x501ea0, 0x50b6e8, 0x50b7c0, 0x50b9b8, 0x50b888, 0x50b570, 0x50f540, 0x50e940, 0x50da88, 0x5196f0, 0x519860, 0x519918];
// 분석 도구의 합성 풀 크기, 대상 SID, Player 한 기록의 크기와 첫 신전 DWORD 위치, 10.78 관측 행 수이다. (2026-11-10 WB04-E254)
const CAPACITY = 24000;
const SOURCE = 50;
const STRIDE = 0xb4;
const TEMPLE = 0x7c;
const ROWS = 818;

// fixture 폴더와 10.78 행 읽기이다. 파일이 없으면 null 이다. (2026-11-10 WB04-E254)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const fixture = (name) => readRows(path.join(FIXTURES, name));
const rows = fixture('assetpostpop-x86.tsv')?.filter((row) => row[0] === 'originals') ?? null;
const skip = { skip: !rows && '원본 fixture 없음' };

// 10진 문자열 DWORD 를 부호 없는 수로, 문자열을 Latin-1 바이트 hex 로 바꾼다. (2026-11-10 WB04-E254)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const textHex = (text) => Buffer.from(text, 'latin1').toString('hex');

// 명시 진단 중단을 다른 오류와 구별하는 예외이다(cpppj 의 runtime_error "명시 … 진단 중단"). (2026-11-10 WB04-E254)
class DiagnosticStop extends Error {}

// 실제 몸체와 같은 외부 효과를 합성하는 검사 월드다(cpppj AssetScene). 판단은 fixture 의 native 결과가 정한다. 풀 번호 5..127 을 client 구역에 확보한다. (2026-11-10 WB04-E254)
class AssetScene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0 }));
        this.baseState = new SquidPostPopState();
        this.carrierState = new CarrierPostPopState();
        this.state = new AssetPostPopState();
        this.events = [];
        this.allocated = 0;
        this.reserveMask = 0;
        this.contentExists = 0;
        // 원본 fixture 의 모든 관찰 번호를 확보한다.
        for (let id = 5; id <= 127; id += 1) assert.equal(this.pool.allocate(2), id);
    }

    // free 변경 뒤에도 같은 물리 바이트를 쓰고 관찰한다(cpppj AssetCurrentSlot).
    slot(sid = SOURCE) { return this.pool.physical(sid); }

    // 새 입력만 준비하며 몸체 결과는 계산하지 않는다.
    prepare(row) {
        assert.equal(row.length, 21);
        this.events = [];
        this.allocated = 0;
        this.state = new AssetPostPopState();
        this.baseState = new SquidPostPopState();
        this.carrierState = new CarrierPostPopState();
        const { state } = this;
        this.baseState.suppressed = true;
        this.carrierState.boss = true;
        state.inBattle = number(row[4]) !== 0;
        state.loadingDepth = number(row[5]);
        state.fortMode = state.loadingDepth !== 0;
        state.boss = number(row[6]) !== 0;
        this.reserveMask = number(row[7]);
        state.residenceAnimation = number(row[8]) !== 0;
        state.templeCount = 4;
        state.selected = number(row[12]) ? 62 : 0;
        this.contentExists = number(row[13]);
        // 목록은 capacity 밖의 과거 값도 유지한다.
        for (const list of [state.residences, state.temples, state.geysers]) {
            list.entries = [7, 8, 9, 10];
            list.count = number(row[14]);
        }
        Object.assign(this.types[70], { flags1: number(row[10]), flags2: 0x200, cost: Math.fround(Number.parseFloat(row[11])) });
        const raw = this.pool.allocated(SOURCE);
        raw.fill(0);
        put(raw, 0, PATCH_ASSET_TABLES[number(row[1])]);
        raw[10] = 70;
        raw[11] = 0;
        raw[34] = 1;
        raw[40] = number(row[3]) & 0xff;
        put(raw, 12, 0xabcd, 2);
        put(raw, 14, 0x42480000);
        put(raw, 18, 0x42500000);
        put(raw, 36, number(row[9]), 4);
        // native 공간 iterator 가 차례로 반환하는 두 실제 지면 번호다.
        for (const sid of [60, 61]) {
            const neighbor = this.pool.allocated(sid);
            neighbor.fill(0);
            neighbor[10] = sid === 60 ? 94 : 95;
            neighbor[34] = 2;
        }
        if (state.boss) {
            const other = this.pool.allocated(61);
            other[10] = 70;
            other[40] = (this.contentExists * 8) & 0xff;
        }
        const selected = this.pool.allocated(62);
        selected.fill(0);
        put(selected, 18, 70, 2);
    }

    // 기대값을 만들지 않고 기존 장면에 명시 일반 입력만 공급한다(cpppj PrepareAssetCurrent).
    prepareCurrent(kind, flags = 1) {
        const row = [...rows[0]];
        Object.assign(row, { 1: String(kind), 2: String(flags), 3: '0', 4: '0', 5: '0', 6: '0', 7: '7', 8: '0', 9: '25', 10: '0', 11: '1000', 12: '0', 13: '0', 14: '0' });
        this.prepare(row);
    }

    // allocator 의 각 호출별 성공 비트다. 반환값은 입력의 확보 여부만 표현한다.
    allocate(size) {
        this.events.push(`A:${size}`);
        const index = this.allocated;
        this.allocated += 1;
        return (this.reserveMask & (1 << (index % 3))) !== 0;
    }

    // 직접 Damageable 몸체의 명시 base 경계를 원본과 같은 사건으로 관찰한다.
    carrierHooks(base) {
        return {
            carrierCheck: () => { throw new Error('이번 자산 몸체는 Carrier 확인을 호출하지 않습니다'); },
            claimGround: () => { throw new Error('이번 관찰은 Damageable 를 외부 경계로 둡니다'); },
            validateBase: (sid, flags) => base.validateBase(sid, flags),
            base: (sid, flags) => { this.events.push(`D:${flags}`); base.postPopBase(sid, flags); },
        };
    }

    // native 가 대체한 효과와 같은 인자/호출 규약으로 사건과 raw 변경을 공급한다.
    hooks() {
        const { pool, events } = this;
        const push = (event) => this.events.push(event);
        void events;
        return {
            reserveRegular: () => this.allocate(40),
            addRegular: (sid, event, delay) => push(`R:${sid}:${event}:${floatToBits(delay)}`),
            createGunProcess: (sid) => { if (this.allocate(56)) push(`P:${sid}`); },
            removeAnimation: (sid) => push(`M:${sid}`),
            checkForms: () => true,
            gunFrame: (angle) => { push(`F:${angle}`); return 2; },
            setGraph: (sid, graph) => { push(`Q:${graph}`); pool.allocated(sid)[30] = graph & 0xff; },
            surface: () => 60,
            setOwner: (sid, owner) => { push(`O:${sid}:${owner}`); pool.allocated(sid)[34] = owner & 0xff; },
            notifySurface: (sid) => push(`N:${sid}`),
            startBlocker: (sid) => push(`K:${sid}`),
            geyserRegistration: (sid, value) => push(`G:${sid}:${value}`),
            atPosition: () => [60, 61],
            unpop: (sid, flags) => push(`U:${sid}:${flags}`),
            repop: (sid, flags) => push(`V:${sid}:${flags}`),
            typeDword: (sid, offset) => (offset === 0x74 ? 17 : offset === 0x130 ? 27 : 39),
            playImpactSound: (value) => push(`E:${value}`),
            clearPlacement: () => push('C'),
            islandNumber: () => 3,
            islandExists: () => true,
            registerTemple: (island, sid) => push(`I:${island}:${sid}`),
            regionNumber: () => 3,
            destroy: (sid, flags) => { push(`X:${sid}:${flags}`); pool.allocated(sid)[11] |= 2; },
            hasContent: () => this.contentExists !== 0,
            createSlot: (sid) => push(`S:${sid}`),
            refreshProduction: () => push('T'),
        };
    }

    // 현재 관찰 결과를 기계어 fixture 형식으로 직렬화한다(사건·슬롯 50·세 목록·Player 1·신전 개수·두 이웃 프레임).
    output() {
        const { state, pool } = this;
        const lists = [state.residences, state.temples, state.geysers].map((list) => `${list.count}:${list.entries.join(',')}`).join('|');
        const player = state.players[1];
        return [
            this.events.join(';'), Buffer.from(pool.slot(SOURCE)).toString('hex'), lists,
            [player.initialTemple, player.initialTempleCopy, player.newTemple, player.newTempleCopy].join(','),
            String(state.templeCount), `${get(pool.slot(60), 36, 4)}|${get(pool.slot(61), 36, 4)}`,
        ];
    }
}

// 같은 장면의 공통·Damageable·가상 분배를 실제 제품 몸체로 구성한다(cpppj AssetCurrentBody). 외부 효과만 입력으로 공급한다. (2026-11-10 WB04-E254)
class CurrentBody {
    constructor(scene, hooks) {
        this.scene = scene;
        this.base = new SquidPostPop(scene.pool, scene.types, scene.baseState);
        this.damageable = new RawCarrierPostPop(scene.pool, scene.types, scene.carrierState, scene.carrierHooks(this.base));
        this.assets = new RawAssetPostPop(scene.pool, scene.types, this.base, this.damageable, scene.state, { ...hooks });
        this.base.setBody('asset', this.assets);
    }

    // 실제 상위가 증가시킨 깊이와 현재 VT+20 을 사용한다.
    run(flags = 1) {
        this.scene.baseState.depth += 1;
        this.base.postPopVirtual(SOURCE, flags);
    }
}

// 훅 묶음에서 지정한 이름을 뺀 사본이다(cpppj 의 `hooks.x={}`). (2026-11-10 WB04-E254)
function without(hooks, ...names) {
    const copy = { ...hooks };
    // 지정한 장치만 미연결로 만든다.
    for (const name of names) delete copy[name];
    return copy;
}

// 독립 호환 진입(activate)으로 원본 10.78 818행을 재생한다. sharedAuthority 면 같은 Client 권한을 빌리고 독립 fallback 은 반대 값으로 둔다. (2026-11-10 WB04-E254)
function replay(sharedAuthority = false, oppositeBattle = false) {
    const authority = { server: false, boss: false };
    const scene = new AssetScene();
    let count = 0;
    // 새 몸체의 정상 반환마다 raw·목록·Player·사건을 비교한다.
    for (const row of rows) {
        scene.prepare(row);
        authority.server = scene.state.inBattle;
        authority.boss = scene.state.boss;
        if (sharedAuthority) {
            scene.state.sharedAuthority = authority;
            scene.state.boss = !authority.boss;
            if (oppositeBattle) scene.state.inBattle = !authority.server;
        }
        const body = new CurrentBody(scene, scene.hooks());
        assert.ok(body.base.handlesDerived(SOURCE));
        body.base.activate(SOURCE, number(row[2]));
        assert.equal(scene.baseState.depth, 0);
        assert.deepEqual(scene.output(), row.slice(15), `행 ${count} 몸체 ${row[1]}`);
        count += 1;
    }
    assert.equal(count, ROWS);
}

test('asset-post-pop: 원본 10.78 818행의 열두 전용 몸체(사건·슬롯·목록·Player·신전 개수·이웃 프레임)가 같다', skip, () => replay());

test('asset-post-pop: 공유 권한은 독립 fallback(반대 boss)과 전투 표시(반대 inBattle)와 무관하게 같은 818행을 낸다', skip, () => {
    replay(true);
    replay(true, true);
});

// 직접 Damageable/Carrier 지원은 등록을 요구하며 모르는 파생 가상 표는 계속 거부한다.
test('asset-post-pop: 직접 Damageable/Carrier 가상 표는 등록된 Carrier 몸체로만 분배되고 모르는 표는 거부한다', skip, () => {
    const scene = new AssetScene();
    scene.prepare(rows[0]);
    const base = new SquidPostPop(scene.pool, scene.types, scene.baseState);
    const hooks = scene.carrierHooks(base);
    let checks = 0;
    scene.carrierState.boss = false;
    hooks.carrierCheck = () => { checks += 1; };
    const carrier = new RawCarrierPostPop(scene.pool, scene.types, scene.carrierState, hooks);
    base.setBody('carrier', carrier);
    put(scene.slot(), 0, 0x50a820);
    assert.ok(base.handlesDamageable(SOURCE));
    base.activate(SOURCE, 0);
    assert.ok(checks === 0 && scene.baseState.depth === 0);
    put(scene.slot(), 0, 0x503d80);
    assert.ok(base.handlesCarrier(SOURCE));
    base.activate(SOURCE, 0);
    assert.ok(checks === 1 && scene.baseState.depth === 0);
    base.setBody('carrier', null);
    assert.ok(!base.handlesCarrier(SOURCE));
    assert.throws(() => base.activate(SOURCE, 0));
    put(scene.slot(), 0, 0x11111111);
    assert.ok(!base.handlesDerived(SOURCE));
    assert.throws(() => base.activate(SOURCE, 0));
});

// 외부 효과 누락은 최초 raw/목록/장부 쓰기 전에 거부한다.
test('asset-post-pop: 명시 사전 검사는 효과 누락을 raw/목록/장부 쓰기 전에 거부한다', skip, () => {
    const scene = new AssetScene();
    scene.prepare(rows[0]);
    put(scene.slot(), 0, PATCH_ASSET_TABLES[9]);
    const body = new CurrentBody(scene, {});
    const before = scene.output();
    assert.throws(() => body.base.activate(SOURCE, 1));
    assert.deepEqual(scene.output(), before);
    assert.equal(scene.baseState.depth, 0);
    assert.ok(RawAssetPostPop.hasDedicatedVtable(0x50da88) && !RawAssetPostPop.hasDedicatedVtable(0x50a820));
    // 다른 풀의 공통 장부/Damageable 과 짧은 TYPE 표는 구성 때 거부한다.
    const other = new AssetScene();
    assert.throws(() => new RawAssetPostPop(other.pool, scene.types, body.base, body.damageable, scene.state, {}), TypeError);
    assert.throws(() => new RawAssetPostPop(scene.pool, scene.types.slice(1), body.base, body.damageable, scene.state, {}), TypeError);
});

// 기존 독립 PE 기대값을 공유 Player BYTE 에 그대로 재생하며 기존 state.players 사본과 구별한다.
test('asset-post-pop: 같은 818행을 공유 Player 9기록에 직접 기록하고 독립 사본은 건드리지 않는다', skip, () => {
    const scene = new AssetScene();
    let count = 0;
    // 원본 입력과 기대값은 바꾸지 않고 모든 자산 몸체를 같은 9기록에 다시 재생한다.
    for (const row of rows) {
        scene.prepare(row);
        Object.assign(scene.state.players[1], { initialTemple: 901, initialTempleCopy: 902, newTemple: 903, newTempleCopy: 904 });
        // 신전 이외의 필드 쓰기를 검출하는 실제 공유 9기록이다. 아홉 기록의 신전 네 DWORD 만 원본 입력과 같은 0 으로 준비한다.
        const records = new Uint8Array(9 * STRIDE).fill(0xa5);
        for (let owner = 0; owner < 9; owner += 1) records.fill(0, owner * STRIDE + TEMPLE, owner * STRIDE + TEMPLE + 16);
        const expectedRecords = records.slice();
        const owner = { value: 1 };
        const body = new CurrentBody(scene, scene.hooks());
        body.assets.bindPlayerRecords(records, () => owner.value);
        body.base.activate(SOURCE, number(row[2]));
        const output = scene.output();
        const fields = row[18].split(',');
        const actual = [];
        // 원본 출력대로 네 DWORD 만 기대 기록에 쓰고 실제 BYTE 결과를 직렬화한다.
        for (let field = 0; field < 4; field += 1) {
            put(expectedRecords, STRIDE + TEMPLE + field * 4, number(fields[field]));
            actual.push(get(records, STRIDE + TEMPLE + field * 4));
        }
        output[3] = actual.join(',');
        assert.deepEqual(output, row.slice(15), `행 ${count}`);
        assert.deepEqual(records, expectedRecords, `행 ${count} 기록`);
        const player = scene.state.players[1];
        assert.ok(player.initialTemple === 901 && player.initialTempleCopy === 902 && player.newTemple === 903 && player.newTempleCopy === 904 && scene.baseState.depth === 0);
        count += 1;
    }
    assert.equal(count, ROWS);
});

// 잘못된 9기록 바인딩은 기존 참조를 바꾸거나 Player 바이트를 쓰기 전에 거부한다.
test('asset-post-pop: Player 9기록 바인딩은 크기가 다르면 거부한다', skip, () => {
    const scene = new AssetScene();
    scene.prepare(rows[0]);
    const body = new CurrentBody(scene, scene.hooks());
    const records = new Uint8Array(9 * STRIDE).fill(0xa5);
    body.assets.bindPlayerRecords(records);
    const before = records.slice();
    assert.throws(() => body.assets.bindPlayerRecords(records.subarray(0, records.length - 1)), TypeError);
    assert.deepEqual(records, before);
    assert.throws(() => body.assets.bindPlayerRecords(new Uint8Array(0)), TypeError);
    assert.throws(() => body.assets.bindPlayerRecords(records, 1), TypeError);
});

// 새 독립 PE 입력의 현재 owner·음향 콜백 뒤 신전 DWORD 를 같은 공유 기록에 재생한다.
test('asset-post-pop: 콜백 뒤 owner 변경과 현재 신전 DWORD 가 원본 10.78 288행의 Player 9기록 전체와 같다', skip, () => {
    const playerRows = fixture('asset-player-x86.tsv').filter((row) => row[0] === 'originals');
    const scene = new AssetScene();
    let count = 0;
    // 현재 raw/전역을 변경하는 외부 경계 뒤의 실제 원본 반환을 전수 비교한다.
    for (const row of playerRows) {
        assert.equal(row.length, 17);
        scene.prepare(rows[0]);
        Object.assign(scene.state, { inBattle: true, loadingDepth: 1, fortMode: true, selected: 62 });
        scene.reserveMask = 0;
        const raw = scene.pool.allocated(SOURCE);
        put(raw, 0, PATCH_ASSET_TABLES[number(row[1])]);
        raw[34] = number(row[4]) & 0xff;
        raw[40] = number(row[3]) & 0xff;
        // 새 Native 입력과 같은 stale BYTE 다. 실제 PE 에 공급한 네 DWORD 입력을 아홉 기록에 각각 준비한다.
        const records = new Uint8Array(9 * STRIDE).fill(0xa5);
        for (let owner = 0; owner < 9; owner += 1) {
            put(records, owner * STRIDE + TEMPLE, number(row[8]));
            put(records, owner * STRIDE + TEMPLE + 4, 0x11112222);
            put(records, owner * STRIDE + TEMPLE + 8, 0x33334444);
            put(records, owner * STRIDE + TEMPLE + 12, 0x55556666);
        }
        const local = { value: number(row[6]) };
        const base = new SquidPostPop(scene.pool, scene.types, scene.baseState);
        const carrierHooks = scene.carrierHooks(base);
        const common = carrierHooks.base;
        carrierHooks.base = (sid, flags) => { scene.pool.allocated(sid)[34] = number(row[5]) & 0xff; common(sid, flags); };
        const damageable = new RawCarrierPostPop(scene.pool, scene.types, scene.carrierState, carrierHooks);
        const hooks = scene.hooks();
        const sound = hooks.playImpactSound;
        hooks.playImpactSound = (token) => {
            local.value = number(row[7]);
            const value = Number.parseInt(row[9], 10);
            if (value >= 0) put(records, number(row[5]) * STRIDE + TEMPLE, value);
            sound(token);
        };
        const assets = new RawAssetPostPop(scene.pool, scene.types, base, damageable, scene.state, hooks);
        assets.bindPlayerRecords(records, () => local.value);
        base.setBody('asset', assets);
        base.activate(SOURCE, number(row[2]));
        const output = scene.output();
        output[3] = Buffer.from(records).toString('hex');
        assert.deepEqual(output, row.slice(10, 16), `행 ${count}`);
        assert.equal(local.value, number(row[16]), `행 ${count} 현재 owner`);
        count += 1;
    }
    assert.equal(count, 288);
});

// 각도 조회가 서버 권한을 바꾸면 Gun 생성 관문은 그 뒤의 현재 owner 를 다시 읽는다.
test('asset-post-pop: Gun 각도 조회 뒤 서버 권한을 다시 읽는다', skip, () => {
    const found = rows.find((row) => number(row[1]) === 1 && number(row[2]) === 2 && number(row[3]) === 0 && number(row[4]) === 0 && number(row[5]) === 0);
    assert.ok(found);
    const scene = new AssetScene();
    scene.prepare(found);
    const authority = { server: true, boss: scene.state.boss };
    scene.state.sharedAuthority = authority;
    scene.state.inBattle = true;
    const hooks = scene.hooks();
    const frameLookup = hooks.gunFrame;
    // 원문은 이 두 각도/그래프 자식 호출 뒤 540bc0 을 읽는다.
    hooks.gunFrame = (frame) => { authority.server = false; return frameLookup(frame); };
    const body = new CurrentBody(scene, hooks);
    body.base.activate(SOURCE, 2);
    assert.ok(!authority.server && scene.state.inBattle);
    assert.deepEqual(scene.output(), found.slice(15));
});

// 네 실제 PostPop 의 현재 checking·로딩·편집 상태와 진단 복귀를 독립 원문 1,344행으로 비교한다.
test('asset-post-pop: 네 Gun 의 checking·로딩·편집 진단 순서와 복귀가 원본 10.78 1,344행과 같다', skip, () => {
    const scene = new AssetScene();
    const authority = { server: false, boss: false };
    let count = 0;
    // 원문 생성/판정/진단 장치의 입력만 재생하며 발사·실제 게임·통신은 실행하지 않는다.
    for (const row of fixture('gun-postpop-checking-x86.tsv')) {
        assert.equal(row.length, 9);
        const input = row[0].split(',');
        const control = row[1].split(',');
        assert.ok(input.length === 14 && control.length === 5);
        scene.prepare(['originals', ...input, ...new Array(6).fill('0')]);
        const checking = { value: number(control[0]) };
        scene.state.checkingForms = () => checking.value;
        scene.state.fortMode = number(control[4]) !== 0;
        authority.server = number(input[3]) !== 0;
        scene.state.sharedAuthority = authority;
        scene.state.inBattle = !authority.server;
        const mutation = number(control[3]);
        const hooks = scene.hooks();
        const { createGunProcess: create, removeAnimation: remove } = hooks;
        // 실제 new 시점의 전역 변경은 실패 반환에도 남는다.
        hooks.createGunProcess = (sid) => {
            if (mutation === 1 || mutation === 4) {
                checking.value = mutation === 1 ? 0 : 1;
                scene.state.loadingDepth = mutation === 1 ? 1 : 0;
                scene.state.fortMode = mutation === 1;
            }
            create(sid);
        };
        // 선택한 생성 분기는 서버가 바뀌어도 다시 취소하지 않는다.
        hooks.removeAnimation = (sid) => { remove(sid); if (mutation === 5) authority.server = false; };
        hooks.checkForms = (sid) => { scene.events.push(`C:${sid}`); if (mutation === 2) checking.value = 0; return number(control[1]) !== 0; };
        // 진단 뒤 현재 HP/extra 와 전역을 다시 읽는지 전체 raw 로 대조한다.
        hooks.assertion = (expression, file, line) => {
            scene.events.push(`X:${textHex(expression)}:${textHex(file)}:${line}`);
            if (!number(control[2])) throw new DiagnosticStop('명시 PostPop 진단 중단');
            if (mutation === 3) {
                checking.value = 0;
                scene.state.loadingDepth = 1;
                scene.state.fortMode = true;
                put(scene.pool.allocated(SOURCE), 12, 0x1234, 2);
                put(scene.pool.allocated(SOURCE), 36, 0x6789);
            }
        };
        const base = new SquidPostPop(scene.pool, scene.types, scene.baseState);
        const damageable = new RawCarrierPostPop(scene.pool, scene.types, scene.carrierState, scene.carrierHooks(base));
        const assets = new RawAssetPostPop(scene.pool, scene.types, base, damageable, scene.state, hooks);
        let outcome = 'return';
        // 생성 앞 사전 검사에서 원문 진단을 실행하지 않는다.
        assets.validate(SOURCE, number(input[1]));
        assert.deepEqual(scene.events, []);
        try { assets.postPop(SOURCE, number(input[1])); } catch (error) {
            if (!(error instanceof DiagnosticStop)) throw error;
            outcome = 'assertion';
        }
        const output = scene.output();
        assert.equal(output[0], row[2], `행 ${count} 사건`);
        assert.equal(output[1], row[3], `행 ${count} 슬롯`);
        assert.equal(outcome, row[4], `행 ${count} 결과`);
        assert.deepEqual([checking.value, scene.state.loadingDepth, scene.state.fortMode, authority.server], [number(row[5]), number(row[6]), number(row[7]) !== 0, number(row[8]) !== 0], `행 ${count} 전역`);
        count += 1;
    }
    assert.equal(count, 1344);
});

// 누락된 진단·판정은 생성 뒤 원문 소비 위치에서만 중단하며 checking 0 은 판정 장치를 요구하지 않는다.
test('asset-post-pop: Gun 의 누락된 판정/진단 장치는 생성 뒤 소비 위치에서만 중단한다', skip, () => {
    const scene = new AssetScene();
    scene.prepare(rows[0]);
    put(scene.slot(), 0, PATCH_ASSET_TABLES[2]);
    scene.state.inBattle = true;
    const checking = { value: 0 };
    scene.state.checkingForms = () => checking.value;
    const base = new SquidPostPop(scene.pool, scene.types, scene.baseState);
    const damageable = new RawCarrierPostPop(scene.pool, scene.types, scene.carrierState, scene.carrierHooks(base));
    const missingGetter = new RawAssetPostPop(scene.pool, scene.types, base, damageable, scene.state, without(scene.hooks(), 'checkForms'));
    missingGetter.postPop(SOURCE, 1);
    assert.ok(scene.events.length === 3 && scene.events.at(-1) === 'D:1');
    scene.events = [];
    checking.value = 1;
    missingGetter.validate(SOURCE, 1);
    assert.deepEqual(scene.events, []);
    assert.throws(() => missingGetter.postPop(SOURCE, 1));
    assert.ok(scene.events.length === 2 && scene.events.at(-1) === 'P:50');
    scene.events = [];
    const missingAssertion = new RawAssetPostPop(scene.pool, scene.types, base, damageable, scene.state, { ...scene.hooks(), checkForms: () => false });
    assert.throws(() => missingAssertion.postPop(SOURCE, 1));
    assert.ok(scene.events.length === 2 && scene.events.at(-1) === 'P:50');
});

// ── 거주 구역·간헐천의 실제 가상 진입 ──

// 주어진 몸체 번호의 10.78 행을 실제 가상 진입(깊이 증가 뒤 현재 VT+20)으로 재생하고 행 수를 돌려준다. (2026-11-10 WB04-E254)
function replayVirtual(select) {
    const scene = new AssetScene();
    let count = 0;
    // 기존 입력·기대값을 그대로 읽는다.
    for (const row of rows) {
        if (!select(number(row[1]))) continue;
        scene.prepare(row);
        new CurrentBody(scene, scene.hooks()).run(number(row[2]));
        assert.deepEqual(scene.output(), row.slice(15), `몸체 ${row[1]} 행 ${count}`);
        assert.equal(scene.baseState.depth, 0);
        count += 1;
    }
    return count;
}

test('asset-post-pop: 실제 가상 진입으로 재생한 거주 구역·간헐천(188)·Gun 여섯 몸체(318)·바리케이드(48)·신전 세 wrapper(264)가 원본과 같다', skip, () => {
    assert.equal(replayVirtual((body) => body === 6 || body === 8), 188);
    assert.equal(replayVirtual((body) => body < 6), 318);
    assert.equal(replayVirtual((body) => body === 7), 48);
    assert.equal(replayVirtual((body) => body >= 9), 264);
});

test('asset-post-pop 거주/간헐천: 최초가 아니면 TYPE/목록/예약 전수 검사를 추가하지 않고, 지면·소유자 장치는 도달할 때만 요구한다', skip, () => {
    // 두 실제 부모 모두 최초가 아니면 앞단 장치를 요구하지 않는다(간헐천은 뒤 Damageable 이 flags 0 을 읽는다).
    for (const kind of [6, 8]) {
        const scene = new AssetScene();
        scene.prepareCurrent(kind, 0);
        scene.slot()[10] = 255;
        scene.state.temples.count = 99;
        new CurrentBody(scene, {}).run(0);
        assert.ok(scene.baseState.depth === 0 && scene.state.residences.count === 0 && scene.state.geysers.count === 0);
        assert.deepEqual(scene.events, kind === 6 ? [] : ['D:0']);
    }
    // 첫 거주 구역의 선택된 지면 장치가 없으면 앞선 쓰기는 없다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(6);
        const body = new CurrentBody(scene, without(scene.hooks(), 'surface'));
        assert.throws(() => body.run());
        assert.ok(scene.state.residences.count === 0 && scene.allocated === 0 && scene.baseState.depth === 1 && scene.events.length === 0);
    }
    // 지면 없음/owner 0 은 소유자 장치를 선택하지 않고 owner 2 만 그 위치에서 중단한다.
    for (const owner of [0, 2]) {
        const scene = new AssetScene();
        scene.prepareCurrent(6);
        scene.slot(60)[34] = owner;
        const body = new CurrentBody(scene, without(scene.hooks(), 'setOwner'));
        if (owner) {
            assert.throws(() => body.run());
            assert.ok(scene.state.residences.count === 0 && scene.allocated === 0 && scene.baseState.depth === 1);
        } else {
            body.run();
            assert.ok(scene.state.residences.count === 1 && scene.allocated === 3 && scene.baseState.depth === 0);
        }
    }
});

test('asset-post-pop 거주 구역: 확보 실패·늦은 통지 누락·통지 뒤 현재 프레임/nibble·free 로 바뀐 물리 슬롯', skip, () => {
    // extra 9 여도 최초 앞단을 실행하며 세 확보 실패는 생성자 누락을 요구하지 않는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(6);
        scene.reserveMask = 0;
        scene.slot()[40] = 9;
        scene.state.temples.count = 99;
        new CurrentBody(scene, { ...without(scene.hooks(), 'setOwner', 'addRegular'), surface: () => 0 }).run();
        assert.deepEqual(scene.events, ['A:40', 'A:40', 'A:40', 'N:50']);
        assert.ok(scene.state.residences.count === 1 && scene.baseState.depth === 0 && get(scene.pool.slot(SOURCE), 42, 4) === 25 && scene.pool.slot(SOURCE)[12] === 0xc0);
    }
    // 지면 통지가 없을 때 앞선 목록/예약/하위 nibble 쓰기를 보존하고 잔여 프레임/공통은 미도달이다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(6);
        const body = new CurrentBody(scene, { ...without(scene.hooks(), 'notifySurface'), surface: () => 0 });
        assert.throws(() => body.run());
        assert.ok(scene.state.residences.count === 1 && scene.allocated === 3 && scene.events.length === 6 && scene.pool.slot(SOURCE)[12] === 0xc0);
        assert.ok(get(scene.pool.slot(SOURCE), 42, 4) === 0 && scene.baseState.depth === 1);
    }
    // 통지 뒤 현재 프레임·nibble·애니메이션 플래그로 잔여 값을 계산한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(6);
        scene.reserveMask = 0;
        const notifySurface = (sid) => { const raw = scene.slot(sid); raw[12] |= 5; put(raw, 36, 17); scene.state.residenceAnimation = true; };
        new CurrentBody(scene, { ...scene.hooks(), surface: () => 0, notifySurface }).run();
        assert.ok(get(scene.pool.slot(SOURCE), 42, 4) === 3 && get(scene.pool.slot(SOURCE), 36, 4) === 17 && scene.baseState.depth === 0);
    }
    // 소유자 콜백이 free/contained 와 TYPE 을 바꿔도 같은 물리 슬롯의 원문 쓰기를 유지한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(6);
        scene.reserveMask = 0;
        const setOwner = (sid, owner) => { const raw = scene.slot(sid); raw[34] = owner; raw[11] = 9; raw[10] = 255; };
        new CurrentBody(scene, { ...scene.hooks(), setOwner }).run();
        const raw = scene.pool.slot(SOURCE);
        assert.ok(raw[11] === 9 && raw[10] === 255 && raw[12] === 0xc0 && get(raw, 42, 4) === 25 && scene.baseState.depth === 0);
    }
});

test('asset-post-pop 거주 구역: 현재 목록의 signed 가득 참·음수 위치·store 뒤 증가와 중복 유지', skip, () => {
    const unreachable = (text) => () => { throw new Error(text); };
    // signed count>=capacity 면 과거 배열 포인터와 쓰기 장치를 해석하지 않는다(양수 초과와 두 음수 용량 비교).
    for (const [count, capacity] of [[7, 4], [0, 0xffffffff], [0xffffffff, 0xfffffffe]]) {
        const scene = new AssetScene();
        scene.prepareCurrent(6);
        scene.reserveMask = 0;
        const current = new SquidPostPopList();
        current.bindCurrent({ storage: unreachable('가득 찬 목록의 미도달 저장소'), capacity: () => capacity, readCount: () => count, writeCount: unreachable('가득 찬 목록의 미도달 쓰기') });
        scene.state.sharedResidences = current;
        new CurrentBody(scene, { ...scene.hooks(), surface: () => 0 }).run();
        assert.ok(scene.allocated === 3 && scene.baseState.depth === 0, `count ${count} capacity ${capacity}`);
    }
    // signed 음수 위치가 실제 저장을 선택하면 앞선 지면 효과 뒤 물리 범위 오류로 중단한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(6);
        const current = new SquidPostPopList();
        current.bindCurrent({ storage: unreachable('음수 위치의 미도달 저장소'), capacity: () => 4, readCount: () => 0xffffffff, writeCount: unreachable('미도달 쓰기') });
        scene.state.sharedResidences = current;
        const body = new CurrentBody(scene, { ...scene.hooks(), surface: () => { scene.events.push('S'); return 0; } });
        assert.throws(() => body.run(), /음수 저장 위치/);
        assert.deepEqual(scene.events, ['S']);
        assert.ok(scene.allocated === 0 && scene.baseState.depth === 1);
    }
    // 같은 현재 배열/count 를 쓰며 중복 SID 와 비활성 꼬리를 보존한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(6);
        scene.reserveMask = 0;
        const values = [7, 8, 9];
        let count = 0;
        const current = new SquidPostPopList();
        current.bindCurrent({ storage: () => values, capacity: () => 3, readCount: () => count, writeCount: (value) => { assert.equal(values[count], 50); count = value; } });
        scene.state.sharedResidences = current;
        const body = new CurrentBody(scene, { ...scene.hooks(), surface: () => 0 });
        body.run();
        body.run();
        assert.ok(count === 2 && scene.state.residences.count === 0 && scene.baseState.depth === 0);
        assert.deepEqual(values, [50, 50, 9]);
    }
});

test('asset-post-pop 간헐천: 등록 누락·extra 관문·현재 TYPE 비용·Repop 누락·받침 아닌 후보', skip, () => {
    // 실제 등록 장치 앞의 간헐천 목록 추가를 사전 누락 검사로 지우지 않는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(8);
        const body = new CurrentBody(scene, without(scene.hooks(), 'geyserRegistration'));
        assert.throws(() => body.run());
        assert.ok(scene.state.geysers.count === 1 && scene.state.geysers.entries[0] === 50 && scene.allocated === 0 && scene.baseState.depth === 1);
    }
    // extra 9 는 목록/등록/예약/이웃을 생략하고 실제 Damageable 만 호출한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(8);
        scene.slot()[40] = 9;
        new CurrentBody(scene, {}).run();
        assert.deepEqual(scene.events, ['D:1']);
        assert.ok(scene.state.geysers.count === 0 && scene.baseState.depth === 0);
    }
    // 등록 콜백 뒤 현재 TYPE 의 비용을 WORD 로 저장하고 확보 실패 뒤에도 이웃/공통으로 진행한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(8);
        scene.reserveMask = 0;
        const hooks = without(scene.hooks(), 'addRegular', 'unpop', 'repop');
        hooks.geyserRegistration = (sid, value) => { assert.equal(value, 1); scene.events.push('G'); scene.slot(sid)[10] = 75; scene.types[75].cost = 513.875; };
        hooks.atPosition = () => { scene.events.push('P'); return []; };
        new CurrentBody(scene, hooks).run();
        assert.equal(get(scene.pool.slot(SOURCE), 12, 2), 513);
        assert.deepEqual(scene.events, ['G', 'A:40', 'P', 'D:1']);
        assert.equal(scene.baseState.depth, 0);
    }
    // 현재 이웃 Unpop 이 free 로 바꾼 뒤에도 프레임 8 은 저장되며 미연결 Repop 위치에서 중단한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(8);
        const hooks = without(scene.hooks(), 'repop');
        hooks.unpop = (sid, flags) => { assert.ok(sid === 60 && flags === 0); scene.events.push('U'); scene.slot(sid)[11] = 1; };
        const body = new CurrentBody(scene, hooks);
        assert.throws(() => body.run());
        assert.ok(scene.state.geysers.count === 1 && get(scene.pool.slot(SOURCE), 12, 2) === 1000 && scene.pool.slot(60)[11] === 1);
        assert.ok(get(scene.pool.slot(60), 36, 4) === 8 && get(scene.pool.slot(61), 36, 4) === 0 && scene.events.at(-1) === 'U' && scene.baseState.depth === 1);
    }
    // 현재 TYPE 오류는 목록/등록 뒤 실제 비용 조회에서만 거부한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(8);
        const body = new CurrentBody(scene, { ...scene.hooks(), geyserRegistration: (sid) => { scene.events.push('G'); scene.slot(sid)[10] = 255; } });
        assert.throws(() => body.run(), /현재 TYPE 범위 오류/);
        assert.deepEqual(scene.events, ['G']);
        assert.ok(scene.state.geysers.count === 1 && scene.allocated === 0 && scene.baseState.depth === 1);
    }
    // 현재 후보가 받침 TYPE 이 아니면 Unpop/Repop 장치를 요구하지 않는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(8);
        scene.slot(60)[10] = 75;
        new CurrentBody(scene, { ...without(scene.hooks(), 'unpop', 'repop'), atPosition: () => [60] }).run();
        assert.ok(scene.baseState.depth === 0 && scene.events.at(-1) === 'D:1');
    }
});

test('asset-post-pop 간헐천: 실제 일반 finder 가 효과 사이에 바뀐 링크를 따라가고 WORD setter 는 현재 다리 TYPE 에 통지한다', skip, () => {
    // 실제 일반 finder 는 첫 효과가 바꾼 다음 후보의 링크를 이후 Next 에서 읽는다. 입력 체인은 60→61→62 이며 같은 위치/발자국의 63 은 아직 체인 밖이다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(8);
        const hash = new SquidHash();
        hash.setCell(1, 25, 26, 60);
        for (const sid of [60, 61, 62, 63]) {
            const raw = scene.slot(sid);
            raw.fill(0);
            raw[10] = sid === 60 ? 94 : 95;
            put(raw, 4, sid === 60 ? 61 : sid === 61 ? 62 : 0, 2);
            put(raw, 14, floatToBits(50));
            put(raw, 18, floatToBits(52));
        }
        Object.assign(scene.types[94], { footX: 1, footY: 1 });
        Object.assign(scene.types[95], { footX: 1, footY: 1 });
        const hooks = without(scene.hooks(), 'atPosition');
        hooks.visitAtPosition = makeAssetPostPopFinder(scene.pool, hash, scene.types);
        hooks.unpop = (sid) => { scene.events.push(`U${sid}`); if (sid === 60) put(scene.slot(61), 4, 63, 2); };
        hooks.repop = (sid, flags) => { assert.ok(flags === 0x40 && get(scene.pool.slot(sid), 36, 4) === 8); scene.events.push(`V${sid}`); };
        const body = new CurrentBody(scene, hooks);
        body.assets.validate(SOURCE, 1);
        assert.deepEqual(scene.events, []);
        body.run();
        assert.deepEqual(scene.events, ['G:50:1', 'A:40', 'R:50:0:1036831949', 'U60', 'V60', 'U61', 'V61', 'U63', 'V63', 'D:1']);
        assert.ok(get(scene.pool.slot(62), 36, 4) === 0 && scene.baseState.depth === 0);
    }
    // 현재 다리 번호와 같으면 WORD 저장 뒤 통지를 실행하고 그 콜백 뒤 새 TYPE/좌표로 진행한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(8);
        const bridge = { value: 70 };
        scene.state.currentBridgeType = () => bridge.value;
        const hooks = scene.hooks();
        hooks.notifySurface = (sid) => {
            assert.equal(get(scene.pool.slot(sid), 12, 2), 1000);
            scene.events.push('H');
            scene.slot(sid)[10] = 75;
            put(scene.slot(sid), 14, floatToBits(40));
            scene.types[75].cost = 12;
        };
        hooks.atPosition = (x, y) => { assert.ok(x === 40 && y === 52); scene.events.push('P'); return []; };
        new CurrentBody(scene, hooks).run();
        assert.deepEqual(scene.events, ['G:50:1', 'H', 'A:40', 'R:50:0:1036831949', 'P', 'D:1']);
        assert.ok(get(scene.pool.slot(SOURCE), 12, 2) === 1000 && scene.baseState.depth === 0);
    }
    // 선택된 다리 통지가 없으면 이미 끝난 목록/등록/WORD 를 보존하고 예약 전에 중단한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(8);
        scene.state.bridgeType = 70;
        const body = new CurrentBody(scene, without(scene.hooks(), 'notifySurface'));
        assert.throws(() => body.run());
        assert.deepEqual(scene.events, ['G:50:1']);
        assert.ok(scene.state.geysers.count === 1 && get(scene.pool.slot(SOURCE), 12, 2) === 1000 && scene.allocated === 0 && scene.baseState.depth === 1);
    }
    // Native 간헐천 본문은 비유한 비용을 0 으로 쓰고(typed 는 거부) 현재 섬/종유석 TYPE getter 를 후보마다 읽는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(8);
        scene.types[70].cost = Infinity;
        const typed = new CurrentBody(scene, scene.hooks());
        assert.throws(() => typed.run(), /CRT 정수 범위 오류/);
        scene.prepareCurrent(8);
        scene.types[70].cost = Infinity;
        scene.state.currentIslandType = () => 95;
        scene.state.currentStalagType = () => 0x100 + 95;
        const native = new CurrentBody(scene, scene.hooks());
        scene.baseState.depth += 1;
        native.assets.geyserPostPop(SOURCE, 1);
        assert.equal(get(scene.pool.slot(SOURCE), 12, 2), 0);
        // 섬 TYPE 의 현재값이 95 이므로 후보 61(TYPE 95)만 다시 Pop 되고 60(TYPE 94)은 건너뛴다.
        assert.deepEqual(scene.events.filter((event) => event[0] === 'U' || event[0] === 'V'), ['U:61:0', 'V:61:64']);
        assert.equal(scene.baseState.depth, 0);
    }
});

// ── Gun 여섯 몸체 ──

test('asset-post-pop Gun: 최초가 아니면 미도달 장치/TYPE 을 요구하지 않고, 각도·생성·제거 장치는 도달 위치에서만 요구한다', skip, () => {
    // 같은 실제 PATCH 부모로 flags 0 과 잘못된 TYPE 을 공급한다(GunSun 만 필수 TYPE 조회를 남긴다).
    for (const kind of [0, 1, 2, 4, 5]) {
        const scene = new AssetScene();
        scene.prepareCurrent(kind, 0);
        scene.slot()[10] = 255;
        scene.state.temples.count = 99;
        new CurrentBody(scene, {}).run(0);
        assert.ok(scene.baseState.depth === 0 && scene.allocated === 0, `몸체 ${kind}`);
    }
    // 그래프 setter 가 없으면 이미 실행한 현재 각도 getter 의 효과를 보존한다.
    for (const getter of [false, true]) {
        const scene = new AssetScene();
        scene.prepareCurrent(1, 2);
        const body = new CurrentBody(scene, getter ? without(scene.hooks(), 'setGraph') : without(scene.hooks(), 'setGraph', 'gunFrame'));
        assert.throws(() => body.run(2));
        assert.deepEqual(scene.events, getter ? ['F:25'] : []);
        assert.ok(scene.baseState.depth === 1 && scene.allocated === 0);
    }
    // 그래프 효과 뒤 서버/extra/TYPE 이 바뀌면 현재 관문을 읽고 같은 물리 슬롯을 유지한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(1, 2);
        scene.state.inBattle = true;
        const hooks = without(scene.hooks(), 'createGunProcess');
        hooks.setGraph = (sid, graph) => {
            assert.equal(graph, 2);
            scene.events.push('Q');
            const raw = scene.slot(sid);
            raw[30] = graph;
            raw[10] = 255;
            raw[11] = 9;
            scene.state.inBattle = false;
        };
        new CurrentBody(scene, hooks).run(2);
        assert.deepEqual(scene.events, ['F:25', 'Q', 'D:2']);
        assert.ok(scene.pool.slot(SOURCE)[30] === 2 && scene.pool.slot(SOURCE)[11] === 9 && scene.baseState.depth === 0);
    }
    // 선택된 생성 장치 누락은 각도 설정 또는 애니메이션 제거 뒤 그 위치에서 중단한다.
    for (const kind of [1, 4]) {
        const scene = new AssetScene();
        scene.prepareCurrent(kind);
        scene.state.inBattle = true;
        scene.state.fortMode = true;
        const body = new CurrentBody(scene, without(scene.hooks(), 'createGunProcess'));
        assert.throws(() => body.run());
        assert.deepEqual(scene.events, kind === 1 ? ['F:25', 'Q:2'] : ['M:50']);
        assert.ok(scene.baseState.depth === 1 && scene.allocated === 0);
    }
    // 선택되지 않은 서버/extra 가지는 생성·제거·진단 장치를 요구하지 않는다(서버인 입력은 extra 1 로 차단).
    for (const kind of [1, 2, 3, 4]) {
        for (const server of [false, true]) {
            const scene = new AssetScene();
            scene.prepareCurrent(kind);
            scene.state.inBattle = server;
            scene.slot()[40] = server ? 1 : 0;
            new CurrentBody(scene, without(scene.hooks(), 'createGunProcess', 'removeAnimation', 'checkForms', 'assertion')).run();
            assert.ok(scene.allocated === 0 && scene.events.at(-1) === 'D:1' && scene.baseState.depth === 0, `몸체 ${kind} 서버 ${server}`);
        }
    }
    // 제거 콜백이 서버/extra 를 바꿔도 이미 선택한 생성 분기를 취소하지 않는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(4);
        scene.state.inBattle = true;
        scene.state.fortMode = true;
        const hooks = scene.hooks();
        hooks.removeAnimation = (sid) => { scene.events.push('M'); scene.state.inBattle = false; scene.slot(sid)[40] = 1; };
        hooks.createGunProcess = () => scene.events.push('P');
        new CurrentBody(scene, hooks).run();
        assert.deepEqual(scene.events, ['M', 'P', 'D:1']);
        assert.equal(scene.baseState.depth, 0);
    }
});

test('asset-post-pop Gun 장식·썬: 확보 실패/생성자 누락, 최초가 아닌 썬의 TYPE 조회와 WORD 복원·다리 통지·로딩 백업', skip, () => {
    // 장식은 확보가 실패하면 생성자 누락을 요구하지 않고 공통 깊이를 감소한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(5);
        scene.reserveMask = 0;
        scene.slot()[10] = 255;
        new CurrentBody(scene, without(scene.hooks(), 'addRegular')).run();
        assert.deepEqual(scene.events, ['A:40']);
        assert.ok(scene.allocated === 1 && scene.baseState.depth === 0);
    }
    // 확보 성공 뒤 선택 생성자가 없으면 앞선 확보를 지우거나 공통 몸체를 실행하지 않는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(5);
        const body = new CurrentBody(scene, without(scene.hooks(), 'addRegular'));
        assert.throws(() => body.run());
        assert.deepEqual(scene.events, ['A:40']);
        assert.ok(scene.allocated === 1 && scene.baseState.depth === 1);
    }
    // 최초가 아니어도 현재 속성에 따라 free/contained 물리 슬롯의 WORD 를 복원한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(3, 0);
        scene.types[70].flags1 = 0x1000;
        const raw = scene.slot();
        raw[11] = 9;
        put(raw, 36, 0x12345678);
        new CurrentBody(scene, {}).run(0);
        assert.ok(get(scene.pool.slot(SOURCE), 12, 2) === 0x5678 && scene.pool.slot(SOURCE)[11] === 9 && scene.baseState.depth === 0);
        assert.deepEqual(scene.events, ['D:0']);
    }
    // flags 0 에서도 원문의 마지막 TYPE 접근을 생략하지 않는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(3, 0);
        scene.slot()[10] = 255;
        const body = new CurrentBody(scene, {});
        assert.throws(() => body.run(0), /현재 TYPE 범위 오류/);
        assert.ok(scene.events.length === 0 && scene.baseState.depth === 1);
    }
    // 현재 다리 DWORD 가 일치하면 WORD 저장 뒤 통지 장치를 요구한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(3, 0);
        scene.types[70].flags1 = 0x1000;
        scene.state.currentBridgeType = () => 70;
        put(scene.slot(), 36, 0x9876);
        const body = new CurrentBody(scene, {});
        assert.throws(() => body.run(0));
        assert.ok(get(scene.pool.slot(SOURCE), 12, 2) === 0x9876 && scene.events.length === 0 && scene.baseState.depth === 1);
    }
    // 다리 TYPE 은 DWORD 전체로 비교한다(0x100+70 은 통지하지 않음).
    {
        const scene = new AssetScene();
        scene.prepareCurrent(3, 0);
        scene.types[70].flags1 = 0x1000;
        scene.state.currentBridgeType = () => 0x100 + 70;
        new CurrentBody(scene, {}).run(0);
        assert.deepEqual(scene.events, ['D:0']);
    }
    // 통지 복귀 뒤 현재 TYPE/free/WORD 를 원래 사본으로 되돌리거나 재검사하지 않는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(3, 0);
        scene.types[70].flags1 = 0x1000;
        scene.state.bridgeType = 70;
        const notifySurface = (sid) => {
            assert.equal(get(scene.pool.slot(sid), 12, 2), 25);
            scene.events.push('N');
            const raw = scene.slot(sid);
            raw[10] = 255;
            raw[11] = 9;
            put(raw, 12, 0x4321, 2);
        };
        new CurrentBody(scene, { ...scene.hooks(), notifySurface }).run(0);
        assert.deepEqual(scene.events, ['N', 'D:0']);
        assert.ok(get(scene.pool.slot(SOURCE), 12, 2) === 0x4321 && scene.pool.slot(SOURCE)[11] === 9 && scene.baseState.depth === 0);
    }
    // 생성 효과 뒤 현재 로딩/TYPE 과 WORD 를 읽어 DWORD 백업 → WORD setter → 통지 순서로 진행한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(3);
        scene.state.inBattle = true;
        const hooks = scene.hooks();
        hooks.createGunProcess = (sid) => {
            scene.events.push('P');
            scene.state.loadingDepth = 1;
            const raw = scene.slot(sid);
            raw[10] = 75;
            raw[11] = 9;
            put(raw, 12, 0x8765, 2);
            scene.types[75].flags1 = 0x1000;
            scene.state.bridgeType = 75;
        };
        hooks.notifySurface = (sid) => { assert.ok(get(scene.pool.slot(sid), 36, 4) === 0x8765 && get(scene.pool.slot(sid), 12, 2) === 0x8765); scene.events.push('N'); };
        new CurrentBody(scene, hooks).run();
        assert.deepEqual(scene.events, ['P', 'N', 'D:1']);
        assert.ok(scene.pool.slot(SOURCE)[11] === 9 && scene.baseState.depth === 0);
    }
});

// ── 바리케이드와 두 펜스 ──

// 일반 바리케이드(0)·아크 스파이어(1)·애시드 펜스(2)의 명시 가상 표 입력만 준비한다(cpppj PrepareAssetFence). (2026-11-10 WB04-E254)
function prepareFence(scene, kind, flags = 1) {
    scene.prepareCurrent(7, flags);
    put(scene.slot(), 0, [0x50e940, 0x50e668, 0x50e738][kind]);
}

test('asset-post-pop 펜스: 두 원문의 P 후처리 100행(예약 사건·물리 무변경)이 같다', skip, () => {
    let count = 0;
    // 원문별 기존 외부 경계 trace 와 물리 무변경을 확인한다. 공격·수신·통신 행은 실행하지 않는다.
    for (const kind of [1, 2]) {
        const scene = new AssetScene();
        for (const row of fixture(kind === 1 ? 'arc-spire-x86.tsv' : 'acid-fence-x86.tsv')) {
            if (row[0] !== 'P') continue;
            const values = row[1].split(',');
            assert.equal(values.length, 3);
            const flags = number(values[0]);
            prepareFence(scene, kind, flags);
            scene.slot()[40] = number(values[1]) & 0xff;
            const before = Buffer.from(scene.pool.slot(SOURCE)).toString('hex');
            const hooks = scene.hooks();
            hooks.reserveRegular = () => { scene.events.push('N:40'); return number(values[2]) !== 0; };
            hooks.addRegular = (sid, event, delay) => scene.events.push(`R:${event}:${sid}:${floatToBits(delay)}`);
            new CurrentBody(scene, hooks).run(flags);
            assert.equal(scene.output()[0], row[2], `부류 ${kind} 행 ${count}`);
            assert.equal(Buffer.from(scene.pool.slot(SOURCE)).toString('hex'), before);
            assert.equal(scene.baseState.depth, 0);
            count += 1;
        }
    }
    assert.equal(count, 100);
});

test('asset-post-pop 바리케이드: 서버·최초·비로딩에서만 차단 시작을 요구하고 extra 는 관문이 아니다', skip, () => {
    // 세 부모 모두 flags 0 에서는 앞단 미도달 장치/TYPE 전수 검사를 추가하지 않는다.
    for (const kind of [0, 1, 2]) {
        const scene = new AssetScene();
        prepareFence(scene, kind, 0);
        scene.slot()[10] = 255;
        scene.state.temples.count = 99;
        new CurrentBody(scene, {}).run(0);
        assert.deepEqual(scene.events, ['D:0']);
        assert.equal(scene.baseState.depth, 0);
    }
    // 서버가 아니거나 로딩 중이면 통지를 요구하지 않는다.
    for (const server of [false, true]) {
        const scene = new AssetScene();
        prepareFence(scene, 0);
        scene.state.inBattle = server;
        scene.state.loadingDepth = server ? 1 : 0;
        scene.slot()[40] = 9;
        new CurrentBody(scene, {}).run();
        assert.deepEqual(scene.events, ['D:1']);
        assert.equal(scene.baseState.depth, 0);
    }
    // 실제 통지 위치의 누락은 Damageable/공통 깊이 감소 전에 중단한다.
    {
        const scene = new AssetScene();
        prepareFence(scene, 0);
        scene.state.inBattle = true;
        const body = new CurrentBody(scene, {});
        assert.throws(() => body.run());
        assert.ok(scene.events.length === 0 && scene.baseState.depth === 1);
    }
    // 통지 뒤 현재 TYPE/free/extra 와 전역을 복구하지 않고 같은 직접 부모를 실행한다.
    {
        const scene = new AssetScene();
        prepareFence(scene, 0);
        scene.state.inBattle = true;
        const startBlocker = (sid) => {
            scene.events.push('B');
            const raw = scene.slot(sid);
            raw[10] = 75;
            raw[11] = 9;
            raw[40] = 9;
            scene.state.inBattle = false;
            scene.state.loadingDepth = 1;
        };
        new CurrentBody(scene, { ...scene.hooks(), startBlocker }).run();
        assert.deepEqual(scene.events, ['B', 'D:1']);
        assert.ok(scene.pool.slot(SOURCE)[11] === 9 && scene.baseState.depth === 0);
    }
    // 세 실제 가상 표의 공통 몸체는 extra 9 여도 서버 최초 통지를 실행한다.
    for (const table of [0x50e940, 0x50ea08, 0x50ead0]) {
        const scene = new AssetScene();
        prepareFence(scene, 0);
        put(scene.slot(), 0, table);
        scene.slot()[40] = 9;
        scene.state.inBattle = true;
        new CurrentBody(scene, scene.hooks()).run();
        assert.deepEqual(scene.events, ['K:50', 'D:1']);
        assert.equal(scene.baseState.depth, 0);
    }
});

test('asset-post-pop 두 펜스: extra 관문·확보 실패/누락·생성자 누락/실패·관문 재평가 없음·잘못된 TYPE', skip, () => {
    for (const kind of [1, 2]) {
        // extra 1/8/9 는 예약 앞단을 생략하고 직접 부모만 실행한다.
        for (const extra of [1, 8, 9]) {
            const scene = new AssetScene();
            prepareFence(scene, kind);
            scene.slot()[40] = extra;
            new CurrentBody(scene, {}).run();
            assert.deepEqual(scene.events, ['D:1']);
            assert.equal(scene.baseState.depth, 0);
        }
        // 확보가 실패하면 생성자를 요구하지 않고 변경된 물리 슬롯의 부모를 계속한다.
        {
            const scene = new AssetScene();
            prepareFence(scene, kind);
            const hooks = without(scene.hooks(), 'addRegular');
            hooks.reserveRegular = () => { scene.events.push('A'); const raw = scene.slot(); raw[10] = 75; raw[11] = 9; return false; };
            new CurrentBody(scene, hooks).run();
            assert.deepEqual(scene.events, ['A', 'D:1']);
            assert.ok(scene.pool.slot(SOURCE)[11] === 9 && scene.baseState.depth === 0);
        }
        // 선택된 확보가 없으면 아직 생성/직접 부모를 실행하지 않는다(잘못된 TYPE 도 앞선 예약 장치 실패를 덮지 않는다).
        {
            const scene = new AssetScene();
            prepareFence(scene, kind);
            scene.slot()[10] = 255;
            const body = new CurrentBody(scene, {});
            assert.throws(() => body.run(), /실제 하위/);
            assert.ok(scene.events.length === 0 && scene.baseState.depth === 1);
        }
        // 확보 뒤 생성자가 없으면 앞선 부수효과를 유지하고 부모는 미도달이다.
        {
            const scene = new AssetScene();
            prepareFence(scene, kind);
            const body = new CurrentBody(scene, without(scene.hooks(), 'addRegular'));
            assert.throws(() => body.run());
            assert.deepEqual(scene.events, ['A:40']);
            assert.ok(scene.allocated === 1 && scene.baseState.depth === 1);
        }
        // 확보 중 extra 9 로 바뀌어도 이미 선택한 생성자와 원문 지연(아크 1초, 애시드 0.3초)을 전달한다.
        {
            const scene = new AssetScene();
            prepareFence(scene, kind);
            const hooks = scene.hooks();
            hooks.reserveRegular = () => { scene.events.push('A'); scene.slot()[40] = 9; return true; };
            hooks.addRegular = (sid, event, delay) => { assert.ok(sid === SOURCE && event === 0 && delay === (kind === 1 ? 1 : Math.fround(0.3))); scene.events.push('R'); };
            new CurrentBody(scene, hooks).run();
            assert.deepEqual(scene.events, ['A', 'R', 'D:1']);
            assert.equal(scene.baseState.depth, 0);
        }
        // 생성 콜백의 명시 실패 뒤 앞선 확보/콜백 쓰기를 보존한다.
        {
            const scene = new AssetScene();
            prepareFence(scene, kind);
            const addRegular = (sid) => { scene.events.push('R'); scene.slot(sid)[11] = 9; throw new Error('명시 펜스 생성 중단'); };
            const body = new CurrentBody(scene, { ...scene.hooks(), addRegular });
            assert.throws(() => body.run(), /명시 펜스 생성 중단/);
            assert.deepEqual(scene.events, ['A:40', 'R']);
            assert.ok(scene.pool.slot(SOURCE)[11] === 9 && scene.baseState.depth === 1);
        }
        // 생성 효과가 TYPE 을 범위 밖으로 바꾸면 뒤 부모의 실제 조회에서만 실패한다.
        {
            const scene = new AssetScene();
            prepareFence(scene, kind);
            const body = new CurrentBody(scene, { ...scene.hooks(), addRegular: (sid) => { scene.events.push('R'); scene.slot(sid)[10] = 255; } });
            assert.throws(() => body.run());
            assert.deepEqual(scene.events, ['A:40', 'R']);
            assert.equal(scene.baseState.depth, 1);
        }
    }
});

// ── 전체 몸체를 훅으로 받는 부류(성장형 아이스·썬 바리케이드·너겟) ──

// 성장형 아이스(0)·썬 바리케이드(1)·너겟(2)의 원문 VT 만 공급한다(cpppj PrepareAssetTail). (2026-11-10 WB04-E254)
function prepareTail(scene, kind, flags = 1) {
    scene.prepareCurrent(0, flags);
    put(scene.slot(), 0, [0x50e888, 0x509470, 0x50dc30][kind]);
}

// 프레임 코드가 없는 188칸 표이다(raw-ice-tower.js 는 TYPE 마다 { codes } 를 받는다). (2026-11-10 WB04-E254)
const emptyFrames = () => Array.from({ length: 188 }, () => ({ codes: [] }));

test('asset-post-pop 전체 몸체 부류: 가상 진입은 앞단 TYPE 검사를 추가하지 않고, 몸체가 없으면 분배에서 거부한다', skip, () => {
    // 실제 하위는 flags 0 에서 같은 직접/공통 부모만 호출한다. 첫 몸체 선택 뒤에는 TYPE 을 다시 검증하지 않는다.
    for (const kind of [0, 1, 2]) {
        const scene = new AssetScene();
        prepareTail(scene, kind, 0);
        scene.slot()[10] = 255;
        scene.state.temples.count = 99;
        let parent = null;
        const finish = (sid, flags) => { scene.events.push('P'); parent.postPopBase(sid, flags); };
        const frames = emptyFrames();
        const nugget = new RawNuggetPostPop(scene.pool, { damageable: finish });
        const hooks = scene.hooks();
        hooks.growingIcePostPop = (sid, flags) => postPopGrowingIce(scene.pool, frames, sid, flags, undefined, undefined, finish);
        hooks.sunFencePostPop = finish;
        hooks.nuggetPostPop = (sid, flags) => nugget.postPop(sid, flags);
        const body = new CurrentBody(scene, hooks);
        parent = body.base;
        body.run(0);
        assert.deepEqual(scene.events, ['P']);
        assert.equal(scene.baseState.depth, 0);
    }
    // 상위에 빈 하위 성공을 설치하지 않고 선택한 몸체 누락만 명시 실패한다(raw TYPE 오류보다 먼저).
    for (const kind of [0, 1, 2]) {
        const scene = new AssetScene();
        prepareTail(scene, kind, 0);
        scene.slot()[10] = 255;
        const body = new CurrentBody(scene, {});
        assert.throws(() => body.run(0), /선택된 실제 장치 미연결/);
        assert.ok(scene.events.length === 0 && scene.baseState.depth === 1);
    }
    // 훨리베이스·맨 오 워 풀·데빌메이커도 전체 몸체 훅으로만 분배한다(공중 3종의 행동은 실행하지 않는다).
    for (const [table, name] of [[0x509930, 'sunAviaryPostPop'], [0x5099e8, 'rainAviaryPostPop'], [0x509aa0, 'windAviaryPostPop']]) {
        const scene = new AssetScene();
        scene.prepareCurrent(0, 1);
        put(scene.slot(), 0, table);
        assert.throws(() => new CurrentBody(scene, {}).run(), /선택된 실제 장치 미연결/);
        const calls = [];
        const body = new CurrentBody(scene, { [name]: (sid, flags) => { calls.push([sid, flags]); body.base.postPopBase(sid, flags); } });
        assert.throws(() => new CurrentBody(scene, {}).assets.validate(SOURCE, 1), /전체 후처리 미연결/);
        body.assets.validate(SOURCE, 1);
        scene.baseState.depth = 0;
        body.run(1);
        assert.deepEqual(calls, [[SOURCE, 1]]);
        assert.equal(scene.baseState.depth, 0);
    }
});

test('asset-post-pop 성장형 아이스: 원문의 P 예약 212행과 확보 실패·현재 프레임 조회 순서', skip, () => {
    const scene = new AssetScene();
    let count = 0;
    // 독립 Native trace 를 기대값 재계산 없이 대조한다. 성장/재생성/공격 행은 선택하지 않는다.
    for (const row of fixture('ice-tower-x86.tsv')) {
        if (row[0] !== 'P') continue;
        const values = row[1].split(',');
        assert.equal(values.length, 5);
        const flags = number(values[0]);
        prepareTail(scene, 0, flags);
        scene.slot()[40] = number(values[1]) & 0xff;
        const frames = emptyFrames();
        // 원문 150/151 의 현재 프레임 수 입력을 일반 TYPE 70/75 에 공급한다(분기를 계산하지 않고 개수만).
        for (const [type, length] of [[70, number(values[2])], [75, 9]]) frames[type] = { codes: Array.from({ length }, (_, index) => ({ side: 0x41, variant: 0, number: index, flags: 0 })) };
        let parent = null;
        const hooks = scene.hooks();
        hooks.growingIcePostPop = (sid, mode) => postPopGrowingIce(scene.pool, frames, sid, mode,
            () => { scene.events.push('N:40'); if (number(values[4]) === 1) scene.slot()[10] = 75; return number(values[3]) !== 0; },
            (current, event, delay) => scene.events.push(`R:${event}:${current}:${floatToBits(delay)}`),
            (current, value) => { scene.events.push(`P:${value}`); parent.postPopBase(current, value); });
        const body = new CurrentBody(scene, hooks);
        parent = body.base;
        body.run(flags);
        assert.equal(scene.output()[0], row[2], `행 ${count}`);
        assert.equal(scene.baseState.depth, 0);
        count += 1;
    }
    assert.equal(count, 212);
    // 확보 실패는 현재 TYPE/프레임과 생성자 조회를 건너뛰고 같은 공통 깊이를 감소한다.
    {
        const tail = new AssetScene();
        prepareTail(tail, 0);
        tail.slot()[10] = 255;
        let parent = null;
        const hooks = tail.hooks();
        hooks.growingIcePostPop = (sid, flags) => postPopGrowingIce(tail.pool, emptyFrames(), sid, flags,
            () => { tail.events.push('A'); tail.slot()[11] = 9; return false; }, undefined, (current, value) => { tail.events.push('P'); parent.postPopBase(current, value); });
        const body = new CurrentBody(tail, hooks);
        parent = body.base;
        body.run();
        assert.deepEqual(tail.events, ['A', 'P']);
        assert.ok(tail.pool.slot(SOURCE)[11] === 9 && tail.baseState.depth === 0);
    }
    // 확보 성공 뒤 원문 TYPE 조회가 생성자 진입보다 먼저 발생한다.
    {
        const tail = new AssetScene();
        prepareTail(tail, 0);
        const hooks = tail.hooks();
        hooks.growingIcePostPop = (sid, flags) => postPopGrowingIce(tail.pool, emptyFrames(), sid, flags, () => { tail.events.push('A'); tail.slot()[10] = 255; return true; }, undefined, undefined);
        const body = new CurrentBody(tail, hooks);
        assert.throws(() => body.run(), /현재 프레임 TYPE/);
        assert.deepEqual(tail.events, ['A']);
        assert.equal(tail.baseState.depth, 1);
    }
});

test('asset-post-pop 너겟: 확보 실패는 난수/생성을 건너뛰고, 늦은 난수/생성자 누락은 앞선 조회·확보·쓰기를 유지한다', skip, () => {
    // 독립된 두 확보가 실패하면 난수/생성 장치를 요구하지 않고 직접 부모로 간다.
    {
        const scene = new AssetScene();
        prepareTail(scene, 2);
        let parent = null;
        const nugget = new RawNuggetPostPop(scene.pool, {
            hasRegular: (sid, event) => { assert.equal(event, 1); scene.events.push('H'); return false; },
            allocate: () => { scene.events.push('A'); return false; },
            damageable: (sid, flags) => parent.damageable.damageablePostPop(sid, flags),
        });
        const body = new CurrentBody(scene, { ...scene.hooks(), nuggetPostPop: (sid, flags) => nugget.postPop(sid, flags) });
        parent = body;
        body.run();
        assert.deepEqual(scene.events, ['H', 'A', 'A', 'D:1']);
        assert.equal(scene.baseState.depth, 0);
    }
    // 첫 확보 뒤 난수가 없으면 앞선 조회/확보를 유지하고 두 번째 예약에 도달하지 않는다.
    {
        const scene = new AssetScene();
        prepareTail(scene, 2);
        const nugget = new RawNuggetPostPop(scene.pool, { hasRegular: () => { scene.events.push('H'); return false; }, allocate: () => { scene.events.push('A'); return true; } });
        const body = new CurrentBody(scene, { ...scene.hooks(), nuggetPostPop: (sid, flags) => nugget.postPop(sid, flags) });
        assert.throws(() => body.run());
        assert.deepEqual(scene.events, ['H', 'A']);
        assert.equal(scene.baseState.depth, 1);
    }
    // 첫 난수 뒤 생성자 누락은 콜백이 바꾼 현재 슬롯도 보존한다.
    {
        const scene = new AssetScene();
        prepareTail(scene, 2);
        const nugget = new RawNuggetPostPop(scene.pool, {
            hasRegular: () => { scene.events.push('H'); return false; },
            allocate: () => { scene.events.push('A'); return true; },
            random: (low, high) => { assert.ok(low === 0 && high === Math.fround(0.3)); scene.events.push('R'); scene.slot()[11] = 9; return 0.25; },
        });
        const body = new CurrentBody(scene, { ...scene.hooks(), nuggetPostPop: (sid, flags) => nugget.postPop(sid, flags) });
        assert.throws(() => body.run());
        assert.deepEqual(scene.events, ['H', 'A', 'R']);
        assert.ok(scene.pool.slot(SOURCE)[11] === 9 && scene.baseState.depth === 1);
    }
});

// ── 신전 ──

// 신전 진단에서 주는 변경 입력만 공급한다(cpppj MutateVortexCurrent). 원문 결과를 계산하지 않는다. (2026-11-10 WB04-E254)
function mutateVortex(scene, bridge, mutation) {
    const raw = scene.slot();
    if (mutation === 1) {
        raw[34] = 8;
        raw[11] = 9;
        raw[40] = 9;
        put(raw, 12, 0x4567, 2);
        put(raw, 36, 0x6789);
        scene.state.loadingDepth = 1;
        bridge.value = 75;
        raw[10] = 75;
    } else if (mutation === 2) {
        raw[34] = 0;
        scene.state.localOwner = 8;
        bridge.value = 70;
        put(raw, 12, 0x7777, 2);
    } else if (mutation >= 3 && mutation <= 5) {
        raw[10] = 75;
        raw[11] = 9;
        raw[40] = 0;
        raw[34] = mutation >= 4 ? 8 : 1;
        scene.state.loadingDepth = 0;
        if (mutation === 4) { scene.state.selected = 62; scene.state.localOwner = 8; }
        if (mutation === 5) bridge.value = 75;
    }
}

test('asset-post-pop 신전: 진단·WORD·중복 없는 목록·Player 9기록이 원본 10.78 936행과 같다', skip, () => {
    const scene = new AssetScene();
    let count = 0;
    // 새 관측은 원문 몸체/WORD/중복 목록이며 외부 장치의 반환/변경 입력만 공급한다.
    for (const row of fixture('vortex-current-x86.tsv')) {
        assert.equal(row.length, 13);
        const input = row[0].split(',');
        const control = row[1].split(',');
        assert.ok(input.length === 14 && control.length === 5);
        // Native 는 각 입력의 풀을 새로 준비하므로 앞선 행의 free 입력을 먼저 해제한다.
        scene.slot()[11] = 0;
        scene.prepare(['originals', ...input, ...new Array(6).fill('0')]);
        scene.slot()[34] = number(control[0]) & 0xff;
        scene.types[75].flags2 = 0x200;
        const bridge = { value: number(control[4]) };
        scene.state.currentBridgeType = () => bridge.value;
        const records = new Uint8Array(9 * STRIDE);
        const mutation = number(control[3]);
        const hooks = scene.hooks();
        const sound = hooks.playImpactSound;
        hooks.playImpactSound = (value) => { if (mutation === 5) mutateVortex(scene, bridge, 5); sound(value); };
        hooks.islandExists = () => number(control[1]) !== 0;
        // Native 의 명시 Owner 장치는 free/contained 와 무관하게 같은 BYTE 만 쓴다.
        hooks.setOwner = (sid, owner) => { scene.events.push(`O:${sid}:${owner}`); scene.slot(sid)[34] = owner & 0xff; };
        hooks.assertion = (expression, file, line) => {
            scene.events.push(`X:${textHex(expression)}:${textHex(file)}:${line}`);
            if (mutation >= 1 && mutation <= 4) mutateVortex(scene, bridge, mutation);
            if (!number(control[2])) throw new DiagnosticStop('명시 Vortex 진단 중단');
        };
        const body = new CurrentBody(scene, hooks);
        body.assets.bindPlayerRecords(records);
        let outcome = 'return';
        try { body.run(number(input[1])); } catch (error) {
            if (!(error instanceof DiagnosticStop)) throw error;
            outcome = 'assertion';
        }
        const output = scene.output();
        const fields = [];
        // 세 관측 Player 의 네 DWORD 와 전체 9기록 Adler 를 함께 확인한다(비활성 필드도 그대로).
        for (const owner of [0, 1, 8]) for (const offset of [0x7c, 0x80, 0x84, 0x88]) fields.push(get(records, owner * STRIDE + offset));
        const label = `행 ${count} 입력 ${row[0]} / ${row[1]}`;
        assert.equal(output[0], row[2], `${label} 사건`);
        assert.equal(output[1], row[3], `${label} 슬롯`);
        assert.equal(output[2], row[4], `${label} 목록`);
        assert.equal(output[4], row[5], `${label} 신전 개수`);
        assert.equal(adler32(records), number(row[6]), `${label} 기록 Adler`);
        assert.equal(fields.join(','), row[7], `${label} 신전 필드`);
        assert.deepEqual([scene.state.loadingDepth, scene.state.selected, scene.state.localOwner, bridge.value, outcome, scene.baseState.depth],
            [number(row[8]), number(row[9]), number(row[10]), number(row[11]), row[12], 0], `${label} 전역`);
        count += 1;
    }
    assert.equal(count, 936);
});

test('asset-post-pop 신전: 최초가 아니면 앞단을 요구하지 않고, 진단·음향·다리 통지 누락은 앞선 효과를 유지한 채 그 위치에서 중단한다', skip, () => {
    // 세 실제 몸체는 flags 0 에서 공통 부모 뒤 앞단 TYPE/장치를 요구하지 않는다.
    for (const kind of [9, 10, 11]) {
        const scene = new AssetScene();
        scene.prepareCurrent(kind, 0);
        scene.slot()[10] = 255;
        scene.state.temples.count = 99;
        new CurrentBody(scene, {}).run(0);
        assert.deepEqual(scene.events, ['D:0']);
        assert.ok(scene.baseState.depth === 0 && scene.state.templeCount === 4);
    }
    // owner 0 진단 누락은 이미 실행한 Damageable/공통 깊이 감소를 되돌리지 않는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9, 2);
        scene.slot()[34] = 0;
        const body = new CurrentBody(scene, {});
        assert.throws(() => body.run(2));
        assert.deepEqual(scene.events, ['D:2']);
        assert.ok(scene.baseState.depth === 0 && scene.state.players[0].newTemple === 0);
    }
    // 섬 존재 진단 장치가 없으면 앞선 WORD/Player/두 목록/프레임은 남는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        const body = new CurrentBody(scene, { ...scene.hooks(), islandExists: () => false });
        assert.throws(() => body.run());
        assert.ok(scene.baseState.depth === 0 && scene.state.templeCount === 5 && scene.state.temples.count === 1 && scene.state.residences.count === 1);
        assert.ok(get(scene.pool.slot(SOURCE), 36, 4) === 27 && get(scene.pool.slot(SOURCE), 12, 2) === 0 && scene.state.players[1].initialTemple === 50);
    }
    // 선택 음향 누락은 TYPE 의 sound 조회 뒤이며 WORD/목록 앞에서 중단한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        const hooks = without(scene.hooks(), 'playImpactSound');
        hooks.typeDword = (sid, offset) => { assert.equal(offset, 0x74); scene.events.push('F'); return 17; };
        const body = new CurrentBody(scene, hooks);
        assert.throws(() => body.run());
        assert.deepEqual(scene.events, ['D:1', 'F']);
        assert.ok(scene.state.templeCount === 5 && get(scene.pool.slot(SOURCE), 12, 2) === 0xabcd && scene.state.temples.count === 0 && scene.baseState.depth === 0);
    }
    // 현재 다리 TYPE 의 WORD 0 저장 뒤 통지가 없으면 Player 최초 신전보다 먼저 중단한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        scene.state.bridgeType = 70;
        const body = new CurrentBody(scene, without(scene.hooks(), 'notifySurface'));
        assert.throws(() => body.run());
        assert.ok(get(scene.pool.slot(SOURCE), 12, 2) === 0 && scene.state.players[1].initialTemple === 0 && scene.state.templeCount === 5 && scene.state.temples.count === 0 && scene.baseState.depth === 0);
    }
});

test('asset-post-pop 신전: 중복 없는 현재 목록(store 뒤 증가·signed 가득 참·음수 위치)과 선택 Form 의 물리 범위·배치 취소', skip, () => {
    const unreachable = (text) => () => { throw new Error(text); };
    // 같은 두 현재 배열/count 에 저장하고 두 번째 최초 통지에서도 SID 중복을 만들지 않는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        const temples = [7, 8, 9, 10];
        const residences = [11, 12, 13, 14];
        const counts = { temples: 0, residences: 0, stores: 0 };
        const first = new SquidPostPopList();
        const second = new SquidPostPopList();
        first.bindCurrent({ storage: () => temples, capacity: () => 4, readCount: () => counts.temples, writeCount: (value) => { assert.equal(temples[counts.temples], 50); counts.stores += 1; counts.temples = value; } });
        second.bindCurrent({ storage: () => residences, capacity: () => 4, readCount: () => counts.residences, writeCount: (value) => { assert.equal(residences[counts.residences], 50); counts.stores += 1; counts.residences = value; } });
        scene.state.sharedTemples = first;
        scene.state.sharedResidences = second;
        const body = new CurrentBody(scene, scene.hooks());
        body.run();
        body.run();
        assert.deepEqual(counts, { temples: 1, residences: 1, stores: 2 });
        assert.deepEqual([temples, residences], [[50, 8, 9, 10], [50, 12, 13, 14]]);
        assert.ok(scene.state.temples.count === 0 && scene.baseState.depth === 0);
    }
    // 원문 signed count>=capacity 는 과거 포인터/중복 검색/저장을 모두 생략한다(두 높은 비트 입력도 signed DWORD 로 비교).
    for (const [count, capacity] of [[4, 4], [0, 0xffffffff], [0x80000000, 0x80000000]]) {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        const list = new SquidPostPopList();
        list.bindCurrent({ storage: unreachable('가득 찬 신전 목록의 미도달 물리'), capacity: () => capacity, readCount: () => count, writeCount: unreachable('가득 찬 목록의 미도달 쓰기') });
        scene.state.sharedTemples = list;
        new CurrentBody(scene, scene.hooks()).run();
        assert.ok(scene.state.residences.count === 1 && scene.baseState.depth === 0, `count ${count}`);
    }
    // 음수 실제 저장 위치는 앞선 음향/WORD/Player 뒤 물리 경계에서 실패한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        const list = new SquidPostPopList();
        list.bindCurrent({ storage: unreachable('미도달 배열'), capacity: () => 4, readCount: () => 0xffffffff, writeCount: unreachable('미도달 count 쓰기') });
        scene.state.sharedTemples = list;
        const body = new CurrentBody(scene, scene.hooks());
        assert.throws(() => body.run(), /신전 목록 음수 저장 위치/);
        assert.ok(scene.state.residences.count === 0 && scene.state.players[1].initialTemple === 50 && get(scene.pool.slot(SOURCE), 12, 2) === 0 && scene.baseState.depth === 0);
    }
    // 선택 SID 상위 비트를 잘라 유효 Form 으로 바꾸거나 잘못된 WORD TYPE 을 조용히 무시하지 않는다(두 물리 오류 모두 선택 Form 소비 위치에서만).
    for (const wide of [false, true]) {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        scene.state.selected = wide ? 65598 : 62;
        put(scene.slot(62), 18, wide ? 70 : 255, 2);
        const body = new CurrentBody(scene, scene.hooks());
        assert.throws(() => body.run(), /물리 범위 오류/);
        assert.ok(scene.state.temples.count === 1 && scene.state.residences.count === 1 && get(scene.pool.slot(SOURCE), 36, 4) === 25 && scene.baseState.depth === 0);
    }
    // 실제 선택 Form 이 신전이면 배치 취소 장치를 두 목록 뒤/프레임 앞에서 요구한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        scene.state.selected = 62;
        const body = new CurrentBody(scene, without(scene.hooks(), 'clearPlacement'));
        assert.throws(() => body.run());
        assert.ok(scene.state.temples.count === 1 && scene.state.residences.count === 1 && get(scene.pool.slot(SOURCE), 36, 4) === 25 && scene.baseState.depth === 0);
    }
    // 현재 소유자 getter: 선택 Form 의 배치 취소는 getter 의 현재 DWORD 와 소유자가 같을 때만 한다(독립 localOwner 는 쓰지 않음).
    for (const [local, cleared] of [[1, true], [0x101, false], [2, false]]) {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        scene.state.selected = 62;
        scene.state.localOwner = 1;
        const body = new CurrentBody(scene, scene.hooks());
        body.assets.bindPlayerRecords(new Uint8Array(9 * STRIDE), () => local);
        body.run();
        assert.equal(scene.events.includes('C'), cleared, `현재 소유자 ${local}`);
    }
});

test('asset-post-pop 신전: boss 재조회·다른 신전 삭제 iterator·slot 내용물/생산 갱신·원소 wrapper·등록 뒤 로딩', skip, () => {
    // 같은 가상 Owner 효과 뒤 현재 boss 가 꺼지면 미도달 iterator 장치를 요구하지 않는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        scene.state.boss = true;
        const hooks = without(scene.hooks(), 'regionNumber', 'destroy');
        const owner = hooks.setOwner;
        hooks.setOwner = (sid, value) => { owner(sid, value); scene.state.boss = false; };
        new CurrentBody(scene, hooks).run();
        assert.ok(scene.state.temples.count === 1 && scene.events.at(-1).startsWith('R:50:1:') && scene.baseState.depth === 0);
    }
    // 일치하는 다른 신전에서만 삭제 장치를 요구하며 앞선 섬 등록/소유자 효과를 유지한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        scene.state.boss = true;
        scene.slot(60)[10] = 70;
        const body = new CurrentBody(scene, without(scene.hooks(), 'destroy'));
        assert.throws(() => body.run());
        assert.ok(scene.state.temples.count === 1 && scene.events.at(-1) === 'O:50:1' && scene.baseState.depth === 0);
    }
    // 다음 SID 는 삭제 전에 선택하며 그 효과로 free 가 된 다음 후보도 원문의 현 위치에서 처리한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        scene.state.boss = true;
        for (const sid of [60, 61, 62]) scene.slot(sid)[10] = 70;
        const destroyed = [];
        const destroy = (sid, flags) => {
            assert.equal(flags, 0x100000);
            destroyed.push(sid);
            if (sid === 60) { scene.slot(61)[11] = 1; scene.slot(62)[11] = 1; }
        };
        new CurrentBody(scene, { ...scene.hooks(), destroy }).run();
        assert.deepEqual(destroyed, [60, 61]);
        assert.equal(scene.baseState.depth, 0);
    }
    // 다른 영역(번호가 다른)의 신전과 buried(extra 8) 신전은 삭제하지 않는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        scene.state.boss = true;
        for (const sid of [60, 61, 62]) scene.slot(sid)[10] = 70;
        scene.slot(61)[40] = 8;
        put(scene.slot(62), 14, floatToBits(99));
        const destroyed = [];
        const hooks = { ...scene.hooks(), destroy: (sid) => destroyed.push(sid), regionNumber: (x) => (x === 99 ? 4 : 3) };
        new CurrentBody(scene, hooks).run();
        assert.deepEqual(destroyed, [60]);
    }
    // 내용물이 있으면 생성자 없이 갱신으로 가며 선택된 갱신 누락은 그 자리에서 실패한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        const hooks = without(scene.hooks(), 'createSlot', 'refreshProduction');
        hooks.hasContent = (sid, type) => { assert.equal(type, 133); scene.events.push('H'); return true; };
        const body = new CurrentBody(scene, hooks);
        assert.throws(() => body.run());
        assert.ok(scene.events.at(-1) === 'H' && get(scene.pool.slot(SOURCE), 36, 4) === 27 && scene.baseState.depth === 0);
    }
    // 내용물 없음 뒤 생성 누락은 앞선 현재 조회/등록/프레임을 보존한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        const hooks = without(scene.hooks(), 'createSlot');
        hooks.hasContent = () => { scene.events.push('H'); return false; };
        const body = new CurrentBody(scene, hooks);
        assert.throws(() => body.run());
        assert.ok(scene.events.at(-1) === 'H' && scene.state.temples.count === 1 && get(scene.pool.slot(SOURCE), 36, 4) === 27 && scene.baseState.depth === 0);
    }
    // 공통 extra 8 생략 뒤 원소 wrapper 의 독립 프레임/예약은 실행하며 free 변경을 거부하지 않는다(윈드 wrapper 는 TYPE+134 조회가 없다).
    for (const kind of [9, 10, 11]) {
        const scene = new AssetScene();
        scene.prepareCurrent(kind);
        scene.slot()[40] = 8;
        const hooks = without(scene.hooks(), 'typeDword', 'addRegular');
        hooks.reserveRegular = () => { scene.events.push('A'); return false; };
        if (kind !== 10) hooks.typeDword = (sid, offset) => { assert.equal(offset, 0x134); scene.events.push('F'); scene.slot(sid)[11] = 9; return 0x87654321; };
        new CurrentBody(scene, hooks).run();
        assert.deepEqual(scene.events, kind === 10 ? ['D:1', 'A'] : ['D:1', 'F', 'A']);
        assert.ok(get(scene.pool.slot(SOURCE), 36, 4) === (kind === 10 ? 25 : 0x87654321) && scene.state.templeCount === 5 && scene.baseState.depth === 0);
    }
    // 섬 등록 효과 뒤 현재 로딩을 읽어 미도달 내용물/갱신 장치를 생략한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        const hooks = without(scene.hooks(), 'hasContent', 'createSlot', 'refreshProduction');
        const registration = hooks.registerTemple;
        hooks.registerTemple = (island, sid) => { registration(island, sid); scene.state.loadingDepth = 1; };
        new CurrentBody(scene, hooks).run();
        assert.ok(scene.state.loadingDepth === 1 && scene.state.temples.count === 1 && scene.baseState.depth === 0);
    }
});

// fixture 와 cpppj 계약이 구별하지 못하는 경계(변형 시험에서 살아남은 입력)를 닫는 계약이다. (2026-11-10 WB04-E254)
//  - 가상 표 별칭(썬 0x50bad0·윈드 신전 0x5197a8)은 주 표와 같은 몸체다. Regular 지연은 단정도 값이고 간헐천 비용은 단정도로 저장된 값을 절삭한다.
//  - 목록 추가는 store 뒤 현재 count 를 다시 읽어 1 을 더한다. 공통 장부가 다른 풀이면 구성 때 거부한다.
//  - 명시 사전 검사: 공통 검사(contained 상태 거부)를 포함하고, 바리케이드는 서버일 때만 차단 장치를 요구하며, 손상된 목록 count 를 거부한다.
//  - Gun: 각도는 signed DWORD, client Pop flags 는 0x10 과 0x40 둘 다(서버 SID 15000 에서 확인), 썬의 로딩 백업은 프레임 DWORD 전체를 쓴다.
//  - 거주 구역은 지면 SID 0 의 슬롯을 읽지 않는다. 간헐천은 종유석 TYPE getter 의 현재값을 쓰고 이웃 프레임을 DWORD 전체로 쓴다.
//  - 신전: 프레임 DWORD 전체 쓰기, 소유자 9 는 초기 신전 필드를 건너뜀, 선택 Form kind 는 WORD, boss 가 아니면 다른 신전을 지우지 않음, iterator 는 SID 2 부터, 모르는 가상 표는 실제 진입에서도 사전 검사가 거부,
//    제네레이터는 최초 flag 에서 현재 TYPE 을 먼저 조회한다. Native finder 는 비유한 좌표를 0 칸으로 본다.
test('asset-post-pop 경계 계약: 가상 표 별칭·단정도 지연/비용·목록 count 재조회·사전 검사·Gun/거주/간헐천/신전의 폭과 관문', skip, () => {
    // 가상 표 별칭: 주 표와 같은 사건·슬롯(가상 표 4바이트 제외)·목록을 낸다.
    for (const [alias, primary, kind] of [[0x50bad0, 0x50b9b8, 3], [0x5197a8, 0x519860, 10]]) {
        const run = (table) => {
            const scene = new AssetScene();
            scene.prepareCurrent(kind);
            scene.types[70].flags1 = 0x1000;
            put(scene.slot(), 0, table);
            new CurrentBody(scene, scene.hooks()).run();
            const output = scene.output();
            output[1] = output[1].slice(8);
            return output;
        };
        assert.deepEqual(run(alias), run(primary));
        assert.ok(RawAssetPostPop.hasDedicatedVtable(alias));
    }
    // Regular 지연은 단정도 값으로 전달한다(장식 0.05·간헐천 0.1·신전 0.08·썬더 신전 0.1).
    for (const [kind, expected] of [[5, 0.05], [8, 0.1], [9, 0.08], [11, 0.1]]) {
        const scene = new AssetScene();
        scene.prepareCurrent(kind);
        const delays = [];
        new CurrentBody(scene, { ...scene.hooks(), addRegular: (sid, event, delay) => delays.push(delay) }).run();
        assert.deepEqual(delays, [Math.fround(expected)], `몸체 ${kind}`);
    }
    // 간헐천 비용은 단정도로 저장된 값을 절삭한다(1 − 1e-10 은 단정도 1.0).
    {
        const scene = new AssetScene();
        scene.prepareCurrent(8);
        scene.types[70].cost = 1 - 1e-10;
        new CurrentBody(scene, scene.hooks()).run();
        assert.equal(get(scene.pool.slot(SOURCE), 12, 2), 1);
    }
    // 목록 추가: 저장소 조회가 count 를 바꾸면 처음 읽은 위치에 저장하고 현재 count 에 1 을 더한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(6);
        scene.reserveMask = 0;
        const values = new Array(20).fill(0);
        let count = 0;
        const current = new SquidPostPopList();
        current.bindCurrent({ storage: () => { count += 10; return values; }, capacity: () => 20, readCount: () => count, writeCount: (value) => { count = value; } });
        scene.state.sharedResidences = current;
        new CurrentBody(scene, { ...scene.hooks(), surface: () => 0 }).run();
        assert.ok(count === 11 && values[0] === 50);
    }
    // 공통 장부만 다른 풀이어도 구성 때 거부한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(0);
        const body = new CurrentBody(scene, {});
        const other = new AssetScene();
        const otherBase = new SquidPostPop(other.pool, other.types, other.baseState);
        assert.throws(() => new RawAssetPostPop(scene.pool, scene.types, otherBase, body.damageable, scene.state, {}), TypeError);
    }
    // 명시 사전 검사: 공통 검사 포함·바리케이드의 서버 관문·손상된 목록 count.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(0, 0);
        scene.slot()[11] = 8;
        assert.throws(() => new CurrentBody(scene, {}).assets.validate(SOURCE, 0), /raw 자산 상태 오류/);
        prepareFence(scene, 0);
        new CurrentBody(scene, {}).assets.validate(SOURCE, 1);
        scene.state.inBattle = true;
        assert.throws(() => new CurrentBody(scene, {}).assets.validate(SOURCE, 1), /바리케이드 최초 프로세스 경계 누락/);
        scene.prepareCurrent(6);
        scene.state.temples.count = 99;
        assert.throws(() => new CurrentBody(scene, scene.hooks()).assets.validate(SOURCE, 1), /count 범위 오류/);
    }
    // Gun 각도는 signed DWORD 로 읽는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(1, 2);
        put(scene.slot(), 36, 0xffffffff);
        new CurrentBody(scene, scene.hooks()).run(2);
        assert.equal(scene.events[0], 'F:-1');
    }
    // 썬의 로딩·client Pop 진단: 서버 SID(15000)에서는 flags 0x10 또는 0x40 이 있을 때만 진단한다.
    for (const [flags, diagnosed] of [[1, false], [0x11, true], [0x41, true], [0x21, false]]) {
        const scene = new AssetScene();
        scene.prepareCurrent(3);
        scene.state.inBattle = true;
        scene.pool.physical(15000).set(scene.pool.slot(SOURCE));
        const lines = [];
        const body = new CurrentBody(scene, { ...scene.hooks(), assertion: (expression, file, line) => lines.push(line) });
        body.assets.postPop(15000, flags);
        assert.deepEqual(lines, diagnosed ? [0x32a] : [], `flags ${flags.toString(16)}`);
        assert.ok(scene.events.includes('P:15000'));
    }
    // 썬의 로딩 백업은 WORD 를 프레임 DWORD 전체에 쓴다(상위 WORD 를 0 으로).
    {
        const scene = new AssetScene();
        scene.prepareCurrent(3);
        scene.state.loadingDepth = 1;
        scene.types[70].flags1 = 0x1000;
        put(scene.slot(), 36, 0x12345678);
        new CurrentBody(scene, scene.hooks()).run();
        assert.equal(get(scene.pool.slot(SOURCE), 36, 4), 0xabcd);
        assert.equal(get(scene.pool.slot(SOURCE), 12, 2), 0xabcd);
    }
    // 거주 구역: 지면 SID 0 이면 슬롯 0 의 소유자를 읽지 않는다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(6);
        scene.pool.physical(0)[34] = 2;
        new CurrentBody(scene, { ...without(scene.hooks(), 'setOwner'), surface: () => 0 }).run();
        assert.equal(scene.state.residences.count, 1);
        scene.pool.physical(0)[34] = 0;
    }
    // 간헐천: 종유석 TYPE getter 의 현재값(94)으로 후보 60 만 다시 Pop 하고, 이웃 프레임은 DWORD 전체를 8 로 쓴다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(8);
        scene.state.currentIslandType = () => 200;
        scene.state.currentStalagType = () => 94;
        put(scene.slot(60), 36, 0x12345678);
        new CurrentBody(scene, scene.hooks()).run();
        assert.deepEqual(scene.events.filter((event) => event[0] === 'U' || event[0] === 'V'), ['U:60:0', 'V:60:64']);
        assert.equal(get(scene.pool.slot(60), 36, 4), 8);
    }
    // 신전: 프레임은 DWORD 전체로 쓴다(윈드 wrapper 는 TYPE+0x130 값 27 그대로).
    {
        const scene = new AssetScene();
        scene.prepareCurrent(10);
        put(scene.slot(), 36, 0x11223344);
        new CurrentBody(scene, scene.hooks()).run();
        assert.equal(get(scene.pool.slot(SOURCE), 36, 4), 27);
    }
    // 신전: 소유자 9(Player 범위 밖)는 초기 신전 필드를 건너뛰고 계속한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        scene.slot()[34] = 9;
        new CurrentBody(scene, scene.hooks()).run();
        assert.ok(scene.state.players.every((player) => player.initialTemple === 0) && scene.state.temples.count === 1 && scene.baseState.depth === 0);
    }
    // 신전: 선택 Form 의 kind 는 WORD 전체로 TYPE 표를 조회한다(0x146 은 범위 오류).
    {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        scene.state.selected = 62;
        put(scene.slot(62), 18, 0x146, 2);
        assert.throws(() => new CurrentBody(scene, scene.hooks()).run(), /물리 범위 오류/);
    }
    // 신전: boss 가 아니면 같은 영역의 다른 신전을 지우지 않는다. boss 면 SID 2 부터 훑는다(슬롯 3 의 신전도 지움).
    for (const boss of [false, true]) {
        const scene = new AssetScene();
        scene.prepareCurrent(9);
        scene.state.boss = boss;
        const low = scene.pool.physical(3);
        const saved = low.slice();
        low.fill(0);
        low[10] = 70;
        const destroyed = [];
        new CurrentBody(scene, { ...scene.hooks(), destroy: (sid) => destroyed.push(sid) }).run();
        low.set(saved);
        assert.deepEqual(destroyed, boss ? [3] : []);
    }
    // 모르는 가상 표는 실제 진입에서도 사전 검사가 거부하고, 제네레이터는 최초 flag 에서 현재 TYPE 을 먼저 조회한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(0);
        put(scene.slot(), 0, 0x11111111);
        const body = new CurrentBody(scene, scene.hooks());
        assert.throws(() => body.assets.postPop(SOURCE, 0), /가상 표\/타입 미복원/);
        assert.deepEqual(scene.events, []);
        scene.prepareCurrent(0);
        scene.slot()[10] = 255;
        assert.throws(() => new CurrentBody(scene, scene.hooks()).run(), /자산 postPop 현재 TYPE 범위 오류/);
    }
    // Native finder 는 비유한 좌표를 0 칸으로 보고 typed finder 는 거부한다. visitor 가 없으면 거부한다.
    {
        const scene = new AssetScene();
        scene.prepareCurrent(8);
        const hash = new SquidHash();
        const raw = scene.slot(60);
        raw.fill(0);
        raw[10] = 94;
        put(raw, 14, floatToBits(0.5));
        put(raw, 18, floatToBits(52));
        hash.setCell(1, 0, 26, 60);
        Object.assign(scene.types[94], { footX: 1, footY: 1 });
        const visited = [];
        makeAssetPostPopFinder(scene.pool, hash, scene.types, true)(NaN, 52, (sid) => visited.push(sid));
        assert.deepEqual(visited, [60]);
        assert.throws(() => makeAssetPostPopFinder(scene.pool, hash, scene.types)(NaN, 52, () => {}), /CRT/);
        assert.throws(() => makeAssetPostPopFinder(scene.pool, hash, scene.types)(0, 52, null), /미연결/);
    }
});
