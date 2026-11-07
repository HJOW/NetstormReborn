// 원본 자료의 바깥 링크(`http:`·`https:`·`mailto:` 등)를 브라우저로 열 수 있는지 판정하는 모듈이다. DOM 을 쓰지 않아 Node 에서도 검사할 수 있다.
// 원본은 바깥 주소를 설정 `autoOpen` 접두면 셸로 실행했다(4a00c0). 웹은 2026-11-05 사용자 결정에 따라 셸 실행 대신 **브라우저 새 탭/창**으로 연다.
// 열 대상은 치환이 끝난 `http:`/`https:` 주소(호스트가 있는 것)와 `mailto:`(받는 사람이 있는 것)뿐이다.
// 열지 않는 대상은 다음과 같고, 호출자가 그 이유를 콘솔로 알린다.
//  - `file:`(원본 PieceDump 가 로컬에 만드는 파일이라 웹에 없음), `telnet:`·`javascript:`·`data:` 등 그 밖의 접두
//  - 치환 뒤에도 `{…}` 가 남은 주소(예: `http://{UPDATEURL}`, `mailto:{EMAIL1}`)나 호스트·받는 사람이 빈 주소
//  - 공백·제어 문자를 품은 주소(주소가 아닌 글자가 섞인 경우)
// 원본 주소의 도메인이 지금 살아 있는지는 확인하지 않는다(사라진 사이트가 열릴 수 있음).
// (2026-11-07 WB03-AD: 바깥 링크 새 탭/창 열기 — docs/webpj-external-access-20261105.md 7절 구현)
import { asciiLower } from '../assets/config.js';

// 열 수 있는 주소의 종류 이름이다. 호출자(열기 장치)가 종류별로 열기 방식을 고른다. (2026-11-07 WB03-AD)
export const EXTERNAL_LINK_KIND = Object.freeze({ WEB: 'web', MAIL: 'mail' });

// 주소 문자열 전체 길이의 한도이다. 원본 링크 본문 한도(256)보다 넉넉히 잡은 방어값이며, 이를 넘으면 열지 않는다. (2026-11-07 WB03-AD)
export const EXTERNAL_LINK_MAX_LENGTH = 2048;

// 주소에 들어가면 안 되는 글자(공백·제어 문자·치환 자리표시 괄호)이다. (2026-11-07 WB03-AD)
const FORBIDDEN_CHARACTERS = /[\u0000- \u007f-\u009f{}]/;

// 닫힌 판정을 만든다. reason 은 콘솔 안내에 붙는 한국어 사유이다. (2026-11-07 WB03-AD)
function refuse(reason) {
    return Object.freeze({ open: false, kind: null, url: null, reason });
}

// 바깥 주소 원문 하나를 판정한다.
// 반환: 열 수 있으면 { open: true, kind: 'web'|'mail', url, reason: null }, 아니면 { open: false, kind: null, url: null, reason }.
// 원문은 첫 NUL 앞까지만 읽고(원본 C 문자열) 앞뒤 공백은 지운다. 접두는 ASCII 대소문자를 구분하지 않는다.
// 사용: resolveExternalLink('http://www.netstormhq.com') → { open: true, kind: 'web', url: 'http://www.netstormhq.com/' }. (2026-11-07 WB03-AD)
export function resolveExternalLink(original) {
    const nul = String(original).indexOf('\0');
    const text = (nul < 0 ? String(original) : String(original).slice(0, nul)).trim();
    if (text === '') return refuse('빈 주소입니다.');
    if (text.length > EXTERNAL_LINK_MAX_LENGTH) return refuse('주소가 너무 깁니다.');
    const colon = text.indexOf(':');
    const scheme = colon < 0 ? '' : asciiLower(text.slice(0, colon));
    if (scheme !== 'http' && scheme !== 'https' && scheme !== 'mailto') return refuse(scheme === '' ? '접두가 없는 주소입니다.' : `'${scheme}:' 주소는 웹에서 열지 않습니다.`);
    // 자리표시(`{…}`)가 치환되지 않았거나 공백·제어 문자가 있으면 주소가 아니다.
    if (FORBIDDEN_CHARACTERS.test(text)) return refuse('치환되지 않은 자리표시나 공백이 남은 주소입니다.');
    if (scheme === 'mailto') {
        // 받는 사람은 `?` 앞부분이며 비어 있으면 열지 않는다. 원본 자료의 `{EMAIL1}` 은 치환 뒤 `www.netstormhq.com` 이라 `@` 가 없지만,
        // 원본도 그 주소를 그대로 메일 처리기에 넘겼으므로 `@` 는 요구하지 않는다(메일 처리기가 다루는 일).
        const recipients = text.slice(colon + 1).split('?')[0];
        if (recipients === '') return refuse('받는 사람이 없는 메일 주소입니다.');
        return Object.freeze({ open: true, kind: EXTERNAL_LINK_KIND.MAIL, url: text, reason: null });
    }
    // 웹 주소는 `//호스트` 를 갖춰야 하며 URL 해석이 같은 접두와 비지 않은 호스트를 돌려줘야 한다.
    if (!/^[a-z]+:\/\/[^/?#]/i.test(text)) return refuse('호스트가 없는 웹 주소입니다.');
    let parsed;
    try {
        parsed = new URL(text);
    } catch {
        return refuse('해석할 수 없는 웹 주소입니다.');
    }
    // URL 해석이 돌려주는 접두(`scheme:`)가 위에서 읽은 접두와 같아야 한다(소스 감사가 접두 문자열 상수를 금지하므로 비교 값을 만들어 쓴다).
    if (parsed.protocol !== `${scheme}:` || parsed.hostname === '') return refuse('호스트가 없는 웹 주소입니다.');
    return Object.freeze({ open: true, kind: EXTERNAL_LINK_KIND.WEB, url: parsed.href, reason: null });
}
