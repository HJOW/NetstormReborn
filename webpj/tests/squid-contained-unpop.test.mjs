// 공통 contained 해제 종단(RawSquidContainedUnpop)의 링크·진단 복귀·UI 큐 계약을 명시 입력으로 검사한다(WB04-D2).
// 원본 x86 관측 표가 없는 모듈이라 cpppj tests/SquidContainedUnpopTests.cpp 의 계약 검사 중 10.78 부분을 옮겼다(기대값은 그 검사와 같다).
// 새 CPU 관측이나 실제 플레이 검사가 아니다. Take 상위 경로(FactoryTakeChecking)는 Factory(D4)와 함께 옮긴다.
// (2026-11-05 WB04-D2: 공통 contained 종단 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawSquidContainedUnpop, commonContainedParent, commonContainedPrevious, commonContainedSetPrevious } from '../www/core/squid-contained-unpop.js';

// 원문 Squid.cpp 진단 파일 이름이다. (2026-11-05 WB04-D2)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';

// cpppj ContainedUnpopScene 과 같은 입력 장면이다: 풀 24000, source 50(TYPE 74, 상태 12), parent 70(TYPE 74, +6 머리 = source). (2026-11-05 WB04-D2)
class ContainedScene {
    // 0~99 슬롯을 비우고 기본 링크를 둔다.
    constructor() {
        // 검사 소유 풀과 대상 번호이다.
        this.pool = new SidPool(24000);
        this.source = 50;
        this.parent = 70;
        this.previous = 0;
        // 현재 전역 입력이다(getter 가 호출 때마다 읽는다).
        this.contentFormType = 6;
        this.gumpFormType = 3;
        this.uiEvent = 0x02000039;
        this.checking = 0;
        this.metamap = false;
        this.boss = true;
        // 장치 호출 횟수와 진단·명령 기록, 장치 중 입력 변경이다.
        this.parentCalls = 0;
        this.previousCalls = 0;
        this.setPreviousCalls = 0;
        this.noticeCalls = 0;
        this.diagnostics = [];
        this.commands = [];
        this.onAssertion = null;
        this.onNotice = null;
        // 관측할 합성 슬롯만 초기화한다.
        for (let sid = 0; sid < 100; sid += 1) this.raw(sid).fill(0);
        this.raw(this.source)[10] = 74;
        this.raw(this.source)[11] = 12;
        this.raw(this.parent)[10] = 74;
        this.put(this.parent, 6, this.source);
    }

    // 물리 슬롯 쓰기 대여이다(free 진단 복귀도 같은 바이트).
    raw(sid) { return this.pool.physical(sid); }

    // WORD 를 읽는다.
    word(sid, offset) { return get(this.raw(sid), offset, 2); }

    // WORD 만 쓴다.
    put(sid, offset, value) { put(this.raw(sid), offset, value, 2); }

    // 현재 전역 getter 묶음이다.
    state() {
        return {
            contentFormType: () => this.contentFormType,
            gumpFormType: () => this.gumpFormType,
            uiEvent: () => this.uiEvent,
            checking: () => this.checking,
            inMetamapMode: () => this.metamap,
            boss: () => this.boss,
        };
    }

    // 가상 getter 반환 입력과 setter 쓰기 위치를 제품 몸체와 분리한 장치이다.
    hooks() {
        return {
            parent: (sid) => { assert.equal(sid, this.source); this.parentCalls += 1; return this.parent; },
            previous: (sid) => { assert.equal(sid, this.source); this.previousCalls += 1; return this.previous; },
            setPrevious: (sid, value) => { this.setPreviousCalls += 1; this.put(sid, this.raw(sid)[10] < 70 ? 14 : 16, value); },
            parentNotice: (parent, sid, reason) => {
                assert.deepEqual([parent, sid, reason], [this.parent, this.source, 2]);
                this.noticeCalls += 1;
                if (this.onNotice) this.onNotice();
            },
            command: (event, object, value) => this.commands.push([event, object, value]),
            assertion: (expression, file, line) => {
                this.diagnostics.push(line);
                if (this.onAssertion) this.onAssertion(expression, file, line);
            },
        };
    }

    // 처리기를 만든다(장치 일부를 바꿀 수 있다).
    make(overrides = {}) { return new RawSquidContainedUnpop(this.pool, this.state(), { ...this.hooks(), ...overrides }); }
}

