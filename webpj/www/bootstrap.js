// 웹 판본의 진입 모듈이다. webpack 이 이 모듈에서 import 로 닿는 모든 스크립트를 번들 하나(`dist/netstorm.bundle.js`)로 묶으며, index.html 은 그 번들 하나만 불러온다.
// 예전에는 index.html 의 인라인 모듈 스크립트가 같은 일을 했다: Netstorm 을 만들어 `#netstorm_root` 에 붙이고 `window.netstorm` 으로 노출한다.
// 사용: DOMContentLoaded 뒤 `new Netstorm().init(root)` 를 한 번 부르고 개발자 콘솔·e2e 시험이 쓰는 전역 `window.netstorm` 을 정한다. 이 모듈은 부작용만 가지며 내보내는 것이 없다.
// (2026-11-24 WB04-E1293: webpack 도입으로 index.html 의 인라인 모듈 스크립트를 이 모듈로 옮겼다)
import { Netstorm } from './netstorm.js';

// 문서가 준비되면 앱을 만들어 시작한다(번들 스크립트는 defer 로 불려 DOMContentLoaded 앞에서 실행된다).
document.addEventListener('DOMContentLoaded', function () {
    const netstorm = new Netstorm();
    netstorm.init(document.getElementById('netstorm_root'));
    window.netstorm = netstorm; // 개발자 콘솔에서 setAspect() 등을 시험하기 위한 임시 노출이며 e2e 시험도 이 전역을 쓴다.
});
