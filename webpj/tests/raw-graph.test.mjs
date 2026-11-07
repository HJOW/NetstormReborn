// 실제 SID 풀·4단계 해시·spot을 정수 Graph에 연결하는 raw-graph.js(RawGraph/RawGraphLive 이식)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
// 원본(originals) 10.78 행만 재생한다(originalCD 행은 범위 밖). 재생하는 관측:
//  - graph-reset-x86.tsv 16행, graph-live-checking-x86.tsv·graph-live-free-callback-x86.tsv(Add/Flood/Rebuild의 진단·로그 재진입),
//    graph-detach-live-x86.tsv·graph-region-free-x86.tsv(삭제 정책/시계/영역), graph-reference-checking-x86.tsv의 Remove 행,
//    graph-sentinel-checking-x86.tsv, graphforms-x86.tsv(전체 풀 재구성의 내부 Form 생략), graphlookup-x86.tsv(위치 머리 번호/원천 프레임), graphrebuild-x86.tsv(소진·전역 재구성).
// Pop/PostPop/Unpop·SquidDisplay가 필요한 fixture(rawgraph/regiongraph/graphremove/graphrecovery)는 SquidPostPop 처리기 단위에서 대조한다.
// (2026-11-08 WB04-E40: RawGraph/RawGraphLive 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { GRAPH_RECOVERY, checkGraphSentinel, removeGraphSurface } from '../www/core/graph.js';
import { RawGraph, RawGraphStorage, resetRawGraphTable } from '../www/core/raw-graph.js';
import { rawPathSurfaceAt } from '../www/core/raw-path-surface.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { REPO_ROOT } from './helpers.mjs';
import { adler32, floatToBits, put, readRows } from './raw-support.mjs';

// 표면 타입 flags1 비트와 첫 자산 타입(Node 가 쓰는 번호는 74 이상)이다. (2026-11-08 WB04-E40)
const SURFACE = 0x800;
const WORLD = 256;

// fixture 파일 경로이다. (2026-11-08 WB04-E40)
const fixture = (name) => path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name);

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E40)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// hex 문자열을 바이트 배열로 만든다. (2026-11-08 WB04-E40)
const fromHex = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));

// 부호 있는 16비트로 해석한다. (2026-11-08 WB04-E40)
const int16 = (value) => (((value & 0xffff) << 16) >> 16);

// 255행 레코드를 6 BYTE 리틀엔디언 바이트로 직렬화한다(reserved 포함). (2026-11-08 WB04-E40)
function recordBytes(records) {
    const bytes = new Uint8Array(records.length * 6);
    // 각 행의 세 WORD 를 차례로 쓴다.
    records.forEach((record, index) => {
        put(bytes, index * 6, record.surfaces & 0xffff, 2);
        put(bytes, index * 6 + 2, record.inUse & 0xffff, 2);
        put(bytes, index * 6 + 4, record.reserved & 0xffff, 2);
    });
    return bytes;
}

// 스택 4096 DWORD 를 리틀엔디언 바이트로 직렬화한다. (2026-11-08 WB04-E40)
function stackBytes(stack) {
    const bytes = new Uint8Array(stack.length * 4);
    // 사용/미사용 영역 전체를 그대로 쓴다.
    stack.forEach((value, index) => put(bytes, index * 4, value, 4));
    return bytes;
}

// 6 BYTE 레코드 바이트를 레코드 배열로 해석한다. (2026-11-08 WB04-E40)
function recordsFromBytes(bytes) {
    const records = [];
    // 세 WORD 를 부호 있는 값으로 읽는다.
    for (let index = 0; index < bytes.length / 6; index += 1) {
        const word = (field) => int16(bytes[index * 6 + field * 2] | (bytes[index * 6 + field * 2 + 1] << 8));
        records.push({ surfaces: word(0), inUse: word(1), reserved: word(2) });
    }
    return records;
}

// 188개 기본 타입 표(발자국 1×1)와 빈 프레임 표를 만든다. (2026-11-08 WB04-E40)
const makeTypes = () => Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
const makeFrames = () => Array.from({ length: 188 }, () => []);

// 제자리 변경 보조: 진단 콜백이 같은 행을 고치는 원문 동작을 흉내 낸다. (2026-11-08 WB04-E40)
const setRecord = (records, index, surfaces, inUse, reserved) => Object.assign(records[index], { surfaces, inUse, reserved });

// 슬롯을 raw 입력으로 채운다(원문 Node: vtable·next·타입·상태·좌표·그래프·레벨·프레임·extra). n = [x,y,footX,footY,flags1,flags2,side,variant,frame,state,extra,graph,level]. (2026-11-08 WB04-E40)
function node(pool, sid, number, n, next = 0) {
    const bytes = pool.physical(sid);
    bytes.fill(0);
    put(bytes, 0, 0x501dd0);
    put(bytes, 4, next, 2);
    put(bytes, 8, 23, 2);
    bytes[10] = number;
    bytes[11] = n[9];
    put(bytes, 14, floatToBits(n[0]));
    put(bytes, 18, floatToBits(n[1]));
    bytes[30] = n[11];
    bytes[33] = n[12];
    bytes[34] = 1;
    put(bytes, 36, n[8], 4);
    bytes[40] = n[10];
}

// 예외가 났는지만 본다. (2026-11-08 WB04-E40)
function throws(operation) {
    try { operation(); } catch { return true; }
    return false;
}

// 전체 표와 전체 스택의 흔적을 Adler 쌍으로 만든다. (2026-11-08 WB04-E40)
const graphHashes = (graph) => [adler32(recordBytes(graph.records())), adler32(stackBytes(graph.floodStack()))];

// 실행기보다 오래 사는 두 배열의 모든 바이트이다. (2026-11-08 WB04-E40)
const sharedBytes = (storage) => hex(recordBytes(storage.records)) + hex(stackBytes(storage.stack));

// 장면 공통 입력: 원문 위치 여섯 개와 원문처럼 5..10 을 쓰는 풀·타입 75.. 설정이다. (2026-11-08 WB04-E40)
const POSITIONS = [[20, 20], [19, 20], [21, 20], [22, 20], [20, 21], [20, 19]];

// 현재 checking 이 켜졌는지 보는 호출 기록 장치이다. (2026-11-08 WB04-E40)
class SceneBase {
    constructor() {
        this.pool = new SidPool(32768, { server: true });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = makeTypes();
        this.frames = makeFrames();
        this.storage = new RawGraphStorage();
        this.ids = [];
        this.checking = 0;
        this.events = '';
    }

    // 호출 순서 문자열에 이벤트를 붙인다.
    append(value) { this.events += (this.events ? '|' : '') + value; }

    // 원문 255행 직렬화를 hex 로 돌려준다.
    records() { return hex(recordBytes(this.storage.records)); }

    // 4096 DWORD 스택을 hex 로 돌려준다.
    stack() { return hex(stackBytes(this.storage.stack)); }

    // 여섯 슬롯 전체를 이어 붙인 hex 이다.
    slots() { return this.ids.map((sid) => hex(this.pool.slot(sid))).join(''); }

    // 슬롯 하나에 쓸 수 있는 물리 대여이다.
    raw(index) { return this.pool.physical(this.ids[index]); }
}

