// 원본 HTML 링크(`<a href="목적지">`)의 목적지 문자열을 분배하는 모듈이다. 도움말 창과 Tell 대화상자가 함께 쓴다.
// 목적지는 `접두:본문` 꼴이며 접두와 본문은 원본 토큰 읽기(구분자 `:`)로 나눈다. 분배 규칙(원본 4a00c0, cpppj HelpLinkPolicy.cpp)은 다음과 같다.
//  1) 접두의 앞 네 글자가 `cmd:`(ASCII 대소문자 무시)이면 스크립트 명령이다. 본문(예: `Tell,TechSupport`)을 명령 큐에 넣는 쪽(004c9090)으로 넘긴다.
//  2) 접두가 없거나(콜론이 없음) 본문이 `#` 로 시작하면 도움말 주제이다. 본문을 주제 이름으로 넘긴다(방식 1, 학습 0).
//  3) 그 밖에는 바깥 주소(`http:`·`mailto:` 등)이다. 전체 원문·접두·본문을 넘긴다. 원본은 설정 `autoOpen` 에 접두가 있으면 셸로 열고 아니면 접두 이름의 명령 설정을 실행하지만,
//     웹 판본은 바깥 프로그램을 실행할 수 없고 원본의 주소(과거 도메인)도 사라졌으므로 호출자(호스트)가 알림만 남기게 한다.
// 접두는 80바이트 미만, 본문은 256바이트 미만이어야 한다(원본 물리 범위). 넘으면 RangeError 이다. 입력은 첫 NUL 까지만 읽는다(원본 C 문자열).
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다. 원본 x86 기대값(cpppj/tests/fixtures/help-links-x86.tsv 91행)과 분류·접두·본문이 같음을 검사한다.
// (2026-10-31 WB03-K: Tell 대화상자 링크 훅을 위해 도움말 창의 단순 접두 판정을 원본 규칙으로 바꾸며 추가)
import { asciiLower } from '../assets/config.js';
import { readRawTextToken } from './html-text.js';

// 접두 문자열의 한도(미만)이다. 접두는 `:` 를 포함해 79글자 이하여야 한다(원본 80바이트 버퍼). (2026-10-31 WB03-K)
export const HELP_LINK_PROTOCOL_LIMIT = 80;

// 본문 문자열의 한도(미만)이다(원본 256바이트 버퍼). (2026-10-31 WB03-K)
export const HELP_LINK_BODY_LIMIT = 256;

// 문자열을 첫 NUL 앞까지로 자른다(원본 C 문자열). (2026-10-31 WB03-K)
function cutAtNul(value) {
    const text = String(value);
    const nul = text.indexOf('\0');
    return nul < 0 ? text : text.slice(0, nul);
}

// 목적지를 접두와 본문으로 나눈다(원본 487620 의 토큰 읽기). 콜론이 없으면 접두는 빈 문자열이고 본문은 전체이다.
// 콜론이 있으면 첫 토큰(구분자 `:`, 따옴표 안은 구분하지 않음, 앞 공백 건너뜀, 78글자 제한)에 `:` 를 붙인 것이 접두이고, 연속된 콜론을 건너뛴 나머지가 본문이다.
// 반환: { protocol, body }. 사용: parseHelpLinkAddress('cmd:Tell,TechSupport') → { protocol: 'cmd:', body: 'Tell,TechSupport' }. (2026-10-31 WB03-K)
export function parseHelpLinkAddress(value) {
    const text = cutAtNul(value);
    let protocol = '';
    let body = text;
    if (text.includes(':')) {
        const token = readRawTextToken(text, 0, ':');
        protocol = `${token.text}:`;
        body = text.slice(token.next);
    }
    if (protocol.length >= HELP_LINK_PROTOCOL_LIMIT || body.length >= HELP_LINK_BODY_LIMIT) throw new RangeError('도움말 링크의 접두(80)·본문(256) 한도를 넘었습니다.');
    return { protocol, body };
}

// 목적지를 분류한다. 반환: 'script'(cmd: 명령), 'help'(도움말 주제), 'external'(바깥 주소). 한도를 넘으면 RangeError 이다.
// 사용: classifyHelpLink('mailto:a@b') → 'external'. (2026-10-31 WB03-K)
export function classifyHelpLink(value) {
    const address = parseHelpLinkAddress(value);
    if (asciiLower(address.protocol.slice(0, 4)) === 'cmd:') return 'script';
    if (address.protocol === '' || (address.body !== '' && address.body[0] === '#')) return 'help';
    return 'external';
}

// 목적지를 분배한다(원본 4a00c0). hooks.script(본문)·hooks.help(본문)·hooks.external(전체 원문, 접두, 본문) 중 분류에 맞는 하나만 부른다. 없는 훅은 부르지 않는다.
// 도움말 주제의 본문은 목적지 전체(`#` 로 시작하는 본문 포함)이다. 분류 이름을 돌려준다.
// 사용: dispatchHelpLink(destination, { script: (body) => ..., help: (topic) => ..., external: (original) => ... }). (2026-10-31 WB03-K)
export function dispatchHelpLink(value, hooks = {}) {
    const text = cutAtNul(value);
    const address = parseHelpLinkAddress(text);
    const kind = classifyHelpLink(text);
    if (kind === 'script') hooks.script?.(address.body);
    else if (kind === 'help') hooks.help?.(address.body);
    else hooks.external?.(text, address.protocol, address.body);
    return kind;
}
