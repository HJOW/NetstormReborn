# webpj 외부 접속 기능 목록과 처리 — WB02-E

2026-11-05, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 두 번째 선행 보완이다. AGENTS.md는 로컬 멀티플레이 접속을 제외한 외부 도메인/IP 접속 기능을 찾아 문서화하고 **클라이언트 유효성 검사를 비활성화**하라고 요구한다. 이 문서는 **웹 런타임(webpj/www)**의 요청·주소 소비자·미연결 기능·외부 실행 억제를 정리한다. 원본 바이너리/자료의 전체 주소 목록과 cpppj 처리는 [cpppj 단위684](exe/cpp-external-services-20261101.md)와 그 JSON 목록이 기준이며 여기서 다시 복제하지 않는다.

## 1. 결론

- 웹 런타임의 네트워크 요청은 **문서 기준 `./data/` 아래 같은 출처 정적 자산**뿐이다(`AssetLoader`). 외부 서버·DNS·업데이트·검증·진단·오류 보고 요청은 없다.
- 원본 자료의 바깥 주소는 링크를 누르면 **치환이 끝난 `http:`/`https:`/`mailto:` 주소만 브라우저 새 탭/창(메일은 메일 처리기)으로 연다.** `file:`·치환되지 않은 자리표시(`{…}`)·호스트 없는 주소·그 밖의 접두는 열지 않고 이유와 함께 콘솔로만 알린다. 주소 이동(`location`)·외부 프로그램 실행 경로는 없다. 2026-11-05 사용자 결정을 **2026-11-07 WB03-AD에서 구현했다(7절).** (그 전에는 모든 바깥 주소가 콘솔 안내뿐이었다.)
- 구 클라이언트 검증(`UserNotUpdated`, "Not Validated")은 **열지 않는다**(2026-09-30 프로젝트 결정, WB03-J 구현 유지). 검증 성공값을 합성하지도 않는다.
- 위 경계는 신규 검사 `webpj/tests/external-access.test.mjs`(5개)가 소스·자산 목록·앱 적재·링크·미연결 기능 단위로 고정한다.

## 2. 웹 런타임 요청 (같은 출처 자산)

| 소비자 | 요청 | 처리 |
| --- | --- | --- |
| `www/assets/asset-loader.js` `AssetLoader` | `manifest.json` → 묶음 파일 | `new URL(상대경로, baseUrl)`. 기본 `assetBase`는 `./data/`(문서 기준). manifest 경로는 모두 상대 경로이며 `..`·절대 주소가 없다 |
| `www/netstorm.js` `_load`·`_loadKorean`·`_startMusicDownload` | 묶음 `menu` 29파일(약7.43MB), 한국어 `korean` 15파일(약3.23MB, 한국어일 때만), `menu-music` 1파일(약17.99MB, 소리 사용 시 뒤에서) | 같은 `AssetLoader`. 호스트가 `fetch` 옵션을 주지 않으면 브라우저 `fetch`를 쓴다 |
| `www/platform/glyph-rasterizer.js` | 없음 | D2Coding WOFF2는 이미 받은 바이트로 `FontFace`를 만든다(URL 요청 없음) |
| `www/audio/audio-system.js` | 없음 | 받은 바이트를 Web Audio로 해독한다 |
| `www/platform/settings.js`·`progress.js` | 없음 | 설정/진행은 브라우저 `localStorage`만 쓴다(쿠키·서버 저장 없음) |
| `www/index.html` | 없음 | 바깥 스크립트·스타일·글꼴을 부르지 않는다. 모듈은 같은 폴더의 `netstorm.js`다 |

검사: 소스 정적 감사(주석 제외 코드에서 `XMLHttpRequest`·`WebSocket`·`EventSource`·`sendBeacon`·`RTCPeerConnection`·Worker·`location` 이동·전역 `open(`·`document.write`·절대 주소 문자열 금지, `fetch` 사용 파일은 `netstorm.js`·`asset-loader.js`뿐), manifest의 전 묶음 경로가 `https://netstorm.example/game/data/` 아래로 풀림, 영어/한국어 `_load()`의 실제 요청 목록이 manifest와 해당 묶음 파일과 정확히 같음. `help-window.js`의 `window.open(topic)`은 `HelpWindow` 지역 변수의 첫 주제 열기이며 브라우저 창 열기가 아니다.

