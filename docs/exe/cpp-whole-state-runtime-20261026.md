# 실제 Client의 전체 State 정리 부모 연결 — 단위463

2026-10-26 **HJOW-Athlon**에서 원본10.78을 기준으로 작업했다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 산출물을 변경하지 않았다.

## 실행 경로

`CleanupClientStateWorld`가 기존 원문 부모 `CleanupStateWorld(004b2df0)`를 실제 Client 장기 전역·연결표·SP·시계·음향·메타구역·선택·Template·Player·Pool·표·Graph에 연결한다. 직접 store와 현재 값 재조회는 기존 `BindClientStateWorldGlobals`와 부모를 재사용한다.34Effect는 실제 하위에 분배하며 UI Kill/Cleanup/Replace와 현재 SP 표시는 같은 공통 Gump 장치를 받는다.

owner1..8의 AI, Player 링크, 단일 Kernel Clear는462의 `ClientStateRuntimeRetirement`에 연결했다. 실제 Core0 선택과 두 직접 store 뒤, 실제 Pool reset 앞에서 모든 Runtime 고정뷰를 반납한다. 그 뒤 같은 저장소의 reset과 실제 Player 재초기화를 진행한다. `MapMode`는 보호 PE의 C3 단일 RET이며, 미복원 하위를 빈 성공으로 대체하는 분기가 아니다.

`UberGump::CleanupNativeStateWorld`가 이 부모를 실제 UI·Client에 연결한다. 별도 `RetireWorldForState`는 이전 Runtime의 메뉴/Form·입력·Guide·그리기 참조를 해제하고 같은 Session을 소멸시킨다. 일반 `ClearWorld`의 기한·게임 정지·게임 전역 재초기화를 반복하지 않고, 이미 만든 새 HUD도 지우지 않는다.

`SetPlayerSpAmount`는 기존 `RawFortMoneyLoad::SetPlayerSp(00447810)`의 지정 본문을 공통화했다. 전체 State도 같은 현재 known/owner/Editor getter와 장기 분산 SP를 사용한다. 새 임시 World·가짜 Player 장부·별도 난수로 SP 정리를 대신하지 않는다. 기존 Fort Money는 같은 함수로 위임하며 기존 독립 Native 기대를 유지한다.

원문 `0043e530` 예산 Idle은 known이 비영이면 `0040ec50(localOwner)`로 장기 SP를 직접 읽는다. 첫 GUI에서 전체 정리 뒤 Core1이 선택됐는데 예산 표시가 반납한 Session을 요구한 공백을 확인했다. 실제 초기 HUD와 Session의 `ProductionControlContext`에 같은 `ClientSpGlobals`를 제공하고 예산/돈 조회를 그 저장소에 연결했다. 기존 독립 문맥은 종전 Session 경로를 유지한다. 비용·실제 내용물 효과 등 현재 게임이 필요한 경계는 계속 실제 Runtime을 요구한다.

정리 부모가 중첩 정지를 전부 Resume한 뒤에도 C++의 `uiPaused_` 보유 표식이 남으면 다음 `Pause(true)`가 정지를 건너뛰게 된다. 실제 `PopGamePause()`의 마지막 깊이가 풀렸을 때 이 호스트 표식도 반납하도록 했다. 원문 GameClock의 Resume 횟수·시계 쓰기는 추가하지 않는다. 새로운 중첩 정지 검사는 UI/스크립트 두 깊이와 두 음향 hold를 정리한 뒤, 다음 UI 정지가 다시 취득·반납됨을 확인한다.

## 검증의 구별

원문 부모294관측과 기존 작은 하위/세 판본 Fort Money 관측은 그대로 사용한다. 새 C++ 검사는 실제 Client와 네 HUD 공장을 구성하여 전체 초기 정리·다음 Core/Player/HUD 교환·선택 장치 실패의 prefix·반납 뒤 예산 Idle의 현재 SP 읽기를 확인한다. 이것은 원문 자식을 새로 모의한 기대값이나 전체 미션 자연 완주 증거가 아니다.

