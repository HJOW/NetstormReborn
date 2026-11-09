// 다리 가상 삭제 분배기 raw-squid-destroy-dispatch.js(RawSquidDestroyDispatch)를 cpppj `DestroyDispatchTests` 5건(손 시험, 원본 fixture 없음)과 웹 계약으로 대조한다.
//  - 4/5 경계·signed 음수·hard/보통·debugKeep·Graph 비활성의 판정, editor/권한/abstract/buried 단락이 무효 Graph·프레임 읽기를 건너뜀, 살아 있는 raw 프레임·가상 표 변경과 공통 몸체 직접 호출 구별,
//    무효 Graph/프레임의 쓰기 전 예외, 다른 풀/Graph 공간 연결 거부.
//  - 웹 계약: 현재 권한/편집기 전역 대여(`bindCurrentModes`)가 mode 의 값을 덮고 호출마다 읽힘, 모드 객체 변경의 다음 호출 반영.
//  - CD 10.72 판(BYTE 프레임 등)은 웹 범위 밖이다. 같은 PC 의 cpppj 통합 시험 실행 파일에서 `destroy_dispatch` 5건 통과를 확인했다(게임 실행 아님).
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E214: 다리 가상 삭제 분배 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, floatToBits } from './raw-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { RawGraph } from '../www/core/raw-graph.js';
import { DEFAULT_BRIDGE_DECAY_MODE } from '../www/core/bridge-decay.js';
import { RawSquidDestroyDispatch } from '../www/core/raw-squid-destroy-dispatch.js';
import { adler32 } from './raw-support.mjs';

// 실제 다리 타입과 raw free 비트, 공통 가상 표이다. 프레임은 보통 0·hard 1 의 합성 입력이다. (2026-11-10 WB04-E214)
const BRIDGE = 82;
const FREE = 1;
const COMMON_VTABLE = 0x11010000;

// 예외가 나는지 본다. (2026-11-10 WB04-E214)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-10 WB04-E214)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 가상 판정을 검사하며 공통 pre/post 깊이만 완료하는 최소 삭제 장면이다(cpppj DispatchScene). (2026-11-10 WB04-E214)
class DispatchScene {
    // surfaces 는 유효 그래프 0 의 signed WORD 수, hard 는 현재 프레임(1 이 단단함), graphEnabled 가 거짓이면 Graph 없이 구성한다.
    constructor(surfaces, hard, graphEnabled = true) {
        this.pool = new SidPool(32768, { server: true });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0, zOrder: 0, constructorAddress: 0 }));
        this.codes = Array.from({ length: 188 }, () => []);
        Object.assign(this.types[BRIDGE], { constructorAddress: typeConstructorAddress(BRIDGE), flags1: 0x802, flags2: 4, footX: 1, footY: 1 });
        this.codes[BRIDGE] = [{ side: 75, variant: 80, number: 1, flags: 0 }, { side: 75, variant: 80, number: 2, flags: 0x40 }];
        this.frames = this.codes.map((codes) => new FrameCodeTable(codes));
        this.records = Array.from({ length: 255 }, () => ({ surfaces: 0, inUse: 0, reserved: 0 }));
        this.records[0] = { surfaces, inUse: 1, reserved: 23 };
        this.mode = { ...DEFAULT_BRIDGE_DECAY_MODE };
        this.unpop = new SquidUnpop(this.pool, this.hash, this.spots);
        this.base = new RawSquidDestroy(this.pool, this.unpop, this.types);
        this.neighbors = new RawSquidNeighbors(this.pool, this.hash, this.spots, this.types, this.frames);
        this.graph = new RawGraph(this.pool, this.hash, this.spots, this.types, this.codes, { records: this.records });
        this.dispatch = new RawSquidDestroyDispatch(this.base, this.neighbors, this.mode, graphEnabled ? this.graph : null);
        this.surfaces = surfaces;
        this.calls = 0;
        this.source = new SquidFactory(this.pool, this.types, { unpop: this.unpop }).create(BRIDGE, 2);
        put(this.pool.allocated(this.source), 36, hard ? 1 : 0, 4);
        this.pool.allocated(this.source)[30] = 0;
        // 공통 몸체가 넘긴 flags 를 보존하고 두 깊이의 완료 계약을 수행한다.
        this.hooks = {
            emit: (event) => {
                this.calls += 1;
                if (event.effect === SQUID_DESTROY_EFFECT.preDestroy) {
                    assert.equal(event.flags, 0x1234);
                    this.base.completePreDestroy();
                } else if (event.effect === SQUID_DESTROY_EFFECT.postDestroy) this.base.completePostDestroy();
                else assert.equal(event.effect, SQUID_DESTROY_EFFECT.transmit);
            },
            // 선택 UI 는 이 판정 장면에서 비어 있다.
            selected: () => 0,
        };
    }

    // 거부 시 전체 풀·표·스택·콜백·깊이를 보존하고 전달 시 실제 SID 반납까지 확인한다. (2026-11-10 WB04-E214)
    check(expected) {
        const before = hex(this.pool.bytes());
        const freeBefore = this.pool.freeCount;
        const stack = adler32(new Uint8Array(this.graph.floodStack().buffer.slice(0)));
        assert.equal(this.dispatch.destroy(this.source, 0x1234, this.hooks), expected);
        // 인자 0x1234 의 client 비트가 Transmit 을 억제하므로 공통 pre/post 두 번만 호출된다.
        if (expected) {
            assert.ok((this.pool.slot(this.source)[11] & FREE) !== 0);
            assert.equal(this.pool.freeCount, freeBefore + 1);
            assert.equal(this.calls, 2);
        } else {
            assert.equal(hex(this.pool.bytes()), before);
            assert.equal(this.pool.freeCount, freeBefore);
            assert.equal(this.calls, 0);
        }
        assert.equal(this.graph.records()[0].surfaces, this.surfaces);
        assert.equal(this.graph.records()[0].reserved, 23);
        assert.equal(adler32(new Uint8Array(this.graph.floodStack().buffer.slice(0))), stack);
        assert.equal(this.base.preDepth, 0);
        assert.equal(this.base.postDepth, 0);
    }
}

