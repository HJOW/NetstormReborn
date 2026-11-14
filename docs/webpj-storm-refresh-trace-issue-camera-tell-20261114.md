# webpj SP 표시 재부착·trace Chat·초기 점 명령·카메라 pan·내장 Tell 소형 5단위 (WB04-E437, 2026-11-14)

RawMenuControls 선행 **RawStormPowerRefresh 42줄·TraceChat 45줄·RawIssueCommands 49줄·RawCameraPan 50줄·EmbeddedTell 53줄(.cpp/.h, todo→port, 합계 239줄)**을 새 모듈 [`www/ui/storm-power-refresh.js`](../webpj/www/ui/storm-power-refresh.js)·[`www/ui/trace-chat.js`](../webpj/www/ui/trace-chat.js)·[`www/ui/issue-commands.js`](../webpj/www/ui/issue-commands.js)·[`www/core/raw-camera-pan.js`](../webpj/www/core/raw-camera-pan.js)·[`www/ui/embedded-tell.js`](../webpj/www/ui/embedded-tell.js)로 이식했다. 기준선 `1f00a25a` 전체 소스를 읽었다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약·손 계산 값·실제 인프라(GumpCreate 등록, 공개 tell 자료의 GameData/Config)로 한 것이며 원본 직접 관측 수치가 아니다.

## 기준선 뒤 변경 (3.6절)

다섯 단위 모두 기준선 뒤 변경이 없다.

## 구현

- **`findCurrentGumpSerial`/`refreshStormPowerGump`(원문 00464020/0043dad0)**: 등록표 1..999의 현재 슬롯만 앞에서부터 본다(루트 0칸 생략, 빈 칸은 노드 조회 없이 건너뜀). 노드의 serial을 DWORD로 비교하고 dead 비트(0x10000)가 켜진 노드는 건너뛰어 첫 토큰을 돌려준다. 1000칸 미만 표는 TypeError, 조회 장치 누락은 Error, 사라진 노드는 RangeError다. 재부착은 못 찾으면 아무 장치도 부르지 않고, 찾으면 깊이를 보관 → Push → **Push 뒤의 현재 x/y**와 0/0으로 Attach → 두 가상 호출이 정상 복귀한 뒤에만 보관한 깊이를 되돌린다(중간 예외/장치 누락에서는 복원하지 않는다). 금액/SP/Idle/문자열은 다시 계산하지 않는다.
- **`forwardTraceToChat`(원문 434f40→42f190)**: enabled가 0이면 아무 장치도 읽지 않는다. 비영이면 현재 색을 읽어 8DWORD 문맥 `[0,1,색,0,0,0,1,1]`을 먼저 고정하고, 첫 NUL까지의 서식을 처리한 뒤 현재 Chat 포인터를 읽어 0이 아닐 때만 append한다(root 0이어도 서식 오류는 그대로 난다). 서식은 인자 없는 일반 문자와 `%%`만 지원하고, 그 밖의 `%`(끝의 `%` 포함)는 원문이 정의하지 않은 가변인자 읽기이므로 명시 거부(Error)한다. 출력 한도는 1023자(RangeError)다.
- **`registerIssueCommands`/`invokeIssueCommand`(원문 004d53a0, 콜백 004d5300/004d5340/004d5360)**: 등록은 `flash`·`flash`(중복 유지)·`desc`(표시 1)·`cheatorama`(표시 1) 순서의 네 호출이다. flash는 인자가 있으면 atol, 없으면 0으로 flash한 뒤 `thunderQuietDistant.wav`를 요청하고, desc는 `description(-1, 인자|없음, 0)`만 요청하며(빈 문자열은 "있음"), cheatorama는 인자가 없으면 TypeError, 기본 구분자의 **첫 토큰**을 atol(32비트 감김)한 값이 정확히 `0x845fed`(십진 8675309)일 때만 공유 DWORD(`{ value }` 상자)에 저장하고 기존 값은 지우지 않는다. 알려지지 않은 콜백은 장치를 읽지 않고 `null`(nullopt)을 돌려준다. 치트 비밀번호를 Config에 저장하지 않는다.
- **`processCameraPan`(원문 4d6b54)**: 가운데 버튼 폴링 → 전역 읽기 → 이동 관문(창 밖이고 밖 이동이 꺼짐 / 포인터와 가운데 버튼이 모두 뗌 / 대화상자 비영)이면 팝업을 닫지 않고 false → 팝업 닫기 → 전역 다시 읽기 → 좌표 -1 또는 비활성이면 요청 없이 true → 두 축 계산 → Shift가 눌려 있으면(폴링 코드에 뗌 비트 없음) 각각 `/8` 이전 값의 절댓값이 큰 축만 남기고(같으면 X를 지움) → `request(x/8, y/8)`(0 방향 절삭)이다. 한 축은 `FTOL(클라이언트/폭 × (제한된 위치 − 중심))`의 하위 DWORD이며 중심 = 원점 + 폭/2, 제한된 위치 = `min(max(원점, 포인터), 원점+폭)`이다(덧셈/뺄셈은 signed 32비트로 감김, 음수 폭도 같은 CMP 순서, 폭 0은 ±무한대/NaN이라 FTOL이 0, `Abs(INT_MIN)`은 여전히 음수). 카메라 현재값은 직접 쓰지 않고 요청만 한다.
- **`showEmbeddedTell`/`showEmbeddedTellInConfig`(원문 004c8f60)**: 이름은 첫 NUL까지만 쓴다. 현재 Mission 본문 → (없으면) 매번 새로 읽는 tell 설정 → 둘 다 없으면 missing 경고 뒤 0이다. 본문이 있으면(빈 섹션도 포함) construct → setHtml → 본문 반납 → 현재 부모 serial을 다시 읽어 돌려준다. construct가 0이면 Error(원문 bad_alloc)다. 실제 경로는 `GameData` 같은 설정 인터페이스의 `registry`·`pathSpec('missionSpec','tell')`·`loadFile`로 임시 `Config('tell')`을 읽고 섹션을 찾은 뒤 **UI 생성 전에 등록부에서 내린다**(예외가 나도 내린다). 등록된 tell 캐시는 쓰지 않는다. 키는 원문 좁은 키를 `decodeOriginalText`로 해독해 mission/섹션 조회에 넘긴다(모든 글자가 0..255이면 바이트로 보고, 255를 넘는 글자가 있으면 이미 해독된 문자열로 보아 그대로 둔다 — 웹 적응).
- 웹 적응 표기: C++ `logic_error`→`Error`, `out_of_range`/`length_error`→`RangeError`, `invalid_argument`→`TypeError`, `optional`→`null`/`undefined` 없음 판정, `uint32_t&`→`{ value }` 상자, `std::span<const uint32_t>`→`Uint32Array`/배열, 10.72 CD 분기는 이식하지 않는다.

