// Form 호스트(Depend·Gump·Content)·contained 생성·내용물 부모 통지·Form 연결 검사의 계약을 명시 입력으로 검사한다(WB04-D5a).
// cpppj 의 FormLifecycleTests·DependCreateCurrentTests·ContentAttachCurrentTests·FormTakeTests(Gump/ContentField)·FormAttach/Pop/CreationCurrentTests·
// RawFormLinkedTests 의 10.78 부분을 옮겼다(기대값은 그 검사와 같다). 실제 World(RawSurfaceWorld) 검사는 같은 Factory·GameAuxState 를 직접 빌려 주는 형태로 옮겼다.
// 분배기(SquidFormDispatch)·ContentFormLifetime 이 필요한 검사는 D5b 로 남긴다. 새 CPU 관측이나 실제 플레이 검사가 아니다. (2026-11-06 WB04-D5a: Form 호스트 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { SQUID_FORM_VTABLES } from '../www/core/squid-vtables.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidForms } from '../www/core/squid-forms.js';
import { commonContainedParent } from '../www/core/squid-contained-unpop.js';
import { DependFormHost } from '../www/core/depend-form-host.js';
import { GumpFormHost } from '../www/core/gump-form-host.js';
import { ContentFormHost } from '../www/core/content-form-host.js';
import { RawContentParentNotice } from '../www/core/content-parent-notice.js';
import { RawContainedCreate, makeContainedCreateHooks, INDEPENDENT_CONTAINED_STATE } from '../www/core/contained-create.js';
import { checkRawFormLinked, readRawProcessFormParent } from '../www/core/raw-form-linked.js';

// 원문 Squid.cpp 진단 파일 이름이다. (2026-11-06 WB04-D5a)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-06 WB04-D5a)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}));

// 오류가 나는지 확인한다(성공을 무조건 인정하지 않음). (2026-11-06 WB04-D5a)
function throws(action) {
    try {
        action();
    } catch {
        return true;
    }
    return false;
}

// 같은 풀의 공장·공간 해제·삭제기 묶음이다(cpppj 검사의 공통 준비). (2026-11-06 WB04-D5a)
function worldParts(types = formTypeInputs(), { server = true } = {}) {
    const pool = new SidPool(32768, { server });
    const factory = new SquidFactory(pool, types);
    const unpop = new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536));
    const destroy = new RawSquidDestroy(pool, unpop, types);
    return { pool, types, factory, unpop, destroy };
}

// 슬롯 WORD 를 읽는다. (2026-11-06 WB04-D5a)
const word = (pool, sid, offset) => get(pool.slot(sid), offset, 2);

// 쌍 생성→원본 방향 조회→제거와, 조회 알림이 같은 쌍을 지워도 바깥 재삭제가 dead 에서 자연 반환하는지 본다(cpppj FormLifecycle_DependencyPairAndReentrantNotice).
test('DependFormHost: 쌍 생성·조회·제거와 재진입 통지', () => {
    const { pool, factory, destroy } = worldParts();
    const forms = new SquidForms(pool);
    const watched = factory.create(90, 2);
    const watcher = factory.create(91, 2);
    const depend = new DependFormHost(pool, destroy);
    const pair = depend.create(watched, watcher);
    assert.deepEqual([depend.has(watched, watcher), depend.has(watcher, watched)], [true, false]);
    assert.deepEqual([forms.parent(pair.watchedForm), forms.parent(pair.watcherForm)], [watched, watcher]);
    depend.remove(watcher, watched);
    assert.deepEqual([depend.has(watched, watcher), word(pool, watched, 6), word(pool, watcher, 6)], [false, 0, 0]);
    assert.deepEqual([pool.slot(pair.watchedForm)[11] & 3, pool.slot(pair.watcherForm)[11] & 3], [3, 3]);
    assert.deepEqual([pool.deletions(true)[0].sid, pool.deletions(true)[1].sid], [pair.watchedForm, pair.watcherForm]);
    let calls = 0;
    // 통지가 같은 쌍을 지운다.
    const reentrant = new DependFormHost(pool, destroy, {
        notice: (notifiedWatcher, notifiedWatched, form) => {
            calls += 1;
            assert.deepEqual([notifiedWatcher, notifiedWatched], [watcher, watched]);
            reentrant.destroy(form, 0x10);
        },
    });
    const second = reentrant.create(watched, watcher);
    reentrant.notify(second.watchedForm, watched);
    assert.deepEqual([calls, reentrant.has(watched, watcher), destroy.preDepth, destroy.postDepth], [1, false, 0, 0]);
});

