# 로컬 건설 배치 복원

2026-10-11, **HJOW-Athlon**. AGENTS.md·두 LEFT_JOBS와 [서버 요청 인계](cpp-construction-request-reconstruction.md)를 확인했다. 시작 트리는 깨끗하고 현재/마지막 디컴파일 PC가 같았다. 새 `constructionlocal` **11/12/12개**를 읽기 전용 Ghidra로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 원본/복사본·analyzeManager·클론 창은 실행하지 않았다.

## 복원한 로컬 순서

[`RawConstructionLocal`](../../cpppj/src/o/RawConstructionLocal.h)는 **10.78 `004433b0` ↔ CD/10.37 `004d14a0`**의 전체 몸체다. 대응은 신뢰도 A, `callseq/strong`(0.834). Ghidra의 네 float 인자 표시는 실제 호출 명령과 다르며, 입력은 **미리보기 SID, float x/y, DWORD 인자/방향** 다섯 개다.

1. 미리보기의 **+18 WORD 타입**을 먼저 캡처하고 가상 +0으로 연관 객체 번호를 조회한다. 조회 뒤 **+20 WORD 반환 플래그, +30 BYTE 품질(패치)/+26 BYTE 품질(CD)**을 캡처한다. 일반 Squid의 타입 BYTE +10과 혼동하지 않는다.
2. 가상 +0x10으로 미리보기를 플래그 0으로 삭제한다. 캡처한 플래그에 **0x8 비트**가 있으면 `(플래그 전체, 연관 번호 DWORD 전체, 품질)`로 생산 칸 조각 반환 helper를 부른다.
3. 실제 CanonDecoder의 유효 칸마다 **factory(type, 2)**로 클라이언트 SID를 확보한다. 본 조각 SID가 받침 SID보다 먼저 만들어진다.
4. 타입 `flags1 & 0x400`이면 **시작 칸 하나의 spot bit 2**를 검사한다. 좌표는 float를 넓혀 **+0.9998999834060669 뒤 0방향 절삭**한다. 서버 요청의 전체 발자국 검사와 다르다.
5. 시작 칸에 표면 bit가 없으면 `(x-(island 폭-1), y-(island 높이-1))`에서 noIsland decoder의 받침 SID를 같은 플래그 2로 확보한다. **받침을 abstract=1·품질=0·인자/방향=0으로 먼저 배치**한다.
6. 본 SID 목록을 원래 좌표/인자/방향과 캡처한 품질로 **abstract=1** 배치한다. 소유자는 현재 로컬 플레이어 DWORD다.
7. 본 배치 뒤 현재 타입 플래그를 다시 읽는다. **0x8000**이면 첫 본 조각 SID로 사제 이동을 요청하며 시각은 두 DWORD 모두 0이다. 빈 decoder의 첫 SID는 원본 지역 배열 초기값 **0**이다.
8. 요청 메시지를 초기화하고 **서버 번호 9**로 보낸다. 받침/본 배치와 사제 이동이 모두 전송보다 앞선다. 로컬 배치 자체에는 비용 차감이 없다.

요청은 머리 4바이트 뒤 **타입 BYTE + float x/y + 인자 WORD + 방향 BYTE + 품질 BYTE + 0시각 8바이트**로, 본문 21바이트다. 두 판본 모두 품질 바이트를 쓴다. CD 디컴파일에서 누락된 품질 인자는 `004d17a6`·`004d17aa`의 BYTE 쓰기로 확인했다. 인자는 배치까지 DWORD 전체를 전달하고 전송에서만 하위 WORD로 줄인다. 시각 생성자/메시지 초기화도 원본 명령으로 실행했다. 메시지 등록/전송 계층 자체는 아직 경계다.

## 실제 연결과 자료 계약

[`MakeConstructionLocalHooks`](../../cpppj/src/o/RawConstructionLocal.cpp)는 같은 SID 풀의 **SquidFactory::Create**와 **RawConstructionPlace::Place**를 연결한다. 다른 풀의 factory/배치는 거부한다. 타입/프레임/지도/플레이어·패턴 전역의 수명과 일관성은 호출자가 보장한다. SID 목록 span은 Place 호출 중에만 빌려 쓰므로 경계에서 보관하면 안 된다.

기존 [ConstructionRequestTests](../../cpppj/tests/ConstructionRequestTests.cpp)의 통합 장면은 수동으로 예측 아홉 조각을 만들던 부분을 실제 로컬 배치로 바꿨다. **로컬 factory/abstract 배치 → 요청 본문 → 실제 전체 MayPlace → 서버 확정/factory → 통지 → 예측 정리 → 서버 배치/소유자 지정**까지 같은 풀을 공유한다. 지면 정책의 거부는 예측을 보존하고, 허용 전역을 켠 재요청은 아홉 서버 조각으로 교체한다. Pop/삭제의 공간 효과는 좌표와 해시 삽입/해제 경계에 한정했다. 실게임 wire 메시지 라우팅을 연결한 것은 아니다.

별도 통합 검사는 본 다리 SID를 먼저 확보하고 받침 아홉 칸을 먼저 놓는 실제 모듈 순서, abstract/HP 0/소유자, 금 간 다리의 품질 프레임과 수명 4, 사제 이동/요청 순서를 두 판본에서 확인한다. Form 생성·가상 연관 조회/삭제, 생산 칸 조각 반환 `004468b0`/CD `0043e930`, 사제 이동 `00443000`/CD `004d1120`, 메시지 전송은 아직 명시 경계다.

