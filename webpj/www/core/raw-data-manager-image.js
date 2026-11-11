// 10.78 파일 이미지의 같은 manager/MarkStream20/이름 표를 실제 섹션 목록에 읽는 DataManager 부모들을 옮긴 모듈이다(cpppj `o/RawDataManagerImage.cpp/.h`, 10.78 전용, 기준선 1f00a25a 와 같음).
//  - advanceDataManagerMark(source, hooks)(0044cdb0): 이전 mark 가 있으면 mark 위치의 부호 있는 16비트 길이가 (pos − mark)와 같은지 확인(MarkLength 진단) → 현재 pos 를 새 mark 로 기록 → signed end < pos 이면 진단(MarkPosition) → pos + 2.
//    진단이 mark/pos/end 를 바꾸면 그 현재 값을 후속 쓰기에 쓴다.
//  - readDataManagerImage(manager, source, hooks)(0044d380): 이름 표(가상 +4 의 같은 SNART 표)를 먼저 조회하고 시작 pos 가 2 가 아니면 진단(SourcePosition) 뒤 mark 를 진행한 다음, 이름 표 순서대로 빈 이름이 나올 때까지 각 섹션을 읽는다:
//    mark 없음이면 진단(MissingMark) → 길이 = (부호 있는 16비트 mark 길이 − 2)의 16비트 wrap → 새 노드(60) 할당·생성 → 입력 pos 를 payload 뒤로 갱신(end 는 signed 로 올림) → 입력 payload 를 노드 stream 에 쓰고 pos 0 →
//    같은 이름이 이미 있고 새 섹션이 비어 있으면(버퍼 또는 end 0) 새 노드의 stream 소멸 + 노드 반납(기존 것 유지), 아니면 기존 것을 제거하고 새 노드를 목록에 추가 → mark 진행. 마지막 mark 는 end+2 까지 진행할 수 있다.
//  - readTemplateFileSignature(manager, source, hooks)(004bce10): data(+0)와 end(+8)가 둘 다 비영일 때만 현재 타입 수(signed ≥ 256 이면 진단 뒤 계속)를 확인하고 입력의 첫 두 BYTE 를 읽어(매 BYTE 현재 data/pos 를 다시 읽고 pos 증가) manager +0x10c 에 첫 BYTE, +0x110 에 둘째 BYTE 가 0xfe 인지(1/0)를 쓴다.
//    첫 BYTE 가 'F'(0x46)가 아니면 현재 파일 이름으로 거부 요청(rejectFile)을 보내고 false 이다. 논리 end 의 두 BYTE 길이는 추가로 검사하지 않고 물리 범위는 별도로 보호한다.
//  - readDataManagerFile(manager, hooks)(0044d980): 20BYTE MarkStream 초기화(mark −1) → 파일 읽기(이름, 1, 길이 DWORD 주소, 0)의 반환 data 토큰 → end = 길이 → 없으면 false → 가상 서명 → 실패면 반납 후 false → 섹션 본문 → 현재 data 할당을 한 번만 반납한다.
//    서명/본문이 source.data 를 바꾸면 마지막의 현재 포인터를 반납하고, 예외에서도 같은 반납을 한 번 시도한다(반납 자체의 예외로 중복 반납하지 않음).
// hooks 는 도달한 분기에서만 요구한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E284: 4-B DataManager 파일 이미지 읽기 이식)
import { readSnartName } from './raw-snart.js';
import { writeMemStream } from './raw-mem-stream.js';
import { destroyMemStream } from './raw-data-manager-lifetime.js';
import { appendDataManagerStream, removeDataManagerStream } from './raw-data-manager-list.js';
import { findDataManagerSection } from './raw-data-manager-selection.js';
import { constructDataManagerStream } from './raw-data-manager-resize.js';

// 진단 뒤 계속하는 원본 위치를 구별한다. 물리 범위 초과는 별도로 거부한다. (2026-11-11 WB04-E284)
export const DATA_MANAGER_IMAGE_ASSERTION = Object.freeze({
    sourcePosition: 'SourcePosition', missingMark: 'MissingMark', markLength: 'MarkLength', markPosition: 'MarkPosition', nameSeparator: 'NameSeparator',
});

// MarkStream20 의 크기(첫 16BYTE MemStream + mark DWORD)와 노드/manager/Template manager 의 실제 최소 폭이다. (2026-11-11 WB04-E284)
const SOURCE_BYTES = 20;
const NODE_BYTES = 60;
const MANAGER_BYTES = 12;
const TEMPLATE_MANAGER_BYTES = 0x114;

// 비정렬 DWORD 를 읽는다. (2026-11-11 WB04-E284)
function read(bytes, at) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 지정 DWORD 의 네 BYTE 에만 쓴다(인접 보호 바이트 보존). (2026-11-11 WB04-E284)
function put(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 255;
}

// 미연결 진단을 빈 성공으로 채우지 않는다. (2026-11-11 WB04-E284)
function assertion(hooks, kind) {
    if (typeof hooks?.assertion !== 'function') throw new Error('DataManager 파일 이미지의 실제 진단 미연결');
    hooks.assertion(kind);
}

