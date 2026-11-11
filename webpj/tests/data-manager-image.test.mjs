// DataManager 파일 이미지 읽기(cpppj `o/RawDataManagerImage`, 원본 0044cdb0 mark · 0044d380 섹션 본문 · 004bce10 서명 · 0044d980 파일 부모)의 웹 대응 `core/raw-data-manager-image.js` 와 client 전체 읽기 어댑터
// `core/client-data-manager-file-read.js` 를 검사한다(4-B, WB04-E284).
//  - 고정 관측 `datamanager-read-x86.tsv` 144행: 전체 1400BYTE(manager 12 + MarkStream20 + 노드 6개 + 버퍼 7개 + 입력 128BYTE 둘 + 이름 표 64BYTE, `~값x개수~` 압축)에서 섹션 본문(ReadDataManagerImage)을 변이 3종으로 실행해
//    매 외부 효과(이름 표 조회 T · 비교 Q · 노드 할당 S · 버퍼 할당 A · 반납 F/N) 직전 전체 메모리와 최종 메모리를 대조한다.
//  - 손 시험(cpppj DataManagerRead_MarkAssertionUsesCurrentFields · InitialPositionAndRequiredNames · DataManagerFile_SignatureBytesAndSignedTypeLimit · SignatureFailureAndCurrentRereads ·
//    ReadParentBranchesAndCleanupCurrent · BodyAndReleaseFailureOnlyOnce): mark 진단 뒤 현재 값 사용, 시작 pos 진단과 필수 이름 표, 서명 두 BYTE·signed 타입 수·FE 정확 비교와 보호 영역, 서명 실패·진단 뒤 재조회,
//    파일 부모 네 분기와 현재 포인터 반납 한 번.
//  - client 어댑터: 웹은 BaseFile 전체 읽기(`RawBaseFileWholeRead`, adapt)를 `GameFiles.read` 로 대체하므로 최소 4BYTE 할당과 논리 길이를 분리한 소유 블록만 검사한다(DataManagerLiveRead 의 BaseFile 사건 로그 64행은 웹에 모델이 없다).
// CampaignTemplates_* 시험은 CampaignTemplateStorage(4-D)가 이 모듈들을 쓸 때 옮긴다. 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E284)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';
import { compactMemStream } from '../www/core/raw-mem-stream.js';
import { compareDataManagerSectionNames } from '../www/core/raw-data-manager-selection.js';
import {
    DATA_MANAGER_IMAGE_ASSERTION, advanceDataManagerMark, readDataManagerFile, readDataManagerImage, readTemplateFileSignature,
} from '../www/core/raw-data-manager-image.js';
import { bindDataManagerFileRead, readDataManagerFileImage } from '../www/core/client-data-manager-file-read.js';
import { GameFiles } from '../www/assets/game-files.js';

// 저장소 안의 고정 관측 파일이다. (2026-11-11 WB04-E284)
const FIXTURE = resolve(dirname(fileURLToPath(import.meta.url)), '../../cpppj/tests/fixtures/datamanager-read-x86.tsv');

