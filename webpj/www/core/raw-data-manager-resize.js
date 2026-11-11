// DataManager 의 같은 연결 목록에서 임시 stream 생성·이름 교체·최소 길이 확장을 원본 순서로 처리하는 모듈이다(cpppj `o/RawDataManagerResize.cpp/.h`, 10.78 전용, 기준선 1f00a25a 와 같음).
//  - constructDataManagerStream(stream, name, size, hooks)(0044d070): 노드 첫 16BYTE(MemStream)를 비우고 max(size,1) 로 할당한 뒤 이름과 NUL 만 쓰고(name 이 null 이면 이름 첫 BYTE 만 0) next(+0x38)를 0 으로 쓴다. 같은 핸들을 돌려준다.
//  - createDataManagerStream(manager, name, hooks)(0044d1b0): 같은 이름이 있으면 제거 → 새 노드 60 할당 → 비영이면 16384 로 생성 → 목록에 추가하고, 새 핸들이 아니라 추가 효과 뒤의 **현재 manager.current** 를 돌려준다.
//  - cloneDataManagerStream(manager, sourceManager, name, hooks)(0044d230): 다른 manager 의 현재 원문을 찾고(없으면 진단 뒤 계속) 대상 생성 뒤 원문 data(+0)가 있을 때만 대입하고 대상 pos 를 0 으로 돌린다.
//  - renameDataManagerStream(manager, name, previousName, hooks)(0044d160): 대상 이름이 있으면 제거한 **뒤에** 원래 이름을 찾고 대상 이름과 NUL 만 쓴다(선행 current 를 캐시하지 않음).
//  - ensureDataManagerStreamSize(manager, name, minimum, hooks)(0044d290): signed end 가 최소보다 작을 때만 임시 이름 "T$e$m$p" 의 새 stream 을 minimum 으로 만들어 원문 내용을 복사하고 남은 부분을 0 으로 채우며 pos/end 를 용량으로 올린 뒤
//    목록에 추가하고 임시 이름을 원래 이름으로 교체한다. 원문 노드는 생성 전에 보관하되 data/end 는 생성이 끝난 뒤 다시 읽는다.
// hooks = { selection:{stream,compare?,assertion?}, list:{lifetime:{stream,releaseBuffer,releaseStream},compact,missingPredecessor}, memory:{allocate,release,buffer,assertion}, allocateStream(60)→핸들/0, missingSource() }.
// 모든 조회는 같은 노드/버퍼를 빌리며 각 경계 뒤 현재 원문을 다시 읽는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E283: 4-B DataManager 임시 stream·이름 교체·최소 확장 이식)
import { allocateMemStream, copyMemStream, writeMemStream } from './raw-mem-stream.js';
import { appendDataManagerStream, removeDataManagerStream } from './raw-data-manager-list.js';
import { findDataManagerSection } from './raw-data-manager-selection.js';

// 0044d290 이 생성과 이름 교체에 같은 값으로 쓰는 원본 임시 이름이다. (2026-11-11 WB04-E283)
export const DATA_MANAGER_TEMPORARY_NAME = 'T$e$m$p';

// 노드 안 이름 위치와 이름 영역 크기, 새 stream 의 기본 용량(0x4000)이다. (2026-11-11 WB04-E283)
const NAME_OFFSET = 16;
const NAME_BYTES = 40;
const NEXT_OFFSET = 0x38;
const NODE_BYTES = 60;
const CREATE_CAPACITY = 0x4000;

// 비정렬 DWORD 를 같은 저장소에서 읽는다. (2026-11-11 WB04-E283)
function read(bytes, at) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 지정 DWORD 의 네 BYTE 에만 쓴다(인접 이름/링크 보존). (2026-11-11 WB04-E283)
function put(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 255;
}

// 포인터를 자르지 않고 같은 실제 60BYTE 노드를 빌린다. (2026-11-11 WB04-E283)
function nodeOf(hooks, handle) {
    if (!handle || typeof hooks?.selection?.stream !== 'function') throw new Error('DataManager 확장의 실제 노드 조회 미연결');
    const bytes = hooks.selection.stream(handle);
    if (bytes.length < NODE_BYTES) throw new RangeError('DataManager 확장의 실제 60BYTE 누락');
    return bytes;
}

// 원본 strcpy 의 이름과 NUL 만 쓰며 나머지 이름 40BYTE 는 초기화하지 않는다. 이름은 첫 NUL 까지이고 40BYTE 이상이면 거부한다. (2026-11-11 WB04-E283)
function writeName(node, name) {
    const nul = name.indexOf('\0');
    const text = nul < 0 ? name : name.slice(0, nul);
    if (text.length >= NAME_BYTES) throw new RangeError('DataManager 실제 이름 40BYTE 범위');
    for (let index = 0; index < text.length; index += 1) node[NAME_OFFSET + index] = text.charCodeAt(index) & 255;
    node[NAME_OFFSET + text.length] = 0;
}

// 실제 접근하는 양의 범위만 조회하고 0 길이는 잘못된 포인터를 역참조하지 않는다. (2026-11-11 WB04-E283)
function buffer(hooks, pointer, position, length) {
    if (!length) return new Uint8Array(0);
    if (!pointer || typeof hooks?.memory?.buffer !== 'function') throw new Error('DataManager 확장의 실제 버퍼 조회 미연결');
    const bytes = hooks.memory.buffer(pointer);
    if (position > bytes.length || length > bytes.length - position) throw new RangeError('DataManager 확장의 실제 버퍼 물리 범위');
    return bytes.subarray(position, position + length);
}

