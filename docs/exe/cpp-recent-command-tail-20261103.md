# 일반 명령12개의 press 후처리

단위784는782(`c4b8a17d`) 뒤의 최소 입력 제어 변경이다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 산출물·AGENTS.md·원본·기존 fixture는 바꾸지 않는다.

ChangeFontFace/ChangeSoundQuality, ChangePal/ChangePriority/SetFortType, TakeScreenShot/UrlAlias/Run/MoveIsland/LookAtSid, DrawWater, CreateHelpList의 기존 본문과 실제 장치를 유지한다. release는 기존처럼 소비 후 반환하고 press는 같은 현재 Help/Console/정지/모달/월드 후처리로 진행한다. 실패하면 기존 예외와 앞선 효과를 유지하며 정상 성공이나 새 dirty를 합성하지 않는다.

원문004d62b0의 정상 일반 명령 사슬은004d8bda→00413d50과 공통 꼬리로 합류한다. 기존의 무조건 return과 호출 범위가 달랐다. getter 생략만으로 실제 플레이 상태 변경이나 오류를 확정하지 않으며 같은 장면의 모든 조건부 writer가 선택됐다는 새 관측도 없다. 이 단위는 해당 제어 흐름 차이를 수정한다.

지역 recentCommandHandled와 기존 createAtHandled/aiUtilityHandled/makeAiHandled/aiDebugHandled를 늦은 GameMenuCommand 및 미연결 분류의 두 guard에서 모두 검사한다. 이미 실행한 명령을 다시 실행하지 않게 하며 기존 AI·CreateAt 본문·기존 타이틀 next42 처리를 보존한다. 활성 caller가 없는 관리명령을 기본 UI에 추가하지 않는다. MoveIsland의MP분기는 새로 검사하지 않는다.

새 C++ 검사·fixture·Native 관측·게임 실행은 추가하지 않는다. 창 없는 실제 Uber/Input 조립 경계가 없어 별도 가짜 Screen/UI를 만들지 않았으며 기존 관련 helper의 집중 회귀와 정적 접합 대조를 수행한다. helper 통과를 실제 Uber 전체 꼬리/Console/GUI 효과 검증 완료로 합산하지 않는다. 완료 빌드·집중 결과와 보호 해시는 아래와 보고서에 기록한다.

## 최종 확인값

게임/ScriptBindings/MenuSession을 Release 빌드했으며 경고/오류0이다. 기존 Utility283·Configuration238·Sound32·FontParent·DrawWater384·HelpList128·CreateAt408의 집중7개가 모두 통과했다. 새 검사/Native/게임/MP0이며 전체71대상/전체C++/34경로/ZIP·물리/PCM·지정플레이는반복하지않았다.

현재 한파일의8행/플래그접합외기존효과·hook·네handled본문이모두보존됨을대조했다. 최초감사도구prefix가Event밖분류도선택해중단한것은event.code범위로교정했다. 제품/검사 실패와구별한다. EXE SHA256은 `60004899d0fc92b11dc74e6b641a6e13cdf74f878b83855cfa31992a5a52bc0d`이며 기존보호2720개/fixture900은불변이다. [보고서](../../cpppj/recent-command-tail-20261103.json)·`extracted/cpp-resume-784/`를 따르며 다음785이고 전체목표는진행중이다.
