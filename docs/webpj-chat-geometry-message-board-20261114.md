# webpj Chat 창 기하·메시지 표 HudChatGeometry·MessageBoard (WB04-E445, 2026-11-14)

RawMenuControls 선행 **HudChatGeometry 135줄·MessageBoard 94줄(기준선 `1f00a25a`의 .cpp/.h 줄 수, 합계 229줄, todo→port)**을 새 모듈 [`www/ui/hud-chat-geometry.js`](../webpj/www/ui/hud-chat-geometry.js)·[`www/ui/message-board.js`](../webpj/www/ui/message-board.js)로 이식했다(같은 커밋에 [E444 묶음](webpj-bg-gif-ns-info-20261114.md)도 포함). 기준선 전체 소스를 읽었다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약과 손 계산 값으로 한 것이며 원본 직접 관측 수치가 아니다.

## 기준선 뒤 변경 (3.6절)

두 단위 모두 기준선 뒤 변경이 없다.

## 구현

- **`HudChatGeometry`(원문 0042e6e0 등)**: root(`HudChatNode`, 0x13c 바이트 ChatWindow)의 파생/기본 주소가 같은 객체인지 호출마다 대조하고, 자식(배경·본문 글·도우미·입력줄)은 각 가상 호출 뒤에도 root 에서 다시 읽는다. `attach`는 기본 부착 → 배경 Push·크기(입력줄이 보일 때만 본문 높이에서 입력줄 높이를 뺌, 폭은 root 폭−왼쪽 여백) → 도우미 높이(입력줄 표시와 원시 BYTE 두 관문을 모두 통과해야 15.0) → 배경 부착(여백,0) → 본문 줄 조회(004bc2c0) → 본문 Push·크기(배경 크기 복사)·맨 아래 요청·부착 → 도우미 Push·부착(여백, root 높이−도우미 높이) → 입력줄 Push·폭만 복사·부착(여백+17.0, root 높이−입력줄 높이) 순서다. 모든 크기는 float32 저장 전 배정도 중간값에 배율을 곱해 float32 로 저장한다(예: 폭 16777217×1.5 → 25165826). `setHelperVisible`은 0 이 아닌 값을 BYTE 1 로 쓰고 재부착은 하지 않는다. `setInputVisible`은 root·입력줄 Push 뒤 값이 비영이거나 현재 challenge 가 비영이면 표시(값 0 은 challenge 조회가 필요), 입력줄(현재 좌표)과 root 를 parent 0 으로 재부착한다. `toggleInputVisible`은 현재 Chat 의 입력줄만 Push 하고 Push 뒤 현재 root 를 다시 읽어 표시 비트를 뒤집고 같은 입력줄만 재부착한다. `inputVisible`은 현재 Chat 이 없으면 자식/노드를 조회하지 않고 0 이다. `dispatchChatInputKey`는 뗌·코드 마스크(0x03ffffff) 뒤가 13 이 아닌 입력을 먼저 거부하고(Ctrl/Shift/Alt/출처 비트는 막지 않음), 입력줄이 보이면 처리하지 않고, 아니면 표시→열기 후 true 다. `consumeChatInputToggle`은 `{ value }` 요청이 0 이 아니면 전환 뒤 0 을 쓰며 전환이 던지면 요청을 지우지 않는다.
- **`MessageBoard`(004d3090)**: 20칸 serial 표(첫 칸 0xffffffff 가 첫 호출 표식으로 모두 0 으로 채움)에서 호출 진입에 억제를 한 번 판정한다(종류 11/16 은 통과, `suppressed` 훅이 있으면 매 호출 현재 값). 칸마다 비어 있지 않으면 조회해 stale 이면 0 으로 쓰고, 종류·플레이어·인자가 모두 같고 종류가 17 이 아니면 검색을 멈춘다(원문대로 중복 방지가 아니라 앞에 빈 칸이 없을 때만 생성이 막힌다). `enabled=false` 는 일치 레코드를 삭제 요청하고 그 칸을 0 으로 쓰며, true 는 첫 빈 칸에 생성하고 serial 을 기록한 뒤 현재 폭을 조회해 x=(화면 폭−폭)의 uint32 감김(signed)·y=칸×12 에 부착한다. 빈 칸이 없으면 생성하지 않는다. 원문 NULL 문자열(`null`/`undefined`)은 `createText` 가 있을 때만 전달되고(빈 문자열은 NULL 과 구별), 없으면 Error 다. `warningDeadline`은 벽 시각+10 초(double), `warningExpired`는 기한 0(영구)이 아니고 벽 시각이 기한 이상이면 참(NaN 은 거짓)이다.
- 웹 적응 표기: C++ `invalid_argument`→`TypeError`, `runtime_error`/`logic_error`→`Error`, `std::int32_t&`→`{ value }` 상자, `std::optional<string_view>`→`null`/`undefined`+문자열.

## 검사와 발견

새 시험 파일 2개, **28건 통과**(`hud-chat-geometry.test.mjs` 15건·`message-board.test.mjs` 13건). 기하는 root 1·배경/본문/도우미/입력줄 4자식의 기록 틀로 호출 순서와 좌표를 손 계산 값으로 비교하고(입력줄 숨김·도우미 BYTE 0·Push 가 자식/여백을 바꾸는 재조회·큰 폭의 배정도 중간값 포함), 표는 20칸을 채우는 시나리오·stale 정리·중복 정지·Warning 17·억제·NULL 문자열·uint32 x 감김을 값으로 확인한다.

**처음 실패한 시험은 없다.** 전체 검사 중 두 시험(port-map)이 대응표에 없는 새 모듈 때문에 실패한 것은 같은 단위의 PORT_MAP 행을 아직 넣지 않았기 때문이며 행을 넣은 뒤 해소했다.

**변형 확인:** 연산자/상수 변형 118개와 의미 변형 24개(`^`/`~`/`&` 표시 비트·배율 곱 순서·표시 관문·마스크·부호·uint32 감김·첫 호출 초기화 등), 합계 142개 중 139개를 잡았다. 처음 생존한 항목은 값 1 의 표시 요청, 도우미 부착 y(높이 대신 기본 높이), uint32 x 감김, 첫 호출의 전체 0 채움, 기본 레코드 값을 시험해 잡았다. 남은 3개는 동등 변형이다: 도우미 표시 높이가 방금 쓴 기본 높이와 같은 값이라 대체해도 같은 결과(1), 표 검색/삭제 루프의 `<`↔`<=`(범위 밖 `undefined` 읽기와 Uint32Array 범위 밖 쓰기가 무시되어 같은 결과, 2).

## 남은 경계

- ChatWindow 본체(Console/TextEntry 생성·입력)·Chat 입력이 이 분배기를 부르는 연결·MessageGump 파생 Gump(기준선 뒤 6커밋 판정 대기)·메시지 표를 부르는 미션 쪽 연결은 별도 단위다. 앱은 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
