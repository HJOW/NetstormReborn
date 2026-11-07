// AI 진단 일반 명령 두 개(ToggleReporting·ToggleDebug)를 현재 brain reporting 토글과 알려진 진단 DWORD 주소 해석에 연결하는 입력 분배 모듈이다(cpppj `client/AiDebugCommandInput.cpp/.h`, 10.78 전용, 원문 413d50).
//  - ToggleReporting(0x020000d0): 메타맵이 아니고 y>0 일 때만 y 를 Player owner(9 이상이면 RangeError)로 보고, Player 행의 +0xb0 brain 토큰이 있으면 그 본문의 +0xb00 DWORD 를 0↔1 로 뒤집는다(앞의 물리 읽기가 성공한 뒤 같은 DWORD 전체를 덮어씀).
//  - ToggleDebug(0x020000d1): y 전체 DWORD 를 알려진 안정 전역의 주소 식별자로 보고 readDebug(주소)가 29 이면 99, 아니면 29 를 writeDebug(주소, 값)로 쓴다. 임의 32비트 포인터를 호스트 주소로 역참조하지 않는다.
// applyAiDebugCommandState 는 뗌을 가리지 않는 원문 raw child 이고, dispatchAiDebugCommandInput 은 공통 뗌 관문을 보존해(뗌은 소비만) 상위에서 쓴다. 선택된 하위 물리의 누락은 그 접근에서 거부하고 부분 쓰기는 되돌리지 않는다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E27: AI 진단 일반 명령 이식으로 추가)
import { InputCode } from './input-queue.js';
import { asciiLower } from '../assets/config.js';

// 두 명령의 보호 PE 초기값(00542a34/38)이다. (2026-11-08 WB04-E27)
export const AI_REPORTING_COMMAND = 0x020000d0;
export const DEBUG_SWITCH_COMMAND = 0x020000d1;
// 수정키 제거 마스크, Player 행의 brain 토큰 위치, brain 본문의 reporting DWORD 위치, Player 수(0..8)이다. (2026-11-08 WB04-E27)
const KEY_MASK = 0x03ffffff;
const BRAIN_TOKEN_OFFSET = 0xb0;
const REPORTING_OFFSET = 0xb00;
const PLAYER_COUNT = 9;

// 선택된 하위 경계가 없으면 빈 성공으로 세지 않고 거부한다. (2026-11-08 WB04-E27)
function need(device) {
    if (typeof device !== 'function') throw new Error('AI 진단 입력 실제 경계 미연결');
    return device;
}

// 원문 DWORD 읽기 폭을 보존한다. 범위 밖이면 RangeError 이다. (2026-11-08 WB04-E27)
function word(bytes, offset) {
    if (!bytes || offset > bytes.length || bytes.length - offset < 4) throw new RangeError('AI 진단 현재 DWORD 물리범위');
    let value = 0;
    // 낮은 바이트부터 조합한다.
    for (let index = 0; index < 4; index += 1) value += bytes[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}

// 원문 이름표에는 두 이름뿐이며 첫 NUL 앞만 비교한다. 임의 별칭은 없다. (2026-11-08 WB04-E27)
export function aiDebugCommandScriptCode(name) {
    const end = name.indexOf('\0');
    const key = asciiLower(end < 0 ? name : name.slice(0, end));
    if (key === 'togglereporting') return AI_REPORTING_COMMAND;
    if (key === 'toggledebug') return DEBUG_SWITCH_COMMAND;
    return 0xffffffff;
}

// 분류에도 원문처럼 최상위 수식/뗌 비트만 제거한다. (2026-11-08 WB04-E27)
export function isAiDebugCommand(code) {
    const value = (code & KEY_MASK) >>> 0;
    return value === AI_REPORTING_COMMAND || value === DEBUG_SWITCH_COMMAND;
}

// state = { metamap(), player(owner) → Player 행 바이트, body(token) → 살아 있는 brain 본문, readDebug(주소), writeDebug(주소, 값) } 이다. 같은 현재 본문 +0xb00 과 진단 DWORD 만 쓴다. (2026-11-08 WB04-E27)
export function applyAiDebugCommandState(event, state) {
    const code = (event.code & KEY_MASK) >>> 0;
    if (code === AI_REPORTING_COMMAND && !need(state.metamap)() && event.y > 0) {
        const owner = event.y >>> 0;
        if (owner >= PLAYER_COUNT) throw new RangeError('AI Reporting Player owner 물리범위');
        const token = word(need(state.player)(owner), BRAIN_TOKEN_OFFSET);
        if (token) {
            const body = need(state.body)(token);
            const value = word(body, REPORTING_OFFSET) === 0 ? 1 : 0;
            // 앞 물리 읽기 성공 뒤 같은 DWORD 전체 비트를 덮어쓴다.
            for (let index = 0; index < 4; index += 1) body[REPORTING_OFFSET + index] = (value >>> (8 * index)) & 0xff;
        }
    }
    if (code === DEBUG_SWITCH_COMMAND) {
        const address = event.y >>> 0;
        const value = need(state.readDebug)(address);
        need(state.writeDebug)(address, value === 29 ? 99 : 29);
    }
}

// 뗌은 모드/Player/포인터 resolver 를 읽지 않고 소비만 한다. 다른 명령은 false 이다. (2026-11-08 WB04-E27)
export function dispatchAiDebugCommandInput(event, state) {
    if (!isAiDebugCommand(event.code)) return false;
    if ((event.code & InputCode.release) === 0) applyAiDebugCommandState(event, state);
    return true;
}