// 머리/중간 삭제와 Form/자산 이웃의 서로 다른 prev 필드를 확인한다.
test('RawSquidContainedUnpop: 혼합 링크와 부모 통지', () => {
    // Form(3)·자산(74) 이웃마다 확인한다.
    for (const nextType of [3, 74]) {
        // 부모 머리와 중간 항목을 구분한다.
        for (const hasPrevious of [false, true]) {
            const scene = new ContainedScene();
            const next = 61;
            scene.previous = hasPrevious ? 60 : 0;
            scene.put(scene.source, 4, next);
            scene.raw(next)[10] = nextType;
            scene.put(next, 14, 0xaaaa);
            scene.put(next, 16, 0xbbbb);
            if (hasPrevious) {
                scene.raw(scene.previous)[10] = 6;
                scene.put(scene.previous, 4, scene.source);
                scene.put(scene.parent, 6, scene.previous);
            }
            // 부모 통지 전에 링크/상태 쓰기가 끝나 있어야 한다.
            scene.onNotice = () => {
                assert.equal(scene.raw(scene.source)[11], 4);
                assert.equal(scene.word(next, nextType < 70 ? 14 : 16), scene.previous);
            };
            scene.make().detach(scene.source);
            assert.equal(scene.word(scene.parent, 6), hasPrevious ? scene.previous : next);
            if (hasPrevious) assert.equal(scene.word(scene.previous, 4), next);
            assert.equal(scene.word(next, nextType < 70 ? 16 : 14), nextType < 70 ? 0xbbbb : 0xaaaa);
            assert.equal(scene.word(scene.source, 4), next);
            assert.equal(scene.raw(scene.source)[11], 4);
            assert.deepEqual([scene.parentCalls, scene.previousCalls, scene.setPreviousCalls, scene.noticeCalls, scene.diagnostics.length, scene.commands.length], [1, 1, 1, 1, 0, 0]);
        }
    }
});

// free 부모는 진단 뒤 계속하고 dead 부모만 링크 해제를 건너뛴다. 진단이 바꾼 현재 dead 가 결정한다.
test('RawSquidContainedUnpop: dead/free 부모와 진단 복귀', () => {
    // 살아 있음/free/dead/둘 다를 확인한다.
    for (const parentState of [0, 1, 2, 3]) {
        const scene = new ContainedScene();
        scene.raw(scene.parent)[11] = parentState;
        scene.onAssertion = (expression, file, line) => assert.deepEqual([expression, file, line], ['!p.isFree()', SQUID_CPP, 0x3a8]);
        scene.make().detach(scene.source);
        assert.equal(scene.parentCalls, 1);
        assert.equal(scene.diagnostics.length, parentState & 1 ? 1 : 0);
        assert.equal(scene.previousCalls, parentState & 2 ? 0 : 1);
        assert.equal(scene.noticeCalls, parentState & 2 ? 0 : 1);
        assert.equal(scene.word(scene.parent, 6), parentState & 2 ? scene.source : 0);
        assert.equal(scene.raw(scene.source)[11], parentState & 2 ? 12 : 4);
    }
    const changed = new ContainedScene();
    changed.raw(changed.parent)[11] = 1;
    changed.onAssertion = () => { changed.raw(changed.parent)[11] = 2; };
    changed.make().detach(changed.source);
    assert.deepEqual([changed.previousCalls, changed.noticeCalls, changed.raw(changed.source)[11]], [0, 0, 12]);
});

// 늦은 진단 전의 두 링크 쓰기는 남고, 부모 머리는 저장한 next, 통지는 현재 TYPE/전역을 쓴다.
test('RawSquidContainedUnpop: 저장 next 와 현재 TYPE 구별', () => {
    const scene = new ContainedScene();
    scene.previous = 60;
    scene.put(scene.source, 4, 61);
    scene.raw(61)[10] = 3;
    scene.onAssertion = (expression, file, line) => {
        assert.deepEqual([expression, file, line], ['prev == 0', SQUID_CPP, 0x3c0]);
        assert.equal(scene.word(scene.previous, 4), 61);
        assert.equal(scene.word(61, 14), 60);
        scene.put(scene.source, 4, 62);
        scene.put(scene.parent, 6, 88);
        scene.raw(scene.source)[10] = 5;
        scene.contentFormType = 5;
        scene.raw(scene.source)[11] = 0x8c;
    };
    scene.make().detach(scene.source);
    assert.deepEqual([scene.diagnostics.length, scene.word(scene.parent, 6), scene.word(scene.source, 4), scene.raw(scene.source)[11], scene.noticeCalls], [1, 61, 62, 0x84, 1]);
});

