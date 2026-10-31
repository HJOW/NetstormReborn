# 단위655 — 시작 CD 결과와 Zone 뒤 실제 Renderer 초기화

2026-10-31, HJOW-Athlon(192.168.0.94). 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23 자료다. 원문/디컴파일 파일은 바꾸지 않았다. 보고서는 [mission-entry-20261031.json](../../cpppj/mission-entry-20261031.json)이다.

## 실제 호출 연결

WinMain은 옵션 해석 전에0054db6c를0으로 만든 뒤 `00440c60`의 반환값을 기록한다(`004392e7`→`004392ed`→`004392f9`). 이 시작 CD 결과는 State7의005c85b8·CDDir 설정·캐시00558dd4와 별개다. `ClientCdGlobals::InitializeStartup`이 실제0 쓰기→같은 캐시 확인→결과 저장을 수행하며 `Client::Run`의 원문 위치에서 호출한다. 조회가 예외로 중단돼도 앞선0과 캐시의 실제 효과를 보존한다. 시작 레지스트리 InstallMax/DiagResults는 별도 잔여다.

미션 전환의 Renderer 초기화 `004993c0`은 이미 `ClientCameraGlobals::InitializeNativeRenderer`와 `Client::InitializeNativeRendererView`에 복원돼 있었으나 로컬 로그인 앞단에 연결되지 않았다. 원문 State7은 Zone 설치/반납 뒤 `004b75cc`에서 Renderer를 초기화하고 이후 PlayerKnown0·UI2·다음 상태 요청을 수행한다.

실제10.78 로그인 서비스가 기존 Client 몸체를 제공하고, 현재 `Login.Begin`은 Zone 이미지 반납 직후/첫 접속 이전에 이를 호출한다. 기존 `RawLocalMissionState`의 `initializeRenderer` 경계도 같은 장치를 빌린다. 앞선 Player 정리를 채택한 실제 미션은 장치가 없으면 그 위치에서 중단한다. 상태8부터를 명시 범위로 삼는 독립 부분 검사는 기존 장치 생략 범위를 유지하며 전체 State7 성공으로 세지 않는다.10.72의 기존 어댑터에는 새 장치를 등록하지 않는다.

초기화는 같은 논리 Renderer/목표/snapshot과 WorldRedraw·MapRevision을 사용한다. 별도 카메라나 호스트 Renderer를 새로 만들지 않는다. 현재 배경 분모·CD 기록·난수는 초기화 대상이 아니다. 이는 새 알고리즘을 만든 작업이 아니라 복원된 몸체의 실제 호출 누락 수정이다.

## 확인한 범위

`netstorm_mission_entry_tests`는23등록 중 **고유4개/최종 실패0**이다. 다른19개와 주 검사 전체는 실행하지 않았다.

- 새 시작 검사1개: 실제 Client의 별도 시작 DWORD, 앞선0→조회/성공/예외/실패, 캐시/State7 값/RNG 분리, 순차 실제 Client의 독립 소유권.
- 새 Session 접합1개/입력2개: 같은 실제 Client의 Player/Core/Gameplay/State와 요청86BYTE를 빌린다. 실제 Zone 반납 뒤/첫 패킷 전 Renderer 호출·view/목표/snapshot·redraw1·MapRevision의 DWORD 감김·분모 보존과 이후 phase14/PlayerKnown을 확인한다. 장치 누락은 Zone 반납 뒤·패킷 전/phase0에서 중단한다.
- 기존2개: 독립 원문 카메라16행과 CD492행의 전체 물리/결과 대조. 두 몸체는 이번에 변경하지 않았다.

최초 새2개 실행에서 Session은 통과했으나 시작 검사가 두 Client를 동시에 생성해 기존 `Only one client may exist` 관문에 걸렸다(1단언). 첫 Client를 반납한 뒤 두 번째를 만드는 입력으로 고쳤다. 제품 관문은 유지했고 해당 시작 검사만 다시 실행했다. 이후 기존2개도 각각 한 번 실행했다. 최종 고유4개, 검사 함수 총5호출/실행4회이며 최초 실패 로그도 보존했다.

새 독립 PE 실행/fixture는 없다. 기존508행을 재대조했고 시작/State7의 호출 위치는 진입점부터 정렬한 기계어로 정적으로 확인했다. 이를 새로운 Native 관측 수로 세지 않는다. 최초 제품/집중 빌드,10.78 서비스 조건 반영 라이브러리의 제품 최종 재링크, 검사 입력 정정 후 증분 빌드 모두 종료0·경고/오류0이다.

보호 관련7입력은 불변이다.6개는645 기준(AGENTS/두PE/TAFF/카메라·State 기존fixture), CD fixture1개는654 보고서의 해시와 비교했다. 전체2632개/기존773개 재감사와 구별한다. 원시 근거는 이 PC의 `extracted/cpp-mission-entry-655-*`에 있다. EXE SHA-256은 `18d2cad79e9c14c8a9544e75eab24b768a7425ff6b47091790cd20d1b63fb4ac`다.

실제 게임/GUI·CD 열거/삽입·새ZIP·전체34·신규MP 검사는 없고 주 검사 EXE도 재빌드하지 않았다. 더 많은 기능 묶음 뒤 필요한 스테이지 통합을 수행한다. 사제 포획/희생·특정 적 공격·승리 재현과 공중3종 전용 행동 검사는 하지 않았다.

## 잔여

현재 Session `Login.Begin`은 상태8 Run/Poll/Pump와 phase14/loaded를 동기로 요구한다. Renderer 접합을 전체 비동기 State7/State1·UI2·자연 Tick/정리 수명 완료로 확대하지 않는다. 메타 State19의 CD005c85bc·시작 레지스트리·LeaveBattle 페널티도 별도 잔여다. 다른 UI/Edit/시나리오·호환성·BaseFile 소비자/ABI와 이후 MP 무시험 사전 구현·4~7차를 계속한다. 자연 완주/전체 동일성은 사용자 확인 대상이다.
