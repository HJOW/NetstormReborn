// DataManager 의 같은 연결 목록/current 를 쓰는 섹션 검색(0044ce60)과 선택(0044ceb0)을 옮긴 모듈이다(cpppj `o/RawDataManagerSelection.cpp/.h`, 10.78 전용, 기준선 1f00a25a 와 같음).
//  - findDataManagerSection(manager, name, hooks)(0044ce60): current(+8)를 head(+4)로 놓고 노드를 head 부터 따라가며(next = 노드 +0x38) 이름(노드 +16 부터 NUL 종료 40BYTE)이 처음 일치하는 노드를 찾는다.
//    비교(CRT stricmp 경계) 뒤에는 캡처한 노드가 아니라 **현재 current** 를 다시 읽어 다음 노드로 진행한다. 끝까지 없으면 current 는 0 이고 0 을 돌려준다. 첫 일치에서 그 핸들을 돌려준다.
//  - selectDataManagerSection(manager, name, writable, hooks)(0044ceb0): 검색한 뒤 current 가 0 이면 진단(MissingCurrent, 진단 뒤 current 를 다시 읽음), writable 이 0 이면 현재 노드의 버퍼(+0)/end(+8)가 비어 있을 때 진단(EmptyRead, 뒤에 다시 읽음),
//    마지막에 현재 노드의 pos(+4)만 0 으로 쓰고 current 를 돌려준다. 다른 노드와 필드는 건드리지 않는다.
//  - compareDataManagerSectionNames(first, second): 공식 섹션 이름의 ASCII 대소문자 무시 비교(NUL 이전만)이며 128 이상의 글자는 CRT 로캘 의존이라 복원하지 않고 거부한다.
// 노드는 호스트 주소를 DWORD 로 자르지 않고 현재 60BYTE 의 불투명 핸들로 찾는다. hooks = { stream(핸들)→60BYTE 이상 Uint8Array(MemStream16 + 이름40 + next DWORD), compare?(a,b)→0이면 일치, assertion?(종류) }.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E282: 4-B DataManager 섹션 검색/선택 이식)

// 원본 진단 두 종류이다. 진단 뒤 현재 포인터를 다시 읽는 지점도 구분한다. (2026-11-11 WB04-E282)
export const DATA_MANAGER_SELECTION_ASSERTION = Object.freeze({ missingCurrent: 'MissingCurrent', emptyRead: 'EmptyRead' });

// 노드 안 이름 위치·크기와 next 위치, 실제 노드/manager 최소 폭이다. (2026-11-11 WB04-E282)
const NAME_OFFSET = 16;
const NAME_BYTES = 40;
const NEXT_OFFSET = 0x38;
const NODE_BYTES = 60;
const MANAGER_BYTES = 12;

// 비정렬 DWORD 를 같은 저장소에서 읽는다. (2026-11-11 WB04-E282)
function read(bytes, at) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 지정 DWORD 의 네 BYTE 에만 쓴다(인접 필드 보존). (2026-11-11 WB04-E282)
function put(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 255;
}

// 동일 노드의 실제 최소 폭을 확인하고 잘못된 메모리를 성공으로 바꾸지 않는다. (2026-11-11 WB04-E282)
function nodeOf(handle, hooks) {
    if (!handle || typeof hooks?.stream !== 'function') throw new Error('DataManager 현재 노드/조회 연결 누락');
    const bytes = hooks.stream(handle);
    if (bytes.length < NODE_BYTES) throw new RangeError('DataManager 실제 60BYTE 노드 누락');
    return bytes;
}

// 노드의 이름을 첫 NUL 까지 읽은 이진 문자열로 돌려준다. 이름 40BYTE 안에 NUL 이 없으면 거부한다. (2026-11-11 WB04-E282)
function nameOf(node) {
    const bytes = node.subarray(NAME_OFFSET, NAME_OFFSET + NAME_BYTES);
    const end = bytes.indexOf(0);
    if (end < 0) throw new RangeError('DataManager 이름 40BYTE 내 NUL 누락');
    return String.fromCharCode(...bytes.subarray(0, end));
}

