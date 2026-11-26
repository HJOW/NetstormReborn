# 표시·Regular 오류 문맥의 파일 기록 — 단위1377

2026-11-27, HJOW-Athlon. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 현재 목표의 완료 감사를 진행했다. 마지막 디컴파일 PC도 HJOW-Athlon이며 원문/디컴파일 변경은 없다. D9의 새 ID 참가 범위 확인은 계속 대기하며 이 단위는 기존7차 상세 로그 요구를 보완한다.

## 실제 누락 근거

1375의 `restart-3/restarted/stderr.log`에는 Regular 실패 SID15001/TYPE89/event3가 있다. 그러나 같은 실행의 `bot/user/logs/20261127022933.log`에는 frameCheck 파일/행과 최종 예외만 있고 SID/TYPE/프레임 상세가 없다. 원인 조사에서 별도 stderr와 호스트 보고를 읽어야 했다. 제품이 이미 만든 `Frame assertion failed ...` 상세도 Client의 선택적 저장소에만 보관하고 공통 오류 파일에는 전달하지 않았다.

## 변경과 검증 경계

표시 assertion 어댑터는 현재 원문 상세 저장소를 파일에 먼저 기록한 뒤 기존 표현식/파일/행과 표준 진단을 처리한다. 원문 상세를 지우거나 게임 상태를 바꾸지 않는다. 선택 확인창에서 종료해도 이미 계산한 상세가 남도록 기록 순서를 정했다.

Regular 예외는 같은 풀의 현재 SID/TYPE/frame·event/count/payload와 PATCH raw42를 공통 오류 파일에 즉시 기록한다. raw42는 파생별 추가 필드이고 거주 구역에서는 기본 프레임이다. CD의36BYTE 슬롯에서는 프레임BYTE만 읽는다. 범위 밖 SID는 슬롯을 읽지 않고 풀 크기와 요청을 기록한다. 기록 실패는 원래 예외를 교체하지 않으며 기존 stderr도 유지한다.

추가 집중 검사는 실제 RawSurfaceStorage의 TYPE89/frame25 상세→공통 파일, 상세 부재, Regular 예외/풀 불변·범위 밖 SID, CD 프레임BYTE 범위를 검증한다. 게임 상태를 고쳐 실패를 합성하지 않고 창 없는 독립 원문 저장소를 사용한다.

## 결과와 사용법

Release 제품/집중 대상 빌드 종료0, 새4개와 기존 오류 로그14개를 합쳐18/18 통과했다. 실제 파일을 읽어 타입/프레임/사건 정보를 확인하며 원래 예외·풀 바이트·상세 저장소의 불변을 검증했다. 실제 한국어1280×720 창 모드의 튜토리얼1 F4 A→B 문구 전환과 종료0도 확인했다. 보호 원본/CD·fixture·AGENTS.md·README.md 2,810파일은1373~1377 전후 동일하다.

제품1377 EXE SHA-256은 `6851dfabaa7f6b8eb98eb4c8b312b9374d3287ae4ee845552d6fd9d78dc0846b`다. 게임 상태에 대한 결과를 바꾸지 않은 진단 연결이며1375의 기존 참가자 복구/집중130검사를 새로 반복한 것으로 세지 않는다. 최신 ZIP1376은 수정 전이므로 다음1378에서 갱신한다. [기계 판독 결과](../../cpppj/error-context-1377-20261127.json). 원시는 `extracted/build-error-context-1377.log`·`extracted/tests-error-context-1377.log`·`extracted/cpp-error-context-1377`이다.

새 검사 실행 파일은 아래처럼 사용한다. 창/게임/네트워크를 열지 않고 자기 임시 logs 경로에서 검사 후 정리한다. 원본·실제 사용자 저장소를 사용하지 않는다. `--list`로 사례 목록, `--filter ClientErrorContext`로 추가4개만 선택한다.

```powershell
cpppj/build/bin/Release/netstorm_error_context_tests.exe
cpppj/build/bin/Release/netstorm_error_context_tests.exe --filter ClientErrorContext
```
