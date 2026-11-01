# P/R 실제 사제 선택·반복 View 명령 — 단위703

2026-11-02, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23 자료다. 기존 P/R는 `world:priest`의 화면 좌표를 다시 Pick해 선택했으므로 가림/화면 밖 상태와 반복 키 동작이 원문과 달랐다. 이제 기존 실제 참여 조회 `FindPriestParticipation`과 세 인자 `RawUserSelection::Select`, 같은 일반 입력 큐를 사용한다.

## 복원한 순서와 수명

원문 `004d9884..004d98e1`은 현재 owner와 **조회 전 selected**를 캡처하고00494f10을 호출한다. 결과가 앞서 캡처한 selected와 다르면005caf40을0으로 쓴다. 결과가 같고 현재 카운터가 비0이면 현재 owner를 다시 읽고 View Your Priest 명령(005429d4/020000b8)을 x/y 모두 같은 owner로 큐에 넣는다. 나머지는 Select(candidate,0,1)을 호출한 뒤 **그때의 현재 카운터**에1을 더한다.

후보0·선택 거부에도 임의 조기 반환을 넣지 않는다. 조회가 selected를 바꿔도 비교 기준을 현재 selected로 바꾸지 않는다. 선택 콜백 뒤 현재 ffffffff는0으로 감긴다. 반복 키는 직접 카메라를 부르지 않고 기존 메뉴 명령 분배기를 거친다.

누적 DWORD를 실제 Client 선택 owner에 `priestSelections`로 추가했다. BSS0으로 시작하고 `ResetForGeneralWorld`, 단일/다중 정리에서는 초기화하지 않는다. 원문 전체 정밀 자료에서005caf40의 입력 분기 쓰기만 확인했다. 명시 ReadDword/WriteDword에도 별도 필드로 연결했으며 기존9개 필드 번호는 유지했다.

## 원문과 집중 확인

[독립 원문 도구](../../tools/decomp_priest_selection_shortcut.py)는 **P/R 부분 분기220입력**을 실행했다. 허용 범위는004d9884..004d98e1과 일반 큐 call/스택 정리004d9bb3..004d9bba다. 공통 후속점004d9bbb에서 멈추고 입력 대비 스택 증가0을 확인했다. 전체 함수 반환이나 실제 OS/창 입력을 합성하지 않았다.

00494f10 참여 조회,004d5c90 선택,00452fe0 큐는 명시 경계다. 조회 뒤 selected/owner/count 변경과 선택 뒤 현재 카운터 변경을 포함하며, 실제 사제 포획/희생/운반 행위를 재현하는 검사가 아니다. [fixture](../../cpppj/tests/fixtures/priest-selection-shortcut-x86.tsv)·[원문 보고서](../../cpppj/priest-selection-shortcut-native-20261102.json)는 호출 순서·반환 뒤 selected/count/owner와 해시를 보존한다.

새 C++3개는220행 전체 대조, 실제 Client 카운터/World reset 뒤 보존과 실제 InputQueue의020000b8/owner 인자, 경계 누락 선행 효과와 ffffffff 감김이다. 기존 N/다중/단일 선택18개를 함께 실행해 **21개 통과·실패0**이다. 기존410/601/1074/87행은 fixture 재생이며 새 원문 실행 수에 합산하지 않는다. `build-v1` Release는 경고·오류0, `focused-v1` 첫 실행 통과다.

## 제품·보호·다음

실행 제품703/표시11.01 SHA-256은 `09148c38829a6a234d925a6790af463938bac3d3626ea642714a800bf7830622`다. [제품 보고서](../../cpppj/priest-selection-shortcut-20261102.json)와 원시 `extracted/cpp-priest-selection-shortcut-703/`를 따른다. 기존 AGENTS·원본1394·CD425·fixture801의2621개 변경0, 새fixture1 포함 현재2622개를 확인했다. AGENTS/전체 디컴파일 자료는 수정하지 않았다.

이번 실제 게임/GUI 실행은0이다. 최신 GUI는700 제품이다. 703의 P/R 화면 동작·물리 입력·전체 픽셀/PCM·실제 사제 상황별 선택 성공을 확인했다고 확대하지 않는다. 전체 C++·전체34·새 ZIP·포획/희생·특정 적 공격·승리·공중3종 전용 생산/생성/행동·MP/통신 검사는 없다.

다음 입력 지점은 H/F4/F5의 View 명령 큐와 기존 메뉴 관문 대조다. H의004d9879는005429ac를 읽어 일반 큐 경로로 가지만 현재 H/F4는 직접 Home을 부른다. 전체 분기와 F4/F5는 다음 단위에서 확인하며 아직 완료로 세지 않는다. 이어 남은 ESC/View·도킹·DD 조정/확인 UI와 Edit 지형/객체·시나리오 writer/파일 수명을 잇는다. 전체 목표는 미완료이며 MP 무시험 사전 구현과4~7차 순서를 유지한다.
