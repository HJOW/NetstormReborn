// SquidFactory 의 진단 복귀·부분 쓰기·현재 TYPE/전역 재조회·Form 진입 계약을 명시 입력으로 검사한다(WB04-D4).
// cpppj 의 CreationTests·FactoryTakeDiagnosticsTests·FactoryPostCreateTests·SquidCreationParentTests·AssetConstructorCurrentTests·
// SquidContainedUnpopTests(FactoryTakeChecking)·FormCreationCurrentTests·FormAttach/Pop/UnpopCurrentTests 의 10.78 부분을 옮겼다(기대값은 그 검사와 같다).
// cpppj 의 실제 World/Session(RawSurfaceWorld) 검사는 웹에 World 가 아직 없어 같은 Factory 에 GameAuxState·깊이 소유자·종속 분리를 직접 빌려 주는 형태로 옮겼다.
// 새 CPU 관측이나 실제 플레이 검사가 아니다. (2026-11-06 WB04-D4: SquidFactory 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { SQUID_FORM_VTABLES } from '../www/core/squid-vtables.js';
import { PATCH_TYPE_CONSTRUCTORS, typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidFactory, createRawSquid } from '../www/core/squid-factory.js';
import { squidFormGlobals } from '../www/core/squid-forms.js';
import {
    RawSquidContainedUnpop, commonContainedParent, commonContainedPrevious, commonContainedSetPrevious,
} from '../www/core/squid-contained-unpop.js';

// 원문 진단 파일 이름·base VT·다이스 보조 생성자 주소이다. (2026-11-06 WB04-D4)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';
const BASE_VT = 0x501dd0;
const DAIS_CONSTRUCTOR = 0x44a420;

// 실제 생성자 주소 표 전체를 넣은 188칸 TYPE 입력이다(cpppj Types). (2026-11-06 WB04-D4)
const constructorTypes = () => PATCH_TYPE_CONSTRUCTORS.map((address) => ({ constructorAddress: address }));
// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-06 WB04-D4)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}));

// 오류가 나는지 확인한다(성공을 무조건 인정하지 않음). (2026-11-06 WB04-D4)
function throws(action) {
    try {
        action();
    } catch {
        return true;
    }
    return false;
}

// 두 바이트 배열이 같은지 네이티브 비교로 확인한다. (2026-11-06 WB04-D4)
const same = (left, right) => Buffer.from(left).equals(Buffer.from(right));

// Create 입구: fakeSurface 진단 부재·client 의 서버 할당은 풀을 바꾸기 전에 멈추고, TYPE 범위/void NULL 생성자 오류는 할당 뒤에 멈춘다(cpppj SquidFactory_CreateGuards).
test('SquidFactory.create: 선택 경계별 할당 전/후 실패', () => {
    const pool = new SidPool(32768, { server: false });
    const factory = new SquidFactory(pool, constructorTypes());
    const before = pool.bytes();
    assert.ok(throws(() => factory.create(162, 2)));
    assert.ok(throws(() => factory.create(74, 0)));
    assert.ok(same(pool.bytes(), before));
    assert.deepEqual([pool.freeCount, pool.predictableCursor], [32763, 1]);
    // Form 의 빈 +28 에 자산 진단/HP 를 더하지 않는다.
    assert.equal(factory.create(1, 2), 5);
    assert.ok(throws(() => factory.create(188, 2)));
    assert.ok(throws(() => factory.create(153, 2)));
    assert.deepEqual([pool.freeCount, pool.slot(6)[11], pool.slot(7)[11]], [32760, 4, 4]);
});

// 미복원 생성자: Construct/Take 는 바꾸기 전에 거부하고 Create 는 원문처럼 할당을 되돌리지 않는다(cpppj SquidFactory_DerivedGuards).
test('SquidFactory: 미복원 생성자 경로의 변경 전 거부와 Create 의 선행 할당', () => {
    const pool = new SidPool(32768);
    const types = constructorTypes();
    types[82].constructorAddress = 0x12345678;
    const factory = new SquidFactory(pool, types);
    const sid = factory.create(74);
    const before = pool.bytes();
    const [free, cursor] = [pool.freeCount, pool.predictableCursor];
    // 잘못된 생성자로 가는 두 진입 경로를 확인한다.
    for (const type of [82, 153, 159, 169]) {
        assert.ok(throws(() => factory.construct(type, sid)), `construct ${type}`);
        assert.ok(throws(() => factory.take(type, sid)), `take ${type}`);
    }
    assert.ok(same(pool.bytes(), before));
    assert.deepEqual([pool.freeCount, pool.predictableCursor], [free, cursor]);
    // 미지원/void 반환/미연결 진단은 할당을 되돌리지 않는다.
    for (const type of [82, 153, 159, 169]) {
        const previous = pool.freeCount;
        assert.ok(throws(() => factory.create(type)), `create ${type}`);
        assert.equal(pool.freeCount, previous - 1);
    }
});

// anim 생성자(161)는 Take 에서 HP DWORD 를 지우고 공통 postTake 는 HP 를 보존하며, 반납은 SID 풀로 돌아간다(cpppj SquidFactory_AnimTake 의 10.78 부분).
test('SquidFactory.take: anim 생성자의 HP 지우기와 반납', () => {
    const pool = new SidPool(32768);
    const unpop = new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536));
    const factory = new SquidFactory(pool, constructorTypes(), { unpop });
    const sid = factory.create(161);
    const bytes = pool.allocated(sid);
    bytes.set([1, 2, 3, 4], 26);
    bytes[34] = 9;
    factory.postTake(sid);
    assert.deepEqual([...bytes.subarray(26, 30)], [1, 2, 3, 4]);
    assert.equal(factory.take(161, sid), sid);
    assert.deepEqual([...bytes.subarray(26, 30), bytes[34]], [0, 0, 0, 0, 9]);
    const free = pool.freeCount;
    pool.release(sid);
    assert.equal(pool.freeCount, free + 1);
    assert.ok(pool.slot(sid)[11] & 1);
});

// 독립 Construct 는 TYPE/상태/HP/깊이를 초기화하지 않고, 예약·범위 밖·free·contained 슬롯은 쓰지 않는다(cpppj SquidFactory_Construct_PreservesCallerFields).
test('SquidFactory.construct: 호출자 필드 보존과 슬롯 상태 거부', () => {
    const pool = new SidPool(32768);
    const types = constructorTypes();
    Object.assign(types[82], { flags1: 0x10, maxHitPoints: 999, zOrder: 5 });
    const factory = new SquidFactory(pool, types);
    const sid = factory.create(82);
    const bytes = pool.allocated(sid);
    bytes[10] = 0;
    bytes[26] = 7;
    bytes[35] = 77;
    bytes[34] = 9;
    const before = pool.bytes();
    assert.equal(factory.construct(82, sid), sid);
    assert.ok(same(pool.bytes(), before));
    // 예약 0·범위 밖·free 서버 슬롯은 쓰기 전에 거부한다.
    for (const target of [0, 32768, 15001]) assert.ok(throws(() => factory.construct(82, target)), `sid ${target}`);
    assert.ok(same(pool.bytes(), before));
    bytes[11] |= 8;
    const contained = pool.bytes();
    assert.ok(throws(() => factory.construct(82, sid)));
    assert.ok(same(pool.bytes(), contained));
});