// 진단/로그 재진입 장면(graph-live-checking-x86.tsv 의 한 행). 원문 입력만 조립하며 기대값은 fixture 에서 읽는다. (2026-11-08 WB04-E40)
class LiveGraphScene extends SceneBase {
    constructor(row) {
        super();
        this.scene = row[1];
        this.checking = Number(row[2]);
        this.action = Number(row[3]);
        this.traces = 0;
        const numbers = [row[1] === 'Flood' ? 3 : 254, 3, 4, 4, 5, 6];
        // 원문 객체를 실제 할당 슬롯에만 기록하고 주변 발자국 지도도 같은 입력으로 만든다.
        for (let index = 0; index < 6; index += 1) {
            this.ids[index] = this.pool.allocate(2);
            assert.equal(this.ids[index], index + 5);
            const raw = this.raw(index);
            raw.fill(0);
            const type = 75 + index;
            Object.assign(this.types[type], { footX: 1, footY: 1, flags1: SURFACE, flags2: 4 });
            this.frames[type] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
            raw[10] = type;
            raw[30] = numbers[index];
            const x = index === 0 && row[1] === 'AddZero' ? 0 : POSITIONS[index][0];
            const y = POSITIONS[index][1];
            put(raw, 14, floatToBits(x));
            put(raw, 18, floatToBits(y));
            this.hash.setCell(0, x, y, this.ids[index]);
        }
        if (row[1] === 'AddVoid') this.raw(0)[11] = 4;
        // 비활성 행의 reserved 와 전체 오래된 스택도 원문 입력대로 넣는다.
        this.storage.records.forEach((record, index) => Object.assign(record, { surfaces: 0, inUse: 0, reserved: 100 + index }));
        // 원문 표의 장부 차이는 보정하지 않는다.
        for (const [index, surfaces, inUse] of [[3, 2, 2], [4, 2, 1], [5, 1, 1], [6, 1, 1], [7, 0, 1], [254, 0x7dfd, 0x7dfd]]) {
            this.storage.records[index].surfaces = surfaces;
            this.storage.records[index].inUse = inUse;
        }
        this.storage.stack.forEach((_, index) => { this.storage.stack[index] = (0xa0000000 + index) >>> 0; });
        this.graph = new RawGraph(this.pool, this.hash, this.spots, this.types, this.frames, { storage: this.storage, recovery: GRAPH_RECOVERY.fullPool });
        this.graph.bindDiagnostics({ checking: () => this.checking, assertion: (expression, file, line) => this.assertion(expression, file, line) });
        this.graph.bindTrace((level, text) => this.trace(level, text));
    }

    // 원문 진단 입력을 기록하고 지정한 실제 행/다음 슬롯 변경을 수행한 뒤 돌아온다.
    assertion(expression, file, line) {
        this.append(`a:${line}:${expression}:${file}`);
        if (this.action === 1) {
            this.checking = 0;
            setRecord(this.storage.records, 3, 9, 0, -310);
            const raw = this.raw(0);
            raw[11] = 0;
            put(raw, 14, floatToBits(20));
            put(raw, 18, floatToBits(20));
        } else if (this.action === 2) {
            this.raw(0)[11] = 0;
            if (line === 0x100 || line === 0xc2) {
                this.raw(2)[30] = 5;
                this.storage.records[5].surfaces = 3;
            } else if (line === 0x113) {
                this.storage.records[5].surfaces = 1;
                this.storage.records[4].surfaces = 77;
            }
        } else if (this.action === 4 && line === 0x68) this.checking = 0;
    }

    // 로그 인자의 수를 먼저 보관한 뒤 뒤 후보/현재 행/스택/지도만 Native 입력대로 바꾼다.
    trace(level, text) {
        this.append(`l:${level}:${text.slice(0, text.indexOf(' '))}`);
        this.traces += 1;
        if (this.traces !== 1) return;
        if (this.action === 3) {
            const number = this.pool.slot(this.ids[0])[30];
            // 지역 연결 목록은 건드리지 않고 뒤 SID 의 실제 번호를 바꾼다.
            for (const index of [2, 4]) this.raw(index)[30] = number;
            if (number < 251) this.storage.records[number].surfaces = 91;
            this.storage.stack[4095] = 0xdeadbeef;
        } else if (this.action === 4) {
            this.storage.records[254].surfaces = 0;
            this.storage.records[254].inUse = 0;
        } else if (this.action === 5) this.hash.setCell(0, 22, 20, 0);
        else if (this.action === 6) this.raw(2)[11] = 1;
    }
}

// 한 PE fixture 의 전체 현재 표/스택/슬롯과 진단·로그 순서를 비교한다. originals 행만 재생하고 재생 수를 돌려준다. (2026-11-08 WB04-E40)
function replayLiveRows(name) {
    let count = 0;
    // 기대값은 원문 출력이며 JS 모델로 다시 계산하지 않는다.
    for (const row of readRows(fixture(name))) {
        assert.equal(row.length, 10);
        if (row[0] !== 'originals') continue;
        const scene = new LiveGraphScene(row);
        let result = '-';
        if (row[1].startsWith('Add')) scene.graph.add(scene.ids[0]);
        else if (row[1] === 'Flood') result = String(scene.graph.flood(scene.ids[0], 7));
        else scene.graph.rebuild(row[1] === 'Rebuild1');
        const label = `${name} ${row[1]} checking=${row[2]} action=${row[3]}`;
        assert.equal(result, row[4], `${label} 반환`);
        assert.equal(scene.records(), row[5], `${label} 기록`);
        assert.equal(scene.stack(), row[6], `${label} 스택`);
        assert.equal(scene.slots(), row[7], `${label} 슬롯`);
        assert.equal(scene.checking, Number(row[8]), `${label} checking`);
        assert.equal(scene.events || '-', row[9], `${label} 이벤트`);
        count += 1;
    }
    return count;
}

// 삭제 장면(graph-detach-live-x86.tsv 의 한 행): 현재 정책/진단/로그/시계 복귀와 값 원천을 확인하는 입력이다. (2026-11-08 WB04-E40)
class DetachLiveScene extends SceneBase {
    constructor(row) {
        super();
        this.checking = Number(row[2]);
        this.action = Number(row[3]);
        this.enabled = 1;
        this.traces = 0;
        this.clocks = 0;
        const separator = row[1].indexOf(':');
        this.scene = row[1].slice(0, separator);
        this.rebuildFlag = Number(row[1].slice(separator + 1));
        this.enabled = this.scene === 'Disabled' ? 0 : 1;
        // 동일 raw 슬롯/타입/위치와 네 일반 지도 중 0단계만 채운다.
        for (let index = 0; index < 6; index += 1) {
            this.ids[index] = this.pool.allocate(2);
            assert.equal(this.ids[index], index + 5);
            const raw = this.raw(index);
            raw.fill(0);
            const type = 75 + index;
            Object.assign(this.types[type], { footX: 1, footY: 1, flags1: SURFACE, flags2: 4 });
            this.frames[type] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
            raw[10] = type;
            raw[11] = index === 0 ? 2 : 0;
            raw[30] = this.scene === 'Lookup' && index ? 4 : 3;
            const x = POSITIONS[index][0];
            const y = this.scene === 'Lookup' && index === 5 ? 20 : POSITIONS[index][1];
            put(raw, 14, floatToBits(x));
            put(raw, 18, floatToBits(y));
            this.hash.setCell(0, x, y, this.ids[index]);
        }
        if (this.scene === 'Lookup') put(this.raw(5), 4, this.ids[0], 2);
        if (this.scene === 'Missing') this.hash.setCell(0, 20, 20, 0);
        Object.assign(this.types[81], { footX: 3, footY: 3, flags1: SURFACE, flags2: 8 });
        this.frames[81] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
        // 미사용 행의 reserved 와 전체 미활성 스택 흔적까지 원문 입력과 같다.
        this.storage.records.forEach((record, index) => Object.assign(record, { surfaces: 0, inUse: 0, reserved: 100 + index }));
        setRecord(this.storage.records, 3, 6, this.scene === 'Inactive' ? 0 : 2, this.storage.records[3].reserved);
        setRecord(this.storage.records, 4, 6, 1, this.storage.records[4].reserved);
        this.storage.records[254].surfaces = this.action === 5 ? 0 : 0x7dfd;
        this.storage.records[254].inUse = this.action === 5 ? 0 : 0x7dfd;
        this.storage.stack.forEach((_, index) => { this.storage.stack[index] = (0xa0000000 + index) >>> 0; });
        this.graph = new RawGraph(this.pool, this.hash, this.spots, this.types, this.frames, { storage: this.storage, recovery: GRAPH_RECOVERY.fullPool });
        this.graph.bindDetachState(() => this.enabled, () => this.rebuildFlag);
        this.graph.bindDiagnostics({ checking: () => this.checking, assertion: (expression, file, line) => this.assertion(expression, file, line) });
        this.graph.bindTrace((level, text) => this.trace(level, text));
        this.graph.bindClock(() => this.clock());
    }

