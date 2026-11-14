// 다중 선택 계열의 원본 10.78 독립 관측 대조: 토글(004da6b0)·다중 이동(004d5500)·기본 선택 비트·실제 목록 helper(ClientUserSelectionGlobals 의 추가/제거)를 601행 fixture 로 대조하고,
// cpppj `MultipleSelectionTests`·`MouseSelectionRecheckTests`·`UserSelectionTests` 의 전역 시험(DWORD 비트/배열 재할당/후반 실패)을 같은 입력 구성으로 옮긴다.
// 입력 열만 공급하고 관측 열(경계 사건 순서·현재 전역/표시 비트/30칸 배열)은 비교에만 쓴다. 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E495)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawUserHighlight } from '../www/core/raw-user-highlight.js';
import { RawUserSelection } from '../www/core/raw-user-selection.js';
import { ClientUserSelectionGlobals, MULTIPLE_CAPACITY } from '../www/core/client-user-selection-globals.js';

// 탭 구분 fixture 의 머리글/주석 줄을 건너뛴 행 목록이다(빈 열 보존).
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#')).map((line) => line.split('\t')); }
// 부호 없는 십진수 목록과 float32 비트 보기이다.
const numbers = (text) => text.split(',').map(Number);
const f32 = new Float32Array(1); const u32 = new Uint32Array(f32.buffer);
const floatBits = (value) => { f32[0] = value; return u32[0]; };
// 비정렬 little endian 쓰기이다(낮은 바이트부터 폭만큼, 주변 바이트 보존).
function put(raw, offset, value, width = 4) { for (let index = 0; index < width; index += 1) raw[offset + index] = Math.floor((value >>> 0) / 2 ** (8 * index)) & 0xff; }
// 10.78 풀 용량과 TYPE 수이다.
const CAPACITY = 24000; const TYPES = 188;
// 188 개의 빈 TYPE 레코드를 만든다.
const makeTypes = () => Array.from({ length: TYPES }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));

