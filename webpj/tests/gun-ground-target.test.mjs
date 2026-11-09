// 별도 지상 표적 검색 raw-gun-ground-target.js(RawGunGroundTarget·GunGroundTargetGlobals)의 지상 전용 필터·실제 해시·거리·Depend 조회 경계를 cpppj 계약으로 대조한다(cpppj `GunGroundTargetTests` 10건).
//  - cpppj 시험은 원본 fixture 없이 손으로 쓴 계약 시험이다(원문 주석 "작성·컴파일만 한다"). 실제 SidPool(비서버)·SquidHash 1단계 버킷·SquidFactory.take(서버 대역 15000..15002)·기존 RawGunTarget 공통 필터 위에서 같은 장면을 옮겼다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E211: 지상 표적 검색 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash, SQUID_HASH_SCALES } from '../www/core/squid-hash.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { RawGunTarget } from '../www/core/raw-gun-target.js';
import { RawGunGroundTarget, GunGroundTargetGlobals } from '../www/core/raw-gun-ground-target.js';

// 두 원본 진단의 식이다. (2026-11-09 WB04-E211)
const DEPEND_TEXT = 'inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy';
const ALLY_TEXT = 'shootMyFriends || me->isAlly(me->getPlayerId())';

// 예외가 나는지 본다. (2026-11-09 WB04-E211)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실제 서버 슬롯/네 단계 해시/기존 Gun 과 현재 진단 전역을 함께 준비한 장면이다(cpppj GroundScene). (2026-11-09 WB04-E211)
class GroundScene {
    constructor() {
        this.pool = new SidPool(32768, { server: false });
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 1, footY: 1, maxHitPoints: 0, group: 0, hotspotX: 0, hotspotY: 0 }));
        this.targetTypes = Array.from({ length: 188 }, () => ({ range: 0, theme: 0, directionFrames: [0, 0, 0, 0], heightExtension: 0 }));
        this.shapes = Array.from({ length: 188 }, () => ({ loaded: false, frameCount: 0, frames: [] }));
        this.state = {
            friendlyFire: false, checking: false, showRanges: false, sunCannonType: 128, sunFenceType: 136, windArcherType: 140, thunderArcherType: 148,
            windHalfAngle: 30, scaleX: 16, scaleY: 11, allyMasks: new Array(9).fill(0), playerFlags: new Array(9).fill(0), angles: {}
        };
        this.common = new RawGunTarget(this.pool, this.hash, this.types, this.targetTypes, this.shapes, this.state, { drawRange: () => assert.fail('지상 검색은 사정선을 그리지 않는다') });
        this.globals = new GunGroundTargetGlobals();
        this.globals.boss = true;
        this.diagnostics = [];
        this.duringDiagnostic = null;
        this.globals.assertion = (text, line) => {
            assert.equal(text, line === 0x158 ? DEPEND_TEXT : ALLY_TEXT);
            this.diagnostics.push(line);
            if (this.duringDiagnostic) this.duringDiagnostic(line);
        };
        this.types[90].group = 3;
        this.targetTypes[90].range = 10;
        this.types[80].flags1 = 0x10;
        this.types[81].flags1 = 0x10;
        this.types[72].group = 3;
        assert.ok(this.pool.allocate(2) === 5 && this.pool.allocate(2) === 6);
        const factory = new SquidFactory(this.pool, this.types);
        // 원본의 서버 대역에만 실제 take 하며 결과 SID 를 사후에 덮어쓰지 않는다.
        for (const sid of [15000, 15001, 15002]) assert.equal(factory.take(80, sid), sid);
        this.state.allyMasks[1] = 1 << 1;
    }

    // 명시 raw 객체를 실제 1단계 버킷 머리에 넣고 next 를 원본 순서로 저장한다. (2026-11-09 WB04-E211)
    object(sid, x, y, type = 80) {
        const raw = this.pool.allocated(sid);
        raw.fill(0);
        raw[10] = type;
        raw[34] = 2;
        put(raw, 26, 1000);
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        const scale = SQUID_HASH_SCALES[1];
        const cellX = Math.trunc(Math.trunc(Math.fround(x)) / scale);
        const cellY = Math.trunc(Math.trunc(Math.fround(y)) / scale);
        put(raw, 4, this.hash.cell(1, cellX, cellY), 2);
        this.hash.setCell(1, cellX, cellY, sid);
    }

    // 실제 현재 Depend Form 과 next 를 명시 입력한다. (2026-11-09 WB04-E211)
    child(sid, type, next = 0) {
        const raw = this.pool.allocated(sid);
        raw.fill(0);
        raw[10] = type;
        put(raw, 4, next, 2);
    }

    // 같은 전역 객체를 빌려 호출 중 바뀐 값도 다음 조회 위치에서 읽는다. (2026-11-09 WB04-E211)
    actions() { return new RawGunGroundTarget(this.pool, this.hash, this.types, this.targetTypes, this.common, this.state, this.globals); }

    // 명시 Pop 좌표/실제 1×1 중심/방향 0/프레임 0 의 원본 문맥이다. (2026-11-09 WB04-E211)
    query() { return { x: 50.5, y: 50.5, centerX: 50, centerY: 50, type: 90, direction: 0, owner: 1, frame: 0 }; }
}

