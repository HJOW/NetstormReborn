# N·Shift+N 다음 이동 유닛 선택 — 단위702

2026-11-02, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23 자료다. 원문 `004da880`은 있었지만 현재 UserInput에는 호출이 없었다. 이제 N/n의 누름 사건을 같은 실제 Session 풀·선택·카메라에 연결한다. Shift는 원문 비트 `0x20000000`을 그대로 전달한다.

## 검색과 효과 순서

`NextUnitSelection`은 다음 원문 규칙을 보존한다.

- 현재 기본 선택이 free이면 시작점을0으로 취급한다. 예약1 뒤부터 선행 증가하고, 현재 선택이 있으면 그 다음부터 검색한 뒤 처음으로 한 번 순환한다.
- iterator `0040eb40`처럼 free 비트1만 건너뛴다. dead2나 내용물/void를 임의 제외하지 않는다. 소유자 BYTE와 검색 전 캡처한 전체 localOwner DWORD를 비교하고 TYPE의 flags2&30000을 읽는다.
- 후보에 먼저 실제 Select(candidate,0,1), CenterOn(candidate), 현재 paused 순서다. 첫 선택이 정지/콘솔 등으로 거부되어도 카메라 이동을 생략하지 않는다.
- 정지가 아니면 일반 N은 Select를 다시 호출한다. Shift 비영값이면 후보의 현재 이동 플래그를 다시 읽고, 현재 기본 선택의 free/이동 상태에 따라 ToggleMultiple(candidate,1) 또는 두 번째 Select를 호출한다. Shift를 임의 누적 선택 정책으로 바꾸지 않는다.

`GameWorld`는 기존 `RawUserSelection::Select/ToggleMultiple`과 `CenterOnObject`를 경계로 공급한다. 현재 소유자와 정지도 기존 실제 장치를 사용한다. 입력 `004d9a2f..004d9a42`의5명령에서 and20000000→push→call004da880→스택 정리→복귀를 정적으로 기록했다. 원문 switch의ASCII4e는 N 분기다.

## 원문과 집중 검사

[독립 원문 도구](../../tools/decomp_next_unit_selection.py)는 실제004da880·0040eb40·004ac200을 **410개 입력**으로 실행했다. 선택004d5c90/카메라00433ba0/정지00460de0/다중004da6b0와 TYPE 주소 조회0049a840은 명시 경계다. 반환/호출 인자 순서·선택/소유자와 변경된8개 상태/플래그를 [fixture](../../cpppj/tests/fixtures/next-unit-selection-x86.tsv)에 기록했다. [원문 보고서](../../cpppj/next-unit-selection-native-20261102.json)는 기계어/도구/fixture 해시와 경계를 포함한다.

원문 fixture는8칸 입력 풀이고 제품 집중 검사는 지원되는 실제24000칸 풀에 같은8개 입력을 놓고 나머지를 모두 free로 유지한다. 후미의 무후보 순회 길이가 같다는 주장이 아니며, 선택/효과의 관측 결과를 대조한다. 실제 공중 유닛을 생산/생성하거나 행동시키는 입력은 아니다.

새 C++3개는410행 전체 대조, 실제 단일 선택/다중 토글 접합과 정지 시 가운데 이동, 도달한 경계 누락의 선행 효과 보존이다. 기존 다중/단일 선택15개와 함께 **18개 통과·실패0**이다. 기존601/1074/87행 fixture는 재생했고 새 원문 실행 수에 합산하지 않는다. `build-v1` Release는 경고·오류0이며 `focused-v1`은 첫 실행에서 통과했다.

## 제품·보호·남은 범위

실행 제품702/표시11.01의 SHA-256은 `ccc59b4926ccb9ece2f1a431bdc613dd84d92e398db1da5e781c62c193ad28e4`다. [제품 보고서](../../cpppj/next-unit-selection-20261102.json)와 원시 `extracted/cpp-next-unit-selection-702/`를 따른다. 기존 AGENTS·원본1394·CD425·fixture800의2620개 변경0, 새fixture1 포함 현재2621개를 확인했다. AGENTS/전체 디컴파일 자료는 수정하지 않았다.

이번 게임/GUI 실행은0이다. 최신 실제 GUI는700 제품이며702의 N·Shift+N 화면 성공으로 합산하지 않는다. 전체 C++·전체34·새 ZIP·물리 입력·실제 PCM·픽셀 동일성·포획/희생·특정 적 공격·승리·공중3종 전용 생산/생성/행동·MP/통신 검사는 없다. 손상된 포인터/풀 외부 접근·전체 예외 ABI는 보장하지 않는다.

다음은 남은 ESC/View·도킹·DD 조정/확인 UI와 Edit 지형/객체·시나리오 writer/파일 수명이다. 기본 Shift 그룹 선택/이동701과 다음 유닛 선택702를 다시 미구현으로 세지 않는다. 전체 목표는 미완료이며 MP 무시험 사전 구현과4~7차 순서를 유지한다.
