// 실제 10.78 메뉴가 읽는 명령 전역의 초기 ID(주소 식별자→명령 DWORD)와 HTML/대본 이름→명령 조회, 일반 메뉴 명령 판정, ESC·F9 키 판정을 한곳에 모은 표 모듈이다(cpppj `client/GameMenuCommands.cpp/.h`, 10.78 전용).
// 사용:
//   gameMenuCommandValue(address) → 확인한 원본 전역 주소(00542xxx)의 현재 제품 명령 ID 또는 null. 주소는 식별자일 뿐이며 호스트 포인터로 역참조하지 않는다.
//   gameMenuScriptCode(name) → 원본 Tell 하위의 이름→명령 조회(ASCII 대소문자 무시, 미등록은 0xffffffff). 아래 순서로 각 입력 모듈의 이름표를 먼저 조회한 뒤 이 모듈의 이름 표를 본다:
//     MakeAi → AiUtility → CreateAt → DrawWater → AiDebug → CreateHelpList → Presentation → Utility → Configuration → ScenarioEffect → ScenarioObject → Vortex → 이름 표.
//   isGameMenuCommand(code) → 상위 입력 표시를 제거한 실제 메뉴/하위 명령이면 true(설정·유틸리티 명령, 주소표의 모든 값, TechnicalHelp 0x02000049·ScreenModeChange 0x02000002·AcceptScreenModeChange 0x020000e4).
//   isEscapeKeyPress(event) → 원문은 `code & 03ffffff` 전체가 0x1b 이고 뗌이 아닐 때만 ESC 이며 수정키는 비교 대상이 아니고 하위 BYTE 가 같은 명령·마우스·가상 키는 구별한다.
//   queuePlayerListKey(event, queue) → F9(0x780000)는 Player List(0x020000f0), Shift+F9 는 Pause(0x020000cf) 명령을 두 인자 0 으로 예약한다. 뗌과 Control 은 거부하고 Alt 는 거부하지 않는다.
// 포함하지 않는 것: `HtmlScriptCommandValue`(UberGump 의 DialogScriptCode 가 Tell·Config·Warning·ViewSpot·MoveDialog·SetOwner·Connect·Salvage·SendMoney·FortSelection·mission 이름과 이 표를 묶는다 — 해당 단위 몫).
// exclude-host 단위(QuickHelpList·CaptureInput·RecheckCdInput)는 웹에 효과를 옮기지 않지만 명령 ID 와 이름은 같은 표에 남겨 `isGameMenuCommand`/`gameMenuScriptCode` 의 결과가 cpppj 와 같게 한다.
// 원본 관측: 원본 PE 43개 이름표 `html-solo-command-names-pe.tsv` 의 (이름, 전역 주소, 명령) 를 tests/game-menu-commands.test.mjs 가 이 표와 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E538: 6단계 게임 메뉴 GameMenuCommands 이식)
import { InputCode } from './input-queue.js';
import { asciiLower } from '../assets/config.js';
import { AI_PARAM_SET_COMMAND, BRAIN_CRANK_MONEY_COMMAND, CREATE_INITIAL_GEYSERS_COMMAND, aiUtilityCommandScriptCode } from './client-input-commands.js';
import { ADD_RANDOM_COMMAND, DESTROY_AT_COMMAND, MAKE_AI_COMMAND, makeAiCommandScriptCode } from './script-command-inputs.js';
import { CREATE_AT_COMMAND, createAtScriptCode } from './create-at-input.js';
import { DRAW_WATER_COMMAND, drawWaterScriptCode } from './draw-water.js';
import { AI_REPORTING_COMMAND, DEBUG_SWITCH_COMMAND, aiDebugCommandScriptCode } from './ai-debug-command-input.js';
import { CHANGE_FONT_FACE_COMMAND, CHANGE_SOUND_QUALITY_COMMAND, presentationCommandScriptCode } from './presentation-command-input.js';
import { CHANGE_PAL_COMMAND, CHANGE_PRIORITY_COMMAND, SET_FORT_TYPE_COMMAND, configurationCommandScriptCode, isConfigurationCommand } from './configuration-command-input.js';
import { isUtilityCommand, utilityCommandScriptCode } from './utility-command-input.js';
import { LANGUAGE_CHANGED_COMMAND, SET_LANGUAGE_COMMAND } from './language-request.js';
import { GET_NEW_TECH_COMMAND, REQUEST_STORMPOWER_COMMAND, UPGRADE_ALTAR_COMMAND } from './ritual-choice-input.js';
import { URL_COMMAND } from './url-input.js';
import { PLAY_INTRO_COMMAND } from './intro-menu.js';
import { scenarioEffectScriptCode } from './scenario-effect-input.js';
import { scenarioObjectScriptCode } from './scenario-object-input.js';
import { vortexCommandScriptCode } from './vortex-command-input.js';
import { sendMoneyScriptCode } from './send-money-input.js';
import { SHOW_TEC_MENU2_COMMAND, SHOW_TEC_SIDE_MENU_COMMAND } from './raw-knowledge-menu.js';

