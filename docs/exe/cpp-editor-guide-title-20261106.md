# 편집 세션의 현재 Guide 조회와 Test Battle 제목 — 단위950

실제 State1 편집 세션의 데모 조회 누락과 Test Battle 제목의 모드 전달 누락을 수정했다. 제품949 대조에서는 편집 진입 후 ESC에서 미연결 getter로 종료1을 재현했다. 새950은 **Edit→TEST01→Test Battle→Return to Editing→Main Menu/No→종료0**을 통과했고, 실제 Win32 제목도 `EDITOR:TEST01 → TEST:TEST01 → EDITOR:TEST01 → MAIN`으로 확인했다. [완료 JSON](../../cpppj/editor-guide-title-20261106.json)을 따른다.

2026-11-06, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 PC/2026-11-03이다. 실행 확인 면제 호스트에서 실제 클론 창·무음·격리 사용자 프로필을 사용했다. 원본/CD·AGENTS.md·기존 fixture·디컴파일 원시는 수정하지 않았다.

## 두 실제 연결

- [State1 설치](../../cpppj/src/client/UberEditor.inc)는 `missionOutcome.demo`를 연결하지 않아 Session의 `FillMissing` 장치가 남았다. 실제 ESC의 커서/입력 경로가 이를 호출하면 `RawCampaignSession.cpp:1585`의 미연결 오류가 발생했다. 미션 설치와 같은 현재 Runtime getter를 연결했다. 원문 `00463b50`처럼 미션이 없으면0이고, 이후 미션이 있으면 현재 `+68`을 읽는다. `missionPost`나 생성 시점의 상수를 데모 여부로 대신하지 않는다.
- [제목 갱신](../../cpppj/src/client/UberGump.cpp)은 현재 데모 `+68`만 넘기고 `WindowTitleState.testBattle`을 기본false로 남겼다. 현재 `mode64`도 전달한다. 원문 `00460730`의 Editor→일반 요새→데모/시험/일반 미션 우선순위와 `004639e0(+64)`/`00463a20(+68)`을 유지한다. Test Battle의 모드는 실제 공장 반환 뒤1로 쓰이는 기존 순서를 바꾸지 않는다.

원문 State3는 이름 첫 BYTE를 지운 뒤 같은 제목 함수를 호출하고 현재 상태를 재검사한다. 그 특이 continuation은 여전히 도달 미입증/미연결이며 이번 수정으로 완료 처리하지 않는다. 제목 함수의 직접 접근과 실제 State1/Test Battle 소비자를 추적하다 찾은 두 누락이다.

## 실제 입력과 실패 보존

[MCP 도구](../../tools/mcp_cpp_editor_title_smoke.py)는 실제 Edit·목록 항목·Game 메뉴·Go!/Return to Editing·LeaveEditMode의 No를 사용한다. Win32 `GetWindowTextW`는 실행한 PID의 보이는 최상위 창만 읽고 활성화/OS 입력은 하지 않는다. 제목 설정에 서로 다른 표식을 넣어 실제 선택 키를 구별하며, 게임 시계/Player/AI/승패 값은 바꾸지 않는다.

TEST01 원문 Fort를 검사 사본의 `d/title950/`에 바이트 그대로 추가하고 그 단일 FileSpec과 정상 `mayEditMissions=1`을 사용했다. 원문 english는 바꾸지 않았다. 편집 결과는 별도 사용자 작업 루트에 쓰며 실행 자료1395파일은 불변이다.

| 실행 | 결과 |
| --- | --- |
| control-v1 / 제품949 | 일반 목록 설정에서 TEST01을 찾지 못해 진입 전 입력 오류. 도구가 Quit로 종료0. 기능 오류의 재현 횟수에 합산하지 않음 |
| control-v2 / 제품949 | 단일 FileSpec·편집 허용 설정. 실제 `EDITOR:TEST01`·편집 모드/Session 진입 뒤 ESC에서 Guide 데모 getter 미연결로 종료1 |
| fixed-v1 / 제품950 | 같은 적재 도구/입력으로 전체 왕복 통과. 실제 OS 제목·모드·Session/최종 반납·자료 불변·종료0 |

수정 전 제품은 실행 파일/DLL을 고정하고 전후 자료/사용자 루트를 분리했다. 전후 유효 비교 실행(control-v2/fixed-v1)의 적재 도구 SHA는 같다. 구제품은 Test Battle 제목까지 도달하지 못했으므로 구제품의 잘못된 시험 제목을 실제로 관측했다고 주장하지 않는다. 그 누락은 현재 코드/원문 분기와 기존 제목 fixture로 대조했고 수정 제품에서 실제 선택을 확인했다.

최종 관측8개·앱 입력13개·논리1024×768 PNG4개다. 시험 전투/복귀 편집/최종 메인을 육안 확인했다. 최초 도구는 `main-0`/`editor-0` 파일명을 재사용해 원문 TSV6개가 남았으며, 선택 필드/실제 OS 제목8관측은 summary에 모두 보존했다. 실행 뒤 도구의 보고 파일명에 고유 인덱스를 넣었다. 이 출력 보존 변경 때문에 실행을 반복하거나 성공 조건을 바꾸지 않았고 적재/현재 도구 SHA를 구별했다.

## 빌드와 범위

게임/제품 UI 집중 대상2개의 Release 빌드는 종료0·컴파일 경고/오류0이다. 기존 `WindowTitle_`3검사(PATCH/CD 각576행과 실제 설정 치환)가 실패0이다. 새 C++ fixture/검사는 추가하지 않고 실제 바인딩 회귀를 MCP 도구로 확인했다.

제품950/11.01·11,248,128BYTE·EXE SHA-256은 `9ac4a32bd849b6c6f464fb18a08e9f217c4102407193a7cd53f1915ed14da577`이다. 보호2787개/fixture967개가 불변이다. 원시는 Git 제외 `extracted/cpp-resume-950/`에 보존한다.

3절3번 셋째 항목의 일부 연결을 완료했다. Native3의 다른 선택·State1/단독State7·전체 cleanup Tick/연속 전환·LeaveBattle 다른 이유/CD·State6은951부터 계속한다. 일반 전투 저장/복구·존재 저장 reader와 MP 부모는 기존8번 잔여다. 실제 포획/희생·특정 적 공격·승리·공중3종 전용·MP/통신·물리 입력/PCM/CD·전체 미션/CTest/ZIP은 실행하지 않았다.
