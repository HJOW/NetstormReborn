# webpj 애시드 포획 명령(RawAcidZap) 이식 — WB04-E48

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 마흔여덟 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawAcidZap`(10.78; 원본 40ad30 11바이트 포획 명령 작성과 48bfe0 로컬/원격 수신)을 옮겼다. 다음 단위의 애시드 울타리(`RawAcidFence`)가 이 작성/수신을 그대로 부른다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-acid-zap.js` — `RawAcidZap`

- `new RawAcidZap(pool, types, {altar, growingIceTower}, {message, protocol, length}, {sound, dissolve, destroy})`: 10.78 풀·188 타입과 세 훅이 모두 있어야 한다. 제단(164)/성장 중 아이스 타워(150) 전역과 등록 헤더는 호출마다 현재값을 읽는다.
  - `build(victim, first, second, orientation)` [40ad30]: `[protocol&0x7f, message, 선언 길이 WORD, 대상 WORD, 첫 폴 WORD, 둘째 폴 WORD, 방향 signed BYTE]` 11바이트. 선언 길이는 4 미만도 보정하지 않는다.
  - `receive(owner, flags, packet)` [48bfe0]: 송신 owner/flags와 둘째 폴·동맹·boss·editor는 읽지 않는다. 대상이 free/dead(상태 비트 1·2)이거나 첫 폴이 free이거나 대상 TYPE이 제단/성장 중 아이스 타워이거나 사제 genus(0x200000)이면 끝. 아니면 소리(대상 좌표/`dissolve.wav`/반복 0) → 분해 생성(소리 뒤 현재 패킷의 세 WORD와 방향) → 현재 가상 삭제(처음 읽은 대상, 플래그 = 첫 폴의 **현재** owner<<16 | 0x400048).

## 2. 검증 (`tests/acid-zap.test.mjs`, 3건)

독립 원본 관측 `acid-zap-x86.tsv` 1017행(판본 열이 없는 10.78 단일 관측): 수신 927행(free/dead·두 제외 TYPE·사제·입력 무시·소리/분해 콜백이 슬롯 owner/가상 표/패킷 WORD를 바꾸는 10종 변경 뒤의 효과 문자열과 6개 슬롯)과 헤더 90행(프로토콜 상위 비트·메시지·선언 길이·모든 페이로드의 11바이트 hex). 계약: 구성 오류/훅 누락/짧은 패킷 거부, 상위 비트 제거와 길이 무보정.

최초 실패와 교정: 모듈은 1017행 전부를 첫 실행에 통과했다.

## 3. 경계

- 실제 소리(`dissolve.wav`)·분해 Form 생성(450510)·가상 삭제는 월드 조립에서 공급한다. 패킷 헤더 등록 표(`RawFormPacket`)의 세션 소유 값은 호출자가 객체로 넘긴다.

## 4. 다음 시작점

`RawArcSpire`(+`arc-spire-x86` 665행)와 이를 쓰는 `RawAcidFence`(+`acid-fence-x86`), 그 뒤 Player 나머지·종류별 PostPop·`SquidDisplay`/`SquidFrame`이다.