// 이름이 없을 때 원본이 돌려주는 값이다. (2026-11-15 WB04-E538)
const NOT_FOUND = 0xffffffff;
// 수정키를 지운 입력 코드 마스크이다. (2026-11-15 WB04-E538)
const KEY_MASK = 0x03ffffff;

// exclude-host 단위의 명령 ID 이다. 00542964 의 Help 목록 출력 부모, 543070 의 연속 화면 저장, RecheckCD 의 serial 정리이며 웹에는 효과가 없고 표의 일관성만 위해 둔다. (2026-11-15 WB04-E538)
export const CREATE_HELP_LIST_COMMAND = 0x0200009c;
export const CAPTURE_COMMAND = 0x0200009d;
export const RECHECK_CD_COMMAND = 0x020000fd;

// 00542xxx 는 원문 주소 식별자이며 런타임 호스트 포인터로 역참조하지 않는다. 같은 값의 주소 항목이 여럿이어도 허용 결과는 같고 조회는 첫 쌍을 돌려준다(원문 순서 보존). (2026-11-15 WB04-E538)
const COMMANDS = Object.freeze([
    [0x005429a8, CREATE_INITIAL_GEYSERS_COMMAND], [0x005429f8, BRAIN_CRANK_MONEY_COMMAND], [0x005428d8, AI_PARAM_SET_COMMAND], // 772의원문세일반명령이다.
    [0x005429d0, MAKE_AI_COMMAND], // 005429d0의 실제 초기 일반 명령이다.
    [0x005427a4, 0x0200002b], // PE542ce0의 JoinTestBattle 이름표가 가리키는 실제 일반 입력 명령이다. 새 메뉴를 노출하지 않는다.
    [0x00542760, DRAW_WATER_COMMAND], // 원문 후단 진단 명령이며 기본 메뉴 노출은 바꾸지 않는다.
    [0x542a34, AI_REPORTING_COMMAND], [0x542a38, DEBUG_SWITCH_COMMAND], // 조건부AI메뉴의실제현재명령이다.
    [0x00542964, CREATE_HELP_LIST_COMMAND], // 원문 일반입력의 현재 전용Help 출력부모다.
    [0x00542764, CHANGE_FONT_FACE_COMMAND], [0x00542a68, CHANGE_SOUND_QUALITY_COMMAND], // 원문 두 현재 옵션 명령이다.
    [0x00542b84, SHOW_TEC_SIDE_MENU_COMMAND], [0x00542b88, SHOW_TEC_MENU2_COMMAND], // 원문추가지식선택버튼의현재명령도같은허용표를쓴다.
    [0x00542988, SET_LANGUAGE_COMMAND], [0x0054298c, LANGUAGE_CHANGED_COMMAND], // 원문의 두 언어 명령이다.
    [0x00542984, 0x020000a4], // 영역 확인창의 DecendToChalLevel도 같은 현재 명령 조회와 공통 메뉴 허용표를 공유한다.
    [0x00542b40, 0x0200011c], [0x00542b44, 0x0200011e], // RequestMenu/RefreshPlayers의실제일반입력과HTML버튼이같은보호PE명령을사용한다.
    [0x00542a54, PLAY_INTRO_COMMAND], // 원문 PlayIntro 이름의 초기 DWORD다.
    [0x00542ae8, RECHECK_CD_COMMAND], // RecheckCD의 이름·메타·큐 명령이다. 실제 CD 재탐색이 아니라 현재 serial 정리다.
    [0x00542968, CAPTURE_COMMAND], // 연속 화면 저장의 이름과 정수 명령을 같은 큐로 보낸다.
    [0x00542864, URL_COMMAND], // 원문 URL의 이름·숫자·HTML 버튼을 같은 입력으로 보낸다.
    [0x00542a74, CREATE_AT_COMMAND], // 현재 Runtime 생성 부모만 허용하고 기본 UI는 추가하지 않는다.
    [0x00542a78, DESTROY_AT_COMMAND], // 원문DestroyAt의 문자열 토큰도 같은 일반 입력/버튼 큐다.
    [0x00542954, ADD_RANDOM_COMMAND], // 원본AddRandom 이름도 같은 일반 입력/버튼 큐를 사용한다.
    [0x0054287c, 0x02000061], // 원문 Cheat의 같은 일반 SP 가산 명령이다.
    [0x00542780, 0x02000022],
    [0x005427b8, 0x02000030],
    [0x005428ac, 0x0200006d],
    [0x005428dc, 0x02000079],
    [0x00542914, 0x02000088],
    [0x00542924, 0x0200008c],
    [0x00542928, 0x0200008d],
    [0x00542938, 0x02000091],
    [0x00542944, 0x02000094],
    [0x00542948, 0x02000095],
    [0x0054295c, 0x0200009a], // PlaySound의 문자열 인자와 여섯 음향 인자를 전달한다.
    [0x005427ac, 0x0200002d], // DigUp 이름도 기존 일반 입력의 같은 보상 경로를 사용한다.
    [0x00542a44, UPGRADE_ALTAR_COMMAND], // 원문의제단업그레이드요청이며즉시지급과구별한다.
    [0x00542a48, GET_NEW_TECH_COMMAND], // 원문의기술선택6b요청이다.
    [0x00542ad8, REQUEST_STORMPOWER_COMMAND], // 원문의시간초과SP보상선택요청이다.
    [0x00542970, 0x0200009f], // GetTechnology의 이름과 HTML 버튼도 같은 일반 입력으로 전달한다.
    [0x00542974, 0x020000a0],
    [0x005429ac, 0x020000ae],
    [0x005429d4, 0x020000b8],
    [0x005429fc, 0x020000c2],
    [0x00542a08, 0x020000c5],
    [0x00542a0c, 0x020000c6],
    [0x00542a20, 0x020000cb], // SaveGoMain의 두 저장 선택은 같은 실제 Native State3 예약 명령을 사용한다.
    [0x00542a30, 0x020000cf],
    [0x00542a28, 0x020000cd], // GetFullDeck의 원본 입력 ID다.
    [0x00542a4c, 0x020000d6],
    [0x00542a60, 0x020000db],
    [0x00542ae0, 0x020000fb], [0x00542adc, 0x020000fa], [0x005428f0, 0x0200007e], // Request/Break/ConfirmAlliance의실제일반입력과HTML Yes가같은현재명령을쓴다.
    [0x00542a8c, 0x020000e6],
    [0x00542a90, 0x020000e7],
    [0x00542aa0, 0x020000eb], // Tips의 이전·다음·확인은 같은 TellTip 명령의 두 번째 인자를 쓴다.
    [0x00542ab4, 0x020000f0],
    [0x00542ab8, 0x020000f1],
    [0x00542700, 0x02000001],
    [0x005429ec, 0x020000be],
    [0x00542b54, 0x02000123],
]);

