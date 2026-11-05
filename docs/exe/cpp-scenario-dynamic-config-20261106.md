# 동적 시나리오·TARC·HTML 설정 쓰기 대조 — 단위940

2026-11-06, HJOW-Athlon. 사용자 지시에 따라 LEFT_JOBS.md 3절을 앞부터 진행한다. 1번 UI는929에서 완료했으므로 이번 단위는 **2번의 첫 미완료 항목**이다. 4~8차를 우선하던 이전 판단은 철회했다. 마지막 전체 정밀 디컴파일도 HJOW-Athlon/2026-11-03이므로 같은 산출물을 읽었다.

## 원문과 현재 연결

- `004cf730`은 지역 `param`을 등록하고 이름/등록 객체/현재 미션/임시 tell을 순서대로 조회한다. 임시 tell은 본문 조회 뒤 반납하고 param은 HTML 생성 뒤 반납한다. 현재 `ShowTellInConfig`/`FindTellSectionInConfig`가 같은 순서다.
- `00440760`의 `{@미션.키}`는 현재 `missionSpec`으로 임시 Config를 읽고 키를 구한 뒤 반납한다. `ConfigInterface::Registry().loadMission`은 현재 Client 파일 로더를 사용한다. 일반 파일·TARC의 현재 파일 수명은771에서 복원한 경로다.
- `004cf280`은 본문 전체 치환 → 첫 HTML → 메타 작성 → 두 번째 HTML 순서다. `<$Config,...>`는 첫 HTML에서 `004c9090`/실제 입력 큐에 들어가며 즉시 설정을 쓰지 않는다. 메타의 명령 인자는 작성 시점의 치환 결과를 보관한다.
- `UberGump::LoadingHtmlContext`/`QueueHtmlScriptCommand`는 실제 `DialogScriptCode`와 같은80칸 문자열 고리/40칸 큐를 사용한다. 일반 Config 입력은 현재 고리의 `event.y` 문자열을 `ApplyScriptConfig` → `ConfigInterface::Set`에 전달한다. 새 별칭이나 별도의 명령 번호표를 추가하지 않았다.

10.72 대응은 정밀 함수 표의 `004c3a20`(Tell), `004c3950`(param), `0042a6b0`(동적 Config), `004c3de0`(태그 명령)이며 이번에 CD 실행이나 전체 본문 재검사를 한 것은 아니다.

## 보호 데이터의 정적 조사

[조사 도구](../../tools/cpp_scenario_dynamic_inventory.py)는 기존687/715 조사에 추가해 TARC50개와 느슨한 영문499개, 총549문서의 치환식 포함 명령/인자 위치를 읽었다. **980곳, 대소문자 통합22명령**이다. 이 중 비멀티11명령967곳은 Config461·MissionBegin439·Tell46·Warning9·DoNothing3·ScreenModeAbsChange3·Salvage2·GetTechnology/SendAllBucks/StartDemo/URL 각1이다. MP/관리10명령11곳은 3절8번으로 유지하고, 원본 오자 `MissinBegin`2곳은 별칭으로 고치지 않는다.

이549문서에는 명령 이름 자체에 `{...}`가 있는 명시 템플릿은 없었다. 이 결과는 사용자 파일/설정에서 동적 이름을 만들 수 없다는 뜻이 아니다. 템플릿 조사 문법은 섹션 경계를 넘겨 오자 문장을 합치지 않으며 실제 Config 확장/조건/실행 결과를 대신하지 않는다. 출처 대표8곳·전체 위치 해시·원문 manifest 해시는 [JSON](../../cpppj/scenario-dynamic-inventory-20261106.json)에 있다. 첫 전체 위치 원시는 Git 제외 `extracted/cpp-resume-940/inventory-full-first.json`에 보관했다.

## 집중 검증 — 완료

`HtmlWindowFactoryClientTests.cpp`에 두 연결 검사를 추가했다. 첫 검사는 새 UTF-8 작업 폴더의 암호화 TARC → 같은 경로의 느슨한 파일 → 삭제 뒤 TARC 복귀를 실제 Client Config 로더로 처리한다. `{@control.inline}`/`{@control.menu}`를 실제 HTML 명령 조회기로 해석하고 지역 param이 사라진 뒤에도 인라인 명령/실제 메뉴 클릭이 보관한 인자를 큐에서 읽어 설정에 쓴다. HTML 부모/메뉴/모달의 공통 반납도 확인한다.

둘째 검사는 동적 파일 읽기에서 명시 예외를 발생시킨 뒤 임시 Config/param과 부분 HTML 부모를 반납하고 재생성하는 경로다. 이 예외 장치는 실제 디스크 부분 I/O·OS 메모리 고갈의 재현은 아니다.

최종 Release 집중 빌드는 경고/오류0이다. 실제 HTML 공장17개(새2개), PE43개 이름/메뉴 큐2개, 대본262행/기존 큐2개, 현재 Config 파일 읽기7개, **고유28개 검사·실패0**이다. 통합 검사 전체를 실행한 것은 아니다.

첫 빌드는 새 검사의 `size_t`/`uint32_t`를 한 auto 선언에 묶어 C3538 두 건이 발생했고 선언을 분리했다. 첫 실행은 Client를 두 개 만든 검사 구성과 원문 Config가 대문자로 정규화한 `d/CONTROL.english`를 소문자로 기대한 검사 입력에서 각각 실패했다. 실제 경로/등록부 반납 상태를 먼저 기록한 뒤 단일 Client 구성과 원문 대문자 기대값으로 고쳤다. 제품 코드·기존 fixture·기대 동작은 바꾸지 않았다. 빌드 v2~v4, 실행 v1/v2/최종 로그는 Git 제외 `extracted/cpp-resume-940/`에 보존했다.

기존 보호2787개(원본/CD/AGENTS/fixture967개)는 전후 경로·내용 불변이며 새 fixture는0개다. 제품939/11.01 EXE SHA-256도 `06030769a5f21dcdbe37cae9a1d40522be02f1e0b6a3246dc103f926eeea5098`로 동일하다. 실제 게임·멀티플레이·통신·사제 포획/희생·특정 적 공격·승리·사용자 확인 테스트는 실행하지 않았다.

## 완료 판정과 다음 순서

3절2번 첫 항목의 남은 비멀티 시나리오 명령은 기존687/715의 고정 이름·명령 부모 근거와 이번 현재 파일/동적 치환/UI writer 연결 대조로 완료 처리한다. 원본 오자는 보존하고, MP/관리 명령은3절8번에 남긴다. 임의 사용자 스크립트의 모든 조합·모든 동적 실행이나 자연 완주의 증명으로 확대하지 않는다. 새 구체적인 누락이 확인되면 해당 항목을 다시 연다.

새 검사는 실제 Client 파일/Config 로더와 실제 HTML 공장/메뉴/입력 큐/설정 쓰기 본체를 연결하지만, 전체 UberGump 사건 루프나 OS 물리 입력을 실행한 것은 아니다. 다음941은 **3절2번 둘째 항목**의 지형·객체·소유자 도구와 저장/재파싱·State1/State7 실패 수명이다. 8차 UI는 선행하지 않는다.
