# webpj 공통 경로 Halt 부모(RawPathHaltParent) 이식 — WB04-E144

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백마흔네 번째 부분이다. 모든 경로 프로세스(PathProcess)가 공유하는 Halt 부모(Pathprocess.cpp `00489ff0`) — 경로 가상 Halt 호출, 자기 삭제 뒤의 진단 복귀, 강제 정지 판정, 운반 계열 부모의 재지정 — 를 옮긴 `o/RawPathHaltParent`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. 선행은 E98 `RawPathRuntime`(경로 이미지)과 E103 `RawPathRetarget`(재지정 몸체)이다.

## 1. 옮긴 것

`www/core/raw-path-halt-parent.js`

- `runPathHaltParent(checking, hooks)` — `checking` 은 현재 DWORD 를 돌려주는 함수 또는 숫자(호출 도중에도 현재 값을 읽음). 최초 부모를 저장하고 감시 토큰을 건 뒤(`armWatch`) 가상 Halt 결과(10 계속/11 강제 정지/12 이미 죽음)를 얻는다. 경로가 이미 죽었으면(`alive` 거짓) checking 이 비영이고 결과가 12 가 아닐 때 원문 진단 `result == haltALREADY_DEAD`(`\Ns\O\Pathprocess.cpp` 0x2d5)를 낸 뒤 참을 반환한다(삭제 뒤에는 객체를 읽지 않음). 살아 있으면 토큰을 풀고 결과 10 이면서 저장 부모의 현재 flags 비트(0x20)가 강제 정지이면 결과를 11 로 바꾼다. 결과 11 이면 `mayKill`(현재 boss 또는 Form 의 로컬 범위)일 때 `kill` 하고 참을 반환한다. 아니면 현재 권한(`boss`)이고 저장 부모의 현재 TYPE genus 가 운반 계열(0x30000)이고 목표(TargetB 또는 TargetA)가 있으면 토큰을 걸고 `retarget`(자기 삭제할 수 있음) 뒤 경로가 죽었으면 참, 아니면 토큰을 풀고 계속한다. 마지막 반환은 `결과 !== 10` 이다.
- 선택된 위치의 경계가 없으면 이미 일어난 효과를 되돌리지 않고 거부한다. `armWatch`/`clearWatch` 는 독립 수명 관찰만 쓰는 호출자가 생략할 수 있다.

## 2. 검증

`tests/path-halt-parent.test.mjs`(2건):

1. `path-halt-parent-hardware-x86.tsv` 2288행(실제 CPU 관측) — 가상 Halt 결과(10/11/12)·checking·boss·Form 번호·부모 flags·TYPE genus·두 목표 윤곽·재지정 `find` 응답·후보 선택·부분 효과 입력(action 비트 0~9: 경로 소멸·checking 변화·boss 토글·flags 토글·TYPE 변경·부모 번호 변경·진단 중 상태 변경)에서 정지 여부·경로 소멸·checking·boss·부모 번호 필드·Form·두 목표·부모 TYPE/flags·`find` 횟수·호출 순서(H/F/C·Y/R·G/K/A)가 모두 같다. 재지정은 E103 실제 `RawPathRetarget`, 경로 이미지는 E98 실제 `RawPathRuntime` 이다.
2. 삭제 뒤 현재 진단/수명 관찰만 사용: 진단 장치가 필요한데 없으면 거부, 진단이 `checking` 을 끄면 그대로 계속하고(0x2d5 식·파일 확인), 진단이 중단하면 전파하며, 결과 12 는 진단 없이 반환하고 숫자 checking 도 허용한다.

최초 실패와 교정: 새 모듈은 2288행 fixture 를 시험 쪽 `RawPathRetarget` 너겟 타입 인자(함수형 getter)만 고쳐 통과했다.

## 3. 경계와 남은 것

- 이 몸체를 부르는 경로 프로세스 본체(`RawPathProcess`)와 실제 Host 삭제(`kill`)는 경로 프로세스 계열 단위에서 잇는다. 같은 파일군의 `RawPathArrivalParent`(공통 도착)·`RawPriestArrival` 등은 별도 단위다.
- 10.72(CD) 경로는 범위 밖이다.

## 4. 다음 시작점

`RawPathArrivalParent`(공통 도착), `RawPacketDispatch`→`RawConstructionCommand`, Canon/사제 배치 계열, `RawBurningSources`(연소점 선택)와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