// 원본 이름→ID 표에서 확인한 메뉴 확인창의 필수 이름(소문자 키)이다. 위의 입력 모듈 이름표에 없는 이름만 여기서 조회한다. (2026-11-15 WB04-E538)
const KEY_COMMANDS = new Map([
    ['metarefresh', 0x020000c6], // PE5431b8의 실제 이름이며 현재 모드 판정 뒤 기존16h/TCP11을 재요청한다.
    ['battledesc', 0x020000f3], // PE543320의 실제 HTML 이름이며 양수 고리 토큰을 CS 논리 주소와 구별해 해석한다.
    ['jointestbattle', 0x0200002b], // 보호 PE518d34→542ce0→5427a4의 원문 이름을 같은 허용표/입력 큐에 연결한다.
    ['chalsailjoinbattle', 0x0200006b], // PE542ee0의 실제 이름이며 현재 진단 경고/좌석 참가 소비자에 연결한다.
    ['chalsailpos', 0x0200006c], // PE542ee8의 실제 이름이며 packed 위치를 현재 항해 소비자가 읽는다.
    ['chalforcejoinbattle', 0x020000e3], // PE5432a0의 실제 경고 확인은 옵션을 바꾸지 않고 직접 참가한다.
    ['chaljoinbattlenowarning', 0x020000fe], // PE543378의 실제 확인은 경고 억제 설정 뒤 참가한다.
    ['querydecendtochallevel', 0x020000e8], // PE5432c8의 원문 오자를 그대로 지원하며 현재 영역 번호의 확인 Tell을 연다.
    ['decendtochallevel', 0x020000a4], // PE5430a8→542984의 실제 이름/명령이며 영역 확인창 Yes가 같은4b8810 부모를 실행한다.
    ['requestmenu', 0x0200011c], // PE543420→542b40의서버섹션요청이며로컬Tell/Zsend별칭으로바꾸지않는다.
    ['refreshplayers', 0x0200011e], // PE543428→542b44의현재CS이름표/NSInfo갱신이며새서버요청을만들지않는다.
    ['requestalliance', 0x020000fb], ['breakalliance', 0x020000fa], ['confirmalliance', 0x0200007e], ['declaredraw', 0x020000db], // PE543364/335c/2f7c/3264의실제이름이며Confirm은기존78h승낙이다.
    ['serverforcebattlestart', 0x020000f5], // PE00543330 이름표/00542ac8의 실제 일반 명령이다.
    ['delstatefile', 0x020000ac], // PE005430e8→005429a4의 실제 저장상태 삭제 명령이다.
    ['statereconnect', 0x020000ab], // PE005430e0→005429a0의 실제 저장 복구 명령이다.
    ['forceserverdeath', 0x020000a9], // PE005430d0→00542998의 실제 서버 이전 명령이다.
    ['setlanguage', SET_LANGUAGE_COMMAND],
    ['languagechanged', LANGUAGE_CHANGED_COMMAND],
    ['playintro', PLAY_INTRO_COMMAND],
    ['url', URL_COMMAND], // 외부 주소뿐 아니라 내부 Help/cmd 링크도 같은4a00c0이해석한다.
    ['capture', CAPTURE_COMMAND], // 543070의 실제 이름이며 새 모드 관문을 추가하지 않는다.
    ['recheckcd', RECHECK_CD_COMMAND], // 543370의 실제 이름이다. 종료된 온라인 검증/CD 재탐색으로 바꾸지 않고 현재 serial 정리만 연결한다.
    ['addrandom', ADD_RANDOM_COMMAND], // 00543048의 원문 이름이며 수량1은 실제 일반 입력이 전달한다.
    ['destroyat', DESTROY_AT_COMMAND], // 00543290의 실제 이름이며 좌표/필터 문자열을 보존한다.
    ['changemoney', 0x02000061], // 원문542e90의 실제 이름이며 기존 signed SP 가산 몸체를 공유한다.
    ['playsound', 0x0200009a], // 원문543058의 이름→54295c이며 다른 소리 명령의 별칭이 아니다.
    ['viewpriest', 0x020000b8], // 원문543148의 명시 owner를 기존4950d0 경로에 전달한다.
    ['setallbridge', 0x020000d6], // 원문543238의 품질 숫자를 기존 전체 다리 변경에 전달한다.
    ['digup', 0x0200002d], // 원문542cf0의 이름이며 메뉴 노출 여부를 새 실행 관문으로 넣지 않는다.
    ['quitapp', 0x02000001],
    ['requestquitapp', 0x02000094],
    ['menujustclosed', 0x020000be],
    ['reloadtheams', 0x02000123],
    ['pause', 0x020000cf],
    ['techhelp', 0x02000049], // 원문 TechHelp 이름표와54281c의 현재 일반 명령이다.
    ['getnewtech', GET_NEW_TECH_COMMAND],
    ['requeststormpower', REQUEST_STORMPOWER_COMMAND],
    ['upgradealtar', UPGRADE_ALTAR_COMMAND], // 원문세이름과메뉴행이같은요청입력에연결된다.
    ['getfulldeck', 0x020000cd],
    ['toggleminimap', 0x02000037], // 원본542d40 이름표→5427d4의 독립 미니맵 전환 명령이다.
    ['togglebottomgump', 0x02000088], // 원본542fc8 이름표→542914의 하단 채팅 전환이며0/1/2 인자는 기존 도킹 경로가 해석한다.
    ['addisland', 0x02000022], // 00542780의 실제 Editor 섬 추가 명령이다.
    ['savegomain', 0x020000cb],
    ['telltip', 0x020000eb],
    ['newfort', 0x02000079], // LOADLOADFORT가 예약하는 새 이름 창의 원본 명령이다. y=0/1은 실제 큐에 그대로 보존한다.
    ['screenmodeabschange', 0x02000030], // 원본 DirectDrawManager/Window Mode가 사용하는 절대 모드 명령이다.
    ['screenmodechange', 0x02000002], // 실제542704 조정 비트 반전 명령이다.
    ['acceptscreenmodechange', 0x020000e4], // 실제542a84 확인 뒤 기억 명령이다.
]);