    // 선택된 원문 진단만 기록하며 첫 sentinel 진단의 현재 checking/활성 변경을 보존한다.
    assertion(expression, file, line) {
        this.append(`a:${line}:${expression}:${file}`);
        if (line === 0x59 && (this.action === 9 || this.action === 10)) {
            const raw = this.raw(0);
            raw[11] = 1;
            if (this.action === 10) raw[10] = 82;
        }
        if ((this.action === 2 || this.action === 5) && line === 0x68) {
            this.checking = 0;
            if (this.action === 5) this.enabled = 0;
        }
    }

    // 첫 Flood 로그에서 뒤 후보/기록/정책/지도만 지정 입력대로 바꾼다.
    trace(level, text) {
        this.append(`l:${level}:${text.slice(0, text.indexOf(' '))}`);
        this.traces += 1;
        if (this.traces !== 1) return;
        if (this.action === 1) {
            // 지역 연결 배열은 그대로 두고 다음 후보의 실제 graph BYTE 를 바꾼다.
            for (const index of [2, 4]) this.raw(index)[30] = 3;
            setRecord(this.storage.records, 3, 91, 2, -321);
            this.storage.stack[4095] = 0xdeadbeef;
            this.rebuildFlag = 1;
        } else if (this.action === 2) {
            this.storage.records[254].surfaces = 0;
            this.storage.records[254].inUse = 0;
        } else if (this.action === 4) this.raw(2)[11] = 1;
        else if (this.action === 6) this.hash.setCell(0, 22, 20, 0);
        else if (this.action === 8) this.rebuildFlag = 0;
    }

    // 두 원문 지역 시각 읽기의 경계에서 현재 정책을 바꾸며 값 원천 좌표는 보존돼야 한다.
    clock() {
        this.append('t');
        this.clocks += 1;
        if (this.clocks === 1 && this.action === 3) {
            this.rebuildFlag = 1;
            this.enabled = 0;
            const raw = this.raw(0);
            put(raw, 14, floatToBits(200));
            put(raw, 18, floatToBits(200));
            setRecord(this.storage.records, 3, 91, 2, -321);
        } else if (this.clocks === 1 && this.action === 7) this.rebuildFlag = 0;
        return 17.25;
    }
}

test('graph-reset: 전체 1530 BYTE 입력의 Reset 후 기록을 원본 16개 출력과 대조하고 풀·스택은 보존한다', () => {
    const rows = readRows(fixture('graph-reset-x86.tsv'));
    assert.equal(rows.length, 17);
    assert.equal(rows[0][0], 'seed');
    // 이전 표의 출력은 다음 초기 입력에 재사용하지 않는다.
    for (const row of rows.slice(1)) {
        const seed = Number(row[0]);
        const input = Uint8Array.from({ length: 1530 }, (_, index) => (index * 37 + seed) & 0xff);
        const stack = new Uint32Array(4096).fill(0x12345678);
        const pool = new SidPool(24000, { server: false });
        const hash = new SquidHash();
        const graph = new RawGraph(pool, hash, new Uint8Array(65536), makeTypes(), makeFrames(), { records: recordsFromBytes(input), stack });
        const before = hex(pool.slot(5));
        graph.resetTable();
        assert.equal(hex(recordBytes(graph.records())), row[1]);
        assert.equal(hex(pool.slot(5)), before);
        assert.ok(graph.floodStack().every((value) => value === 0x12345678));
        // 대여 실행기와 독립 helper 는 같은 Reset 몸체를 쓴다.
        const storage = new RawGraphStorage();
        storage.records = recordsFromBytes(input);
        storage.stack.fill(0x12340000);
        resetRawGraphTable(storage);
        assert.equal(hex(recordBytes(storage.records)), row[1]);
        assert.ok(storage.stack.every((value) => value === 0x12340000));
    }
});

test('graph-live-checking: Add·Flood·Rebuild 의 진단/로그 재진입을 원본 10.78 행의 표·스택·슬롯·이벤트와 대조한다', () => {
    const count = replayLiveRows('graph-live-checking-x86.tsv');
    assert.equal(count, 72);
});

test('graph-live-free-callback: 첫 로그에서 뒤 후보가 free 가 되어도 내부 Flood 가 계속 쓰는 행을 대조한다', () => {
    const count = replayLiveRows('graph-live-free-callback-x86.tsv');
    assert.equal(count, 12);
});

// 삭제 fixture 한 파일을 재생한다. 영역 단독 통지는 inspectRegion 으로 구별한다. (2026-11-08 WB04-E40)
function replayDetachRows(name, region) {
    let count = 0;
    // 기대값은 원문 출력이며 JS 모델로 다시 계산하지 않는다.
    for (const row of readRows(fixture(name))) {
        assert.equal(row.length, 12);
        if (row[0] !== 'originals') continue;
        const scene = new DetachLiveScene(row);
        if (region) {
            setRecord(scene.storage.records, 3, 0, 0, scene.storage.records[3].reserved);
            scene.storage.records[3].surfaces = 0;
            scene.storage.records[3].inUse = 0;
            scene.spots[20 * 256 + 20] = 8;
            scene.graph.invalidateRegion(scene.ids[0]);
        } else {
            const replacement = scene.scene === 'Override';
            scene.graph.detach(scene.ids[0], scene.rebuildFlag !== 0, replacement ? 9 : 1, replacement ? 81 : null);
        }
        const label = `${name} ${row[1]} checking=${row[2]} action=${row[3]}`;
        assert.equal(scene.records(), row[5], `${label} 기록`);
        assert.equal(scene.stack(), row[6], `${label} 스택`);
        assert.equal(scene.slots(), row[7], `${label} 슬롯`);
        assert.equal(scene.checking, Number(row[8]), `${label} checking`);
        assert.equal(scene.events || '-', row[9], `${label} 이벤트`);
        if (!region) {
            assert.equal(scene.enabled, Number(row[10]), `${label} enabled`);
            assert.equal(scene.rebuildFlag, Number(row[11]), `${label} rebuild`);
        }
        count += 1;
    }
    return count;
}

test('graph-detach-live: 삭제의 현재 정책·진단·로그·시계 복귀와 값 원천을 원본 10.78 행과 대조한다', () => {
    assert.equal(replayDetachRows('graph-detach-live-x86.tsv', false), 216);
});

test('graph-region-free: 영역 Remove 진단으로 free/비표면이 되어도 같은 graph BYTE 를 쓰는 행을 대조한다', () => {
    assert.equal(replayDetachRows('graph-region-free-x86.tsv', true), 4);
});

test('graph-reference-checking: Remove 의 signed WORD·전체 checking·식/파일/행 순서를 원본 10.78 행과 대조한다', () => {
    let count = 0;
    let skipped = 0;
    // 기대값은 독립 PE 결과이며 JS 로 재계산하지 않는다.
    for (const row of readRows(fixture('graph-reference-checking-x86.tsv'))) {
        assert.equal(row.length, 10);
        if (row[0] !== 'originals') continue;
        // Zero 행은 RawPlayerPlacementAnchor.GraphFor(0)을 요구하므로 플레이어 위치 단위에서 대조한다.
        if (row[1] !== 'Remove') { skipped += 1; continue; }
        const record = { surfaces: int16(Number(row[2])), inUse: int16(Number(row[3])), reserved: 19433 };
        let checking = Number(row[4]);
        const action = Number(row[5]);
        const events = [];
        const assertion = (expression, file, line) => {
            events.push(`${line}:${expression}:${file}`);
            if (action === 1) checking = 0;
            else if (action === 2) Object.assign(record, { surfaces: 3, inUse: 1, reserved: -310 });
            else if (action === 3) record.surfaces = 4;
            else if (action === 4) record.inUse = 1;
        };
        removeGraphSurface(record, { checking: () => checking, assertion });
        assert.equal(hex(recordBytes([record])), row[6], `Remove ${row[2]},${row[3]} 기록`);
        assert.equal(checking, Number(row[7]));
        assert.equal(events.join('|') || '-', row[8]);
        count += 1;
    }
    assert.equal(count, 360);
    assert.equal(skipped, 20);
});