## 3. 주소·링크 소비자

| 경로 | 원본 동작 | 웹 처리 |
| --- | --- | --- |
| Tell 본문 `<a href>` → `Netstorm._onDialogLink` → `_followLink` → `dispatchHelpLink` | 원본 4a00c0: `cmd:` 명령, 주제, 그 밖은 `autoOpen` 접두면 셸 실행·아니면 접두 이름의 명령 설정 실행 | `cmd:`는 대화상자 명령, 주제는 도움말 창, **바깥 주소는 `_openExternalAddress`가 판정해 열 수 있는 것만 새 탭/창으로 연다(7절)**. 한도(접두80/본문256) 초과는 경고 후 무시 |
| 도움말 창 링크 → `HelpWindow` `onExternal` → `_openHelp`의 훅 | 위와 같은 분배 | **같은 `_openExternalAddress`** |
| 시작 창 흐름 `_runMainMenuStartup` | Welcome / Blank → (`UserNotUpdated`) → Tips | `UserNotUpdated` 생략. Welcome 또는 Blank→Tips만 연다 |
| 대화상자 명령 `_runDialogCommand` | Tell·Config·TellTip·TechnicalHelp·Zsend·MissionBegin 등 | Tell/Config/TellTip만 실행, 나머지는 "아직 구현되지 않음" 안내만(TechnicalHelp·Zsend·서버 명령 없음) |
| HTML 지시문 `$Zsend`·`$Input` | 87h 요청 작성(구 서버 전송)·입력 창 | 컨트롤을 만들지 않고 `onUnsupported` 통지만 |
| Help/Options 펼침 메뉴 | Technical Help(외부 help.exe), Pass Server Diagnostic(방화벽/서버 진단), Direct Draw/Resolution | 웹 메뉴에서 제외(원본 스크린샷 비교용 `includeAll`에만 존재) |

### 3.1 웹에서 닿는 자료의 바깥 주소

조사 범위는 아카이브의 영어 `.english`/`.cfg`와 `www/data/d`의 한국어/느슨한 파일 63개다(독일어·프랑스어 등 웹 미사용 언어 제외). 주소처럼 보이는 문자열 **17개**, `<a href>` 목적지 분류는 **help179·script1·external11**이다. external 11종은 7절의 판정을 거친다: **치환 뒤 10종은 열리고 `file:NetStormPieceList.html` 1종만 열리지 않는다**(`http://{UPDATEURL}`은 게임 치환으로 `http://www.netstormhq.com`, `mailto:{EMAIL1}`은 `mailto:www.netstormhq.com`이 된다).

| 바깥 링크 목적지 | 출처 |
| --- | --- |
| `http://www.netstormhq.com` | tell(Tips 팁0·39 등)·help(영/한) |
| `http://te.netstormhq.com`, `http://www.activision.com` | help(영/한) |
| `http://www.netstormhq.com/content.php?content.27`, `http://www.microsoft.com`, `http://www.titanic.com/netstorm/buynetstormdirect.html`, `http://www.titanic.com/netstorm/demo.html`, `http://{UPDATEURL}`, `file:NetStormPieceList.html` | tell.english(구 업데이트/구매/데모/조각 목록 섹션) |
| `mailto:{EMAIL1}` | tell(영/한) |
| `mailto:bugs@titanic.com` | config(영/한) |

링크가 아닌 주소 문자열: `update.english`의 구 `netstorm.game-host.org/.../70Update_Alpha4_3~6.zip` 4개, config의 `http://www.activision.com,Visit…`/`http://www.titanic.com,Visit…`(원본 사이트 메뉴 값), tell의 `http://{{param.1}.WebSite}`(서버 목록 치환). 웹에는 이 값을 읽는 소비자(업데이트 다운로드·사이트 메뉴·서버 목록)가 없다.

## 4. 미연결·제외 기능 (외부 서비스)

