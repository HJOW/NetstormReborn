# webpj webpack 번들 도입 (WB04-E1293, 2026-11-24)

사용자 지시(2026-11-24): "webpack 을 도입해 스크립트 전체를 번들 하나로 묶어 보고, 용량이 과도하게 크면(기준 9.9MB 정도) 롤백을 고려한다." 모든 웹 스크립트를 `www/dist/netstorm.bundle.js` 하나로 묶어 `index.html`이 그것만 불러오게 했다.

## 결과 요약 (크기 판정)

| 빌드 | 크기 | 9.9MB 기준 |
| --- | --- | --- |
| **운영(`npm run build`, 최소화)** | **3,419,745바이트 = 3.42MB(10진) = 3.26MiB** | **통과**(한도의 약 35%) |
| 개발(`npm run build:dev`, 최소화 없음) | 11,390,370바이트 = 11.39MB = 10.9MiB (+ 소스 맵 11.5MB) | 초과 — 개발 번들은 배포하지 않는다 |

- 번들에는 진입 모듈에서 import로 닿는 988개 모듈이 들어간다(`www` 아래 .js 1,056개 중 68개는 앱이 import하지 않는 미연결 모듈이라 빠진다). 원본 소스 합계는 주석 포함 약 10.2MB였고 최소화로 주석·공백이 빠져 3.4MB가 됐다. **롤백 기준은 배포할 운영 번들에 적용했고 통과했다.** 개발 번들이 9.9MB를 넘는 것은 주석(한국어 설명)이 그대로 남기 때문이다.
- 게임 자료(`www/data`)·글꼴은 번들에 넣지 않았다. 지금처럼 같은 출처에서 따로 받는다(이 크기에 포함되지 않는다).
- 운영 빌드는 약 19초(전체 재빌드), 개발 빌드는 약 2초이다.

## 구성

- `webpack.config.js`: 진입 `www/bootstrap.js` → `www/dist/netstorm.bundle.js` 하나. `splitChunks: false`·`runtimeChunk: false`로 조각을 나누지 않는다. 운영은 Terser 최소화(`keep_classnames`·`keep_fnames`: 시험·진단이 `constructor.name`과 함수 이름을 읽기 때문), 라이선스 주석 파일 없음. `output.publicPath: 'auto'`로 하위 경로 정적 배포에서도 번들 자신의 주소를 기준으로 한다(추가 조각이 없어 실제로는 쓰이지 않는다).
- `www/bootstrap.js`(신규, 웹 전용 모듈로 대응표 `WEB_ONLY`에 등록): 예전 `index.html` 인라인 모듈 스크립트의 내용(DOMContentLoaded 뒤 `new Netstorm().init(root)`, `window.netstorm` 노출)을 그대로 옮겼다.
- `www/index.html`: 인라인 모듈 스크립트를 `<script src="dist/netstorm.bundle.js" defer>` 하나로 바꿨다. 상대 경로라 하위 경로 배포가 유지된다. 게임은 이 번들 하나로 구동한다.
- `www/debug.html`(신규, 사용자 지시 2026-11-24): **번들을 쓰지 않고 이전 방식대로** `netstorm.js` ES 모듈을 직접 불러와 게임을 구동하는 디버그용 진입 페이지이다(번들 도입 전 `index.html`과 같은 인라인 모듈 스크립트). 소스를 고친 즉시 번들 빌드 없이 확인하거나 소스 그대로의 스택으로 디버깅할 때 쓰고(`npm start` 뒤 `http://localhost:9803/debug.html`), `import()`로 앱과 같은 모듈 인스턴스를 함께 쓰는 e2e 11개 파일이 이 페이지를 연다(아래 전체 e2e 참조).
- `tools/check-bundle-size.js`(`npm run check-bundle`): 번들이 9,900,000바이트(9.9MB, 10진)를 넘으면 종료 코드 1로 끝난다. `npm run build`는 webpack 뒤 이 검사를 이어서 돌린다.
- `tools/ensure-bundle.js`(`npm run ensure-bundle`): 번들이 없거나 `www` 소스(data·dist 제외)·`webpack.config.js`·`package-lock.json`보다 오래됐으면 운영 번들을 다시 만들고 크기를 검사한다. `npm start`(`prestart`)와 Playwright `webServer.command`(`node tools/ensure-bundle.js && node server.js`, 대기 240초)가 쓴다. 이미 떠 있는 서버를 재사용하는 e2e 실행에서는 번들을 다시 만들지 않으므로 소스를 고친 뒤에는 서버를 내리고 시작한다.
- `package.json`: 개발 의존성 `webpack` 5.113.0·`webpack-cli` 7.3.1·`terser-webpack-plugin` 5.6.1(정확한 버전 고정), 스크립트 `build`·`build:dev`·`check-bundle`·`ensure-bundle`·`prestart`. `package-lock.json` 갱신.
- **Git 추적(사용자 결정 2026-11-24 "번들도 git 에 넣자"):** 번들(`www/dist/netstorm.bundle.js`, 3.4MB)을 Git에 추적한다. 처음에는 산출물로 보고 `.gitignore`에 넣었으나 결정에 따라 제외를 지웠고, 줄바꿈 변환(`core.autocrlf=true`)이 바이트를 바꾸지 않도록 `.gitattributes`에 `webpj/www/dist/** -text`를 더했다. `www`를 Git에서 그대로 정적 배포해도 번들이 있어 동작한다. 대신 **`www` 소스를 고친 단위는 `npm --prefix webpj run build`로 번들을 다시 만들어 함께 커밋해야 한다**(`ensure-bundle`이 e2e·`npm start` 때 자동으로 만들기도 한다). 새 시험 `tests/bundle-fresh.test.mjs`가 같은 설정으로 임시 폴더에 다시 빌드해(약 22초) 커밋된 번들과 바이트가 같은지·크기 한도 안인지 확인하며, 소스만 고치고 번들을 빠뜨리면 실패한다(소스에 한 줄을 더해 실패함을 확인했다).
- 감사·도구: 외부 접속 감사(`tests/external-access.test.mjs`)와 대응표 도구(`tools/port-map.mjs`의 `listWebModules`)는 `www/dist`를 소스로 보지 않는다(번들 산출물이고 원본 소스는 따로 감사된다). `e2e/static-deployment.spec.js`는 이제 `dist/netstorm.bundle.js` 요청을 확인한다.
- Node 시험(`tests/*.test.mjs`)은 `www` ES 모듈을 직접 import하므로 번들과 무관하게 그대로 돈다. 번들은 Chromium e2e와 실제 실행에서만 쓰인다.

