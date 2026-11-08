// Player 소어 루저 후처리가 읽고 쓰는 현재 Template State 28 BYTE(getter 004bd550)와 isMine 재검사·선택 저장 통지(Notify 004bd540/004bd360)를 옮긴 모듈이다(cpppj `o/RawPlayerTemplateState.cpp/.h`, 10.78 전용).
// get(slot): Template 첫 DWORD 0 이면 진단(MissingStateTemplate) 뒤에도 0 이면 물리 경계 오류 → 현재 State 스트림 position 0 쓰기 →
//   빈 State(데이터 없음/end 0)는 미정 스택 28 BYTE 를 외부 입력으로 받아 원본이 쓰는 필드만 초기화한 뒤 실제 섹션 재생성 소비자(replace)가 돌려준 새 현재 스트림을 쓴다 →
//   end(signed)가 28 미만이면 최소 확장 부모(ensureMinimum)가 돌려준 새 스트림, 그것도 없으면 분리 할당(allocate) 뒤 원문 복사·0 채움·position/end 28 → 현재 스트림 bytes 를 돌려준다.
// changed(slot): manager 0 은 파일명 첫 BYTE 물리 접근 경계 오류 → '.' 로 시작하는 이름 진단 → isMine(+8)이면 transient(+12) 진단 뒤 isMine 을 다시 읽어 자연 반환 →
//   현재 inMission 진단 → Template 첫 DWORD 0 진단 → preventSave(+16)면 자연 반환 → 실제 파일 저장 부모(0044d8d0)를 부른다.
// 자료 형식: headers 는 Template 9×20 BYTE(Uint8Array 180, +0 manager, +8 isMine, +12 transient, +16 preventSave), 스트림은 { bytes: Uint8Array, position, end, present } 이다.
// 스트림/이름은 배열이나 (slot) => 값 getter 로 받으며 매 호출 현재 값을 다시 조회한다. inMission 은 { value: boolean } 소유자이다.
// CD 10.72 의 즉시 로딩 깊이 증감·진단 줄은 옮기지 않았다(웹은 10.78 만). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E55: Player Template State 이식으로 추가)

// 원본 getter/Notify 가 보고 뒤 계속하는 진단 종류이다(10.78 Template.cpp 줄 0x1fc/0x1c1/0x1c3/0x1c6/0x1c7). (2026-11-08 WB04-E55)
export const PLAYER_TEMPLATE_STATE_ASSERTION = Object.freeze({
    missingStateTemplate: 'MissingStateTemplate',
    dotFileName: 'DotFileName',
    mineTransientFlag: 'MineTransientFlag',
    inMissionSave: 'InMissionSave',
    missingNotifyTemplate: 'MissingNotifyTemplate',
});

// 미연결/계약 위반 경계의 오류 문구이다. 검사와 상위 연결이 경계 위치를 구분할 때 쓴다. (2026-11-08 WB04-E55)
export const PLAYER_TEMPLATE_STATE_BOUNDARY = Object.freeze({
    initialization: 'Player 빈 State의 미정 바이트/실제 섹션 생성 미연결',
    allocation: 'Player shortState 실제MemStream할당 미연결',
    save: 'Player Template실제DataManager파일저장44d8d0 미연결',
});

// State 최소 길이(28), Template 기록 수(9)와 기록 크기(20)이다. (2026-11-08 WB04-E55)
export const PLAYER_TEMPLATE_STATE_SIZE = 28;
const TEMPLATE_COUNT = 9;
const TEMPLATE_STRIDE = 20;

// 004bd550 이 빈 State 에 실제로 쓰는 필드만 초기화한다: 첫 BYTE 는 (b & 0xf9) | 1, 4~9·12~17·22~25 는 0. 나머지 바이트(상위 5비트 포함)는 보존한다.
// 사용: initializePlayerTemplateStateBytes(bytes28). (2026-11-08 WB04-E55)
export function initializePlayerTemplateStateBytes(bytes) {
    if (!(bytes instanceof Uint8Array) || bytes.length !== PLAYER_TEMPLATE_STATE_SIZE) throw new RangeError('Player State 초기화는 28 BYTE 가 필요합니다.');
    bytes[0] = (bytes[0] & 0xf9) | 1;
    bytes.fill(0, 4, 10);
    bytes.fill(0, 12, 18);
    bytes.fill(0, 22, 26);
}

// 배열 또는 getter 를 (slot) => 값 조회로 바꾼다. (2026-11-08 WB04-E55)
function lookup(source, name) {
    if (typeof source === 'function') return source;
    if (Array.isArray(source) && source.length === TEMPLATE_COUNT) return (slot) => source[slot];
    throw new TypeError(`Player Template ${name} 은 9칸 배열이나 getter 여야 합니다.`);
}

