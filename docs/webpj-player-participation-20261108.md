# webpj Player 참여(로그인·동맹·AI 부착/해제) 이식 — WB04-E5

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 다섯 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawPlayerParticipation`(10.78 PATCH 전용)을 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-player-participation.js` — Player 9×180 BYTE 기록, 방향별 9×9 DWORD 동맹표, 숨김 9개를 **복사하지 않고 빌려** 원문 효과 순서대로 쓴다.

- **`checkLogin`(004a6dc0)**: 유효 owner 1..8의 flags(+0x50) 값 1 비트 부재에서만 `flags=(flags&~0x100)|3`을 쓰고 연결을 수락한다(로컬 전송에서 pending==owner이면 연결 지정을 하지 않음). 이미 로그인했거나 무효 owner이면 거부 응답→연결 해제를 순서대로 수행한다.
- **`setAlliance`(00490ea0)**: 자기 bit를 포함한 mask를 +0x54에 쓰고 현재 한 행 9칸만 갱신한다. `playerKnown`이면 채팅 채널 0x40/0x80을 지우고 양방향 동맹이 모두 참인 Player를 0x40, 숨김이면 0x80에 다시 등록한다(매 Player마다 로컬 번호·표·이름을 현재 값으로 다시 읽음).
- **`addAlly`(0048f6a0)**: 지정 방향의 한 bit/한 cell만 켜고, battle이면 현재 mask를 서버(9)에 요청한다. 반대 방향은 건드리지 않는다.
- **`receiveAlliance`(00491220)**: 수신 packet의 owner·mask에 자기 bit를 더해 packet에 되쓰고 → Player mask → 방향표/채팅 → `broadcast(packet)` 순서로 한다. broadcast는 **같은 packet 객체**의 변경된 바이트를 읽는다.
- **`attachBrain` / `removeBrain` / `removeAllBrains`**: 실제 생성 소유자가 넘긴 32비트 brain 토큰을 +0xb0에 기록하고 flags 0x20을 켠다(기존 brain은 assertion만 보고 덮어씀, 구독자가 없으면 owner+10000을 정하고 +0x30..0x4d 30 BYTE를 지움). 제거는 파괴→반납→+0xb0=0이며 **flags 0x20은 원본대로 지우지 않는다**. 파괴/반납 callback 안에서는 이전 토큰이 계속 조회된다. 가짜 brain을 만들지 않는다(`RawAiCreate`는 별도 단위).
- **State 반납 증빙 `BrainResetPass`**: cleanup의 owner 1..8 위치에서 `removeBrainForState`가 정상 복귀한 뒤에만 발급하고, `returnBrainForState`가 한 번 소비하면 그 owner의 새 설치·반복 제거를 닫는다(`brainReturnedForState`). `removeAllBrains`는 닫힌 NULL을 허용하되 각 owner 방문을 생략하지 않는다.

### C++ 이동 전용 수명 증빙의 JS 대응

C++의 `BrainResetPass`는 복사 불가·이동 가능 객체이며 발급 모듈의 주소/수명에 묶인다. JS에는 소멸·주소 재사용이 없으므로 같은 성질을 다음으로 옮겼다.

| 성질 | C++ | JS |
| --- | --- | --- |
| 외부에서 만들 수 없음 | 비공개 생성자 | 비공개 `#lifetime` 토큰을 알아야만 통과(생성자는 열려 있어도 위조 증빙은 거부) |
| 발급한 모듈에서만 소비 | 모듈 주소·대여 비교 | `data.source === this` 와 `data.lifetime === this.#lifetime` |
| 한 번만 소비 | 이동 후 빈 객체 | 비공개 `#spent` 플래그 |
| 소멸한 모듈의 증빙 거부 | 같은 주소의 새 모듈도 거부 | 새 모듈은 새 토큰이라 거부 |
| 파괴/반납 효과 진행 중 소비 금지 | 대여 중 호출 수 | `#activeBrainCalls`(finally로 복귀 시 감소) |
| 실패한 효과에는 발급 없음 | 예외 전파 | 예외 전파, 현재 brain은 그대로 남김 |

## 2. 검증 (`tests/player-participation.test.mjs`, 10건)

| 검사 | 근거 | cpppj 대응 |
| --- | --- | --- |
| 전체 기록 1620 BYTE·방향표 324 BYTE·효과 순서 608행 (Login 210·Alliance 122·Add 72·Attach 192·Remove 6·RemoveAll 6) | `player-participation-x86.tsv`(판본 열 없는 10.78 전용 표) | PlayerParticipation_Native608 |
| 수신 packet 288행(보호 41 BYTE 저장소 안 9 BYTE, 두 정밀도 열 모두) | `player-alliance-packet-x86.tsv` | PlayerParticipation_AlliancePacketNative288 |
| brain 토큰 수명·방향별 동맹 독립성 | (독립 입력) | BrainLifetimeAndDirectionalRows |
| 증빙이 owner1..8을 닫음·효과 실패 시 발급 없음·타 모듈/재소비/위조/소멸 증빙 거부·진행 중 소비 거부·raw 변조 거부·`removeAllBrains` 방문 순서 | (독립 입력) | StateReset* 5건 + Guide RemoveAll |
| 저장소 크기·현재 입력 다섯 조회 누락·효과 장치 누락 거부 | (독립 입력) | — (JS 호스트 경계) |

최초 실패와 교정: 구현 직후 608+288행과 계약 검사가 처음부터 통과했다. 원본 대조가 가능한 항목은 모두 원문 기대값을 그대로 썼고 모듈 출력을 기대값으로 삼지 않았다.

## 3. 경계

- cpppj `StateResetRetiredOwnerRejectsCreateAndRawIdentityChange`의 **`RawAiCreate` 연동 부분**(닫힌 owner에서 색/신전/할당 전에 생성을 거부)은 `RawAiCreate` 단위를 옮길 때 대조한다. raw 외부 변조 거부는 이 단위에서 검사했다.
- 실제 세션의 현재 전역(`localOwner`·`playerKnown`·battle·metamap·localTransport)은 `bindCurrentInputs`로 호스트가 공급한다(World/Session 조립은 WB05). 그전까지는 값형 `PlayerParticipationState`로 독립 대조한다.
- 외부 효과 장치(채팅·연결·서버 요청·brain 파괴/반납)는 `hooks`에 주입한다. 장치가 없으면 해당 위치에서 오류이며 이미 쓴 필드는 되돌리지 않는다.
- CD 10.72 배치와 멀티플레이 연결(`assignConnection`/`sendAlliance`의 실제 전송)은 범위 밖이다.

## 4. 다음 시작점

Player 묶음의 나머지 `RawPlayerTechnology`(`RawPlayerWorld` 선행)·`RawPlayerInitialization`·`RawPlayerColor.h`(RawBridgeConnect 선행)·알림 계열, 그리고 `SquidPostPop` 처리기 본체·종류별 PostPop·`SquidDisplay`·`SquidFrame`.