// 조회/제거는 VT 가 아니라 현재 Depend TYPE 번호로 후보를 고르고, TYPE 이 맞으면 상대 조회가 감시 VT 를 요구한다(cpppj DependFormHost::Has/Remove 의 원문 비교, WB04-D5a 음성 대조 보강).
test('DependFormHost.has/remove: 현재 Depend TYPE 번호로 후보를 고른다', () => {
    const { pool, factory, destroy } = worldParts();
    const depend = new DependFormHost(pool, destroy);
    const watched = factory.create(90, 2);
    const watcher = factory.create(91, 2);
    const pair = depend.create(watched, watcher);
    // VT 는 감시 클래스지만 TYPE 번호가 다르면 후보가 아니다.
    pool.physical(pair.watchedForm)[10] = 9;
    assert.equal(depend.has(watched, watcher), false);
    depend.remove(watcher, watched);
    assert.deepEqual([word(pool, watched, 6), pool.slot(pair.watchedForm)[11] & 3], [pair.watchedForm, 0]);
    // TYPE 이 같아도 감시 VT 가 아니면 상대 조회에서 거부한다.
    pool.physical(pair.watchedForm)[10] = 1;
    put(pool.physical(pair.watchedForm), 0, SQUID_FORM_VTABLES.gump);
    assert.ok(throws(() => depend.has(watched, watcher)));
});

// 공통 부착은 링크 완료 뒤 Pop 전송 부재에서 멈추고, 같은 부모 감시 쌍도 해제하며, Content 의 WORLD/dead 진단 부재를 거부한다(cpppj FormLifecycle_ContractsAndSameParentPair).
test('Form 호스트: 부분 쓰기·같은 부모 쌍·Content 진단 부재', () => {
    const { pool, types, factory, destroy } = worldParts();
    const forms = new SquidForms(pool);
    const parent = factory.create(90, 2);
    const form = forms.create(1, 0);
    const before = Buffer.from(pool.slot(form)).toString('hex');
    assert.ok(throws(() => forms.attach(form, parent)));
    assert.ok(Buffer.from(pool.slot(form)).toString('hex') !== before);
    assert.deepEqual([pool.slot(form)[11], word(pool, form, 16), word(pool, parent, 6)], [8, parent, form]);
    forms.unpop(form);
    assert.deepEqual([pool.slot(form)[11], word(pool, parent, 6)], [4, 0]);
    const depend = new DependFormHost(pool, destroy);
    const pair = depend.create(parent, parent);
    assert.ok(depend.has(parent, parent));
    depend.destroy(pair.watchedForm, 0x10);
    assert.deepEqual([word(pool, parent, 6), destroy.preDepth], [0, 0]);
    const content = new ContentFormHost(pool, types, () => {});
    const value = factory.createContained(2);
    put(pool.allocated(value), 20, 0x80, 2);
    assert.ok(throws(() => content.robust(value)));
    put(pool.allocated(value), 20, 0x20, 2);
    pool.allocated(parent)[11] |= 2;
    assert.ok(throws(() => content.attach(value, parent, 0x10)));
    assert.ok(pool.slot(value)[11] & 4);
});

// contained 생성기의 TYPE 6 생성→필드→정렬 부착→사제 포획 알림과 Damage 재부착·Unpop 통지 순서(cpppj FormLifecycle_ContentCreateAndCurrentCaptureState).
test('ContentFormHost: contained 생성·전투 사제 포획·Damage 재부착', () => {
    const types = formTypeInputs();
    types[90].flags2 = 0x10000;
    types[80].flags2 = 0x200000;
    const { pool, factory } = worldParts(types);
    const parent = factory.create(90, 2);
    const reasons = [];
    let captured = 0;
    const content = new ContentFormHost(pool, types, (notifiedParent, form, reason) => {
        assert.equal(notifiedParent, parent);
        reasons.push(reason);
        if (reason === 1) put(pool.allocated(form), 12, 0xfe, 2);
    }, {
        state: { inBattle: () => true, checkingLinks: () => true },
        captured: (capturedParent, previousOwner) => {
            assert.deepEqual([capturedParent, previousOwner], [parent, 0x7e]);
            captured += 1;
        },
    });
    const create = new RawContainedCreate(pool, INDEPENDENT_CONTAINED_STATE, content.bindCreate(factory, { notifySurface: () => {} }));
    const form = create.create(parent, 80, 7, 0x20, 0x10, 3);
    assert.ok(content.robust(form));
    assert.deepEqual([captured, reasons, word(pool, form, 18), word(pool, parent, 6), pool.slot(form)[30]], [1, [1], 80, form, 3]);
    assert.equal(content.damage(form, 0, -3.5), -3.5);
    assert.equal(content.damage(form, 1, 9), 0);
    assert.deepEqual([captured, reasons, word(pool, form, 20) & 0x100], [2, [1, 2, 1], 0]);
    content.unpop(form);
    assert.deepEqual([reasons.at(-1), word(pool, parent, 6)], [2, 0]);
});