test('graph-sentinel-checking: sentinel 진단 복귀와 Free 의 현재 기록 쓰기를 원본 10.78 행과 대조한다', () => {
    let count = 0;
    // 각 행마다 장기 물리/진단 owner 를 새로 구성하여 앞 입력의 효과를 섞지 않는다.
    for (const row of readRows(fixture('graph-sentinel-checking-x86.tsv'))) {
        assert.equal(row.length, 10);
        if (row[0] !== 'originals') continue;
        const storage = new RawGraphStorage();
        setRecord(storage.records, 3, 7, 1, 19433);
        setRecord(storage.records, 254, int16(Number(row[2])), int16(Number(row[3])), 27499);
        storage.stack[4095] = 0x12345678;
        const pool = new SidPool(32768, { server: true });
        const graph = new RawGraph(pool, new SquidHash(), new Uint8Array(65536), makeTypes(), makeFrames(), { storage });
        let checking = Number(row[4]);
        const action = Number(row[5]);
        const events = [];
        // 진단은 현재 sentinel/checking 또는 같은 Free 대상의 reserved 도 변경한 뒤 원문처럼 돌아온다.
        const diagnostics = {
            checking: () => checking,
            assertion: (expression, file, line) => {
                events.push(`${line}:${expression}:${file}`);
                if (action === 1) checking = 0;
                else if (action === 2) storage.records[254].inUse = 0x7dfd;
                else if (action === 3) storage.records[254].inUse = -1;
                else if (action === 4) setRecord(storage.records, 3, 9, -1, -310);
            },
        };
        graph.bindDiagnostics(diagnostics);
        if (row[1] === 'Check') checkGraphSentinel(storage.records[254], diagnostics);
        else graph.free(row[1] === 'Free' ? 3 : 254);
        const label = `${row[1]} ${row[2]},${row[3]} checking=${row[4]} action=${row[5]}`;
        assert.equal(hex(recordBytes([storage.records[254]])), row[6], `${label} sentinel`);
        assert.equal(hex(recordBytes([storage.records[3]])), row[7], `${label} 대상`);
        assert.equal(checking, Number(row[8]), `${label} checking`);
        assert.equal(events.join('|') || '-', row[9], `${label} 이벤트`);
        assert.equal(storage.stack[4095], 0x12345678);
        count += 1;
    }
    assert.equal(count, 540);
});

test('graph-sentinel-checking: 선택된 진단 장치가 없으면 명시 오류로 멈추고 기록을 임의로 정상화하지 않는다', () => {
    const record = { surfaces: 0, inUse: 0, reserved: 313 };
    assert.throws(() => checkGraphSentinel(record, { checking: () => 1 }), /진단 미연결/);
    assert.deepEqual(record, { surfaces: 0, inUse: 0, reserved: 313 });
    checkGraphSentinel(record, { checking: () => 0 });
});

test('graphforms: 전체 풀 재구성이 내부 Form 을 생략하는 원본 10.78 관측(32행)의 전체 슬롯·기록·스택을 대조한다', () => {
    let count = 0;
    // 각 행은 초기 입력과 실제 PE 정상 반환을 함께 보존한다.
    for (const row of readRows(fixture('graphforms-x86.tsv'))) {
        assert.equal(row.length, 11);
        if (row[0] !== 'originals') continue;
        const pool = new SidPool(32768, { server: false });
        const hash = new SquidHash();
        const expectedIds = row[4].split(',');
        const before = row[5].split(';');
        const after = row[6].split(';');
        const ids = [];
        // 실제 SID client 할당 순서와 초기 전체 raw 슬롯을 준비한다.
        expectedIds.forEach((expected, index) => {
            const id = pool.allocate(2);
            ids.push(id);
            assert.equal(id, Number(expected));
            const bytes = fromHex(before[index]);
            const slot = pool.physical(id);
            assert.equal(bytes.length, slot.length);
            slot.set(bytes);
        });
        hash.setBucket(0, 20, 20, ids[0]);
        const spots = new Uint8Array(65536);
        const types = makeTypes();
        const frames = makeFrames();
        Object.assign(types[74], { flags1: SURFACE, flags2: 2, footX: 1, footY: 1 });
        frames[74] = [{ side: 65, variant: 80, number: 1, flags: 0 }, { side: 65, variant: 80, number: 2, flags: 0 }];
        const seed = Number(row[9]);
        const stack = Uint32Array.from({ length: 4096 }, (_, index) => (seed + index) >>> 0);
        const graph = new RawGraph(pool, hash, spots, types, frames, { records: recordsFromBytes(fromHex(row[7])), stack });
        graph.rebuild(Number(row[1]) !== 0);
        // 내부 Form 여섯 개의 NaN/stale 필드를 포함한 전체 슬롯도 보존돼야 한다.
        ids.forEach((id, index) => assert.equal(hex(pool.slot(id)), after[index], `graphforms 슬롯 ${index}`));
        assert.equal(hex(recordBytes(graph.records())), row[8]);
        assert.equal(hex(stackBytes(graph.floodStack())), row[10]);
        assert.equal(hash.bucket(0, 20, 20), ids[0]);
        assert.ok(spots.every((value) => value === 0));
        count += 1;
    }
    assert.equal(count, 32);
});

test('graphforms: 공간 머리에 내부 Form 이 등장하는 손상 입력은 기록·풀을 쓰기 전에 거부한다', () => {
    const pool = new SidPool(32768, { server: false });
    const id = pool.allocate(2);
    const slot = pool.physical(id);
    slot[10] = 46;
    slot[11] = 4;
    const hash = new SquidHash();
    hash.setBucket(0, 20, 20, id);
    const graph = new RawGraph(pool, hash, new Uint8Array(65536), makeTypes(), makeFrames());
    const before = pool.bytes();
    assert.ok(throws(() => graph.rebuild(false)));
    assert.deepEqual(pool.bytes(), before);
    assert.equal(hash.bucket(0, 20, 20), id);
    assert.equal(graph.records()[0].inUse, 0);
});

// fixture 문자열("x,y,z;x,y,z" 또는 "-")을 정수 행으로 나눈다. (2026-11-08 WB04-E40)
const parseRows = (text) => (text === '-' ? [] : text.split(';').map((entry) => entry.split(/[,\s]+/).filter(Boolean).map(Number)));

// 세 Totalmade 표에 넣은 오래된 DWORD 입력의 Adler(원문 fixture 의 고정 입력)이다. (2026-11-08 WB04-E40)
const countTableSums = [0, 17, 34].map((offset) => adler32(stackBytes(Uint32Array.from({ length: 256 }, (_, index) => (31337 + Math.imul(index, 0x9e3779b1) + offset) >>> 0))));

