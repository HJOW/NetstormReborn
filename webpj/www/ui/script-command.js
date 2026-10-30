// 대화상자 본문의 스크립트 명령(`<$이름,인자>`)과 버튼 명령(`Config,키=값`) 해석 모듈이다.
// 원본 004c9090(명령 큐 넣기)·00441580(Config 명령)의 토큰 규칙을 옮겼다. 토큰 읽기는 html-text.js 의 공통 helper(원본 00487620/00487710)를 쓴다.
//  - 명령 이름은 구분자 `#,;>` 로, 인자는 구분자 `;>` 로 읽는다(인자 안의 쉼표는 남는다). 앞 space/tab 은 건너뛰고 따옴표는 출력하지 않으며 입력은 78글자까지 읽는다.
//  - 인자가 숫자 글자로 시작하면 원본 atol(ConfigParseLong)로 읽은 정수이고, 그 밖에는 문자열이다.
//  - Config 명령의 인자는 `키=값` 이며 키는 구분자 `=` 로, 값은 구분자 LF 로 읽어(값 안의 `=`·`;` 는 자르지 않는다) 설정에 쓴다. 키가 비어 있으면 아무것도 쓰지 않는다.
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