// 감시 쌍 생성은 두 Create 마다 현재 Depend TYPE(GameAuxState)을 읽고, 진단이 바꾼 VT 의 감시 클래스로 제거하며, 전체 DWORD 범위 오류는 첫 할당 뒤에 멈춘다
// (cpppj DependCreateCurrent_WorldReadsNumberTwiceAndKeepsPreviousClassRoute 의 Factory/GameAuxState 부분).
test('DependFormHost.create: 현재 Depend TYPE 두 번 읽기와 감시 클래스 제거', () => {
    const types = formTypeInputs();
    types[7].constructorAddress = 0;
    types[8].constructorAddress = typeConstructorAddress(1);
    const { pool, factory, destroy } = worldParts(types);
    const aux = new GameAuxState();
    let expectedFirst = 0;
    let diagnostics = 0;
    // base postCreate 진단의 정상 복귀가 실제 VT 와 현재 번호를 바꾼다.
    factory.bindDiagnostics({
        types,
        assertion: (expression, file, line) => {
            assert.deepEqual([expression, file, line], ['!isForm()', SQUID_CPP, 0x7d4]);
            diagnostics += 1;
            const raw = pool.allocated(expectedFirst);
            assert.equal(raw[10], 7);
            put(raw, 0, SQUID_FORM_VTABLES.depend);
            aux.dependFormType = 8;
        },
    });
    const depend = new DependFormHost(pool, destroy);
    depend.bindCreation(factory, () => aux.dependFormType);
    // 같은 소유자로 두 세대를 반복한다.
    for (let generation = 0; generation < 2; generation += 1) {
        aux.dependFormType = 7;
        const watched = factory.create(74, 2);
        const watcher = factory.create(74, 2);
        expectedFirst = pool.firstFree(true);
        const free = pool.freeCount;
        const before = diagnostics;
        const pair = depend.create(watched, watcher);
        assert.deepEqual([diagnostics, aux.dependFormType, pair.watcherForm], [before + 1, 8, expectedFirst]);
        assert.deepEqual([pool.slot(pair.watcherForm)[10], pool.slot(pair.watchedForm)[10]], [7, 8]);
        assert.ok(depend.isDependForm(pair.watcherForm) && depend.isDependForm(pair.watchedForm) && depend.has(watched, watcher));
        assert.deepEqual([commonContainedParent(pool, pair.watcherForm), commonContainedParent(pool, pair.watchedForm)], [watcher, watched]);
        depend.remove(watcher, watched, 0x10);
        assert.deepEqual([depend.has(watched, watcher), pool.freeCount, word(pool, watched, 6), word(pool, watcher, 6), destroy.preDepth, destroy.postDepth], [false, free, 0, 0, 0, 0]);
        aux.dependFormType = 0xffffffff;
        const remaining = pool.freeCount;
        assert.ok(throws(() => depend.create(watched, watcher)));
        assert.equal(pool.freeCount, remaining - 1);
    }
    aux.resetForGeneralWorld();
    assert.equal(aux.dependFormType, 0xffffffff);
});

// 다른 풀 공장 연결은 거부하고, 선택된 Process 부착 접두 부재는 앞선 두 할당/링크를 남긴다(cpppj DependCreateCurrent_BindingAndSelectedAttachFailureKeepLinks).
test('DependFormHost.create: 다른 풀 공장 거부와 Process 부착 부재의 부분 효과', () => {
    const types = formTypeInputs();
    types[7].constructorAddress = typeConstructorAddress(2);
    const { pool, factory, destroy } = worldParts(types);
    const foreign = new SquidFactory(new SidPool(32768), types);
    const host = new DependFormHost(pool, destroy);
    assert.ok(throws(() => host.bindCreation(foreign, () => 7)));
    host.bindCreation(factory, () => 7);
    const watched = factory.create(74, 2);
    const watcher = factory.create(74, 2);
    const first = pool.firstFree(true);
    const free = pool.freeCount;
    assert.ok(throws(() => host.create(watched, watcher)));
    assert.equal(pool.freeCount, free - 2);
    const second = word(pool, first, 20);
    assert.deepEqual([word(pool, first, 18), word(pool, second, 18), word(pool, second, 20), word(pool, watched, 6), word(pool, watcher, 6)], [0, first, 0, 0, 0]);
});