// cpppj TakeDiagnosticScene 과 같은 입력: TYPE 74/75 이름·zOrder, 서버 74 와 client 74, 같은 풀의 실제 Unpop. (2026-11-06 WB04-D4)
class TakeDiagnosticScene {
    // 실제 풀·해시·spot·Form 입력 TYPE 표와 진단/상세 기록이다.
    constructor() {
        this.pool = new SidPool(32768);
        this.spots = new Uint8Array(65536);
        this.types = formTypeInputs();
        this.unpop = new SquidUnpop(this.pool, new SquidHash(), this.spots);
        this.factory = new SquidFactory(this.pool, this.types, { unpop: this.unpop });
        this.detail = null;
        this.lines = [];
        this.sequence = [];
        this.onAssertion = null;
        this.onDetail = null;
        Object.assign(this.types[74], { name: 'SERVER\0tail', zOrder: 17 });
        Object.assign(this.types[75], { name: 'CLIENT', zOrder: 34 });
        this.factory.bindDiagnostics({
            types: this.types,
            // 범위 진단과 TYPE 불일치 진단만 오며 정상 복귀한다.
            assertion: (expression, file, line) => {
                assert.equal(file, SQUID_CPP);
                assert.equal(expression, line === 0x1a7 ? '(FIRST_SERVER_SID <= sid && sid < numSquids)' : '0');
                this.lines.push(line);
                this.sequence.push('assert');
                this.onAssertion?.(line);
            },
            // 상세 문구 사본을 받는다.
            detail: (text) => {
                this.detail = text;
                this.sequence.push('detail');
                this.onDetail?.();
            },
        });
        this.source = this.factory.create(74);
        this.client = this.factory.create(74, 2);
        this.unpop.bindDiagnostics({ types: this.types });
    }

    // 물리 슬롯 쓰기 대여이다.
    raw(sid) { return this.pool.physical(sid); }
}

// 서버 범위 진단은 정상 복귀하면 물리 범위 안의 client 번호로 계속한다(cpppj FactoryTakeDiagnostics_SidAssertionResumesPhysicalClientSlot).
test('SquidFactory.take: SID 범위 진단 뒤 client 슬롯 재개', () => {
    const scene = new TakeDiagnosticScene();
    const [free, first, cursor] = [scene.pool.freeCount, scene.pool.firstFree(true), scene.pool.predictableCursor];
    assert.equal(scene.factory.take(74, scene.client), scene.client);
    assert.deepEqual(scene.lines, [0x1a7]);
    assert.equal(scene.detail, null);
    assert.deepEqual([scene.raw(scene.client)[10], scene.raw(scene.client)[11], scene.raw(scene.client)[35]], [74, 4, 17]);
    assert.deepEqual([scene.pool.freeCount, scene.pool.firstFree(true), scene.pool.predictableCursor], [free, first, cursor]);
});

// 풀 밖 SID 는 원문 진단을 먼저 부르되 물리 범위 밖 접근은 하지 않고, 진단 미연결 공장은 바꾸기 전에 멈춘다(cpppj FactoryTakeDiagnostics_OutOfPool).
test('SquidFactory.take: 풀 밖 SID 의 진단 뒤 범위 오류', () => {
    const scene = new TakeDiagnosticScene();
    const before = scene.pool.bytes();
    assert.ok(throws(() => scene.factory.take(74, 32768)));
    assert.deepEqual(scene.lines, [0x1a7]);
    assert.equal(scene.detail, null);
    assert.ok(same(scene.pool.bytes(), before));
    const disconnected = new SquidFactory(scene.pool, scene.types);
    assert.ok(throws(() => disconnected.take(74, scene.client)));
    assert.ok(same(scene.pool.bytes(), before));
});

// TYPE 불일치: 상세(이름 복사)→진단→실제 void Unpop→진단이 바꾼 현재 생성자(anim recipe)·zOrder 순서다(cpppj FactoryTakeDiagnostics_Mismatch).
test('SquidFactory.take: TYPE 불일치 상세→진단→현재 생성자', () => {
    const scene = new TakeDiagnosticScene();
    scene.raw(scene.source)[10] = 75;
    const free = scene.pool.freeCount;
    const spots = scene.spots.slice();
    // 상세 뒤 이름을 바꿔도 이미 복사한 문구가 유지된다.
    scene.onDetail = () => {
        assert.deepEqual([scene.raw(scene.source)[10], scene.raw(scene.source)[11]], [75, 4]);
        scene.types[74].name = 'changed server';
        scene.types[75].name = 'changed client';
    };
    // 진단 정상 복귀가 현재 생성자와 zOrder 를 바꾼다.
    scene.onAssertion = (line) => {
        assert.equal(line, 0x1ba);
        assert.equal(scene.detail, 'Invalid take. Server type SERVER, client type CLIENT');
        scene.types[74].constructorAddress = 0x4794e0;
        scene.types[74].zOrder = 0x123;
    };
    assert.equal(scene.factory.take(74, scene.source), scene.source);
    const raw = scene.raw(scene.source);
    assert.deepEqual(scene.lines, [0x1ba]);
    assert.deepEqual(scene.sequence, ['detail', 'assert']);
    assert.deepEqual([get(raw, 0), raw[10], raw[11], raw[35], raw[40] & 2], [0x50cc08, 74, 4, 0x23, 2]);
    assert.deepEqual([...raw.subarray(26, 30)], [0, 0, 0, 0]);
    assert.equal(scene.pool.freeCount, free);
    assert.ok(same(scene.spots, spots));
});

// 실행 중 장치를 다시 연결해도 진행 중인 장치는 끝까지 쓰고 다음 진단은 새 장치를 조회한다(cpppj FactoryTakeDiagnostics_CurrentDeviceCanRebindDuringBothCallbacks).
test('SquidFactory.take: 콜백 중 진단 장치 재연결', () => {
    const scene = new TakeDiagnosticScene();
    scene.raw(scene.source)[10] = 75;
    let replacement = 0;
    // 상세 중 새 진단을 연결하고 그 진단이 다시 연결을 비운다.
    scene.onDetail = () => {
        scene.factory.bindDiagnostics({
            types: scene.types,
            assertion: (expression, file, line) => {
                assert.deepEqual([expression, file, line], ['0', SQUID_CPP, 0x1ba]);
                replacement += 1;
                scene.factory.bindDiagnostics({ types: scene.types });
                scene.types[74].zOrder = 51;
            },
        });
    };
    assert.equal(scene.factory.take(74, scene.source), scene.source);
    assert.deepEqual([replacement, scene.lines.length, scene.detail, scene.raw(scene.source)[35]], [1, 0, 'Invalid take. Server type SERVER, client type CLIENT', 51]);
});

