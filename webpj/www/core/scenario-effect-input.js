// 대본이 예약하는 시나리오 표시 명령 네 개(StartSmoke·Lightning·CreateRandomGuns·DumpSquids)를 현재 월드와 표시 생성 부모에 연결하는 입력 분배 모듈이다(cpppj `client/ScenarioEffectInput.cpp/.h`, 10.78 전용, 원문 4d77f3/4d7f45).
// 수정키를 지운 코드가 네 명령이 아니면 false 이다. 키 뗌과 CreateRandomGuns/DumpSquids 는 장치를 요구하지 않고 효과 없이 true 이다(두 이름은 일반 입력에서 원문도 무동작이며 개발자 F7 덤프와 구별).
//  - StartSmoke(0x02000035): smoke(y 전체 DWORD, 0, 0). 메뉴/소유자/Edit/미션 관문이 없다.
//  - Lightning(0x020000b6): width() 를 먼저 읽은 뒤 technology()(현재 기술 부모 조회)를 호출하고, 기술 조회가 폭을 바꿔도 먼저 읽은 폭으로 lightning(부모, [0,60], [폭,60], 20.0)을 호출한다.
// 선택된 표시 생성 부모의 누락은 조용한 성공으로 바꾸지 않고 거부한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E25: 시나리오 표시 입력 이식으로 추가)
import { InputCode } from './input-queue.js';
import { asciiLower } from '../assets/config.js';

// 원문 이름표(5427cc/5429cc/542acc)의 서로 다른 명령 번호이다. DumpSquids 는 00543418→00542b3c 의 등록 이름이다. (2026-11-08 WB04-E25)
export const SMOKE_COMMAND = 0x02000035;
export const LIGHTNING_COMMAND = 0x020000b6;
export const RANDOM_GUNS_COMMAND = 0x020000f6;
export const DUMP_SQUIDS_COMMAND = 0x0200011b;
// 수정키 제거 마스크와 번개 수명(float 20.0)·높이 상수이다. (2026-11-08 WB04-E25)
const KEY_MASK = 0x03ffffff;
const LIGHTNING_LIFE = 20;
const LIGHTNING_Y = 60;

// 선택된 장치가 없으면 빈 성공으로 바꾸지 않고 거부한다. (2026-11-08 WB04-E25)
function need(device) {
    if (typeof device !== 'function') throw new Error('시나리오 표시의 실제 장치 미연결');
    return device;
}

// 원문 등록 철자 네 개만 ASCII 대소문자 무시로 조회하고 미등록이면 0xffffffff 이다. (2026-11-08 WB04-E25)
export function scenarioEffectScriptCode(name) {
    const key = asciiLower(name);
    if (key === 'startsmoke') return SMOKE_COMMAND;
    if (key === 'lightning') return LIGHTNING_COMMAND;
    if (key === 'createrandomguns') return RANDOM_GUNS_COMMAND;
    if (key === 'dumpsquids') return DUMP_SQUIDS_COMMAND;
    return 0xffffffff;
}

// 수정키를 제거한 명령 번호가 네 명령 중 하나인지이다. (2026-11-08 WB04-E25)
export function isScenarioEffectCommand(code) {
    const value = (code & KEY_MASK) >>> 0;
    return value === SMOKE_COMMAND || value === LIGHTNING_COMMAND || value === RANDOM_GUNS_COMMAND || value === DUMP_SQUIDS_COMMAND;
}

// event = { code, x, y }, hooks = { smoke(parent, 0, 0), width(), technology(), lightning(parent, [x,y], [x,y], life) } 이다. 소비하면 true 이다. (2026-11-08 WB04-E25)
export function dispatchScenarioEffectInput(event, hooks) {
    const code = (event.code & KEY_MASK) >>> 0;
    if (!isScenarioEffectCommand(code)) return false;
    if ((event.code & InputCode.release) !== 0 || code === RANDOM_GUNS_COMMAND || code === DUMP_SQUIDS_COMMAND) return true;
    if (code === SMOKE_COMMAND) need(hooks.smoke)(event.y >>> 0, 0, 0);
    else {
        const width = need(hooks.width)();
        const parent = need(hooks.technology)();
        need(hooks.lightning)(parent, [0, LIGHTNING_Y], [width, LIGHTNING_Y], LIGHTNING_LIFE);
    }
    return true;
}