## 검증

- 번들이 만들어지고 경고·오류 없이 컴파일됨(webpack 5.113.0). 모든 import는 확장자가 명시된 ES 모듈이며 `import.meta`·동적 import·워커·CommonJS 호출이 없어 설정 변경 없이 묶였다(`require`라는 이름의 지역 함수 몇 개는 webpack이 CommonJS 호출로 보지 않았다).
- 대응표 검사 오류 0(`bootstrap.js`를 `WEB_ONLY`에 등록하기 전에는 "대응도 웹 전용 표시도 없는 웹 모듈" 오류 1), 외부 접속 감사·대응표·자산 Node 시험 58건 통과.
- 번들 기준 e2e: 정적 하위 경로 배포·스모크·최초 실행·오류 보고·favicon·외부 링크 열기·title·언어 선택 11건 통과. 전체 e2e는 아래 '전체 e2e' 항목을 본다.

## 전체 e2e

번들 기준 전체 `npx playwright test`(로그 `webpj/extracted/webpj-e1293-e2e-all.log`, 23.9분): **166건 통과·12건 건너뜀(옵트인)·5건 실패**. 실패 5건(`client-clock`·`client-text-entry-galmuri`·`internal-transport-packets`·`korean-ingame` 채팅 도우미·`tutorial-menu-start` 첫 Tutorial)은 모두 번들의 결함이 아니라 시험 방식의 문제였다: 이 시험들은 페이지에서 `await import('/ui/….js')`로 모듈을 따로 불러와 앱과 **같은 모듈 인스턴스**를 가정한다(프로토타입을 감싸 패킷·번역 호출을 세거나 Gump 클래스를 직접 만든다). 번들에서는 모듈이 번들 안에 있어 URL로 접근하면 별개의 사본이 만들어진다. 그래서 `import()`를 쓰는 e2e 11개 파일(`client-clock`·`client-text-entry-galmuri`·`fixed-time`·`galmuri-bold-fallback`·`internal-transport-packets`·`korean-ingame`·`l2-tutorial1`·`l3-tutorial1`·`send-money-dialog`·`tutorial-menu-start`·`web-sound-device`)이 `debug.html`(번들 없는 모듈 직접 적재)을 열도록 바꿨고, 그 11개 파일 39건이 모두 통과했다. 나머지 e2e 약 150건은 `index.html`의 번들로 통과했다. 새 스모크 e2e(`e2e/smoke.spec.js`) 2건은 `index.html`이 `/dist/netstorm.bundle.js` 하나만 불러오고 `debug.html`이 번들 없이 ES 모듈을 100개 넘게 하나씩 불러온다는 것을 요청 목록으로 확인한다. 바꾼 뒤 전체 e2e를 처음부터 다시 돌리지는 않았다(실패 5건이 든 11개 파일만 재실행).

## 결정 대기·주의

- **번들 Git 추적 여부(결정됨).** 추적한다(위). 소스를 고칠 때마다 3.4MB 번들 변경이 쌓이는 비용은 사용자가 받아들였다. 번들은 결정적으로 같은 바이트를 만든다(같은 소스·같은 webpack/terser 버전에서). 의존성 버전(`package-lock.json`)을 올리면 번들 바이트가 달라질 수 있어 그때도 다시 빌드해 커밋한다.
- **롤백 방법.** 이 단위의 커밋을 되돌리면 `index.html`이 다시 모듈 스크립트를 쓰고(원본 소스는 건드리지 않았다) `www/bootstrap.js`·`www/debug.html`·설정·도구·의존성만 사라진다. 또는 번들 없이 쓰려면 `debug.html`을 열면 된다.
- 운영 번들의 오류 스택은 최소화되어 읽기 어렵다. 디버깅은 `npm run build:dev`(소스 맵 포함)로 한다.
- 번들 도입은 코드 변환(Babel 등)을 하지 않는다. 지원 브라우저는 이전과 같이 ES 모듈을 쓰는 최신 브라우저이다.
