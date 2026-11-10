// 원본 004bd130 원시 stream 내용물 읽기(cpppj RawFortContentsLoad)의 웹 대응 `core/raw-fort-contents-load.js` 를 검사한다(4-B, WB04-E275).
//  - 고정 관측 `fort-contents-load-x86.tsv` 668행: 개수/종류/QA·QB/listFlags/중첩 입력을 두 stream 의 실제 16 바이트와 2048 바이트 물리 버퍼에 두고, 현재 변환표 토큰(11/22)과 checking·TYPE·Create 를
//    명시 외부 응답으로 공급해 결과(반환·깊이·변환 토큰·checking·Create 수·부모 flags)·두 stream 의 8 DWORD·하위 호출 순서를 대조한다. 변이(mutation)는 호출 경계에서 적용하는 원본 관측의 명시 효과이다
//    (stop = 호출 중단, conversionN = 변환 토큰 교체, dataB/posN/endN = stream 필드 교체, flags0/flagsQB/listN = 현재 TYPE 교체, parent4/flags4 = 부모 flags, checking0, depth7, return0 = Create 반환 0).
//  - 손 시험(cpppj FortContentsLoad_SelectedDevicesAndPhysicalBounds): 빈 data 는 장치 없이 false, 선택된 Create 누락은 앞선 소비 뒤 실패, 논리 end 와 별개로 물리 끝에서만 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E275)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';
import { loadFortContents } from '../www/core/raw-fort-contents-load.js';

// 저장소 안의 고정 관측 파일이다. (2026-11-10 WB04-E275)
const FIXTURE = resolve(dirname(fileURLToPath(import.meta.url)), '../../cpppj/tests/fixtures/fort-contents-load-x86.tsv');

// 비정렬 DWORD 읽기/쓰기. (2026-11-10 WB04-E275)
function get(bytes, at) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}
function put(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 255;
}

// 원본 관측기에도 공급한 count/type/QA/QB/listFlags/중첩 원시 입력이다(변환 결과나 Create 기대는 포함하지 않는다). (2026-11-10 WB04-E275)
function program(profile) {
    const inputs = [[1, 1, 0x11], [0], [3, 2, 254, 3, 128, 255, 1, 4, 85, 52, 128, 16], [3, 1, 16, 1, 1, 1, 0], [2, 5, 16, 2, 1, 0, 2, 7, 1, 4],
        [2, 1, 1, 0], [2, 5, 1, 2, 1, 1, 0, 1, 4], [3, 245, 0, 246, 0, 255, 0], [], [1, 3, 128, 255, 0]];
    if (profile !== 8) return inputs[profile];
    const bytes = [255];
    for (let index = 0; index < 255; index += 1) bytes.push(1, 0);
    return bytes;
}

// 호출 중단과 실제 변환 주소 0 물리 실패를 정상 false 와 구별하는 검사 표식이다. (2026-11-10 WB04-E275)
class ContentsStop extends Error {}
class ContentsFault extends Error {}