C++는 짧은 지도/필수 경계 누락, 예약/풀 밖 미리보기, 없는 타입, 비유한 좌표와 필요한 spot의 지도 밖 인덱스를 거부한다. SID 생성 번호와 지역 목록 **18개 상한**도 검사한다. 이 진단은 안전한 호스트 계약이며 원본의 모든 오류 경로를 복원했다고 주장하지 않는다. 생성/배치 경계가 예외를 던지면 이미 발생한 삭제/할당을 되돌리지 않는다. 상위 cursor가 배치 가능 판정을 마친 입력을 넘기는 것이 원래 호출 계약이다.

## 비패턴 방향 보정

새 기계어 관찰은 비패턴 타입에 **0x100 및 음수 방향**을 전달해도 배치가 진행되고 원래 DWORD가 배치 경계에, 하위 BYTE가 요청에 실리는 것을 보여 줬다. 기존 C++ 비패턴 CanonDecoder의 0~3 회전 제한은 이 입력을 거부했다. 비패턴 경로에서는 회전표의 읽기 결과가 좌표/프레임 출력에 영향을 주지 않으므로 내부 회전은 0으로 유지하고 방향 크기/부호 거부를 제거했다. **CD 홀수 방향 거부와 패턴 타입의 회전 범위 검사는 유지**한다. raw decoder 내부 바이트나 표 밖 읽기 자체를 복제하는 변경이 아니다. 새 로컬 관찰과 기존 decoder/배치 전체 회귀로 확인한다.

## 독립 근거와 검사

첫 전체 회귀는 방향 8을 거부해야 한다고 가정한 기존 `priest_geometry_guards` 한 항목에서 실패했다. 방향 8의 전체 로컬 관찰을 세 PE에 추가하고, 그 검사를 정상 한 칸/프레임/좌표/종료를 확인하도록 고쳤다. 최종 결과는 이 수정 뒤 재실행한 것이다.

[`decomp_constructionlocal_oracle.py`](../../tools/decomp_constructionlocal_oracle.py)는 로컬 전체·decoder/프레임 검색·좌표 절삭·요청 초기화/길이 쓰기·CD 0시각 생성을 실제 명령으로 실행한다. **가상 Form 조회/삭제·생산 칸 반환·SID 생성·예측 배치·사제 이동·전송만 대체**한다. 저장 입력은 **1,450개**(558/446/446), 두 x87 정상 반환 **2,900회**, 누적 **467,010개**(465,560 + 1,450). 조회에서 raw 플래그/품질을 바꾸고 삭제에서 raw를 지우며 본 배치에서 타입 플래그를 바꾸는 입력도 포함한다.

저장 [fixture](../../cpppj/tests/fixtures/constructionlocal-x86.tsv)와 [근거 JSON](../../cpppj/constructionlocal.json)은 전체 사건 순서/인자/실제 SID 목록과 메시지 본문을 담는다. 실제 진입/정상 반환·경계 사건 수·cdecl/thiscall 스택·보존 레지스터·x87·실행/쓰기 범위·SHA를 감사하며 원본 assert/게임/OS 호출은 0이다. 원본 기대값을 C++나 Python의 별도 분기 계산으로 만들지 않는다.

새 C++ 검사 **6개**는 세 PE 전체 재생, 입력/자료 거부, Form 캡처와 현재 타입 플래그, 실제 factory/받침/품질 연결을 확인한다. 기존 요청 통합 검사도 앞의 로컬 흐름까지 확장했다. Release 경고/오류 **0**·CTest 내부 **589개·실패 0**(142.96초), 근거 감사 **92종 모두 통과**. 새 변이 11개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과).

## 다음 작업

1. **커서 내려놓기 `004473e0` ↔ CD `0043d660`**: 법적 위치 갱신·반올림·world/GUI 분기·droppiece.wav·로컬 배치·비용 차감·커서 잠금/정리. 기존 이동 조각과 새 건설 조각을 구별해야 한다. 이번에 함수의 어셈블리를 읽었지만 복원/독립 검증은 아직 하지 않았다.
2. 생산 칸 반환 helper, 요청의 유닛 밀어내기/실패 정리, 확정의 실제 사제 선택을 잇는다.
3. 사제 이동·건설 시작/진행/완료, 예측 삭제의 `0x2000000` 후속 효과·타입별 Pop, 메시지 `0x2f`/`0x6c` 수신·건설 메뉴/커서 GUI를 구현한다.

**게임에서 건물을 지어 미션을 완주하는 흐름은 아직 연결되지 않았다.** 한국어 3차 → outpost/LAN 4차 → 화면 요구사항/MCP 5차의 현재 목표 순서를 유지한다.

## 재실행

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name constructionlocal
python -X utf8 tools/decomp_constructionlocal_oracle.py
python -X utf8 tools/decomp_constructionlocal_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

기본 CTest는 저장 fixture만 읽고 원본 게임/Python을 실행하지 않는다. 새 내보내기/로그는 Git 제외 `extracted/constructionlocal/`·`extracted/constructionlocal-*.log`에 있다.