실제 메인 메뉴의 명시 정리 두 번에서 새 전체 HUD 토큰22→203과 Player base의 두 번 교체를 관측했다. 실제 Campaign→Early Missions→Early1의 MORE/OK 뒤에도 같은 부모를 호출하여 현재 연결표2→0, World/Runtime 대여 반납과 새 HUD 토큰290을 확인했다. 두 최종 화면의 실제 예산·생산·미니맵·하단 패널을 직접 검토했다. 화면은 미션이 없는 정리 후의 중간 상태이며 Editor 설치 화면으로 세지 않는다.

최초 build1의 C2664는 refresh callback 자체 대신 호출 결과를 전달한 코드 오류였다. callback 전달로 교정했다. 첫 신규 검사에서는 실제9칸 색 표의 열째 요소를 읽은 입력 오류를 발견했다. `kOwnerColorCount=9`를 확인해 실제9칸의 출력을 검사하며, 원문 밖 이웃 canary는 기존 StateBridgeReset의 독립10칸 입력에 계속 남긴다. 기존 Native fixture·기존 숫자 단언은 바꾸지 않았다. 첫 GUI의 Session 부재 오류는 위 장기 SP 연결로 수정했다. 실패 로그를 지우거나 성공 로그로 덮어쓰지 않는다.

최종 빌드·집중/전체 사례 이름·게임/소스 SHA·GUI 원시 경로·보호 파일 검증은 [실행 보고서](../../cpppj/whole-state-runtime-20261026.json)에 기록한다. 원시 자료는 `extracted/cpp-whole-state-463-*`다. CTest는 `netstorm_tests` wrapper 하나이며 내부 C++ 사례 수와 wrapper 수를 구별한다.

최종 build6 종료0·경고/오류0, 신규5개 포함 집중252고유/261관측·실패0, 같은 제품의 전체 C++3148개·실패0과 실제 GUI2건이 통과했다. 이전459의3130개 이름을 모두 유지했다. 게임SHA는 `7f4517d2…`다. 기존 생산 검사 본문의 바이트와 명시 보호범위(`originals/`·`originalCD/`·`AGENTS.md`)1820파일, 기존 fixture654파일을 최종 검사와 GUI 뒤에도 유지했다. 이 보호 집합을 과거1821파일 manifest와 같은 것으로 간주하지 않는다.34경로·긴 Early 완주·전체 화면/음향 동일성은 이번에 반복하지 않았다.

전체1 제품은3147사례·실패0이었으나, 검증 도구가 stdout/stderr 혼입으로 손상된 행을 이름으로 읽어 이전5개 누락으로 잘못 진단했다. 최종 검사 실행기에 `--list`를 추가하여 사례 본문·원본·창을 실행하지 않고 실제 컴파일된 등록 이름만 출력한다. 최종 대조는 이 목록, 전체 무필터 실행의 사례 수/실패 수/종료코드, 이전459 커밋 `b959f1bd`의3130개 실제 소스 등록 이름을 함께 사용한다. 기존 원시 행과 첫 진단도 보존하며, 등록 목록만으로 실행 통과를 선언하지 않는다.

실제 UI 검사의 `test-state-world`는 명시적인 검사 전용 전체 정리 호출이다. 일반 실행에서 추가되는 자동 조작이 아니며, 자연 Edit 예약의 State1 prefix/checkpoint/tail/공통 확정을 대신하지 않는다. 자료는 보호 원본에서 복사한 별도 경로를 사용한다.

## 남은 연결

이번 부모에 도달한 일반/초기 Editor 경로를 구별한다. 실제 metamap의 `LeaveMetamap`와 비mission 전투 phase14의 `saveBattle`는 선택될 때 필수 외부 장치를 요구한다. 현재 Uber는 이 두 미복원 경계를 빈 성공으로 연결하지 않는다. 일반 미션은 missionPost 조건으로 saveBattle를 생략한다. 별도22칸 초기 송신버퍼의 실제 생성 writer도 아직 남는다.

자연 Edit next1을 Tick에서 소비하는 원문 prefix, 로컬 listener/client9, 준비 대여 Session·실제 checkpoint, UI/팔레트 tail·공통 State 확정을 계속 연결해야 한다. 편집 도구·새 맵·Save as·Test Battle·언어별 시나리오와 전체 미션 자연 완주·픽셀/음향 동일성도 계속 남아 있다. 이 문서는 그 범위의 완료를 주장하지 않는다. CD10.72는 기존 작은 계산 회귀를 유지하지만 전체 State 제품 복원 완료로 세지 않는다.
