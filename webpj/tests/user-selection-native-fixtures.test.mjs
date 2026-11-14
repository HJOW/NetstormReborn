// 사용자 선택 계열의 원본 10.78 독립 관측 대조: 실제 세 인자 Select·Clear·ESC(Select 0)·다중 정리·기억·Carrier/Archer 메뉴 입구(RawUserSelection, 1,074 + 87 + 154행)와
// 사용자 후보 선택(RawUserPick, 1,092행)을 cpppj `UserSelectionTests`·`UserPickTests` 와 같은 입력 구성으로 비교한다. 입력 열만 공급하고 관측 열(반환·경계 사건 순서·현재 전역)은 비교에만 쓴다.
// 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E495)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { RawUserHighlight } from '../www/core/raw-user-highlight.js';
import { RawUserSelection } from '../www/core/raw-user-selection.js';
import { RawUserPick } from '../www/core/raw-user-pick.js';

// 탭 구분 fixture 의 머리글/주석 줄을 건너뛴 행 목록이다(빈 열 보존).
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#')).map((line) => line.split('\t')); }
// 부호 없는 십진수 목록과 float32 비트 보기이다(음수였던 입력도 상위 비트를 잃지 않는다).
const numbers = (text) => text.split(',').map(Number);
const f32 = new Float32Array(1); const u32 = new Uint32Array(f32.buffer);
const floatBits = (value) => { f32[0] = value; return u32[0]; };
const floatOf = (bits) => { u32[0] = bits >>> 0; return f32[0]; };
// 풀 용량과 TYPE 수(10.78)이다.
const CAPACITY = 24000; const TYPES = 188;
// 비정렬 little endian 쓰기이다(낮은 바이트부터 폭만큼, 주변 바이트 보존).
function put(raw, offset, value, width = 4) { for (let index = 0; index < width; index += 1) raw[offset + index] = Math.floor((value >>> 0) / 2 ** (8 * index)) & 0xff; }