// 외부 장치(두 stream · 버퍼 · 변환표 · TYPE)와 호출 기록을 입력 열만으로 준비한다. (2026-11-10 WB04-E275)
class ContentsDevices {
    constructor(fields) {
        // fixture 의 앞 10 개 원시 DWORD 이다.
        this.input = fields.slice(0, 10).map(Number);
        this.mutation = fields[10];
        this.conversion = this.input[1];
        this.checking = this.input[2];
        this.parentFlags = this.input[8];
        this.depth = 3;
        this.typeCalls = 0;
        this.parentCalls = 0;
        this.creates = 0;
        this.assertions = 0;
        this.currentType = 0;
        this.lastConversionType = 0;
        this.zeroReturn = false;
        this.trace = '';
        this.streams = [new Uint8Array(16), new Uint8Array(16)];
        this.buffers = [new Uint8Array(2048), new Uint8Array(2048)];
        this.tables = [new Uint8Array(255), new Uint8Array(255)];
        // 현재 TYPE 두 필드만 명시 공급한다.
        this.types = Array.from({ length: 256 }, () => ({ flags1: 0, contentListFlags: 0 }));
        this.types[2] = { flags1: 0x1000, contentListFlags: 0x104 };
        this.types[3] = { flags1: 0x2000, contentListFlags: 0 };
        this.types[4] = { flags1: 0x3000, contentListFlags: 0 };
        this.types[5] = { flags1: 0x20000, contentListFlags: 0 };
        // 독립 원시 변환 입력: 토큰 11 은 245→2, 토큰 22 는 245→0 이고 22 는 0..4 를 1,2,3,0,1 로 바꾼다.
        for (let index = 0; index < this.tables.length; index += 1) {
            for (let type = 0; type < 255; type += 1) this.tables[index][type] = type;
            this.tables[index][245] = index ? 0 : 2;
        }
        this.tables[1].set([1, 2, 3, 0, 1], 0);
        const bytes = program(this.input[0]);
        // 각 반환 stream 에 다른 물리 버퍼를 공급하고 callback pos 에도 독립 원문(512)을 둔다.
        for (let stream = 0; stream < this.streams.length; stream += 1) {
            put(this.streams[stream], 0, this.input[4] ? stream + 1 : 0);
            put(this.streams[stream], 4, this.input[6]);
            put(this.streams[stream], 8, this.input[5]);
            put(this.streams[stream], 12, 2048);
            this.buffers[stream].fill(stream ? 0xdd : 0xcc);
            for (const position of [0, 512]) this.buffers[stream].set(bytes, position);
        }
    }

    // 실제 하위 진입의 인자와 순서를 기록한다.
    record(text) {
        if (this.trace) this.trace += ';';
        this.trace += text;
    }

    // 명시 외부 효과만 적용하며 Contents 의 후속 분기/반환은 대신 계산하지 않는다.
    mutate(boundary) {
        const separator = this.mutation.indexOf('-');
        if (separator < 0 || this.mutation.slice(0, separator) !== boundary) return;
        const action = this.mutation.slice(separator + 1);
        const fields = this.streams[this.input[9]];
        if (action === 'stop') throw new ContentsStop();
        if (action.startsWith('conversion')) this.conversion = Number(action.slice(10));
        else if (action === 'dataB') put(fields, 0, 2);
        else if (action.startsWith('pos')) put(fields, 4, Number(action.slice(3)));
        else if (action.startsWith('end')) put(fields, 8, Number.parseInt(action.slice(3), 16));
        else if (action === 'flags0') this.types[this.currentType].flags1 = 0;
        else if (action === 'flagsQB') this.types[this.currentType].flags1 = 0x2000;
        else if (action.startsWith('list')) this.types[this.currentType].contentListFlags = Number(action.slice(4));
        else if (action === 'parent4' || action === 'flags4') this.parentFlags = 4;
        else if (action === 'checking0') this.checking = 0;
        else if (action === 'depth7') this.depth = 7;
        else if (action === 'return0') this.zeroReturn = true;
        else throw new Error('Contents검사효과미해석');
    }

    // 실제 직접 읽기와 같은 저장소를 연결하고 TYPE/Create/진단만 명시 외부 응답으로 둔다.
    hooks() {
        return {
            access: (token, operation) => {
                if (token < 1 || token > 2) throw new RangeError('Contents검사stream물리토큰');
                operation(this.streams[token - 1], {
                    buffer: (data) => {
                        if (data < 1 || data > 2) throw new RangeError('Contents검사data물리토큰');
                        return this.buffers[data - 1];
                    },
                });
            },
            conversionBuffer: (token) => {
                if (!token) {
                    this.record(`F:${this.lastConversionType}:1`);
                    throw new ContentsFault();
                }
                if (token !== 11 && token !== 22) throw new RangeError('Contents검사conversion물리토큰');
                const fields = this.streams[this.input[9]];
                const data = get(fields, 0);
                const position = get(fields, 4);
                // 변환 진단 전 처음 인덱스만 관측한다.
                if (position && position <= 2048 && data >= 1 && data <= 2) this.lastConversionType = this.buffers[data - 1][position - 1];
                return this.tables[token === 11 ? 0 : 1];
            },
            type: (type) => {
                if (type >= this.types.length) throw new RangeError('Contents검사TYPE물리범위');
                this.currentType = type;
                this.record(`T:${type}`);
                this.typeCalls += 1;
                this.mutate(`type${this.typeCalls}`);
                return this.types[type];
            },
            parentListFlags: (parent) => {
                this.record(`P:${parent}`);
                this.parentCalls += 1;
                this.mutate(`parent${this.parentCalls}`);
                return this.parentFlags;
            },
            create: (parent, type, quantity, flags, attach, quality) => {
                this.record(`C:${parent}:${type}:${quantity}:${flags}:${attach}:${quality}`);
                this.zeroReturn = false;
                this.creates += 1;
                this.mutate(`create${this.creates}`);
                return this.zeroReturn ? 0 : 20 + this.creates;
            },
            assertion: (line) => {
                this.record(`A:${line}`);
                this.assertions += 1;
                this.mutate(`assert${this.assertions}`);
            },
        };
    }

