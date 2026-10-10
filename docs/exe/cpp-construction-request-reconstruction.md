# 서버 건설 요청 복원

2026-10-11, **HJOW-Athlon**. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 [배치 통지 처리기](cpp-construction-notice-reconstruction.md)의 다음 항목을 진행했다. 시작 작업 트리는 깨끗했고 현재 PC가 마지막 디컴파일 PC와 같았다. 새 `constructionrequest` 목록 **26/22/22개**를 읽기 전용 Ghidra로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 원본 게임·복사본·analyzeManager·클론 창은 실행하지 않았다.

## 복원한 요청 순서

[`RawConstructionRequest`](../../cpppj/src/o/RawConstructionRequest.h)는 **10.78 `00444760` ↔ CD/10.37 `004d17d0`**의 서버 요청 몸체를 복원한다. 디컴파일 대응은 신뢰도 A, `callseq/strong`(0.818)이다. 입력은 앞 단계 `ConstructionConfirmRequest`와 같은 원본 열 DWORD다. CD 디컴파일에는 일곱 인자로 보이지만 실제 호출 명령과 확정 경계의 관찰에서는 **시각의 하위/상위 DWORD와 품질까지 열 인자**를 전달한다.

1. 편집 모드이면 요청 플레이어를 현재 로컬 플레이어 **DWORD 전체**로 바꾼다.
2. 전체 MayPlace에 `(타입, decoder 인자, x, y, 방향, 플레이어, mode=0)`을 전달한다. 거부하면 나머지 효과를 생략한다.
3. decoder의 모든 유효 칸에서 현재 타입의 발자국 범위를 구하고 일반 finder로 훑는다. 후보가 abstract이고 noIsland 타입이 아니면 **소유자 BYTE = 요청 플레이어 DWORD, 타입 BYTE = 요청 타입 DWORD, float x/y = decoder 칸 좌표**를 모두 만족해야 한다. 하나라도 다르면 즉시 false다. 프레임은 비교하지 않으며 서버 영역의 후보도 검사한다. buried 제외는 일반 finder 자체의 동작이다.
4. 타입 `flags2 & displaceGenus`가 겹치면 기존 유닛 밀어내기 경계를 호출한다. 아래 판본 차이를 따른다.
5. island 타입의 폭·높이로 받침 시작 좌표를 구하고 요청 타입의 float 발자국 사각형을 계산한다. `flags1 & 0x400` 타입만 받침을 검사한다.
6. 발자국의 정수 칸에 **spot bit 2가 하나도 없거나**, 요청 좌표의 현재 표면이 **abstract**이면 noIsland를 먼저 확정한다. 받침 인자/방향/플래그/시각/품질은 모두 0이다. **받침 확정의 실패 반환값을 무시하고 본 요청을 계속한다.**
7. 본 요청의 열 인자를 보존하여 확정한다. 성공이면 true다. 실패하면 현재 타입의 `flags1 & 0x400`을 다시 읽고 `(x,y,0x1000,플레이어)`로 실패 정리 경계를 호출한 뒤 false다.

받침 좌표는 `(x - (island 폭 - 1), y - (island 높이 - 1))`를 단정도에 저장한 값이다. 같은 noIsland 타입을 요청하더라도 주변 noIsland 후보는 소유자/좌표 비교에서 제외된다. 빈 decoder도 뒤쪽 밀어내기/확정 단계까지 진행하는 원본 동작을 유지했다. 빈 목록이 만들어진 뒤의 통지 처리기는 앞 단계의 안전 계약에 따라 별도로 거부하므로, 빈 decoder의 통합 실행을 지원한다고 주장하지 않는다.

## 판본 차이와 좌표 처리

10.78 `00443800`은 `(x,y,타입,플레이어)`를 받고 **false이면 중단**한다. CD `004d2450`은 `(x,y,타입)`의 void 호출이며 **플레이어 인자가 없고 반환 검사도 없다**. 두 함수를 사제 선택으로 해석하지 않았다. 유닛을 밀어내는 실제 하위 공간/이동 효과는 후속이다.

요청의 칸 범위는 [정리 모듈의 CellArea](cpp-construction-clear-reconstruction.md)와 같은 산술이다. 발자국만큼 왼쪽/위로 물린 점을 단정도로 저장하고 1~255로 자르며 유효한 원래 칸 좌표와 잇는다. 작은 모서리는 절삭, 큰 모서리는 **+0.99999 뒤 절삭**이다. 이번에는 타입 발자국 helper `0049ae80`/`004449d0`를 실제 실행하여 재사용한 C++ 계산을 따로 대조했다.

받침 검사의 **spot 사각형은 큰 모서리도 절삭**하며 inclusive 정수 범위를 훑는다. 표면 SID 조회는 두 판본 모두 **+0.9999 뒤 절삭**한다(패치 `00500edc`, CD `00506958`: `0.9998999834060669F`). 해시 0단계 머리를 직접 읽고 지도 밖 좌표이면 SID 0의 예약 슬롯을 사용한다. finder 범위의 거의 올림, spot의 절삭, 표면의 거의 올림을 서로 다른 연산으로 유지했다. CD helper의 Ghidra `(int)float` 표시는 실제로 float 비트를 signed DWORD와 비교하는 명령이어서 양수 0.5도 유효하다. 두 판본의 0.5 좌표 원본 관찰과 C++가 일치한다.

## 실제 모듈 연결과 남은 경계