// MarkStream 은 첫 16BYTE 와 추가 mark DWORD 가 모두 있어야 한다. (2026-11-11 WB04-E284)
function checkSource(source) {
    if (!(source instanceof Uint8Array) || source.length < SOURCE_BYTES) throw new RangeError('DataManager 파일 이미지의 실제 MarkStream20 누락');
}

// 실제 접근할 범위만 빌리며 0 길이는 포인터 조회를 생략한다. (2026-11-11 WB04-E284)
function range(hooks, pointer, position, length) {
    if (!length) return new Uint8Array(0);
    if (!pointer || typeof hooks?.streams?.memory?.buffer !== 'function') throw new Error('DataManager 파일 이미지의 실제 버퍼 미연결');
    const bytes = hooks.streams.memory.buffer(pointer);
    if (position > bytes.length || length > bytes.length - position) throw new RangeError('DataManager 파일 이미지의 실제 버퍼 물리 범위');
    return bytes.subarray(position, position + length);
}

// 원본 getMark 는 부호 있는 16비트이며 길이 계산의 짧은 정수 wrap 도 보존한다. (2026-11-11 WB04-E284)
function markValue(source, hooks) {
    const bytes = range(hooks, read(source, 0), read(source, 16), 2);
    return ((bytes[0] | (bytes[1] << 8)) << 16) >> 16;
}

// 실제 60BYTE 노드만 빌리고 잘못된 NULL 을 임의 새 노드로 대체하지 않는다. (2026-11-11 WB04-E284)
function nodeOf(hooks, handle) {
    if (!handle || typeof hooks?.streams?.selection?.stream !== 'function') throw new Error('DataManager 파일 이미지의 실제 노드 미연결');
    const bytes = hooks.streams.selection.stream(handle);
    if (bytes.length < NODE_BYTES) throw new RangeError('DataManager 파일 이미지의 실제 노드 60 누락');
    return bytes;
}

// 004a8800/004a8850 의 같은 SNART 표를 한 번 변환하고 지정 이름을 첫 NUL 까지 읽는다. (2026-11-11 WB04-E284)
function nameAt(names, index, hooks) {
    return readSnartName(names, index, () => assertion(hooks, DATA_MANAGER_IMAGE_ASSERTION.nameSeparator));
}

// 0044cdb0: 진단이 mark/pos/end 를 바꾸면 원본 본문처럼 그 현재 값을 후속 쓰기에 사용한다. 사용: advanceDataManagerMark(source20, hooks). (2026-11-11 WB04-E284)
export function advanceDataManagerMark(source, hooks) {
    checkSource(source);
    const mark = read(source, 16);
    if (mark !== 0xffffffff && markValue(source, hooks) !== ((read(source, 4) - mark) | 0)) assertion(hooks, DATA_MANAGER_IMAGE_ASSERTION.markLength);
    put(source, 16, read(source, 4));
    if ((read(source, 8) | 0) < (read(source, 4) | 0)) assertion(hooks, DATA_MANAGER_IMAGE_ASSERTION.markPosition);
    put(source, 4, read(source, 4) + 2);
}

// 0044d380: 입력 pos 는 payload 뒤에 갱신하고 빈 새 섹션이 기존 것을 덮지 않는 원본 병합 순서를 유지한다. hooks = { streams(DataManagerResizeHooks), names()→SNART 표, assertion(종류) }. (2026-11-11 WB04-E284)
export function readDataManagerImage(manager, source, hooks) {
    checkSource(source);
    if (!(manager instanceof Uint8Array) || manager.length < MANAGER_BYTES || typeof hooks?.names !== 'function') throw new RangeError('DataManager 파일 이미지의 manager/가상 이름 표 미연결');
    const names = hooks.names();
    if (read(source, 4) !== 2) assertion(hooks, DATA_MANAGER_IMAGE_ASSERTION.sourcePosition);
    advanceDataManagerMark(source, hooks);
    // 마지막 빈 이름까지 원래 이름 표 순서대로 읽는다. 마지막 mark 는 end+2 까지 진행할 수 있다.
    for (let index = 0; ; index += 1) {
        const name = nameAt(names, index, hooks);
        if (name === '') return;
        if (read(source, 16) === 0xffffffff) assertion(hooks, DATA_MANAGER_IMAGE_ASSERTION.missingMark);
        const narrow = (markValue(source, hooks) - 2) & 0xffff;
        const signedLength = (narrow << 16) >> 16;
        const length = signedLength >>> 0;
        if (typeof hooks.streams.allocateStream !== 'function') throw new Error('DataManager 파일 이미지의 실제 새 노드 할당 미연결');
        const stream = hooks.streams.allocateStream(NODE_BYTES) >>> 0;
        if (stream) constructDataManagerStream(stream, name, signedLength, hooks.streams);
        const target = nodeOf(hooks, stream);
        const position = read(source, 4);
        const pointer = read(source, 0);
        const end = (position + length) >>> 0;
        put(source, 4, end);
        if ((read(source, 8) | 0) < (end | 0)) put(source, 8, end);
        writeMemStream(target, range(hooks, pointer, position, length), length, hooks.streams.memory);
        put(target, 4, 0);
        const existing = findDataManagerSection(manager, name, hooks.streams.selection);
        if (existing && (!read(target, 0) || !read(target, 8))) {
            destroyMemStream(target, hooks.streams.list.lifetime.releaseBuffer);
            if (typeof hooks.streams.list.lifetime.releaseStream !== 'function') throw new Error('DataManager 빈 새 섹션의 실제 노드 반납 미연결');
            hooks.streams.list.lifetime.releaseStream(stream);
        } else {
            if (existing) removeDataManagerStream(manager, existing, hooks.streams.list);
            appendDataManagerStream(manager, stream, hooks.streams.list);
        }
        advanceDataManagerMark(source, hooks);
    }
}

