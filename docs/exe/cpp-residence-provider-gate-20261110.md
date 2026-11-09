# 작업1238 — Fort Dude 생성의 실제 공급원 count 연결 수정

**제품1238에서1237의 Fort Dude 생성 목록 오접속을 수정하고 허용 검증을 마감했다. 전체 목표는 미완료다.**

제품1237에서450a90의00592f8c를 거주 구역 수로 잘못 읽고 Native `sidLists.residences`/typed `assetState.Residences()`를 연결했다. 실제 원문/기존 RawStateSidLists에서00592f80은 공급원 헤더, +4=00592f84 배열, +c=00592f8c count다. 거주 구역은0059a9d4 헤더/+c=0059a9e0이다. 이를 검토한 뒤 두 caller를 현재 `sidLists.providers`/`core.bookkeeping.Providers()`로 고쳤다. 함수/검사의 파라미터와 설명도 공급원으로 바로잡았다. Dude count를 먼저 읽고 공급원 count를 다음에 읽는 signed 비교는 유지한다.

근거는450a90의 실제 비교 명령,49a2b0의 거주 구역 등록 주소59a9d8/e0,472f70의 공급원 등록592f84/8c와 Mana.cpp 원문 진단이다. 이번에는128개 서로 다른 Fort/Dude count/공급원 count/거주 구역 count/Source0·50 입력으로 실제 PE가 읽은 주소를 관찰했다. Fort일 때55a4f8→592f8c를 읽고59a9e0은 읽지 않는다. 최초 srcSid 진단/진입점 호출까지 검증하며 큰 하위 생성/작업 전체 실행으로 세지 않는다.

새 Native 회귀는 공급원0/거주 구역10에서Source0도 조기 반환하고, 공급원1/거주 구역0에서는 원문srcSid 진단이 선택되는 것을 확인한다. 이전 검사의 두 목록을 혼동한 입력도 실제 공급원으로 고쳤다. 이것은 원본 기대값/실행 관문을 약화하는 수정이 아니다. 일반 미션에서는 Fort 관문이 먼저 반환하므로1237의 일반 거주 구역 프레임/원문 재생 증거는 유지되지만, Fort 생성 한도와 그 설명은1238을 기준으로 한다.

마지막 디컴파일은 Client양판본2026-11-03/R2026-11-07의 HJOW-Athlon이다. 현재도 같은 호스트이며 새 디컴파일/기존 정밀 C 변경0이다. 원시 증거는`extracted/cpp-resume-1238/`, 보호 원본/CD/기존fixture·AGENTS2,807파일이다. Regular 연결71/87·잔여16VT/14함수와 전체 M6/16상위계획은 아직 미완료다. 수정 마감 뒤 다음은 Dude1VT/450fa0다. 지정한 MP·공중3종 전용·포획/희생·특정 공격·승리/직접 확인 실행 제외를 유지한다.

최종 Release/x64 제품2·집중1대상 빌드134.146초/종료0·경고/오류0, 변경 소스/검사6개 고정·비MP156/156이다. 최종 EXE의 Tutorial4 정상 생산·건설 요청→TYPE75/owner1 실제 생성→600프레임 대기·정상 종료0·PNG 확인을 마쳤다. 보호2,807파일과 실행 전후 제품/격리 설치 자료가 불변이며 시작한 빌드/집중/GUI 작업은 모두 종료했다.

- Netstorm.exe: 14215680 bytes, SHA256 `03f942d6b272a8b12ee820b42873aebb58eccad57e0ff7517699a0d6da545f4d`.

- R.exe: 677376 bytes, SHA256 `e33d946fd08305dc7b7c3b07fb1d1819e2fb63fb1f0b75a3d4dbb54067d559ed`.

[확정 보고서](../../cpppj/residence-provider-gate-20261110.json).
