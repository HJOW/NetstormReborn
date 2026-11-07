# webpj Player 식별자(PlayerFinder) 이식 — WB04-E4

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 네 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawPlayerIdentity`(원문 004841c0, 10.78)를 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-player-identity.js`:

- **레코드 60 BYTE**: +0 owner DWORD, +4 subscriber DWORD, +8 이름 40 BYTE(+NUL), +48 주소 DWORD, +52 모드 DWORD. `RawPlayerIdentity.initialize`(00484190)는 owner·subscriber·이름 첫 BYTE·주소·모드만 쓰고 이름 뒤 미작성 바이트와 마지막 DWORD를 보존한다.
- **`resolve` 4단계**(부분 완료 유지): ① 로컬 일치 — owner/subscriber/이름/주소 중 하나만 맞아도 현재 로컬 정보로 덮어씀 → ② battle/editor 모드면 Player 1..8의 첫 OR 일치(Player 0 제외) → ③ metamap 모드면 활성 기록 40개를 subscriber 또는 이름으로 비교(owner는 조건 아님, 기록 미연결이면 거부) → ④ 채팅 100개 첫 일치(활성 flag·subscriber 비영 필터 없음). 반환 true는 모든 필드의 완성을 뜻하지 않는다.
- **네 getter**: `subscriberForOwner`·`ownerForSubscriber`·`addressForSubscriber`·`nameForSubscriber`/`nameForOwner`. `resolve`의 bool을 별도 성공 관문으로 쓰지 않고 레코드 필드를 그대로 돌려주며, 이름 실패는 현재 `unknownName`(SOMEONE)이다.
- 문자열은 원본 바이트 그대로의 이진 문자열(문자 하나 = 1 BYTE)이며 비교는 C locale ASCII 대소문자만 접는다. 채팅의 40 BYTE 이름 조회는 인접 주소 첫 BYTE의 NUL까지 41 BYTE를 읽는다. 저장소(players 9×180·meta 40×460·chat 100×48)는 복사하지 않고 빌린다.

## 2. 검증 (`tests/player-identity.test.mjs`)

| 검사 | fixture(`player-identity-x86`, 10.78 851행 중) | cpppj 대응 |
| --- | --- | --- |
| 전체 60 BYTE 레코드 | ctor 4 + resolve 179 = 183행: 반환과 레코드 hex(이름 뒤·마지막 DWORD 표식 포함) | PlayerIdentity_NativeRecords366(10.78 분) |
| 네 getter | subscriber·owner·address·name 각 161 = 644행 | PlayerIdentity_NativeGetters1288(10.78 분) |
| 계약 | (독립 입력) 현재 원문 변경 읽기, metamap 기록 누락 거부, 부분 완료, 저장소 크기·NUL 부재 거부 | PlayerIdentity_CurrentStorageAndRequiredMeta |

최초 실패와 교정: 구현 직후 183+644행이 처음부터 통과했다(주석 오타만 수정).

## 3. 경계

- 객체 메뉴 DWORD 24행(`menu`)은 client `RawObjectMenu`(Player 메뉴 몸체)를 필요로 해 이 단위에서 제외했고, 그 단위를 옮길 때 `BindObjectMenuIdentity` 연결과 함께 대조한다.
- CD 10.72 배치(Player 172 BYTE·meta 296 BYTE)와 CD 851행은 범위 밖이다.
- 실제 로컬 전역(`localOwner`·`subscriber`·`address`·`name`·모드)은 호스트가 `reads`로 공급한다(World/Session 조립은 WB05).

## 4. 다음 시작점

Player 묶음의 나머지: `RawPlayerParticipation`(C++ 수명 증빙 `BrainResetPass`는 JS 대응 설계가 필요), `RawPlayerTechnology`, `RawPlayerInitialization`, `RawPlayerColor.h`(RawBridgeConnect 선행), 알림 계열. 그리고 WB04-E의 `SquidPostPop` 처리기 본체·`SquidDisplay`·`SquidFrame`.
