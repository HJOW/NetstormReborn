# webpj Totalmade(허용·집계 표)와 Early1 단계 판정기(RawTotalMade·RawTutorialOne) 이식 — WB04-E237

2026-11-10, HJOW-Athlon. 같은 fixture(`tutorial-one-x86.tsv`)에 묶인 두 몸체를 함께 옮겼다. ① 원본 Totalmade 의 허용 표 조작(`AllowAll`·`Allow`·`Allowed`)과 누적 통계 조회(`Local`·`Built`·`Global`·`BuiltForGenus`)와 집계 표 reset(004c23c0~004c2560), ② Early Mission 1(튜토리얼 1)의 단계 판정기(PATCH 004c3a20: 다리 제작 수·F4 명령·신전/간헐천 Graph·현재 SP 로 공유 미션 게이트의 `Advance` 를 부르는 switch). ②는 E231 `RawMissionFrameGate` 의 `advance` 와 `RawTotalMade.builtForGenus(4)` 를 쓴다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E236 에 이어 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawTotalMade.cpp/.h`·`RawTutorialOne.cpp/.h`)·시험(`TutorialOneTests`)·fixture(`tutorial-one-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: `RawTotalMade` `0365fa99`(2026-10-25), `RawTutorialOne`·fixture `c3a294ed`(2026-10-15).
- 원본 fixture: `tutorial-one-x86.tsv` 에서 10.78(`originals`) `Total` 52행과 `Stage` 266행(CD 10.72 `Total` 52행은 웹 범위 밖). `Total` 은 여덟 연산(허용 전체·허용·허용 조회·reset·로컬·누적·전역·부류별 합) × 입력의 반환값·진단 횟수·네 표(로컬/누적/전역/허용) 1024 BYTE 전체, `Stage` 는 단계 문자·이벤트·제작 수·신전/간헐천 Graph 목록·SP 비트(단정도)·전투 표시에서 반환값과 호출 순서(`B` 제작 수 조회·`G<번호>` Graph·`M<플레이어>` SP·`A` Advance·`S` 간헐천 생성)다.
- 공유 장치: `squid-postpop-state.js`(`SquidPostPopState`·`RawTotalMadeStorage`·`SquidPostPopList`), E231 상태 이름(`stage`).

## 1. 옮긴 것

`www/core/raw-total-made.js`(DOM 없음, 10.78 전용)

- `resetTotalMadeCounts(storage)`·`setTotalMadeAllowAll(restrictions, allowed)`·`setTotalMadeAllow(restrictions, type, allowed)`: 세션 없는 독립 helper. 허용값은 금지 DWORD 의 0/1 로 뒤집어 쓰고(허용 → 0), 첫 255칸만 쓰며 256번째 칸은 보존하고, 허용 표 쓰기는 물리 범위 밖만 쓰기 전에 거부한다.
- `RawTotalMade(types, counts, restrictions, assertion)`: 188칸 TYPE 표와 255칸 이상의 허용 표가 있어야 생성(아니면 TypeError). `allowAll`·`allow`·`allowed`(유효 타입 1~187 의 금지 DWORD 가 0)·`resetCounts`(세 집계 표 첫 255칸을 로컬 → 누적 → 전역 순서로 0)·`local`·`built`·`global`(해당 표의 DWORD 비트, 유효하지 않은 타입은 진단 뒤 0)·`builtForGenus(genus)`(타입 70 부터 genus 가 한 비트라도 겹치는 타입의 누적 집계를 DWORD 감김으로 합산한 부호 있는 결과, 빈 genus 는 진단 뒤 0). 진단 `assertion(TotalMadeAssertion)` 이 없으면 진단 경로에서 TypeError 로 거부한다.

`www/core/raw-tutorial-one.js`

