# webpj 외부 접속 기능 목록과 처리 — WB02-E

2026-11-05, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 두 번째 선행 보완이다. AGENTS.md는 로컬 멀티플레이 접속을 제외한 외부 도메인/IP 접속 기능을 찾아 문서화하고 **클라이언트 유효성 검사를 비활성화**하라고 요구한다. 이 문서는 **웹 런타임(webpj/www)**의 요청·주소 소비자·미연결 기능·외부 실행 억제를 정리한다. 원본 바이너리/자료의 전체 주소 목록과 cpppj 처리는 [cpppj 단위684](exe/cpp-external-services-20261101.md)와 그 JSON 목록이 기준이며 여기서 다시 복제하지 않는다.

## 1. 결론

- 웹 런타임의 네트워크 요청은 **문서 기준 `./data/` 아래 같은 출처 정적 자산**뿐이다(`AssetLoader`). 외부 서버·DNS·업데이트·검증·진단·오류 보고 요청은 없다.
- 원본 자료의 바깥 주소(`http:`·`mailto:`·`file:`·치환 주소)는 링크로 눌러도 **열지 않고 콘솔 안내만** 한다. 새 창·주소 이동·외부 프로그램 실행 경로가 없다.
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
| Tell 본문 `<a href>` → `Netstorm._onDialogLink` → `_followLink` → `dispatchHelpLink` | 원본 4a00c0: `cmd:` 명령, 주제, 그 밖은 `autoOpen` 접두면 셸 실행·아니면 접두 이름의 명령 설정 실행 | `cmd:`는 대화상자 명령, 주제는 도움말 창, **바깥 주소는 `console.info`만**. 한도(접두80/본문256) 초과는 경고 후 무시 |
| 도움말 창 링크 → `HelpWindow` `onExternal` → `_openHelp`의 훅 | 위와 같은 분배 | **바깥 주소는 `console.info`만** |
| 시작 창 흐름 `_runMainMenuStartup` | Welcome / Blank → (`UserNotUpdated`) → Tips | `UserNotUpdated` 생략. Welcome 또는 Blank→Tips만 연다 |
| 대화상자 명령 `_runDialogCommand` | Tell·Config·TellTip·TechnicalHelp·Zsend·MissionBegin 등 | Tell/Config/TellTip만 실행, 나머지는 "아직 구현되지 않음" 안내만(TechnicalHelp·Zsend·서버 명령 없음) |
| HTML 지시문 `$Zsend`·`$Input` | 87h 요청 작성(구 서버 전송)·입력 창 | 컨트롤을 만들지 않고 `onUnsupported` 통지만 |
| Help/Options 펼침 메뉴 | Technical Help(외부 help.exe), Pass Server Diagnostic(방화벽/서버 진단), Direct Draw/Resolution | 웹 메뉴에서 제외(원본 스크린샷 비교용 `includeAll`에만 존재) |

### 3.1 웹에서 닿는 자료의 바깥 주소

조사 범위는 아카이브의 영어 `.english`/`.cfg`와 `www/data/d`의 한국어/느슨한 파일 63개다(독일어·프랑스어 등 웹 미사용 언어 제외). 주소처럼 보이는 문자열 **17개**, `<a href>` 목적지 분류는 **help179·script1·external11**이다. external 11종은 모두 위 3절의 콘솔 안내로 끝난다.

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
| 사용자 선택 웹/메일/telnet 링크·`autoOpen`·프로토콜 명령 | 열지 않음(콘솔 안내). 브라우저 새 창/`mailto:` 실행도 하지 않음 |
| Technical Help(`techHelpExe`/`techHelpFile`) | 메뉴 제외·명령 미실행 |
| 구 업데이트 다운로드(`PatchUrl`, `update.english`) | 소비자 없음 |
| 오류 보고 주소(`bugreport.cgi`, `bugs@titanic.com`) | 자동 전송 없음. mailto 링크는 콘솔 안내만 |

## 5. 개발·검사 도구 (런타임 아님)

| 도구 | 접속 |
| --- | --- |
| `webpj/server.js` | 개발용 정적 서버. 포트9803, 호스트 미지정이라 **모든 인터페이스에서 수신**한다(LAN의 다른 기기에서도 접근 가능). `www`만 공개하며 바깥으로 나가는 요청은 없다. 배포 제품이 아니며 이번에 바꾸지 않았다 |
| Playwright(`playwright.config.js`·`e2e/`) | `http://localhost:9803`만 연다 |
| `tools/prepare-assets.js`, `tools/make-*-expected.py` | 로컬 파일 읽기/쓰기만 한다(네트워크 모듈 없음) |

## 6. 검증과 남은 경계

- `node --test tests/external-access.test.mjs` 5/5 통과(소스 감사의 음성 대조 포함). 전체 결과는 인계 문서 0.1절에 기록한다.
- 주소 목록 집계는 임시 조사 스크립트로 했다. 원본 판독 규칙은 기존 `help-link.js` 분류를 그대로 썼다. 실제 DNS/HTTP 조회나 서버 상태 확인은 하지 않았다.
- 원본/CD 실행, analyzeManager, 디컴파일은 하지 않았다.
- 바깥 링크를 사용자 선택으로 새 탭에 열지(현재는 열지 않음) 여부, 개발 서버를 `localhost`로 한정할지 여부는 개발자 결정 사항으로 남긴다. 현재 정책은 WB03-K의 "외부 링크를 열지 않음"을 유지한다.
