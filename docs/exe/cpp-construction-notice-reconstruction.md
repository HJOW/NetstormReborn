# 건설 배치 통지 처리기 복원

2026-10-10 착수 → 2026-10-11 완료, **HJOW-Athlon**. AGENTS.md와 두 LEFT_JOBS 문서를 다시 읽고 [로컬 예측 조각 정리](cpp-construction-clear-reconstruction.md)의 다음 작업을 진행했다. 시작 작업 트리는 깨끗했고 현재 PC와 마지막 디컴파일 PC가 같았다. 새 `constructionnotice` 목록 **6/5/5개**를 읽기 전용 Ghidra로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10.** 원본 게임·복사본·analyzeManager·클론 창을 실행하지 않았다.

## 복원 범위와 호출 순서

[`RawConstructionNotice`](../../cpppj/src/o/RawConstructionNotice.h)는 메시지 `0x3f` 처리기 **10.78 `004441b0` ↔ CD/10.37 `004d1d00`**의 전체 분기를 옮긴다. 디컴파일 대응은 `callseq/strong`, 신뢰도 A이며 원본 처리기의 발신자 인자 둘은 사용되지 않는다. 입력은 앞 단계의 `ConstructionNotice`를 그대로 받는다. wire 바이트를 해독하는 수신 계층은 아직 연결하지 않았다.

1. 통지의 타입·좌표·인자·방향·플레이어로 로컬 예측 조각을 정리한다.
2. 타입 `flags1 & 0x10` 또는 dais 타입 번호 일치이고 통지 `flags & 1`이 꺼져 있으면 `abstract=1`, 그 밖에는 0이다. 건설 사제 필요 여부는 `flags1 & 0x8000`과 같은 통지 비트로 판정한다. genus의 dais 비트만으로 abstract를 결정하지 않는다.
3. 타입 `group(+0x9c) != 10`이고 통지 플레이어 바이트가 로컬 플레이어 **DWORD 전체**와 같으면 최근 배치 좌표 다섯 개를 뒤로 밀고 새 좌표 비트를 첫 칸에 넣으며 카운터를 0으로 만든다. 이 갱신은 배치 전에 일어난다.
4. 통지 SID 목록 전체와 계산한 abstract·품질로 조각을 배치한다.
5. abstract일 때 아래 사제/건설 분기를 처리한다.
6. 정상 반환 직전에 갱신 경계를 한 번 부른다.

건설 사제가 필요한 타입은 통지 플레이어가 현재 로컬 플레이어와 다르거나 로컬 재이동 전역이 켜져 있으면 `00443000`/CD `004d1120`을 호출한다. 첫 SID와 double 도착 시각의 두 DWORD를 비트 그대로 전달한다. 권한 전역은 이 분기를 막지 않는다. 로컬 비교는 배치 뒤에 다시 읽으므로 배치 경계에서 바뀐 상태가 반영된다.

건설 사제가 필요 없는 abstract이고 권한이 있으면 **배치 뒤 첫 SID의 현재 타입**에서 genus를 읽는다. `0x404200` 비트가 모두 꺼져 있으면 `00443fe0`/CD `004d06a0`으로 건설을 시작한다(둘째 인자 0). 관련 비트가 있으면 소유 사제를 찾고, raw 상태의 **하위 두 비트 `&3`**가 0이고 사제 genus `0x200000`이며 이동 불가가 아닐 때만 접근점을 구한다. void 비트 4 자체는 처리기의 거부 조건이 아니다.

첫 접근은 객체 발자국에 플래그 **7**, 실패 뒤 재시도는 **3**을 준다. 두 시도 모두 현재 사제 좌표를 다시 읽는다. 접근점을 원본 `0.9999899864196777F`를 더하는 거의 올림으로 보정하고 이동 함수에 목적 객체 SID를 전달한다. 첫 성공이면 재시도하지 않는다. 두 번 실패하거나 사제 조건이 맞지 않으면 **첫 객체의 현재 타입·소유자**로 환불 → 첫 객체 삭제(플래그 0) → 갱신 순서다. 실패 시 전체 SID 목록을 삭제하는 함수가 아니다.

## 패치판에서만 하는 contained 삭제

10.78은 소유 사제 첫 조회 뒤 `00488fe0(priestType)`로 contained 타입을 얻고, 소유 사제를 **두 번째로 조회하여 그 SID**에 `004b1a90(SID, containedType, 0, 1)`을 부른다. 이 효과는 사제 raw 상태 검사보다 앞에 있다. CD/10.37 처리기에는 없다.

디컴파일에는 두 번째 사제 조회가 인자를 둘 받는 것처럼 보인다. 실제 스택 명령을 확인했다. `0, 1`과 contained 타입을 스택에 남겨 두고 두 번째 조회는 플레이어 인자 하나만 정리한 뒤, 반환 SID를 더하여 contained 삭제의 네 인자로 사용한다. `containedType`은 별도 타입 대응 조회이며 공간 모양 포인터가 아니다. C++는 이 순서를 `containedType → findPriest → removeContained`로 유지한다.

## 연결과 안전 계약

[`MakeConstructionNoticeHooks`](../../cpppj/src/o/RawConstructionNotice.cpp)는 같은 풀의 실제 **RawConstructionClear·RawConstructionPlace**를 정리·배치·환불에 연결한다. 선택적으로 **RawPriestState::Immobile**도 연결할 수 있다. [`MakeConstructionNoticeConfirmHooks`](../../cpppj/src/o/RawConstructionNotice.h)는 서버 확정의 직접 처리 경계를 이 처리기에 연결한다. 다른 풀을 연결하면 거부한다.

