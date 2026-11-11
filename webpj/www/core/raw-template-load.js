// Template manager 생성자와 기존 파일을 유지하는 바깥 Load 의 원본 제어 순서를 옮긴 모듈이다(cpppj `o/RawTemplateLoad.cpp/.h`, 원본 004bcc60 · 가상 +0 의 004bcca0 · 004bf550, 10.78 전용).
//  - constructTemplateDataManager(manager, fileName, global)(004bcc60): manager276 의 지정 DWORD(+0 가상표 0x514690, +4·+8·+0x10c = 0, +0x110 = 0xffffffff)와 +12 부터의 NUL 종료 이름만 쓰고, 005c89c0 의 같은 4BYTE 전역을 0 으로 쓴다.
//    이름 뒤 공간과 manager 밖 BYTE 는 건드리지 않는다. 파일명은 첫 NUL 까지만 복사하고 256BYTE 이상(NUL 자리 없음)이면 원본 밖 쓰기 대신 거부한다. 파일명 입력은 manager/global 과 겹치지 않아야 한다.
//  - templateDataManagerFileName(manager)(가상 +0, 004bcca0): 같은 manager+12 의 NUL 종료 이름을 **살아 있는 뷰**(Uint8Array)로 빌린다. 이름이 바뀌면 뷰도 같은 BYTE 를 본다. NUL 이 없으면 거부한다.
//  - loadTemplateFile(header, fileName, hooks)(004bf550): 20BYTE 헤더의 +0 에 기존 manager 가 있으면 같은 이름이든 다른 이름이든 할당/읽기/해제 없이 이름만 비교해(다르면 진단) 돌아간다.
//    빈 헤더면 +4 를 0 으로 쓰고 할당(276) → 비영이면 생성자 → 헤더 +0 에 반환값(EAX) 설치 → 읽기 전용 사전 설정(+8)이 0 일 때만 판정(0041af80)해 +8 에 1/0 정규화 → +16 을 0 → 파일 읽기(0044d980)를 실행한다.
//    읽기 성공이면 헤더 +0 을 다시 읽어 비영이고 현재 슬롯일 때만 창 제목을 갱신하며, 실패면 읽기 뒤 헤더 +0 을 다시 읽어 비영이면 base 소멸 → 저장 포인터 free 를 부르고 +0 만 0 으로 쓴다.
//    헤더의 다른 필드(+12 등)와 파일명 뒤 바이트는 임의로 초기화하지 않는다. 실패 때 Win32/C++ 정리 정책은 실제 소유 어댑터 몫이다.
// hooks 는 도달한 분기에서만 요구한다(없으면 그 호출 위치에서 거부). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E281: 4-B RawTemplateLoad 이식)

// 10.78 manager/Template 헤더/파일명 배열의 실제 폭과 manager 안 이름 위치이다. (2026-11-11 WB04-E281)
const MANAGER_BYTES = 276;
const HEADER_BYTES = 20;
const NAME_OFFSET = 12;
const NAME_BYTES = 256;

// 비정렬 DWORD 를 같은 저장소에서 읽는다. (2026-11-11 WB04-E281)
function read(bytes, at) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 원본이 쓰는 지정 DWORD 의 네 BYTE 에만 쓴다(주변 필드를 보존). (2026-11-11 WB04-E281)
function put(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (8 * index)) & 255;
}

// 선택한 효과가 없으면 성공으로 채우지 않고 그 호출 지점에서 거부한다. (2026-11-11 WB04-E281)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('Template Load 의 실제 하위 효과 미연결');
    return hook;
}

// 파일명 입력(문자열 또는 BYTE 배열)을 BYTE 코드 목록으로 바꾼다. 문자열의 글자는 모두 한 BYTE(0~255)여야 한다. (2026-11-11 WB04-E281)
function nameBytes(fileName) {
    if (typeof fileName === 'string') {
        const bytes = new Uint8Array(fileName.length);
        for (let index = 0; index < fileName.length; index += 1) {
            const code = fileName.charCodeAt(index);
            if (code > 255) throw new RangeError('Template 파일명 글자는 한 BYTE 여야 합니다.');
            bytes[index] = code;
        }
        return bytes;
    }
    if (fileName instanceof Uint8Array) return fileName.slice();
    throw new TypeError('Template 파일명은 문자열 또는 Uint8Array 여야 합니다.');
}