// 선택된 상세/진단 장치 부재는 앞선 효과만 남기고, 진단이 예외로 끝나면 복사한 상세만 남는다(cpppj FactoryTakeDiagnostics_SelectedMissingWriters·AssertionAbort).
test('SquidFactory.take: 상세/진단 장치 부재와 진단 중단', () => {
    const scene = new TakeDiagnosticScene();
    scene.raw(scene.source)[10] = 75;
    const before = scene.pool.bytes();
    let assertions = 0;
    const noDetail = new SquidFactory(scene.pool, scene.types, { unpop: scene.unpop });
    noDetail.bindDiagnostics({ types: scene.types, assertion: () => { assertions += 1; } });
    assert.ok(throws(() => noDetail.take(74, scene.source)));
    assert.deepEqual([assertions, scene.detail], [0, null]);
    assert.ok(same(scene.pool.bytes(), before));
    const noAssertion = new SquidFactory(scene.pool, scene.types, { unpop: scene.unpop });
    noAssertion.bindDiagnostics({ types: scene.types, detail: (text) => { scene.detail = text; } });
    assert.ok(throws(() => noAssertion.take(74, scene.source)));
    assert.equal(scene.detail, 'Invalid take. Server type SERVER, client type CLIENT');
    assert.ok(same(scene.pool.bytes(), before));
    const abort = new TakeDiagnosticScene();
    abort.raw(abort.source)[10] = 75;
    const untouched = abort.pool.bytes();
    abort.onAssertion = () => { throw new Error('명시 진단 종료'); };
    assert.ok(throws(() => abort.factory.take(74, abort.source)));
    assert.deepEqual([abort.detail, abort.lines], ['Invalid take. Server type SERVER, client type CLIENT', [0x1ba]]);
    assert.ok(same(abort.pool.bytes(), untouched));
});

// cpppj PostCreateScene 과 같은 입력: 같은 {value} 서버 권한, TYPE 74/75 의 zOrder·HP, 이웃 payload. (2026-11-06 WB04-D4)
class PostCreateScene {
    // 빈 표 사본으로 공장을 만든 뒤 같은 현재 표를 빌려 준다.
    constructor() {
        this.server = { value: true };
        this.pool = new SidPool(32768, { serverOwner: this.server });
        this.types = Array.from({ length: 188 }, () => ({}));
        this.factory = new SquidFactory(this.pool, this.types);
        this.sid = this.pool.allocate(0);
        this.lines = [];
        this.resume = null;
        this.types[74] = { zOrder: 0x123, flags1: 0x10, maxHitPoints: 0x12345678 };
        this.types[75] = { zOrder: 0x234, flags1: 0x10, maxHitPoints: 0x23456789 };
        const raw = this.raw();
        raw[10] = 74;
        raw[34] = 9;
        raw[35] = 77;
        raw.set([1, 2, 3, 4], 26);
        this.factory.bindDiagnostics({
            types: this.types,
            // 세 진단의 식·줄과 정상 복귀 중 외부 변경이다.
            assertion: (expression, file, line) => {
                assert.equal(file, SQUID_CPP);
                assert.equal(expression, line === 0x7d4 ? '!isForm()' : '!isContained()');
                this.lines.push(line);
                this.resume?.(line);
            },
        });
    }

    // 같은 물리 슬롯이다.
    raw() { return this.pool.physical(this.sid); }
}

// PostCreate 의 세 진단을 일괄 사전 검사로 바꾸지 않고, 각 위치에서 현재 TYPE/상태로 계속한다(cpppj FactoryPostCreate_EntryDiagnostics…).
test('SquidFactory.postCreate: 입구 진단 복귀 뒤 현재 TYPE 과 꼬리 진단 순서', () => {
    const scene = new PostCreateScene();
    const raw = scene.raw();
    raw[10] = 3;
    raw[11] = 13;
    const free = scene.pool.freeCount;
    // 각 진단 시점의 앞선 쓰기와 외부 변경이다.
    scene.resume = (line) => {
        if (line === 0x7d4) {
            assert.deepEqual([raw[35], raw[34]], [77, 9]);
            raw[10] = 75;
        }
        if (line === 0x7d5) {
            assert.deepEqual([raw[35], raw[26]], [77, 1]);
            raw[11] |= 2;
        }
        if (line === 0x7df) {
            assert.deepEqual([raw[35], raw[34], raw[26], raw[29]], [0x34, 0, 0x89, 0x23]);
            raw[11] &= ~8 & 0xff;
        }
    };
    scene.factory.postCreate(scene.sid);
    assert.deepEqual(scene.lines, [0x7d4, 0x7d5, 0x7df]);
    assert.deepEqual([raw[10], raw[11], raw[35], scene.pool.freeCount], [75, 7, 0x34, free]);
});

// 비서버는 owner/HP 만 건너뛰고 꼬리 진단은 그대로 하며, 진단이 바꾼 TYPE 표/서버를 초기 사본으로 덮지 않는다(cpppj FactoryPostCreate_NonServer…·CurrentTypeAndServer…).
test('SquidFactory.postCreate: 비서버 분기와 진단 뒤 현재 표/서버', () => {
    const nonServer = new PostCreateScene();
    nonServer.server.value = false;
    const raw = nonServer.raw();
    raw[11] = 12;
    nonServer.factory.postCreate(nonServer.sid);
    assert.deepEqual(nonServer.lines, [0x7d5, 0x7df]);
    assert.deepEqual([raw[35], raw[34], ...raw.subarray(26, 30), raw[11]], [0x23, 9, 1, 2, 3, 4, 12]);
    const current = new PostCreateScene();
    const slot = current.raw();
    slot[10] = 0;
    // 진단이 TYPE·표·서버를 바꾼다.
    current.resume = (line) => {
        assert.equal(line, 0x7d4);
        slot[10] = 75;
        current.types[75].zOrder = 0x345;
        current.server.value = false;
    };
    current.factory.postCreate(current.sid);
    assert.deepEqual([current.lines, slot[35], slot[34], slot[26]], [[0x7d4], 0x45, 9, 1]);
});

// 입구 진단 부재는 쓰기 전에, 꼬리 진단 부재는 zOrder/owner/HP 쓰기 뒤에 멈추며, 다이스 flags2 는 extra 비트로 HP 를 고른다(cpppj FactoryPostCreate_Missing…·AbstractHp…·PhysicalFreeDead…).
test('SquidFactory.postCreate: 진단 부재 시점·다이스 HP 분기·free/dead 슬롯', () => {
    const missing = new PostCreateScene();
    const raw = missing.raw();
    raw[10] = 3;
    missing.factory.bindDiagnostics({ types: missing.types });
    const before = raw.slice();
    assert.ok(throws(() => missing.factory.postCreate(missing.sid)));
    assert.ok(same(raw, before));
    raw[10] = 74;
    raw[11] = 12;
    // 첫 진단이 정상 복귀하며 연결을 비운다.
    missing.factory.bindDiagnostics({
        types: missing.types,
        assertion: (expression, file, line) => {
            assert.equal(line, 0x7d5);
            missing.factory.bindDiagnostics({ types: missing.types });
        },
    });
    assert.ok(throws(() => missing.factory.postCreate(missing.sid)));
    assert.deepEqual([raw[35], raw[34], ...raw.subarray(26, 30), raw[11]], [0x23, 0, 0x78, 0x56, 0x34, 0x12, 12]);
    const abstract = new PostCreateScene();
    const slot = abstract.raw();
    abstract.types[74].flags2 = 0x400000;
    slot[40] = 0;
    abstract.factory.postCreate(abstract.sid);
    assert.deepEqual([...slot.subarray(26, 30), slot[34]], [1, 2, 3, 4, 0]);
    slot[40] = 1;
    abstract.types[74].flags1 = 0;
    abstract.factory.postCreate(abstract.sid);
    assert.deepEqual([...slot.subarray(26, 30), abstract.lines.length], [0x78, 0x56, 0x34, 0x12, 0]);
    const freeDead = new PostCreateScene();
    const physical = freeDead.raw();
    physical[11] = 7;
    const [free, next] = [freeDead.pool.freeCount, freeDead.pool.firstFree(false)];
    freeDead.factory.postCreate(freeDead.sid);
    assert.deepEqual([physical[11], physical[35], physical[26], freeDead.pool.freeCount, freeDead.pool.firstFree(false), freeDead.lines.length], [7, 0x23, 0x78, free, next, 0]);
});