test('GunGroundTarget: 비행/벌룬은 공통 필터 전에 제외하고 일반 지상 표적은 실제 공통 필터로 간다(AirborneAndCommonTargetFilters)', () => {
    const scene = new GroundScene();
    scene.object(15000, 52, 50);
    const actions = scene.actions();
    assert.ok(actions.pool === scene.pool && actions.mayTarget(scene.query(), 15000));
    scene.types[80].flags2 = 0x100000;
    assert.ok(!actions.mayTarget(scene.query(), 15000));
    scene.types[80].flags2 = 0x20000;
    assert.ok(!actions.mayTarget(scene.query(), 15000));
    scene.types[80].flags2 = 0;
    scene.pool.allocated(15000)[34] = 1;
    assert.ok(!actions.mayTarget(scene.query(), 15000));
    scene.state.friendlyFire = true;
    assert.ok(actions.mayTarget(scene.query(), 15000));
});

test('GunGroundTarget: Depend 는 자산 70 이상을 건너뛰고 현재 전역 TYPE 의 첫 Form 을 찾는다(WalkerUsesActualDependChainAndCurrentType)', () => {
    const scene = new GroundScene();
    scene.object(15000, 52, 50);
    scene.types[80].flags2 = 0x10000;
    scene.child(5, 80, 6);
    scene.child(6, 1);
    put(scene.pool.allocated(15000), 6, 5, 2);
    assert.ok(!scene.actions().mayTarget(scene.query(), 15000));
    scene.globals.dependType = 2;
    assert.ok(scene.actions().mayTarget(scene.query(), 15000));
    scene.globals.dependType = 80;
    assert.ok(scene.actions().mayTarget(scene.query(), 15000));
    scene.globals.dependType = 1;
    scene.pool.allocated(6)[11] = 3;
    assert.ok(!scene.actions().mayTarget(scene.query(), 15000));
});

test('GunGroundTarget: 제외 TYPE 72 는 손상된 Depend 체인도 조회하기 전에 반환한다(ExcludedSourcePrecedesDependencyLookup)', () => {
    const scene = new GroundScene();
    scene.object(15000, 52, 50);
    scene.types[80].flags2 = 0x10000;
    put(scene.pool.allocated(15000), 6, 65535, 2);
    const query = scene.query();
    query.type = 72;
    assert.ok(!scene.actions().mayTarget(query, 15000));
    query.type = 90;
    assert.ok(throws(() => scene.actions().mayTarget(query, 15000)));
});