test('다리 가상 삭제: 4/5 경계·signed 음수·hard/보통·debugKeep·Graph 비활성을 각각 판정한다', () => {
    new DispatchScene(4, true).check(true);
    new DispatchScene(5, true).check(false);
    new DispatchScene(-1, true).check(true);
    new DispatchScene(5, false).check(true);
    const debug = new DispatchScene(5, false);
    debug.mode.debugKeep = true;
    debug.check(false);
    new DispatchScene(5, true, false).check(true);
});

test('다리 가상 삭제: editor/권한/abstract/buried 는 무효 Graph 와 프레임 조회를 모두 건너뛴다', () => {
    // 네 종류의 첫 조건 실패를 각각 독립 장면으로 검사한다.
    for (let gate = 0; gate < 4; gate += 1) {
        const scene = new DispatchScene(5, true);
        scene.pool.allocated(scene.source)[30] = 254;
        put(scene.pool.allocated(scene.source), 36, 99, 4);
        if (gate === 0) scene.mode.editor = true;
        else if (gate === 1) scene.mode.authority = false;
        else scene.pool.allocated(scene.source)[40] |= gate === 2 ? 1 : 8;
        scene.check(true);
    }
    const small = new DispatchScene(4, true);
    put(small.pool.allocated(small.source), 36, 99, 4);
    small.check(true);
});

test('다리 가상 삭제: 살아 있는 raw 프레임을 매번 읽고 모드/가상 표 변경과 공통 몸체 우회를 구별한다', () => {
    const frame = new DispatchScene(5, true);
    frame.check(false);
    put(frame.pool.allocated(frame.source), 36, 0, 4);
    frame.check(true);
    const ordinary = new DispatchScene(5, true);
    put(ordinary.pool.allocated(ordinary.source), 0, COMMON_VTABLE);
    ordinary.check(true);
    const direct = new DispatchScene(5, true);
    direct.base.destroy(direct.source, 0x1234, direct.hooks);
    assert.ok((direct.pool.slot(direct.source)[11] & FREE) !== 0);
});

