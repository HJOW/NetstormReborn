// 대화상자 본문의 스크립트 명령(`<$이름,인자>`)과 버튼 명령(`Config,키=값`) 해석 모듈이다.
// 원본 004c9090(명령 큐 넣기)·00441580(Config 명령)의 토큰 규칙을 옮겼다. 토큰 읽기는 html-text.js 의 공통 helper(원본 00487620/00487710)를 쓴다.
//  - 명령 이름은 구분자 `#,;>` 로, 인자는 구분자 `;>` 로 읽는다(인자 안의 쉼표는 남는다). 앞 space/tab 은 건너뛰고 따옴표는 출력하지 않으며 입력은 78글자까지 읽는다.
//  - 인자가 숫자 글자로 시작하면 원본 atol(ConfigParseLong)로 읽은 정수이고, 그 밖에는 문자열이다.
//  - Config 명령의 인자는 `키=값` 이며 키는 구분자 `=` 로, 값은 구분자 LF 로 읽어(값 안의 `=`·`;` 는 자르지 않는다) 설정에 쓴다. 키가 비어 있으면 아무것도 쓰지 않는다.
// WB04-E343: 원본 004c9090 의 명령 큐 넣기(queueScriptCommand)와 80칸 문자열 고리(ScriptStringRing, 004c7910)를 더했다. 명령 코드는 40칸 입력 큐(core/input-queue.js)의 명령 사건으로 예약하고 문자열 인자는 고리 토큰으로 넘긴다.
// 명령 표(어떤 이름이 어떤 동작인지)는 이 모듈 밖에서 정한다: 웹 판본은 Config·Warning 등 필요한 명령만 연결하고 나머지는 알림만 한다.
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다. (2026-10-30 WB03-J: 시작 환영 창의 `<$config,welcomeGiven=1>` 과 팁 창의 `Config,tellTips=0` 을 실행하기 위해 추가)
import { configParseLong } from '../assets/config.js';
import { readRawTextToken } from './html-text.js';

// 명령 이름 토큰의 구분자이다(원본 004c9090 의 첫 필드). (2026-10-30 WB03-J)
const COMMAND_DELIMITERS = '#,;>';

// 명령 인자 토큰의 구분자이다(원본 004c9090 의 두 번째 필드. 쉼표는 구분자가 아니다). (2026-10-30 WB03-J)
const ARGUMENT_DELIMITERS = ';>';

// 스크립트 명령 원문(`<$` 뒤부터의 문자열, 예: `config,welcomeGiven=1>...`)을 이름과 인자로 나눈다.
// 반환: { name, argument, number }. number 는 인자가 숫자 글자로 시작할 때의 정수(원본 atol, 32비트 감김)이고 그 밖에는 null 이다.
// 사용: parseScriptCommand('config,welcomeGiven=1>') → { name: 'config', argument: 'welcomeGiven=1', number: null }. (2026-10-30 WB03-J)
export function parseScriptCommand(source) {
    const command = readRawTextToken(source, 0, COMMAND_DELIMITERS);
    const argument = readRawTextToken(source, command.next, ARGUMENT_DELIMITERS);
    const first = argument.text.length > 0 ? argument.text[0] : '';
    const number = first >= '0' && first <= '9' ? configParseLong(argument.text) : null;
    return { name: command.text, argument: argument.text, number };
}

// Config 명령(원본 00441580)을 실행한다. argument 는 `키=값` 문자열이고 set(키, 값)이 실제 설정 쓰기이다. 키가 비어 있으면 쓰지 않고 false, 썼으면 true 를 돌려준다.
// 키는 구분자 `=` 로, 값은 구분자 LF 로 읽는다. 사용: applyScriptConfig('tellTips=0', (key, value) => data.set(key, value)). (2026-10-30 WB03-J)
export function applyScriptConfig(argument, set) {
    const keyword = readRawTextToken(argument, 0, '=');
    const value = readRawTextToken(argument, keyword.next, '\n');
    if (keyword.text.length === 0) return false;
    set(keyword.text, value.text);
    return true;
}

// 원본 명령 표의 두 항목 코드이다(PATCH 명령 표의 Config/Warning). 문자열 이름 조회는 호출자가 hooks.lookup 으로 공급한다. (2026-11-12 WB04-E343)
export const SCRIPT_CONFIG = 0x0200006d;
export const SCRIPT_WARNING = 0x02000102;