// 비정렬 DWORD 읽기/쓰기이다. (2026-11-11 WB04-E284)
const get = (bytes, at) => (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
function put(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 255;
}

// 소문자 16진수 표기이다. (2026-11-11 WB04-E284)
const bytesHex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
const textHex = (text) => bytesHex(Uint8Array.from(text, (char) => char.charCodeAt(0)));

// 반복 표기 `~값x개수~` 까지 모든 바이트를 복원한다. (2026-11-11 WB04-E284)
function readBytes(text) {
    const out = [];
    let at = 0;
    // 일반 16진 두 글자 또는 반복 표기를 원래 순서대로 펼친다.
    while (at < text.length) {
        if (text[at] !== '~') {
            out.push(Number.parseInt(text.slice(at, at + 2), 16));
            at += 2;
            continue;
        }
        const end = text.indexOf('~', at + 1);
        const separator = text.indexOf('x', at + 1);
        if (end < 0 || separator < 0 || separator >= end) throw new Error('파일 읽기 원문의 반복 표기 누락');
        const value = Number.parseInt(text.slice(at + 1, separator), 16);
        const count = Number.parseInt(text.slice(separator + 1, end), 10);
        for (let index = 0; index < count; index += 1) out.push(value);
        at = end + 1;
    }
    return Uint8Array.from(out);
}

// 예외가 나는지 본다. (2026-11-11 WB04-E284)
function throws(action, type = Error) {
    try {
        action();
        return false;
    } catch (error) {
        return error instanceof type;
    }
}

// 섹션 본문 144행: 매 경계 직전 전체 1400BYTE 와 최종 메모리가 같다. 기대값은 fixture 에서만 읽는다. (2026-11-11 WB04-E284)
test('DataManager 파일 이미지 본문: 원본 10.78 144입력의 매 경계 직전 전체 1400BYTE·최종 메모리가 같다', () => {
    const NODES = 0x12100100;
    const BUFFERS = 0x12200100;
    const INPUTS = 0x12201000;
    const NODE_START = 64;
    const BUFFER_START = 520;
    const INPUT_START = 1080;
    const NAMES_START = 1336;
    const rows = readFileSync(FIXTURE, 'utf8').split(/\r?\n/).filter((line) => line !== '' && line[0] !== '#').map((line) => line.split('\t'));
    assert.equal(rows.length, 144);
    // 초기화와 변이는 원본 입력만 사용하며 행 사이에 결과를 넘기지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 4);
        const input = row[0].split(',');
        assert.equal(input.length, 7);
        const variant = Number(input[6]);
        const bytes = readBytes(row[1]);
        assert.equal(bytes.length, 1400);
        const manager = bytes.subarray(8, 20);
        const source = bytes.subarray(36, 56);
        let created = 0;
        let allocating = 0;
        let eventIndex = 0;
        const events = row[3].split(';');
        // 같은 60BYTE 노드다.
        const node = (pointer) => {
            const offset = pointer - NODES - 8;
            if (offset % 76 || offset < 0 || offset / 76 >= 6) throw new RangeError('파일 읽기 원문의 노드 범위');
            return bytes.subarray(NODE_START + (offset / 76) * 76 + 8, NODE_START + (offset / 76) * 76 + 8 + 60);
        };
        // 실제 입력 원문/대체 입력/할당은 서로 다른 같은 배열이다.
        const buffer = (pointer) => {
            if (pointer === INPUTS || pointer === INPUTS + 128) return bytes.subarray(INPUT_START + pointer - INPUTS, INPUT_START + pointer - INPUTS + 128);
            const offset = pointer - BUFFERS - 8;
            if (offset % 80 || offset < 0 || offset / 80 >= 7) throw new RangeError('파일 읽기 원문의 버퍼 범위');
            return bytes.subarray(BUFFER_START + (offset / 80) * 80 + 8, BUFFER_START + (offset / 80) * 80 + 8 + 64);
        };
        // 외부 효과 직전 전체 상태를 즉시 최초 원문과 비교한다.
        const event = (kind, value) => {
            assert.ok(eventIndex < events.length, `파일 읽기 원문에 없는 경계 효과 ${kind},${value}`);
            const expected = events[eventIndex].split(',');
            eventIndex += 1;
            assert.equal(expected.length, 3);
            assert.equal(expected[0], kind);
            assert.equal(expected[1], value);
            assert.ok(Buffer.from(bytes).equals(readBytes(expected[2])), `경계 ${kind},${value}: 전체 메모리 불일치`);
        };
        const hooks = { streams: { selection: {}, list: { lifetime: {} }, memory: {} } };
        hooks.names = () => {
            event('T', 'names');
            return bytes.subarray(NAMES_START, NAMES_START + 64);
        };
        hooks.streams.selection.stream = node;
        hooks.streams.selection.compare = (left, right) => {
            event('Q', `${textHex(left)}/${textHex(right)}`);
            return compareDataManagerSectionNames(left, right);
        };
        hooks.streams.allocateStream = (size) => {
            event('S', String(size));
            assert.ok(size === 60 && created < 3);
            allocating = NODES + (3 + created) * 76 + 8;
            created += 1;
            return allocating;
        };
        hooks.streams.memory.buffer = buffer;
        hooks.streams.memory.allocate = (size) => {
            event('A', String(size));
            const index = (allocating - NODES - 8) / 76;
            assert.ok(size <= 64);
            if (variant === 1) put(source, 0, INPUTS + 128);
            return BUFFERS + (index >= 3 ? index : 6) * 80 + 8;
        };
        // free 뒤 current 변이를 원본 후속 쓰기와 구분한다.
        const release = (kind, pointer) => {
            event(kind, String(pointer));
            if (variant === 2) put(manager, 8, 0xdead5678);
        };
        hooks.streams.memory.release = (pointer) => release('F', pointer);
        hooks.streams.list.lifetime.stream = node;
        hooks.streams.list.lifetime.releaseBuffer = hooks.streams.memory.release;
        hooks.streams.list.lifetime.releaseStream = (pointer) => release('N', pointer);
        hooks.streams.list.compact = (pointer) => {
            allocating = pointer;
            compactMemStream(node(pointer), hooks.streams.memory);
        };
        hooks.streams.list.missingPredecessor = () => { throw new Error('정상 파일 원문에 없는 선행 링크 진단'); };
        hooks.streams.memory.assertion = () => { throw new Error('정상 파일 원문에 없는 MemStream 진단'); };
        hooks.assertion = () => { throw new Error('정상 파일 원문에 없는 mark/SNART 진단'); };
        readDataManagerImage(manager, source, hooks);
        assert.ok(Buffer.from(bytes).equals(readBytes(row[2])), `최종 메모리 ${row[0]}`);
        assert.equal(eventIndex, events.length, `경계 효과 수 ${row[0]}`);
    }
});