나머지 경계는 건설 사제 이동, 소유 사제 조회, contained 타입/삭제, 객체 발자국과 접근점 계산, 실제 이동, 가상 삭제, 건설 시작, 갱신이다. 접근점 경계는 원본의 발자국(인자 0) → 접근점 계산 두 함수를 묶고 **스냅은 처리기에서 수행**한다. 실제 경로 탐색이나 건설 진행을 완성한 것으로 취급하지 않는다.

C++는 없는 타입·빈 SID 목록·19개 초과·0번 또는 풀 밖 SID를 정리/이력 변경 전에 거부한다. 이 검증은 원본 수신기의 검증을 복원했다고 주장하는 것이 아니다. 원본 발신이 정상으로 생성하는 목록을 위한 안전 계약이고, 빈 목록의 첫 SID를 읽는 정의되지 않은 경로는 지원하지 않는다. 좌표·double 시각 비트는 보존한다. 독립 실행에는 이력의 NaN/-0 좌표, 접근점의 NaN/무한대/큰 정수도 넣어 실제 CRT 결과와 대조했다.

## 독립 원본 근거와 검사

[`decomp_constructionnotice_oracle.py`](../../tools/decomp_constructionnotice_oracle.py)는 세 실제 PE의 처리기 전체를 두 x87 제어 워드로 실행했다. 새 저장 입력 **1,611개**(판본마다 **537개**), 정상 반환 **3,222회**. 두 정밀도의 관찰이 모두 같다. 저장 입력 누적은 **463,368개**다(앞 단계 461,757 + 1,611).

처리기·SID 항목 조회·genus 조회·거의 올림·CRT는 실제 명령이다. 정리·배치와 그 밖의 효과는 위 명시 경계로 관찰한다. 모든 입력에서 정리 → 배치 → 갱신이 각각 한 번이며, 배치 시점의 이력과 최종 이력을 별도로 저장했다. 배치 경계가 조각의 타입/소유자를 바꾸고 첫 이동 경계가 사제 좌표를 바꾸는 합성 입력도 포함한다. 허용 몸체 밖의 실행 및 이력/스택 밖의 원본 쓰기를 거부하고 정상 반환·보존 레지스터·스택·x87 균형·SHA·사건/호출 수를 감사한다. assert/게임/OS 호출 0.

[`ConstructionNoticeTests.cpp`](../../cpppj/tests/ConstructionNoticeTests.cpp)의 새 검사 **5개**는 세 PE 출력의 전체 재생, 효과 전 잘못된 입력 거부/판본별 필수 연결, 배치 경계에서 상태가 변할 때 뒤쪽 분기가 현재 값을 읽는 것을 확인한다. 기존 [통합 검사](../../cpppj/tests/ConstructionClearTests.cpp)도 수동 처리기 자리에서 **실제 통지 처리기와 두 어댑터**를 쓰도록 바꿨다. 실제 factory·소유자 지정·정리·배치·확정을 통해 예측 noIsland 아홉 조각을 서버 확정 아홉 조각으로 교체하고, 해시 탐색·배치 이력·최종 갱신 횟수를 확인한다. 통합 검사의 Pop/삭제는 좌표·해시 등록/해제 경계다.

Release 경고/오류 **0**·CTest 내부 **577개·실패 0**(147.68초), 근거 감사 **90종 모두 통과**. 새 변이 10개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과). 처음에는 거의 올림을 0.5 반올림으로 바꾸는 변이가 미검출이었다. 양수 20.25/21.25와 음수 -2.75/-3.75 접근점의 독립 원본 입력을 판본마다 둘씩 더해 빈틈을 메웠고, 변이 10개를 다시 실행해 모두 검출했다. 원본 보호 파일·AGENTS.md·dotnetpj 변경과 커밋/푸시 없음.

## 다음 작업

1. 서버 요청 `00444760`/CD `004d17d0`: 기존 전체 MayPlace·사제 선택·확정 호출을 연결하고 요청의 거부/응답을 대조한다.
2. 로컬 배치 `004433b0`와 커서 내려놓기 `004473e0`: SID 확보·예측 배치·통지/취소 수신을 잇는다.
3. 건설 사제 이동 `00443000`/CD `004d1120`, 건설 시작 `00443fe0`/CD `004d06a0`와 진행·완료를 복원한다. 접근점·소유 사제 조회·contained 삭제 경계도 실제 모듈로 잇는다.
4. 예측 정리의 가상 삭제를 공통 삭제 몸체에 연결하며 같은 조각 플래그 `0x2000000`의 후속 효과를 확인한다. 타입별 실제 Pop·건설 메뉴/배치 커서 GUI·메시지 `0x2f`/`0x6c` 수신도 남았다.

게임에서 건물을 지어 미션을 완주하는 흐름은 아직 연결되지 않았다. 한국어 3차 → outpost/LAN 4차 → 화면 요구사항/MCP 5차의 현재 목표 순서를 유지한다.

## 재실행

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name constructionnotice
python -X utf8 tools/decomp_constructionnotice_oracle.py
python -X utf8 tools/decomp_constructionnotice_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

기본 CTest는 저장된 [fixture](../../cpppj/tests/fixtures/constructionnotice-x86.tsv)를 읽으며 원본 게임/Python을 실행하지 않는다. [근거 JSON](../../cpppj/recovery-constructionnotice-evidence.json)과 도구/fixture는 UTF-8/LF다. 새 내보내기는 Git 제외 `extracted/constructionnotice/`, 검사 로그는 `extracted/constructionnotice-*.log`에 있다.