// 독립 호스트도 TYPE 1 의 현재 생성자를 읽고(Gump VT 를 Depend VT 로 덮지 않음), Content VT 도 공통 thunk 로 부착한다
// (cpppj DependCreateCurrent_IndependentHostUsesCurrentInputConstructor·FormAttachCurrent_ContentVtableReallyUsesCommonThunk).
test('DependFormHost.create: 현재 생성자 VT 와 공통 부착 thunk', () => {
    const gumpTypes = formTypeInputs();
    gumpTypes[1].constructorAddress = typeConstructorAddress(3);
    const gumpWorld = worldParts(gumpTypes);
    const host = new DependFormHost(gumpWorld.pool, gumpWorld.destroy);
    const watched = gumpWorld.factory.create(74, 2);
    const watcher = gumpWorld.factory.create(74, 2);
    const pair = host.create(watched, watcher);
    const pool = gumpWorld.pool;
    assert.deepEqual([get(pool.slot(pair.watcherForm), 0), get(pool.slot(pair.watchedForm), 0), pool.slot(pair.watcherForm)[10]], [SQUID_FORM_VTABLES.gump, SQUID_FORM_VTABLES.gump, 1]);
    assert.ok(!host.isDependForm(pair.watcherForm));
    assert.deepEqual([commonContainedParent(pool, pair.watcherForm), commonContainedParent(pool, pair.watchedForm)], [watcher, watched]);
    const contentTypes = formTypeInputs();
    contentTypes[7].constructorAddress = typeConstructorAddress(6);
    const contentWorld = worldParts(contentTypes);
    const bound = new DependFormHost(contentWorld.pool, contentWorld.destroy);
    bound.bindCreation(contentWorld.factory, () => 7);
    const parentA = contentWorld.factory.create(74, 2);
    const parentB = contentWorld.factory.create(74, 2);
    const contentPair = bound.create(parentA, parentB);
    // 두 Form 모두 Content VT·TYPE 7·contained 로 붙는다.
    for (const sid of [contentPair.watcherForm, contentPair.watchedForm]) {
        const raw = contentWorld.pool.slot(sid);
        assert.deepEqual([get(raw, 0), raw[10], raw[11]], [SQUID_FORM_VTABLES.content, 7, 8]);
    }
    assert.deepEqual([commonContainedParent(contentWorld.pool, contentPair.watcherForm), word(contentWorld.pool, parentB, 6)], [parentB, contentPair.watcherForm]);
});

// 실제 Gump 부착→지정 Take→재부착→공통 삭제는 소유 DWORD/payload 와 같은 풀/깊이를 유지한다(cpppj FormTake_GumpAttachmentAndDestroyIntegration).
test('GumpFormHost: 부착·Take·재부착·삭제 통합', () => {
    const { pool, types, factory, destroy } = worldParts();
    const forms = new GumpFormHost(pool, factory, types, destroy);
    const parent = factory.create(90, 2);
    const form = forms.create(0);
    const head = forms.create(2);
    forms.setOwner(form, 0xabcd5678);
    forms.attach(form, parent, 0x10);
    forms.attach(head, parent, 0x10);
    const count = pool.freeCount;
    assert.equal(word(pool, parent, 6), head);
    assert.equal(forms.take(form), form);
    assert.deepEqual([forms.owner(form), forms.parent(form), word(pool, parent, 6), word(pool, head, 4), pool.freeCount, pool.slot(form)[11] & 12], [0xabcd5678, parent, head, 0, count, 4]);
    forms.attach(form, parent, 0x10);
    forms.destroy(form, 0x10);
    assert.deepEqual([pool.slot(form)[11] & 3, word(pool, parent, 6), destroy.preDepth, destroy.postDepth, pool.deletions(false)[0].type], [3, head, 0, 0, 3]);
});