// 진단이 pos/end 를 바꾸면 원본 mark 부모도 그 현재 값을 읽어 진행한다(cpppj DataManagerRead_MarkAssertionUsesCurrentFields). (2026-11-11 WB04-E284)
test('DataManager mark: 진단이 pos/end 를 바꾸면 현재 값으로 mark 를 진행한다', () => {
    const source = new Uint8Array(20);
    const image = Uint8Array.of(5, 0, 0, 0, 0, 0, 0, 0);
    put(source, 0, 1);
    put(source, 4, 3);
    put(source, 8, 3);
    put(source, 16, 0);
    let diagnoses = 0;
    const hooks = {
        streams: { memory: { buffer: (pointer) => { assert.equal(pointer, 1); return image; } } },
        assertion: (kind) => {
            diagnoses += 1;
            assert.equal(kind, DATA_MANAGER_IMAGE_ASSERTION.markLength);
            put(source, 4, 4);
            put(source, 8, 4);
        },
    };
    advanceDataManagerMark(source, hooks);
    assert.equal(diagnoses, 1);
    assert.deepEqual([get(source, 16), get(source, 4), get(source, 8)], [4, 6, 4]);
});

// 이름 표 가상 조회가 먼저 수행되고 잘못된 시작 pos 진단의 수리를 다음 mark 가 읽어야 한다(cpppj DataManagerRead_InitialPositionAndRequiredNames). (2026-11-11 WB04-E284)
test('DataManager 이미지: 이름 표 조회가 먼저이고 시작 pos 진단의 수리를 다음 mark 가 읽으며 필수 이름 표 누락은 거부한다', () => {
    const manager = new Uint8Array(12);
    const source = new Uint8Array(20);
    const names = Uint8Array.of(0x2f, 0x2f, 0, 0);
    put(source, 4, 3);
    put(source, 8, 2);
    put(source, 16, 0xffffffff);
    let order = '';
    const hooks = {
        names: () => { order += 'N'; return names; },
        assertion: (kind) => {
            order += 'D';
            assert.equal(kind, DATA_MANAGER_IMAGE_ASSERTION.sourcePosition);
            put(source, 4, 2);
        },
    };
    readDataManagerImage(manager, source, hooks);
    assert.equal(order, 'ND');
    assert.deepEqual([get(source, 16), get(source, 4), names[0], names[1], names[2]], [2, 4, 0, 0, 0xff]);
    assert.ok(throws(() => readDataManagerImage(manager, source, {}), RangeError));
});