| 원본 기능(근거: cpppj684) | 웹 상태 |
| --- | --- |
| Auto Updater 클라이언트 검증·`security.dat`·`UserUpdated`/`UserNotUpdated` | **비활성**. 업데이터·검증 요청·성공값 합성 없음. 시작 흐름이 안내 창을 열지 않음 |
| 인터넷 master/root 서버·디렉터리(`masterServer`, `!rootservers.dat`, nsLaunchC) | 없음. 멀티플레이 자체가 웹 제외 |
| ping/네트워크 진단(`Ping*`/`PingName*`, `diagServer`), Pass Server Diagnostic | 없음. 메뉴 항목 제외 |
| 로컬 LAN 서버(6799/6800/UDP8998)·Outpost | 웹 제외(AGENTS.md webpj 범위). 로컬 예외 규칙도 적용할 대상이 없음 |
| 사용자 선택 웹/메일/telnet 링크·`autoOpen`·프로토콜 명령 | `http:`/`https:`/`mailto:`만 7절 조건으로 새 탭/창·메일 처리기로 연다(2026-11-07 WB03-AD). `autoOpen`·프로토콜 명령 설정·`telnet:`·`file:`은 열지 않는다(콘솔 안내) |
| Technical Help(`techHelpExe`/`techHelpFile`) | 메뉴 제외·명령 미실행 |
| 구 업데이트 다운로드(`PatchUrl`, `update.english`) | 소비자 없음 |
| 오류 보고 주소(`bugreport.cgi`, `bugs@titanic.com`) | 자동 전송 없음. mailto 링크는 사용자가 누를 때만 메일 처리기로 넘긴다(7절) |

## 5. 개발·검사 도구 (런타임 아님)

| 도구 | 접속 |
| --- | --- |
| `webpj/server.js` | 개발용 정적 서버. 포트9803, 호스트 미지정이라 **모든 인터페이스에서 수신**한다(LAN의 다른 기기에서도 접근 가능). `www`만 공개하며 바깥으로 나가는 요청은 없다. 배포 제품이 아니다. 2026-11-05 사용자 결정으로 다른 PC 접속을 제한하지 않고 현행을 유지한다 |
| Playwright(`playwright.config.js`·`e2e/`) | `http://localhost:9803`만 연다 |
| `tools/prepare-assets.js`, `tools/make-*-expected.py` | 로컬 파일 읽기/쓰기만 한다(네트워크 모듈 없음) |

## 6. 검증과 남은 경계

- `node --test tests/external-access.test.mjs` 5/5 통과(소스 감사의 음성 대조 포함). 전체 결과는 인계 문서 0.1절에 기록한다.
- 주소 목록 집계는 임시 조사 스크립트로 했다. 원본 판독 규칙은 기존 `help-link.js` 분류를 그대로 썼다. 실제 DNS/HTTP 조회나 서버 상태 확인은 하지 않았다.
- 원본/CD 실행, analyzeManager, 디컴파일은 하지 않았다.
- 남겨 두었던 두 결정은 2026-11-05에 사용자가 정했다: 바깥 링크는 새 탭/창으로 연다(7절, 2026-11-07 구현), 개발 서버는 접속 제한 없이 유지한다.

## 7. 바깥 링크 새 탭/창 열기 — 구현 완료 (결정 2026-11-05, 구현 2026-11-07 WB03-AD)

사용자 결정: 원본 자료의 바깥 링크를 누르면 원본의 셸 실행(`autoOpen`/프로토콜 명령) 대신 **브라우저 새 탭이나 창으로 연다.** 구현은 다음과 같고, 이 절의 조건은 검사(`tests/external-link.test.mjs`·`tests/external-access.test.mjs`)가 고정한다.