// 004bcc60: 원본 생성자에서 쓰지 않는 파일명 뒤 공간과 manager 밖 BYTE 는 보존한다. 사용: constructTemplateDataManager(manager276, 'x.fort', global4). (2026-11-11 WB04-E281)
export function constructTemplateDataManager(manager, fileName, global) {
    const bytes = nameBytes(fileName);
    const nul = bytes.indexOf(0);
    const length = nul < 0 ? bytes.length : nul;
    if (!(manager instanceof Uint8Array) || manager.length < MANAGER_BYTES || length >= NAME_BYTES) throw new RangeError('Template 생성자의 실제 manager/파일명 범위');
    if (!(global instanceof Uint8Array) || global.length !== 4) throw new RangeError('Template 생성자의 실제 4BYTE 판본 전역 누락');
    put(manager, 4, 0);
    put(manager, 8, 0);
    put(manager, 0, 0x514690);
    put(manager, 0x10c, 0);
    put(manager, 0x110, 0xffffffff);
    put(global, 0, 0);
    // 입력 C 문자열의 첫 NUL 까지 한 BYTE 씩 순서대로 복사한다(끝에 NUL 하나).
    for (let index = 0; index <= length; index += 1) manager[NAME_OFFSET + index] = index === length ? 0 : bytes[index];
}

// 가상 +0 의 004bcca0: 파일명은 별도 metadata 문자열 대신 실제 manager 배열에서 빌린다(살아 있는 뷰). NUL 이 없으면 거부한다. (2026-11-11 WB04-E281)
export function templateDataManagerFileName(manager) {
    if (!(manager instanceof Uint8Array) || manager.length < MANAGER_BYTES) throw new RangeError('Template 파일명 getter 의 manager276 누락');
    const bytes = manager.subarray(NAME_OFFSET, NAME_OFFSET + NAME_BYTES);
    const end = bytes.indexOf(0);
    if (end < 0) throw new RangeError('Template 파일명의 실제 NUL 누락');
    return bytes.subarray(0, end);
}

// 004bf550: 하위 효과 뒤 원본이 다시 읽는 ptr/readonly/current 를 같은 헤더에서 재조회한다.
// hooks = { fileName(manager)→이름, sameName(a,b)→bool(CRT stricmp==0), wrongName(), allocate(276)→포인터/0, construct(포인터,이름)→포인터, readOnly(이름)→bool, readFile(포인터)→bool, isCurrent()→bool,
//          refreshTitle(), lifetime:{ destroy(포인터), release(포인터) } }. 사용: loadTemplateFile(header20, 'x.fort', hooks). (2026-11-11 WB04-E281)
export function loadTemplateFile(header, fileName, hooks) {
    if (!(header instanceof Uint8Array) || header.length !== HEADER_BYTES) throw new RangeError('Template Load 의 실제 20BYTE 헤더 누락');
    const old = read(header, 0);
    // 이미 파일을 가진 헤더는 같음/다름 어느 경우에도 할당/읽기/해제를 하지 않는다.
    if (old) {
        const name = need(hooks?.fileName)(old);
        if (!need(hooks.sameName)(name, fileName)) need(hooks.wrongName)();
        return;
    }
    put(header, 4, 0);
    let manager = need(hooks?.allocate)(MANAGER_BYTES) >>> 0;
    if (manager) manager = need(hooks.construct)(manager, fileName) >>> 0;
    put(header, 0, manager);
    const readOnly = read(header, 8) !== 0 || need(hooks.readOnly)(need(hooks.fileName)(manager));
    put(header, 8, readOnly ? 1 : 0);
    put(header, 16, 0);
    if (need(hooks.readFile)(read(header, 0))) {
        if (read(header, 0) && need(hooks.isCurrent)()) need(hooks.refreshTitle)();
        return;
    }
    const failed = read(header, 0);
    if (failed) {
        need(hooks.lifetime?.destroy)(failed);
        need(hooks.lifetime.release)(failed);
    }
    put(header, 0, 0);
}
