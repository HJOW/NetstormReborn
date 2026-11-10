// 10.78 Objects stream 적재(cpppj RawFortObjectStream = RawFortObjectLoad::ObjectStream/ChunkStream/SectionStream/LoadStream, 원본 004bdc60..004be610)의 웹 대응 `core/raw-fort-object-stream.js` 를 검사한다(4-B, WB04-E276).
//  - 같은 고정 관측 `fortobjectload-x86.tsv` 의 원본 609행을 값형 경로(fort-object-load.test.mjs)가 아니라 **원시 stream 경로**로 다시 재생한다: 객체 573행은 저장 BYTE 를 stream 에 두고 objectStream 으로,
//    전체 순회 36행은 20 Terr 와 Chaff 의 저장 입력을 loadStream 으로 실행해 효과 순서 · 전체 슬롯 · 난수 · 전역 상태를 같은 기대값과 대조한다. 효과 장치와 입력은 fort-load-scene.mjs 의 장면 도우미를 값형 경로와 공유한다.
//  - 손 시험(cpppj FortObjectStream_*): 잘린 둘째 객체는 앞선 객체와 증가한 cursor 를 남기고 실패하며, 부모 순회 실패는 깊이만 복구하고 변환표를 반납하지 않는다.
// 내용물(contents)은 raw-fort-contents-load.js(E275)로 읽는다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E276)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { FortObjectLoad } from '../www/core/fort-object-load.js';
import { loadFortContents } from '../www/core/raw-fort-contents-load.js';
import { chunkStream, loadStream, objectStream, sectionStream } from '../www/core/raw-fort-object-stream.js';
import { F, harness } from './fort-load-scene.mjs';

// 소문자 16진수 문자열을 바이트 배열로 바꾼다. (2026-11-10 WB04-E276)
function hexBytes(hex) {
    return Uint8Array.from(hex.match(/../g) ?? [], (pair) => Number.parseInt(pair, 16));
}

// 비정렬 DWORD 쓰기/읽기. (2026-11-10 WB04-E276)
function putDword(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 255;
}
function getDword(bytes, at) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 독립 MemStream 의 물리 16 바이트와 입력 BYTE 를 함께 소유한다: data 토큰 1 · pos 0 · end/alloc = 바이트 수. (2026-11-10 WB04-E276)
class ObjectStreamMemory {
    constructor(bytes) {
        this.fields = new Uint8Array(16);
        this.bytes = bytes;
        putDword(this.fields, 0, 1);
        putDword(this.fields, 8, bytes.length);
        putDword(this.fields, 12, bytes.length);
    }

    // 매 읽기에서 현재 fields/버퍼를 빌린다. (2026-11-10 WB04-E276)
    access(operation) {
        operation(this.fields, { buffer: (token) => { assert.equal(token, 1); return this.bytes; } });
    }
}

// 같은 장면의 실제 효과와 직접 Contents 읽기를 조립한 stream 장치이다. conversion 은 { value } 상자(현재 005c89bc 대응)이다. (2026-11-10 WB04-E276)
function streamDevices(scene, memory, conversion, table) {
    const effects = scene.hooks;
    const access = (token, operation) => {
        if (!token || token > memory.length) throw new RangeError('원문 Objects 검사 stream 범위');
        memory[token - 1].access(operation);
    };
    return {
        access,
        manager: () => 0x1111,
        select: (manager, name, writable) => {
            assert.equal(manager, 0x1111);
            assert.equal(writable, 1);
            const index = name === 'Chaff' ? 20 : Number(name.slice(4));
            assert.ok(index < memory.length);
            return index + 1;
        },
        // 기존 Native fixture 도 004bd800 을 명시 외부 경계로 두었다. 새 실제 준비 완료로 세지 않는다.
        prepareTypes: () => {},
        conversion: () => conversion.value,
        checking: () => 0,
        conversionBuffer: (token) => { assert.equal(token, 1); return table; },
        releaseConversion: () => { conversion.value = 0; },
        assertion: (line) => { throw new Error(`Objects 원문 검사 예상 밖 진단:${line}`); },
        // 같은 stream 위치에서 실제 004bd130(parent, stream, 9, 0)을 실행한다.
        contents: (parent, stream) => {
            const reader = {
                access,
                conversionBuffer: (token) => { assert.equal(token, 1); return table; },
                type: (type) => ({ flags1: scene.types[type].flags1, contentListFlags: scene.types[type].contentListFlags }),
                parentListFlags: (sid) => scene.types[scene.pool.slot(sid)[10]].containerListFlags,
                create: (sid, type, quantity, list, flags, quality) => effects.createContent(sid, type, quantity, list, flags, quality),
                assertion: () => { throw new Error('Objects 원문 Contents 예상 밖 진단'); },
            };
            loadFortContents(parent, stream, 9, 0, { get conversion() { return conversion.value; }, checking: () => 0 }, reader);
        },
    };
}