// prev 가 자기 next 를 받는 SetNext 진단 뒤에도 저장한 next 를 다시 쓴다.
test('RawSquidContainedUnpop: self-next 진단 뒤 쓰기 유지', () => {
    const scene = new ContainedScene();
    scene.previous = 60;
    scene.put(scene.source, 4, 60);
    scene.put(scene.parent, 6, 60);
    scene.raw(scene.previous)[10] = 3;
    scene.onAssertion = (expression, file, line) => {
        assert.deepEqual([expression, file, line], ['n != getSid()', 'c:\\ns\\o\\squid.h', 0x123]);
        scene.put(scene.previous, 4, 99);
    };
    scene.make().detach(scene.source);
    assert.deepEqual([scene.diagnostics.length, scene.word(scene.previous, 4), scene.word(scene.previous, 14), scene.raw(scene.source)[11], scene.noticeCalls], [1, 60, 60, 4, 1]);
});

// 선택하지 않은 장치는 요구하지 않고, 선택된 장치 누락은 앞선 효과를 남긴 채 실패한다.
test('RawSquidContainedUnpop: 선택된 장치 누락은 부분 링크를 남긴다', () => {
    const scene = new ContainedScene();
    scene.previous = 60;
    scene.put(scene.parent, 6, 60);
    scene.put(scene.source, 4, 61);
    assert.throws(() => scene.make({ setPrevious: undefined }).detach(scene.source), /setPrevious/);
    assert.deepEqual([scene.word(scene.previous, 4), scene.raw(scene.source)[11], scene.noticeCalls], [61, 12, 0]);
    // 일반 Form(3) 제거는 부모 통지를 선택하지 않는다.
    const form = new ContainedScene();
    form.raw(form.source)[10] = 3;
    form.make({ parentNotice: undefined }).detach(form.source);
    assert.equal(form.raw(form.source)[11], 4);
    // 자산의 부모 통지 누락은 완료된 해제를 취소하지 않는다.
    const asset = new ContainedScene();
    assert.throws(() => asset.make({ parentNotice: undefined }).detach(asset.source), /parentNotice/);
    assert.deepEqual([asset.raw(asset.source)[11], asset.word(asset.parent, 6)], [4, 0]);
});

// UI 검색은 상태와 무관하게 Form(≤0x45)만 보고, 전체 DWORD 로 비교하며, 처음 한 번만 큐에 넣는다.
test('RawSquidContainedUnpop: UI 검색 필터와 한 번 큐', () => {
    const scene = new ContainedScene();
    scene.put(scene.source, 6, 80);
    scene.raw(80)[10] = 74;
    scene.put(80, 4, 81);
    scene.raw(81)[10] = 69;
    scene.put(81, 4, 82);
    scene.raw(82)[10] = 3;
    scene.raw(82)[11] = 0xff;
    scene.put(82, 4, 83);
    scene.raw(83)[10] = 3;
    const unpop = scene.make();
    unpop.notifyUi(scene.source);
    assert.deepEqual(scene.commands, [[scene.uiEvent, scene.source, 0]]);
    scene.commands.length = 0;
    scene.gumpFormType = 0x10003;
    unpop.notifyUi(scene.source);
    assert.deepEqual(scene.commands, [], 'TYPE DWORD 를 BYTE 로 줄이지 않는다');
    scene.gumpFormType = 74;
    unpop.notifyUi(scene.source);
    assert.deepEqual(scene.commands, [], '자산은 Form 검색 대상이 아니다');
    scene.gumpFormType = 3;
    const missing = scene.make({ command: undefined });
    assert.throws(() => missing.notifyUi(scene.source), /command/);
    scene.put(scene.source, 6, 0);
    missing.notifyUi(scene.source);
    assert.deepEqual(scene.commands, [], '빈 목록은 명령 장치 없이 정상 반환한다');
});

// 진단 복귀 뒤 현재 머리/TYPE/사건을 읽고, checking0·메타맵·boss 단락은 진단을 생략한다.
test('RawSquidContainedUnpop: UI 진단 뒤 현재 머리와 전역', () => {
    const scene = new ContainedScene();
    scene.raw(scene.source)[11] = 14;
    scene.checking = 0x80000000;
    scene.boss = false;
    scene.onAssertion = (expression, file, line) => {
        assert.deepEqual([expression, file, line], ['inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy', '\\Ns\\O\\Squidfinder.cpp', 0x158]);
        scene.put(scene.source, 6, 80);
        scene.raw(80)[10] = 5;
        scene.gumpFormType = 5;
        scene.uiEvent = 0xf1234567;
    };
    scene.make().notifyUi(scene.source);
    assert.equal(scene.diagnostics.length, 1);
    assert.deepEqual(scene.commands, [[0xf1234567, 50, 0]]);
    // 각 단락 평가가 진단을 생략한다.
    for (const gate of [0, 1, 2]) {
        const quiet = new ContainedScene();
        quiet.raw(quiet.source)[11] = 14;
        quiet.checking = gate ? 1 : 0;
        quiet.metamap = gate === 1;
        quiet.boss = gate === 2;
        quiet.make({ assertion: undefined }).notifyUi(quiet.source);
        assert.equal(quiet.diagnostics.length, 0);
    }
});