// 부모 삭제의 release 는 소유 ID 가 있을 때만 UI 통지(owner, 현재 부모)를 부르고, 반납된 슬롯은 TYPE 기록으로 판정한다(cpppj FormClassLifetime 의 Gump 부분 계약).
test('GumpFormHost: 부모 삭제 통지와 클래스 판정', () => {
    const { pool, types, factory, destroy } = worldParts();
    const notices = [];
    const gumps = new GumpFormHost(pool, factory, types, destroy, { notice: (owner, parent) => notices.push([owner, parent]) });
    const parent = gumps.create(2);
    const owned = gumps.create(2);
    const silent = gumps.create(2);
    gumps.setOwner(owned, 0x12345678);
    gumps.attach(owned, parent, 0x10);
    gumps.attach(silent, parent, 0x10);
    gumps.destroy(parent, 0);
    assert.deepEqual(notices, [[0x12345678, parent]]);
    // 부모와 함께 지워진 종속 Gump 도 반납되며, 반납 슬롯은 TYPE 기록(3)으로 판정한다.
    assert.ok(gumps.isGumpForm(parent) && (pool.slot(parent)[11] & 1) && (pool.slot(owned)[11] & 1) && gumps.isGumpForm(owned));
    // 사용 중 슬롯은 바뀐 TYPE 이 아니라 현재 VT 로 판정한다.
    const probe = gumps.create(2);
    pool.physical(probe)[10] = 7;
    assert.ok(gumps.isGumpForm(probe));
    put(pool.physical(probe), 0, SQUID_FORM_VTABLES.depend);
    assert.ok(!gumps.isGumpForm(probe) && throws(() => gumps.owner(probe)) && throws(() => gumps.destroy(probe)));
    assert.ok(throws(() => new GumpFormHost(new SidPool(32768), factory, types, destroy)));
});

// 통지 뒤 현재 TYPE 의 소비 위치: 빌린 현재 표면 통지가 바꾼 부모 TYPE(255)의 범위 오류가 전투 판정보다 먼저 드러난다(cpppj ContentAttachCurrent_ActualHostReadsBorrowedTypesAfterNotice).
test('ContentFormHost.attach: 통지 뒤 빌린 현재 TYPE 재조회', () => {
    // 독립 사본과 실제 현재 표 대여의 차이다.
    for (const current of [false, true]) {
        const { pool, types, factory } = worldParts();
        const parent = factory.create(90, 2);
        const form = factory.createContained(2);
        put(pool.allocated(form), 18, 85, 2);
        let notices = 0;
        let captured = 0;
        const state = {};
        const host = new ContentFormHost(pool, types, () => {
            notices += 1;
            types[85].flags2 = 0x200000;
            pool.allocated(parent)[10] = 255;
        }, { state, captured: () => { captured += 1; } });
        if (current) host.bindCurrentTypes(types);
        host.bindPop((sid) => { pool.allocated(sid)[11] &= ~4 & 0xff; });
        let stopped = false;
        try {
            host.attach(form, parent, 0x10);
        } catch (error) {
            stopped = error instanceof RangeError;
        }
        assert.deepEqual([stopped, notices, captured, word(pool, parent, 6)], [current, 1, 0, form]);
    }
});

// void 진단·생산 TYPE 의 늦은 소비보다 정렬 링크가 먼저 끝나야 하고, 빌린 Pop 에서 멈추면 그 앞의 링크/부모/contained 가 남는다(cpppj ContentAttachCurrent_ActualHostReachesPopAfterLinks…).
test('ContentFormHost.attach: 늦은 Pop 실패 앞의 링크 완료', () => {
    // void 0 입력과 범위 밖 생산 TYPE 입력이다.
    for (const missingVoid of [true, false]) {
        const { pool, types, factory } = worldParts();
        const parent = factory.create(90, 2);
        const form = factory.createContained(2);
        const raw = pool.allocated(form);
        put(raw, 18, missingVoid ? 85 : 0xffff, 2);
        if (missingVoid) raw[11] &= ~4 & 0xff;
        let calls = 0;
        const host = new ContentFormHost(pool, types, () => { throw new Error('늦은 Pop 실패 뒤 통지 도달 오류'); });
        // 링크·부모·contained 가 끝난 뒤의 Pop 에서 멈춘다.
        host.bindPop((current, flags) => {
            calls += 1;
            assert.deepEqual([current, flags, word(pool, parent, 6), word(pool, form, 16), pool.slot(form)[11] & 8], [form, 0x110, form, parent, 8]);
            throw new Error('pop stop');
        });
        assert.throws(() => host.attach(form, parent, 0x10), /pop stop/);
        assert.deepEqual([calls, word(pool, parent, 6), word(pool, form, 4), word(pool, form, 14), word(pool, form, 16), pool.slot(form)[11] & 8], [1, form, 0, 0, parent, 8]);
    }
});

