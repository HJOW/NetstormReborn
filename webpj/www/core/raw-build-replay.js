// RawBuildReplay의 200행 실제 물리·한 프레임 실행/라벨 조회·통지를 옮긴다(225553df, WB04-E601).
// 사용: 호출자가 현재 상태/playing 상자·패킷과 실제 hooks를 준다. FILE 판독 어댑터는 후속 Client 설치 구간이다.

// 원문 고정 행 수·행 크기와 C++ 상태의 실제 이벤트/시각·인덱스/개수 크기다.
export const BUILD_REPLAY_EVENT_LIMIT = 200;
export const BUILD_REPLAY_EVENT_BYTES = 40;
export const BUILD_REPLAY_STATE_BYTES = 8016;
const EVENTS_BYTES = BUILD_REPLAY_EVENT_LIMIT * BUILD_REPLAY_EVENT_BYTES;

// 선택된 실제 하위 함수만 원문 위치에서 요구한다.
function need(fn) { if (typeof fn !== 'function') throw new Error('건설 기록 재생의 실제 하위 미연결'); return fn; }

// 원문40BYTE의 같은 물리 행을 빌린다. 좌표/지연 저장은 단정도이며 이름과 정수는 비트를 유지한다.
export class BuildReplayEvent {
    // 외부 상태가 소유한 같은40BYTE와 비정렬 필드 뷰다.
    #raw; #view;
    // 새 행 사본을 만들지 않고 caller의 실제 구간만 빌린다.
    constructor(raw) { this.#raw = raw; this.#view = new DataView(raw.buffer, raw.byteOffset, BUILD_REPLAY_EVENT_BYTES); }
    // 주소 자체를 교체하지 않고 같은 행의 쓰기 가능한 바이트를 빌린다.
    get raw() { return this.#raw; }
    // NUL 포함20BYTE 이름의 현재 쓰기 가능한 구간이다.
    get name() { return this.raw.subarray(0, 20); }
    // 같은 원문 단정도 X/Y와 signed 인자/회전·지연이다. 쓰기는 해당 필드만 바꾼다.
    get x() { return this.#view.getFloat32(20, true); } set x(value) { this.#view.setFloat32(20, value, true); }
    get y() { return this.#view.getFloat32(24, true); } set y(value) { this.#view.setFloat32(24, value, true); }
    get argument() { return this.#view.getInt32(28, true); } set argument(value) { this.#view.setInt32(28, value, true); }
    get rotation() { return this.#view.getInt32(32, true); } set rotation(value) { this.#view.setInt32(32, value, true); }
    get delay() { return this.#view.getFloat32(36, true); } set delay(value) { this.#view.setFloat32(36, value, true); }
    // bit_cast가 필요한 현재 좌표/인자 DWORD를 산술 float 변환 없이 읽는다.
    bits(offset) { return this.#view.getUint32(offset, true); }
}

// 이벤트200개와 double 이전 시각·현재/개수 DWORD를 실제 C++ 구조체 배치로 소유한다.
export class BuildReplayState {
    // 같은 원문 상태 바이트와 고정 행 뷰다. 미판독 tail은 호출자의 입력을 보존한다.
    #raw; #events; #view;
    // 기본0은 C++ 호스트의 명시 초기값이며 원본 미정 heap의 관측값이라고 주장하지 않는다.
    constructor(raw = new Uint8Array(BUILD_REPLAY_STATE_BYTES)) {
        if (!(raw instanceof Uint8Array) || raw.length < BUILD_REPLAY_STATE_BYTES) throw new TypeError('건설 재생 실제 상태8016BYTE 필요');
        this.#raw = raw.subarray(0, BUILD_REPLAY_STATE_BYTES); this.#view = new DataView(raw.buffer, raw.byteOffset, BUILD_REPLAY_STATE_BYTES);
        // 각 뷰는 같은 이벤트 행만 빌리며 나머지 원문 물리를 복사/초기화하지 않는다.
        this.#events = Object.freeze(Array.from({ length: BUILD_REPLAY_EVENT_LIMIT }, (_, index) => new BuildReplayEvent(this.raw.subarray(index * BUILD_REPLAY_EVENT_BYTES, (index + 1) * BUILD_REPLAY_EVENT_BYTES))));
    }
    // 같은 실제 구조체와 고정 행 뷰이며 필드에 쓰되 주소를 새 배열로 교체하지 않는다.
    get raw() { return this.#raw; } get events() { return this.#events; }
    // 원문 double 시각과 unsigned32 저장/읽기다. signed 비교는 Run의 실제 위치에서만 한다.
    get previousTime() { return this.#view.getFloat64(EVENTS_BYTES, true); } set previousTime(value) { this.#view.setFloat64(EVENTS_BYTES, value, true); }
    get current() { return this.#view.getUint32(EVENTS_BYTES + 8, true); } set current(value) { this.#view.setUint32(EVENTS_BYTES + 8, value, true); }
    get count() { return this.#view.getUint32(EVENTS_BYTES + 12, true); } set count(value) { this.#view.setUint32(EVENTS_BYTES + 12, value, true); }
}

// count 이후라도 같은200행 물리는 조회하고, 실제 배열 밖 주소만 거부한다.
function current(state) { const index = state.current; if (index >= BUILD_REPLAY_EVENT_LIMIT) throw new RangeError('건설 재생 현재 이벤트 물리 범위'); return state.events[index]; }
// 현재 원문 BYTE의 첫 NUL까지만 읽는다. 라벨은 접두 검사 없이 두 BYTE를 생략한다.
function name(event, offset = 0) {
    const bytes = event.name; let result = '';
    // 두 번째 BYTE 앞의 NUL을 라벨 구간의 종료로 대신하지 않는다.
    for (let index = offset; index < bytes.length; index += 1) { if (bytes[index] === 0) return result; result += String.fromCharCode(bytes[index]); }
    throw new RangeError('건설 재생 이벤트 이름 NUL 누락');
}
// 정해진 폭의 lowBYTE만 쓰며 품질/시각/인접 필드는 보존한다.
function put(bytes, offset, value, width = 4) { for (let index = 0; index < width; index += 1) bytes[offset + index] = value >>> (index * 8); }

//4957a0: 한 프레임 한 이벤트만 소비한다. playing은 같은 현재 DWORD의 {value} 참조이고 packet은 최소25BYTE다.
export function runBuildReplayFrame(state, playing, packet, hooks) {
    const index = state.current;
    if ((index | 0) >= (state.count | 0)) { playing.value = 0; need(hooks.kill)(); return; }
    const event = current(state);
    if (!(need(hooks.now)() - state.previousTime > event.delay)) return;
    state.previousTime = need(hooks.now)(); state.current = index + 1;
    if (!need(hooks.validPoint)(event.x, event.y)) return;
    if (!(packet instanceof Uint8Array) || packet.length < 25) throw new RangeError('건설 재생 송신의 현재25BYTE 부족');
    need(hooks.constructPacket)(packet);
    const type = need(hooks.findType)(name(event)); put(packet, 9, event.bits(24)); packet[4] = type; put(packet, 5, event.bits(20)); put(packet, 13, event.bits(28), 2); packet[15] = event.rotation;
    need(hooks.send)(9, packet.subarray(0, 25));
}

//495710: 현재 이벤트의 보드 유효성만 판정한다. 이름 접두/count로 라벨을 추정하지 않는다.
export function isBuildReplayLabel(state, hooks) { const event = current(state); return !need(hooks.validPoint)(event.x, event.y); }
//495730: NULL callback은 실제 이벤트 조회보다 먼저 생략한다. 좌표 판정 효과 뒤 같은 행의 현재 이름을 읽는다.
export function notifyBuildReplayLabel(state, sid, hooks) {
    if (!hooks.label) return;
    const event = current(state); const label = need(hooks.validPoint)(event.x, event.y) ? '' : name(event, 2); hooks.label(label, sid >>> 0);
}