// 일반 Create 도 현재 생성자가 Form 이면 Form VT 와 빈 +28 만 쓰고(자산 HP/깊이 없음), base 면 자산 초기화를 한다(cpppj SquidCreationParent_ActualFactoryUsesCurrentConstructorAndRealFormVtable).
test('SquidFactory.create: 현재 생성자가 Form 이면 빈 +28', () => {
    const pool = new SidPool(32768);
    const types = formTypeInputs();
    const factory = new SquidFactory(pool, types);
    factory.bindDiagnostics({ types });
    // 네 Form 타입을 일반 Create 로 만든다.
    for (const type of [1, 2, 3, 6]) {
        Object.assign(types[type], { zOrder: 99, flags1: 0x10, maxHitPoints: 777 });
        const sid = factory.create(type);
        const raw = pool.slot(sid);
        assert.deepEqual([get(raw, 0), raw[10], raw[11], get(raw, 26), raw[35]], [factory.constructorVtable(types[type]), type, 4, 0, 0]);
    }
    Object.assign(types[74], { constructorAddress: typeConstructorAddress(6), zOrder: 79, flags1: 0x10, maxHitPoints: 999 });
    const formLike = factory.create(74);
    assert.deepEqual([pool.slot(formLike)[10], get(pool.slot(formLike), 0), get(pool.slot(formLike), 26)], [74, SQUID_FORM_VTABLES.content, 0]);
    types[74].constructorAddress = 0;
    const asset = factory.create(74);
    assert.deepEqual([get(pool.slot(asset), 26), pool.slot(asset)[35]], [999, 79]);
});

// fakeSurface 진단은 할당 전에 정상 복귀하며 같은 GameAuxState 의 현재 TYPE 을 읽고, 새 공장도 같은 소유자를 빌린다(cpppj SquidCreationParent_DiagnosisResumes…·ActualWorldKeepsLongLivedFakeType…).
test('SquidFactory.create: fakeSurface 진단과 GameAuxState 장기 전역', () => {
    const pool = new SidPool(32768);
    const types = formTypeInputs();
    const factory = new SquidFactory(pool, types);
    const aux = new GameAuxState();
    aux.fakeSurfaceType = 74;
    factory.bindGameAux(aux);
    let calls = 0;
    // 할당 전 진단이 현재 표의 생성자를 바꾼다.
    factory.bindDiagnostics({
        types,
        assertion: (expression, file, line) => {
            assert.deepEqual([expression, file, line, pool.freeCount], ['type != fakeThreeByThreeSurfaceType', SQUID_CPP, 0x189, 32763]);
            calls += 1;
            types[74].constructorAddress = typeConstructorAddress(3);
        },
    });
    const sid = factory.create(74);
    assert.deepEqual([calls, pool.freeCount, pool.slot(sid)[10], get(pool.slot(sid), 0)], [1, 32762, 74, 0x512188]);
    aux.fakeSurfaceType = 75;
    assert.equal(factory.create(74), sid + 1);
    assert.equal(calls, 1);
    types[76].constructorAddress = 0x12345678;
    const previous = pool.freeCount;
    assert.ok(throws(() => factory.create(76)));
    assert.equal(pool.freeCount, previous - 1);
    // 미연결 cdecl 전에는 TYPE BYTE 를 쓰지 않았다.
    assert.equal(pool.slot(sid + 2)[10], 0);
    assert.ok(throws(() => factory.bindGameAux(new GameAuxState())));
    // 같은 소유자를 빌린 두 공장이 같은 현재 값을 읽는다.
    const shared = new GameAuxState();
    shared.fakeSurfaceType = 74;
    let diagnosed = 0;
    // 세대마다 새 공장을 만들어 같은 소유자를 빌린다.
    for (let generation = 0; generation < 2; generation += 1) {
        const worldPool = new SidPool(32768);
        const worldFactory = new SquidFactory(worldPool, formTypeInputs());
        worldFactory.bindGameAux(shared);
        worldFactory.bindDiagnostics({ assertion: () => { diagnosed += 1; } });
        worldFactory.create(74);
        assert.deepEqual([shared.fakeSurfaceType, diagnosed], [74, generation + 1]);
    }
    shared.resetForGeneralWorld();
    assert.equal(shared.fakeSurfaceType, 74);
});

// 004af720 은 할당/TYPE/가상 후처리 없이 현재 생성자의 bare 쓰기만 하며 free/dead/contained/예약도 거르지 않는다(cpppj SquidCreationParent_ConstructCurrentHasNoStateOrAssetGuard).
test('SquidFactory.constructCurrent: 상태/자산 선행 검사 없는 현재 생성자', () => {
    const pool = new SidPool(32768);
    const types = formTypeInputs();
    const factory = new SquidFactory(pool, types);
    factory.bindDiagnostics({ types });
    const free = pool.freeCount;
    // 예약 0·일반 5·17 슬롯이다.
    for (const sid of [0, 5, 17]) {
        const raw = pool.physical(sid);
        // free/dead/contained/void 상태를 모두 시험한다.
        for (const state of [1, 2, 8, 12]) {
            // 기대 출력과 독립인 payload 다.
            for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 7 + 9) & 0xff;
            raw[10] = 1;
            raw[11] = state;
            types[1].constructorAddress = typeConstructorAddress(6);
            const before = raw.slice();
            factory.constructCurrent(sid);
            assert.ok(get(raw, 0) === SQUID_FORM_VTABLES.content && same(raw.subarray(4), before.subarray(4)));
            types[1].constructorAddress = 0;
            factory.constructCurrent(sid);
            assert.ok(get(raw, 0) === BASE_VT && same(raw.subarray(4), before.subarray(4)));
            types[1].constructorAddress = 0x4429e0;
            const sentinel = raw.slice();
            factory.constructCurrent(sid);
            assert.ok(same(raw, sentinel));
        }
    }
    assert.equal(pool.freeCount, free);
});

// Create 부모의 선택된 장치 부재는 앞선 효과만 남긴다(cpppj SquidCreationParent_MissingDevicesPreserveEarlierParentEffects 의 10.78 부분).
test('createRawSquid: 선택된 장치 부재 시점', () => {
    let trace = '';
    const hooks = {
        fakeSurfaceType: () => 162,
        allocate: () => { trace += 'A'; return 5; },
        constructor: () => { trace += 'T'; return 123; },
    };
    assert.ok(throws(() => createRawSquid(74, 0, hooks)));
    assert.equal(trace, 'AT');
    hooks.construct = () => { trace += 'C'; return 0; };
    hooks.vtable = (object) => {
        assert.equal(object, 0);
        trace += 'V';
        throw new Error('명시 null 반환 객체');
    };
    hooks.storeType = () => { trace += 'W'; };
    trace = '';
    assert.ok(throws(() => createRawSquid(74, 0, hooks)));
    assert.equal(trace, 'ATCV');
});