// 같은 Client 선택 전역/30칸 배열과 물리 풀을 각 입력에서 재사용하는 장면이다(선택 대상은 독립 raw 입력이다).
class MultipleScene {
    // 실제 50 BYTE 슬롯과 188 TYPE 경계이다.
    pool = new SidPool(CAPACITY, { server: true }); types = makeTypes();
    // 현재 입력/전역과 실제 경계 호출 순서, 최초 장치 효과만 바꾸는 명시 입력의 카운터이다.
    owner = new ClientUserSelectionGlobals(); row = []; trace = ''; moves = 0; refreshes = 0;
    // 독립 물리 입력만 쓴다.
    raw(sid) { return this.pool.physical(sid); }
    // 실제 원문 trace 형식으로 현재 인자를 기록한다.
    event(name, values = []) { if (this.trace.length > 0) this.trace += ';'; this.trace += name; for (const value of values) this.trace += `:${value >>> 0}`; }
    // 이전 관측과 무관하게 같은 13DWORD 입력을 구성한다.
    prepare(input) {
        this.row = input; assert.equal(input.length, 13); this.trace = ''; this.moves = 0; this.refreshes = 0; const owner = this.owner;
        owner.resetForGeneralWorld(); owner.selected = input[5]; owner.mode = 5; owner.enabled = input[7]; owner.multipleCount = input[6] | 0; owner.multiple[0] = 52; owner.multiple[1] = 53; owner.multiple[2] = 52;
        if (input[9] === 2) { owner.multiple[0] = 0; owner.multiple[1] = 24000; }
        // 다섯 독립 선택 대상만 초기화해 표시/이동 장치를 가장하지 않는다.
        for (let sid = 50; sid < 55; sid += 1) {
            const raw = this.raw(sid); raw.fill(0); raw[10] = 100; raw[34] = 1; raw[40] = (sid === 50 ? input[4] : 4) & 0xff; put(raw, 14, 0x41280000); put(raw, 18, 0xc0500000);
            if (sid === 50) raw[11] = input[3] & 0xff;
            if (input[9] === 1 && (sid === 52 || sid === 53)) raw[11] = sid === 52 ? 1 : 2;
        }
        this.types[100].flags2 = 0x10000;
    }
    // 원문 oracle 의 명시 경계만 공급하고 토글/목록/이동 부모는 제품에서 실행한다.
    hooks() {
        const owner = this.owner; const row = () => this.row;
        return {
            paused: () => 0, enabled: () => owner.enabled,
            blocked: () => { this.event('B'); if (row()[10] === 8) { owner.multipleCount = 1; owner.multiple[0] = 53; } return row()[8]; },
            selectNotice: (sid, on) => {
                this.event('N', [sid, on]); if (row()[10] === 1) owner.selected = 51; if (row()[10] === 2) owner.multipleCount = 1;
                const raw = this.raw(sid); raw[40] = ((raw[40] & ~4) | ((on & 1) << 2)) & 0xff; // 기본 41c0b0 의 명시 가상 경계이며 단독 기존 Mark 검증과 구별한다.
            },
            refresh: (sid) => { this.event('R', [sid]); this.refreshes += 1; if (row()[10] === 3 && this.refreshes === 1) { owner.multiple[1] = 54; owner.multipleCount = 3; } },
            appendMultiple: (sid) => { this.event('A', [sid]); owner.appendMultiple(sid); }, removeMultiple: (sid) => { this.event('D', [sid]); owner.removeMultiple(sid); },
            writeEnabled: (value) => { owner.enabled = value >>> 0; },
            sound: (x, y, name, flags) => { assert.equal(name, 'select.wav'); this.event('S', [x, y, flags]); if (row()[10] === 4) owner.selected = 52; },
            highlight: (mode) => { this.event('H', [mode]); owner.mode = mode >>> 0; },
            localOwner: () => 1, editor: () => 0, dais: () => 165, dump: () => {}, rangeType: () => 0, content: () => 0, moving: () => 0, movementPreview: () => {},
        };
    }
    // 실제 이동 전송 대신 명시 관측/콜백 뒤 현재값 변경만 수행한다.
    move(sid, x, y) { this.event('M', [sid, x, y]); this.moves += 1; if (this.moves === 1) { if (this.row[10] === 5) this.owner.multipleCount = 1; if (this.row[10] === 6) this.owner.multiple[1] = 54; if (this.row[10] === 7) this.owner.enabled = 7; } }
    // 선택/강조/enabled/개수·다섯 선택 비트·전체 30칸을 실제 물리에서 읽는다.
    current() {
        const owner = this.owner; const result = [owner.selected, owner.mode, owner.enabled, owner.multipleCount >>> 0];
        for (let sid = 50; sid < 55; sid += 1) result.push(this.raw(sid)[40]);
        return [...result, ...owner.multiple].map((value) => value >>> 0);
    }
    // 현재 owner 위에 선택 처리기를 만든다.
    build() { return new RawUserSelection(this.pool, this.types, this.owner.selectionState(), this.hooks()); }
}

