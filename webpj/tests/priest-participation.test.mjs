// 사제 참여 raw-priest-participation.js(RawPriestParticipation)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priest-participation-x86.tsv(10.78 전용 단일 판본) 304행 — 소유자·활성 개수·목록·현재 타입 다섯·노드(풀/체인/버킷 입력) → 찾은 SID, 갱신 뒤 소유자 상태 9칸, 전체 24000슬롯 풀의 Adler-32, 목록 checksum.
//    실제 live 목록·Carrier 종속 조회·일반 typed finder 를 같은 풀에서 쓰며 조회가 풀·목록·버킷 머리를 바꾸지 않음도 확인한다.
//  - 계약: PriestParticipation_PreservesLazyReadsAndDwordOwners·RejectsDifferentPoolAndBrokenContainedChain·PriestOwnerState_ParticipationWritesSharedTableWithoutChangingList.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E68: 사제 참여 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawCarrierPreDestroy } from '../www/core/raw-carrier-pre-destroy.js';
import { PriestPostPopState, resetPriestOwnerState } from '../www/core/raw-priest-post-pop.js';
import { RawPriestParticipation } from '../www/core/raw-priest-participation.js';

// 원본 관측 행이다(단일 판본 파일). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E68)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priest-participation-x86.tsv'));

// 독립 기계어 관찰 행 수, 풀 용량, 사용하지 않는 꼬리도 free 로 직접 순회하는 실제 세션의 슬롯 폭이다. (2026-11-08 WB04-E68)
const ROWS = 304;
const CAPACITY = 24000;
const STRIDE = 50;

// 예외가 나는지 본다. (2026-11-08 WB04-E68)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 단정도 비트를 수로 바꾼다. (2026-11-08 WB04-E68)
const bitsToFloat = (bits) => {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
};

// 쉼표로 구분한 부호 있는 수 목록을 읽는다(해시 단계 -1 과 DWORD 비트를 한 입력 파서로 읽음). (2026-11-08 WB04-E68)
const values = (text) => text.split(',').map(Number);

// 소유자 상태 9칸을 원본과 같은 쉼표 문자열로 만든다. (2026-11-08 WB04-E68)
const states = (priests) => priests.ownerState.join(',');

// 조회 전용 Carrier 몸체는 실행하면 안 되는 삭제 효과를 빈 성공이 아닌 명시 예외로 둔다. (2026-11-08 WB04-E68)
function queryOnlyCarrierHooks() {
    return {
        validateDamageable: () => { throw new Error('참여조회는 Carrier 하위 삭제 검사를 호출하지 않는다'); },
        damageablePre: () => { throw new Error('참여조회는 Carrier 삭제를 호출하지 않는다'); },
        dependentFound: () => { throw new Error('참여조회는 Carrier 후처리를 호출하지 않는다'); }
    };
}