// Player Template State 서비스이다. Template 9 header·현재 State 스트림·파일 이름·inMission 을 같은 원본 전역으로 빌린다.
// 사용: const service = new RawPlayerTemplateState(headers, streams, names, mission, assertion, { allocate }); service.get(2); service.changed(2). (2026-11-08 WB04-E55)
export class RawPlayerTemplateState {
    // 실제 Template 9×20 BYTE(manager/isMine/transient/preventSave)를 빌린 저장소이다.
    #headers;
    // (slot) => 현재 State 스트림 조회이다(빌린 manager 의 같은 스트림/커서).
    #states;
    // (slot) => 현재 DataManager 파일 이름 조회이다.
    #names;
    // 실제 미션 생성자가 설정한 inMission 의 { value } 소유자이다.
    #mission;
    // 원본 보고 뒤 현재 조건을 다시 조회하는 진단 장치이다.
    #assertion;
    // 분리 스트림 입력의 확장용 할당(slot, size) → Uint8Array 이다(없으면 null).
    #allocate;
    // 분리 할당의 원문 복사가 끝난 뒤 이전 State 할당을 반납하는 장치이다(없으면 null).
    #allocationComplete;
    // 실제로 빈 State 가 선택됐을 때만 쓰는 { bytes, replace } 생성 경계이다.
    #initialization = null;
    // 실제 임시 섹션/이름 교체까지 수행하는 현재 최소 확장 부모이다.
    #ensureMinimum = null;
    // isMine/미션/누락/preventSave 관문 뒤에만 고르는 실제 파일 저장 부모이다.
    #save = null;

    // headers(180 BYTE)·스트림·이름·inMission 소유자·진단은 서비스보다 오래 살아야 한다. options 의 allocate(slot, size) → Uint8Array 는 분리 스트림 확장용,
    // allocationComplete(slot) 는 원문 복사 뒤 이전 할당 반납이다(실제 Session 은 bindMinimumSize 부모를 쓴다).
    constructor(headers, states, names, mission, assertion, { allocate = null, allocationComplete = null } = {}) {
        if (!(headers instanceof Uint8Array) || headers.length !== TEMPLATE_COUNT * TEMPLATE_STRIDE) throw new RangeError('Player Template 실제 9기록 크기 오류');
        if (typeof mission !== 'object' || mission === null || !('value' in mission)) throw new TypeError('Player Template inMission 은 { value } 소유자여야 합니다.');
        if (typeof assertion !== 'function') throw new TypeError('Player Template 진단 장치 누락');
        this.#headers = headers;
        this.#states = lookup(states, 'State');
        this.#names = lookup(names, '이름');
        this.#mission = mission;
        this.#assertion = assertion;
        this.#allocate = allocate;
        this.#allocationComplete = allocationComplete;
    }

    // 빈 State 를 실제 미정 입력(bytes(slot, out28))과 섹션 재생성 소비자(replace(slot, bytes28) → 새 현재 스트림)에 연결한다.
    bindInitialization(initialization) {
        if (typeof initialization?.bytes !== 'function' || typeof initialization?.replace !== 'function') throw new TypeError('빈 State의 미정 입력/실제 생성 연결 누락');
        this.#initialization = { bytes: initialization.bytes, replace: initialization.replace };
    }

    // 실제 최소 확장 부모(44d290: ensure(slot, 28) → 새 현재 스트림)를 연결한다.
    bindMinimumSize(ensure) {
        if (typeof ensure !== 'function') throw new TypeError('최소 State 확장의 실제 부모 연결 누락');
        this.#ensureMinimum = ensure;
    }

    // Notify 의 실제 파일 저장 부모(0044d8d0: save(slot))를 연결한다.
    bindFileSave(save) {
        if (typeof save !== 'function') throw new TypeError('Template Notify의 실제 파일 저장 연결 누락');
        this.#save = save;
    }

