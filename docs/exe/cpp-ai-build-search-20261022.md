# AI 행별 배치 탐색과 가장 먼 합법 후보

2026-10-22 단위290. 시작 HEAD는 `eb047e82`다. AGENTS/공통·dotnetpj 인수인계와 깨끗한 작업 트리를 다시 읽었다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-21**로 같다. 기존 정밀 원문/보호 PE 정적 읽기만 했으며 새 디컴파일/원본/CD/AGENTS/dotnetpj 변경과 원본/native/C++ 검사/CTest/게임 실행은 없다. 기존 검사·게임 중지를 유지한다.

사제 행동 `00411cd0`의 선행 의존인 **배치 탐색3개 함수**를 `RawAiBuildSearch`에 복원했다. `0040f660`은 상태 초기화, `0040f700`은 행별 작업 예산/최대 거리 후보, `0040f130`은 footprint 전체 빈칸 검사다. `00411b40` 건설 TYPE 선택과 전체 사제 행동은 다음 범위다.

## 실제574 상태와 초기화

같은0xb08 본체의574부터34BYTE만 사용하며 별도 후보 목록/성공 flag를 만들지 않는다.

| 상대 위치 | 본체 위치 | 원문 역할 |
| --- | --- | --- |
| 00/04/08/0c | 574/578/57c/580 | 현재 TYPE/최대 거리제곱/선택x/y |
| 10/14/18/1c | 584/588/58c/590 | 기준x/y/캐시된 TYPE포인터/전체 owner DWORD |
| 20/24/28/2c/30 | 594/598/59c/5a0/5a4 | 최소x/최소y/최대x/최대y/현재행 |

`0040f660`은 native `ret20h`의 **여덟 DWORD 인자**를 따른다. 현재 TYPE→첫 TYPE조회/캐시→기준 좌표/owner→score0→선택 y/x -10000 비트를 쓴다. 두 번째 현재 TYPE조회로 footprint를 읽고 `ftol(left+footX)`, `ftol(top+footY)`, `ftol(right)`, `ftol(bottom)`을 저장한다. 처음 행은 최소y다. 좌표 clamp, ±1 경계 보정, TYPE0 특별 생략을 추가하지 않는다. 다른 본체 바이트는 보존한다.

TYPE포인터는 `kAiSearchTypeBase=6b000000h`와 원본500stride의 **opaque32 대응**이다. 보호 PE의 런타임 TYPE 가상주소를 주장하지 않으며 호스트64비트 포인터를 잘라 저장하지 않는다. 해당 핸들은 같은 현재188TYPE 표로만 역해석한다. 초기 TYPE번호와 캐시18을 구분해, 탐색 중 현재 TYPE는 footprint에 쓰고 현재 캐시 포인터는 전체 배치 검사에 쓴다. 캐시 손상을 원래 TYPE/기본값으로 조용히 바꾸지 않는다. 이 표현은 새로운 게임 TYPE 사본이나 게임 할당 효과를 만들지 않는다.

## 행별 탐색·기한·거리

TYPE0은 시계도 읽지 않는다. 그 외는 처리할 행이 없어도 최초 실제 벽시각을 한 번 읽고 deadline을 만든다. 원본 QWORD `00501230`은 일반 double0.03이 아니라 **double(float0.03)=0.029999999329447746**이다. budgeted0은 행 중단을 하지 않는다. 비영은 매 행 시작에서 현재 벽시각을 읽고 **deadline보다 크거나 unordered이면 중단**한다. equality는 진행한다. 이는 native `test ah,41h / jp`를 그대로 해석한 결과다. 행 내부에는 시간 검사를 넣지 않아 한 행의 배치 검사가 예산을 넘어도 그 행을 완료한다. 재호출은 새 deadline과 기존 현재행/score에서 계속한다.

최소x와현재y는 행 시작에 저장하며 x를 증가시킨다. 최대x는 매 후보 뒤 현재 본체에서 다시 읽는다. 행 완료는 처음 저장한y+1로 덮지 않고 **현재 본체 cursor+1**을 쓴다. 하위 배치 효과가 current minX/cursor/캐시/owner/최대 범위를 바꾼 경우에도 원문 재조회 순서를 유지한다. 완료했다고 TYPE나 선택 좌표를 자동 지우지 않는다. 선택 소비와 TYPE0 쓰기는 상위 사제 행동의 몫이다.

각 후보는 `(spot&3f)==2` 뒤 `0040f130` 전체 footprint를 검사한다. 이 하위는 현재 TYPE의 x-footX+1..x를 바깥 반복, y-footY+1..y를 안쪽 반복으로 방문한다. 상위 bit6/7은 무시한다. 0이하 footprint에서 원본이 칸에 접근하지 않고 true를 돌려줄 수 있는 동작도 유지한다. 손상된 지도 주소는 명시 오류이며 합법 칸으로 clamp하지 않는다.

