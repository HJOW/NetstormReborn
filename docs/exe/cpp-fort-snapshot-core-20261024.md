# 전체 저장과 Editor 기반의 공용 등록

2026-10-24, 단위381. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 마지막 수행 2026-10-23**이다. 기존 원본 실행 파일과 정밀 디컴파일 산출물을 읽어 준비한 독립 모듈을 공용 빌드에 등록한다. 원본/CD와 AGENTS.md는 변경하지 않는다.

## 이번 변경

- [전체 Snapshot 부모](cpp-fort-snapshot-parent-20261023.md)는 Fort와 Battle의 순서 및 현재 선택 재조회를 보존한다. 전체 부모가 Technology·Money·Mission 섹션을 생성하며, 실제 Session 연결은 별도 작업이다.
- [Money](cpp-fort-money-save-20261023.md)는 현재 SP 대신 같은 게임 난수와 float32 계산을 사용한다. [Mission](cpp-fort-mission-save-20261024.md)은 실제 Guide의 현재 미션을 두 번 읽고 이름·sequence·flags를 저장한다.
- [Badges 저장](cpp-fort-badges-save-20261024.md)과 [단일 장부](cpp-fort-badges-globals-20261024.md)는 같은 28×220BYTE 기록을 사용한다. 원본 CRT 초기화와 count만 지우는 reset을 구별한다.
- [Editor Players](cpp-editor-players-20261024.md)는 원본의 실제 Player 생성·필드 쓰기·타입 순회와 필수 후처리 경계를 보존한다. [현재 Fort 이름](cpp-fort-current-name-20261024.md)은 Snapshot·파일명·Notify·이전 경로 삭제 요청·제목·설정 순서를 보존한다. 파일 삭제는 이 모듈의 요청 경계이며 실제 파일 조작이 아니다.
- [Notify 최소 교정](cpp-fort-notify-next-20261024.md)을 실제 `RawPlayerTemplateState::Changed`에 적용한다. 첫 manager가 없으면 원본 첫 BYTE의 물리 접근 경계에서 거부하고, 정상 빈 C 문자열은 허용한다. 이후 진단·현재값 재조회·저장 관문은 그대로다.

일곱 모듈과 여덟 검사 파일을 각각 `src/o/CMakeLists.txt`와 `tests/CMakeLists.txt`에 등록한다. 각 검사는 독립 원문 TSV를 별도의 컴파일 정의로 받는다. 최초 native 보고서의 ‘제품 미작성/미등록’ 상태와 고정 SHA는 당시 관측을 나타내므로 덮어쓰지 않는다. 현재 통합 상태는 이 문서와 LEFT_JOBS.md에 기록한다.

## 검증 기록

- 원문 도구 여덟 개의 `--verify`가 모두 종료0이다. 최초 실행 파일·디컴파일·도구·fixture SHA가 일치한다. 현재 제품에서 만든 출력을 원문 기대값으로 사용하지 않는다.
- 공용 Release 제품과 검사 빌드는 모두 종료0이며 관측 경고·오류0이다. 신규18검사와 기존 PlayerRegular3·EditorFiles5·TemplateSerializationGlobals4·실제 두 Terr/Chaff 왕복1·동적 저장소3을 합쳐 관련 **34검사/실패check0**을 통과했다.
- 최초 동적 저장소 필터를 잘못 `CampaignTemplateDynamic`으로 지정해 ‘대상 없음’으로 종료1이었다. 원시 로그를 보존하고 실제 등록 이름인 `CampaignTemplates_Dynamic`으로 다시 실행해 3검사 모두 통과했다. 제품 실패나 기대값 변경은 없다.
- 제품 SHA256은 `be150cdccc7b8a0aaa8bec93d3c8198100ce3d934ef9757e08e6b58b2aebc45e`, 검사 SHA256은 `4ba58a5bbea707b6800b6efd1ae4ad5065f5adae5fe0348c2f7104eb3cf60ec3`이다. 원시 로그는 `extracted/cpp-fort-snapshot-core-20261024-*`로 보존한다.
- 최종 전체 CTest는 **2464검사/실패check0·종료0**, 324.10초(전체324.14초)다. CTest 외부 결과와 보존한 내부 LastTest 로그의 실제 검사 수를 모두 확인했다. 이번 변경은 새 독립 몸체의 등록과 Notify 첫 BYTE 처리다. 실제 파일/Notify·현재 저장소 회귀와 전체 검사를 수행했고, 이 단위에서 GUI 전체 진입을 반복하지 않았다.
- 원본 1394파일을 단위379 실행 후 전체 manifest와 다시 비교해 추가·누락·SHA 변경0을 확인했다. `extracted/cpp-fort-snapshot-core-20261024-originals-check.json`에 결과를 보존한다. 엄격 UTF-8·Python AST·JSON과 문서 링크를 확인했다. TSV의 마지막 빈 trace 열은 원문 자료이므로 그대로 유지한다.

## 남은 연결

전체 Session Snapshot, 실제 Client 수명의 Badges 장부 및 readOnly 설정, 작업 디렉터리의 파일명/목록/읽기/저장/삭제, generic Editor 설치와 메뉴는 아직 연결되지 않았다. [Session 연결 설계](cpp-fort-snapshot-session-bindings-20261024.md), [Client 파일 저장 전역](cpp-fort-file-save-globals-20261024.md), [Badges 수명](cpp-badges-client-next-20261024.md)을 후속 근거로 사용한다. Battle CoreData와 Editor 후처리의 미연결 경계를 빈 성공으로 채우지 않는다.

이번 단위의 독립 원문 통과를 전체 Editor 작동, 캠페인 전체 완주 또는 UI·음향 동일성 완료로 확대하지 않는다. 이전 단위379의 같은 제품 34개 진입과 Early1 자연 완주는 해당 제품에 대한 기존 관측으로 유지한다.
