# webpj 튜토리얼 3~6 단계 판정기(RawTutorialLater) 이식 — WB04-E239

2026-11-10, HJOW-Athlon. Early Missions 3~6(Capture the Priest·Tactical Combat·Subtle Defense·Raw Power)의 현재 단계 조건(PATCH 004c3f00/004c40f0/004c42b0/004c45e0)을 옮겼다. 사제 Graph·배치 지역·내용물/포획/상승 사건·생산 목록·전역/로컬 제작 수·선택 상태·5초 지연으로 공유 미션 게이트의 `Advance` 를 부른다. E231 `RawMissionFrameGate`·E234 `RawMissionStart` 의 일회성 알림 필드·E237 `RawTotalMade` 를 실제로 써서 원본의 읽기 순서와 DWORD 감김을 보존한다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E238 에 이어 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawTutorialLater.cpp/.h`)·시험(`TutorialLaterTests`)·fixture(`tutorial-later-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스·fixture `400aaefb`(2026-10-15).
- 원본 fixture: `tutorial-later-x86.tsv` 533개 고유 입력(10.78 전용). 입력은 쉼표로 이은 30개 명시 값(미션 번호·단계·사건 DWORD 셋·생산 목록 비트·사제/Graph/지역/주변/포획 반환·제작 집계·선택·타이머(double 비트)·정지 깊이·안내 잠금·재진입 변이 5종 등), 출력은 게이트 14필드·정지 깊이·현재 TYPE 재조회와 외부 하위 호출 순서(`Y<주소>`·`F`·`G`·`C`·`I`·`R`·`N`·`H`·`S`·`Z`·`X`·`T<섹션>`·`K`·`Q`·`P`)·반환·옵션 8 BYTE·허용 표 값·선택·일회성 경고 표식 둘·현재 owner 다.
- 공유 장치: E231 `raw-mission-frame-gate.js`, E234 `raw-mission-start.js`(`createMissionStartState`), E237 `raw-total-made.js`, `squid-postpop-state.js`, `sid-pool.js`.

## 1. 옮긴 것

`www/core/raw-tutorial-later.js`(DOM 없음, 10.78 전용)

- `RawTutorialLater(pool, types, players, made, gate, mission, state, hooks)`: 10.78 풀·188칸 TYPE 표·9×180 Player 표·옵션 3 BYTE 이상·`state.type` 함수·효과 13개(`findPriest`·`graphFor`·`productionContains`·`indicateTechnology`·`regionFor`·`nearby`·`priestCaptured`·`selected`·`gameSeconds`·`resetSelection`·`clearSelection`·`tell`·`closeTransientGumps`)가 있어야 생성된다(아니면 TypeError).
- `handle(tutorial, event)`: 사건 DWORD 3칸이 있어야 하고 미션 번호 3~6 만 받으며 반환값은 항상 0. 명령 비교(`event[0] & 0x03ffffff`)에서만 modifier 상위 비트를 지우고 SID/type 인자는 보존한다. 현재 TYPE 번호는 원본 전역 주소(`state.type(주소)`)로 읽는 위치마다 다시 조회한다.
  - 3(Capture the Priest): A 사거리 표시 명령 → Advance(옵션 BYTE 2 = 2), B 적/자기 사제가 모두 있으면 Graph 가 같을 때만(없으면 바로) Advance, C/D 생산 목록(D 는 기술 알림 포함), E 배치 완료 명령에서 배치 객체의 TYPE·지면 지역(플레이어 지역과 같으면 불가)·주변 부류가 없을 때 Advance, F 적 사제가 없거나 포획이면 Advance, G 내용물 통지(부류 0x200000, 객체 +0x22 가 0).
  - 4(Tactical Combat): A 허용 표 쓰기와 생산 둘, B/D 전역 ≤ 로컬 제작 수(원본 cmp 동일/감긴 sub 부호), C 기술 알림·생산, E 내용물 통지와 세 번째 객체 TYPE, F 허용 쓰기와 로컬 제작 수 > 0, G/H 포획/상승 명령.
  - 5(Subtle Defense): switch 전에 일회성 경고 둘을 각각 현재 값으로 다시 판정한다: 전역 제작 수 < 1 이고 `+c0` 가 0 이면 `VortexDestroyed`, 단계가 B 를 넘고 생산 목록 넷 중 하나라도 없고 `+c4` 가 0 이면 `FactoryDestroyed`(Tell 이 열렸으면 임시 Gump 정리 뒤 이 미션의 정지, 표식은 창 결과와 무관하게 1). 이어 A 기술 사건, B 생산 넷, C 선택 5초 double 지연(첫 선택이 0 이면 두 번째 선택·raw 조회 생략)과 Advance 뒤 선택 해제, D/E/F.
  - 6(Raw Power): A 전역 제작 수 둘, B 사거리 명령, C 기술 알림·생산, D 전역 제작 수, E 세 전역 제작 수가 모두 0.