// 논리 end 1 도 실제 두 BYTE 가 있으면 원본처럼 읽는다. signed 타입 수/FE 정확 비교와 양쪽 보호 영역(cpppj DataManagerFile_SignatureBytesAndSignedTypeLimit). (2026-11-11 WB04-E284)
test('Template 파일 서명: 논리 end 1 에서도 두 BYTE 를 읽고 signed 타입 수·FE 정확 비교를 따르며 manager 의 두 DWORD 외 보호 영역을 건드리지 않는다', () => {
    // FE 이외의 값과 signed 음수는 원본의 서로 다른 비교를 통과한다.
    for (const flag of [0, 0xfe, 0xff]) {
        for (const count of [-1, 255, 256]) {
            const bytes = new Uint8Array(292).fill(0xa7);
            const manager = bytes.subarray(8, 284);
            const source = new Uint8Array(20);
            const input = Uint8Array.of(0x46, flag);
            const before = bytes.slice();
            let diagnoses = 0;
            put(source, 0, 1);
            put(source, 8, 1);
            put(source, 12, 2);
            put(source, 16, 0xffffffff);
            const hooks = {
                buffer: (pointer) => { assert.equal(pointer, 1); return input; },
                typeCount: () => count,
                tooManyTypes: () => { diagnoses += 1; },
            };
            assert.equal(readTemplateFileSignature(manager, source, hooks), true);
            assert.deepEqual([get(source, 4), get(source, 8), get(source, 12), get(source, 16)], [2, 1, 2, 0xffffffff]);
            assert.equal(get(manager, 0x10c), 0x46);
            assert.equal(get(manager, 0x110), flag === 0xfe ? 1 : 0);
            assert.equal(diagnoses, count >= 256 ? 1 : 0);
            // 원본 서명이 실제로 쓰는 두 DWORD 만 비교에서 제외한다.
            for (let index = 0; index < bytes.length; index += 1) if (index < 8 + 0x10c || index >= 8 + 0x114) assert.equal(bytes[index], before[index], `보호 영역 ${index}`);
        }
    }
});

// 진단 뒤 현재 source 재조회·잘못된 서명의 현재 이름/제거 요청·NULL/짧은 물리 입력을 구별한다(cpppj DataManagerFile_SignatureFailureAndCurrentRereads). (2026-11-11 WB04-E284)
test('Template 파일 서명: 진단 뒤 현재 source 를 다시 읽고 잘못된 서명은 현재 이름으로 거부 요청하며 NULL/짧은 입력을 구별한다', () => {
    const manager = new Uint8Array(276);
    const source = new Uint8Array(20);
    const input = Uint8Array.of(0x3f, 0x46, 0xfe); // '?', 'F', 0xfe
    let rejected = 0;
    let diagnoses = 0;
    put(source, 0, 1);
    put(source, 8, 3);
    const hooks = {
        typeCount: () => 256,
        tooManyTypes: () => { diagnoses += 1; put(source, 0, 2); put(source, 4, 1); },
        buffer: (pointer) => { assert.equal(pointer, 2); return input; },
    };
    assert.equal(readTemplateFileSignature(manager, source, hooks), true);
    assert.deepEqual([diagnoses, get(source, 4), get(manager, 0x10c), get(manager, 0x110)], [1, 3, 0x46, 1]);
    hooks.typeCount = () => 188;
    hooks.fileName = () => 'rejected.fort';
    hooks.rejectFile = (name) => {
        assert.equal(name, 'rejected.fort');
        assert.equal(get(source, 4), 2);
        assert.equal(get(manager, 0x10c), 0x3f);
        assert.equal(get(manager, 0x110), 0);
        rejected += 1;
    };
    put(source, 4, 0);
    assert.equal(readTemplateFileSignature(manager, source, hooks), false); // '?','F' 는 서명 실패이며 FE 판정도 0 이다.
    assert.equal(rejected, 1);
    put(source, 0, 0);
    const before = manager.slice();
    assert.equal(readTemplateFileSignature(manager, source, {}), false); // NULL 입력은 이름/타입 수/제거 경계를 호출하지 않는다.
    assert.deepEqual(manager, before);
    put(source, 0, 2);
    put(source, 4, 2);
    assert.ok(throws(() => readTemplateFileSignature(manager, source, hooks), RangeError));
    assert.equal(get(source, 4), 3);
    assert.equal(get(manager, 0x10c), 0xfe); // 첫 BYTE 이후의 물리 범위 거부도 앞선 쓰기를 되돌리지 않는다.
});

