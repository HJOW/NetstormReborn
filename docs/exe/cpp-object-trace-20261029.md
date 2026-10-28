# 객체 trace의 모든 Form 재귀 복원 — 단위554

2026-10-29 / HJOW-Athlon. 사용자의 새 `/goal`로553의 일시 정지를 해제하고 진행했다. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽었으며 마지막 완료된 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 현재 PC와 같다. 디컴파일 산출물과 보호 원본은 변경하지 않았다.

## 복원한 범위

`RawObjectTrace`는10.78의 `0044dab0`을 실제50BYTE 풀에서 실행한다. SID0은 아무 장치 없이 복귀하고 free는 `sid %d is free\n` 한 줄만 요청한다. 나머지는 현재 객체의 가상+0x70→깊이 DWORD+3→로그 들여쓰기 setter→모든 Form 재귀→현재 깊이-3→setter→자식이 있으면 `~1~yEND %04d\n` 순서다. 외부 효과 뒤 예전 깊이를 복원하지 않으며 DWORD 감김도 보존한다.

여기서 모든 Form 탐색기는 `004b22d0/004b1b80`이며 **TYPE0..69**가 대상이다. ContentForm(TYPE6)만 찾는 기존 `RawContainedFinder`와 다르다. 후보의 next를 반환 전에 저장하므로 재귀 호출이 현재 형제 링크를 바꾸어도 이미 저장한 next는 유지한다. 가상 trace 뒤 부모 head·각 후보의 현재 TYPE와 논리 끝 번호는 실제 소비 시점에 읽는다. free/dead/void/contained 후보를 추가로 제외하지 않는다. dead 부모 assert는 원본 checking/metamap/boss 관문에서만 요청한다.

호스트의 물리 풀 밖 접근·재귀/형제 고리는 명시 오류로 거부한다. 원문은 손상 체인에서 범위 밖 접근 또는 끝없는 재귀에 빠질 수 있다. 이 호스트 경계에서 앞서 발생한 출력/깊이 변경을 되돌리지 않으며, 정상 후보를 전역 방문 집합으로 생략하지 않는다.

**가상+0x70의 객체별 서식·실제 로그 장치와 Ctrl Shift F7은 아직 연결하지 않았다.** 이 부품을 간단한 SID 목록으로 대신 연결하지 않았다. 실제 게임은552 제품 그대로이며 새 게임/원본/네트워크 실행은0이다. 553의 프로세스 요약 부품과 이번 재귀 부품은 실제 진단 연결의 선행 작업이다.

## 원문과 검사

`tools/decomp_object_trace.py`는 실제 PE의 `0044dab0/004b22d0/004b1b80/004c2600/0044daa0` 몸체를 실행한다. 가상+0x70·로그·assert만 외부 장치로 관측하고, 재귀/필터/링크/깊이 setter는 기계어를 실행한다. 원문 함수 바이트 불변·정상 반환/ESP·x87 제어 워드/TOP·예외 목록 복구를 확인했다. **103고유 입력×두 정밀도=206관측, 차이0**이다.

고정 fixture는 SID0/free/단일/다단, TYPE69/70 경계·TYPE0, 현재 논리 끝·물리 풀의 구별, DWORD 감김, dead assert와 권한/메타맵 예외, 가상 호출 중 head/next/TYPE/상태/끝 번호/깊이 변경,80개 고정seed 유한 트리를 포함한다. C++ 신규 검사2개의 모든 원문 행이 일치했다. 별도 호스트 오류 검사는 장치 누락·SID0·손상 체인/범위에서 앞선 효과 보존을 확인했다.

최초 `test-object-trace-v1.log`의 실패1개는 호스트 고리 검사 입력의 루트TYPE82가 Form 필터에서 제외된 탓이다. 원본 필터에 맞게 테스트 루트만TYPE69로 바꿨으며 제품/원문 기대값은 변경하지 않았다. 최종 신규2+ProcessSummary2+Kernel5+Contained15의 **고유집중24개/실패0**, 등록3402, Release `netstorm_tests` 빌드 경고/오류0이다. 전체 C++/CTest와 게임 실행은 하지 않았다. 549의 자동 전술 실패/보류를 해제하지 않는다.

보고서는 `cpppj/object-trace-20261029.json`, 원시는 `extracted/cpp-object-trace-554-20261029/`의 `before.json`, `native.json`, `build-v1.log`, `build-v2.log`, `test-object-trace-v1.log`, `test-object-trace-v2.log`, `test-summary.log`, `test-kernel.log`, `test-contained.log`다. 보호1820파일·기존fixture713개(합계2533)의 바이트 보존과 게임552 동일성을 보고서에 기록했다.

## 이어갈 작업

1. 가상trace의 base `004ac400`, 자산 `004b0fb0`, Dude `00450d60` 및 내장 Form별 본체/현재 파생 필드를 독립 대조·복원한다. 선택 진단의 현재 간단한 로그를 전체 trace 완료로 세지 않는다.
2. 객체 덤프 `004d6020`의 현재 로그 전역/모드·슬롯 순회·BaseFile 경로·파일 후처리와 정상/실패 복귀 순서를 복원한다. 파일 열기 실패 때 원본 모드/tracelevel이 그대로 남는 경로를 임의 정상화하지 않는다.
3. 553의 요약을 실제 Kernel Form/부모에 연결한다. ThunderProcess(TYPE55)의 실제 등록/부착과 검사 전용 UiKernelTell을 먼저 구별한다.
4. Guide/Gump 뒤, console/정지 앞의 Ctrl Shift F7에서 `004437f0` 빈RET→객체 덤프→프로세스 요약을 원문 순서로 호출하고 짧은 실제 입력을 검증한다.

전체 목표는 진행 중이며 싱글플레이/UI/Edit·상태 수명·Win11 실기·전체 동일성은 미완료다. 자연 완주/사용자 개입·스펠 도움말 그림·연녹색 직사각형·언어파일 부재의 보류와 공중3종 정적 제한·MP 신규 검사 금지는 유지한다. 원격 push는 하지 않는다.