// Pop/PostPop 이 필요 없는 그래프 연산 fixture 를 재생한다(Setup 입력과 Detach/Region/Add/Rebuild/Allocate/Flood). 반환은 비교한 연산 수이다. (2026-11-08 WB04-E40)
function replayGraphRows(name, { globalFixture = false, recovery = GRAPH_RECOVERY.reject } = {}) {
    let active = false;
    let pool;
    let hash;
    let graph;
    let types;
    let frames;
    let spots;
    let ids = [];
    let calls = 0;
    // 기대값은 독립 PE 출력이다. originalCD 구간은 다음 Begin 까지 건너뛴다.
    for (const row of readRows(fixture(name))) {
        if (row[0] === 'Begin') {
            active = row[1] === 'originals';
            if (!active) continue;
            types = makeTypes();
            frames = makeFrames();
            spots = new Uint8Array(65536);
            pool = new SidPool(32768, { server: false });
            hash = new SquidHash();
            const expected = row[3].split(',').map(Number);
            ids = expected.map(() => pool.allocate(2));
            assert.deepEqual(ids, expected);
            continue;
        }
        if (!active) continue;
        if (row[0] === 'Setup') {
            hash.reset();
            spots.fill(0);
            if (globalFixture) {
                // 이 fixture 는 free 슬롯도 입력한다. 다음 Setup 은 같은 풀/번호를 새로 준비한다.
                pool = new SidPool(32768, { server: false });
                for (const id of ids) assert.equal(pool.allocate(2), id);
            }
            const nodes = parseRows(row[1]);
            assert.equal(nodes.length, ids.length);
            // 지도는 발자국 전체가 아니라 실제 기준점 버킷/체인을 입력한다.
            nodes.forEach((n, index) => {
                types[74 + index] = { flags1: n[4], flags2: n[5], footX: n[2], footY: n[3] };
                frames[74 + index] = [{ side: n[6], variant: n[7], number: 1, flags: 0 }, { side: 65, variant: 80, number: 2, flags: 0 }];
                const mapped = !(n[9] & 4);
                node(pool, ids[index], 74 + index, n, mapped ? hash.bucket(n[12], n[0], n[1]) : 0);
                if (mapped) hash.setBucket(n[12], n[0], n[1], ids[index]);
            });
            // 내부 spot 후보를 별도로 준비한다.
            for (const [x, y, value] of parseRows(row[2])) spots[y * WORLD + x] = value;
            const seed = Number(row[4]);
            const stack = Uint32Array.from({ length: 4096 }, (_, index) => (seed + index) >>> 0);
            graph = new RawGraph(pool, hash, spots, types, frames, { records: recordsFromBytes(fromHex(row[3])), stack, recovery });
            continue;
        }
        const args = parseRows(row[1]);
        if (row[0] === 'Detach') graph.detach(ids[0], args[0][0] !== 0, args[0][1]);
        else if (row[0] === 'Region') graph.invalidateRegion(ids[0]);
        else if (row[0] === 'Add') graph.add(ids[0]);
        else if (row[0] === 'Rebuild') graph.rebuild(args[0][0] !== 0);
        else if (row[0] === 'Allocate') assert.equal(graph.allocate(), Number(row[2]));
        else if (row[0] === 'Flood') assert.equal(graph.flood(ids[0], args[0][0]), Number(row[2]));
        else assert.fail(`${name}: Pop/PostPop 이 필요한 행 ${row[0]}`);
        if (row.length === 17) assert.equal(hex(recordBytes(graph.records())), row[16], `${name} 전체 표`);
        const label = `${name} ${row[0]} ${row[1]} #${calls}`;
        row[3].split(';').forEach((expected, index) => assert.equal(hex(pool.slot(ids[index])), expected, `${label} 슬롯 ${index}`));
        const [recordSum, stackSum] = graphHashes(graph);
        assert.equal(recordSum, Number(row[4]), `${label} 기록`);
        assert.equal(stackSum, Number(row[5]), `${label} 스택`);
        const heads = [];
        // 4단계 해시 전체를 원본 배열 순서로 대조한다.
        for (let level = 0; level < 4; level += 1) heads.push(new Uint8Array(hash.entries(level).buffer));
        assert.equal(adler32(Uint8Array.from(heads.flatMap((part) => Array.from(part)))), Number(row[6]), `${label} 해시`);
        assert.equal(adler32(spots), Number(row[7]), `${label} spot`);
        assert.equal(adler32(pool.bytes()), Number(row[8]), `${label} 풀`);
        // 그래프 연산은 PostPop 장부와 표시 목록을 건드리지 않는다.
        assert.equal(Number(row[9]), 53);
        assert.equal(Number(row[10]), 5);
        assert.deepEqual([row[11], row[12], row[13]].map(Number), countTableSums, `${label} 통계`);
        assert.equal(row[14], '0');
        assert.equal(row[15], '-');
        calls += 1;
    }
    return calls;
}

test('graphlookup: 같은 위치 다른 SID 의 graph BYTE 와 dead 원천 타입/프레임을 원본 10.78 행 128개와 대조한다', () => {
    assert.equal(replayGraphRows('graphlookup-x86.tsv'), 128);
});

test('graphrebuild: 무효 수집·전역 재구성·소진 할당을 원본 10.78 행 96개와 대조한다', () => {
    assert.equal(replayGraphRows('graphrebuild-x86.tsv', { globalFixture: true }), 96);
});

// 기록 255행(앞 251행은 같은 값)을 만든다. (2026-11-08 WB04-E40)
const fullRecords = (surfaces, inUse, reserved) => Array.from({ length: 255 }, (_, index) => (index < 251 ? { surfaces, inUse, reserved } : { surfaces: 0, inUse: 0, reserved: 0 }));

test('여유 번호 경로는 무관한 raw 프레임을 읽지 않고, 전체 재구성 실패는 쓰기를 남기지 않는다', () => {
    const types = makeTypes();
    const frames = makeFrames();
    types[74].flags1 = SURFACE;
    frames[74] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
    const pool = new SidPool(32768, { server: false });
    const sid = pool.allocate(2);
    node(pool, sid, 74, [20, 20, 1, 1, 0x800, 2, 65, 80, 99, 0, 0, 254, 0]);
    const hash = new SquidHash();
    hash.setBucket(0, 20, 20, sid);
    const spots = new Uint8Array(65536);
    const records = fullRecords(0, 0, 0);
    setRecord(records, 0, 31, 0, 71);
    setRecord(records, 1, 0, 0, 83);
    const graph = new RawGraph(pool, hash, spots, types, frames, { records });
    const saved = pool.bytes();
    assert.equal(graph.allocate(), 0);
    assert.equal(graph.records()[0].surfaces, 0);
    assert.equal(graph.records()[0].reserved, 71);
    const before = graphHashes(graph);
    assert.ok(throws(() => graph.rebuild(true)));
    assert.ok(throws(() => graph.rebuild(false)));
    assert.deepEqual(graphHashes(graph), before);
    assert.deepEqual(pool.bytes(), saved);
    assert.equal(hash.bucket(0, 20, 20), sid);
    assert.ok(spots.every((value) => value === 0));
    // 같은 손상 풀도 전체 251개 소진 시에는 재구성 전에 거부한다.
    const full = new RawGraph(pool, hash, spots, types, frames, { records: fullRecords(1, 1, 71) });
    const old = graphHashes(full);
    assert.ok(throws(() => full.allocate()));
    assert.deepEqual(graphHashes(full), old);
    assert.deepEqual(pool.bytes(), saved);
    // 기존 직접 Flood 는 지도에 없는 void 원천도 받는다. 전역 해시 보호가 그 계약을 바꾸지 않는다.
    const bytes = pool.physical(sid);
    put(bytes, 36, 0);
    bytes[11] = 4;
    hash.setBucket(0, 20, 20, 0);
    assert.equal(graph.flood(sid, 2), 1);
    assert.equal(bytes[11], 4);
    assert.equal(bytes[30], 2);
});