// 원문 543068 의 단일 이름이며 입력 문자열의 첫 NUL 까지만 비교한다(QuickHelpList 의 CreateHelpListScriptCode). (2026-11-15 WB04-E538)
function createHelpListScriptCode(name) {
    const nul = name.indexOf('\0');
    return asciiLower(nul < 0 ? name : name.slice(0, nul)) === 'createhelplist' ? CREATE_HELP_LIST_COMMAND : NOT_FOUND;
}

// 등록한 원문 전역과 동일 명령 ID 의 첫 쌍만 반환한다. 세 설정 명령과 개발자 NextStage 의 현재 전역도 같은 숫자 조회로 연결한다. (2026-11-15 WB04-E538)
export function gameMenuCommandValue(address) {
    address >>>= 0;
    // 개발자 Ctrl+Shift+F7 도 원문의 NextStage 전역과 같은 명령을 큐에 예약한다.
    if (address === 0x00542a40) return 0x020000d3;
    if (address === 0x0054279c) return CHANGE_PAL_COMMAND;
    if (address === 0x0054291c) return CHANGE_PRIORITY_COMMAND;
    if (address === 0x00542a10) return SET_FORT_TYPE_COMMAND;
    // 주소와 명령 숫자를 뒤바꾸지 않는다.
    for (const [source, value] of COMMANDS) if (source === address) return value;
    return null;
}