## 검사와 발견

새 시험 파일 5개, **31건 통과**: `storm-power-refresh.test.mjs`(5, 실제 `GumpCreate`로 등록한 노드·기록 장치), `trace-chat.test.mjs`(6), `issue-commands.test.mjs`(6), `raw-camera-pan.test.mjs`(7, 손 계산 값), `embedded-tell.test.mjs`(7, 공개 영어/한국어 tell 자료의 실제 `GameData`). 카메라 시험은 기본 전역(1024×768 클라이언트, 512×384 지도)에서 `2×(300−256)=88 → 11`, `2×(100−192)=−184 → −23`처럼 값을 손으로 계산해 적었고, `INT_MIN` 경로는 원점 `INT_MIN+1`·폭 −4·클라 8·위치 1073741823으로 `FTOL = 2147483648`(하위 DWORD `INT_MIN`)이 되게 구성해 X/Y 양쪽에서 확인했다.

**변형 확인:** 연산자/정수 상수 변형 130개(storm-power-refresh 17·trace-chat 25·issue-commands 24·raw-camera-pan 50·embedded-tell 14) 중 125개를 잡았다. 처음 생존 13개 중 8개는 보강으로 잡았다: dead 마스크의 인접 비트(비트 0 단독/0x1ffff 에서 0x10000 제외), 선두 NUL 서식/이름(`nul < 0` 경계), 좌표 −2(`| 1`로 −1과 섞임), Y 축 원점·폭·클라 값(Y 축 `INT_MIN` 구성). 남은 5개는 동등 변형이다: trace-chat `index >= source.length`→`>`(끝의 `%`에서 `undefined !== '%'`가 같은 예외를 낸다), `Abs(0)` 변형 2개(`0<=0`, `0<1`에서도 `sub(0,0)=0`), 지운 축 값 0→1(`1/8`도 0 방향 절삭으로 0).

## 남은 경계

- 이 단위들은 호스트 장치(Chat 창 append·색·현재 Chat 포인터, 명령 분배기·MP description, 입력 폴링/전역·팝업 Kill·카메라 목표 요청, HtmlBody 생성/설치/serial, 예산 Gump의 Push/Attach)를 훅으로 받는다. 실제 연결은 RawMenuControls 허브와 입력/ChatWindow/Html* 단위에서 한다. 앱은 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