// 직접 필드 조회는 TYPE/상태 필터 없이 현재 부모 getter 와 전체 체인을 쓴다(cpppj ContentFieldCurrent_AliasesStatesAndWholeChain 의 10.78 부분, 48 입력).
test('ContentFormHost.robust/linked: TYPE·상태 별칭과 전체 체인', () => {
    const { pool, types, factory } = worldParts();
    const content = new ContentFormHost(pool, types, () => {});
    const parent = factory.createContained(2);
    let inputs = 0;
    // 네 실제 Form getter 를 쓴다.
    for (const formType of [1, 2, 3, 6]) {
        const form = factory.create(formType, 2);
        const raw = pool.physical(form);
        put(raw, 16, parent, 2);
        put(raw, 20, 0x20, 2);
        put(pool.physical(parent), 6, form, 2);
        // 직접 몸체가 읽지 않는 TYPE BYTE 들이다.
        for (const type of [0, 6, 7, 63, 170, 255]) {
            // free/dead/void 필터를 추가하지 않는다.
            for (const state of [4, 255]) {
                raw[10] = type;
                raw[11] = state;
                raw[4] = 0;
                raw[5] = 0;
                assert.ok(content.robust(form) && content.linked(form));
                raw[5] = 0x80;
                assert.ok(!content.linked(form));
                inputs += 1;
            }
        }
        raw[10] = formType;
        raw[11] = 4;
        raw[4] = 0;
        raw[5] = 0;
        put(pool.physical(parent), 6, 0, 2);
        pool.release(form);
    }
    pool.release(parent);
    assert.equal(inputs, 48);
});

// WORLD 진단의 정상 복귀 뒤 현재 flags 와 다시 확보된 풀을 읽고 실행 중 장치를 끝까지 쓴다(cpppj ContentFieldCurrent_RobustDiagnosticReadsCurrentFlagsAfterRepair).
test('ContentFormHost.robust: WORLD 진단 뒤 현재 flags 와 재확보 풀', () => {
    // 진단 복귀 뒤 WORLD 를 다시 검사하거나 이전 flags 를 반환하지 않는다.
    for (const keepWorld of [false, true]) {
        const { pool, types, factory } = worldParts();
        const form = factory.createContained(2);
        put(pool.allocated(form), 20, 0x80, 2);
        let diagnostics = 0;
        const state = {};
        // 진단이 현재 바이트/장치와 물리 저장소를 바꾼다.
        state.assertion = (expression, file, line) => {
            assert.deepEqual([expression, file, line], ['!(getListFlags() & CFF_WORLD)', SQUID_CPP, 0xbb1]);
            diagnostics += 1;
            state.assertion = null;
            pool.reset(32768 + 2);
            const raw = pool.physical(form);
            raw[10] = 255;
            raw[11] = 255;
            raw[20] = keepWorld ? 0xa0 : 0;
        };
        const content = new ContentFormHost(pool, types, () => {}, { state });
        assert.deepEqual([content.robust(form), diagnostics, pool.capacity], [keepWorld, 1, 32770]);
    }
});

// Regular+5c(damage)의 count 0/음수는 this 를 읽지 않고 받은 float 를 그대로 돌려준다(범위 밖 번호·-0·음수 비트 포함)(cpppj ContentFieldCurrent_ZeroNegativeRegularSkipsObjectFields).
test('ContentFormHost.damage: 0/음수 count 의 조기 반환', () => {
    const { pool, types } = worldParts();
    const content = new ContentFormHost(pool, types, () => {});
    // 범위 밖 this·여러 count·float 비트 조합이다.
    for (const form of [0, 65535]) {
        for (const count of [0, -1, -2147483648]) {
            for (const bits of [0x80000000, 0xc0600000]) assert.equal(floatToBits(content.damage(form, count, bitsToFloat(bits))), bits);
        }
    }
});

// 분리 콜백의 VT/부모/flags 변경 뒤에도 원문이 먼저 저장한 부모로 현재 클래스 재부착을 하고, 선택 실패는 완료 접두를 남긴다(cpppj ContentFieldCurrent_ReattachUsesCurrentClassCachedParentAndWritePrefix).
test('ContentFormHost.damage: 저장한 부모의 현재 클래스 재부착', () => {
    // Content 재부착/Gump 전환/장치 실패/미연결 다른 VT 이다.
    for (const profile of [0, 1, 2, 3]) {
        const { pool, types, factory } = worldParts();
        const depth = { value: 17 };
        factory.bindFormPopDepth(depth);
        const parent = factory.createContained(2);
        const other = factory.createContained(2);
        const form = factory.createContained(2);
        const content = new ContentFormHost(pool, types, () => {});
        content.bindPop((sid, flags) => factory.popForm(sid, flags));
        put(pool.allocated(form), 18, 75, 2);
        put(pool.allocated(form), 20, 0x120, 2);
        content.attach(form, parent, 0x10);
        pool.allocated(form)[10] = 7;
        const detached = new SquidForms(pool, { notice: () => {} });
        let reattached = 0;
        // 분리 콜백이 부모·flags·VT 를 바꾼다.
        content.bindUnpop((sid) => {
            assert.equal(sid, form);
            detached.unpop(sid);
            put(pool.physical(form), 16, other, 2);
            put(pool.physical(form), 20, 0xab34, 2);
            if (profile === 1 || profile === 3) put(pool.physical(form), 0, SQUID_FORM_VTABLES.gump);
        });
        if (profile !== 3) {
            // 선택한 현재 클래스의 실제 공통/정렬 부착이다.
            content.bindReattach((sid, into, flags) => {
                assert.deepEqual([sid, into, flags, word(pool, form, 20), depth.value], [form, parent, 0x10, 0xaa34, 17]);
                reattached += 1;
                if (profile === 2) throw new Error('명시 재부착 장치 실패');
                if (profile === 1) factory.attachForm(sid, into, flags);
                else content.attach(sid, into, flags);
            });
        }
        const failed = throws(() => assert.equal(content.damage(form, 1, 9), 0));
        assert.deepEqual([failed, reattached, depth.value, word(pool, form, 20)], [profile >= 2, profile === 3 ? 0 : 1, 17, 0xaa34], `profile ${profile}`);
        assert.deepEqual([word(pool, form, 16), word(pool, parent, 6), pool.slot(form)[11] & 4], profile >= 2 ? [other, 0, 4] : [parent, form, 0], `profile ${profile}`);
    }
});