// 매 행 새 입력을 만들되 제품 모듈과 현재 타입 참조는 같은 인스턴스로 유지하는 장면이다. (2026-11-08 WB04-E68)
class ParticipationScene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
        this.priests = new PriestPostPopState();
        this.contained = new ContainedFinderState();
        this.carrier = new RawCarrierPreDestroy(this.pool, this.contained, queryOnlyCarrierHooks());
        this.state = { priestType: 158, altarType: 164, daisType: 165 };
        this.participation = new RawPriestParticipation(this.pool, this.hash, this.types, this.priests, this.carrier, this.state);
        this.dirty = new Set();
        this.initialized = false;
    }

    // 전체 풀을 0 으로 비우고 모든 슬롯을 free 로 둔다(풀 외곽을 짧은 가짜 범위로 대체하지 않는다). 이후에는 앞 행이 쓴 슬롯만 되돌린다. (2026-11-08 WB04-E68)
    clear() {
        if (!this.initialized) {
            // 최초에는 용량 전체를 free 로 직접 초기화한다.
            for (let sid = 0; sid < CAPACITY; sid += 1) {
                const raw = this.pool.physical(sid);
                raw.fill(0);
                raw[11] = 1;
            }
            this.initialized = true;
        } else {
            // 앞 행이 노드로 쓴 슬롯만 free 로 되돌린다.
            for (const sid of this.dirty) {
                const raw = this.pool.physical(sid);
                raw.fill(0);
                raw[11] = 1;
            }
        }
        this.dirty.clear();
    }

    // 슬롯과 체인을 동일한 명시 입력으로 준비하고 결과는 제품 몸체가 계산하게 한다.
    prepare(row) {
        const current = values(row[3]);
        assert.equal(current.length, 5);
        this.state.priestType = current[0] >>> 0;
        this.state.altarType = current[1] >>> 0;
        this.state.daisType = current[2] >>> 0;
        this.contained.containedType = current[3] >>> 0;
        this.contained.boss = current[4] !== 0;
        this.contained.checkingDead = 0;
        this.priests.priests.count = Number(row[1]);
        this.priests.priests.entries = values(row[2]).map((value) => value >>> 0);
        // 실제 기계어가 쓴 상태와 정확히 같은 초기 9 DWORD 이다.
        for (let index = 0; index < this.priests.ownerState.length; index += 1) this.priests.ownerState[index] = 0x9000 + index;
        this.clear();
        this.hash.reset();
        this.types.forEach((type) => { type.flags2 = 0; type.footX = 0; type.footY = 0; });
        // 명시 입력의 배열 순서대로 머리를 삽입한다. 포획 여부/선택 결과는 입력하지 않는다.
        for (const entry of row[4].split(';')) {
            const node = values(entry);
            assert.equal(node.length, 16);
            const number = node[0] & 0xffff;
            const slot = this.pool.physical(number);
            this.dirty.add(number);
            slot.fill(0);
            const type = node[1];
            slot[10] = type;
            slot[11] = node[2];
            slot[34] = node[3];
            put(slot, 12, node[4] >>> 0, 2);
            slot[40] = node[5];
            put(slot, 6, node[6] >>> 0, 2);
            put(slot, 4, node[7] >>> 0, 2);
            put(slot, 14, node[10] >>> 0);
            put(slot, 18, node[11] >>> 0);
            if (node[8] || node[9] || type === this.contained.containedType) {
                put(slot, 18, node[8] >>> 0, 2);
                put(slot, 20, node[9] >>> 0, 2);
            }
            assert.ok(type < this.types.length);
            this.types[type].flags2 = node[15] >>> 0;
            this.types[type].footX = node[13];
            this.types[type].footY = node[14];
            if (node[12] >= 0) {
                const level = node[12];
                const x = bitsToFloat(node[10]);
                const y = bitsToFloat(node[11]);
                put(slot, 4, this.hash.bucket(level, x, y), 2);
                this.hash.setBucket(level, x, y, number);
                slot[33] = level;
            }
        }
    }

    // 풀 전체의 독립 사본이다. (2026-11-08 WB04-E68)
    bytes() {
        return this.pool.bytes();
    }
}

test('PriestParticipation: 원본 10.78 304행의 Find/Refresh 결과·소유자 상태·풀 checksum·목록 checksum 이 같고 조회는 풀·목록·버킷을 바꾸지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new ParticipationScene();
    let checked = 0;
    // 각 행에서 전체 풀/목록/버킷을 다시 준비해 앞 행 출력에 의존하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 9);
        scene.prepare(row);
        const player = Number(row[0]);
        const before = scene.bytes();
        const entries = [...scene.priests.priests.entries];
        const initial = [...scene.priests.ownerState];
        // 모든 실제 해시 배열을 보존해 조회가 등록 체인을 변경하지 않는지 검사한다.
        const heads = [0, 1, 2, 3].map((level) => scene.hash.entries(level));
        const result = scene.participation.find(player);
        assert.equal(result, Number(row[5]), `행 ${checked}`);
        assert.deepEqual(scene.priests.ownerState, initial, `행 ${checked}`);
        scene.participation.refresh(player);
        assert.equal(states(scene.priests), row[6], `행 ${checked}`);
        const after = scene.bytes();
        assert.ok(Buffer.compare(Buffer.from(before), Buffer.from(after)) === 0, `행 ${checked}`);
        assert.equal(adler32(after), Number(row[7]), `행 ${checked}`);
        assert.ok(scene.priests.priests.count === Number(row[1]) && JSON.stringify(scene.priests.priests.entries) === JSON.stringify(entries), `행 ${checked}`);
        const list = new Uint8Array(16);
        // 개수 밖 꼬리까지 원본 목록 checksum 과 비교한다.
        entries.forEach((entry, index) => put(list, index * 4, entry));
        assert.equal(adler32(list), Number(row[8]), `행 ${checked}`);
        // 각 단계의 버킷 머리도 모두 읽기 전용이다.
        [0, 1, 2, 3].forEach((level) => assert.deepEqual(scene.hash.entries(level), heads[level], `행 ${checked}`));
        assert.ok(scene.participation.pool === scene.pool && scene.participation.priests === scene.priests);
        checked += 1;
    }
    assert.equal(checked, ROWS);
    assert.equal(STRIDE * CAPACITY, scene.bytes().length);
});