- `RawTutorialOne(state, players, geysers, hooks)`: 9×0xb4 Player 표와 효과 5개(`builtForGenus`·`graphFor`·`money`·`advance`·`spawnGeysers`)가 있어야 생성. `handle(event)`: 명령 = `event & 0x03ffffff`; 단계 'A' F4 명령이거나 다리 제작 수 > 0 이면 Advance 후 reclaim 명령이면 1 반환, 'B' 제작 수 > 7, 'C' 제작 수 > 18 이면 Advance 뒤 간헐천 생성, 'D' 현재 플레이어 신전 Graph 를 한 번 구한 뒤 최근 간헐천 목록(현재 signed count 만큼, 매 조건에서 다시 읽음)에서 Graph 가 같은 항목마다 Advance, 'E'/'F' 전투 중이고 SP ≥ 200/600 이면 Advance. 상태·목록은 복제하지 않는다.

## 2. 검증

`tests/tutorial-one.test.mjs`(5건)

- 원본 관측 재생 3건: `Total` 52행(여덟 연산, 진단 횟수 포함, 네 표 전체), reset 행을 독립 helper 와 공유 장부 두 경로에 각각 재생(같은 물리 배열 유지·허용 표 불변·제어/목록/미사용 독립 값 불변), `Stage` 266행(단락 평가·반환·다중 Graph·SP 포함 경계·NaN 의 호출 순서).
- cpppj 계약 포팅 1건 + 웹 계약 1건: 공유 상태/가변 목록에서 callback 뒤 현재 count·허용 표·장부를 읽음; 허용 표 쓰기 범위(첫 255칸·256번째 보존·음수/표 밖 거부)·잘못된 타입 진단(−1·0·188)과 빈 genus 진단·진단 장치 누락 거부·판본/크기 거부, 합이 2^31 이상이면 음수 반환, 부류 합산이 70 미만 타입과 겹치지 않는 부류를 더하지 않음, 'D' 의 음수(부호 있는) count 는 순회하지 않고 물리 길이를 넘는 count 는 범위 오류, Player 표 크기·필수 효과 누락·현재 Player 범위 거부.

최초 실패와 교정: 모듈은 처음부터 52+266행이 모두 통과했다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 변형 56건(Totalmade 28·Early1 28) 중 7건이 살아남아 3건은 웹 계약을 더해 잡았다(반환 부호, 음수 count, 물리 길이 초과 count).

- 실패한 변형: 초기화 칸 수·첫 타입 70·타입 수, 세 표 reset 각각, 허용값 뒤집기, 크기/범위/음수 검사, 유효 타입 경계(1~187), 진단 종류, 허용 조회 조건, 세 집계 표 대응, 빈 genus 조건/진단, 겹침 판정, 명령 마스크·다리 부류·Player 폭·신전 위치·SP 기준·단계 문자, 'A' 의 `||`/순서/비교/reclaim 반환, 'B'·'C' 임계값과 생성 순서, 'D' 의 Player 범위·Graph 한 번 구하기·count 재조회·Graph 비교, 'E'/'F' 의 전투 표시·비교, 생성 시 크기/효과 검사(각 1건 모두 실패).
- 적용되지만 통과한 변형(동등 변형): ① 진단 장치 존재 검사 제거 2건 — 함수가 아니면 호출 자체가 TypeError 다. ② 부류 합산에서 누산 중 `>>> 0` 제거 — 마지막 `| 0` 이 같은 감김을 한다. ③ 부류 마스크의 `>>> 0` 대신 `| 0` — 비트 AND 와 0 판정이 같은 결과다.

## 3. 경계와 남은 것

- 실제 미션/세션에 연결하지 않았다. 단계 전역을 미션 게이트(E231 `RawMissionFrameGate` 상태의 `stage`)와 같은 저장소로 합치는 일, 실제 제작 수 조회·Graph 조회(`RawGraph`)·SP 조회(0040ec50)·간헐천 생성(00486360)과 호출 지점(튜토리얼 1 미션의 프레임)은 순서4·6 몫이다(PORT_MAP session 0). 같은 fixture 를 쓰는 다른 단계(튜토리얼 2~6)는 `RawTutorialTwo`·`RawTutorialLater` 에서 다룬다.
- CD 10.72 `Total` 52행은 웹 범위 밖이다.

## 4. 다음 시작점

WB04-E238 은 3.1절에서 고른다. 기준선 뒤 변경이 없는 단위(`RawTerrainGrowth`·`RawTutorialTwo`·`RawTutorialLater`·`RawColorMapCache`)를 권한다.
