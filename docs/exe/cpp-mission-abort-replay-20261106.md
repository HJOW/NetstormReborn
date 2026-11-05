# 실제 Early1 중단·계속·재시작·메인 복귀 — 단위945

2026-11-06, HJOW-Athlon. AGENTS.md 실행 확인 면제 PC에서 cpppj 제품939/11.01을 실제 창 모드·무음으로 실행했다. 마지막 정밀 디컴파일은 같은 PC/2026-11-03이다. 게임 코드와 EXE는 바꾸지 않았다.

[재현 도구](../../tools/mcp_cpp_mission_abort_smoke.py)는 stdio MCP로 같은 앱 내부 입력 경로를 사용한다. 사용자 데이터는 실행별 `extracted/cpp-resume-945/live-v*/user-data/`로 격리하고 실행 자료 `assets/game-data/`는 읽기 전후 전체 파일 지문을 비교한다. 원본 게임을 실행한 것이 아니다.

## 최종 실제 실행

최종 `live-v7`의 **한 프로세스**에서 다음 흐름을 두 번 이어갔다.

1. 새 사용자 Welcome → Welcome2의 실제 OK/Main Menu로 시작 안내를 닫는다.
2. 원문 리소스 `tutorial1`(화면의 Early1)을 시작하고 표시된 MORE/OK 같은 시작 안내만 닫아 실제 월드·입력 가능 단계에 도달한다.
3. ESC → Game → Leave Mission → **Continue Mission**으로 같은 진행을 계속한다.
4. 같은 중단 창에서 **Replay Mission**을 선택하고 새 초기화/월드에 다시 진입한다.
5. 같은 중단 창의 **Main Menu**를 선택한다. 정상 Tips가 열리면 실제 OK로 닫고 미션/Session 보고가 사라진 메인 메뉴를 확인한다.

앱 입력37개·원문 보고26개, **새 진입2회+재시작2회=총4회 진입**이며 마지막은 phase0/page main·mission 없음·raw_session 없음이다. `netstorm_quit` 뒤 종료 코드0이다. 논리 PNG3개(800×600)를 저장했고 실제 미션/최종 메인을 육안 확인했다. 물리 키/마우스·창 DC 출력의 검증으로 세지 않는다.

최종 실행의 자료 지문은 전후 같고, 별도 보호2787개/fixture967개도 불변이다. EXE SHA-256은 `06030769a5f21dcdbe37cae9a1d40522be02f1e0b6a3246dc103f926eeea5098`다. C++ 재빌드/새 fixture는 없고 Python 문법 검사를 통과했다. 소리는 껐으므로 PCM/청취 증거는 없다.

## 최초 시도와 도구 교정

총 실제 게임 프로세스7개 중 최종1개가 전체 계획을 통과했다. 앞선 결과를 최종 통과 횟수에 합산하지 않는다.

- v1/v2: 표시명 `Early1`을 리소스 ID로 보냈다. 수정 명령의 Python 문법 오류 때문에 같은 입력이 한 번 더 실행된 것도 포함한다.
- v3: ID를 `tutorial1`로 고쳤지만 도구가 `currentLanguage=1`을 보내 `.1` 파일을 찾도록 했다. 원문 설정은 번호가 아닌 확장자 이름이므로 `currentLanguage=english`로 고쳤다. 제품의 파일 조회를 바꾸지 않았다.
- v4: 미션/계속/재시작/중단은 도달했지만 첫 사용자 Welcome을 즉시 main으로 기대해 도구 단언이 실패했다.
- v5/v6: Welcome2·정상 Tips까지 실제 버튼으로 닫는 흐름이 빠져 있었다. 현재 보고로 버튼 존재를 확인한 뒤 OK/Main Menu만 누르도록 보완했다. Tips를 끄거나 사용자 설정·미션 상태를 합성하지 않았다.

실패 프로세스는 종료됐거나 검사 도구가 자신이 실행한 프로세스만 종료했다. 최초 로그·actions·보고·성공 PNG/summary는 Git 제외 `extracted/cpp-resume-945/`에 남긴다. [완료 JSON](../../cpppj/mission-abort-replay-20261106.json)을 따른다.

## 범위와 다음

3절3번 첫 항목의 공통 교체/파일 namespace/참조/기한은 기존 원문286/831행·944의 실제 세대3회와 이번 실제 Retry/Continue/Abort 왕복을 연결해 정리한다. Next Tutorial/Next Mission의 이름·큐·공통 Begin/Fork 부모는 기존 고정 근거와 정적 연결을 유지한다. 자연 승리 뒤 버튼/진행 저장·전체 AI/다리 재고/전체 미션 조합의 동일성은 사용자 확인 범위로 남긴다. 포획/희생·특정 적 공격·시나리오 승리·공중3종 전용·MP/통신·전체34/ZIP/CTest는 실행하지 않았다.

다음946은 **3절3번 둘째 항목**, 데모 세 종류의 재시작 뒤 두 번째360초 만료·추가 주기/정리다. 그 뒤 같은3번의 Native3/State1/단독State7/LeaveBattle 잔여를 진행한다. 8번을 선행하지 않는다.