test('조회 머리의 무효/미사용 번호는 원천 프레임 판독보다 먼저 반환하고 손상 입력은 쓰기 전에 실패한다', () => {
    const types = makeTypes();
    const frames = makeFrames();
    // 원천과 조회 머리의 타입은 같아도 SID/graph byte 는 별개다.
    for (const type of [74, 75]) {
        types[type].flags1 = SURFACE;
        frames[type] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
    }
    const pool = new SidPool(32768, { server: false });
    const root = pool.allocate(2);
    const head = pool.allocate(2);
    node(pool, root, 74, [20, 20, 1, 1, 0x800, 4, 65, 80, 200, 2, 0, 0, 0]);
    node(pool, head, 75, [20, 20, 1, 1, 0x800, 4, 65, 80, 0, 2, 0, 254, 0]);
    put(pool.physical(head), 4, root, 2);
    const hash = new SquidHash();
    hash.setBucket(0, 20, 20, head);
    const spots = new Uint8Array(65536);
    const records = fullRecords(0, 0, 0);
    setRecord(records, 0, 3, 1, 71);
    setRecord(records, 1, 4, 0, 72);
    const graph = new RawGraph(pool, hash, spots, types, frames, { records });
    const before = graphHashes(graph);
    const lookup = pool.physical(head);
    // invalid 와 미사용 번호는 읽을 필요 없는 잘못된 원천 프레임을 허용한다.
    for (const number of [254, 1]) {
        lookup[30] = number;
        const saved = pool.bytes();
        graph.validateDetach(root);
        graph.detach(root);
        assert.deepEqual(graphHashes(graph), before);
        assert.deepEqual(pool.bytes(), saved);
    }
    lookup[30] = 0;
    assert.ok(throws(() => graph.detach(root)));
    assert.deepEqual(graphHashes(graph), before);
    assert.equal(hash.bucket(0, 20, 20), head);
    // 새 조회 경로의 범위 밖 번호와 void 머리도 변경 전에 거부한다.
    for (const state of [2, 4]) {
        lookup[11] = state;
        lookup[30] = state === 2 ? 255 : 0;
        const saved = pool.bytes();
        assert.ok(throws(() => graph.validateDetach(root)));
        assert.ok(throws(() => graph.detach(root)));
        assert.deepEqual(graphHashes(graph), before);
        assert.deepEqual(pool.bytes(), saved);
        assert.ok(spots.every((value) => value === 0));
    }
});

test('삭제 계획의 소진/손상/활성 원천 거부는 공간 해제 전에 전체 슬롯/지도/표/스택을 보존한다', () => {
    const types = makeTypes();
    const frames = makeFrames();
    // 삭제 원천과 두 이웃은 동일한 정상 표면 프레임을 가진다.
    for (let type = 74; type <= 76; type += 1) {
        Object.assign(types[type], { flags1: SURFACE, flags2: 4 });
        frames[type] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
    }
    const pool = new SidPool(32768, { server: false });
    const root = pool.allocate(2);
    const left = pool.allocate(2);
    const right = pool.allocate(2);
    node(pool, root, 74, [20, 20, 1, 1, 0x800, 4, 65, 80, 0, 2, 0, 0, 0]);
    node(pool, left, 75, [19, 20, 1, 1, 0x800, 4, 65, 80, 0, 0, 0, 0, 0]);
    node(pool, right, 76, [21, 20, 1, 1, 0x800, 4, 65, 80, 0, 0, 0, 0, 0]);
    const hash = new SquidHash();
    hash.setBucket(0, 20, 20, root);
    hash.setBucket(0, 19, 20, left);
    hash.setBucket(0, 21, 20, right);
    const spots = new Uint8Array(65536);
    // 소진 복구는 이 범위 밖이므로 새 번호 확보 전에 실패해야 한다.
    const records = fullRecords(3, 1, 71);
    const graph = new RawGraph(pool, hash, spots, types, frames, { records });
    const before = graphHashes(graph);
    const raw = pool.bytes();
    const oldSpots = Uint8Array.from(spots);
    assert.ok(throws(() => graph.validateDetach(root)));
    assert.ok(throws(() => graph.detach(root)));
    assert.deepEqual(graphHashes(graph), before);
    assert.deepEqual(spots, oldSpots);
    assert.deepEqual(pool.bytes(), raw);
    pool.physical(root)[11] = 0;
    assert.ok(throws(() => graph.detach(root)));
    pool.physical(root)[11] = 2;
    // 위치 버킷에 다른 좌표의 SID 가 들어간 손상 입력은 그래프/공간 변경 전에 거부한다.
    hash.setBucket(0, 20, 20, left);
    assert.ok(throws(() => graph.validateDetach(root)));
    assert.ok(throws(() => graph.detach(root)));
    assert.deepEqual(graphHashes(graph), before);
    assert.deepEqual(spots, oldSpots);
    assert.deepEqual(pool.bytes(), raw);
    hash.setBucket(0, 20, 20, root);
    put(pool.physical(left), 4, left, 2);
    assert.ok(throws(() => graph.detach(root)));
    assert.deepEqual(graphHashes(graph), before);
    assert.deepEqual(spots, oldSpots);
    // 미사용 원천은 finder 생성 전 반환하므로 이웃의 손상 next 를 읽지 않는다.
    const inactiveRecords = fullRecords(3, 1, 71);
    inactiveRecords[0].inUse = 0;
    const inactive = new RawGraph(pool, hash, spots, types, frames, { records: inactiveRecords });
    const inactiveBefore = graphHashes(inactive);
    inactive.detach(root);
    assert.deepEqual(graphHashes(inactive), inactiveBefore);
    assert.deepEqual(spots, oldSpots);
});

test('Add 의 손상 프레임은 쓰기 전에 거부하고, 사전 계산(projected)은 풀·지도·spot·기록을 바꾸지 않는다', () => {
    const types = makeTypes();
    const frames = makeFrames();
    types[74].flags1 = SURFACE;
    frames[74] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
    const pool = new SidPool(32768, { server: false });
    const sid = pool.allocate(2);
    node(pool, sid, 74, [90, 90, 1, 1, 0x800, 0, 65, 80, 0, 4, 0, 254, 1]);
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    // 정상 251개가 모두 사용 중인 입력은 원본 전역 재구성 경로를 요구한다.
    const graph = new RawGraph(pool, hash, spots, types, frames, { records: fullRecords(1, 1, 71) });
    const raw = pool.bytes();
    const before = graphHashes(graph);
    // 각 사전 계산은 현재 void 객체에 쓰지 않는다: 소진(Reject)과 소수 좌표 모두 쓰기 전에 거부한다.
    for (const x of [20, 20.25]) assert.ok(throws(() => graph.validatePostPop(sid, false, true, { x, y: 20, level: 1, type: types[74] })));
    assert.deepEqual(pool.bytes(), raw);
    assert.deepEqual(graphHashes(graph), before);
    assert.equal(hash.bucket(1, 20, 20), 0);
    assert.ok(spots.every((value) => value === 0));
    // 손상 프레임 번호는 Add 가 쓰기 전에 검출한다.
    const second = new RawGraph(pool, hash, spots, types, frames);
    pool.physical(sid)[11] = 0;
    pool.physical(sid)[36] = 99;
    pool.physical(sid)[30] = 254;
    put(pool.physical(sid), 14, floatToBits(20));
    put(pool.physical(sid), 18, floatToBits(20));
    const corrupted = pool.bytes();
    const secondBefore = graphHashes(second);
    assert.ok(throws(() => second.add(sid)));
    assert.deepEqual(graphHashes(second), secondBefore);
    assert.deepEqual(pool.bytes(), corrupted);
});