// NULL/미지원 주소는 풀을 읽지 않고, 다이스 보조 생성자는 진단 정상 복귀 뒤에만 VT 를 쓰며 진단 중단·범위 밖은 VT 를 쓰지 않는다(cpppj AssetConstructorCurrent_NullUnknown…).
test('SquidFactory.constructByAddress: NULL·미지원·다이스 진단 경계', () => {
    const pool = new SidPool(24000, { server: false });
    const factory = new SquidFactory(pool, Array.from({ length: 188 }, () => ({})));
    const raw = pool.physical(50);
    raw.fill(0xa7);
    raw[10] = 74;
    raw[11] = 3;
    const before = raw.slice();
    const free = pool.freeCount;
    assert.deepEqual([factory.constructByAddress(0x4429e0, 65535), factory.constructByAddress(0, 65535), factory.constructByAddress(0x123456, 65535)], [true, false, false]);
    assert.ok(pool.freeCount === free && same(raw, before));
    assert.ok(throws(() => factory.constructByAddress(DAIS_CONSTRUCTOR, 50)));
    assert.ok(same(raw, before));
    let diagnoses = 0;
    // 진단이 자기 연결을 비워도 정상 복귀 뒤 VT 를 쓴다.
    factory.bindDiagnostics({
        assertion: (expression, file, line) => {
            factory.bindDiagnostics({});
            assert.deepEqual([expression, file, line], ['false', '.\\Dais.cpp', 0x484]);
            diagnoses += 1;
            raw[10] = 75;
            raw[34] = 4;
        },
    });
    assert.equal(factory.constructByAddress(DAIS_CONSTRUCTOR, 50), true);
    assert.deepEqual([diagnoses, get(raw, 0), raw[10], raw[34], pool.freeCount], [1, 0x507950, 75, 4, free]);
    put(raw, 0, 0x11223344);
    // 진단 중단 전 외부 쓰기는 남고 다음 VT 저장은 하지 않는다.
    factory.bindDiagnostics({
        assertion: () => {
            diagnoses += 1;
            raw[34] = 9;
            throw new Error('선택 진단 중단 입력');
        },
    });
    assert.ok(throws(() => factory.constructByAddress(DAIS_CONSTRUCTOR, 50)));
    assert.deepEqual([diagnoses, get(raw, 0), raw[34]], [2, 0x11223344, 9]);
    factory.bindDiagnostics({ assertion: () => { diagnoses += 1; } });
    assert.ok(throws(() => factory.constructByAddress(DAIS_CONSTRUCTOR, 65535)));
    assert.deepEqual([diagnoses, get(raw, 0)], [3, 0x11223344]);
});

// cpppj FactoryTakeScene 과 같은 입력: 서버 74(contained, 부모 client 74)·실제 SquidUnpop·실제 contained 종단·같은 진단. (2026-11-06 WB04-D4)
class FactoryTakeScene {
    // 실제 공간 해제/공통 종속 몸체와 같은 물리를 쓴다.
    constructor() {
        this.pool = new SidPool(32768);
        this.spots = new Uint8Array(65536);
        this.types = constructorTypes();
        Object.assign(this.types[74], { zOrder: 17, footX: 1, footY: 1 });
        Object.assign(this.types[75], { zOrder: 34, footX: 1, footY: 1 });
        this.unpop = new SquidUnpop(this.pool, new SquidHash(), this.spots);
        this.factory = new SquidFactory(this.pool, this.types, { unpop: this.unpop });
        this.parentNotices = 0;
        this.diagnostics = [];
        this.commands = [];
        this.onAssertion = null;
        this.onParentNotice = null;
        // 두 부모의 같은 원문 파일/현재 진단이다.
        const assertion = (expression, file, line) => {
            assert.equal(file, SQUID_CPP);
            this.diagnostics.push(line);
            this.onAssertion?.(line);
        };
        this.factory.bindDiagnostics({ types: this.types, assertion });
        this.unpop.bindDiagnostics({ types: this.types, assertion });
        this.source = this.factory.create(74);
        this.parent = this.factory.create(74, 2);
        const raw = this.raw(this.source);
        raw[11] = 8;
        this.put(this.source, 14, this.parent);
        this.put(this.source, 16, 0);
        this.put(this.parent, 6, this.source);
        raw[34] = 7;
        raw.set([0xa0, 0xa1, 0xa2, 0xa3], 26);
        this.contained = new RawSquidContainedUnpop(this.pool, {
            contentFormType: () => 6, gumpFormType: () => 3, uiEvent: () => 0x02000039, checking: () => 0, inMetamapMode: () => false, boss: () => true,
        }, {
            parent: (sid) => commonContainedParent(this.pool, sid),
            previous: (sid) => commonContainedPrevious(this.pool, sid),
            setPrevious: (sid, previous) => commonContainedSetPrevious(this.pool, sid, previous),
            // 실제 분리 뒤의 부모 알림만 관측한다.
            parentNotice: (owner, object, reason) => {
                assert.deepEqual([owner, object, reason], [this.parent, this.source, 2]);
                this.parentNotices += 1;
                this.onParentNotice?.();
            },
            command: (event, object, value) => this.commands.push([event, object, value]),
            assertion,
        });
        this.unpop.bindContainedUnpop((sid) => this.contained.unpop(sid));
    }

    // 물리 슬롯과 WORD 읽기/쓰기, 현재 VT 이다.
    raw(sid) { return this.pool.physical(sid); }
    word(sid, offset) { return get(this.raw(sid), offset, 2); }
    put(sid, offset, value) { put(this.raw(sid), offset, value, 2); }
    table() { return get(this.raw(this.source), 0); }
}

// !free void 도 Unpop 진단을 실행하며 일반 contained 만 실제 링크를 해제한다(cpppj FactoryTakeChecking_VoidAndLiveContainedUseActualUnpop).
test('SquidFactory.take: void/살아 있는 contained 의 실제 Unpop', () => {
    // 같은 Take 요청에서 void 조기 반환 차이를 본다.
    for (const initiallyVoid of [false, true]) {
        const scene = new FactoryTakeScene();
        scene.raw(scene.source)[11] = initiallyVoid ? 12 : 8;
        const free = scene.pool.freeCount;
        const spots = scene.spots.slice();
        assert.equal(scene.factory.take(74, scene.source), scene.source);
        const raw = scene.raw(scene.source);
        assert.deepEqual(scene.diagnostics, initiallyVoid ? [0x5d9, 0x7fa] : [0x5d9]);
        assert.deepEqual([scene.parentNotices, scene.word(scene.parent, 6)], initiallyVoid ? [0, scene.source] : [1, 0]);
        assert.deepEqual([raw[10], raw[11], raw[35], raw[34], raw[26], raw[29], scene.table()], [74, initiallyVoid ? 12 : 4, 17, 7, 0xa0, 0xa3, BASE_VT]);
        assert.ok(scene.pool.freeCount === free && same(scene.spots, spots) && scene.commands.length === 0);
    }
});

