// 판정을 마친 바깥 주소(ui/external-link.js)를 브라우저로 실제로 여는 플랫폼 경계 모듈이다. 게임 코어는 이 모듈을 직접 알지 못하고 호스트 훅으로만 부른다.
// 여는 방식은 숨은 `<a>` 요소의 click() 이다.
//  - 웹 주소: `target="_blank"` 와 `rel="noopener noreferrer"` 로 새 탭/창에 연다. 게임 창(opener)을 건드리지 못하고 이전 주소(Referer)도 보내지 않는다.
//  - 메일 주소: target 없이 브라우저의 메일 처리기로 넘긴다. 처리기가 없으면 아무 일도 없고 게임 화면은 그대로다.
// 팝업 차단을 피하려면 사용자 입력 처리 안에서 동기로 불려야 한다(링크를 누른 pointerdown 처리 안에서 불림). 이 모듈은 비동기 대기를 하지 않는다.
// `window.open` 을 쓰지 않는 이유: 보안 기능 `noopener` 로 열면 성공해도 null 을 돌려주어 실패와 구분할 수 없고, 소스 감사(tests/external-access.test.mjs)도
// 창 열기 API 를 도움말 창의 지역 변수 한 곳으로 한정한다. 이 모듈의 DOM 접근은 문서 하나(document.createElement)와 요소 click() 뿐이다.
// 열기 성공 여부는 알 수 없다(브라우저가 알려주지 않음). 요소를 만들거나 click() 하다 예외가 나면 false 를 돌려주고 게임 진행은 계속한다.
// (2026-11-07 WB03-AD: 바깥 링크 새 탭/창 열기)
import { EXTERNAL_LINK_KIND } from '../ui/external-link.js';

// 주소 하나를 연다. document 는 요소를 만들 문서이고, link 는 resolveExternalLink 가 열 수 있다고 판정한 결과이다.
// 반환: click() 까지 마쳤으면 true, 열 수 없는 판정이거나 문서가 없거나 예외가 났으면 false.
// 사용: openExternalLink(root.ownerDocument, resolveExternalLink('http://www.netstormhq.com')). (2026-11-07 WB03-AD)
export function openExternalLink(document, link) {
    if (!link || link.open !== true || typeof link.url !== 'string' || typeof document?.createElement !== 'function') return false;
    try {
        const anchor = document.createElement('a');
        anchor.href = link.url;
        // 웹 주소만 새 탭/창으로 연다. 메일 주소는 현재 문서를 떠나지 않도록 target 을 주지 않는다.
        if (link.kind === EXTERNAL_LINK_KIND.WEB) {
            anchor.target = '_blank';
            anchor.rel = 'noopener noreferrer';
        }
        anchor.click();
        return true;
    } catch (error) {
        console.warn('외부 링크를 열지 못했습니다.', error);
        return false;
    }
}