    // 현재 변환 토큰을 읽을 때마다 같은 장치를 보게 하는 전역이다(참조 의미).
    globals(checking = () => this.checking) {
        const device = this;
        return { get conversion() { return device.conversion; }, checking };
    }
}

test('Fort 내용물 원시 읽기: 최초 PE 668행의 cursor·Create·현재 변환·재귀·조기 반환이 같다', () => {
    const rows = readFileSync(FIXTURE, 'utf8').split(/\r?\n/).filter((line) => line !== '' && line[0] !== '#').map((line) => line.split('\t'));
    assert.equal(rows.length, 668);
    const statuses = new Map();
    for (const row of rows) {
        assert.equal(row.length, 6);
        const d = new ContentsDevices(row[1].split(','));
        const hooks = d.hooks();
        let status = 'normal';
        let result = false;
        try {
            result = loadFortContents(10, d.input[9] + 1, d.input[7], d.input[3], d.globals(), hooks);
        } catch (error) {
            if (error instanceof ContentsStop) status = 'stop';
            else if (error instanceof ContentsFault) status = 'fault';
            else throw error;
        }
        assert.equal(status, row[2], row[1]);
        const state = row[3].split(',').map(Number);
        assert.deepEqual([Number(result), d.depth, d.conversion, d.checking, d.creates, d.parentFlags], state, row[1]);
        const streamState = row[4].split(',').map(Number);
        const actual = [];
        for (let stream = 0; stream < 2; stream += 1) for (let field = 0; field < 4; field += 1) actual.push(get(d.streams[stream], field * 4));
        assert.deepEqual(actual, streamState, row[1]);
        assert.equal(d.trace || '-', row[5], row[1]);
        statuses.set(status, (statuses.get(status) ?? 0) + 1);
    }
    assert.deepEqual([...statuses].sort(), [['fault', 2], ['normal', 622], ['stop', 44]]);
});

test('Fort 내용물 원시 읽기: 빈 data 는 장치 없이 false, 선택 장치 누락은 앞선 소비 뒤 실패, 물리 끝에서만 거부한다', () => {
    // 빈 data: 어떤 하위 장치도 요구하지 않고 false 이며 pos 는 그대로이다.
    const empty = new ContentsDevices(['0', '11', '1', '0', '0', '1', '0', '9', '0', '0', 'none']);
    assert.equal(loadFortContents(10, 1, 9, 0, { get conversion() { return empty.conversion; } }, { access: empty.hooks().access }), false);
    assert.equal(get(empty.streams[0], 4), 0);
    // Create 누락: 개수·종류·listFlags 를 소비(pos 3)한 뒤 실패하고 Create 는 호출되지 않는다.
    const missing = new ContentsDevices(['0', '11', '0', '1', '1', '1', '0', '9', '0', '0', 'none']);
    const selected = missing.hooks();
    delete selected.create;
    assert.throws(() => loadFortContents(10, 1, 9, 1, { get conversion() { return missing.conversion; } }, selected), /미연결/);
    assert.deepEqual([get(missing.streams[0], 4), missing.creates], [3, 0]);
    // 논리 end 1 로 읽기를 생략하지 않고 물리 끝(2048)에서만 거부한다.
    const physical = new ContentsDevices(['0', '11', '0', '1', '1', '1', '0', '9', '0', '0', 'none']);
    put(physical.streams[0], 4, 2048);
    assert.throws(() => loadFortContents(10, 1, 9, 1, { get conversion() { return physical.conversion; } }, physical.hooks()), RangeError);
    assert.equal(get(physical.streams[0], 4), 2048);
});