test('복구 사전 계산은 다른 void 표면의 손상 프레임/범위 밖 타입을 Pop 없이 검출한다', () => {
    const types = makeTypes();
    const frames = makeFrames();
    types[74].flags1 = SURFACE;
    types[75].flags1 = SURFACE;
    frames[74] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
    frames[75] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
    const pool = new SidPool(32768, { server: false });
    const root = pool.allocate(2);
    const other = pool.allocate(2);
    node(pool, root, 74, [90, 90, 1, 1, 0x800, 0, 65, 80, 0, 4, 0, 254, 0]);
    node(pool, other, 75, [50, 50, 1, 1, 0x800, 0, 65, 80, 99, 4, 0, 254, 0]);
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    // 모든 레코드를 활성으로 하여 Pop 뒤 Add 에 반드시 전역 재구성이 필요하게 만든다.
    const graph = new RawGraph(pool, hash, spots, types, frames, { records: fullRecords(7, 1, 71), recovery: GRAPH_RECOVERY.fullPool });
    // 처음에는 손상 프레임, 다음에는 범위 밖 타입을 전체 순회에서 검출한다.
    for (let scenario = 0; scenario < 2; scenario += 1) {
        const bytes = pool.physical(other);
        if (scenario) {
            put(bytes, 36, 0);
            bytes[10] = 255;
        }
        const saved = pool.bytes();
        const before = graphHashes(graph);
        assert.ok(throws(() => graph.validatePostPop(root, false, true, { x: 20, y: 20, level: 1, type: types[74] })));
        assert.deepEqual(graphHashes(graph), before);
        assert.deepEqual(pool.bytes(), saved);
        assert.equal(hash.bucket(0, 20, 20), 0);
        assert.ok(spots.every((value) => value === 0));
    }
});

test('비표면 그림의 소수 좌표는 영역 칸만 절삭하며 현재 슬롯/이웃 칸/사전 검사 무변경을 보존한다', () => {
    const types = makeTypes();
    const frames = makeFrames();
    types[74].flags1 = SURFACE;
    types[75].flags2 = 8;
    frames[74] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
    const pool = new SidPool(32768, { server: false });
    const image = pool.allocate(2);
    const inside = pool.allocate(2);
    const outside = pool.allocate(2);
    node(pool, image, 75, [20, 20, 1, 1, 0, 8, 65, 80, 0, 0, 9, 254, 1]);
    node(pool, inside, 74, [20, 20, 1, 1, 2048, 0, 65, 80, 0, 0, 0, 1, 0]);
    node(pool, outside, 74, [21, 20, 1, 1, 2048, 0, 65, 80, 0, 0, 0, 2, 0]);
    put(pool.physical(image), 14, floatToBits(20.75));
    put(pool.physical(image), 18, floatToBits(20.999));
    const hash = new SquidHash();
    hash.setCell(0, 20, 20, inside);
    hash.setCell(0, 21, 20, outside);
    const spots = new Uint8Array(65536);
    spots[20 * 256 + 20] = 8;
    spots[20 * 256 + 21] = 8;
    const records = fullRecords(0, 0, 0);
    Object.assign(records[1], { surfaces: 1, inUse: 1 });
    Object.assign(records[2], { surfaces: 1, inUse: 1 });
    const graph = new RawGraph(pool, hash, spots, types, frames, { records });
    const before = graphHashes(graph);
    const source = hex(pool.slot(image));
    graph.validatePostPop(image, true, false, { x: 20.75, y: 20.999, level: 1, type: types[75] });
    assert.deepEqual(graphHashes(graph), before);
    assert.equal(pool.slot(inside)[30], 1);
    assert.equal(pool.slot(outside)[30], 2);
    graph.invalidateRegion(image);
    assert.equal(pool.slot(inside)[30], 254);
    assert.equal(graph.records()[1].inUse, 0);
    assert.equal(pool.slot(outside)[30], 2);
    assert.equal(graph.records()[2].surfaces, 1);
    assert.equal(hex(pool.slot(image)), source);
});

test('대여 실행기는 같은 물리에 쓰고 명시 records/stack snapshot 생성자는 계속 독립 사본을 소유한다', () => {
    const types = makeTypes();
    const frames = makeFrames();
    const pool = new SidPool(32768, { server: false });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const shared = new RawGraphStorage();
    // 생성자가 sentinel 도 고치지 않는지 비영 입력으로 확인한다.
    shared.records.forEach((record, index) => Object.assign(record, { surfaces: 3, inUse: 0, reserved: 0x1200 + index }));
    shared.stack.forEach((_, index) => { shared.stack[index] = (0x70000000 + index) >>> 0; });
    const initial = sharedBytes(shared);
    const borrowed = new RawGraph(pool, hash, spots, types, frames, { storage: shared });
    assert.equal(sharedBytes(shared), initial);
    assert.equal(borrowed.records(), shared.records);
    assert.equal(borrowed.floodStack(), shared.stack);
    const snapshot = new RawGraph(pool, hash, spots, types, frames, { records: shared.records, stack: shared.stack });
    const snapshotBefore = graphHashes(snapshot);
    assert.notEqual(snapshot.records(), shared.records);
    assert.notEqual(snapshot.floodStack(), shared.stack);
    shared.records[0].inUse = 1;
    assert.equal(borrowed.records()[0].inUse, 1);
    assert.equal(snapshot.records()[0].inUse, 0);
    assert.equal(borrowed.allocate(), 1);
    assert.deepEqual(shared.records[1], { surfaces: 0, inUse: 1, reserved: 0x1201 });
    assert.deepEqual(graphHashes(snapshot), snapshotBefore);
    const afterBorrowed = sharedBytes(shared);
    assert.equal(snapshot.allocate(), 0);
    assert.equal(sharedBytes(shared), afterBorrowed);
});

// postDestroy 시험용 장면: 비표면 원천과 같은 칸의 표면 둘을 같은 해시 체인에 둔 실제 풀이다. (2026-11-08 WB04-E40)
function postDestroyScene(storage) {
    const types = makeTypes();
    const frames = makeFrames();
    types[74].flags1 = SURFACE;
    frames[74] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
    const pool = new SidPool(32768, { server: false });
    const source = pool.allocate(2);
    const first = pool.allocate(2);
    const second = pool.allocate(2);
    node(pool, source, 75, [20, 20, 1, 1, 0, 0, 65, 80, 0, 0, 9, 254, 0]);
    node(pool, first, 74, [20, 20, 1, 1, 2048, 0, 65, 80, 0, 0, 0, 254, 0], second);
    node(pool, second, 74, [20, 20, 1, 1, 2048, 0, 65, 80, 0, 0, 0, 254, 0]);
    const hash = new SquidHash();
    hash.setCell(0, 20, 20, first);
    const spots = new Uint8Array(65536);
    return { pool, hash, spots, types, frames, source, first, second, graph: new RawGraph(pool, hash, spots, types, frames, { storage }) };
}

test('postDestroy 의 임시 사본은 검증 성공/후반 체인 실패 모두 실제 장기 표에 쓰지 않고, 실제 실행은 독립 사본과 같은 결과를 만든다', () => {
    const shared = new RawGraphStorage();
    resetRawGraphTable(shared);
    Object.assign(shared.records[252], { surfaces: 7, inUse: 8, reserved: 0x4567 });
    shared.records[1].reserved = 0x1234;
    shared.stack[4095] = 0xa1b2c3d4;
    const scene = postDestroyScene(shared);
    const original = sharedBytes(shared);
    const raw = scene.pool.bytes();
    const oldSpots = Uint8Array.from(scene.spots);
    scene.graph.validatePostDestroy(scene.source);
    assert.equal(sharedBytes(shared), original);
    assert.deepEqual(scene.pool.bytes(), raw);
    assert.deepEqual(scene.spots, oldSpots);
    assert.equal(scene.hash.cell(0, 20, 20), scene.first);
    // 첫 Add 뒤 다음 finder 방문에서 실패할 실제 체인이다.
    put(scene.pool.physical(scene.second), 4, scene.second, 2);
    const cyclicRaw = scene.pool.bytes();
    assert.ok(throws(() => scene.graph.validatePostDestroy(scene.source)));
    assert.ok(throws(() => scene.graph.postDestroy(scene.source)));
    assert.equal(sharedBytes(shared), original);
    assert.deepEqual(scene.pool.bytes(), cyclicRaw);
    put(scene.pool.physical(scene.second), 4, 0, 2);
    // 같은 장면을 독립 snapshot 으로 다시 만들어 대조 사본으로 쓴다.
    const reference = new RawGraphStorage();
    resetRawGraphTable(reference);
    Object.assign(reference.records[252], { surfaces: 7, inUse: 8, reserved: 0x4567 });
    reference.records[1].reserved = 0x1234;
    reference.stack[4095] = 0xa1b2c3d4;
    const expected = postDestroyScene(reference);
    expected.graph.postDestroy(expected.source);
    scene.graph.postDestroy(scene.source);
    assert.deepEqual(graphHashes(scene.graph), graphHashes(expected.graph));
    assert.deepEqual(scene.pool.bytes(), expected.pool.bytes());
    assert.notEqual(sharedBytes(shared), original);
    assert.equal(scene.graph.records(), shared.records);
    assert.equal(scene.graph.floodStack(), shared.stack);
});

