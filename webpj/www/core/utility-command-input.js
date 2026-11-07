// 대본이 예약하는 일반 유틸리티 명령 다섯 개(TakeScreenShot·URLAlias·Run·MoveIsland·LookAtSid)를 기존 부모 장치에 연결하는 입력 분배 모듈이다(cpppj `client/UtilityCommandInput.cpp/.h`, 10.78 전용).
// Guide/Gump/뗌 뒤이며 Console/정지 관문보다 앞이다. 수정키·상위 표시 비트(0x03ffffff 밖)를 지운 코드가 다섯 명령이 아니면 false, 대상의 뗌은 효과 없이 true 이다.
//  - TakeScreenShot(0x0200001c): screenshot() 뒤 tell('TellGifWritten'). screenshot 이 실패하면 Tell 은 실행하지 않는다.
//  - URLAlias(0x0200005a): text(y 전체 DWORD)의 첫 NUL 까지에 ' URL' 을 붙여 키를 만들고(256바이트 이상이면 원문 버퍼 밖 쓰기라 RangeError), readString(키, holder{value})이 찾았을 때만 설정값의 첫 NUL 까지를 url() 에 넘긴다(역시 256 이상이면 RangeError). 조회 실패면 링크 정책을 실행하지 않는다.
//  - Run(0x020000da): text(y)의 첫 NUL 까지를 run(문자열, 1)에 넘긴다. MoveIsland(0x02000006): y 를 읽지 않고 highlight(6). LookAtSid(0x020000aa): y 의 전체 DWORD 를 center(sid)에 넘긴다.
// 문자열은 원본 바이트를 글자 하나당 바이트 하나(0..255)로 담은 JS 문자열이다. 선택된 장치의 누락은 그 위치에서 거부한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E25: 일반 유틸리티 명령 이식으로 추가)
import { InputCode } from './input-queue.js';
import { asciiLower } from '../assets/config.js';

// 명령 번호이다(542768/2860/2a5c 등 원문 등록 명령의 초기 DWORD). (2026-11-08 WB04-E25)
export const TAKE_SCREENSHOT_COMMAND = 0x0200001c;
export const URL_ALIAS_COMMAND = 0x0200005a;
export const RUN_COMMAND = 0x020000da;
export const MOVE_ISLAND_COMMAND = 0x02000006;
export const LOOK_AT_SID_COMMAND = 0x020000aa;
// 수정키 제거 마스크이다. (2026-11-08 WB04-E25)
const KEY_MASK = 0x03ffffff;
// 원문 등록 이름표(소문자)이며 별칭을 추측해 추가하지 않는다. (2026-11-08 WB04-E25)
const COMMAND_NAMES = new Map([
    ['takescreenshot', TAKE_SCREENSHOT_COMMAND],
    ['urlalias', URL_ALIAS_COMMAND],
    ['run', RUN_COMMAND],
    ['moveisland', MOVE_ISLAND_COMMAND],
    ['lookatsid', LOOK_AT_SID_COMMAND],
]);
// 원문 URL 별칭 키/설정값 버퍼 길이(바이트)이다. 이 길이 이상이면 원문은 버퍼 밖에 쓴다. (2026-11-08 WB04-E25)
const ALIAS_BUFFER = 256;

// 선택된 장치가 없으면 빈 성공으로 바꾸지 않고 거부한다. (2026-11-08 WB04-E25)
function need(device) {
    if (typeof device !== 'function') throw new Error('일반 유틸리티 명령의 실제 효과 미연결');
    return device;
}

// strcpy 처럼 첫 NUL 까지만 소유한다. (2026-11-08 WB04-E25)
function untilNul(text) {
    const end = text.indexOf('\0');
    return end < 0 ? text : text.slice(0, end);
}

// 이름을 ASCII 대소문자 무시로 조회하고 미등록이면 0xffffffff 를 돌려준다. (2026-11-08 WB04-E25)
export function utilityCommandScriptCode(name) {
    return COMMAND_NAMES.get(asciiLower(name)) ?? 0xffffffff;
}

// 상위 입력 표시를 제외한 명령 번호 전체가 다섯 명령 중 하나인지이다. (2026-11-08 WB04-E25)
export function isUtilityCommand(code) {
    const value = (code & KEY_MASK) >>> 0;
    for (const command of COMMAND_NAMES.values()) if (value === command) return true;
    return false;
}

// event = { code, x, y }, hooks = { text(token), screenshot(), tell(section), readString(key, holder{value}) → 찾았는지, url(text), run(text, mode), highlight(mode), center(sid) } 이다. 소비하면 true 이다. (2026-11-08 WB04-E25)
export function dispatchUtilityCommandInput(event, hooks) {
    const code = (event.code & KEY_MASK) >>> 0;
    if (!isUtilityCommand(code)) return false;
    if ((event.code & InputCode.release) !== 0) return true;
    // 두 번째 인자는 전체 DWORD 로 보존한다.
    const parameter = event.y >>> 0;
    if (code === TAKE_SCREENSHOT_COMMAND) {
        need(hooks.screenshot)();
        need(hooks.tell)('TellGifWritten');
    } else if (code === URL_ALIAS_COMMAND) {
        // 설정 조회 전에 별칭 문자열과 접미사를 완성한다.
        const key = `${untilNul(need(hooks.text)(parameter))} URL`;
        if (key.length >= ALIAS_BUFFER) throw new RangeError('URLAlias 의 원문 256바이트 키 범위');
        const holder = { value: '' };
        if (need(hooks.readString)(key, holder)) {
            const target = untilNul(holder.value);
            if (target.length >= ALIAS_BUFFER) throw new RangeError('URLAlias 의 원문 256바이트 설정값 범위');
            need(hooks.url)(target);
        }
    } else if (code === RUN_COMMAND) need(hooks.run)(untilNul(need(hooks.text)(parameter)), 1);
    else if (code === MOVE_ISLAND_COMMAND) need(hooks.highlight)(6);
    else need(hooks.center)(parameter);
    return true;
}
