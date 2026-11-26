# WebMCP 실제 게임 조작·캡처·소리 포함 녹화

- 단위: WB04-E1372, 2026-11-26, HJOW-Athlon.
- 대상: 현행 Document WebMCP를 제공하는 브라우저. 실측 Chromium 156.0.8078.4에서 --enable-features=WebMCP 옵션으로 확인했다. 기본 Chromium은 API가 없어도 기존 시작 화면과 게임이 정상 동작한다.
- 구현: [등록](../webpj/www/platform/webmcp-tools.js), [실제 게임 어댑터](../webpj/www/platform/game-agent-host.js), [녹화](../webpj/www/platform/game-recorder.js), [브라우저 검사](../webpj/e2e/webmcp.spec.js).

## 사용 방법

정적 서버를 켜고 지원 브라우저에서 게임을 연다. 최초 시작 화면은 실제 사용자가 한 번 클릭해 Web Audio 장치를 준비한다. 게임 초기화 때 document.modelContext에 아래 도구가 자동 등록된다. 별도 MCP 서버·외부 전송·마이크 권한은 필요하지 않다. 페이지 도구 호출이 가능한 브라우저 에이전트에서 사용한다. 지원 여부는 document.modelContext?.registerTool로 확인한다. 현재 검증한 브라우저 실행 옵션은 --enable-features=WebMCP다.

| 도구 | 입력과 결과 |
| --- | --- |
| netstorm_get_state | 입력 {}. 앱 단계·현재 미션·논리 크기·게임 초·선택 SID·로컬 소유자/SP·포인터/누른 키·표시 Tell 조작점·녹화 상태의 값 사본 |
| netstorm_input | type=pointer/wheel/key/text. 기존 Canvas DOM 사건으로 전달하고 accepted를 반환 |
| netstorm_capture | 입력 {}. 현재 표시 Canvas의 PNG를 MCP image 콘텐츠(base64)와 실제 크기로 반환 |
| netstorm_recording_start | framesPerSecond 5~60(기본30), maximumSeconds 1~1800(기본300). 화면과 효과음·음악을 WebM으로 녹화 |
| netstorm_recording_stop | download 불리언(기본false). 마지막 인코딩 조각까지 기다린 후 Blob URL·파일명·바이트·경과 벽시각·downloadStarted 반환. true면 브라우저 파일 내려받기 실행 |

포인터 좌표 x/y는 현재 논리 화면의 범위 안이다. CSS 확대/축소를 실제 Canvas 좌표로 변환한다. action은 move/down/up/click(기본click), button은 left/middle/right(기본left)다. 휠은 x/y와 deltaY(-1000~1000)를 받는다. 키는 key와 선택 code, action=down/up/press(기본press), 보조 키는 shift/control/alt 불리언을 각 사건에 명시한다. 연속 끌기는 down→move→up으로 전달한다. 포커스 이탈로 실제 입력이 풀리면 다음 도구 입력도 같은 상태를 따른다.

확정 문자는 type=text, text 문자열로 한 번에 UTF-16 16단위 이하를 전달한다. 줄바꿈·제어문자는 거부하며 Enter/Backspace 등은 key 입력으로 보낸다. 원래 IME 확정 사건 경로를 사용한다. 입력 큐가 부족하면 오류를 반환하므로 다음 게임 프레임 후 다시 시도한다. DOM 사건의 accepted는 큐/게임에서 명령이 완료됐다는 뜻이 아니므로 화면·상태를 다시 조회한다.

예: netstorm_input에 {"type":"key","key":"P"}를 주면 실제 사제 선택 단축키가 처리된다. {"type":"key","key":"F9","shift":true}는 Shift-F9이고, {"type":"pointer","action":"click","button":"right","x":400,"y":300}은 논리 화면의 우클릭이다. 메뉴의 모든 이름을 나열하는 API는 아니므로 현재 화면 캡처와 표시되는 조작점을 함께 사용한다.

## 수명과 소리 경로

각 도구 등록은 같은 AbortController 수명에 속한다. 초기화 도중 실패해도 이번 앱의 부분 등록만 취소하고, 다른 앱/문서 도구는 건드리지 않는다. 앱 destroy 때 등록을 먼저 해제하고 녹화 종료·반납을 시작한 뒤 오디오를 닫는다. 녹화 중이었다면 마지막 청크 종료 콜백에서 트랙·타이머를 정리하고 반납 완료 시 Blob URL도 폐기한다. 호스트 오류는 isError와 원래 name/message로 전달한다. 요청 취소는 표준 AbortError를 유지한다.

AudioSystem에 gain=1 출력 버스를 두고 음악·Web Audio 효과음과 원본형 WebSoundDevice의 최종 출력을 모은다. 원본 효과음은 이미 음량이 반영되므로 soundGain에 다시 연결하지 않는다. 녹화는 이 최종 버스를 MediaStreamDestination으로 분기하며 스피커 연결은 유지한다. Canvas captureStream의 영상과 게임 소리 트랙만 MediaRecorder에 전달한다. getUserMedia·getDisplayMedia를 사용하지 않는다.

