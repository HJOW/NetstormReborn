// Tell 이름의 지역 인자(`이름(인자1,인자2)`) 해석 모듈이다. 원본 004c8ea0 을 옮겼다(cpppj client/TellParameters.cpp, 원본 기계어 1,070관측과 대조).
// 예) `SETSIZE(800,600)` 은 이름 `SETSIZE` 를 돌려주고 지역 설정 객체 `param` 에 `1="800"`·`2="600"` 줄을 덧붙인다. 본문의 `{param.1}` 이 그 값을 읽는다.
//  - 이름은 구분자 `(` 로, 인자는 구분자 `,)` 로 원본 토큰 helper 가 읽는다(앞 space/tab 제거·따옴표 제거·입력 78글자 제한·뒤 공백 정리).
//  - 첫 빈 토큰에서 즉시 끝난다. 빈 따옴표 뒤의 다음 값을 다시 찾아 이어 가지 않는다.
//  - 비어 있지 않은 각 인자는 현재 설정 치환(`{키}` 풀기)을 거친 값을 `번호="값"` 줄로 같은 param 객체에 덧붙인다. 앞서 덧붙인 `{param.1}` 은 다음 인자의 치환에서 바로 보인다.
//  - 값은 첫 NUL 에서 끝나고 따옴표·개행을 따로 이스케이프하지 않는다(원본 서식 `%d="%s"`).
// 원본은 지역 입력을 1024바이트 배열에 복사하므로 NUL 을 포함해 1024바이트를 넘는 이름은 쓰기 전에 RangeError 로 거부한다(원본은 메모리가 깨지는 무효 입력).
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다. (2026-10-30 WB03-J: 미션 스크립트와 Options·Help 의 `Tell,이름(인자)` 명령을 위해 추가)
import { readRawTextToken } from './html-text.js';

// 원본 지역 입력 배열의 크기(종료 NUL 포함)이다. 이 크기 이상인 입력은 거부한다. (2026-10-30 WB03-J)
export const TELL_SOURCE_BYTES = 1024;

// 이름 토큰의 구분자이다(원본 PE 005152c8). (2026-10-30 WB03-J)
const NAME_DELIMITERS = '(';

// 인자 토큰의 구분자이다(원본 PE 005152c4). (2026-10-30 WB03-J)
const ARGUMENT_DELIMITERS = ',)';

// 문자열을 첫 NUL 까지로 자른다(C 문자열 경계). (2026-10-30 WB03-J)
function untilNul(text) {
    const nul = text.indexOf('\0');
    return nul >= 0 ? text.slice(0, nul) : text;
}

// Tell 대상 문자열에서 섹션 이름을 뽑고 인자를 hooks.append 로 `번호="값"` 줄로 덧붙인다. 섹션 이름을 돌려준다.
// hooks.substitute(text) → 현재 설정 치환 결과(원본 00441740), hooks.append(line) → 지역 param 객체에 줄 덧붙이기(원본 004402f0). 인자가 없으면 둘 다 필요하지 않다.
// 인자가 있는데 hooks 가 없으면 Error 이다. 호출자는 이 함수를 부르기 전에 param 객체를 등록해 두고 본문 처리가 끝날 때까지 유지해야 한다.
// 사용: const name = parseTellParameters('Foo({a},2)', { substitute: (t) => data.substitute(t), append: (line) => param.append(line) }). (2026-10-30 WB03-J)
export function parseTellParameters(source, hooks = {}) {
    const text = untilNul(source);
    if (text.length >= TELL_SOURCE_BYTES) throw new RangeError('Tell 이름이 원본 1024바이트 지역 입력 범위를 넘었습니다.');
    const name = readRawTextToken(text, 0, NAME_DELIMITERS);
    let offset = name.next;
    let index = 1;
    // 인자를 하나씩 읽는다. 첫 빈 토큰(인용한 빈 문자열 포함)에서 끝난다.
    for (;;) {
        const token = readRawTextToken(text, offset, ARGUMENT_DELIMITERS);
        offset = token.next;
        if (token.text.length === 0) break;
        if (!hooks.substitute || !hooks.append) throw new Error('Tell 파라미터의 설정 치환·줄 덧붙이기 연결이 없습니다.');
        const value = untilNul(hooks.substitute(token.text));
        hooks.append(`${index}="${value}"`);
        index += 1;
    }
    return name.text;
}
