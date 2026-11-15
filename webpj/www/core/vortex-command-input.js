// 신전 가상 6c 의 확인 WORD 와 일반 Descend/ViewMeta 의 서버·지역 WORD 를 실제 입력에서 보존하는 일반 입력 분배 모듈이다(cpppj `client/VortexCommandInput.cpp/.h`, 10.78 전용).
// 사용:
//   vortexCommandScriptCode(name) → 0x0300006f(WipeOut) | 0x0300007c(Ascend) | 0x030000a7(Descend) | 0x02000013(ViewMeta) | 0xffffffff
//     보호 PE 이름표(542f00/542f68/5430c0/542c20)의 네 이름만 ASCII 대소문자를 무시하고 조회하며 별칭을 추가하지 않는다(미등록은 공통 조회를 이어간다).
//   dispatchVortexCommandInput(event, command) → 소비하면 true
//     event = { code, x, y }, command(target, code, parameter) = 실제 객체 명령 부모(Session.CommandObject)이다. 수정키를 지운 코드가 세 신전 명령이 아니면 false,
//     뗌이면 효과 없이 true, 그 밖은 두 번째 인자 y 의 낮은 WORD 를 대상 SID 로, 높은 WORD 를 확인 인자로 전달한다(대상 0 이면 장치를 요구하지 않고 true).
//   dispatchViewMetaInput(event, command, hooks) → 소비하면 true
//     hooks = { serverCount() → signed 서버 행 수, invalidServer(), descend(지역 WORD) }. 현재 ViewMeta 명령이 아니면 false, 뗌이면 true, signed 서버 WORD 가 -1 보다 작거나
//     목록 상한 이상이면 invalidServer(19번 진단 뒤 curServer "0" 쓰기)를, 아니면 낮은 WORD 지역으로 descend 를 요청한다(서버 선택 전역은 여기서 쓰지 않는다).
// 선택된 원문 분기에서 도달한 장치만 요구하며 미연결은 성공으로 대체하지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// 원문에 이 분배기만 따로 관측한 fixture 는 없다(cpppj 에도 전용 시험이 없다). 동작은 cpppj 소스의 분기·단락·WORD 폭을 직접 대조하는 계약 시험으로 확인한다(tests/vortex-command-input.test.mjs).
// (2026-11-15 WB04-E536: 6단계 게임 메뉴 명령 표의 선행 부품 VortexCommandInput 이식)
import { InputCode } from './input-queue.js';
import { asciiLower } from '../assets/config.js';

// 수정키를 지운 코드 마스크이다. (2026-11-15 WB04-E536)
const KEY_MASK = 0x03ffffff;

// 신전 가상 6c 로 가는 세 명령이다(WipeOut·Ascend·Descend: 보호 PE 5428b4/5428e8/542990). (2026-11-15 WB04-E536)
export const VORTEX_COMMANDS = Object.freeze([0x0300006f, 0x0300007c, 0x030000a7]);

// 보호 PE 이름표의 네 항목이다. 이름은 소문자로 비교한다. (2026-11-15 WB04-E536)
const NAMES = Object.freeze([['wipeout', 0x0300006f], ['ascend', 0x0300007c], ['descend', 0x030000a7], ['viewmeta', 0x02000013]]);

// 도달한 실제 원문 효과만 요구한다. 미선택 경계는 요구하지 않는다. (2026-11-15 WB04-E536)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('신전/Descend 입력의 실제 장치 미연결');
    return hook;
}

// 원문 이름표는 ASCII 대소문자만 무시한다. 네 이름 이외에는 공통 조회를 계속하도록 0xffffffff 이다. (2026-11-15 WB04-E536)
export function vortexCommandScriptCode(name) {
    const value = asciiLower(name);
    // 네 이름을 순서대로 비교한다.
    for (const [text, code] of NAMES) if (text === value) return code;
    return 0xffffffff;
}

// 세 신전 명령은 y 의 낮은 WORD 가 대상 SID, 높은 WORD 가 확인 인자이며 대상 0 이면 객체 명령을 부르지 않는다(반환값은 무시). (2026-11-15 WB04-E536)
export function dispatchVortexCommandInput(event, command) {
    const code = (event.code & KEY_MASK) >>> 0;
    if (!VORTEX_COMMANDS.includes(code)) return false;
    if ((event.code & InputCode.release) !== 0) return true;
    const parameter = event.y >>> 0;
    const target = parameter & 0xffff;
    if (target) need(command)(target, event.code >>> 0, parameter >>> 16);
    return true;
}

// 현재 ViewMeta 명령이면 signed 서버 WORD 의 범위를 검사해 잘못된 서버 안내 또는 낮은 WORD 지역 Descend 를 요청한다. (2026-11-15 WB04-E536)
export function dispatchViewMetaInput(event, command, hooks) {
    if (((event.code & KEY_MASK) >>> 0) !== (command >>> 0)) return false;
    if ((event.code & InputCode.release) !== 0) return true;
    // 산술 시프트로 signed 서버 WORD 를 얻는다(-1 은 서버 없음).
    const server = (event.y | 0) >> 16;
    if (server < -1 || server >= need(hooks.serverCount)()) need(hooks.invalidServer)();
    else need(hooks.descend)((event.y >>> 0) & 0xffff);
    return true;
}