test('다리 가상 삭제: 무효 그래프/프레임은 원본 접근 오류 대신 쓰기 전 예외로 남기고 raw 를 바꾸지 않는다', () => {
    const scene = new DispatchScene(5, true);
    scene.pool.allocated(scene.source)[30] = 254;
    const before = hex(scene.pool.bytes());
    assert.ok(throws(() => scene.dispatch.destroy(scene.source, 0x1234, scene.hooks)));
    assert.equal(hex(scene.pool.bytes()), before);
    assert.equal(scene.calls, 0);
    scene.pool.allocated(scene.source)[30] = 255;
    const badGraph = hex(scene.pool.bytes());
    assert.ok(throws(() => scene.dispatch.destroy(scene.source, 0x1234, scene.hooks)));
    assert.equal(hex(scene.pool.bytes()), badGraph);
    assert.equal(scene.calls, 0);
    scene.pool.allocated(scene.source)[30] = 0;
    put(scene.pool.allocated(scene.source), 36, 99, 4);
    const badFrame = hex(scene.pool.bytes());
    assert.ok(throws(() => scene.dispatch.destroy(scene.source, 0x1234, scene.hooks)));
    assert.equal(hex(scene.pool.bytes()), badFrame);
    assert.equal(scene.calls, 0);
    // 음수 프레임도 범위 밖이다.
    put(scene.pool.allocated(scene.source), 36, 0xffffffff, 4);
    assert.ok(throws(() => scene.dispatch.destroy(scene.source, 0x1234, scene.hooks)));
});

test('다리 가상 삭제: 서로 다른 풀/Graph 공간의 연결은 가상 삭제를 실행하기 전에 거부한다', () => {
    const first = new DispatchScene(5, true);
    const second = new DispatchScene(5, true);
    assert.ok(throws(() => new RawSquidDestroyDispatch(first.base, second.neighbors, first.mode)));
    assert.ok(throws(() => new RawSquidDestroyDispatch(first.base, first.neighbors, first.mode, second.graph)));
    const otherGraph = new RawGraph(first.pool, new SquidHash(), first.spots, first.types, first.codes);
    assert.ok(throws(() => new RawSquidDestroyDispatch(first.base, first.neighbors, first.mode, otherGraph)));
    assert.ok(throws(() => new RawSquidDestroyDispatch({ pool: first.pool }, first.neighbors, first.mode)));
});

test('다리 가상 삭제 계약: 현재 권한/편집기 전역 대여가 mode 를 덮고 호출마다 읽히며 mode 변경은 다음 호출에 반영된다', () => {
    const scene = new DispatchScene(5, true);
    const authority = { boss: false, server: true };
    let editor = 0;
    scene.dispatch.bindCurrentModes(authority, () => editor);
    // 권한이 꺼진 현재 전역이 mode.authority(참)를 덮으므로 단단한 프레임이어도 삭제가 진행된다.
    scene.check(true);
    const second = new DispatchScene(5, true);
    second.dispatch.bindCurrentModes({ boss: true, server: true }, () => editor);
    second.check(false);
    editor = 1;
    const third = new DispatchScene(5, true);
    third.dispatch.bindCurrentModes({ boss: true, server: true }, () => editor);
    third.check(true);
    // mode 객체의 변경은 다음 호출부터 반영된다(대여가 없을 때).
    const fourth = new DispatchScene(5, true);
    fourth.check(false);
    fourth.mode.editor = true;
    fourth.check(true);
    // 형식이 틀린 대여는 거부한다.
    assert.ok(throws(() => fourth.dispatch.bindCurrentModes({}, null)));
    assert.ok(throws(() => fourth.dispatch.bindCurrentModes({ boss: true }, 1)));
});

test('다리 가상 삭제 계약: 원본 raw float 좌표 입력은 판정에 쓰이지 않는다(좌표/타입이 달라도 같은 가상 표·그래프·프레임이면 같은 결과)', () => {
    const scene = new DispatchScene(5, true);
    put(scene.pool.allocated(scene.source), 14, floatToBits(250.5));
    put(scene.pool.allocated(scene.source), 18, floatToBits(3.25));
    scene.check(false);
});
