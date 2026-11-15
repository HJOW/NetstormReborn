// Client 의 180초 시간 알림과 Chat front 서식의 임시 교체를 원문 순서로 옮긴 모듈이다(cpppj `client/RawMonitorTime.cpp/.h`, 10.78 전용).
// 장기 현재값 `MonitorTimeState` 는 원문 00544d68/60 과 0054db9c/dba0 의 최초 BSS 0 이며 Session 교체로 초기화하지 않는다.
//  - initialized(DWORD): 첫 호출에서 bit0 만 켜며 다른 비트는 보존한다. deadline(double): 최초 guard 에서 0 을 쓰고 비활성 때 비영 기한을 지우지 않는다.
//  - monitor/alert(int32): 두 ReadInt 현재 DWORD 이며 비영 관문을 bool 로 정규화하지 않는다(쓰기는 signed 32비트로 감는다).
// 사용:
//   stepMonitorTime(state, { wallSeconds(), calendar(), handle(), line?(handle, calendar), format(), submit(format, line) }) — Wind 뒤 Movie 갱신 잠금 없는 가지에서 실행한다. 최초 guard → 두 옵션이 모두 0 이면 끝 →
//     기한 0 이면 벽시각 + 180 예약 → 새 벽시각이 기한 이상이면 달력·이름·한 줄·서식을 만들어 제출하고 전달 뒤 벽시각 + 180 으로 재예약한다. 매 시계 호출은 서로 다른 현재 벽시각이며 도달한 장치만 요구한다.
//   formatMonitorTimeLine(handle, calendar) — 원문 `%s: !%s\n` 의 지역 256BYTE 경계와 두 끝 개행을 보존한다(NUL 뒤는 읽지 않음).
//   monitorCalendarText(epochSeconds) — 원문 32비트 time → localtime → asctime 의 26BYTE 표준 출력(끝 개행 포함 25글자)을 만든다(웹은 브라우저 현지 시간, 32비트 범위 밖은 거부).
//   submitTemporaryChatFormat(current, format, line, edition, hooks) — 최초 front 를 저장하고 임시 서식을 쓴 뒤 일반 submit 복귀에서만 현재 manager 에 복원한다. 판본별 10.78: +0x14·60BYTE, CD 10.72: +0x10·16BYTE.
//     current 는 `{ value }`(현재 manager 토큰), hooks = { memory(토큰) → 실제 물리 Uint8Array, submit(토큰, line) }. submit 예외에서는 원문처럼 앞선 서식 변경을 남긴다.
// 원본 관측: cpppj `monitor-time-hardware-x86.tsv` 1176행(guard/기한/옵션/시각/본문/서식/요청 순서)과 `temporary-chat-format-hardware-x86.tsv` 12행(60/16BYTE 절삭·패딩·현재 manager 재조회)을 tests/monitor-time.test.mjs 가 대조한다.
// 달력(`monitorCalendarText`)과 달리 `Client` 의 Config writer(monitor/alert 옵션 연결)·실제 Chat manager 부모는 4-D Client/채팅 조립의 몫이다. 이 모듈은 DOM 을 쓰지 않는다.
// 기준선 뒤 cpppj 변경 없음(기준선 `225553df` 와 HEAD 소스 동일, 2026-11-15 확인).
// (2026-11-12 WB04-E335: 4-D ClientLocalStateGlobals 의 하위 소유자로 상태 구조체만 추가, 2026-11-15 WB04-E526: StepMonitorTime·FormatMonitorTimeLine·MonitorCalendarText·SubmitTemporaryChatFormat 이식)
import { copyChatManagerFormat } from './raw-chat-manager-prefix.js';

// 시간 알림의 장기 현재값이다. 사용: const state = new MonitorTimeState(); state.monitor = 1; 필드는 모두 원문 폭으로 감겨 저장된다. (2026-11-12 WB04-E335)
export class MonitorTimeState {
    // 첫 bit0 만 켜는 초기화 표식(DWORD)이다.
    #initialized = 0;
    // 다음 알림 벽시각 기한(double)이다. 0 은 예약 없음이다.
    deadline = 0;
    // monitor/alert 두 옵션의 signed DWORD 이다.
    #monitor = 0;
    #alert = 0;