녹화는 한 번에 하나이며 시간 한도 또는 256MiB 청크 경계에서 자동 종료한다(인코더의 다음 청크까지는 넘을 수 있음). 기본 VP8/Opus, 지원 여부에 따라 VP9/Opus 또는 WebM을 쓴다. 게임 일시 정지와 별개인 벽시각으로 녹화 길이를 센다. 다음 녹화 시작 또는 앱 반납 시 이전 Blob URL을 폐기한다. 결과를 보존하려면 그 전에 내려받는다. 다운로드는 기존 platform/external-opener.js의 downloadBytes를 사용한다.

## 실측 검증

제품 모듈을 대체하거나 도구를 수동 등록하지 않고 실제 앱의 자동 등록을 검사했다. 기본 브라우저와 WebMCP 지원 브라우저 두 경우가 Chromium 2/2 통과했다. 지원 브라우저에서는 실제 시작 클릭 → Campaign → Struggle For Freedom → The War Begins → Play Mission을 도구 입력으로 진행했다. PNG 캡처, P 단축키로 사제 선택 SID15000, 원래 채팅 입력/Backspace/로컬 제출, 가운데 버튼 누름 뒤 blur와 재누름/뗌, 앱 반납 후 다섯 도구·주소 해제를 확인했다. 공격·포획·희생·승리는 수행하지 않았다.

녹화 때 음악은 끄고 게임 효과음은 켜 두었다. 실제 내려받기 파일은143,908바이트다. ffprobe의 독립 판독도1024×768 VP8 영상·48kHz 스테레오 Opus를 확인했다([트랙 판독값](webpj-e1372-evidence/media-info.json)). 브라우저에서 이 파일의 소리를 다시 해독해 2채널·72,000샘플, RMS 0.04691185429306312, 최대 절댓값 0.9938000440597534를 확인했다. 효과음이 실제 파일에 들어 있다는 근거이며 사람의 청취·원본과의 완전한 음질 동일성 판정은 아니다. [화면](webpj-e1372-evidence/webmcp-game-canvas.png) · [실제 영상](webpj-e1372-evidence/native-effects.webm) · [관측값](webpj-e1372-evidence/webmcp-game-result.json) · [브라우저 로그](webpj-e1372-evidence/browser.log)

도구 수명·부분 실패 Node 3/3, 소리 출력 관련 13/13, 외부 접속/링크 경계 11/11 통과했다. 최초 전체 회귀 중 별도 앵커 생성을 정적 감사가 잡아 자체 실행을 중단했고, 기존 downloadBytes 경계로 옮긴 뒤 소스 감사를 그대로 통과했다. 감사 허용 범위를 넓히지 않았다. 이후 전체 Node26은6,860건 중6,857통과·3실패였다. 두 실패는 신규 웹 전용 모듈의 대응표 분류 누락으로 WEB_ONLY에 실제 사유를 추가했다. 하나는 효과음 검사가 음성 노드를 고정 번호2로 가리킨 문제여서 실제 재생 직전의 노드 위치를 기준으로 고쳤다. 수정 뒤 해당 소리·대응표 집중13/13통과했다. 제품의 음량 기대값이나 실제 PCM 조건은 바꾸지 않았다. 최종 번들은3,460,761바이트다. 전체 Node26/22 회귀 결과는 완료 뒤 아래에 기록한다.

재검사는 webpj에서 npx playwright test e2e/webmcp.spec.js --workers=1로 실행한다. 두 번째 경우에만 테스트가 실제 브라우저 WebMCP 기능 옵션을 사용한다. 미지원 브라우저에서는 도구가 등록되지 않는 것이 정상이다.

## 표준 근거

2026-11-26 확인한 [WebMCP 편집 초안](https://webmachinelearning.github.io/webmcp/)과 [Chrome agentic JavaScript 도구 안내](https://developer.chrome.com/docs/modern-web-guidance/guides/webmcp/agentic-javascript-tools)를 따른다. 이 제품은 document.modelContext.registerTool과 등록 옵션의 AbortSignal을 사용한다. 구 navigator.modelContext/unregisterTool 예시를 섞지 않는다. 에이전트 클라이언트의 도구 발견/실행 허용은 해당 브라우저 정책에 따른다.

최종 전체 Node26.13.0:6,860/6,860통과, 실패·취소·미실행0,1,102.274초. Node22.23.5도v26종료 뒤 순차 실행해6,860/6,860통과, 실패·취소·미실행0, 1092.815초다. 전체 원문 로그의 SHA-256과 실제 번들·PCM은 [완료 검증값](webpj-e1372-evidence/verification.json)에 기록했다.
