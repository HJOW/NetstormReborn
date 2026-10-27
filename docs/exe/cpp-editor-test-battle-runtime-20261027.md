# Edit Test Battle의 실제 입력과 편집 복귀 장치 연결

단위487, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이다.

원문004d81d0의 c2 부모를 일반 입력에 연결했다. 시작은 현재 Editor Session의 실제 전체 snapshot 뒤 같은 작업 namespace의 Guide를 생성한다. 종료는 Guide Close 뒤 현재 이름을 pending에 복사하고, 모드2와 RequestFortEdit를 원문 순서로 실행한다. Battle14 요청은 실제 현재 파일 로더, 현재 Template State, 게임 시각, 현재 소유 섬 개수와 Notify를 사용한다. 각 callback 사이의 현재 owner/State 재조회는 기존 원문 부모가 담당한다. 다음 State1의 공통 정리와 기존 편집 설치를 앞당기지 않는다.

Test Battle Session의 Notify도 기존 BindFortFileSave를 통해 형제 편집 작업본에 연결했다. 소유자가 없는 섬 분기의 저장을 빈 성공 함수로 대체하지 않는다. 논리 이름과 실제 파일 이름 및 source 수명은 직전486의 연결을 그대로 사용한다. `.english` 부재 시 종료되는 원본 문제 개선은 사용자 지시대로 이번에 추가하지 않았다.

Release build2는 경고/오류0이다. 첫 빌드의 잘못된 API 이름은 실제 기존 BindFortFileSave로 고쳤으며 원시 실패 로그를 보존했다. 집중67고유/67관측·실패0, 등록3,276개와 기존 검사/fixture 전체를 유지했다. 새 동작을 그대로 흉내 낸 검사는 추가하지 않았다. 직전486의 전체3,276/0 뒤 이번은 관련 원문 부모와 Session·파일 수명 검사 및 실제 실행으로 확인했다.

같은 최종 게임 `b241bc7b…`의 실제 War 진입→짧은 진행→Restart→브리핑→짧은 진행은500프레임/약14.33초 종료0이다. 실제 전체 맵 목록은 종료0이나 TEST01이 원문50칸 상한 뒤에 가려졌다. 해당 구성을 바꾸어 성공으로 보고하지 않았으며, 첫 TEST01 선택 실패를 보존했다. 별도 새 복사본에서 다른 느슨한 fort를 복사 대상에서 제외하고 TEST01 fort/english를 그대로 사용한 실행은 Editor→Test Battle→로컬 로그인/State14까지 진행했다. 이어 **총 애니메이션 미복원 TYPE129 stage5**에서 종료1이므로 실제 편집 왕복을 완료했다고 보지 않는다. 다음 단위에서 이 동작을 원본과 대조해 복원한 뒤 왕복 검증을 계속한다.

원본/CD/기존 검사3,196파일과 AGENTS.md, 최종 소스/검사2,848파일을 감사했다. 실제 실행 전후 originals 전체 해시가 같다. 스테이지 클리어와 사용자 개입 검증은 하지 않았다. Windows 10/11·전체화면 호환성은 현재 목표로 남아 있다.

상세 해시는 [보고서](../../cpppj/editor-test-battle-runtime-20261027.json), 최초 실패부터의 원시 기록과 입력 계획은 `extracted/cpp-editor-test-battle-487/` 및 같은 접두 Python 스크립트에 있다. 원격 push는 하지 않는다.