// 원본 name→ID 표의 이름 조회이다. 입력 모듈별 이름표를 원문 순서로 먼저 보고 마지막에 이 모듈의 이름 표를 ASCII 대소문자 무시로 비교한다. (2026-11-15 WB04-E538)
export function gameMenuScriptCode(name) {
    // 각 입력 모듈의 이름표를 원문 순서대로 조회한다.
    for (const lookup of [makeAiCommandScriptCode, aiUtilityCommandScriptCode, createAtScriptCode, drawWaterScriptCode, aiDebugCommandScriptCode,
        createHelpListScriptCode, presentationCommandScriptCode, utilityCommandScriptCode, configurationCommandScriptCode, scenarioEffectScriptCode,
        scenarioObjectScriptCode, vortexCommandScriptCode, sendMoneyScriptCode]) {
        const code = lookup(name);
        if (code !== NOT_FOUND) return code;
    }
    // 이름 비교만 원문 ASCII 대소문자를 무시한다.
    const value = KEY_COMMANDS.get(asciiLower(name));
    return value === undefined ? NOT_FOUND : value;
}

// 실제 큐 연결 허용과 선택 후 미연결 명령 진단에 같은 표를 쓴다. (2026-11-15 WB04-E538)
export function isGameMenuCommand(code) {
    code >>>= 0;
    // 실제 같은 분배가 처리하는 설정/유틸리티 명령이다.
    if (isConfigurationCommand(code)) return true;
    if (isUtilityCommand(code)) return true;
    // 같은 값의 주소 항목이 여럿이어도 허용 결과는 같다.
    for (const [, value] of COMMANDS) if (value === code) return true;
    // TechnicalHelp 와 화면 조정/확인의 실제 원문 기본 전역 명령이다.
    return code === 0x02000049 || code === 0x02000002 || code === 0x020000e4;
}

// 원문은 `code & 03ffffff` 전체가 1b 일 때만 ESC 이며 일반 명령 ID 의 하위 BYTE 를 키로 쓰지 않는다. (2026-11-15 WB04-E538)
export function isEscapeKeyPress(event) {
    return (event.code & InputCode.release) === 0 && ((event.code & KEY_MASK) >>> 0) === 0x1b;
}

// 원본 004d95ce 의 F9/Shift+F9 를 Player List/정지 명령으로 예약하며 뗌과 Ctrl 은 거부한다. Alt 는 별도로 거부하지 않고 Shift 만 목적 명령을 바꾸며 두 인자는 항상 0 이다. (2026-11-15 WB04-E538)
export function queuePlayerListKey(event, queue) {
    if ((event.code & (InputCode.release | InputCode.control)) !== 0 || ((event.code & KEY_MASK) >>> 0) !== (0x78 << InputCode.virtualKeyShift)) return false;
    queue.pushCommand((event.code & InputCode.shift) !== 0 ? 0x020000cf : 0x020000f0, 0, 0);
    return true;
}