// 같은 물리 풀과 실제 강조/선택 모듈이 공유하는 독립 입력 저장소이다.
class SelectionScene {
    // 원본 주소 경계와 같은 실제 물리 풀과 callback 이 바꾸는 TYPE 표이다.
    pool = new SidPool(CAPACITY, { server: true });
    types = Array.from({ length: TYPES }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    // 서로 다른 원본 전역 다섯 개(선택·마우스 TYPE·강조 모드·활성·차단)의 현재 값과 기억 좌표 두 float 비트, 다중 선택 배열, signed 개수, 강조가 조회할 마우스 좌표이다.
    selected = 0; mouseType = 0; mode = 0; enabled = 0; blocked = 0; rememberedPoint = [0, 0]; multiple = [0, 0, 0, 0, 0]; multipleCount = 0; mouseX = 320; mouseY = 240;
    // 원본 31DWORD 입력(기대값 열과 분리)과 실제 외부 경계 호출 순서·인자 기록이다.
    row = []; trace = '';

    // 예약 1/마지막 23999 까지 독립 raw 입력을 주입한다(현재 물리 슬롯의 살아 있는 보기).
    raw(sid) { return this.pool.physical(sid); }
    // 물리 필드의 낮은 바이트부터 원본 폭만 쓴다.
    write(sid, offset, value, width = 4) { put(this.raw(sid), offset, value, width); }
    // 전체 DWORD 와 문자열을 원본 TSV 의 경계 사건 표현으로 보존한다.
    event(letter, values = []) {
        if (this.trace.length > 0) this.trace += ';';
        this.trace += letter;
        // 각 인자의 부호 비트까지 unsigned 십진수로 출력한다.
        for (const value of values) this.trace += `:${value >>> 0}`;
    }
    // 원본 입력에 지정된 외부 효과만 변경하고 부모 선택/반환 분기는 제품에 맡긴다.
    mutate(boundary) {
        if (this.row[27] !== boundary) return;
        switch (boundary) {
        case 1: this.multipleCount = 1; this.mouseType = 77; break;
        case 2: this.selected = 99; break;
        case 3: this.write(50, 10, 101, 1); break;
        case 4: this.write(50, 10, 102, 1); break;
        case 5: this.write(50, 40, 1, 1); break;
        case 6: this.selected = 0; break;
        case 7: this.rememberedPoint = [this.row[23], this.row[24]]; break;
        case 8: this.selected = 202; this.mouseType = 77; break;
        case 9: this.enabled = 0; this.blocked = 1; break;
        case 10: this.selected = 99; this.mouseType = 77; break;
        case 11: this.write(60, 4, 65, 2); break;
        case 12: this.multipleCount = 5; break;
        default: break;
        }
    }
    // 실제 강조 모드 처리기도 선택과 같은 현재 저장소를 읽도록 연결하는 상태 보기이다.
    highlightState() {
        const scene = this;
        return {
            get mode() { return scene.mode; }, set mode(value) { scene.mode = value >>> 0; },
            get selectedSid() { return scene.selected; }, set selectedSid(value) { scene.selected = value >>> 0; },
            get mouseType() { return scene.mouseType; }, set mouseType(value) { scene.mouseType = value >>> 0; },
            get mouseX() { return scene.mouseX; }, set mouseX(value) { scene.mouseX = value | 0; },
            get mouseY() { return scene.mouseY; }, set mouseY(value) { scene.mouseY = value | 0; },
        };
    }
    // 커서/역투영/이동만 원본 관찰의 명시 경계이다.
    highlightHooks() {
        return {
            selectCursor: (value) => { this.event('K', [value]); },
            project: (x, y) => { this.event('J', [x, y]); this.mutate(8); return [floatBits(7.5), floatBits(9.25)]; },
            move: (sid, x, y, force) => { this.event('H', [sid, x, y, force]); },
        };
    }
    // 도달한 실제 경계의 인자/반환만 공급한다. 선택/다중 정리/기억 비교/강조를 대신하지 않는다.
    hooks(highlight) {
        return {
            paused: () => { this.event('P'); return this.row[5]; },
            enabled: () => this.enabled,
            blocked: () => { this.event('B'); return this.blocked; },
            localOwner: () => this.row[14], editor: () => this.row[13], dais: () => 165, manaProcess: () => 50, rangeProcess: () => 51,
            selectNotice: (sid, on) => { this.event('N', [sid, on, this.selected]); this.mutate(sid === 55 ? 1 : sid === 56 ? 12 : sid === 51 ? 2 : on ? 6 : 0); },
            refresh: (sid) => { this.event('R', [sid, this.selected]); this.mutate(sid !== 51 ? 7 : 0); },
            processNotice: (sid, event, info, value) => { this.event('F', [sid]); this.event('Q', [event, info, value]); this.mutate(11); },
            dump: (sid) => { this.event('D', [sid]); this.mutate(3); },
            showMana: (sid, flag) => { this.event('T', [sid, flag]); },
            rangeType: (type) => { this.event('V', [type]); this.mutate(4); return this.row[15]; },
            showRange: (sid, flag) => { this.event('C', [sid, flag]); this.mutate(5); },
            content: (sid) => { this.event('I', [sid, 1, 0, 0]); return this.row[16] & 0xffff; },
            altar: (sid) => { this.event('A', [sid]); this.mutate(9); return this.row[18] & 0xffff; },
            moving: (sid) => { this.event('M', [sid]); return this.row[17]; },
            movementPreview: (sid) => { this.event('E', [sid]); },
            highlight: (value) => { highlight.highlight(value); },
            assertion: () => { this.event('!'); this.trace += ':selectedSid:3841'; },
            sound: (x, y, name, flag) => { this.event('S', [x, y]); this.trace += `:${name}:${flag}`; this.mutate(10); },
        };
    }
    // 새 행은 이전 원본/제품 실행의 변경을 모두 버리고 같은 독립 입력만 설치한다.
    prepare(values) {
        this.row = values; assert.equal(values.length, 31);
        this.trace = ''; this.selected = values[4]; this.mouseType = values[20]; this.mode = values[19]; this.enabled = values[6]; this.blocked = values[7];
        this.rememberedPoint = [values[21], values[22]]; this.multipleCount = values[25] | 0;
        this.multiple = values[26] ? [0, 24000, 55, 0xffffffff, 56] : [55, 56, 57, 58, 59];
        // 예약/물리 경계까지 raw 입력만 설치하며 할당·생성 검증과 섞지 않는다.
        for (const sid of [1, 50, 51, 52, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 99, 202, 23999]) {
            const raw = this.raw(sid); raw.fill(0); raw[10] = 100; raw[34] = values[12] & 0xff; this.write(sid, 14, values[23]); this.write(sid, 18, values[24]);
        }
        this.write(50, 11, values[8], 1); this.write(50, 40, values[9], 1); if (values[18]) this.write(50, 10, 165, 1); this.write(51, 6, 60, 2);
        // 원본 ProcessFinder 의 자산/내장 Form 필터와 다음 링크 캡처를 실제 제품에서 검사한다.
        for (const [sid, type] of [[60, 50], [61, 51], [62, 49], [63, 100], [64, 6], [65, 5]]) { this.write(sid, 10, type, 1); this.write(sid, 4, sid < 65 ? sid + 1 : 0, 2); }
        this.write(57, 11, 1, 1); this.write(58, 11, 2, 1); this.write(59, 11, 8, 1);
        // 현재 네 TYPE 의 명시 입력은 callback 뒤 다시 조회할 수 있다.
        for (const type of [100, 101, 102, 165]) { this.types[type].flags1 = type === 101 ? 0x10000000 : values[10]; this.types[type].flags2 = type === 102 ? 0 : values[11]; }
    }
    // 원본 관찰과 같은 순서로 현재 전역과 변경될 수 있는 물리 필드를 반환한다.
    current() {
        const raw = this.pool.slot(50); const form = this.pool.slot(60);
        return [this.selected, this.mouseType, this.rememberedPoint[0], this.rememberedPoint[1], this.multipleCount >>> 0, this.mode, this.enabled, raw[10], raw[40], form[4] | (form[5] << 8)].map((value) => value >>> 0);
    }
    // 새 선택 처리기와 실제 강조 처리기를 같은 저장소 위에 연결한다.
    build() {
        const highlight = new RawUserHighlight(this.highlightState(), this.highlightHooks());
        return new RawUserSelection(this.pool, this.types, this, this.hooks(highlight));
    }
}

// 행마다 새 입력을 설치하고 한 선택 처리기를 만들어 연산을 실행한 뒤 반환/사건 순서/현재 전역을 대조하는 공용 재생기이다.
function replay(name, expected, run, checkResult = true) {
    const table = rows(name); assert.equal(table.length, expected); const scene = new SelectionScene(); const failures = [];
    // 모든 고정 행을 같은 현재 저장소에서 순서대로 재생한다.
    for (const [index, fields] of table.entries()) {
        assert.equal(fields.length, 4, `행 ${index}`);
        scene.prepare(numbers(fields[0])); const selection = scene.build(); const result = run(scene, selection); const problems = [];
        if (checkResult && (result >>> 0) !== Number(fields[1])) problems.push('result');
        if ((scene.trace || '-') !== fields[2]) problems.push('trace');
        if (scene.current().join(',') !== fields[3]) problems.push('current');
        if (problems.length) failures.push(`${index}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
}

test('RawUserSelection 원본 10.78 1,074행: 세 인자 Select·다중 정리·기억·Carrier/Archer 메뉴 입구의 반환·경계 순서·현재 전역이 일치한다', () => {
    replay('user-selection-x86.tsv', 1074, (scene, selection) => {
        const operation = scene.row[0]; let result = 0;
        if (!operation) result = selection.select(scene.row[1], scene.row[2], scene.row[3]);
        else if (operation === 1) selection.clearMultiple();
        else if (operation === 2) selection.remember(scene.row[23], scene.row[24], 100);
        else {
            const event = { code: 0x20000, x: scene.row[29] | 0, y: scene.row[30] | 0 };
            // 실제 공통 메뉴는 이 관찰의 명시 경계이다.
            const common = (sid, input) => { scene.event('O', [sid, input.code, input.x, input.y]); return scene.row[28]; };
            result = operation === 4 ? selection.archerMenu(50, event, common) : selection.carrierMenu(50, event, common);
        }
        return result;
    });
});

test('RawUserSelection Clear 원본 10.78 87행: enabled 관문·다중 정리·selected 0·강조 0 의 경계 순서와 현재 전역이 일치한다', () => {
    replay('user-selection-clear-x86.tsv', 87, (scene, selection) => { selection.clear(); return 0; });
});

test('RawUserSelection ESC(Select 0) 원본 10.78 154행: 관문·다중/단일 선택·강조·콜백 현재값이 일치한다', () => {
    // 원문 관측의 둘째 열은 비교하지 않는다(cpppj 와 같이 반환은 무시하고 사건 순서/현재 전역만 대조한다).
    replay('escape-selection-x86.tsv', 154, (scene, selection) => selection.select(0, 0, 0), false);
});

test('RawUserSelection 독립 시험: 조기 관문은 실제 효과를 요구하지 않고 미연결 하위는 선행 정리 뒤 그 위치에서 오류이다', () => {
    const scene = new SelectionScene(); const input = new Array(31).fill(0); input[1] = 50; input[6] = 1; input[21] = input[23] = floatBits(12.5); scene.prepare(input);
    assert.equal(new RawUserSelection(scene.pool, scene.types, scene, { paused: () => 1 }).select(50, 0, 0), 0);
    let notices = 0; scene.multipleCount = 1;
    // 정리의 +7c 는 실행되고 미연결 +88 에서 실패해야 한다.
    const missing = new RawUserSelection(scene.pool, scene.types, scene, { paused: () => 0, enabled: () => 1, blocked: () => 0, selectNotice: (sid, on) => { assert.equal(sid, 55); assert.equal(on, 0); notices += 1; } });
    assert.throws(() => missing.select(50, 0, 0), /하위 미연결/);
    assert.equal(notices, 1); assert.equal(scene.multipleCount, 1);
});

test('RawUserSelection 독립 시험: 10.78 풀·188 TYPE 계약과 SID/훅 범위 거부', () => {
    const scene = new SelectionScene();
    assert.throws(() => new RawUserSelection(scene.pool, scene.types.slice(0, 187), scene, {}), TypeError);
    assert.throws(() => new RawUserSelection({ edition: 'cd1072', capacity: 15000 }, scene.types, scene, {}), TypeError);
    // 비활성 clear 는 어떤 하위도 요구하지 않고 전역을 보존한다.
    scene.selected = 51; scene.multipleCount = 2;
    new RawUserSelection(scene.pool, scene.types, scene, { enabled: () => 0 }).clear(); assert.equal(scene.selected, 51); assert.equal(scene.multipleCount, 2);
    // 강조 경계를 의도적으로 비우면 선행 실제 쓰기는 보존되고 그 위치에서 실패한다.
    scene.multipleCount = 0;
    assert.throws(() => new RawUserSelection(scene.pool, scene.types, scene, { enabled: () => 1 }).clear(), /하위 미연결/);
    assert.equal(scene.selected, 0); assert.equal(scene.multipleCount, 0);
    // 범위 밖 SID 는 거부한다(0, 용량 이상).
    const raw = new RawUserSelection(scene.pool, scene.types, scene, { paused: () => 0, enabled: () => 1, blocked: () => 0, selectNotice() {}, refresh() {} });
    assert.throws(() => raw.select(24000, 0, 0), RangeError);
    // 다중 토글은 범위 밖/0 번을 조용히 건너뛴다(원문은 범위 확인 뒤 효과 없이 반환).
    raw.toggleMultiple(0, 1); raw.toggleMultiple(24000, 1);
});

test('RawUserSelection 독립 시험: 가상 통지 중 재할당된 DWORD 배열과 signed 개수를 다음 반복에서 다시 읽는다', () => {
    const scene = new SelectionScene(); const state = { selected: 0xfedcba98, mouseType: 0, rememberedPoint: [0, 0], multiple: [55, 56], multipleCount: 2 }; let enabled = 1;
    for (const number of [55, 56, 57]) { const raw = scene.raw(number); raw.fill(0); raw[10] = 100; }
    const notices = []; const refreshes = []; const first = state.multiple;
    const highlightState = { mode: 0, selectedSid: 0, mouseType: 0, mouseX: 0, mouseY: 0 };
    const highlight = new RawUserHighlight(Object.defineProperties(highlightState, { selectedSid: { get: () => state.selected, set: (value) => { state.selected = value >>> 0; }, enumerable: true } }), {});
    const hooks = {
        enabled: () => enabled, highlight: (mode) => highlight.highlight(mode),
        selectNotice: (sid, on) => {
            assert.equal(on, 0); assert.equal(state.selected, 0xfedcba98); notices.push(sid);
            // 이전 배열을 반납하고 다음 항목을 바꾼다.
            if (sid === 55) { const next = new Array(4096).fill(0); next[0] = 55; next[1] = 57; state.multiple = next; state.multipleCount = 3; }
        },
        refresh: (sid) => { refreshes.push(sid); if (sid === 55) enabled = 0; else state.multipleCount = 2; }, // enabled 는 최초 한 번만, 개수는 반복마다 읽는다.
    };
    new RawUserSelection(scene.pool, scene.types, state, hooks).clear();
    assert.notEqual(state.multiple, first); assert.deepEqual(notices, [55, 57]); assert.deepEqual(refreshes, notices); assert.equal(state.multipleCount, 0); assert.equal(state.selected, 0); assert.equal(enabled, 0);
});

test('RawUserSelection 독립 시험: 풀 밖 DWORD 후보는 자연 생략하고 하위 WORD 로 잘라 통지하지 않으며 후반 배열 실패는 선행 통지를 보존한다', () => {
    const scene = new SelectionScene(); const input = [0x10037, 0xffffffff];
    const state = { selected: 0xffffffff, mouseType: 0, rememberedPoint: [0, 0], multiple: input.slice(), multipleCount: 2 };
    const highlight = new RawUserHighlight({ get mode() { return 0; }, set mode(value) {}, get selectedSid() { return state.selected; }, mouseType: 0, mouseX: 0, mouseY: 0 }, {});
    new RawUserSelection(scene.pool, scene.types, state, { enabled: () => 1, highlight: (mode) => highlight.highlight(mode), selectNotice: () => { throw new Error('범위 밖 DWORD 의 잘린 SID 통지'); } }).clear();
    assert.deepEqual(state.multiple, input); assert.equal(state.multipleCount, 0); assert.equal(state.selected, 0);
    // 배열 길이보다 큰 개수는 첫 항목 통지 뒤 범위 오류이며 개수/선택은 쓰지 않는다.
    const late = { selected: 0x87654321, mouseType: 0, rememberedPoint: [0, 0], multiple: [55], multipleCount: 2 }; scene.raw(55).fill(0);
    let notices = 0; let refreshes = 0;
    assert.throws(() => new RawUserSelection(scene.pool, scene.types, late, { enabled: () => 1, selectNotice: (sid) => { assert.equal(sid, 55); notices += 1; }, refresh: () => { refreshes += 1; } }).clear(), RangeError);
    assert.equal(notices, 1); assert.equal(refreshes, 1); assert.equal(late.multipleCount, 2); assert.equal(late.selected, 0x87654321);
});

test('RawUserPick 원본 10.78 1,092행: 소유자·이동·깊이·좌표 순위와 필터·가상 hit 호출 순서·마지막 근접 교체와 전체 슬롯 불변성이 일치한다', () => {
    const table = rows('user-pick-x86.tsv'); assert.equal(table.length, 1092); const failures = [];
    const pool = new SidPool(CAPACITY, { server: true }); const hash = new SquidHash(); const spots = new Uint8Array(65536); const frames = Array.from({ length: TYPES }, () => new FrameCodeTable([]));
    // 기존 풀의 서버 영역을 포함하고 순위 검사에서 사용하지 않는 지도도 실제 중심 처리기 계약을 만족한다.
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 4, `행 ${index}`);
        const header = numbers(row[0]); const objects = row[1].split(';'); const types = Array.from({ length: TYPES }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        const ids = []; const hits = []; let trace = ''; let cursor = 0;
        // TYPE 80..87 의 합성 역할만 설치한다. 실제 세 공중 유닛의 생성자/VT 는 실행하지 않는다.
        for (const encoded of objects) {
            const item = encoded.split(','); assert.equal(item.length, 12); const sid = Number(item[0]); const type = Number(item[1]); ids.push(sid); hits.push(Number(item[9]) !== 0);
            types[type].flags1 = Number(item[2]); types[type].flags2 = Number(item[3]); types[type].footX = Number(item[10]) | 0; types[type].footY = Number(item[11]) | 0;
            const raw = pool.physical(sid); raw.fill(0); raw[10] = type; raw[34] = Number(item[4]) & 0xff; raw[35] = Number(item[5]) & 0xff; raw[40] = Number(item[8]) & 0xff; put(raw, 14, Number(item[6])); put(raw, 18, Number(item[7]));
        }
        const before = pool.bytes(); const footprint = new RawSquidNeighbors(pool, hash, spots, types, frames); const problems = [];
        const hooks = {
            paused: () => { trace = 'paused'; return header[0]; }, localOwner: () => header[1], metamap: () => header[2], hideBuildings: () => header[3],
            project: (x, y) => { if (x !== 480 || y !== 330) problems.push('project-args'); trace += ';project'; return [floatOf(header[5]), floatOf(header[6])]; },
            begin: (point, first, second) => { if (floatBits(point[0]) !== header[5] || floatBits(point[1]) !== header[6]) problems.push('begin-point'); trace += `;begin:${first},${second}`; cursor = 0; return ids[0]; },
            next: () => (++cursor < ids.length ? ids[cursor] : 0), center: (sid) => footprint.center(sid),
            hit: (sid, x, y) => { if (x !== 480 || y !== 330) problems.push('hit-args'); trace += `;hit:${sid}`; return hits[sid - 50]; },
        };
        const result = new RawUserPick(pool, types, hooks).pick(480, 330, header[4]);
        if (result !== Number(row[2])) problems.push('result'); if (trace !== row[3]) problems.push('trace'); if (Buffer.compare(Buffer.from(before), Buffer.from(pool.bytes())) !== 0) problems.push('slots');
        if (problems.length) failures.push(`${index}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

test('RawUserPick 독립 시험: 정지/빈 검색에서 불필요한 하위를 호출하지 않고 도달한 누락은 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: true }); const types = Array.from({ length: TYPES }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    assert.equal(new RawUserPick(pool, types, { paused: () => 1 }).pick(0, 0), 0);
    assert.throws(() => new RawUserPick(pool, types, { paused: () => 0 }).pick(0, 0), /하위 미연결/);
    const hooks = { paused: () => 0, project: () => [0, 0], metamap: () => 0, begin: () => 24000 };
    assert.equal(new RawUserPick(pool, types, hooks).pick(0, 0), 0); hooks.begin = () => 0; assert.equal(new RawUserPick(pool, types, hooks).pick(0, 0), 0);
    // 10.78 이 아닌 풀은 실행 전 거부한다.
    assert.throws(() => new RawUserPick({ edition: 'cd1072', capacity: 15000 }, types, hooks), TypeError);
});