// 004bce10: 서명은 논리 end 의 두 BYTE 길이를 추가 검사하지 않는다. 실제 원본은 data/end 의 비영만 확인하며 물리 범위는 별도로 보호한다.
// hooks = { buffer(포인터)→입력 바이트, typeCount()→signed 현재 타입 수, tooManyTypes(), fileName()→이름, rejectFile(이름) }. (2026-11-11 WB04-E284)
export function readTemplateFileSignature(manager, source, hooks) {
    checkSource(source);
    if (!(manager instanceof Uint8Array) || manager.length < TEMPLATE_MANAGER_BYTES) throw new RangeError('Template 파일 서명의 manager276 누락');
    if (!read(source, 0) || !read(source, 8)) return false;
    if (typeof hooks?.typeCount !== 'function' || typeof hooks.buffer !== 'function') throw new Error('Template 파일 서명의 현재 타입 수/버퍼 미연결');
    if ((hooks.typeCount() | 0) >= 256) {
        if (typeof hooks.tooManyTypes !== 'function') throw new Error('Template 파일 서명의 타입 수 진단 미연결');
        hooks.tooManyTypes();
    }
    // 실제 BYTE 마다 현재 data/pos 를 다시 읽고 원본 DWORD 증가를 적용한다.
    const byte = () => {
        const pointer = read(source, 0);
        const position = read(source, 4);
        const bytes = hooks.buffer(pointer);
        if (!pointer || position >= bytes.length) throw new RangeError('Template 파일 서명의 실제 BYTE 물리 범위');
        const value = bytes[position];
        put(source, 4, position + 1);
        return value;
    };
    put(manager, 0x10c, byte());
    const flag = byte();
    put(manager, 0x110, flag === 0xfe ? 1 : 0);
    if (read(manager, 0x10c) === 0x46) return true;
    if (typeof hooks.fileName !== 'function' || typeof hooks.rejectFile !== 'function') throw new Error('Template 잘못된 서명의 현재 파일 처리 미연결');
    hooks.rejectFile(hooks.fileName());
    return false;
}

// 0044d980: 원본 정상 복귀는 파일 할당만 반납한다. 호스트 예외에서도 같은 현재 포인터의 반납을 한 번만 시도한다.
// hooks = { image(readDataManagerImage 훅), fileName()→이름, readFile(이름, 1, 길이DWORD 4BYTE, 0)→data 토큰/0, signature(source)→bool, releaseFile(토큰) }. 사용: readDataManagerFile(manager12, hooks) → 성공 여부. (2026-11-11 WB04-E284)
export function readDataManagerFile(manager, hooks) {
    if (!(manager instanceof Uint8Array) || manager.length < MANAGER_BYTES || typeof hooks?.fileName !== 'function' || typeof hooks.readFile !== 'function') throw new TypeError('DataManager 파일 읽기 부모의 manager/파일 경계 누락');
    const source = new Uint8Array(SOURCE_BYTES);
    put(source, 16, 0xffffffff); // 원본의 첫 네 DWORD 0/mark −1 이다.
    let releaseAttempted = false; // 반납 중 예외의 중복 해제를 막는다.
    // 서명/본문이 source.data 를 바꾸면 원본처럼 마지막의 현재 포인터를 반납한다.
    const release = () => {
        if (releaseAttempted) return;
        releaseAttempted = true;
        const pointer = read(source, 0);
        if (!pointer) return;
        if (typeof hooks.releaseFile !== 'function') throw new Error('DataManager 실제 파일 할당 반납 미연결');
        hooks.releaseFile(pointer);
    };
    try {
        const pointer = hooks.readFile(hooks.fileName(), 1, source.subarray(12, 16), 0) >>> 0;
        put(source, 0, pointer);
        put(source, 8, read(source, 12));
        if (!pointer) return false;
        if (typeof hooks.signature !== 'function') throw new Error('DataManager 파일 읽기의 실제 가상 서명 미연결');
        if (!hooks.signature(source)) {
            release();
            return false;
        }
        readDataManagerImage(manager, source, hooks.image);
        release();
        return true;
    } catch (error) {
        release();
        throw error;
    }
}