[`MakeConstructionRequestHooks`](../../cpppj/src/o/RawConstructionRequest.cpp)는 같은 풀의 **RawCanonPlacementPipeline::MayPlace**와 **RawConstructionConfirm::Confirm**을 연결한다. 파이프라인에 `Pool()` 조회를 추가해 다른 풀 연결을 거부한다. 요청·배치·확정의 editor/localPlayer/패턴 번호는 원본 공통 전역이므로 호출자가 같은 값을 공급하고 자료 참조의 수명을 보장한다.

확정은 앞 단계 어댑터를 통해 실제 factory → 통지 처리 → 예측 정리 → 조각 배치/소유자 지정까지 연결할 수 있다. 사제 선택은 확정 모듈의 기존 명시 경계이며 실제 선택 몸체를 새로 복원하지 않았다. 유닛 밀어내기·실패 정리의 하위 몸체, 실제 Pop/삭제, 이동/건설 진행, wire 메시지 수신·로컬 요청·커서·GUI도 후속이다.

C++는 누락된 필수 경계, 짧은 spot 지도, 없는 요청/섬/받침 타입, 비유한 좌표를 첫 효과 전에 거부한다. 원본 함수에 이 검증이 있다고 주장하는 것이 아니라 C++ 자료/효과 연결의 안전 계약이다. 원본 대조는 유한 시작 좌표·1~10 발자국·1~255 후보 좌표를 사용하며 assert 경로는 제외했다.

## 독립 원본 근거와 검사

[`decomp_constructionrequest_oracle.py`](../../tools/decomp_constructionrequest_oracle.py)는 세 실제 PE에서 요청 전체·decoder·타입 발자국·좌표 자르기/절삭/거의 올림·일반 finder·spot/표면 조회를 실행한다. **MayPlace·유닛 밀어내기·확정·실패 정리만 명시 경계**이며 탐색기 Begin 인자는 대체 없이 관찰한다. 다른 몸체/OS 실행과 스택 밖의 쓰기를 거부하고 풀·해시·spot 불변을 확인한다.

새 저장 입력은 **2,192개**(10.78 **816**, CD/10.37 각각 **688**), 두 x87 정밀도의 최상위 정상 반환 **4,384회**다. 두 정밀도의 반환/효과 관찰이 모두 같다. 누적 저장 입력은 **465,560개**(463,368 + 2,192). 각 행에 입력·후보·반환·MayPlace/탐색/밀어내기/확정/실패 정리의 순서와 인자 비트를 저장했다. 보존 레지스터·cdecl/thiscall 스택·x87 균형·SHA·사건/원본 호출 수를 감사하며 assert/게임/OS 호출은 0이다.

[`ConstructionRequestTests.cpp`](../../cpppj/tests/ConstructionRequestTests.cpp)의 새 검사 **6개**는 세 PE 전체 재생, 효과 전 잘못된 자료 거부, 확정 경계가 바꾼 현재 플래그의 실패 정리 반영, 실제 모듈 통합을 확인한다. 통합 검사는 강제 MayPlace를 켜지 않고 모양·미리보기·충돌·지형·최종 관계를 실행한다. 지면 정책이 거부한 첫 요청에서 예측 아홉 조각을 보존하고, 원본 지면 허용 전역을 켠 요청은 실제 factory·확정·통지·정리·배치·소유자 지정으로 서버 조각 아홉 개로 교체한다. Pop/삭제는 좌표와 해시 삽입/해제 경계로 제한했다.

Release 경고/오류 **0**·CTest 내부 **583개·실패 0**(143.80초, 전체 143.84초), 근거 감사 **91종 모두 통과**. 새 변이 11개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과). 원본 보호 파일·AGENTS.md·dotnetpj 변경과 커밋/푸시 없음. 변이는 저장소 밖 사본에만 적용했다.

## 다음 작업

1. 로컬 배치 `004433b0`, 커서 내려놓기 `004473e0`: SID 확보·예측 배치·서버 요청 발신과 통지/취소 수신을 연결한다.
2. 요청의 유닛 밀어내기 `00443800`/CD `004d2450`·실패 정리 `0046e6c0`/CD `0048f960`와 확정의 실제 사제 선택을 복원한다.
3. 건설 사제 이동 `00443000`/CD `004d1120`, 시작 `00443fe0`/CD `004d06a0`와 진행/완료를 복원한다. 소유 사제/접근점/contained 경계도 잇는다.
4. 예측 삭제의 같은 조각 플래그 `0x2000000` 후속 효과·타입별 실제 Pop·건설 메뉴/배치 커서·메시지 `0x2f`/`0x6c` 수신을 구현한다.

**게임에서 건물을 지어 미션을 완주하는 흐름은 아직 연결되지 않았다.** 한국어 3차 → outpost/LAN 4차 → 화면 요구사항/MCP 5차의 현재 목표 순서를 유지한다.

## 재실행

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name constructionrequest
python -X utf8 tools/decomp_constructionrequest_oracle.py
python -X utf8 tools/decomp_constructionrequest_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

기본 CTest는 저장한 [fixture](../../cpppj/tests/fixtures/constructionrequest-x86.tsv)를 읽고 원본 게임/Python을 실행하지 않는다. [근거 JSON](../../cpppj/recovery-constructionrequest-evidence.json)·도구·새 문서는 UTF-8이다. 새 내보내기는 Git 제외 `extracted/constructionrequest/`, 로그는 `extracted/constructionrequest-*.log`에 있다.
