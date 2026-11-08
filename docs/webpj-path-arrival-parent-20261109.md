# webpj 공통 경로 도착 부모(RawPathArrivalParent) 이식 — WB04-E145

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백마흔다섯 번째 부분이다. 모든 경로 프로세스가 공유하는 도착 부모(Pathprocess.cpp `0048b340`) — Step 최종 좌표의 공간 갱신, 도착 막힘 판정, Halt, finished 표시, 부모 도착(+3c)의 반환 코드 분기와 삭제 뒤 안전한 정리 — 와 기반 프로세스 SID 범위 진단(`0041be80`)을 옮긴 `o/RawPathArrivalParent`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. E144 Halt 부모와 같은 경로 프로세스 계열이며, 선행은 E98 `RawPathRuntime`·E1 `RawSquidFinder` 이다.

## 1. 옮긴 것

`www/core/raw-path-arrival-parent.js`

- `pathArrivalBlocked(pool, hash, types, spots, blockingGenus, point)` — 도착점이 지도 칸(`+0.9999f` 편향 절삭)에서 spot bit3 를 가지면, 도착점 칸(편향 없는 절삭) 하나의 일반 finder 후보 중 현재 타입 genus 가 `blockingGenus`(숫자 또는 현재 값을 돌려주는 함수)와 겹치는 첫 후보에서 참. 비유한·지도 밖 좌표는 거부.
- `runPathBaseRobust(hooks)` — 0041be80. 항상 참이되 form 번호가 0 이 아니고 용량 이상이면 진단 `formSid == 0 || form.isValid()`(0x69) 뒤 용량을 다시 읽고, 부모 번호가 0 이 아니고 용량 이상이면 `attachToSid == 0 || s.isValid()`(0x6c)를 낸다.
- `runPathArrivalParent(checking, hooks)` — 저장 부모가 유효하고 살아 있지 않으면 진단 `walker.isValid() && !walker.isDead() && !walker.isFree()`(0x25c). 경로가 살아 있으면 Step → Unpop → Pop → (checking 이면 robust) → robust → 도착 막힘 판정 → Halt(참이면 막힘이 있을 때 정리하고 끝) → `finished=참` → (checking 이면 robust) → 저장 부모가 dead/free 이면 진단 0x286 → 감시 토큰이 있으면 진단 `!deadFlag`(0x289) → 감시 토큰 설정 후 부모 도착(`arrival`: 1000 종료/1001 이미 죽음/1002 대기) → 그 밖의 반환이면 진단 0x28d → 경로가 죽었으면(checking 이고 반환≠1001 이면 진단 0x290) 막힘 정리 → 토큰 해제·(checking 이면 robust)·살아 있으면 1002 는 `finished=거짓`, 그 밖은 `kill` → 막힘이면 정리. 각 효과 뒤 `alive` 를 다시 확인해 삭제 뒤 객체를 읽지 않는다. 선택된 위치의 경계가 없으면 앞선 부분 효과를 유지한 채 거부한다.

## 2. 검증

`tests/path-arrival-parent.test.mjs`(3건):

1. `path-arrival-parent-hardware-x86.tsv` 860행(실제 CPU 관측) — checking·저장 부모·Form 번호·논리 용량·부모 상태·Halt 응답·도착 반환·삭제 입력·막힘 윤곽(후보 체인 0~4)·변동(0~12: Step/Pop/Halt/도착/진단 복귀 중 checking·Form·상태·좌표·토큰·목표 변경)·감시 토큰·finished·도착 좌표에서 경로 소멸·checking·Form/부모 번호·용량·부모 상태·finished·토큰 존재·두 목표·부모 좌표/TYPE/소유자·robust 호출 수와 호출 순서(S/U/P/Q/T/R/K/C/A)가 모두 같다. 실제 일반 finder 의 호출 관측 항목(F/N)은 내부 관측 API 가 없어 비교에서 제외했다(cpppj 시험과 같은 처리).
2. 선택된 진단 장치만 요구: 도착 반환 1003 에서 진단 장치가 없으면 거부하되 앞선 Step/도착 효과는 남고, 진단이 복귀하면(`checking` 끄기) 경로가 kill 된다.
3. 지도 밖·비유한 도착점 거부, spot 도착 비트가 없으면 finder 없이 거짓, `runPathBaseRobust` 가 항상 참이고 진단 0x69·0x6c 를 순서대로 내며 진단 장치가 없으면 거부.

최초 실패와 교정: 새 모듈은 860행 fixture 와 나머지 검사를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 이 몸체를 부르는 경로 프로세스 본체와 사제의 중립 회복(`RawPriestArrival`)·운반체 도착(`RawCarrierArrival`)은 별도 단위다. `kill`/`clear` 의 실제 Host 삭제는 경로 프로세스 계열 단위에서 잇는다.
- 10.72(CD) 경로는 범위 밖이다.

## 4. 다음 시작점

`RawPriestArrival`·`RawCarrierArrival`(선행 `RawCanonPlacementTerrain` 등), `RawPacketDispatch`→`RawConstructionCommand`, Canon/사제 배치 계열, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
