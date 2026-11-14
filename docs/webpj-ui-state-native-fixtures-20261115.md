# webpj UI 현재 상태 원본 관측 연결 (WB04-E482, 2026-11-15)

3.8절 이월 목록 중 사용자 강조(`user-highlight` 178행)와 현재 TYPE HP getter(`rifttype-hit-points`의 10.78 **579행**)를 원본 10.78 독립 x86 관측에 대조했다. 두 묶음 모두 첫 실행에서 전체 일치했고 이식 결함은 나오지 않았다. `PORT_MAP`과 모듈·시험 머리의 "전용 PE fixture 없음/cpppj에도 직접 시험이 없다"는 오기였다.

| fixture | 행 | 대상 | 비교한 것 |
| --- | --- | --- | --- |
| `user-highlight` | 178 | `RawUserHighlight.resetMouse/highlight` | 강조 모드·선택 SID·마우스 TYPE·좌표(전체 DWORD)와 커서(K)/역투영(P)/이동(M) 경계의 인자·순서, 경계 도달 때만 적용되는 변경(모드 7·선택 202·TYPE 77·좌표 -33/65) |
| `rifttype-hit-points`(10.78 579행) | 579 | `makeCurrentTypeHitPoints` | 선택한 물리 TYPE 의 HP 플래그/진단(`checking`·다이스 번호)·사제 1/4 HP·진단 복귀 뒤의 현재 표 이동·전역 변경·반환값·진단 문자열 |

- `rifttype-hit-points`는 기존에 `squid-reward.test.mjs`가 같은 579행으로 `readRiftHitPoints`를 대조하고 있었다. 이 단위는 같은 fixture 에 `CurrentTypeHitPoints`(현재 표 조회·번호 범위·처음 선택 TYPE 보존)를 두 번째 소비자로 연결한 것이며 manifest 항목에 소비자·모듈·cpppj 소스를 더했다(경로는 하나의 항목에 그대로).
- 초기 번호가 음수인 getNum 행은 직접 getter 검사의 몫이라 cpppj 와 같이 건너뛴다. CD(10.72) 행은 웹 범위 밖이라 제외한다.
- `priest-regen-hit-points-current`는 이미 `small-parents.test.mjs`가 10.78 432행 전부를 대조하고 있어(WB04-E15) 3.8절 목록에서 "이미 연결"로 정리했다.

## 변형 확인

- `RawUserHighlight` 자동 변형 21개와 수동 7개: 생존 1개는 `mode` 쓰기와 `resetMouse()`의 순서를 바꾼 변형으로 두 필드 사이에 관측 지점이 없는 동등 변형이다.
- `CurrentTypeHitPoints` 자동 변형 3개 중 `number >= length`를 `>`로 바꾼 1개가 처음 생존했다(경계 번호 188 은 fixture 에 없다). 번호 범위(188 이상·음수는 부호 없는 큰 수)와 표 조회 미연결을 기준선 소스(`CurrentTypeHitPoints.cpp`)의 계약으로 독립 확인하는 시험을 더해 닫았고, 수동 의미 변형 4개(범위·부호 없는 변환·현재 표 대신 선택 표 사용·조회 미연결을 0 으로)도 모두 잡혔다.

## 남은 것

`rifttype-hit-points`의 CD(10.72) 행은 웹 범위 밖이다. 3.8절의 `text-entry-gump`(2,292행)과 채팅창 나머지(`chat-window*` 등), HUD/minimap/menugump 계열은 후속 단위다.
