# 편집 파일 적재 검사 장치와 State 공개 경계 — 단위942

2026-11-06, HJOW-Athlon. LEFT_JOBS.md 3절2번의 객체 적재·파일/전환 실패 수명을 진행했다. 마지막 정밀 디컴파일은 같은 PC/2026-11-03이다. 제품939/11.01은 바꾸지 않았다.

## 알려진 검사 실패 수정

933/938에서 기록한 `FortObjectLoad_ActualFactoryOwnerHpPopAndContent`를 현재 라이브러리로 다시 빌드해 실패를 재현했다. 첫 원인은 검사 장면이 `SquidPop::BindTransmission`의 필수 자식 장치를 제공하지 않은 것이었다. 제품의 현재 `RunSquidActivate`는 서버 Pop9에서 이 장치를 호출하므로, 기존 검사에 **패킷 작성/통신 없이 SID·flags·호출 횟수만 관측하는 callback**을 제공했다.

그 뒤 두 번째 누락이 드러났다. 검사 TYPE6의 생성자 주소가0이라 현재 Factory는 실제 base 생성자를 실행했고, `!isForm()` 진단에서 중단했다. 이 입력은 실제 ContentForm 적재를 확인하는 검사이므로 TYPE6에 기존 보호 원문 생성자 표의 `TypeConstructorAddress(edition,6)`을 명시했다. 주소0을 Form으로 자동 대체하거나 진단을 무시하는 제품 변경을 하지 않았다. 609행×양판본의 원문 재생 장면은 그대로 유지하고 실제 통합 검사 장면만 고쳤다.

최종 양판본의 실제 Factory→owner/HP→Pop9→내용물 Form 부착·부모 알림·해시 검사가 통과했다. 기존 owner2·HP803·내용물2개·내용물 전송0·좌표 해시·Form 부모 기대값은 바꾸지 않았다. PATCH의 Pop 자식은1회/SID serverFirst/flags9, 기존 CD 독립 Pop은0회였다. 이는 로컬 자식 관측이며 MP 동작·실제 전송 검증이 아니다.

## 파일과 State1/State7 경계

기존 파일 검사의 현재 빌드로 기본 이름252행/늦은 파일 선택/실제 Session 설치3개, 비파싱 입력의 최종 파일 재조회·서명/객체 오류2개, UI 공개의 State 확정/실패/즉시 분기3개, Guide 반납/Config 통지 실패2개를 확인했다. FortObjectLoad5개와 합쳐 **고유15개·최종 실패0**, 최종 Release 빌드 경고/오류0이다. 전체 `netstorm_editor_default_name_tests`를 실행한 것은 아니다.

현재 코드도 다시 대조했다. `InstallExistingEditorCheckpoint`는 미션 요청 때 편집 World/HUD/loaded를 확정하지 않고 false를 반환한다. `UberEditor.inc`는 이를 `ReturnWithoutCommit`으로 전달하고 next7이면 UserInput을 만들지 않는다. 현재 Guide 요청은 같은 파일 namespace와 Subscriber를 보존하며 요청 성공 뒤 공유 owner를 교체한다. State7은 `RunCampaignMissionStarting`/공통144BYTE 확정 경로로 넘어가고, 로딩/접속 예외는 열려 있는 안내를 닫고 전파한다. 이번에 새 요청 패킷·연결·State7 실제 실행을 추가한 것은 아니며 기존713/714·660의 근거와 정적 연결을 구별한다.

기존 `CampaignSaveGoMain_ActualManagerGateAndSelectedBattleStop`은 일반 Battle/MP 이월 경로의 기대 불일치로 남기고 실행하지 않았다. 파일 검사5개와 다른 환경/장치 실패도 이번에 해결됐다고 쓰지 않는다. 공통 내부/Factory 전체 감사는3절4번에 있다.

보호2787개·fixture967개 전후 불변, 새 fixture0이다. 실제 게임 창·포획/희생·특정 적 공격·승리·사용자 직접 확인·신규 MP/패킷/통신 검사는 없다. 최초/중간 실패와 최종 로그는 Git 제외 `extracted/cpp-resume-942/`, [완료 JSON](../../cpppj/editor-load-test-devices-20261106.json)에 남겼다. 다음943도3절2번이며 남은 편집 도구/LeaveEditMode 연결의 범위를 정리한다.