합법 빈칸 후보는 기준점으로부터 **가장 먼 거리 제곱**을 고른다. x 차이만 먼저 float에 저장하고, y 차이와 두 제곱/합은 그 전 float 축소를 넣지 않는다. 거리 합도 float 저장 전 값으로 현재 float score와 엄격하게 비교한다. 같은 거리, 거리0/초기 score0, NaN은 교체하지 않는다. 더 클 때만 원본 전체 배치 `0049b510(cachedTYPE,argument0,xfloat,yfloat,flags0,ownerBYTE,mode1)`를 호출한다. 허용이면 하위 효과 뒤 원본처럼 이미 계산한 float score와 좌표를 무조건 기록한다. 현재 score 재비교를 추가하지 않는다.

## 실제 Session 연결

같은 현재 `core.types`/`core.spots`와 실제 전체 `MayPlaceGameplay`를 사용한다. 시계는 기존 `services.construction.wallSeconds`를 공유하며 실제 Client에서는 매 호출 `Client::WallSeconds()`→같은 `GameClock/timeGetTime()`이다. 게임 진행 시각이나 프레임 snapshot/임의 상수로 대신하지 않는다.

명시 API `StartAiBuildSearch(owner,type)`는 원본14af0 준비가 만든 자기 경계270..27c와 적 기준2b0/2b4, owner260을 현재 본체에서 읽어 초기화한다. `AdvanceAiBuildSearch(owner,budgeted)`는 같은 상태를 진행한다. bool은 brain 존재/호출 여부이며 후보 존재/배치 성공을 뜻하지 않는다. 실제 사제 행동과 자동 프레임은 아직 호출하지 않는다.

## 작성·컴파일 확인

새14검사는 하위13개+실제 Session1개다. 정확한34BYTE/주변 보존·signed 경계 절삭, 전체footprint/bitmask/빈 크기, TYPE0/빈행 시계 순서, 최대거리/동률/0/NaN, lazy TYPE조회, 행 중단/재개/equality/unordered/행 원자성, 캐시·owner·행 snapshot/최대x 재조회, 선택 하위 누락/손상 주소와 거리 float 저장 전 비교를 다룬다. 큰 기준 좌표에서는 저장할 float score가 기존값과 같아도 비교 전 실제 거리 값은 커서 허용 검사를 해야 하는 사례를 포함했다. Session 검사는 실제 원본 전략 경계/적 기준/spot/전체 배치/현재 시계/소멸을 사용한다. **모두 작성·컴파일만 하며 실행하지 않는다.**

Release 빌드 세션5388은 실제 종료까지 조회했고 **종료0·경고0·오류0**이다. `extracted/cpp-ai-build-search-20261022-build1.log`와 별도`build1-result.json`을 보존했다. 로그를 내용/개행 변경 없이 UTF-8 바이트로 변환하고 문자열 동등성을 확인했다. [신규 정적 보고서](../../cpppj/ai-build-search-static-20261022.json)는 원본8함수/새14검사 링크·최종 해시와 실제 연결을 기록하며 실행 수는 모두0이다.

[`cpp_ai_build_search_audit.py`](../../tools/cpp_ai_build_search_audit.py)는 원본8함수의 기존 정밀 범위 명령/해시, 예산 double/여덟 인자/float 저장·기한/TYPE stride·사제 caller, 직전 보호 원본 동일 SHA와 최종 소스/바이너리 SHA·새14검사 링크·실제 Session 시계/배치 공유를 기록한다. C++ 검사/CTest/게임/native 실행0이며 과거 기록을 덮어쓰지 않는다.

전체 x87 확장 정밀도/예외와 손상된 물리 주소/캐시의 원본 오류 결과, 자연 탐색 예산/건설·화면/음향은 실행 대조하지 않았다. CD10.72는 기존 대응 `004dc160`(탐색), `004d71d0`(빈 footprint)을 참고로만 남겼고 별도 구현/실행 대조는 없다. 초기화 `0040f660`의 CD 대응은 없다.

다음은 `00411b40/00411a40/00411ad0` 건설TYPE 선택/자금 조건과 실제 `00411cd0` 사제 행동이다. 기존 `0040f660/0040f700` 자리의 가짜 성공을 끼우지 않고 이번 실제 탐색을 연결한다. 건물/다리/가짜 부족좌표와 원래 순서 자동 AI 프레임·종료, 목표15개(Early Missions6+캠페인1-1..1-6+데모3)+Edit/custom Fort·시나리오·전환/저장 수명 및 자연 플레이 동일성은 계속 남아 있다. 인수인계 완료 뒤 `20261022-13 CP`로 로컬 커밋하고 원격 push하지 않는다.