test('PriestParticipation: live 의 이른 성공 뒤 손상된 후속 목록/타입/다이스 전역을 읽지 않고 전체 DWORD 소유자 폭을 유지한다', () => {
    const scene = new ParticipationScene();
    scene.priests.priests.entries = [50, 0x10032];
    scene.priests.priests.count = 2;
    const live = scene.pool.physical(50);
    live.fill(0);
    live[10] = 255;
    live[11] = 9;
    live[34] = 1;
    scene.state.daisType = 0xffffffff;
    assert.equal(scene.participation.find(1), 50);
    scene.participation.refresh(1);
    assert.equal(scene.priests.ownerState[1], 1);
    assert.ok(throws(() => scene.participation.find(257)));
    scene.priests.priests.count = 0;
    live[11] = 0;
    assert.ok(throws(() => scene.participation.find(1)));
    live[10] = 82;
    live[11] = 1;
    scene.priests.ownerState.fill(77);
    scene.participation.refresh(0);
    scene.participation.refresh(9);
    scene.participation.refresh(0xffffffff);
    assert.ok(scene.priests.ownerState.every((value) => value === 77));
});

test('PriestParticipation: 같은 풀 Carrier 연결만 허용하고 실제 접근한 종속 순환을 조용한 실패로 돌리지 않는다', () => {
    const scene = new ParticipationScene();
    const other = new SidPool(CAPACITY, { server: false });
    const foreign = new RawCarrierPreDestroy(other, scene.contained, queryOnlyCarrierHooks());
    assert.ok(throws(() => new RawPriestParticipation(scene.pool, scene.hash, scene.types, scene.priests, foreign, scene.state)));
    assert.ok(throws(() => new RawPriestParticipation(scene.pool, scene.hash, scene.types.slice(1), scene.priests, scene.carrier, scene.state)));
    assert.ok(throws(() => new RawPriestParticipation(scene.pool, scene.hash, scene.types, scene.priests, scene.carrier, scene.state, { pool: other, daisAt: () => 0 })));
    const parent = scene.pool.physical(5);
    parent.fill(0);
    parent[10] = 82;
    put(parent, 6, 10, 2);
    scene.types[82].flags2 = 0x10000;
    const contained = scene.pool.physical(10);
    contained.fill(0);
    contained[10] = 6;
    put(contained, 4, 10, 2);
    put(contained, 18, 157, 2);
    put(contained, 20, 0x20, 2);
    assert.ok(throws(() => scene.participation.find(1)));
});

test('PriestOwnerState: 참여 Find/Refresh 는 같은 공유 9 DWORD 만 쓰고 사제 목록/개수와 독립 fallback 을 보존한다', () => {
    const scene = new ParticipationScene();
    const shared = new Array(9).fill(31);
    scene.priests.ownerState.fill(77);
    scene.priests.sharedOwnerState = shared;
    scene.priests.priests.entries = [50, 60, 77];
    scene.priests.priests.count = 1;
    const live = scene.pool.physical(50);
    live.fill(0);
    live[10] = 158;
    live[34] = 1;
    const entries = [...scene.priests.priests.entries];
    const fallback = [...scene.priests.ownerState];
    scene.participation.refresh(1);
    assert.ok(shared[1] === 0 && shared[0] === 31 && JSON.stringify(scene.priests.ownerState) === JSON.stringify(fallback));
    // 원문 live 목록은 타입을 필터링하지 않으며 refresh 가 현재 타입으로 1 을 쓴다.
    live[10] = 82;
    scene.participation.refresh(1);
    assert.ok(shared[1] === 1 && JSON.stringify(scene.priests.ownerState) === JSON.stringify(fallback));
    resetPriestOwnerState(scene.priests.currentOwnerState(), 1);
    assert.ok(shared[1] === 0 && JSON.stringify(scene.priests.priests.entries) === JSON.stringify(entries) && scene.priests.priests.count === 1 && JSON.stringify(scene.priests.ownerState) === JSON.stringify(fallback));
});