// 원본 fixture 의 'originals' 609행이다. 없는 환경은 원본 대조만 건너뛴다. (2026-11-10 WB04-E276)
const fixturePath = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fortobjectload-x86.tsv');
const rows = fs.existsSync(fixturePath) ? fs.readFileSync(fixturePath, 'utf8').split(/\r?\n/).filter((line) => line.startsWith('originals\t')).map((line) => line.split('\t')) : null;

// 장면 상태 4칸(판본 · 깊이 · scope · 삭제 억제)을 문자열로 만든다. (2026-11-10 WB04-E276)
function stateText(scene) {
    return [scene.state.version, scene.state.loadingDepth, scene.state.scope, Number(scene.state.fortDestroyBlocked)].join(',');
}

// 만들어진 객체 수는 raws() 의 항목 수이다. (2026-11-10 WB04-E276)
function born(scene) {
    const raws = scene.raws();
    return raws === '-' ? 0 : raws.split(';').length;
}

test('Fort Objects stream: 원본 609행(객체 573 · 전체 순회 36)의 효과 순서·전체 슬롯·난수·전역이 stream 경로에서도 같다', (t) => {
    if (!rows) { t.skip('원본 fixture 없음'); return; }
    assert.equal(rows.length, 609);
    let checked = 0;
    for (const row of rows) {
        const v = row[2].split(',').map(Number);
        const scene = harness(v);
        const loader = new FortObjectLoad(scene);
        const conversion = { value: 0 };
        // PE 실행기에 제공한 identity 변환표 입력이다.
        const table = Uint8Array.from({ length: 255 }, (_, index) => index);
        const memory = [];
        if (row[1] === 'O') {
            memory.push(new ObjectStreamMemory(hexBytes(row[3])));
            if (v[F.stored] < 246 && v[F.stored] !== v[F.type]) {
                conversion.value = 1;
                table[v[F.stored]] = v[F.type];
            }
        } else {
            // 20 Terr 와 Chaff 의 저장 입력이며 정책에 따른 소비량은 로더가 정한다.
            for (let index = 0; index < 21; index += 1) {
                let bytes = [0];
                if (index === 0 || index === 1 || index === 20) {
                    bytes = [2];
                    for (let chunk = 0; chunk < 4; chunk += 1) bytes.push(99, 1, 0, 0x21, v[F.stored] & 255, v[F.owner] & 255);
                }
                memory.push(new ObjectStreamMemory(Uint8Array.from(bytes)));
            }
        }
        const devices = streamDevices(scene, memory, conversion, table);
        if (row[1] === 'O') objectStream(loader, 1, v[F.originX] | 0, v[F.originY] | 0, v[F.defaultOwner], devices);
        else loadStream(loader, v[F.slot], devices);
        assert.deepEqual([scene.events.join(';') || '-', scene.raws(), String(scene.random.state), stateText(scene)], row.slice(4), `${row[1]} ${row[2]}`);
        checked += 1;
    }
    assert.equal(checked, 609);
});

test('Fort Objects stream: 잘린 둘째 객체는 앞선 객체와 증가한 cursor 를 남기고 부분 생성 효과를 유지한다', (t) => {
    if (!rows) { t.skip('원본 fixture 없음'); return; }
    const first = rows[0];
    const scene = harness(first[2].split(',').map(Number));
    const loader = new FortObjectLoad(scene);
    const bytes = Uint8Array.from([...hexBytes(first[3]), 0x21, 100]);
    const memory = [new ObjectStreamMemory(bytes)];
    const devices = streamDevices(scene, memory, { value: 0 }, Uint8Array.from({ length: 255 }, (_, index) => index));
    objectStream(loader, 1, 16, 32, 1, devices);
    assert.deepEqual([born(scene), getDword(memory[0].fields, 4)], [1, 3]);
    // 둘째 객체는 stored 100 뒤 필드 BYTE 를 읽다 물리 끝에서 실패하지만 객체 생성과 cursor 는 되돌리지 않는다.
    assert.throws(() => objectStream(loader, 1, 16, 32, 1, devices), RangeError);
    assert.deepEqual([born(scene), getDword(memory[0].fields, 4)], [2, bytes.length]);
    assert.deepEqual([scene.pool.slot(100)[34], scene.pool.slot(101)[10], scene.state.loadingDepth], [1, 100, 3]);
});