test('GunGroundTarget: dead 부모 진단 뒤 head 를 다시 읽고 metamap/boss 에서는 진단만 생략한다(DependencyDiagnosticAndCurrentHead)', () => {
    const scene = new GroundScene();
    scene.object(15000, 52, 50);
    scene.types[80].flags2 = 0x10000;
    scene.globals.checking = 1;
    scene.globals.boss = false;
    scene.pool.allocated(15000)[11] = 2;
    scene.child(5, 1);
    scene.duringDiagnostic = (line) => {
        assert.equal(line, 0x158);
        put(scene.pool.allocated(15000), 6, 5, 2);
    };
    assert.ok(!scene.actions().mayTarget(scene.query(), 15000));
    assert.deepEqual(scene.diagnostics, [0x158]);
    put(scene.pool.allocated(15000), 6, 0, 2);
    scene.diagnostics = [];
    scene.globals.metamap = true;
    assert.ok(scene.actions().mayTarget(scene.query(), 15000) && scene.diagnostics.length === 0);
    scene.globals.metamap = false;
    scene.globals.boss = true;
    assert.ok(scene.actions().mayTarget(scene.query(), 15000) && scene.diagnostics.length === 0);
});

test('GunGroundTarget: 첫 일치 이후 체인은 읽지 않고 일치 없이 순환한 경우만 명시 실패한다(DependencyFirstMatchAndCycleBoundary)', () => {
    const scene = new GroundScene();
    scene.object(15000, 52, 50);
    scene.types[80].flags2 = 0x10000;
    scene.child(5, 1, 5);
    put(scene.pool.allocated(15000), 6, 5, 2);
    assert.ok(!scene.actions().mayTarget(scene.query(), 15000));
    scene.globals.dependType = 2;
    assert.ok(throws(() => scene.actions().mayTarget(scene.query(), 15000)));
});

test('GunGroundTarget: 실제 거리 제곱이 range² 와 같으면 제외하고 조금 큰 사거리에서만 선택한다(ActualHashStrictSquaredRange)', () => {
    const scene = new GroundScene();
    scene.object(15000, 52, 50);
    scene.targetTypes[90].range = 2;
    assert.equal(scene.actions().find(50.5, 50.5, 90, 1), 0);
    scene.targetTypes[90].range = Math.fround(2.01);
    assert.equal(scene.actions().find(50.5, 50.5, 90, 1), 15000);
});

test('GunGroundTarget: 같은 해시 체인의 등거리 후보는 처음 반환한 SID 를 유지한다(ActualHashKeepsFirstEqualCandidate)', () => {
    const scene = new GroundScene();
    scene.object(15000, 52, 50);
    scene.object(15001, 52, 50);
    assert.equal(scene.actions().find(50.5, 50.5, 90, 1), 15001);
});

test('GunGroundTarget: 현재 Player 동맹은 첫 후보 준비 뒤와 각 공통 필터에서 새로 읽고, 진단이 복귀하면 계속한다(CurrentAllianceAndDiagnosticContinuation)', () => {
    const scene = new GroundScene();
    scene.object(15000, 52, 50);
    scene.object(15001, 55, 50);
    let calls = 0;
    scene.state.allyMaskRead = (owner) => {
        assert.equal(owner, 1);
        calls += 1;
        return calls === 2 ? (1 << 1) | (1 << 2) : 1 << 1;
    };
    assert.ok(scene.actions().find(50.5, 50.5, 90, 1) === 15001 && calls === 3);
    scene.state.allyMaskRead = undefined;
    scene.state.allyMasks[1] = 0;
    scene.duringDiagnostic = (line) => {
        assert.equal(line, 0x2fc);
        scene.state.allyMasks[1] = 1 << 1;
    };
    assert.ok(scene.actions().find(50.5, 50.5, 90, 1) !== 0);
    assert.deepEqual(scene.diagnostics, [0x2fc]);
});