| 항목 | 구현 |
| --- | --- |
| 바꾼 자리 | `www/netstorm.js`의 `_followLink` external 훅과 `_openHelp`의 `onExternal`이 같은 `_openExternalAddress(원문, 출처)`를 부른다. 분류 규칙(`ui/help-link.js`)과 대화상자를 먼저 닫는 순서·`$OnExit` 실행 순서는 그대로다 |
| 판정 | `www/ui/external-link.js`의 `resolveExternalLink(원문)`(DOM 없음). 첫 NUL 앞까지만 읽고 앞뒤 공백을 지운다. 접두는 ASCII 대소문자를 구분하지 않는다 |
| 여는 대상 | 치환이 끝난 `http:`/`https:`(`//호스트`가 있고 URL 해석이 같은 접두·비지 않은 호스트를 돌려주는 것)는 새 탭/창, `mailto:`(받는 사람이 비지 않은 것)는 메일 처리기. 주소는 URL 해석으로 정규화한다(`http://www.netstormhq.com` → `http://www.netstormhq.com/`). `mailto:`는 원문을 그대로 쓴다 |
| 열지 않는 대상 | `file:`(원본 PieceDump가 로컬에 만드는 파일, 웹에 없음)·`telnet:`·`javascript:`·`data:` 등 그 밖의 접두, 접두가 없는 주소, 치환 뒤에도 `{…}`가 남거나 공백·제어 문자가 있는 주소(예: `http://{{param.1}.WebSite}`), 호스트가 없는 웹 주소(`http://`·`http:///x`), 받는 사람이 없는 메일, 2048자를 넘는 주소는 이유와 함께 콘솔 안내만 한다. 게임 치환(`gameData.substitute`) 뒤의 `http://{UPDATEURL}`은 `http://www.netstormhq.com`, `mailto:{EMAIL1}`은 `mailto:www.netstormhq.com`(`@` 없음)이 되어 둘 다 열린다. 원본도 이 주소를 그대로 셸/메일 처리기에 넘겼으므로 `@`는 요구하지 않는다 |
| 브라우저 조건 | 열기는 `www/platform/external-opener.js`의 `openExternalLink(문서, 판정)` 한 곳이다: 숨은 `<a>`를 만들어 `href`를 주고 `click()`한다. 웹 주소는 `target="_blank"`·`rel="noopener noreferrer"`로 새 탭/창에 열어 게임 창(opener)을 건드리지 못하고 Referer도 보내지 않는다. 메일 주소는 target 없이 메일 처리기에 넘긴다. 링크를 누른 `pointerdown` 처리 안에서 동기로 불리므로 팝업 차단 대상이 아니다. 열기는 `click()` 예외만 실패로 알 수 있다 |
| `window.open`을 쓰지 않은 이유 | `noopener`로 열면 성공해도 `null`을 돌려주어 차단과 구분되지 않는다(2026-11-05 구상의 "null 반환은 콘솔 안내"는 이 때문에 `click()` 예외로 바꿨다). 소스 감사의 창 열기 허용 경로(`window.open(` 도움말 창 지역 변수 한 곳)도 그대로 둘 수 있다 |
| 호스트 훅 | `new Netstorm({ openExternal: (문서, 판정) => boolean })`로 열기 장치를 바꾼다(기본 `null` = 위 장치). 무브라우저 실행 `tools/headless.mjs`는 실제 탭을 열지 않고 `openedLinks`에 주소만 기록한다. 열기 장치가 `false`를 돌려주면 경고만 하고 게임은 계속한다 |
| 검사 갱신 | 소스 감사: 앵커 요소 생성(`createElement('a')`)은 `platform/external-opener.js` 한 파일뿐, `window.open(`은 그대로 도움말 창 한 곳. 링크 검사: 열 수 있는 주소는 열기 장치로, `{UPDATEURL}`·`{EMAIL1}` 미치환 원문과 `file:`은 이유와 함께 알림만. 새 검사 3개: 판정 경계(열림 7종·거부 30여 종 + 불변 객체), 가짜 문서로 앵커 속성·click·실패, 실제 자료(문서 3.1절 11종의 치환 뒤 판정 + 도움말 전 주제의 바깥 링크) |
| 실제 브라우저 확인 | 하지 않았다. 사용자 클릭 입력 안에서 새 탭이 열리는지(팝업 차단·포커스 이동 뒤 포인터 뗌 처리)는 실제 브라우저 확인 대상이며 이 단위의 정적/단위 검사에 포함되지 않는다. 새 탭이 열리면 게임 캔버스의 `blur`가 입력을 비운다(기존 동작) |
| 범위 밖 | 구 업데이트 다운로드·검증·서버 목록 등 4절의 미연결 기능은 링크 열기와 별개이며 계속 만들지 않는다. 원본 주소 도메인의 생존 여부는 확인하지 않아 사라진 사이트가 열릴 수 있다 |