// 분리가 정상 반환하면(부모 dead 포함) 부모 통지로 새로 연결된 GumpForm 까지 UI 검색한다.
test('RawSquidContainedUnpop: Unpop 은 부모 효과 뒤 UI 를 통지한다', () => {
    // 부모 dead 여부를 각각 확인한다.
    for (const deadParent of [false, true]) {
        const scene = new ContainedScene();
        scene.raw(scene.parent)[11] = deadParent ? 2 : 0;
        if (deadParent) {
            scene.put(scene.source, 6, 80);
            scene.raw(80)[10] = 3;
        } else {
            scene.onNotice = () => { scene.put(scene.source, 6, 80); scene.raw(80)[10] = 3; scene.uiEvent = 0x02000061; };
        }
        scene.make().unpop(scene.source);
        assert.deepEqual(scene.commands, [[deadParent ? 0x02000039 : 0x02000061, 50, 0]]);
        assert.equal(scene.noticeCalls, deadParent ? 0 : 1);
        assert.equal(scene.raw(scene.source)[11], deadParent ? 12 : 4);
    }
});

// 두 contained 진단 사이의 현재 BYTE 를 다시 읽고, Form 전용 void 선행을 추가하지 않는다.
test('RawSquidContainedUnpop: 접두 진단 재확인과 void 미설정', () => {
    const scene = new ContainedScene();
    scene.raw(scene.source)[11] = 0;
    scene.onAssertion = (expression, file, line) => {
        assert.equal(expression, 'containedFlag');
        assert.equal(file, SQUID_CPP);
        assert.equal(line, scene.diagnostics.length === 1 ? 0x3a3 : 0x3b1);
        if (scene.diagnostics.length === 2) scene.raw(scene.source)[11] = 0x88;
    };
    scene.make().detach(scene.source);
    assert.deepEqual([scene.diagnostics.length, scene.raw(scene.source)[11], scene.noticeCalls, scene.word(scene.parent, 6)], [2, 0x80, 1, 0]);
});

// 실제 생성 VT 가 링크 배치를 정하며 선택한 WORD 밖 바이트를 바꾸지 않는다. 모르는 VT 는 거부한다.
test('commonContained*: 실제 VT 의 링크 WORD 배치', () => {
    const scene = new ContainedScene();
    const tables = [0x512110, 0x512098, 0x512188, 0x512208, 0x501dd0];
    // 네 Form 과 대표 자산 VT 를 확인한다.
    tables.forEach((table, index) => {
        const form = index < 4;
        const raw = scene.raw(scene.source);
        put(raw, 0, table);
        scene.put(scene.source, 14, 0xabcd);
        scene.put(scene.source, 16, 0xbcde);
        scene.put(scene.source, 18, 0xcdef);
        raw[10] = form ? 74 : 3;
        raw[11] = 0xff;
        assert.equal(commonContainedParent(scene.pool, scene.source), form ? 0xbcde : 0xabcd);
        assert.equal(commonContainedPrevious(scene.pool, scene.source), form ? 0xabcd : 0xbcde);
        const before = raw.slice();
        commonContainedSetPrevious(scene.pool, scene.source, 0xffff);
        const at = form ? 14 : 16;
        assert.equal(scene.word(scene.source, at), 0xffff);
        // 선택한 WORD 밖 바이트는 그대로이다.
        for (let byte = 0; byte < raw.length; byte += 1) if (byte !== at && byte !== at + 1) assert.equal(raw[byte], before[byte]);
    });
    put(scene.raw(scene.source), 0, 0);
    assert.throws(() => commonContainedParent(scene.pool, scene.source), /가상 표/);
    assert.throws(() => commonContainedPrevious(scene.pool, scene.source), /가상 표/);
    assert.throws(() => commonContainedSetPrevious(scene.pool, scene.source, 1), /가상 표/);
});