// void 입력도 Unpop 진단이 바꾼 현재 상태로 본문을 재개한다(cpppj FactoryTakeChecking_VoidDiagnosticCanResumeContainedUnpop).
test('SquidFactory.take: Unpop 진단 복귀 뒤 contained 재개', () => {
    const scene = new FactoryTakeScene();
    scene.raw(scene.source)[11] = 12;
    // 진단 복귀가 void 를 해제한다.
    scene.onAssertion = (line) => {
        assert.deepEqual([line, scene.raw(scene.source)[11]], [0x5d9, 12]);
        scene.raw(scene.source)[11] = 8;
    };
    scene.factory.take(74, scene.source);
    assert.deepEqual([scene.diagnostics, scene.parentNotices, scene.word(scene.parent, 6), scene.raw(scene.source)[11]], [[0x5d9], 1, 0, 4]);
});

// 부모 dead 는 링크를 유지하지만 UI 후처리와 PostTake 는 생략하지 않는다(cpppj FactoryTakeChecking_DeadParentKeepsLinksAndPostTakeWritesBeforeDiagnostic).
test('SquidFactory.take: 부모 dead 의 링크 보존과 UI·PostTake', () => {
    const scene = new FactoryTakeScene();
    scene.raw(scene.parent)[11] |= 2;
    const gump = scene.factory.createGumpForm(2);
    scene.put(scene.source, 6, gump);
    // zOrder 쓰기와 자신의 UI 검색이 늦은 진단보다 먼저다.
    scene.onAssertion = (line) => {
        if (line === 0x7fa) {
            assert.deepEqual([scene.raw(scene.source)[10], scene.raw(scene.source)[11], scene.raw(scene.source)[35], scene.commands.length], [74, 12, 17, 1]);
        }
    };
    scene.factory.take(74, scene.source);
    assert.deepEqual([scene.diagnostics, scene.word(scene.parent, 6), scene.parentNotices, scene.raw(scene.source)[11]], [[0x5d9, 0x7fa], scene.source, 0, 12]);
    assert.deepEqual(scene.commands, [[0x02000039, scene.source, 0]]);
});

// PostTake 는 !isForm 진단 뒤 TYPE 을 다시 읽고 contained 진단 전에 zOrder 를 쓴다(cpppj FactoryTakeChecking_PostTakeReadsCurrentTypeAndPreservesWriteOrder).
test('SquidFactory.postTake: 진단 뒤 현재 TYPE 과 쓰기 순서', () => {
    const scene = new FactoryTakeScene();
    const raw = scene.raw(scene.source);
    raw[10] = 3;
    raw[11] = 9;
    raw[35] = 0xaa;
    // 두 진단의 앞선 쓰기와 외부 변경이다.
    scene.onAssertion = (line) => {
        if (line === 0x7f6) {
            assert.equal(raw[35], 0xaa);
            raw[10] = 74;
            scene.types[74].zOrder = 0x133;
        } else {
            assert.deepEqual([line, raw[35]], [0x7fa, 0x33]);
            raw[10] = 75;
            scene.types[75].zOrder = 0x144;
        }
    };
    scene.factory.postTake(scene.source);
    assert.deepEqual([scene.diagnostics, raw[10], raw[11], raw[35], raw[26], raw[34]], [[0x7f6, 0x7fa], 75, 9, 0x33, 0xa0, 7]);
});

// 분리 통지 뒤의 현재 생성자/상태로 쓰며, 새 미등록 생성자는 재조회 시점에서 앞선 효과를 남긴 채 멈춘다(cpppj FactoryTakeChecking_CurrentConstructorAndStateAfterUnpop).
test('SquidFactory.take: 분리 통지 뒤 현재 생성자와 상태', () => {
    const scene = new FactoryTakeScene();
    // 원문 anim 생성자는 extra2/VT/WORD8/HP DWORD 를 쓴다.
    scene.onParentNotice = () => {
        const raw = scene.raw(scene.source);
        raw[10] = 75;
        raw[11] = 0x83;
        scene.types[74].constructorAddress = 0x4794e0;
        scene.types[74].zOrder = 0x123;
    };
    scene.factory.take(74, scene.source);
    const raw = scene.raw(scene.source);
    assert.deepEqual([scene.diagnostics, scene.parentNotices, scene.table(), raw[10], raw[11], raw[35], raw[34], raw[40] & 2], [[0x5d9], 1, 0x50cc08, 74, 0x84, 0x23, 7, 2]);
    assert.deepEqual([scene.word(scene.source, 8), scene.word(scene.source, 26), scene.word(scene.source, 28)], [0, 0, 0]);
    const invalid = new FactoryTakeScene();
    // 새 미등록 생성자는 실제 재조회 시점에서 실패한다.
    invalid.onParentNotice = () => {
        invalid.raw(invalid.source)[11] = 0x83;
        invalid.types[74].constructorAddress = 0x12345678;
    };
    assert.ok(throws(() => invalid.factory.take(74, invalid.source)));
    assert.deepEqual([invalid.word(invalid.parent, 6), invalid.raw(invalid.source)[11], invalid.table(), invalid.raw(invalid.source)[26]], [0, 0x80, BASE_VT, 0xa0]);
});

// 최초 TYPE 생성자와 다른 지원 VT 도 현재 가상 Unpop 을 먼저 실행하고, 그 진단이 복구한 현재 생성자를 쓴다(cpppj FactoryTakeChecking_InitialConstructorCanBeRepairedByActualUnpop).
test('SquidFactory.take: 현재 VT 의 Unpop 과 진단이 복구한 생성자', () => {
    const scene = new FactoryTakeScene();
    scene.types[74].constructorAddress = 0x12345678;
    put(scene.raw(scene.source), 0, 0x50cc08);
    // Unpop 진단이 현재 생성자 표를 base 로 되돌린다.
    scene.onAssertion = (line) => {
        assert.deepEqual([line, scene.table()], [0x5d9, 0x50cc08]);
        scene.types[74].constructorAddress = 0;
    };
    assert.equal(scene.factory.take(74, scene.source), scene.source);
    assert.deepEqual([scene.diagnostics, scene.parentNotices, scene.word(scene.parent, 6), scene.table(), scene.raw(scene.source)[10], scene.raw(scene.source)[11], scene.raw(scene.source)[35]],
        [[0x5d9], 1, 0, BASE_VT, 74, 4, 17]);
});

// 자산 VT+48 은 요청 TYPE 이 아니라 진단 복귀 뒤 현재 raw TYPE 의 레코드와 인자 0 으로 공간 해제를 부른다(cpppj Take 의 unpop 장치: TypeRecord(raw[TYPE])).
// 전체 표를 빌리지 않은 공간 해제는 이 인자 레코드의 발자국을 쓰므로 선택 경계의 명시 장치로 관측한다. (2026-11-06 WB04-D4 음성 대조 보강)
test('SquidFactory.take: 자산 Unpop 에 현재 raw TYPE 레코드를 넘긴다', () => {
    const pool = new SidPool(32768);
    const types = constructorTypes();
    const calls = [];
    // 같은 풀의 공간 해제 경계만 관측하는 명시 장치다.
    const unpop = { pool, unpop: (sid, type, flags) => calls.push([sid, type, flags]) };
    const factory = new SquidFactory(pool, types, { unpop });
    const lines = [];
    factory.bindDiagnostics({ types, assertion: (expression, file, line) => lines.push(line), detail: () => {} });
    const sid = factory.create(74);
    const raw = pool.physical(sid);
    raw[10] = 75;
    raw[11] = 0;
    assert.equal(factory.take(74, sid), sid);
    assert.deepEqual(lines, [0x1ba]);
    assert.equal(calls.length, 1);
    assert.ok(calls[0][0] === sid && calls[0][1] === types[75] && calls[0][2] === 0);
    // Form VT 는 공간 해제가 아니라 Form Unpop(void 진단 복귀 뒤 독립 기본 분리)으로 간다.
    const form = factory.createGumpForm(0);
    lines.length = 0;
    assert.equal(factory.take(3, form), form);
    assert.equal(calls.length, 1);
    assert.equal(lines[0], 0xa5b);
});

