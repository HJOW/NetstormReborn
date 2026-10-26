# 실제 Editor의 전체 다리 품질 명령

- 작업 PC: HJOW-Athlon, 2026-10-26.
- 마지막 전체 정밀 디컴파일: HJOW-Athlon / 2026-10-23. 디컴파일 결과와 보호 원본은 변경하지 않았다.
- 대상은 원본10.78 `Set All Bridge`의 Cracked/Normal/Hard 명령이다. Add Island·새 맵·Save as·Test Battle 완료는 아니다.

## 원문과 실제 연결

실제 Edit 메뉴의 `00542a4c/020000d6`를 일반 입력의 release 관문 뒤 `RawCampaignSession::SetAllBridgeQuality`에 전달한다. 원문처럼 Editor 또는 missionPost가 참일 때만 실행하고 그 밖의 상황은 변경하지 않는다. 명령 인자0/1/2는 각각 Cracked/Normal/Hard이며 다른 DWORD는 무효과다.

`RawBridgeEditor`는 `00422a70`의 iterator1 선행 증가부터 같은 풀을 순회한다. SID1은 방문하지 않으며 free 비트만 건너뛰고 dead/void를 별도로 제외하지 않는다. 현재 다리 TYPE을 매 방문 다시 읽는다. iterator가 보관한 첫 end는 free 건너뛰기 한도이고, 각 방문의 주소 유효성은 현재 풀 크기로 검사한다. 예약 슬롯도 원문 물리 범위 안이면 직접 쓸 수 있도록 기존 SidPool의 선택적 raw 소비자와 같은 friend 접근을 사용한다.

`00421db0/00421e60/00421eb0`처럼 수명 비트를 먼저 바꾸고 `004214a0` 진단 뒤 현재 TYPE/프레임을 조회한다. Cracked와 Normal의 기존 독립 검증 몸체를 재사용한다. Hard는 현재 방향과 변형P, 마스크40에 맞는 첫256후보를 원문 순서로 수집한다. 후보가0이면 난수를 소비하지 않고 기존 프레임을 유지하며, 후보가1이어도 공통 게임 난수를 한 번 소비한다. 현재 PE의 `0052f960`은5이고 정밀 원문의 확인된 참조는 읽기뿐이므로 기존 다리 규칙의 같은 상수를 사용한다.

프레임은 원문처럼 실제 DWORD에 직접 쓴 뒤 가상+88의 실제 `SquidDisplay::Update`에 전달한다. `SquidFrame::Set`·Unpop·Pop·추가 저장·새 객체 생성으로 대체하지 않는다. 같은 Client 게임 난수를 빌리므로 다른 플레이 요소의 난수 소비 순서에도 반영된다.

## 독립 증거

`tools/decomp_editor_bridges.py`는 실제 PE의 부모·하위 변경·프레임 후보·난수를 격리 CPU로 실행한다. 할당1024/free/가상+88만 외부 장치로 대체하며 C++ 결과로 기대값을 계산하지 않는다. 실제 진입13함수와 허용14함수를 구분하고 함수별 범위/코드 SHA·원본PE·functions.tsv·도구·fixture SHA, 코드 전후 불변을 `cpppj/editor-bridges-native-20261026.json`에 기록한다.

- 독립30행: 모드5종×seed3종×WORD2종. 전 호출 정상 반환·스택 복구, 최대145,742명령.
- Hard는 각 입력에서 후보 할당/해제6회, 난수5회, 가상 갱신5회다. 후보가 없는 방향은 수명만 바뀐다.
- Cracked 갱신4회, Normal 갱신6회, 미지원 모드 갱신0회다.
- `RawBridgeEditor_ReplaysNativeWholePoolTraceAndRandom`은30행의 전체120만BYTE 풀과 순서·난수를 대조한다. 관심8슬롯 외23992슬롯도 그대로여야 한다.
- 부분 실패의 앞선 수명 쓰기와 미지원 모드의 무효과를 별도 확인한다. 실제 Session 검사에서는 보호 TYPE/공유 게임 난수/모드 관문/풀·해시·spot 보존을 확인한다.

최초 helper 허용표 누락 실패는 `extracted/cpp-editor-bridges-native1.log`에 보존했다. native2의 생성 뒤 native3/4의 전체 바이트 재현은 동일했다. 기존 Native fixture는 변경하지 않았고 새 fixture만 추가했다.

## 제품 검증과 남은 범위

최종 Source2의 Release 빌드 경고/오류0, 신규3개를 포함한 집중103고유/112관측과 전체 C++3,165개·실패0을 확인했다. 같은 게임 `c8739bd5…`의 실제 Edit 메뉴에서 다리314개를 대상으로 Cracked314/Normal222/Hard92개 프레임이 바뀌었으며 모든 결과 프레임을 보호 원문 TYPE의 방향/번호/플래그/후보와 대조했다. SID·소유자·좌표는 보존됐다. [통합 문서](cpp-editor-runtime-integration-20261026.md), `cpppj/editor-runtime-integration-20261026.json`을 본다.

새 맵/편집 도구 전체·파일 저장·Test Battle·시나리오와 모든 미션 자연 완주, 화면·음향 동일성은 후속이다. 이번 CPU 대조를 전체 플레이 완료로 합산하지 않는다. CD10.72 전체 품질 명령은 이 PATCH 전용 모듈의 완료 범위가 아니다.