test('GunGroundTarget: 원본 0.99999 보정은 단순 ceil 과 다르며 검색 문맥은 실제 발자국 절반을 뺀다(SnapBiasAndAsymmetricSourceFootprint)', () => {
    const scene = new GroundScene();
    scene.object(15000, Math.fround(50.4), Math.fround(50.4));
    scene.targetTypes[90].range = Math.fround(0.9);
    assert.equal(scene.actions().find(Math.fround(50.500004), 50.5, 90, 1), 15000);
    const wide = new GroundScene();
    wide.types[90].footX = 2;
    wide.types[90].footY = 4;
    wide.object(15000, 52, 50);
    wide.targetTypes[90].range = Math.fround(2.01);
    assert.equal(wide.actions().find(51, 52, 90, 1), 15000);
});

test('GunGroundTarget: 계약 — 칸 보정은 0.99999f 이다(중심 소수부 0.00005 는 다음 칸 51, 일반 ceil 보정 0.9999f 라면 50; cpppj 시험이 구분하지 않는 경계)', () => {
    // 사거리 0.9 → 검색 사각형은 중심 칸 하나다. 50 칸이면 (50.4, 50.4) 객체가 잡히고 51 칸이면 잡히지 않는다.
    const scene = new GroundScene();
    scene.object(15000, Math.fround(50.4), Math.fround(50.4));
    scene.targetTypes[90].range = Math.fround(0.9);
    assert.equal(scene.actions().find(Math.fround(50.50005), 50.5, 90, 1), 0);
    // 대조점: 중심 소수부 0 이면 50.99999 → 50 칸이라 잡힌다.
    assert.equal(scene.actions().find(50.5, 50.5, 90, 1), 15000);
});

test('GunGroundTarget: 이 별도 검색은 사정선/최종 표적 그림을 내지 않고 거부된 보행 운반체는 이기지 않는다(NoRangeDrawAndRejectedWalkerDoesNotWin)', () => {
    const scene = new GroundScene();
    scene.object(15000, 52, 50);
    scene.object(15001, 55, 50, 81);
    scene.types[80].flags2 = 0x10000;
    scene.child(5, 1);
    put(scene.pool.allocated(15000), 6, 5, 2);
    scene.state.showRanges = true;
    assert.equal(scene.actions().find(50.5, 50.5, 90, 1), 15001);
});

test('GunGroundTarget: 계약 — 10.72 풀·다른 풀의 공통 필터·TYPE 표 크기·전역 형식을 거부하고, 진단 장치 누락은 묵시 통과가 아니다', () => {
    const scene = new GroundScene();
    const other = new RawGunTarget(new SidPool(24000), scene.hash, scene.types, scene.targetTypes, scene.shapes, scene.state);
    assert.throws(() => new RawGunGroundTarget({ edition: 'cd1072' }, scene.hash, scene.types, scene.targetTypes, scene.common, scene.state, scene.globals), TypeError);
    assert.throws(() => new RawGunGroundTarget(scene.pool, scene.hash, scene.types, scene.targetTypes, other, scene.state, scene.globals), TypeError);
    assert.throws(() => new RawGunGroundTarget(scene.pool, scene.hash, scene.types.slice(1), scene.targetTypes, scene.common, scene.state, scene.globals), TypeError);
    assert.throws(() => new RawGunGroundTarget(scene.pool, scene.hash, scene.types, scene.targetTypes, scene.common, scene.state, {}), TypeError);
    scene.object(15000, 52, 50);
    scene.globals.assertion = null;
    scene.state.allyMasks[1] = 0;
    assert.ok(throws(() => scene.actions().find(50.5, 50.5, 90, 1)));
    assert.ok(throws(() => scene.actions().find(50.5, 50.5, 188, 1)));
    assert.ok(throws(() => scene.actions().find(Number.NaN, 50.5, 90, 1)));
});