- Advance 가 거부돼도 선택 해제 두 효과(`resetSelection` → `clearSelection`)는 원본대로 실행한다.

## 2. 검증

`tests/tutorial-later.test.mjs`(3건)

- 원본 관측 재생: 533개 전부. 행마다 새 풀·Player 표·제작 집계(전역/로컬 표를 입력으로 채움)·허용 표·게이트·`+c0`/`+c4`·옵션 8 BYTE 를 입력대로 구성하고 게이트 14필드 비트·정지 깊이·하위 호출 순서·반환·옵션·허용·선택·일회성 경고·owner 를 대조한다. 변이 5종(적 사제 조회 뒤 owner 변경, 기술 알림 뒤 생산 목록 비움, 선택 조회 뒤 선택 변경, Tell 뒤 단계 변경, Gump 정리 뒤 정지 깊이/표식 변경)을 포함한다.
- cpppj 계약 포팅 1건: 필수 효과 13개 각각·옵션 크기·Player 표 크기·`type` 함수 누락 거부, 사건 DWORD 2개와 미션 번호 2 의 거부, 어느 경우에도 단계·옵션·효과가 바뀌지 않는다.
- 웹 계약 1건(원본 fixture 밖 경계): 명령 modifier 상위 비트(0x04000000)가 섞인 사거리 명령도 인정, 일회성 경고 둘이 처음 한 번만 나고 표식이 켜져 있으면 다시 나지 않음, 현재 Player 번호 9 의 지역 범위 거부, 사건 SID 의 TYPE 바이트가 표 밖(200)이면 부류 조회에서 RangeError.

최초 실패와 교정: 모듈과 533행 재생은 처음부터 통과했다. 시험 쪽 결함 하나를 고쳤다(웹 계약 장면의 게이트 안내 잠금이 기본 10 이라 Advance 가 예약되지 않았다. 잠금을 0 으로 두었다). 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 처음 변형 57건 중 7건이 살아남아 5건은 웹 계약을 더해 잡았고 2건은 동등 변형이다.

- 실패한 변형: 명령 마스크 외 모든 분기(허용 표 쓰기 값, 옵션 BYTE 2, 사거리/배치/내용물/기술/포획/상승 명령 대응, 적/자기 사제 조회 순서와 Graph 비교, 사제 `&&`/`||`, 생산 목록 타입 번호와 `||`/`&&`, 기술 알림 호출, 지역·TYPE·주변 부류 비교와 마스크, 포획 조건, 부류 비트, 전역 ≤ 로컬(동일/감긴 부호), 로컬 제작 수 임계, 일회성 경고 조건·표식·정지, 선택 조회 두 번·`||`/`&&`·5초 지연·비교·선택 해제, 세 전역 제작 수 조건), 생성 검사 둘, 사건/번호 검사, 선택 해제 호출 순서와 정지 연결(각 1건 모두 실패).
- 적용되지만 통과한 변형(동등 변형): ① 사건 SID 의 모듈 쪽 풀 범위 검사 제거 — `SidPool.slot` 이 같은 RangeError 를 던진다. ② 전역/로컬 제작 수를 읽는 순서 교환 — 두 조회가 같은 주소 번호를 읽어 trace 가 같은 이름(`Y<주소>`)을 두 번 남기므로 구별되지 않는다.

## 3. 경계와 남은 것

- 실제 미션/세션에 연결하지 않았다. 사제/Graph/지역/주변 부류/선택/생산 목록/Tell/Gump 등 실제 하위와 현재 TYPE 전역 주소 값, 명령 토큰, 옵션 표의 실제 소유자, 호출 지점(튜토리얼 3~6 프레임)은 순서4·6 몫이다(PORT_MAP session 0).
- 튜토리얼 1~2 는 E237·E238, 공통 시작은 E234, 프레임 문은 E231 이다.

## 4. 다음 시작점

WB04-E240 은 3.1절에서 고른다. 기준선 뒤 변경이 없는 단위(`RawTerrainGrowth`·`RawColorMapCache`·`RawContentPreviewProcess`)를 권한다.