    get initialized() { return this.#initialized; }

    set initialized(next) { this.#initialized = next >>> 0; }

    get monitor() { return this.#monitor; }

    set monitor(next) { this.#monitor = next | 0; }

    get alert() { return this.#alert; }

    set alert(next) { this.#alert = next | 0; }
}

// 양 판본 PE 005019e0/005074e0 의 double 180 알림 간격(초)이다. (2026-11-15 WB04-E526)
const MONITOR_INTERVAL = 180;
// 원문 지역 `%s: !%s\n` 버퍼 크기, 임시 Chat front 의 지역 80BYTE 상한, 판본별 front 위치/복사 길이이다. (2026-11-15 WB04-E526)
const LINE_LIMIT = 256;
const FRONT_LIMIT = 80;
const PATCH_FRONT = { offset: 0x14, count: 60 };
const CD_FRONT = { offset: 0x10, count: 16 };

// 미도달 장치는 요구하지 않고 도달한 장치만 명시 거부한다(빈 성공으로 대신하지 않음). (2026-11-15 WB04-E526)
function need(device) {
    if (typeof device !== 'function') throw new Error('시간 알림의 실제 장치 미연결');
    return device;
}

// 원문 NUL 문자열을 실제 manager 범위에서 읽고 지역 80BYTE 초과는 명시 거부한다. 반환은 바이트를 문자 하나로 담은 문자열이다. (2026-11-15 WB04-E526)
function front(bytes, offset) {
    if (offset >= bytes.length) throw new RangeError('임시 Chat front 물리 부족');
    const view = bytes.subarray(offset);
    const end = view.indexOf(0);
    if (end < 0 || end >= FRONT_LIMIT) throw new RangeError('임시 Chat front 지역80BYTE 범위');
    return Array.from(view.subarray(0, end), (byte) => String.fromCharCode(byte)).join('');
}

// 실제 strncpy 60/16 의 padding 과 마지막 BYTE 0 을 같은 물리에만 적용한다. (2026-11-15 WB04-E526)
function setFront(bytes, offset, count, text) {
    if (offset > bytes.length || count > bytes.length - offset) throw new RangeError('임시 Chat front setter 범위');
    copyChatManagerFormat(bytes.subarray(offset), text, count);
    bytes[offset + count - 1] = 0;
}

// NUL 앞까지만 읽는 C 문자열 절단이다. (2026-11-15 WB04-E526)
function cString(text) {
    const end = text.indexOf('\0');
    return end < 0 ? text : text.slice(0, end);
}

// 달력 문자열의 기존 개행과 추가 개행을 모두 보존하며 NUL 이후는 CRT 처럼 읽지 않는다. 결과가 256BYTE 이상이면 지역 버퍼 범위를 거부한다. (2026-11-15 WB04-E526)
export function formatMonitorTimeLine(handle, calendar) {
    const line = `${cString(handle)}: !${cString(calendar)}\n`;
    if (line.length >= LINE_LIMIT) throw new RangeError('시간 알림 원문 지역256BYTE 범위');
    return line;
}

// 시계/달력/Config/submit callback 뒤의 현재 필드를 원문 위치에서 다시 읽는다. (2026-11-15 WB04-E526)
export function stepMonitorTime(state, hooks) {
    if (!(state.initialized & 1)) {
        state.initialized |= 1;
        state.deadline = 0;
    }
    if (!state.monitor && !state.alert) {
        if (state.deadline === 0) state.deadline = 0;
        return;
    }
    if (state.deadline === 0) state.deadline = need(hooks.wallSeconds)() + MONITOR_INTERVAL;
    const now = need(hooks.wallSeconds)();
    if (!(state.deadline <= now)) return;
    const calendar = need(hooks.calendar)();
    const handle = need(hooks.handle)();
    const line = need(hooks.line ?? formatMonitorTimeLine)(handle, calendar);
    const format = need(hooks.format)();
    need(hooks.submit)(format, line);
    state.deadline = need(hooks.wallSeconds)() + MONITOR_INTERVAL;
}

// 영문 요일/월 표(asctime 의 Sun..Sat·Jan..Dec)이다. (2026-11-15 WB04-E526)
const DAYS = Object.freeze(['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat']);
const MONTHS = Object.freeze(['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']);

// 원문과 같은 32비트 시간 범위/영문 요일·월·끝 개행을 사용한다(`Www Mmm dd hh:mm:ss yyyy\n`, 일은 공백 채움 두 칸). 범위 밖 시각은 CRT 실패처럼 거부한다. (2026-11-15 WB04-E526)
export function monitorCalendarText(epochSeconds = Math.floor(Date.now() / 1000)) {
    if (!Number.isFinite(epochSeconds) || epochSeconds < 0 || epochSeconds > 0x7fffffff) throw new RangeError('시간 알림 time32 범위 오류');
    const date = new Date(epochSeconds * 1000);
    const two = (value) => String(value).padStart(2, '0');
    return `${DAYS[date.getDay()]} ${MONTHS[date.getMonth()]} ${String(date.getDate()).padStart(2, ' ')} ${two(date.getHours())}:${two(date.getMinutes())}:${two(date.getSeconds())} ${date.getFullYear()}\n`;
}

// submit 예외에서는 원문처럼 앞선 서식 변경을 남긴다. 정상 복귀에서만 현재 manager 를 다시 읽어 복원한다. edition 이 'patch1078' 이 아니면 CD 10.72 의 +0x10·16BYTE 위치를 쓴다. (2026-11-15 WB04-E526)
export function submitTemporaryChatFormat(current, format, line, edition, hooks) {
    const { offset, count } = edition === 'patch1078' ? PATCH_FRONT : CD_FRONT;
    const initial = current.value;
    const saved = front(need(hooks.memory)(initial), offset);
    setFront(need(hooks.memory)(initial), offset, count, format);
    need(hooks.submit)(current.value, line);
    setFront(need(hooks.memory)(current.value), offset, count, saved);
}