// 필요 진단을 생략하지 않는다. 반환한 뒤의 실제 저장소는 호출자가 다시 읽는다. (2026-11-11 WB04-E282)
function assertion(hooks, kind) {
    if (typeof hooks?.assertion !== 'function') throw new Error('DataManager 선택의 실제 진단 미연결');
    hooks.assertion(kind);
}

// ASCII 소문자로 바꾼다. 128 이상은 CRT 로캘에 의존하므로 복원하지 않고 거부한다. (2026-11-11 WB04-E282)
function lower(code) {
    if (code >= 128) throw new RangeError('DataManager 비ASCII 이름 CRT 로캘 미복원');
    return code >= 0x41 && code <= 0x5a ? code + 32 : code;
}

// NUL 이전의 문자열이다(C 문자열 규칙). (2026-11-11 WB04-E282)
function cString(text) {
    const nul = text.indexOf('\0');
    return nul < 0 ? text : text.slice(0, nul);
}

// 공식 섹션 이름의 첫 차이 부호와 NUL 종료를 CRT 경계와 같게 처리한다: 같으면 0, first 가 크면 1, 작으면 -1. 사용: compareDataManagerSectionNames('STATE', 'state') === 0. (2026-11-11 WB04-E282)
export function compareDataManagerSectionNames(first, second) {
    const left = cString(first);
    const right = cString(second);
    // 매 글자를 읽는 순서대로 비교하고 일치하는 접두 뒤 길이를 비교한다.
    for (let index = 0; index < Math.min(left.length, right.length); index += 1) {
        const a = lower(left.charCodeAt(index));
        const b = lower(right.charCodeAt(index));
        if (a !== b) return a > b ? 1 : -1;
    }
    return left.length === right.length ? 0 : left.length > right.length ? 1 : -1;
}

// 0044ce60: 비교에서 current 가 바뀌어도 캡처한 노드 대신 원본처럼 현재 값으로 진행한다. 사용: findDataManagerSection(manager12, 'State', { stream }) → 핸들/0. (2026-11-11 WB04-E282)
export function findDataManagerSection(manager, name, hooks) {
    if (!(manager instanceof Uint8Array) || manager.length < MANAGER_BYTES) throw new RangeError('DataManager 실제 12BYTE 누락');
    put(manager, 8, read(manager, 4));
    // 원본처럼 현재 포인터가 0 이 될 때 실패하고 첫 일치에서 반환한다.
    while (read(manager, 8)) {
        const node = nodeOf(read(manager, 8), hooks);
        const currentName = nameOf(node);
        const comparison = typeof hooks.compare === 'function' ? hooks.compare(name, currentName) : compareDataManagerSectionNames(name, currentName);
        if (!comparison) return read(manager, 8);
        put(manager, 8, read(nodeOf(read(manager, 8), hooks), NEXT_OFFSET));
    }
    return 0;
}

// 0044ceb0: 쓰기 허용은 비영 DWORD 이며 진단 이후의 현재 노드에만 pos 0 을 쓴다. 사용: selectDataManagerSection(manager12, 'State', 0, hooks) → current 핸들. (2026-11-11 WB04-E282)
export function selectDataManagerSection(manager, name, writable, hooks) {
    findDataManagerSection(manager, name, hooks);
    if (!read(manager, 8)) assertion(hooks, DATA_MANAGER_SELECTION_ASSERTION.missingCurrent);
    if ((writable | 0) === 0) {
        const node = nodeOf(read(manager, 8), hooks);
        if (!read(node, 0) || !read(node, 8)) assertion(hooks, DATA_MANAGER_SELECTION_ASSERTION.emptyRead);
    }
    put(nodeOf(read(manager, 8), hooks), 4, 0);
    return read(manager, 8);
}