// 정렬 부착 뒤 같은 공장의 공통 Pop 공유 깊이를 빌리고, Unpop 은 부모 제거 2 를 알린다(cpppj FormPopCurrent_SortedContentBorrowsSameFactoryDepth).
test('ContentFormHost: 빌린 공장 Pop 의 공유 깊이와 제거 통지', () => {
    const { pool, types, factory } = worldParts();
    const depth = { value: 12 };
    factory.bindFormPopDepth(depth);
    const parent = factory.create(74, 2);
    const form = factory.createContained(2);
    put(pool.allocated(form), 18, 154, 2);
    put(pool.allocated(form), 20, 0x20, 2);
    let notices = 0;
    let pops = 0;
    const content = new ContentFormHost(pool, types, (actualParent, actualForm, reason) => {
        assert.ok(actualParent === parent && actualForm === form && depth.value === 12 && (reason === 1 || reason === 2));
        notices += 1;
    });
    // 같은 공장의 공통 Pop 이다.
    content.bindPop((actual, flags) => {
        assert.deepEqual([actual, flags, depth.value], [form, 0x110, 12]);
        pops += 1;
        factory.popForm(actual, flags);
    });
    content.attach(form, parent, 0x10);
    assert.deepEqual([pops, notices, depth.value, pool.slot(form)[11], word(pool, parent, 6)], [1, 1, 12, 8, form]);
    content.unpop(form);
    assert.deepEqual([notices, depth.value, pool.slot(form)[11], word(pool, parent, 6)], [2, 12, 4, 0]);
});

// 네 Form 의 +24 빈 통지는 TYPE/상태·범위 밖 form 인자를 읽지 않고, 예약 부모 0 도 실제 머리로 연결을 검사한다(cpppj ContentFieldCurrent_AllEightFormParentNoticesAreEmpty 의 10.78 부분).
test('RawContentParentNotice: Form 빈 통지와 파생 분배', () => {
    const { pool, types, factory } = worldParts();
    let derived = 0;
    const notice = new RawContentParentNotice(pool, () => { derived += 1; });
    // 네 실제 생성자 VT 의 빈 몸체다.
    for (const kind of [1, 2, 3, 6]) {
        const parent = factory.create(kind, 2);
        const raw = pool.physical(parent);
        // 빈 몸체가 읽지 않는 필드를 바꾼다.
        for (const type of [0, 7, 170, 255]) {
            // 상태 BYTE 도 바꾼다.
            for (const state of [0, 255]) {
                raw[10] = type;
                raw[11] = state;
                notice.notice(parent, 65535, 0xffffffff);
            }
        }
        raw[10] = kind;
        raw[11] = 4;
        pool.release(parent);
    }
    assert.equal(derived, 0);
    // 워커 계열 VT(0x503d80)의 파생 진입은 장치로 가고, 미지원 VT 는 거부한다.
    const walker = factory.create(74, 2);
    put(pool.physical(walker), 0, 0x503d80);
    notice.notice(walker, 5, 1);
    assert.equal(derived, 1);
    // 파생 장치가 없으면 파생 진입에서 멈추고, 빈 몸체 VT 는 장치 없이도 그대로 돌아간다.
    assert.ok(throws(() => new RawContentParentNotice(pool).notice(walker, 5, 1)));
    new RawContentParentNotice(pool).notice(factory.create(74, 2), 5, 1);
    put(pool.physical(walker), 0, 0x12345678);
    assert.ok(throws(() => notice.notice(walker, 5, 1)));
    assert.equal(derived, 1);
    const form = factory.createGumpForm(2);
    put(pool.physical(form), 16, 0, 2);
    put(pool.physical(0), 6, form, 2);
    const content = new ContentFormHost(pool, types, () => {});
    assert.ok(content.linked(form));
    pool.physical(form)[5] = 0x80;
    assert.ok(!content.linked(form));
});