test('RawUserSelection 다중 선택 원본 10.78 601행: 토글·다중 이동·기본 선택 비트·실제 목록 helper 의 경계 순서와 전역/표시 비트/30칸이 일치한다', () => {
    const table = rows('multiple-selection-x86.tsv'); assert.equal(table.length, 601); const scene = new MultipleScene(); const failures = [];
    // 제품 계산으로 fixture 기대값을 생성하지 않고 독립 원문 도구의 601개 입력을 모두 방문한다.
    for (const [index, fields] of table.entries()) {
        assert.equal(fields.length, 3, `행 ${index}`); scene.prepare(numbers(fields[0])); const selection = scene.build(); const problems = [];
        if (!scene.row[0]) selection.toggleMultiple(scene.row[1], scene.row[2]); else selection.moveMultiple(scene.row[11], scene.row[12], (sid, x, y) => scene.move(sid, x, y));
        if ((scene.trace || '-') !== fields[1]) problems.push('trace'); if (scene.current().join(',') !== fields[2]) problems.push('current');
        if (problems.length) failures.push(`${index}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

test('RawUserSelection 독립 시험: 단일 선택→Shift 추가/제외→주선택 승격과 고정 용량·중복·안정된 제거', () => {
    const scene = new MultipleScene(); scene.prepare([0, 50, 0xffffffff, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0]); scene.raw(51)[40] = 0; const selection = scene.build(); const owner = scene.owner;
    selection.extendSelection(50, 0); assert.ok(owner.selected === 50 && owner.mode === 5 && (scene.raw(50)[40] & 4));
    selection.extendSelection(51, 0); assert.ok(owner.multipleCount === 1 && owner.multiple[0] === 51 && (scene.raw(51)[40] & 4));
    selection.extendSelection(51, 0); assert.ok(owner.multipleCount === 0 && !(scene.raw(51)[40] & 4));
    selection.extendSelection(51, 0); selection.toggleMultiple(50, 0xffffffff);
    assert.ok(owner.selected === 51 && !owner.multipleCount && !(scene.raw(50)[40] & 4) && (scene.raw(51)[40] & 4));
    owner.resetForGeneralWorld();
    // 원문 용량 30 밖의 추가는 확대하지 않는다.
    for (let sid = 1; sid <= 31; sid += 1) owner.appendMultiple(sid);
    assert.ok(owner.multipleCount === 30 && owner.multiple.length === 30 && owner.multiple[29] === 30);
    owner.appendMultiple(15); assert.equal(owner.multipleCount, 30);
    owner.removeMultiple(15); assert.ok(owner.multipleCount === 29 && owner.multiple[14] === 16 && owner.multiple[28] === 30 && owner.multiple[29] === 30);
    assert.equal(MULTIPLE_CAPACITY, 30);
});

test('RawUserSelection 독립 시험: 다중 이동 중 단일 선택 해제가 차단되고 실패 시 enabled 0/기존 목록을 보존한다', () => {
    const scene = new MultipleScene(); scene.prepare([1, 50, 0, 0, 4, 50, 1, 1, 0, 0, 0, 0x41280000, 0xc0500000]); const selection = scene.build(); const owner = scene.owner;
    assert.throws(() => selection.moveMultiple(scene.row[11], scene.row[12], (sid) => {
        assert.ok(sid === 52 && !owner.enabled); assert.ok(selection.select(0, 0, 1) === 0 && owner.selected === 50 && owner.multipleCount === 1);
        throw new Error('이동 전송 실패 명시 입력');
    }), /이동 전송 실패/);
    assert.ok(!owner.enabled && owner.selected === 50 && owner.multipleCount === 1 && (scene.raw(52)[40] & 4));
    selection.moveMultiple(scene.row[11], scene.row[12], (sid) => { assert.ok(sid === 52 && !owner.enabled); });
    assert.ok(owner.enabled === 1 && !owner.multipleCount && owner.selected === 50 && !(scene.raw(52)[40] & 4));
});

// 마우스 TYPE 재확인(004da650)의 실제 선택 부모를 쓰는 장면이다. 큰 외부 효과만 명시 관측한다.
class RecheckScene {
    // 통신을 사용하지 않는 실제 풀과 검색 결과/제단 재지정 결과를 구별하는 두 슬롯이다.
    pool = new SidPool(CAPACITY, { server: false }); first = this.pool.allocate(2); second = this.pool.allocate(2);
    // 실제 선택의 현재 TYPE 배열과 현재 선택 전역·관측 카운터이다. 선택 상태 객체 자체가 이 장면이다.
    types = makeTypes(); selected = 0; mouseType = 74; paused = 0; finds = 0; dumps = 0; notices = 0; sounds = 0;
    // 정수 보정 없이 검색할 저장 float 비트와 선택과 공유하는 빈 다중 목록이다.
    rememberedPoint = [floatBits(12.5), floatBits(13.25)]; multiple = []; multipleCount = 0;
    constructor() {
        // 두 객체는 같은 기억 좌표이며 필드 폭을 그대로 쓴다.
        for (const sid of [this.first, this.second]) { const raw = this.raw(sid); raw.fill(0); raw[10] = 74; put(raw, 14, this.rememberedPoint[0]); put(raw, 18, this.rememberedPoint[1]); }
        this.selection = new RawUserSelection(this.pool, this.types, this, {
            paused: () => this.paused, enabled: () => 1, blocked: () => 0, dais: () => 165, manaProcess: () => 50, rangeProcess: () => 51, rangeType: () => 0,
            dump: () => { this.dumps += 1; }, selectNotice: () => { this.notices += 1; }, refresh: () => {}, processNotice: () => {}, sound: () => { this.sounds += 1; },
            altar: (sid) => { assert.equal(sid, this.first); return this.second; },
        });
    }
    // 유효 명시 슬롯만 수정한다.
    raw(sid) { return this.pool.allocated(sid); }
    // 검색 계산 자체는 실제 Session 통합 검사에서 검증한다.
    find(x, y, type) { this.finds += 1; assert.ok(x === this.rememberedPoint[0] && y === this.rememberedPoint[1] && type === this.mouseType); return this.first; }
    // 실제 선택과 같은 현재 저장소를 사용한다.
    run() { this.selection.recheckMouse((x, y, type) => this.find(x, y, type)); }
}

test('RawUserSelection 마우스 재확인: 마우스 TYPE 0 은 검색기를 조회하지 않고 하위가 바꾼 현재 TYPE 을 복구하지 않는다', () => {
    const zero = new RecheckScene(); zero.mouseType = 0; zero.selection.recheckMouse(undefined); assert.ok(zero.finds === 0 && zero.dumps === 0 && zero.mouseType === 0);
    const missing = new RecheckScene(); missing.selection.recheckMouse((x, y, type) => { assert.equal(missing.find(x, y, type), missing.first); missing.mouseType = 75; return 0; });
    assert.ok(missing.finds === 1 && missing.dumps === 0 && missing.mouseType === 75);
});

test('RawUserSelection 마우스 재확인: 검색 중 바뀐 현재 선택이 found 와 다르면 Select 를 생략한다', () => {
    const scene = new RecheckScene(); scene.selection.recheckMouse((x, y, type) => { const found = scene.find(x, y, type); scene.selected = scene.second; return found; });
    assert.ok(scene.finds === 1 && scene.dumps === 0 && scene.selected === scene.second && scene.mouseType === 74);
});

test('RawUserSelection 마우스 재확인: 정지 거부(0 반환)도 원래 found 의 상태 검사/꼬리를 생략하지 않고 실제 Select 를 거친 뒤 마우스 TYPE 만 지운다', () => {
    const paused = new RecheckScene(); paused.paused = 1; paused.run(); assert.ok(paused.finds === 1 && paused.dumps === 0 && paused.selected === 0 && paused.mouseType === 0);
    const ordinary = new RecheckScene(); ordinary.run(); assert.ok(ordinary.finds === 1 && ordinary.dumps === 1 && ordinary.notices === 1 && ordinary.sounds === 1 && ordinary.selected === ordinary.first && ordinary.mouseType === 0);
});

test('RawUserSelection 마우스 재확인: 다이스가 제단으로 선택을 재지정해도 검색 결과의 현재 extra 를 읽는다', () => {
    // 원래 found 와 반환 대상의 상태를 반대로 둔다.
    for (const extra of [0, 1]) {
        const scene = new RecheckScene(); scene.raw(scene.first)[10] = 165; scene.raw(scene.first)[40] = extra; scene.raw(scene.second)[40] = extra ^ 1; scene.run();
        assert.ok(scene.selected === scene.second && scene.mouseType === (extra ? 74 : 0) && scene.dumps === 2 && scene.sounds === 1);
    }
});

test('ClientUserSelectionGlobals 독립 시험: 아홉 DWORD 비트와 원시 마우스 좌표를 좁히거나 섞지 않고 쓴다', () => {
    const owner = new ClientUserSelectionGlobals(); const fields = ['Enabled', 'Mode', 'Selected', 'MouseType', 'MouseX', 'MouseY', 'RememberedX', 'RememberedY', 'MultipleCount'];
    const input = [0x80000000, 0xffffffff, 0xfedcba98, 0x12345678, 0xfffffffe, 0x80000000, 0x7fc00001, 0xffffffff, 0xfffffff9]; // 전체 상위 비트·NaN 비트·음수 개수의 명시 입력이다.
    fields.forEach((field, index) => owner.writeDword(field, input[index]));
    assert.deepEqual(fields.map((field) => owner.readDword(field)), input); assert.ok(owner.mouse.x === -2 && owner.multipleCount === -7);
    const selection = owner.selectionState(); const highlight = owner.highlightState();
    owner.selected = 0x87654321; assert.ok(selection.selected === 0x87654321 && highlight.selectedSid === 0x87654321); // DWORD 를 저장할 때 SID 소비용 WORD 로 잘리지 않는다.
    highlight.mouseX = -33; assert.equal(owner.mouse.x, -33); highlight.mode = 4; assert.equal(owner.mode, 4);
    assert.equal(owner.readDword('PriestSelections'), 0); owner.writeDword('PriestSelections', 0xffffffff); assert.equal(owner.readDword('PriestSelections'), 0xffffffff);
    owner.resetForGeneralWorld(); assert.equal(owner.readDword('PriestSelections'), 0xffffffff); assert.ok(owner.enabled === 1 && owner.mode === 0 && owner.selected === 0 && owner.multipleCount === 0); // 누적 DWORD 는 일반 reset 에서도 유지한다.
    assert.throws(() => owner.readDword('Unknown'), RangeError); assert.throws(() => owner.writeDword('Unknown', 1), RangeError);
});

test('ClientUserSelectionGlobals 독립 시험: 가상 통지 중 재할당된 실제 DWORD 배열과 signed 개수를 다음 반복에서 모두 다시 읽는다', () => {
    const scene = new MultipleScene(); const owner = scene.owner; owner.selected = 0xfedcba98; owner.multiple[0] = 55; owner.multiple[1] = 56; owner.multipleCount = 2;
    for (const number of [55, 56, 57]) { const raw = scene.raw(number); raw.fill(0); raw[10] = 100; }
    const highlight = new RawUserHighlight(owner.highlightState(), {}); const notices = []; const refreshes = []; const first = owner.multiple;
    new RawUserSelection(scene.pool, scene.types, owner.selectionState(), {
        enabled: () => owner.enabled, highlight: (mode) => highlight.highlight(mode),
        selectNotice: (sid, on) => {
            assert.ok(on === 0 && owner.selected === 0xfedcba98); notices.push(sid);
            // 이전 배열을 반납하고 다음 항목을 바꾼다.
            if (sid === 55) { const next = new Array(4096).fill(0); next[0] = 55; next[1] = 57; owner.multiple = next; owner.multipleCount = 3; }
        },
        refresh: (sid) => { refreshes.push(sid); if (sid === 55) owner.enabled = 0; else owner.multipleCount = 2; }, // enabled 는 최초 한 번만, 개수는 반복마다 읽는다.
    }).clear();
    assert.ok(owner.multiple !== first); assert.deepEqual(notices, [55, 57]); assert.deepEqual(refreshes, notices); assert.ok(owner.multipleCount === 0 && owner.selected === 0 && owner.enabled === 0);
});

test('ClientUserSelectionGlobals 독립 시험: 풀 밖 DWORD 후보는 하위 WORD 로 잘라 통지하지 않고 후반 배열 실패는 선행 통지를 보존한다', () => {
    const scene = new MultipleScene(); const owner = new ClientUserSelectionGlobals(); owner.selected = 0xffffffff; owner.multiple = [0x10037, 0xffffffff]; owner.multipleCount = 2; const input = owner.multiple.slice();
    const highlight = new RawUserHighlight(owner.highlightState(), {});
    new RawUserSelection(scene.pool, scene.types, owner.selectionState(), { enabled: () => owner.enabled, highlight: (mode) => highlight.highlight(mode), selectNotice: () => { throw new Error('범위 밖 DWORD 의 잘린 SID 통지'); } }).clear();
    assert.deepEqual(owner.multiple, input); assert.ok(owner.multipleCount === 0 && owner.selected === 0);
    // 후반 배열 경계 실패는 앞선 통지를 보존하고 아직 실행하지 않은 개수 0/selected 0 꼬리를 만들지 않는다.
    const late = new ClientUserSelectionGlobals(); late.selected = 0x87654321; late.multiple = [55]; late.multipleCount = 2; scene.raw(55).fill(0); let notices = 0; let refreshes = 0;
    assert.throws(() => new RawUserSelection(scene.pool, scene.types, late.selectionState(), { enabled: () => late.enabled, selectNotice: () => { notices += 1; }, refresh: () => { refreshes += 1; } }).clear(), RangeError);
    assert.ok(notices === 1 && refreshes === 1 && late.multipleCount === 2 && late.selected === 0x87654321);
    // 명시 오류 경계: 가득 찬 추가는 개수 검사 전에 반환하고 음수 개수의 추가/제거 경계도 원문과 같다.
    const negative = new ClientUserSelectionGlobals(); negative.multipleCount = -1; assert.throws(() => negative.appendMultiple(5), RangeError); negative.removeMultiple(5); assert.equal(negative.multipleCount, -1);
    negative.multipleCount = 31; assert.throws(() => negative.removeMultiple(5), RangeError); negative.appendMultiple(5); assert.equal(negative.multipleCount, 31);
});

test('ClientUserSelectionGlobals 독립 시험: 초기값·일반 reset·중복 검색 범위·상태 보기 쓰기 폭', () => {
    // PE 의 enabled 1 과 나머지 BSS 0 이다.
    const fresh = new ClientUserSelectionGlobals();
    assert.deepEqual([fresh.enabled, fresh.mode, fresh.selected, fresh.mouseType, fresh.mouse.x, fresh.mouse.y, fresh.remembered[0], fresh.remembered[1], fresh.multipleCount, fresh.priestSelections], [1, 0, 0, 0, 0, 0, 0, 0, 0, 0]);
    assert.ok(fresh.multiple.length === 30 && fresh.multiple.every((value) => value === 0));
    // 일반 reset 은 아홉 DWORD 와 목록을 같은 주소에서 되돌리고 P/R 누적은 보존하며, 줄거나 늘어난 배열도 30칸으로 복구한다.
    const owner = new ClientUserSelectionGlobals(); const box = owner.multiple;
    owner.enabled = 0; owner.mode = 5; owner.selected = 9; owner.mouseType = 8; owner.mouse.x = -4; owner.mouse.y = 6; owner.remembered[0] = 3; owner.remembered[1] = 7; owner.multipleCount = 4; owner.priestSelections = 11; owner.multiple[29] = 77; owner.multiple.length = 31; owner.multiple[30] = 78;
    owner.resetForGeneralWorld();
    assert.deepEqual([owner.enabled, owner.mode, owner.selected, owner.mouseType, owner.mouse.x, owner.mouse.y, owner.remembered[0], owner.remembered[1], owner.multipleCount, owner.priestSelections], [1, 0, 0, 0, 0, 0, 0, 0, 0, 11]);
    assert.ok(owner.multiple === box && owner.multiple.length === 30 && owner.multiple.every((value) => value === 0));
    // 중복 검색은 현재 개수의 앞부분 전체(0 번째와 중간 항목 포함)를 본다.
    owner.appendMultiple(5); owner.appendMultiple(6); owner.appendMultiple(7); owner.appendMultiple(6); owner.appendMultiple(5); assert.deepEqual(owner.multiple.slice(0, 4), [5, 6, 7, 0]); assert.equal(owner.multipleCount, 3);
    // 제거는 앞쪽 항목을 지우고 뒤 항목을 안정적으로 당긴다.
    owner.removeMultiple(5); assert.deepEqual(owner.multiple.slice(0, 3), [6, 7, 7]); assert.equal(owner.multipleCount, 2);
    // 상태 보기의 쓰기는 전체 DWORD 폭을 유지하고(상위 비트 보존) 좌표/개수는 signed 비트로 저장한다.
    const selection = owner.selectionState(); const highlight = owner.highlightState();
    selection.mouseType = 0xfffffff0; assert.equal(owner.mouseType, 0xfffffff0); highlight.mouseType = 0xfffffff2; assert.equal(owner.mouseType, 0xfffffff2);
    highlight.selectedSid = 0xfffffff4; assert.equal(owner.selected, 0xfffffff4); selection.selected = 0xfffffff6; assert.equal(owner.selected, 0xfffffff6);
    highlight.mouseX = -8; highlight.mouseY = -10; assert.deepEqual([owner.mouse.x, owner.mouse.y, highlight.mouseX, highlight.mouseY], [-8, -10, -8, -10]);
    selection.multipleCount = 0xfffffff8; assert.equal(owner.multipleCount, -8); owner.writeDword('MultipleCount', 0xfffffff8); assert.equal(owner.multipleCount, -8);
    highlight.mode = 0xfffffff0; assert.equal(owner.mode, 0xfffffff0); assert.strictEqual(selection.rememberedPoint, owner.remembered); assert.strictEqual(selection.multiple, owner.multiple); assert.strictEqual(selection.currentMultiple(), owner.multiple);
});