    // 현재 Template 한 기록의 DWORD 를 매 조회 읽는다.
    #header(slot, offset) {
        if (slot >= TEMPLATE_COUNT || offset > 16) throw new RangeError('Player Template 실제 9개/20BYTE 기록 범위');
        const at = slot * TEMPLATE_STRIDE + offset;
        const bytes = this.#headers;
        return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
    }

    // 004bd550: 현재 State 28 BYTE 이상을 돌려준다. 생성/확장된 경우에는 소비자가 돌려준 새 현재 스트림을 쓰며 이전 스트림을 계속 쓰지 않는다.
    get(slot) {
        const index = slot >>> 0;
        if (!this.#header(index, 0)) this.#assertion(PLAYER_TEMPLATE_STATE_ASSERTION.missingStateTemplate);
        if (!this.#header(index, 0)) throw new Error('Player State getter 실제 manager 0 메모리 경계');
        let current = this.#states(index);
        // 실제 lookupState(writable 1)의 현재 MemStream position 0 쓰기이다.
        current.position = 0;
        if (!current.present || !current.end) {
            const initialization = this.#initialization;
            if (!initialization) throw new Error(PLAYER_TEMPLATE_STATE_BOUNDARY.initialization);
            const bytes = new Uint8Array(PLAYER_TEMPLATE_STATE_SIZE);
            initialization.bytes(index, bytes);
            initializePlayerTemplateStateBytes(bytes);
            current = initialization.replace(index, bytes);
            if (!current?.present || current.end !== PLAYER_TEMPLATE_STATE_SIZE || current.position !== PLAYER_TEMPLATE_STATE_SIZE || current.bytes.length < PLAYER_TEMPLATE_STATE_SIZE) {
                throw new Error('Player 실제 빈 State 생성/쓰기 계약 위반');
            }
        }
        if ((current.end | 0) < PLAYER_TEMPLATE_STATE_SIZE && this.#ensureMinimum) {
            current = this.#ensureMinimum(index, PLAYER_TEMPLATE_STATE_SIZE);
            if (!current?.present || (current.end | 0) < PLAYER_TEMPLATE_STATE_SIZE || current.end > current.bytes.length) throw new Error('Player 최소 State 확장의 실제 현재 stream 계약 위반');
        }
        const stream = current;
        if ((stream.end | 0) < PLAYER_TEMPLATE_STATE_SIZE) {
            if (typeof this.#allocate !== 'function') throw new Error(PLAYER_TEMPLATE_STATE_BOUNDARY.allocation);
            // 원본 44d290 은 새 기록을 할당한 뒤 현재 원문의 길이/데이터를 다시 읽는다.
            const replacement = this.#allocate(index, PLAYER_TEMPLATE_STATE_SIZE);
            if (!(replacement instanceof Uint8Array) || replacement.length < PLAYER_TEMPLATE_STATE_SIZE || stream.end > PLAYER_TEMPLATE_STATE_SIZE || stream.end > stream.bytes.length) {
                throw new RangeError('Player shortState 현재 원문/새 할당 범위');
            }
            replacement.set(stream.bytes.subarray(0, stream.end), 0);
            replacement.fill(0, stream.end, PLAYER_TEMPLATE_STATE_SIZE);
            // 임시 MemStream 을 현재 State 로 바꾸며 position/end 는 둘 다 28 이다.
            stream.bytes = replacement.subarray(0, PLAYER_TEMPLATE_STATE_SIZE);
            stream.position = PLAYER_TEMPLATE_STATE_SIZE;
            stream.end = PLAYER_TEMPLATE_STATE_SIZE;
            // 원문을 모두 복사한 뒤 이전 실제 버퍼를 반납한다.
            if (typeof this.#allocationComplete === 'function') this.#allocationComplete(index);
        }
        if (stream.bytes.length < PLAYER_TEMPLATE_STATE_SIZE || stream.end > stream.bytes.length) throw new RangeError('Player 현재 State 물리 BYTE 범위');
        return stream.bytes;
    }

    // 004bd540/004bd360: isMine 재검사에 의한 자연 반환과 preventSave 거부 뒤에만 실제 저장 부모를 부른다.
    changed(slot) {
        const index = slot >>> 0;
        if (index >= TEMPLATE_COUNT) throw new RangeError('Player Notify 실제 9칸 범위');
        // 원본은 manager+c 의 첫 BYTE 를 진단보다 먼저 읽으므로 manager 0 은 물리 경계로 거부한다.
        if (!this.#header(index, 0)) throw new RangeError('Player Notify manager0 파일명 첫BYTE 물리접근경계');
        const name = String(this.#names(index) ?? '');
        // 빈 이름의 첫 NUL 도 유효한 첫 BYTE 이며 점 이름만 진단한다.
        if (name.charCodeAt(0) === 0x2e) this.#assertion(PLAYER_TEMPLATE_STATE_ASSERTION.dotFileName);
        if (this.#header(index, 8)) {
            if (this.#header(index, 12)) this.#assertion(PLAYER_TEMPLATE_STATE_ASSERTION.mineTransientFlag);
            if (this.#header(index, 8)) return;
        }
        if (this.#mission.value) this.#assertion(PLAYER_TEMPLATE_STATE_ASSERTION.inMissionSave);
        if (!this.#header(index, 0)) this.#assertion(PLAYER_TEMPLATE_STATE_ASSERTION.missingNotifyTemplate);
        if (this.#header(index, 16)) return;
        if (typeof this.#save !== 'function') throw new Error(PLAYER_TEMPLATE_STATE_BOUNDARY.save);
        this.#save(index);
    }
}
