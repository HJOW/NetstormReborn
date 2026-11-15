# 다음 루프 상태 전환 큐 이식 (WB04-E525)

`client/State`(원본 State.cpp 004b88c0의 다음 루프 상태 전환 자리)를 `webpj/www/core/client-state.js`의
`ClientState`·`ClientPhase`로 옮겼다. 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.
E524의 `releaseCampaignGuideMission`이 `pending.cancel`을 덕타입으로 요구하던 자리를 실제 클래스로 채운다.

## 옮긴 범위

- `post(action)`: 큐가 비어 있을 때만 받는다(이미 대기 중이면 한 번의 입력 묶음이 여러 화면을 건너뛰지
  않도록 무시). 새 항목을 받을 때만 64비트 일련번호가 증가하며 감김에서도 0은 빈 입력으로 남긴다(BigInt).
- `take()`: 맨 앞 하나를 꺼낸다(빈 큐는 `null`). `pending()`: 조회만으로 꺼내지 않는다.
- `takeAfterPoll(poll)`: Poll 진입 전에 대기하던 같은 항목(일련번호 동일)만 Poll 뒤에 꺼낸다. Poll이 새 요청을
  예약하거나 취소 뒤 교체 요청을 넣으면 그 요청은 다음 Tick까지 보존한다. Poll 장치가 없으면 거부하고 Poll이
  예외를 내면 대기 요청을 보존한다.
- `cancel(command)`: 대기 큐 맨 앞의 정확한 명령(대소문자 구분)만 취소하고 다른 요청은 보존한다.
- `phase`/`ClientPhase`: MainMenu·Dialog·LoadingMission·Briefing·Mission.
- DialogAction은 아직 없는 `DialogScript`(8단계)의 형식이라 `{ command, argument }` 평범한 객체로 둔다.

## 시험

`webpj/tests/client-state.test.mjs` 3건: 대기 중 새 전환 무시·Take/Pending·빈 큐, Cancel의 정확한 맨 앞 명령,
TakeAfterPoll(새 요청 예약/취소 뒤 교체/취소만/예외/장치 없음). 이 단위에는 원본 x86 관측 fixture가 없다
(cpppj 시험은 MissionLifecycleTests의 Post/Take 계약).

## 남은 경계

- 앱 수명에는 연결하지 않았다. 실제 Client 루프의 Take/TakeAfterPoll 호출은 4-D E 구획(`ClientMain`)의 몫이다.
- 다음 시작점: `RawMonitorTime`(E526).
