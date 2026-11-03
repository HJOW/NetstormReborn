# Script 캐시의 실제 호출 분류

단위792는791(`3725d58b`) 뒤의 남은 Script 파일 후보를 정적으로 분류했다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 현재 호스트와 같다. 원본·추출물·기존 fixture·AGENTS.md·제품 코드는 변경하지 않는다.

## 현재 코드와 원문 부모

현재 C++ 소스의 `Script(` 토큰은 UberGump의 선언·정의·직접 호출 한 곳이다. 직접 호출은 메뉴 표시의 `edition==Patch1078 ? "UCampaign" : Script("tell")...` 조건식에 있다. 10.78은 고정 문자열을 선택하므로 Script 캐시를 호출하지 않는다. 나머지 CD/호스트 fallback에서는 기존 필수 snapshot 읽기와 영구 캐시 수명을 유지한다.

원문의 `missionSpec` 명시 참조는00440760(@파일 키 확장),00482fb0(미션 시작),004c8f60/004cf730(Tell 조회/실행) 네 부모이며 각 경로는00440380을 호출한다. 현재771의 Config 전용 파일 장치와 구별해 조사했으며, 주소/문자열 일치만으로 UberGump.Script 전체를 같은 원문 부모로 가정하지 않는다. 이번 분류는 기존 Config·Tell·Mission의 전체 동작/간접 caller 재검증이 아니다.

따라서785의 미확정 Script cache 후보는 **10.78 활성 직접 caller가 없는 CD/호스트 경계 유지**로 정리한다. 필수 읽기를40380의 선택 읽기로 임의 변경하거나 이 호스트 캐시를 새10.78 파일 부모 복원으로 합산하지 않는다. 785의 분류 JSON은 당시 snapshot으로 그대로 보관하고 현재 결과는 [보고서](../../cpppj/script-cache-callers-20261103.json)를 따른다.

## 범위와 현재 제품

`tools/cpp_script_cache_callers.py`는 현재 src의 cpp/h/inl과 정밀 raw.c를 읽어 위 세 토큰·조건식·원문 네 부모 및 소스 해시를 기록한다. 정규식 직접 호출 조사이며 AST/외부 바이너리/모든 간접 경로 부재의 증명으로 확대하지 않는다. CD 전체 동일성이나 새 Native 관측도 아니다.

실행 제품은791/11.01을 유지한다. 새 빌드·단위 검사·Native CPU/게임/GUI/MP·물리/PCM·지정 플레이/전체34/ZIP은0이다. 마지막791 집중17개 통과와 전체C++549의 과거7실패를 그대로 유지한다. 보호2724개/fixture904는 불변이며 다음 기준은 `extracted/cpp-resume-792/protected-after.json`이다. 전체 목표는 진행 중이고 다음은 checking잔여50과 나머지 비멀티 UI/Edit/수명/호환성이다.
