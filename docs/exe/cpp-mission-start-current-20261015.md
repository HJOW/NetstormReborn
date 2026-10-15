# 현재 Session 입력과 실제 SP를 사용하는 Mission Start 준비

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. AGENTS와 양 LEFT를 다시 읽었고, 직전 통합 소스 동결 중에는 읽기 전용 설계만 수행했다. **CP63 `400aaef` 완료 신호 뒤** 이 단위의 소스 편집을 시작했다. 직전 커밋의 Release 경고0·집중60·전체1,283개 결과를 이번 변경의 검증으로 합산하지 않는다. 새 디컴파일·native 관찰·공용 빌드·원본/클론 게임·GUI 실행은0이다.

## 현재 전역 조회와 원본 읽기 위치

`RawMissionStart::BindCurrentInputs(MissionStartReads)`가 현재 localOwner/playerKnown/editor의 세 getter를 모두 요구한다. 부분 연결은 거부한다. 미연결 독립 fixture는 기존 `MissionStartGlobals` 값을 그대로 읽고, 연결 뒤에는 getter 반환을 fallback에 복사하거나 프레임별 상태로 저장하지 않는다. denySalvage/denyAscend/allowAnyCapture/ordinaryGeysers의 기존 원본 쓰기는 유지한다.

같은 PC의 전체 정밀 소스 `004c2b20`을 다시 읽어 다음 순서를 확인했다.

| 위치 | 실제 읽기/효과 |
| --- | --- |
| `myStartMoney` 조회 뒤 | editor1회, 이어 playerKnown1회 |
| playerKnown!=0 | 현재 localOwner를 읽어 직접 SP 지정 |
| SP 표시 갱신 뒤 | 현재 localOwner를 다시 읽어 기술 제거 |
| `myTech` 조회 전 | 현재 localOwner를 저장한 뒤 설정을 읽고, 저장한 owner로 기술 추가 |
| AI owner1..8 순회 | 각 반복마다 현재 localOwner를 다시 읽고, 다르거나 demo일 때만 실제 사제 조회/AI 요청 |

외부 SP/표시/기술/AI 효과가 owner를 바꾸면 그 뒤 원본 위치만 변경된 값을 읽는다. 특히 `myTech` 조회의 효과가 owner를 바꿔도 이미 선택한 기술 owner를 다시 바꾸지 않는다.

## 실제 Session의 읽기/직접 SP API

`RawCampaignSession`에 다음 공개 API를 추가했다. 자동 Start/AI/Outcome 호출이나 별도 저장소를 추가하지 않았다.

| API | 실제 대상 |
| --- | --- |
| `LocalOwner()` | 같은 `playerState.battleOwner` / `00540c70` |
| `PlayerKnown()` | 같은 `playerState.playerKnown` / `00595344` |
| `Editor()` | 같은 `globals.editor` / `005c85a4` |
| `Sp(owner)` | 같은 `impl_->sp.Get(owner)`의 DWORD를 float로 재해석 |
| `SetSp(owner,value)` | float의 원본 DWORD를 같은 `impl_->sp.Set`에 직접 전달 |
| `SpStore()` | 같은 `ScrambledSpStore`의 읽기 전용 참조 |

직접 SP 지정은 `0040edb0`이고 `AddSp`의 가산/AI 지갑/화면 갱신과 다르다. Set이 소비하는 실제 난수 잡음도 기존 Session 게임 난수와 같은32×9 분산배열을 사용한다. 초기화·회전·사제 추적·Player 내용물 SP 접두·보상·건설이 이미 연결된 저장소다. `SpStore()`는 후속 소비자의 실제 주소/offset/순열 검증을 위한 읽기 전용 접근이며 다른 저장소를 만들거나 mutable 원문 배열을 노출하지 않는다.

실제 Start 서비스 조립은 후속이다. 현재 API를 사용해 same Session getter와 직접 SetSp를 바인딩해야 하며, AI allocationBytes나 색/전송을 임의0/noop 성공으로 채우지 않는다.

## 회귀 검사와 관찰 집계

`MissionStartTests.cpp`에 검사2개를 추가했다.

1. 기존 native204개 중 **Start48개**의 기대 상태/전체 효과 trace를 현재 getter와 **반대 fallback**으로 재생한다. 잘못된 fallback 사용은 SP/기술/AI trace와 편집 금액에서 드러나며, getter가 fallback을 현재값으로 덮어써도 실패한다. fixture 바이트는 유지한다.
2. SP/표시/기술/AI 효과가 현재 owner를1→2→3→4→5→6→7로 바꾸는 명시 입력으로 editor/playerKnown/owner의 정확한 조회 순서, `myTech` 앞의 owner 보관, 반복 중 재조회를 확인한다. 부분 getter 연결 거부도 검사한다.

`CampaignSessionTests.cpp`에는 보호 `thewarbegins.fort`를 실제 Session에 설치하는 검사1개를 추가했다. 같은 SP 원문32개 배열의 owner1 슬롯만 변경되고 다른8 owner의 모든 DWORD가 그대로인지 확인한다. 실제 순열/offset으로 원문 보존비트를 독립 조립해 float123.25와 대조하고, 배열·첫 배열 포인터 전역의 주소/offset/추적값/순열 유지, 읽기의 무쓰기, 음수0의 비트 보존, owner9 거부, 직접 Set의 외부 UI/AI 생성 없음도 검사한다.

새 검사는 총3개이며 root의 최종 고정 소스 **Release 경고0·오류0·관련 집중52개 실패0·전체 CTest1/1·내부1,290개 실패0·203.58초**에 포함해 통과했다. 로그는 `extracted/goal-login-packets-final-{build,focused}.log`와 `extracted/goal-login-packets-{ctest,tests}.log`다. `git diff --check`를 통과했다. 새 native 고유입력은 **0**이고 기존 Start48 재생을 새 관찰로 합산하지 않는다. 기존 `mission-start-x86.tsv`의 SHA는 **0872a680e563feda45823a75bd9cc0f6be52576629f57745f22054c7a75a5011**이다.

## 현재 SHA와 과거 보고서

현재 제품/검사/근거 SHA는 `cpppj/mission-start-current.json`에 새로 기록한다. 기존 `mission-start.json`, `session-ai.json`, `session-construction.json`, `client-ai-chat.json`, `mission-guide.json`, `session-priest-participation.json`은 각 작성/완료 시점의 **역사 스냅샷**으로 보존한다. CP63 `400aaef`에 저장된 Guide/사제 참여 보고서의 Session/Start 헤더 SHA를 이번 소스로 덮어쓰지 않는다. 과거 도구의 현재 파일 SHA 감사는 해당 역사 커밋을 기준으로 평가해야 한다.

보호 원본·AGENTS·dotnetpj·CMake를 변경하지 않았고, 실제 로그인/Guide/Stage3/미션 게이트/자연 완주 연결은 이번 API 준비와 구분한다.