// 영역 통지 시험용 장면: 표면 둘(20,20)(21,20)과 발자국 2칸 비표면 원천이다. (2026-11-08 WB04-E40)
class RegionScene {
    constructor() {
        this.pool = new SidPool(32768, { server: true });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = makeTypes();
        this.frames = makeFrames();
        Object.assign(this.types[73], { footX: 2, footY: 1 });
        Object.assign(this.types[74], { flags1: SURFACE, footX: 1, footY: 1 });
        this.storage = new RawGraphStorage();
        this.checking = 0x80000000;
        resetRawGraphTable(this.storage);
        this.first = this.pool.allocate(2);
        this.second = this.pool.allocate(2);
        this.source = this.pool.allocate(2);
        // 실제 슬롯의 다른 payload 를 일정하게 유지하여 graph BYTE 밖의 변경도 검출한다.
        for (const sid of [this.first, this.second, this.source]) {
            const raw = this.pool.physical(sid);
            raw.fill(0);
            raw[10] = sid === this.source ? 73 : 74;
            put(raw, 14, floatToBits(sid === this.first ? 20 : 21));
            put(raw, 18, floatToBits(20));
        }
        this.pool.physical(this.first)[30] = 3;
        this.pool.physical(this.second)[30] = 4;
        setRecord(this.storage.records, 3, 0, 0, 971);
        setRecord(this.storage.records, 4, 0, 0, 972);
        setRecord(this.storage.records, 5, 3, 1, 973);
        this.hash.setCell(0, 20, 20, this.first);
        this.hash.setCell(0, 21, 20, this.second);
        this.spots[20 * 256 + 20] = 8;
        this.spots[20 * 256 + 21] = 8;
        this.graph = new RawGraph(this.pool, this.hash, this.spots, this.types, this.frames, { storage: this.storage });
    }
}

test('영역 통지는 진단 중 고친 같은 행과 이후 후보의 현재 번호를 읽고 현재 checking 변경도 보존한다', () => {
    const scene = new RegionScene();
    let assertions = 0;
    scene.graph.bindDiagnostics({
        checking: () => scene.checking,
        assertion: (expression, file, line) => {
            assert.equal(expression, 'inUse');
            assert.equal(file, '\\Ns\\O\\Graph.cpp');
            assert.equal(line, 0x59);
            assertions += 1;
            setRecord(scene.storage.records, 3, 2, 1, 991);
            scene.pool.physical(scene.second)[30] = 5;
            scene.checking = 0;
        },
    });
    const before = scene.pool.bytes();
    scene.graph.validatePostPop(scene.source, true, false);
    assert.equal(assertions, 0);
    assert.deepEqual(scene.pool.bytes(), before);
    assert.equal(scene.storage.records[3].surfaces, 0);
    scene.graph.postPop(scene.source, true, false);
    assert.equal(assertions, 1);
    assert.equal(scene.checking, 0);
    assert.deepEqual(scene.storage.records[3], { surfaces: 1, inUse: 1, reserved: 991 });
    assert.deepEqual(scene.storage.records[4], { surfaces: 0, inUse: 0, reserved: 972 });
    assert.deepEqual(scene.storage.records[5], { surfaces: 2, inUse: 1, reserved: 973 });
    assert.equal(scene.pool.slot(scene.first)[30], 254);
    assert.equal(scene.pool.slot(scene.second)[30], 254);
    assert.equal(scene.storage.records[254].surfaces, 0x7dfd);
    assert.ok(scene.storage.stack.every((word) => word === 0));
});

test('선택된 진단 장치 누락은 현재 첫 행/graph BYTE 쓰기 전에 멈추며 Validate 는 진단 장치를 요구하지 않는다', () => {
    const scene = new RegionScene();
    scene.graph.bindDiagnostics({ checking: () => scene.checking });
    const before = scene.pool.bytes();
    scene.graph.validatePostPop(scene.source, true, false);
    assert.throws(() => scene.graph.invalidateRegion(scene.source), /진단 미연결/);
    assert.deepEqual(scene.pool.bytes(), before);
    assert.equal(scene.storage.records[3].surfaces, 0);
    assert.equal(scene.storage.records[3].inUse, 0);
});

test('공간/타입 연결 검증과 Free 번호 범위, 프레임 조회는 원문 계약을 지킨다', () => {
    const scene = new RegionScene();
    scene.graph.validateSpace(scene.hash, scene.spots);
    assert.throws(() => scene.graph.validateSpace(new SquidHash(), scene.spots));
    assert.throws(() => scene.graph.validateSpace(scene.hash, Uint8Array.from(scene.spots)));
    scene.graph.validateTypes(scene.types);
    const changed = makeTypes();
    changed[10].footX = 4;
    assert.throws(() => scene.graph.validateTypes(changed));
    assert.throws(() => scene.graph.validateTypes(changed.slice(1)));
    scene.graph.validateFree(0);
    scene.graph.validateFree(254);
    for (const bad of [251, 253, 255]) assert.throws(() => scene.graph.validateFree(bad), RangeError);
    // 254 는 sentinel 도 읽지 않고 반환하며, 유효 번호는 앞 두 WORD 만 지운다.
    scene.storage.records[254].surfaces = 0;
    scene.graph.free(254);
    scene.graph.free(5);
    assert.deepEqual(scene.storage.records[5], { surfaces: 0, inUse: 0, reserved: 973 });
    // 프레임 조회는 타입/프레임 표를 캐시하지 않고 범위 밖을 거부한다.
    scene.frames = null;
    const framed = makeFrames();
    framed[74] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
    const graph = new RawGraph(scene.pool, scene.hash, scene.spots, scene.types, framed);
    assert.deepEqual(graph.frame(scene.first), { side: 65, variant: 80, number: 1, flags: 0 });
    put(scene.pool.physical(scene.first), 36, 1, 4);
    assert.throws(() => graph.frame(scene.first), RangeError);
    // 잘못된 크기 입력은 생성 단계에서 거부한다.
    assert.throws(() => new RawGraph(scene.pool, scene.hash, scene.spots, scene.types.slice(1), framed), RangeError);
    assert.throws(() => new RawGraph(scene.pool, scene.hash, scene.spots, scene.types, framed, { records: [{ surfaces: 0, inUse: 0, reserved: 0 }] }), RangeError);
});

test('rawPathSurfaceAt: 0.9999 보정 칸의 해시 0단계 머리를 읽고 지도 밖은 0 이다', () => {
    const hash = new SquidHash();
    hash.setCell(0, 20, 20, 7);
    hash.setCell(0, 0, 5, 9);
    assert.equal(rawPathSurfaceAt(hash, 20, 20), 7);
    assert.equal(rawPathSurfaceAt(hash, 19.5, 19.9), 7);
    assert.equal(rawPathSurfaceAt(hash, 19.001, 20), 7);
    assert.equal(rawPathSurfaceAt(hash, 19, 20), 0);
    assert.equal(rawPathSurfaceAt(hash, -0.0001, 5), 9);
    assert.equal(rawPathSurfaceAt(hash, -2, 5), 0);
    assert.equal(rawPathSurfaceAt(hash, 256, 0), 0);
    assert.throws(() => rawPathSurfaceAt(hash, Number.NaN, 0), RangeError);
});