test('Fort Objects stream: 부모 순회 실패는 깊이만 복구하고 변환표를 반납하지 않으며 소유자 마감도 하지 않는다', (t) => {
    if (!rows) { t.skip('원본 fixture 없음'); return; }
    const values = rows[0][2].split(',').map(Number);
    values[F.flags2] = 0x40000;
    values[F.mission] = 1;
    values[F.qa] = 0;
    const scene = harness(values);
    scene.state.chunksX = 1;
    scene.state.chunksY = 1;
    let finished = 0;
    scene.hooks.finishOwner = () => { finished += 1; };
    const loader = new FortObjectLoad(scene);
    // 두 번째 owner BYTE 만 없는 실제 저장 입력이다.
    const memory = [new ObjectStreamMemory(Uint8Array.from([2, 99, 2, 0, 0x21, 100, 2, 0x21, 100]))];
    const conversion = { value: 0 };
    const devices = streamDevices(scene, memory, conversion, Uint8Array.from({ length: 255 }, (_, index) => index));
    let released = 0;
    devices.prepareTypes = (slot) => {
        assert.equal(slot, 1);
        assert.equal(scene.state.loadingDepth, 4);
        conversion.value = 1;
    };
    devices.releaseConversion = () => { released += 1; conversion.value = 0; };
    assert.throws(() => loadStream(loader, 1, devices), RangeError);
    assert.deepEqual([born(scene), finished, released, conversion.value, scene.state.loadingDepth, scene.state.scope], [2, 0, 0, 1, 3, 1]);
    assert.deepEqual([getDword(memory[0].fields, 4), scene.pool.slot(100)[34], scene.pool.slot(101)[10]], [9, 2, 100]);
});

// 고정 관측이 건드리지 않는 경계를 손으로 확인한다(변형 확인에서 살아남은 변형을 잡기 위해 추가): 저장 종류 246 은 다리와 owner 9, 변환 결과 0 뒤 추가 BYTE 진단 0x2e8, 큰 섬 진단 0x334,
// 청크 개수는 unsigned WORD, 소유자는 슬롯 1..8 만, 정상 꼬리의 변환표 반납, Terr 이름은 두 자리, 음수 end(0x80000000)에서의 signed 비교.
test('Fort Objects stream: 고정 관측 밖 경계(다리 246 · 종류 0 진단 · 큰 섬 · 개수 WORD · 슬롯 소유자 · 변환표 반납 · Terr 이름 · signed end)', (t) => {
    if (!rows) { t.skip('원본 fixture 없음'); return; }
    const values = rows[0][2].split(',').map(Number);
    const identity = () => Uint8Array.from({ length: 255 }, (_, index) => index);
    // 진단 번호를 기록하고 계속하는 장치를 만든다.
    const lenient = (scene, bytes, conversion = { value: 0 }, table = identity()) => {
        const memory = [new ObjectStreamMemory(Uint8Array.from(bytes))];
        const devices = streamDevices(scene, memory, conversion, table);
        const codes = [];
        devices.assertion = (line) => { codes.push(line); };
        return { memory, devices, codes };
    };
    // 저장 종류 246(0xf6)은 현재 변환표를 거치지 않고 다리(82)와 owner 255-246=9 이다.
    const bridge = harness(values);
    bridge.state.version = 1;
    const bridgeInput = lenient(bridge, [0, 0xf6]);
    objectStream(new FortObjectLoad(bridge), 1, 0, 0, 3, bridgeInput.devices);
    assert.ok(bridge.events.includes('C:82:0:100') && bridge.events.includes('O:100:9'), bridge.events.join(';'));
    // 변환표가 종류를 0 으로 바꾸면 객체를 만들지 않고, 추가 BYTE 가 0 이 아니면 진단 0x2e8 뒤 계속한다.
    const empty = harness(values);
    const table = identity();
    table[5] = 0;
    const emptyInput = lenient(empty, [0, 5, 7], { value: 1 }, table);
    assert.equal(objectStream(new FortObjectLoad(empty), 1, 0, 0, 1, emptyInput.devices), 0);
    assert.deepEqual([emptyInput.codes, empty.raws(), getDword(emptyInput.memory[0].fields, 4)], [[0x2e8], '-', 3]);
    // 현재 타입이 큰 섬이면 진단 0x334 를 내고 계속 만든다.
    const large = harness(values);
    large.state.version = 1;
    large.state.largeIslandType = 100;
    const largeInput = lenient(large, [0, 100]);
    objectStream(new FortObjectLoad(large), 1, 0, 0, 1, largeInput.devices);
    assert.deepEqual([largeInput.codes, born(large)], [[0x334], 1]);
    // 개수 WORD 는 unsigned 이다: 0xffff 개를 읽으려다 물리 끝에서 거부된다(signed 면 0개).
    const count = harness(values);
    assert.throws(() => chunkStream(new FortObjectLoad(count), 1, 0, 0, 1, lenient(count, [99, 0xff, 0xff]).devices), RangeError);
    // 0x8000 개(종류 0 인 3 바이트 객체)는 signed 로 읽으면 음수 개수가 되어 물리 끝까지 읽으려다 거부되지만 unsigned 이면 정확히 끝난다.
    const many = harness(values);
    const manyInput = lenient(many, [99, 0x00, 0x80, ...new Array(3 * 0x8000).fill(0)]);
    chunkStream(new FortObjectLoad(many), 1, 0, 0, 1, manyInput.devices);
    assert.deepEqual([getDword(manyInput.memory[0].fields, 4), born(many)], [3 + 3 * 0x8000, 0]);
    // 소유자는 슬롯 1..8 만이고, 정상 꼬리에서 변환표를 한 번 반납하고, Terr 이름은 두 자리이며 Chaff 는 Fort/Chaff 모드에서만 읽는다.
    for (const [slot, expected, chaffMode] of [[5, 5, false], [9, 0, false], [0, 0, true]]) {
        const scene = harness(values);
        scene.state.chaffMode = chaffMode;
        const owners = [];
        scene.hooks.finishOwner = (owner) => owners.push(owner);
        const conversion = { value: 1 };
        const memory = Array.from({ length: 21 }, () => new ObjectStreamMemory(Uint8Array.of(0)));
        const devices = streamDevices(scene, memory, conversion, identity());
        const names = [];
        const select = devices.select;
        devices.select = (manager, name, writable) => { names.push(name); return select(manager, name, writable); };
        loadStream(new FortObjectLoad(scene), slot, devices);
        assert.deepEqual([owners, conversion.value], [[expected], 0], `slot ${slot}`);
        assert.deepEqual(names, [...Array.from({ length: 20 }, (_, index) => `Terr${String(index).padStart(2, '0')}`), ...(chaffMode ? ['Chaff'] : [])]);
    }
    // 끝(end) 필드가 음수(0x80000000)이면 signed 비교로 남은 자료가 없다고 보아 구간을 소비하지 않는다.
    const negative = harness(values);
    const negativeInput = lenient(negative, [2, 99, 1, 0, 0x21, 100, 2]);
    putDword(negativeInput.memory[0].fields, 8, 0x80000000);
    const streams = [negativeInput.memory[0], ...Array.from({ length: 20 }, () => new ObjectStreamMemory(Uint8Array.of(0)))];
    loadStream(new FortObjectLoad(negative), 1, streamDevices(negative, streams, { value: 0 }, identity()));
    assert.deepEqual([negative.state.scope, born(negative)], [0, 0]);
    // 같은 음수 end 에서 WORD 읽기는 signed 로 pos+2 > end 라 진단 0x2e 를 낸다.
    const word = harness(values);
    const wordInput = lenient(word, [99, 0, 0]);
    putDword(wordInput.memory[0].fields, 8, 0x80000000);
    chunkStream(new FortObjectLoad(word), 1, 0, 0, 1, wordInput.devices);
    assert.deepEqual(wordInput.codes, [0x2e]);
});