// 문자열 고리 칸 수(원본 80칸)와 호스트 토큰의 상한(원본 Begin 의 signed 주소 관문 `> 100` 을 유효 수명 전체에서 만족하는 101..INT_MAX)이다. (2026-11-12 WB04-E343)
const STRING_RING_SLOTS = 80;
const STRING_TOKEN_START = 100;
const STRING_TOKEN_LAST = 0x7fffffff;

// 원본 80칸 문자열 고리(004c7910)이다. 같은 문자열도 매번 새 칸과 새 비영 토큰을 소비하고, 81번째 복사부터 가장 오래된 칸의 이전 수명을 정확히 무효화한다.
// 토큰은 호스트 포인터 폭과 무관한 불투명 번호이며 실제 주소 값은 복제하지 않는다. 사용: const token = ring.store('Config,DoneTutorial1=1'); ring.resolve(token). (2026-11-12 WB04-E343)
export class ScriptStringRing {
    // 원본 80칸의 현재 수명이다: { text, token }(token 0 은 빈 칸).
    #slots = Array.from({ length: STRING_RING_SLOTS }, () => ({ text: '', token: 0 }));
    // 현재 칸 위치이다(BSS 최초 0, 첫 store 에서 1).
    #index = 0;
    // 마지막으로 발급한 토큰이다.
    #serial = STRING_TOKEN_START;

    // 다음 칸의 이전 문자열을 해제한 뒤 새 문자열(첫 NUL 앞)을 복사하고 비영 토큰을 돌려준다. 토큰 수명이 소진되면 칸을 비운 채 거부한다. (2026-11-12 WB04-E343)
    store(text) {
        this.#index = (this.#index + 1) % STRING_RING_SLOTS;
        const slot = { text: '', token: 0 };
        this.#slots[this.#index] = slot;
        if (this.#serial === STRING_TOKEN_LAST) throw new RangeError('스크립트 문자열의 양수 주소 토큰 수명 소진');
        const nul = text.indexOf('\0');
        slot.text = nul < 0 ? text : text.slice(0, nul);
        this.#serial += 1;
        slot.token = this.#serial;
        return slot.token;
    }

    // 현재 80칸 안의 실제 토큰만 해석한다. 고리가 덮어쓴 이전 수명이나 잘못된 토큰은 명시 오류이며, 같은 문자열 값은 합치지 않는다. (2026-11-12 WB04-E343)
    resolve(token) {
        // 실제 현재 80개 항목만 번호로 조회한다.
        for (const slot of this.#slots) if (slot.token !== 0 && slot.token === token) return slot.text;
        throw new RangeError('스크립트 문자열의 해제되었거나 잘못된 토큰');
    }

    // 독립 원본 대조에서 확인하는 현재 고리 위치이다. (2026-11-12 WB04-E343)
    get index() { return this.#index; }
}

// 004c9090: 스크립트 명령을 페이지 전환 예약과 섞지 않고 기존 40칸 입력 큐로 보낸다. 명령 이름(구분자 `#,;>`)과 인자(구분자 `;>`, 쉼표 보존)를 읽고, 인자 변환 전에 명령 표를 조회한다.
// 코드 0 은 진단 뒤 계속 진행(코드 0 인 명령 사건을 예약)하고, 0xffffffff 는 인자 처리를 마친 뒤 진단하고 예약 없이 끝낸다. 인자가 숫자 글자로 시작하면 atol(32비트 감김), 그 밖에는 문자열 고리 토큰이다.
// hooks = { lookup(이름) → 코드(미등록 0xffffffff), unknown(이름) → 진단 }. 사용: queueScriptCommand('Config,Done=1', queue, ring, hooks). (2026-11-12 WB04-E343)
export function queueScriptCommand(source, queue, strings, hooks) {
    if (typeof hooks?.lookup !== 'function') throw new TypeError('스크립트 명령 표 조회 미연결');
    const command = readRawTextToken(source, 0, COMMAND_DELIMITERS);
    const argument = readRawTextToken(source, command.next, ARGUMENT_DELIMITERS);
    const code = hooks.lookup(command.text) >>> 0;
    const diagnose = () => {
        if (typeof hooks.unknown !== 'function') throw new Error('스크립트 미등록 명령 진단 미연결');
        hooks.unknown(command.text);
    };
    if (code === 0) diagnose();
    const first = argument.text.length > 0 ? argument.text[0] : '';
    const parameter = first >= '0' && first <= '9' ? configParseLong(argument.text) >>> 0 : strings.store(argument.text);
    if (code === 0xffffffff) {
        diagnose();
        return;
    }
    queue.pushCommand(code, parameter | 0, parameter | 0);
}
