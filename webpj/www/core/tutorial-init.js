// 튜토리얼/캠페인 미션 Init(004c3290)의 실제 설정 조회·이름 정규화·전역 쓰기·요새 로드 요청·기술 Header 순서를 옮긴 모듈이다(cpppj `client/TutorialInit.cpp/.h`, 10.78 전용). 큰 템플릿/Config/로드/기술 Header 몸체는 훅으로 같은 현재 Client 경계에 연결한다.
// initializeTutorialMission(state, flags, demo, globals, hooks) 순서:
//   ① clearTemplates() → tutorialNumber = atol(get('tutorialNumber','0')) 를 DWORD 로 쓰고 0 이 아니면 tutorialSeen = 1 → missionNumber = atol(get('missionNumber','0')) → get('demo', null) 의 첫 NUL 앞이 비어 있지 않을 때만 demo = atol 값 → resetOptions().
//   ② 큰 효과(resetOptions 등) 뒤의 **현재** state.name 을 첫 NUL 앞까지 읽는다(80바이트 이상이면 원본 오버런이라 RangeError). 지역 80바이트(scratch)에 이름을 복사해 NUL 을 쓰고, ASCII 소문자로 바꾼 이름을 NUL 없이 덮어쓴 뒤 첫 글자가 a..z 면 대문자로 바꾼다(원본 C locale 첫 글자 정규화, state.name 자체는 바꾸지 않음).
//   ③ namePrefix = scratch 첫 4바이트의 리틀엔디언 DWORD (짧은 이름의 NUL 뒤 caller 입력 꼬리를 지우지 않음) → set('fileName', scratch 의 이름 길이만큼) → loginSubscriber = 현재 initialSubscriber() → fort = get('loadFort', 현재 state.name 의 첫 NUL 앞)의 첫 NUL 앞(256바이트 이상이면 RangeError)
//   → requestFort(fort) → header() → flags |= 1.
// 도달한 실제 효과를 빈 성공으로 대체하지 않는다(누락은 거부, 앞선 효과는 보존). state = { name, namePrefix, scratch: Uint8Array(80) }, flags/demo 는 { value }, globals = { tutorialNumber, missionNumber, tutorialSeen, loginSubscriber: { value }, initialSubscriber: () => 현재 구독자 }.
// hooks = { clearTemplates, resetOptions, header, get(키, 대체|null) → 문자열, set(키, 값), requestFort(이름) } 이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E38: Tutorial Init 이식으로 추가)
import { asciiLower, configParseLong } from '../assets/config.js';

// 이름 지역 버퍼(local_150, 80바이트)와 로드 이름 버퍼(256바이트) 크기이다. (2026-11-08 WB04-E38)
const NAME_BUFFER = 80;
const FORT_BUFFER = 256;

// 도달한 실제 효과를 빈 성공으로 대체하지 않는다. (2026-11-08 WB04-E38)
function effect(hook) {
    if (typeof hook !== 'function') throw new TypeError('Tutorial Init 실제 효과 누락');
    return hook;
}

// 원본 C 문자열의 첫 NUL 뒤는 이름/설정/로드로 해석하지 않는다. (2026-11-08 WB04-E38)
function cString(value) {
    const end = value.indexOf('\0');
    return end < 0 ? value : value.slice(0, end);
}

// 이름 지역 80바이트·로드 256바이트 범위 밖의 원본 오버런만 안전한 호스트에서 명시 거부한다. (2026-11-08 WB04-E38)
export function initializeTutorialMission(state, flags, demo, globals, hooks) {
    effect(hooks.clearTemplates)();
    globals.tutorialNumber.value = configParseLong(effect(hooks.get)('tutorialNumber', '0')) >>> 0;
    if (globals.tutorialNumber.value) globals.tutorialSeen.value = 1;
    globals.missionNumber.value = configParseLong(effect(hooks.get)('missionNumber', '0')) >>> 0;
    const text = effect(hooks.get)('demo', null);
    if (cString(text) !== '') demo.value = configParseLong(text) >>> 0;
    effect(hooks.resetOptions)();
    const name = cString(state.name);
    if (name.length >= NAME_BUFFER) throw new RangeError('Tutorial Init 이름 80BYTE 초과');
    // 지역 사본에 이름과 NUL 을 쓰고, 소문자 이름으로 덮은 뒤 첫 글자만 대문자로 만든다.
    for (let index = 0; index < name.length; index += 1) state.scratch[index] = name.charCodeAt(index) & 0xff;
    state.scratch[name.length] = 0;
    const lower = asciiLower(name);
    for (let index = 0; index < lower.length; index += 1) state.scratch[index] = lower.charCodeAt(index) & 0xff;
    if (lower.length > 0 && state.scratch[0] >= 0x61 && state.scratch[0] <= 0x7a) state.scratch[0] -= 0x20;
    state.namePrefix = 0;
    // 정규화 지역의 실제 첫 4바이트를 x86 리틀엔디언 DWORD 로 읽는다. 짧은 이름의 꼬리를 지우지 않는다.
    for (let index = 0; index < 4; index += 1) state.namePrefix = (state.namePrefix | (state.scratch[index] << (index * 8))) >>> 0;
    effect(hooks.set)('fileName', String.fromCharCode(...state.scratch.subarray(0, name.length)));
    globals.loginSubscriber.value = globals.initialSubscriber() >>> 0;
    const fort = cString(effect(hooks.get)('loadFort', cString(state.name)));
    if (fort.length >= FORT_BUFFER) throw new RangeError('Tutorial Init loadFort 256BYTE 초과');
    effect(hooks.requestFort)(fort);
    effect(hooks.header)();
    flags.value = (flags.value | 1) >>> 0;
}