// 실제 Unpop/contained/진단 장치의 선택 시점: !free void 도 Unpop 장치를 요구하고, 부분 쓰기(void·zOrder)는 남는다(cpppj FactoryTakeChecking_SelectedMissingDevicesPreservePartialEffects).
test('SquidFactory.take/postTake: 선택된 장치 부재의 부분 효과', () => {
    const disconnected = new FactoryTakeScene();
    disconnected.raw(disconnected.source)[11] = 4;
    const noUnpop = new SquidFactory(disconnected.pool, disconnected.types);
    const before = disconnected.raw(disconnected.source).slice();
    assert.ok(throws(() => noUnpop.take(74, disconnected.source)));
    assert.ok(same(disconnected.raw(disconnected.source), before));
    const missingContained = new FactoryTakeScene();
    missingContained.unpop.bindContainedUnpop(null);
    assert.ok(throws(() => missingContained.factory.take(74, missingContained.source)));
    assert.deepEqual([missingContained.raw(missingContained.source)[11], missingContained.word(missingContained.parent, 6)], [12, missingContained.source]);
    const missingAssertion = new FactoryTakeScene();
    const noAssertion = new SquidFactory(missingAssertion.pool, missingAssertion.types);
    const raw = missingAssertion.raw(missingAssertion.source);
    raw[35] = 0xaa;
    assert.ok(throws(() => noAssertion.postTake(missingAssertion.source)));
    assert.deepEqual([raw[35], raw[11]], [17, 8]);
    raw[10] = 3;
    raw[35] = 0xaa;
    assert.ok(throws(() => noAssertion.postTake(missingAssertion.source)));
    assert.equal(raw[35], 0xaa);
});

// Form 생성 진입: CreateContained/CreateGumpForm 은 고정 VT 가 아니라 현재 TYPE 6/3 생성자를 쓰고, 생성자 0 이면 base 와 자산 초기화(진단 포함)에 간다(cpppj FormCreationCurrent_*).
test('SquidFactory.createContained/createGumpForm: 현재 생성자와 base 경로', () => {
    const pool = new SidPool(32768);
    const types = formTypeInputs();
    const factory = new SquidFactory(pool, types);
    factory.bindDiagnostics({ types });
    const first = factory.createContained(2);
    assert.deepEqual([get(pool.slot(first), 0), pool.slot(first)[10]], [SQUID_FORM_VTABLES.content, 6]);
    types[6].constructorAddress = typeConstructorAddress(1);
    const second = factory.createContained(2);
    assert.deepEqual([get(pool.slot(second), 0), pool.slot(second)[10], pool.slot(second)[11]], [SQUID_FORM_VTABLES.depend, 6, 4]);
    types[3].constructorAddress = typeConstructorAddress(6);
    const third = factory.createGumpForm(2);
    assert.deepEqual([get(pool.slot(third), 0), pool.slot(third)[10], get(pool.slot(third), 26)], [SQUID_FORM_VTABLES.content, 3, 0]);
    const basePool = new SidPool(32768);
    const baseTypes = formTypeInputs();
    Object.assign(baseTypes[3], { constructorAddress: 0, zOrder: 0x123, flags1: 0x10, maxHitPoints: 100 });
    const baseFactory = new SquidFactory(basePool, baseTypes);
    const lines = [];
    baseFactory.bindDiagnostics({
        types: baseTypes,
        assertion: (expression, file, line) => {
            assert.deepEqual([expression, file], ['!isForm()', SQUID_CPP]);
            lines.push(line);
        },
    });
    const free = basePool.freeCount;
    const sid = baseFactory.createGumpForm(2);
    const raw = basePool.slot(sid);
    assert.deepEqual([lines, get(raw, 0), raw[10], raw[35], get(raw, 26), basePool.freeCount], [[0x7d4], BASE_VT, 3, 0x23, 100, free - 1]);
});

// Form 부착 진입: 현재 TYPE genus·GameAuxState 의 contentFormType(대여 해제 뒤 기본 6)·같은 진단을 쓴다(cpppj FormAttachCurrent_WorldUsesLiveGenusAndWeakSessionContentNumber 의 Factory 부분).
test('SquidFactory.attachForm: 현재 genus 와 빌린 contentFormType', () => {
    const pool = new SidPool(32768);
    const types = formTypeInputs();
    const factory = new SquidFactory(pool, types);
    const aux = new GameAuxState();
    let form = 0;
    const lines = [];
    // 같은 진단 장치가 정상 복귀 뒤 현재 슬롯을 고친다.
    factory.bindDiagnostics({
        types,
        assertion: (expression, file, line) => {
            assert.equal(file, SQUID_CPP);
            lines.push(line);
            const raw = pool.physical(form);
            if (expression === '!(getGenus() & (gEMPLACEMENT|gVORTEX |gFACTORY))') {
                assert.equal(raw[11], 4);
                put(raw, 0, SQUID_FORM_VTABLES.depend);
                raw[10] = 69;
            } else if (expression === 'getType()!=contentFormType') {
                assert.equal(raw[11], 8);
                raw[10] = 70;
            } else {
                assert.equal(expression, 'isForm()');
                raw[10] = 1;
            }
        },
    });
    const parent = factory.create(74, 2);
    form = factory.createGumpForm(2);
    pool.physical(form)[10] = 7;
    types[7].flags2 = 0x44200;
    aux.contentFormType = 69;
    factory.bindFormAttach(squidFormGlobals(aux).contentFormType);
    factory.attachForm(form, parent, 0x10);
    assert.deepEqual(lines, [0x3d0, 0x3e0, 0x3e3]);
    assert.deepEqual([pool.slot(form)[10], pool.slot(form)[11], commonContainedParent(pool, form), get(pool.slot(parent), 6, 2)], [1, 8, parent, form]);
    // 대여를 반납하면 같은 69 를 재사용하지 않고 기본 분리/번호로 돌아간다.
    factory.bindFormAttach(null);
    factory.unpopForm(form);
    assert.equal(get(pool.slot(parent), 6, 2), 0);
    form = factory.createGumpForm(2);
    factory.attachForm(form, parent, 0x10);
    assert.deepEqual([lines.length, pool.slot(form)[11]], [3, 8]);
});

// Form 부착 부분 쓰기: 늦은 꼬리 진단 부재와 이웃 VT 미복원은 앞선 링크 쓰기를 남긴다(cpppj FormAttachCurrent_LateContentDiagnosticAndMissingNeighborKeepPrefix).
test('SquidFactory.attachForm: 늦은 꼬리 진단·이웃 VT 부재의 부분 쓰기', () => {
    const pool = new SidPool(32768);
    const factory = new SquidFactory(pool, formTypeInputs());
    const parent = factory.create(74, 2);
    const content = factory.createContained(2);
    const free = pool.freeCount;
    assert.ok(throws(() => factory.attachForm(content, parent, 0x10)));
    assert.deepEqual([pool.slot(content)[11], commonContainedParent(pool, content), get(pool.slot(parent), 6, 2), pool.freeCount], [8, parent, content, free]);
    const form = factory.createGumpForm(2);
    const neighbor = factory.create(74, 2);
    const raw = pool.physical(form);
    put(pool.physical(parent), 6, neighbor, 2);
    put(pool.physical(neighbor), 0, 0xdeadbeef);
    assert.ok(throws(() => factory.attachForm(form, parent, 0x10)));
    assert.deepEqual([get(raw, 16, 2), get(raw, 4, 2), raw[11], get(pool.slot(parent), 6, 2), get(raw, 14, 2)], [parent, neighbor, 4, neighbor, 0]);
});