// 연결 검사는 부모 getter 가 풀을 다시 확보해도 현재 저장소를 읽고, 원문 비종료 순환은 명시 오류이며, ProcessForm getter 는 +16 WORD 만 돌려준다(cpppj RawFormLinked_* 의 10.78 부분).
test('checkRawFormLinked/readRawProcessFormParent: 현재 풀·순환·Process getter', () => {
    const pool = new SidPool(32768);
    let calls = 0;
    // getter 가 풀을 다시 확보하고 새 저장소에 체인을 둔다.
    const parent = (form) => {
        assert.equal(form, 50);
        calls += 1;
        pool.reset(32770);
        put(pool.physical(1), 6, 50, 2);
        put(pool.physical(50), 4, 0, 2);
        return 1;
    };
    assert.deepEqual([checkRawFormLinked(pool, 50, parent), calls, pool.capacity], [true, 1, 32770]);
    put(pool.physical(50), 4, 50, 2);
    const fixed = () => 1;
    assert.ok(throws(() => checkRawFormLinked(pool, 50, fixed)));
    put(pool.physical(50), 4, 32770, 2);
    assert.equal(checkRawFormLinked(pool, 50, fixed), false);
    assert.ok(throws(() => checkRawFormLinked(pool, 50, null)));
    const raw = pool.physical(50);
    put(raw, 0, SQUID_FORM_VTABLES.process);
    raw[10] = 0;
    raw[11] = 0xff;
    raw[16] = 0xcd;
    raw[17] = 0xab;
    assert.equal(readRawProcessFormParent(pool, 50), 0xabcd);
    raw[0] = 0;
    assert.ok(throws(() => readRawProcessFormParent(pool, 50)));
});

// contained 생성은 할당 뒤 현재 contained TYPE 과, 생성 자식이 바꾼 현재 다리 TYPE 을 읽는다(cpppj FormCreationCurrent_ContainedAdapterUsesBorrowedTypeAndPostCreateBridge 의 10.78 부분).
test('RawContainedCreate: 현재 contained/다리 TYPE 과 필드 쓰기 순서', () => {
    const types = formTypeInputs();
    types[7].constructorAddress = typeConstructorAddress(6);
    types[8].constructorAddress = typeConstructorAddress(3);
    const { pool, factory } = worldParts(types);
    factory.bindDiagnostics({ types });
    let containedType = 7;
    let bridgeType = 82;
    let notices = 0;
    let attachments = 0;
    const state = { containedType: () => containedType, bridgeType: () => bridgeType };
    const hooks = makeContainedCreateHooks(pool, factory, {
        // 표면 알림은 TYPE/필드 WORD 쓰기 뒤다.
        notifySurface: (sid) => {
            assert.deepEqual([pool.slot(sid)[10], word(pool, sid, 18), word(pool, sid, 12)], [containedType, 0x1234, 0x5678]);
            notices += 1;
        },
        // 부착은 품질 BYTE·flags WORD 뒤다.
        attach: (sid, parent, flags) => {
            assert.deepEqual([parent, flags, word(pool, sid, 20), pool.slot(sid)[30]], [50, 0x10, 0x2345, 0x67]);
            attachments += 1;
        },
    });
    const create = hooks.create;
    // 자식 정상 복귀가 현재 다리 번호를 바꾼다(부모가 미리 읽으면 실패).
    hooks.create = (type, flags) => {
        const sid = create(type, flags);
        bridgeType = type;
        return sid;
    };
    const body = new RawContainedCreate(pool, state, hooks);
    const first = body.create(50, 0x1234, 0x5678, 0x12345, 0x10, 0x167);
    assert.deepEqual([pool.slot(first)[10], notices, attachments], [7, 1, 1]);
    containedType = 8;
    const second = body.create(50, 0x1234, 0x5678, 0x12345, 0x10, 0x167);
    assert.deepEqual([pool.slot(second)[10], notices, attachments], [8, 2, 2]);
    assert.ok(throws(() => body.create(0, 1, 2, 3, 0x10, 4)));
    assert.ok(throws(() => new RawContainedCreate(pool, state, { create: () => 5 })));
});