// 파일 없음·서명 거부/예외·빈 이름 표의 정상 본문을 같은 부모로 실행한다(cpppj DataManagerFile_ReadParentBranchesAndCleanupCurrent). (2026-11-11 WB04-E284)
test('DataManager 파일 부모: 네 분기(파일 없음·서명 거부·서명 예외·정상)와 서명이 바꾼 현재 포인터의 한 번 반납', () => {
    // 서명 경계가 data 를 바꿀 때 최초 포인터를 잘못 반납하면 실패해야 한다.
    for (let mode = 0; mode < 4; mode += 1) {
        const manager = new Uint8Array(12);
        const names = Uint8Array.of(0x2f, 0x2f, 0);
        let fileNames = 0;
        let reads = 0;
        let signatures = 0;
        let released = 0;
        const hooks = {
            fileName: () => { fileNames += 1; return 'input.fort'; },
            readFile: (name, kind, length, flags) => {
                assert.equal(name, 'input.fort');
                assert.equal(kind, 1);
                assert.equal(flags, 0);
                assert.equal(length.length, 4);
                assert.equal(get(length, 0), 0);
                reads += 1;
                put(length, 0, 2);
                return mode ? 1 : 0;
            },
            signature: (source) => {
                signatures += 1;
                assert.deepEqual([get(source, 0), get(source, 4), get(source, 8), get(source, 12), get(source, 16)], [1, 0, 2, 2, 0xffffffff]);
                put(source, 4, 2);
                if (mode < 3) put(source, 0, 2);
                if (mode === 2) throw new Error('명시 서명 경계 실패');
                return mode === 3;
            },
            image: { names: () => names },
            releaseFile: (pointer) => {
                assert.equal(pointer, mode < 3 ? 2 : 1);
                released += 1;
            },
        };
        if (mode === 2) assert.ok(throws(() => readDataManagerFile(manager, hooks)));
        else assert.equal(readDataManagerFile(manager, hooks), mode === 3);
        assert.deepEqual([fileNames, reads, signatures, released], [1, 1, mode ? 1 : 0, mode ? 1 : 0], `분기 ${mode}`);
    }
});

// 본문/반납 효과의 예외에서도 같은 파일을 중복 반납하지 않으며 필수 계약은 실제 사용 지점에서 거부한다(cpppj DataManagerFile_BodyAndReleaseFailureOnlyOnce). (2026-11-11 WB04-E284)
test('DataManager 파일 부모: 본문/반납 예외에서도 같은 파일을 한 번만 반납하고 누락된 부모 경계는 거부한다', () => {
    const manager = new Uint8Array(12);
    const names = Uint8Array.of(0x2f, 0x2f, 0);
    let released = 0;
    const hooks = {
        fileName: () => 'input.fort',
        readFile: (name, kind, length) => { put(length, 0, 2); return 1; },
        signature: (source) => { put(source, 4, 2); return true; },
        releaseFile: (pointer) => { assert.equal(pointer, 1); released += 1; },
        image: { names: () => { throw new Error('명시 이름 표 실패'); } },
    };
    assert.ok(throws(() => readDataManagerFile(manager, hooks)));
    assert.equal(released, 1);
    hooks.image = { names: () => names };
    hooks.releaseFile = (pointer) => { assert.equal(pointer, 1); released += 1; throw new Error('명시 반납 경계 실패'); };
    assert.ok(throws(() => readDataManagerFile(manager, hooks)));
    assert.equal(released, 2);
    assert.ok(throws(() => readDataManagerFile(manager, {}), TypeError));
});

// client 전체 읽기 어댑터: 파일 없음은 null, 존재하는 파일은 최소 4BYTE 할당과 논리 길이를 분리한 소유 블록이다. (2026-11-11 WB04-E284)
test('client DataManagerFileRead(웹 어댑터): 미발견 null·길이 0 도 최소 4BYTE 할당·논리 길이와 할당 분리·NUL 이후 이름 무시·읽기 훅 설치', () => {
    const files = new GameFiles({ loose: new Map([['d/empty.fort', new Uint8Array(0)], ['d/three.fort', Uint8Array.of(1, 2, 3)], ['d/five.fort', Uint8Array.of(9, 8, 7, 6, 5)]]) });
    assert.equal(readDataManagerFileImage(files, 'd/missing.fort'), null);
    const empty = readDataManagerFileImage(files, 'd/empty.fort');
    assert.deepEqual([empty.length, empty.storage.length], [0, 4]);
    const three = readDataManagerFileImage(files, 'D\\Three.fort');
    assert.equal(three.length, 3);
    assert.deepEqual([...three.storage], [1, 2, 3, 0]);
    const five = readDataManagerFileImage(files, 'd/five.fort\0ignored');
    assert.equal(five.length, 5);
    assert.deepEqual([...five.storage], [9, 8, 7, 6, 5]);
    // 반환 블록은 파일 원본 바이트의 사본이다(할당을 바꿔도 보관된 파일은 그대로).
    five.storage[0] = 0;
    assert.equal(readDataManagerFileImage(files, 'd/five.fort').storage[0], 9);
    // 읽기 훅 설치: 기존 훅을 보존하고 readWhole 만 설치한다.
    const original = { read: () => 'fallback', typeCount: () => 188 };
    const hooks = bindDataManagerFileRead(original, files);
    assert.equal(hooks.read(), 'fallback');
    assert.equal(hooks.typeCount(), 188);
    assert.equal(hooks.readWhole('d/three.fort').length, 3);
    assert.equal(hooks.readWhole('d/missing.fort'), null);
    assert.equal(original.readWhole, undefined);
    assert.ok(throws(() => readDataManagerFileImage(null, 'x'), TypeError));
});

