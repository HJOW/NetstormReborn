# 단위653 — Edit 기본 요새 이름과 선택 뒤 파일 준비

2026-10-31, HJOW-Athlon(192.168.0.94). 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23 자료다. 새 디컴파일/원문 수정은 없다. 보고서는 [editor-default-name-20261031.json](../../cpppj/editor-default-name-20261031.json)이다.

## 수정한 실제 누락

State1 설치에서 현재 이름이 비었거나 점으로 시작하면 원본 `004b733b..004b7350`은 `00460c20`을 호출한 뒤 파일 설치를 계속한다. 이 함수는 `004410f0("newFort",0,0,0)`의 현재 결과를 전체 이름 부모 `00460980`에 전달한다. 복원판은 해당 조건에서 `Outside(DefaultName)`으로 중단했다.

현재는 `ExistingEditorFortHooks::defaultName`을 원문의 호출 위치에서 요구한다. 실제 Uber는 공통 `SelectConfiguredNewFort`와 기존 `SelectCurrentFortName`/Template/작업 파일 부모를 사용한다. 기본 이름 결과가 다시 비거나 점으로 시작해도 재검사/반복/임의 이름 생성은 추가하지 않았다. 이전 한정 관측에 사용한 `DefaultName` 경계 식별자는 기존 검사 장치에 보존했으며 실제 기본 선택의 미복원 표시로 사용하지 않는다.

기본 이름 선택 뒤 파일 준비도 필요했다. 앞선 호스트 준비는 pending 이름의 경로를 사용하지만, 기본 선택과 설정 알림은 그 뒤 실제 이름을 바꿀 수 있다. checkpoint의 path 장치는 이제 `PrepareCurrent()`로 최종 선택된 파일을 같은 이관 장부/보호 작업 namespace에 준비한 다음 존재 여부를 검사한다. 원문/CD에 쓰거나 삭제하지 않으며, 완료된 부재/삭제 항목의 원본 재등장은 기존 장부 정책이 차단한다.

CD10.72의 `004ca8e0`은 실제로 `004a9460("newFort",0,0,0)`→`004ca6a0`이다. 이는 정적 대응 확인이며 CD 실행/전체 복원 근거가 아니다.

## 확인 범위

`tools/decomp_editor_default_name.py`는 실제 이름 앞단 `004b72eb..004b7350`과 기본 선택 `00460c20`을 실행했다. 문자열 비교/Config/Guide/전체 이름 부모는 명시 경계이고 파일 설치 직전에 멈춘다. 전체 `00460980`의 새 Native 검증으로 세지 않는다.

빈 이름/점 이름/일반 이름·대소문자 동등, 이름 변경/Guide 해제, 기본 설정의 일반/빈/점 문자열과 첫 선택 중 현재 이름 변이를252입력으로 대조했다. 기본 선택이 한 번만 수행되는지와 뒤 현재 문자열/미션 값/전체 호출 인자를 동결했다. 독립2프로세스에서 각504관측, 총1008관측/차이0이며 TSV/JSONL이 모두 동일하다.

`netstorm_editor_default_name_tests`는68등록 중 **고유7개/실패0**이었다.

- 새3개: 원문252행의 선택 순서, 실제 Unicode 작업 경로의 늦은 원문 이관/읽기, 실제 Session의 기본 선택→기존 파일/월드 checkpoint 연속 설치.
- 기존4개: 외곽 fixture258행 중148행의 제한 투영/110행 명시 제외와 필수 경계, 전체 이름 부모876행/물리 경계.

실제 파일 검사는 새 `extracted/cpp-editor-default-name-653-disk/한글-😀/` 아래에서 보호 TAFF의 `thewarbegins.fort` 바이트와 작업본을 비교했다. 실제 Session 검사는 기존 독립 설치 장치에서 `Checkpoint385` 기본 선택 뒤 같은 파일/월드까지 진행했다. 이름 setter의 새 GUI 검증이나 전체 State 공통 확정/최종 loaded의 새 검증으로 확대하지 않는다. 같은 대상에 링크한 다른 Session/Editor 큐/MP 검사는 실행하지 않았다.

최초 Release 빌드와 집중 검사부터 통과했다. 빌드 종료0·경고0·오류0이다. 보호 기준645의 관련7입력(AGENTS/두 PE/TAFF/기존fixture3개)을 재해시해 일치를 확인했다. 새fixture1개는 별도이며 전체2632개/기존773개 재감사가 아니다. 원시 근거는 이 PC의 `extracted/cpp-editor-default-name-653-*`에 있다.

제품 EXE SHA-256은 `be02ccaca59aa09e45c107159e1e650122c5f0d7b87b1dbe74e825f43a640077`다. 게임 실행/새ZIP/전체34 통합/주 검사 EXE 재빌드는 없다. 마지막 전체34/ZIP은645의644제품이다. 스테이지 진입·대기는 더 많은 기능 묶음 뒤에 통합한다.

## State7 정적 점검과 잔여

현행 `LoadMissionWorld`는 이미 `CleanupNativeStateWorld(1)`을 호출한다. 오래된 주석만으로 전체 cleanup 미연결이라고 되돌리지 않는다. 다만 Session `Login::Begin`은 수동 Zone 설치 뒤 상태8에서 Run/Poll/Pump하고 phase14/loaded를 동기 요구한다. 전체 비동기 State7 연결은 여전히 잔여다.

원문의 State7은 `00440c60` 결과를005c85b8에 기록한다. 이 함수는 CD 위치/표식 검색이고 난수 getter가 아니다. 현재 `seed`/`RandomSeed` 명칭만 보고 난수 장치에 연결하면 안 된다. `004993c0`도 Renderer 초기화이므로 `refreshCursor`라는 이전 경계 이름만으로 소프트웨어 커서 갱신에 연결하지 않는다. 이번에는 이 큰 부모의 실행/동작을 변경하지 않았다.

다른 State1/State7·이탈/미션 수명, UI/Edit/시나리오/호환성, BaseFile 다른 소비자/ABI와 이후 MP 무시험 사전 구현·4~7차를 계속한다. 사제 포획/희생·특정 적 공격·승리 재현, 공중3종 전용 행동과 신규 MP 검사는 하지 않았다. 자연 완주/전체 동일성은 사용자 확인 대상이다.
