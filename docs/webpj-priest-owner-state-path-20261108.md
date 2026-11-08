# webpj 사제 소유자·사제 상태·경로 도착 애니메이션(RawPriestOwner·RawPriestState·RawPathAnimation) 이식 — WB04-E62

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 예순두 번째 부분이다. 사제(Priest) 후처리 이식(다음 단위)의 선행 부품 세 개를 옮겼다. cpppj 기준선 `1f00a25a` 의 `o/RawPriestOwner`(원본 00491790 사제 소유자 재정의), `o/RawPriestState`(00492090·00427030 이동 불가 조회와 004942b0 HP setter), `o/RawPathAnimation`(0048bd90 도착 프레임 접두와 004ad680/004ad710 방향 조회)이다. 세 파일 모두 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(사제 소유자의 요청 0 CD 처리, HP signed WORD, CD 프레임 BYTE)는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-priest-owner.js` — `RawPriestOwner(pool, mode, state, hooks)`, `PriestOwnerState`, `makePriestOwnerDispatch`, `PRIEST_TYPE`·`PRIEST_VTABLE`

- `set(sid, player)`: 비전투이거나 로딩 중이면 요청 번호로 원래 소유자 WORD(+12)의 하위 7비트를 쓰고(요청 0 이면 진단 `newPlayerId != INVALID_PLAYER_ID`, `.\Priest.cpp`, 최초 전투/비전투에 따라 0x5b/0x68), 로딩이 끝난 전투에서는 기존 원래 소유자를 다시 쓴다. 바뀐 경우(요청 ≠ 0)는 무상태 알림을 한 번 부른다. 유효 소유자(≤ 8)이면 공통 소유자 지정(`base`)을 부르고, 로딩이 아니거나 전투이면 마지막에 로컬 표식 비트 7(요새 모드의 요새 플레이어 또는 전투의 전투 플레이어)을 현재 WORD 에 쓰고 알림을 한 번 더 부른다. 실제 가상 진입 뒤에는 TYPE/free 를 선행 거부하지 않고 가상 표(`0x50f210`)만 본다.
- `makePriestOwnerDispatch(priest, base)`: 사제만 재정의하고 나머지는 base 로 넘기는 분배기로, 섬/종유석 분배기(`makeOwnerDispatch`)와 합성할 수 있다.

`www/core/raw-priest-state.js` — `RawPriestState(pool, hp, types, spots)`, `PriestHitPointMode`

- `groundImmobile`(00492090): genus 0x200000 이 아니면 거짓, HP 가 최대 HP/2(0 방향 절삭) 미만이면 지면 좌표를 읽지 않고 참, 아니면 좌표에 0.9999 를 더해 절삭한 칸의 spot & 6 이 없거나 extra 0x20 이면 참. `immobile`(00427030)은 extra 0x20 이면 HP/좌표를 읽지 않고 참이다. 예약/풀 밖 SID, free/contained 비트, 자산이 아닌 TYPE 은 거부한다.
- `setHitPoints`(004942b0): 사제 TYPE/가상 표를 확인한 뒤 HP 를 쓰고, 절반 HP 전환이면 권한이 없을 때 `half(−1)` 로 되돌리고, 회복(저HP → 정상)은 소유자 허용(`PriestHitPointMode.isRecoveryAllowed`: 독립 배열 또는 현재 Player 9 기록 owner×0xb4+0x7c 의 DWORD 0 아님)이 없으면 이전 HP 로 되돌리며, 허용되면 `unpop` → (새 HP 가 저HP 면 좌표를 0.99999 배율로 정수 보정) → `repop` 한다. 필요한 소유자 칸과 공간 효과만 쓰기 전에 검사한다. 허용은 Unpop 전에 결정한다.
- `bindCurrentTypes` 는 실제 World 의 188칸 TYPE 표를 대여한다. 최대 HP 는 같은 풀의 `SquidReward.maxHitPoints` 를 쓴다.

`www/core/raw-path-animation.js` — `RawPathAnimation(pool, frames, hooks)`, `pathSide`, `pathDirection`, `pathRestFrame`

- `pathSide`/`pathDirection`: 프레임 코드의 side 를 signed char 로 읽고 방향은 side − 'A'(0..7 로 제한하지 않음). 배열 밖은 변경 전 거부한다. `pathRestFrame`: 현재 글자 구간의 첫 물리 프레임 + 1(글자는 'A'..'P' 만).
- `stop(sid)`: free/예약 SID, 타입/현재 프레임 오류는 가상 효과 전에 거부한 뒤 `unpop(0)` → 프레임 DWORD 쓰기 → `repop(0x40)`. 이미 정지 프레임이어도 두 호출을 반복한다. 경로 종료/프로세스 정리는 호출자의 몫이다.

## 2. 검증

- `tests/priest-owner.test.mjs`(5건): `priestowner-x86.tsv` 10.78 3528행(모드·로딩·요청·원래 단어·현재 소유자 → 알림/공통 지정 호출 순서와 슬롯 전체), `priest-owner-current-local-x86.tsv` 10.78 72행(진단·알림이 바꾼 현재 모드/WORD, 공통 부모 중단과 TYPE/free 변경 뒤 후단의 재조회). 계약: 필수 장치/외국 VT 거부와 요청 0 진단 위치의 부분 효과, 실제 사제 생성자·공통 소유자와 순차 로딩/전투 전환, 섬/종유석 분배기와의 합성.
- `tests/priest-state.test.mjs`(8건): `prieststate-x86.tsv` 10.78 6752행(Ground 2720·Wrapper 2720·Set 1312), HP setter 1312행을 값형 배열과 현재 Player 9 기록으로 다시 재생, 사용하지 않는 경로의 좌표/기록 폭 비검사, 소유자 8 까지의 전체 DWORD 읽기, 복사된 모드의 boss getter 재조회, Unpop 전 허용 결정.
- `tests/path-animation.test.mjs`(3건): `pathanimation-x86.tsv` 10.78 Read 608행·Stop 1408행(호출 기록과 슬롯 50바이트), 배열 밖 입력의 사전 거부, 실제 Factory/Pop/Unpop 으로 새 크기의 해시 단계 재등록과 반복 정지.

최초 실패와 교정: 세 모듈은 위 fixture 전부(소유자 3600 + 상태 6752 + 경로 2016행)를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 World 의 모드/로딩/요새·전투 플레이어, 알림(004214a0), 공통 소유자 분배기 등록과 Player 9 기록의 장기 연결은 WB05 에서 한다. 사제 PostPop 이 이 부품들을 조립한다(다음 단위).
- `RawPathAnimation` 의 보행 프로세스(경로 종료·타이머)는 PathProcess 이식 단위에서 한다.

## 4. 다음 시작점

`RawPriestShield`/`RawPriestForcefield`, `RawPriestPostPop`·`RawPriestPostPopTail`(이번 부품과 `RawCarrierPostPop` 연결), `RawAssetPostPop`, 후보/지형 권한(`RawCanonPlacementPermission`) 이다.