// 0044d070: 생성자는 할당 뒤의 현재 입력 이름을 복사하며 name 이 null 이면 첫 이름 BYTE 만 0 으로 쓴다. 사용: constructDataManagerStream(핸들, 'State', 28, hooks) → 같은 핸들. (2026-11-11 WB04-E283)
export function constructDataManagerStream(stream, name, size, hooks) {
    const node = nodeOf(hooks, stream);
    node.fill(0, 0, 16);
    allocateMemStream(node, Math.max(size | 0, 1) >>> 0, hooks.memory);
    if (name !== null && name !== undefined) writeName(node, name);
    else node[NAME_OFFSET] = 0;
    put(node, NEXT_OFFSET, 0);
    return stream;
}

// 0044d1b0: 원본 생성 부모는 새 this 가 아니라 마지막 추가 효과 뒤의 manager.current 를 반환한다. 사용: createDataManagerStream(manager12, 'State', hooks). (2026-11-11 WB04-E283)
export function createDataManagerStream(manager, name, hooks) {
    const existing = findDataManagerSection(manager, name, hooks.selection);
    if (existing) removeDataManagerStream(manager, existing, hooks.list);
    if (typeof hooks.allocateStream !== 'function') throw new Error('DataManager 섹션 생성의 실제 노드 할당 미연결');
    const stream = hooks.allocateStream(NODE_BYTES) >>> 0;
    if (stream) constructDataManagerStream(stream, name, CREATE_CAPACITY, hooks);
    appendDataManagerStream(manager, stream, hooks.list);
    return read(manager, 8);
}

// 0044d230: 원문 노드는 생성 전의 같은 객체이며 그 data/end 만 대상 생성이 끝난 뒤 다시 읽는다. 사용: cloneDataManagerStream(manager, sourceManager, 'State', hooks). (2026-11-11 WB04-E283)
export function cloneDataManagerStream(manager, sourceManager, name, hooks) {
    const original = findDataManagerSection(sourceManager, name, hooks.selection);
    if (!original) {
        if (typeof hooks.missingSource !== 'function') throw new Error('DataManager 복제의 원문 누락 진단 미연결');
        hooks.missingSource();
    }
    const source = nodeOf(hooks, original);
    const stream = createDataManagerStream(manager, name, hooks);
    const target = nodeOf(hooks, stream);
    if (read(source, 0)) copyMemStream(target, source, hooks.memory);
    put(target, 4, 0);
    return stream;
}

// 0044d160: 기존 대상의 반납 효과 뒤 원래 이름을 찾으므로 선행 current 를 캐시하지 않는다. 사용: renameDataManagerStream(manager, '새', '옛', hooks) → current. (2026-11-11 WB04-E283)
export function renameDataManagerStream(manager, name, previousName, hooks) {
    const target = findDataManagerSection(manager, name, hooks.selection);
    if (target) removeDataManagerStream(manager, target, hooks.list);
    const source = findDataManagerSection(manager, previousName, hooks.selection);
    writeName(nodeOf(hooks, source), name);
    return read(manager, 8);
}

// 0044d290: 부모는 생성 전 원문 노드를 보관하되 실제 data/end 는 생성이 끝난 뒤 다시 읽는다. minimum 은 signed DWORD 이다. 사용: ensureDataManagerStreamSize(manager12, 'State', 28, hooks) → current. (2026-11-11 WB04-E283)
export function ensureDataManagerStreamSize(manager, name, minimum, hooks) {
    const original = findDataManagerSection(manager, name, hooks.selection);
    if (!original) {
        if (typeof hooks.missingSource !== 'function') throw new Error('DataManager 최소 확장의 누락 진단 미연결');
        hooks.missingSource();
    }
    const source = nodeOf(hooks, original);
    if ((read(source, 8) | 0) >= (minimum | 0)) return read(manager, 8);
    if (typeof hooks.allocateStream !== 'function') throw new Error('DataManager 임시 stream 실제 할당 미연결');
    const allocated = hooks.allocateStream(NODE_BYTES) >>> 0;
    if (allocated) constructDataManagerStream(allocated, DATA_MANAGER_TEMPORARY_NAME, minimum, hooks);
    const target = nodeOf(hooks, allocated);
    const length = read(source, 8);
    writeMemStream(target, buffer(hooks, read(source, 0), 0, length), length, hooks.memory);
    // 남은 용량 구간을 0 으로 채우고 pos/end 를 용량으로 올린다(생성 뒤 alloc 재조회).
    const remaining = (read(target, 12) - read(target, 4)) >>> 0;
    buffer(hooks, read(target, 0), read(target, 4), remaining).fill(0);
    const end = read(target, 12);
    put(target, 4, end);
    if ((read(target, 8) | 0) < (end | 0)) put(target, 8, end);
    appendDataManagerStream(manager, allocated, hooks.list);
    renameDataManagerStream(manager, name, DATA_MANAGER_TEMPORARY_NAME, hooks);
    return read(manager, 8);
}