// 계약: mark 진단의 signed 비교(end < pos)와 부호 있는 16비트 mark 길이, 진단 장치 누락 거부. 원본 144행은 정상 입력이라 진단 분기에 닿지 않는다. (2026-11-11 WB04-E284)
test('DataManager mark 계약: end < pos 진단은 signed 이고 mark 길이는 부호 있는 16비트이며 진단 장치가 없으면 거부한다', () => {
    // MarkPosition: signed end(−1) < pos(1) 이면 진단, pos 가 0x80000000(음수)이면 end 1 은 작지 않아 진단이 없다.
    const kinds = [];
    const hooks = { assertion: (kind) => kinds.push(kind) };
    const make = (pos, end, mark = 0xffffffff) => {
        const source = new Uint8Array(20);
        put(source, 4, pos);
        put(source, 8, end);
        put(source, 16, mark);
        return source;
    };
    advanceDataManagerMark(make(1, 0xffffffff), hooks);
    assert.deepEqual(kinds, [DATA_MANAGER_IMAGE_ASSERTION.markPosition]);
    kinds.length = 0;
    advanceDataManagerMark(make(0x80000000, 1), hooks);
    assert.deepEqual(kinds, []);
    advanceDataManagerMark(make(5, 5), hooks);
    assert.deepEqual(kinds, []);
    advanceDataManagerMark(make(5, 4), hooks);
    assert.deepEqual(kinds, [DATA_MANAGER_IMAGE_ASSERTION.markPosition]);
    // 부호 있는 16비트 mark: 0xffff(−1) 길이는 pos(3) − mark(4) = −1 과 같아 진단이 없다.
    kinds.length = 0;
    const negative = make(3, 99, 4);
    put(negative, 0, 1);
    advanceDataManagerMark(negative, { streams: { memory: { buffer: () => Uint8Array.of(0, 0, 0, 0, 0xff, 0xff) } }, assertion: (kind) => kinds.push(kind) });
    assert.deepEqual(kinds, []);
    // 진단 장치가 필요한데 없으면 빈 성공 대신 거부한다.
    assert.ok(throws(() => advanceDataManagerMark(make(5, 4), {})));
    assert.ok(throws(() => advanceDataManagerMark(new Uint8Array(19), hooks), RangeError));
});

