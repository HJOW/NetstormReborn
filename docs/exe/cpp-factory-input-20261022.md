# Factory 사용자 입력과 실제 메뉴 연결

2026-10-22 단위285, 시작 HEAD `fc6f4ec1 / 20261022-07 CP`. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-21**로 같다. 기존 정밀 원문과 보호 10.78 PE를 정적으로 읽었다. 새 디컴파일·보호 원본/CD/AGENTS/dotnetpj 변경과 게임·집중 C++ 검사 재개는 없다.

## 입력 복원

[`RawFactoryInput`](../../cpppj/src/o/RawFactoryInput.cpp)은 원본 가상 입력 `004547d0`의 상위6비트 제거 뒤 상태 순환→업그레이드→연구→공통 부모 순서를 합성한다. 실제 PE 명령 전역 `00542884/005427c0/005428f8`은 각각 `03000063/03000032/03000080`이다.

- 레벨 순환은 편집 모드 검사를 추가하지 않고 가상80을 한 번 읽어 **signed DWORD 증가/감김 후 %3**을 가상84에 넘긴 뒤 기존 `00453780` 프레임 보정을 실행한다.
- 업그레이드는 현재 Factory BYTE TYPE의 복호화 signed 비용을 **먼저 float로 축소**한다. 이후 현재 로컬 소유자 DWORD의 실제 분산 SP 비트를 읽고, 그 효과 뒤 현재 editor를 판정한다. 편집기에서도 SP 조회를 생략하지 않는다. 비편집 상태에서 `!(costFloat<=moneyFloat)`면 `upgradeNoBucks`를 Tell하고 처리1을 반환한다. 실제 x87 `test ah,41 / jnp`에 따라 비용 초과와 unordered SP 모두 자금 부족이다. 16,777,217 같은 정수 비용도 float 축소 뒤 비교하며 double 정수 비교로 바꾸지 않는다.
- 자금 관문 뒤에만 기존 `00453c90`의 실제 그림→event4→event3을 조회한다. 이미 진행 중이면 `alreadyUpgrading`을 Tell하고, 아니면 단위283/284의 같은 실제 요청·20초 event4·15초58h·그림/완료 경로를 사용한다. 임의 소유자 동일성/레벨 상한/보스 검사를 추가하지 않는다. 원본 메뉴의 항목 표시 조건과 실제 입력 조건을 구분한다.
- 연구는 기존 `RawFactoryResearch::Input`에 수정키/code와 packedArgument를 그대로 전달한다. 다른 명령은 같은 `RawObjectCommand` 부모로 위임한다. 기존 `SelectFactoryKnowledge`도 새 `CommandFactory`를 사용한다.

기존 연구 입력에는 슬롯 조회 직후 종류5의 원본 `owner: slotsRemaining/A=slots` 로그를 연결했다. 실제 Session은 현재 owner와 로그 장치를 사용한다. 기존 독립 원본 fixture는 로그 관찰 생략을 유지한다. 로그 뒤 현재 시각을 wire에 쓰되, 로그가 슬롯을 바꾸더라도 앞선 슬롯 지역값의 양수 판정은 유지한다. `researchHelp`는 같은 실제 Help 창·문자열 고리/별칭·모드1에 연결했다.

## Client 연결

Session은 현재 editor/로컬 소유자, 같은 분산 SP/비용/상태 프레임/실제 Host/연구/공통 객체를 빌린다. Factory 자금/진행 중 안내는 기존 `CampaignClientUi::tell`과 같은 실제 Tell 설정·창·게임 시계를 사용한다. 도움말은 기존 `RawMenuControls::OpenHelp`를 사용한다.

`RawFactoryMenu`는 이미 업그레이드 항목을 `03000032`로 작성했으나 `UberGump`의 메뉴 전송 허용 목록과 `CampaignObjectMenuClient::Command`가 이를 거부했다. 두 경계에서 업그레이드/상태 순환을 허용하고 `CommandFactory`로 전달했다. 같은 허용 목록에서 빠져 있던 **이미 실제 분배가 존재하는** 소유자/Hurt/삭제·운반체 내려놓기/폭탄 명령도 기존 처리기로 전달하도록 연결했다. 새 게임 효과나 편집 메뉴 항목을 임의 추가한 것은 아니다.

## 확인 기록

새11검사는 입력8개, 연구 슬롯 로그/시각1개, 실제 Session2개다. 비용 동일 경계/다른 소유자·레벨3의 추가 관문 부재, 자금 부족 우선, NaN/무한대/float 비용 축소/음수 비용, 편집기 SP 조회·예약 관문, 하위 뒤 현재 owner/editor, signed 레벨 나머지, 원문 위임/명령 우선순위, 선택 하위 누락의 접두 보존, 실제 로컬 SP/event4/중복 요청 차단, 실제 상태 프레임/연구 도움말/부모 위임을 다룬다. **모두 작성·컴파일만 하며 실행하지 않는다.**

첫 Release 빌드69711은 종료0/오류0이며 새 상태 순환 검사의 signed/unsigned 비교 C4389 경고1건을 확인했다. `build1.log/build1-result.json`을 보존하고 비교를 uint32로 맞췄다. 재빌드 세션83227은 실제 종료까지 조회했고 **종료0·경고0·오류0**이다. 최종 기록은 `extracted/cpp-factory-input-20261022-build2.log`와 `build2-result.json`이다. 두 로그를 내용 변경 없이 UTF-8로 변환했다.

[`cpp_factory_input_audit.py`](../../tools/cpp_factory_input_audit.py)의 [신규 정적 보고서](../../cpppj/factory-input-static-20261022.json)는 원본11함수/3명령 전역·실제 호출 순서·보호 원본 동일 SHA·최종 소스/바이너리 SHA·새11검사 링크/Client 연결 경계와 첫 빌드 경고 기록을 확인한다. **C++ 검사·CTest·게임·native 실행0**이다. 이전 보고서는 당시 증거로 보존했다.

## 남은 범위

이번 입력/메뉴 연결은 실제 GUI 클릭·Tell/Help 표시·반복 업그레이드 성공을 실행 검증한 증거가 아니다. 전체 x87 확장 정밀도/예외와 CD10.72도 미검증이다. 다른 AI 행동·원래 순서 자동 프레임/종료, Template/State7·전환/저장 수명, Early Missions6+캠페인1-1..1-6+데모3(총15개)와 Edit/custom Fort/시나리오의 자연 입력·플레이·화면/음향 동일성은 계속 남아 있다.

최종 빌드/정적 기록/인수인계를 확정한 뒤 `20261022-08 CP` 형식으로 로컬 커밋한다. 원격 push하지 않는다.