test('Fort Objects stream: 장치 누락·판본·지도 크기·진단 복귀 경계', (t) => {
    if (!rows) { t.skip('원본 fixture 없음'); return; }
    const scene = harness(rows[0][2].split(',').map(Number));
    const loader = new FortObjectLoad(scene);
    const memory = [new ObjectStreamMemory(Uint8Array.from([99, 0, 0]))];
    const devices = streamDevices(scene, memory, { value: 0 }, new Uint8Array(255));
    // 개수 0 인 청크는 표지와 WORD 만 소비한다. 표지가 99 가 아니면 진단 0x37a 를 내고 계속한다.
    chunkStream(loader, 1, 0, 0, 1, devices);
    assert.equal(getDword(memory[0].fields, 4), 3);
    const diagnosed = [];
    const wrong = [new ObjectStreamMemory(Uint8Array.from([7, 0, 0]))];
    const lenient = streamDevices(scene, wrong, { value: 0 }, new Uint8Array(255));
    lenient.assertion = (line) => diagnosed.push(line);
    chunkStream(loader, 1, 0, 0, 1, lenient);
    assert.deepEqual(diagnosed, [0x37a]);
    // 지도가 비어 있으면 stream 을 소비하지 않는다.
    scene.state.chunksX = 0;
    const untouched = [new ObjectStreamMemory(Uint8Array.from([99, 1, 0]))];
    sectionStream(loader, 1, 1, -1, streamDevices(scene, untouched, { value: 0 }, new Uint8Array(255)));
    assert.equal(getDword(untouched[0].fields, 4), 0);
    assert.equal(scene.state.scope, 2);
    // 필수 장치가 없으면 도달한 첫 읽기에서 거부한다.
    assert.throws(() => objectStream(loader, 1, 0, 0, 1, {}), /실제 장치 미연결/);
    // 10.78 전용이다.
    assert.throws(() => objectStream({ ...loader, pool: { ...scene.pool, edition: 'cd1072' } }, 1, 0, 0, 1, devices), TypeError);
});