// 계약: 섹션 읽기의 앞부분 — MissingMark 진단, 부호 있는 16비트 길이의 wrap(할당 크기), 입력 end 올림의 signed 비교. 장치는 필요한 지점까지만 연결하고 센티널 예외로 흐름을 끊는다. (2026-11-11 WB04-E284)
test('DataManager 이미지 계약: MissingMark 진단·16비트 길이 wrap·입력 end 올림(signed)을 섹션 본문 앞부분에서 구별한다', () => {
    const stop = new Error('센티널: 관측 지점 도달');
    // 이름 'A' 하나만 있는 SNART 표를 가진 최소 장면이다. 노드 7은 새 stream 이다.
    const scene = (mark, extra = {}) => {
        const manager = new Uint8Array(12);
        const source = new Uint8Array(20);
        const nodes = new Map([[7, new Uint8Array(60)]]);
        const names = Uint8Array.of(0x2f, 0x41, 0x2f, 0);
        put(source, 0, 1);
        put(source, 4, 2);
        put(source, 8, 1);
        put(source, 16, mark);
        const hooks = {
            names: () => names,
            assertion: () => {},
            streams: {
                selection: { stream: (handle) => nodes.get(handle) },
                list: { lifetime: { stream: (handle) => nodes.get(handle), releaseBuffer: () => {}, releaseStream: () => {} }, compact: () => {}, missingPredecessor: () => {} },
                memory: { allocate: () => 100, release: () => {}, buffer: (pointer) => (pointer === 1 ? Uint8Array.of(0, 0, 4, 0, 1, 2, 3, 4) : new Uint8Array(64)), assertion: () => {} },
                allocateStream: () => 7,
            },
            ...extra,
        };
        return { manager, source, hooks };
    };
    // MissingMark: 첫 mark 진행의 MarkPosition 진단이 mark 를 −1 로 되돌리면 본문 첫 항목이 MissingMark 진단을 낸다(진단은 한 번씩 순서대로).
    const kinds = [];
    const missing = scene(0xffffffff);
    missing.hooks.assertion = (kind) => {
        kinds.push(kind);
        if (kind === DATA_MANAGER_IMAGE_ASSERTION.markPosition) put(missing.source, 16, 0xffffffff);
        if (kind === DATA_MANAGER_IMAGE_ASSERTION.missingMark) throw stop;
    };
    assert.throws(() => readDataManagerImage(missing.manager, missing.source, missing.hooks), (error) => error === stop);
    assert.deepEqual(kinds, [DATA_MANAGER_IMAGE_ASSERTION.markPosition, DATA_MANAGER_IMAGE_ASSERTION.missingMark]);
    // 16비트 wrap: mark 길이 0x8000(−32768) − 2 는 16비트로 감겨 32766 이므로 새 stream 의 할당 크기가 32766 이다(감기지 않으면 1).
    const wrap = scene(0xffffffff);
    let allocated = 0;
    wrap.hooks.streams.memory.buffer = (pointer) => (pointer === 1 ? Uint8Array.of(0, 0, 0, 0x80, 1, 2, 3, 4) : new Uint8Array(64));
    wrap.hooks.streams.memory.allocate = (size) => {
        allocated = size;
        throw stop;
    };
    put(wrap.source, 8, 8);
    assert.throws(() => readDataManagerImage(wrap.manager, wrap.source, wrap.hooks), (error) => error === stop);
    assert.equal(allocated, 32766);
    // 입력 end 올림: 이전 end 가 0xffffffff(−1)이면 signed 로 −1 < 6 이라 새 end 6 으로 올라간다(길이 2, payload 위치 4).
    const raise = scene(0xffffffff);
    put(raise.source, 8, 0xffffffff);
    let observed = -1;
    let inputReads = 0;
    raise.hooks.streams.memory.buffer = (pointer) => {
        if (pointer === 1) {
            inputReads += 1;
            // 첫 조회는 mark 길이(2BYTE), 둘째 조회가 payload 범위이다: payload 를 읽기 직전에 입력 end 를 관측한다.
            if (inputReads === 2) {
                observed = get(raise.source, 8);
                throw stop;
            }
            return Uint8Array.of(0, 0, 4, 0, 1, 2, 3, 4);
        }
        return new Uint8Array(64);
    };
    raise.hooks.assertion = () => {}; // end(−1) < pos 의 MarkPosition 진단은 무시한다.
    assert.throws(() => readDataManagerImage(raise.manager, raise.source, raise.hooks), (error) => error === stop);
    assert.equal(observed, 6);
});

// 계약: 서명은 data 와 end 가 모두 비영일 때만 장치를 부르고, Template manager 276BYTE 가 아니면 쓰기 전에 거부한다. (2026-11-11 WB04-E284)
test('Template 파일 서명 계약: data 만 있고 end 가 0 이면 false 이고 manager 크기가 276 미만이면 거부한다', () => {
    const manager = new Uint8Array(276);
    const source = new Uint8Array(20);
    put(source, 0, 1);
    assert.equal(readTemplateFileSignature(manager, source, {}), false); // end 0
    put(source, 0, 0);
    put(source, 8, 1);
    assert.equal(readTemplateFileSignature(manager, source, {}), false); // data 0
    for (const size of [0, 12, 275]) assert.ok(throws(() => readTemplateFileSignature(new Uint8Array(size), source, {}), RangeError), `크기 ${size}`);
    assert.ok(manager.every((byte) => byte === 0));
});