// Form Pop 진입: 진단 뒤 현재 VT/서버와 빌린 공유 깊이(감김 포함)를 쓰고, 다른 깊이 소유자 재연결과 전송/후처리 부재를 거부한다(cpppj FormPopCurrent_ActualWorld…·SelectedMissingDevices…).
test('SquidFactory.popForm: 진단 뒤 현재 VT/권한과 공유 깊이', () => {
    const server = { value: true };
    const pool = new SidPool(32768, { serverOwner: server });
    const factory = new SquidFactory(pool, formTypeInputs());
    const depth = { value: 11 };
    factory.bindFormPopDepth(depth);
    let form = 0;
    let diagnostics = 0;
    // void 는 이미 꺼졌고 다음 현재 권한/VT/깊이를 고친다.
    factory.bindDiagnostics({
        assertion: (expression, file, line) => {
            assert.deepEqual([expression, file, line], ['!xmitJustCreatedFlag', SQUID_CPP, 0x35f]);
            diagnostics += 1;
            const raw = pool.physical(form);
            assert.deepEqual([raw[11], depth.value], [0x10, 11]);
            raw[11] = 8;
            put(raw, 0, SQUID_FORM_VTABLES.gump);
            depth.value = 0xffffffff;
            server.value = false;
        },
    });
    form = factory.createGumpForm(2);
    const raw = pool.physical(form);
    raw[10] = 255;
    raw[11] = 0x14;
    put(raw, 0, 0xdeadbeef);
    factory.popForm(form, 8);
    assert.deepEqual([diagnostics, raw[10], raw[11], depth.value, pool.isServer], [1, 255, 8, 0xffffffff, false]);
    const missingPool = new SidPool(32768);
    const missing = new SquidFactory(missingPool, formTypeInputs());
    const shared = { value: 7 };
    missing.bindFormPopDepth(shared);
    assert.ok(throws(() => missing.bindFormPopDepth({ value: 9 })));
    const gump = missing.createGumpForm(0);
    const slot = missingPool.physical(gump);
    const free = missingPool.freeCount;
    assert.ok(throws(() => missing.popForm(gump, 5)));
    assert.deepEqual([slot[11], shared.value, missingPool.freeCount], [0x10, 7, free]);
    slot[11] = 4;
    put(slot, 0, 0xdeadbeef);
    assert.ok(throws(() => missing.popForm(gump, 0x10)));
    assert.deepEqual([slot[11], shared.value, missingPool.freeCount], [0, 8, free]);
});

// Form Take/Unpop 진입: 현재 VT 의 Form Unpop 이 빌린 종속 분리(현재 contentFormType)에 닿고, 통지 뒤 바뀐 현재 생성자로 Take 하며, 반납 뒤 기본 분리로 돌아간다
// (cpppj FormUnpopCurrent_WorldTakeUsesCurrentSessionDetachAndReturnsToDefault 의 Factory 부분).
test('SquidFactory.take/unpopForm: 빌린 종속 분리와 기본 분리', () => {
    const pool = new SidPool(32768);
    const types = formTypeInputs();
    types[7].constructorAddress = typeConstructorAddress(3);
    const factory = new SquidFactory(pool, types);
    factory.bindDiagnostics({ types });
    const parent = factory.create(74);
    const form = factory.createContained();
    // 이미 부착된 단일 내용물 Form 의 링크 입력이다.
    const attach = () => {
        const raw = pool.physical(form);
        raw[11] = 8;
        put(raw, 4, 0, 2);
        put(raw, 14, 0, 2);
        put(raw, 16, parent, 2);
        put(pool.physical(parent), 6, form, 2);
    };
    attach();
    put(pool.physical(form), 18, 0x12345678);
    pool.physical(form)[10] = 7;
    let notices = 0;
    let commands = 0;
    const contained = new RawSquidContainedUnpop(pool, { contentFormType: () => 7, gumpFormType: () => 3, uiEvent: () => 0x02000039 }, {
        parent: (sid) => commonContainedParent(pool, sid),
        previous: (sid) => commonContainedPrevious(pool, sid),
        setPrevious: (sid, previous) => commonContainedSetPrevious(pool, sid, previous),
        // 실제 분리 알림 뒤 바뀐 현재 생성자를 Take 가 골라야 한다.
        parentNotice: (owner, source, reason) => {
            assert.deepEqual([owner, source, reason, pool.slot(form)[11]], [parent, form, 2, 4]);
            notices += 1;
            types[7].constructorAddress = typeConstructorAddress(1);
        },
        command: () => { commands += 1; },
    });
    factory.bindFormDetach((sid) => contained.detach(sid));
    const free = pool.freeCount;
    assert.equal(factory.take(7, form), form);
    assert.deepEqual([notices, commands, get(pool.slot(form), 0), pool.slot(form)[10], pool.slot(form)[11], get(pool.slot(form), 18), get(pool.slot(parent), 6, 2), pool.freeCount],
        [1, 0, SQUID_FORM_VTABLES.depend, 7, 4, 0x12345678, 0, free]);
    factory.bindFormDetach(null);
    attach();
    factory.unpopForm(form);
    assert.deepEqual([notices, commands, pool.slot(form)[11], get(pool.slot(parent), 6, 2)], [1, 0, 4, 0]);
});

// Retype(004b1620): void/발자국 진단은 정상 복귀하면 계속하고 TYPE BYTE 뒤 현재 TYPE 생성자를 쓰며 HP/소유자는 보존한다. 진단 장치 부재는 쓰기 전에 멈춘다(웹 계약, 원본 관측은 객체 변환 단위).
test('SquidFactory.retype: 진단 순서와 현재 생성자', () => {
    const pool = new SidPool(32768);
    const types = constructorTypes();
    // 발자국 비교용 입력이다(83 만 가로 2).
    for (const record of types) Object.assign(record, { footX: 1, footY: 1 });
    types[83].footX = 2;
    const factory = new SquidFactory(pool, types);
    const sid = factory.create(82);
    const raw = pool.physical(sid);
    raw.set([9, 8, 7, 6], 26);
    raw[34] = 3;
    factory.retype(sid, 74);
    assert.deepEqual([raw[10], get(raw, 0), ...raw.subarray(26, 30), raw[34]], [74, BASE_VT, 9, 8, 7, 6, 3]);
    raw[11] = 0;
    const before = raw.slice();
    assert.ok(throws(() => factory.retype(sid, 83)));
    assert.ok(same(raw, before));
    const lines = [];
    factory.retype(sid, 83, (line) => lines.push(line));
    assert.deepEqual([lines, raw[10], get(raw, 0)], [[0x22d, 0x22e], 83, factory.constructorVtable(types[83])]);
});
