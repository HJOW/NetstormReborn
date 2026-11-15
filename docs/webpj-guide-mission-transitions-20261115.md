# Guide 미션 교체 이식 (WB04-E523)

`client/GuideMissionTransitions`를 `webpj/www/core/guide-mission-transitions.js`로 옮겼다. 기준선
`225553df`와 현재 cpppj 소스가 같음을 `git diff --numstat 225553df HEAD`로 확인했다. 이 단위는 Session이나
Client가 필요 없는 순수 순서 규칙이라 4-D의 막힌 Guide 항목(`CampaignGuideRelease`)보다 먼저 옮길 수 있었다.

## 옮긴 범위

- `beginGuideMission`(00463b70): 현재가 있으면 finish/release 한 번 뒤 이름을 NUL 앞까지만 소유 복사해
  공장(`create`)을 호출한다. 해제 중 새 현재가 생겨도 재확인 루프 없이 한 번의 공장으로 덮어쓴다.
- `closeGuideMission`(00463b20): 현재가 있을 때만 release(가상 +1c 종료는 호출하지 않음).
- `forkGuideMission`(00463da0): 현재 `fork%d`(signed) 값을 첫 해제 전에 복사한 뒤 finish/release하고 Begin.
- `restartGuideMission`(00463cf0): release부터 시작해 Begin 뒤 mode를 복원하고, 새 현재가 있을 때만 demo를 복원.
- `endGuideMission`(00463e40): finish와 release를 한 번씩, 공장은 호출하지 않음.
- `dispatchGuideMissionCommand`: Begin/Reload/Fork/End만 상위 플래그를 지운 코드로 분배하고 그 밖은 false.
  Begin의 세 번째 값(signed, `event.y`)이 100 이하이면 원본의 잘못된 문자열 주소 진단(`invalidBegin`)을 먼저
  요청하고 반환 뒤 같은 인자로 계속한다.
- 선택되지 않은 효과는 빈 성공으로 대신하지 않고 도달한 위치에서 거부한다.

## 시험

`webpj/tests/guide-mission-transitions.test.mjs` 3건:

1. 원본 10.78 관측 `guide-transitions-x86.tsv` **286행 전부**: `T` 256행(연산 Begin·Fork·Restart·명령
   Begin/Fork/Reload/End × 현재·모드·데모·재진입·인자·이름·플래그·x)의 반환·현재·이름 바이트·mode,demo·효과
   순서(F·L·C·G·A)와 `S` 30행(제어 이름/인자가 원본 토큰·80칸 문자열 고리·큐에 예약되는 사건 코드·x==y·
   문자열/숫자 인자·고리 번호; 기존 `queueMissionControlAction` 연결 포함). **첫 실행에서 286행 전부 일치**했다.
2. 이름 NUL 절단, Fork 키 `fork-1`(signed), Close/End의 공장 미호출.
3. 지원하지 않는 명령, 상위 플래그 제거, Begin 인자 101/100/−1의 진단 순서, 빠진 수명/분배 효과 거부.

`webpj/tests/fixtures/manifest.json`에 `guide-transitions` 항목(원본 10.78, 286행)을 추가했다.

## 남은 경계

- 실제 현재 미션/Script/공장(`Client`)과의 연결은 `CampaignGuideRelease`·Session 조립의 몫이다. 앱 수명에는
  연결하지 않았다(PORT_MAP `port`).
- 다음 시작점: `CampaignGuideRelease`(선행 `RawCampaignMissionStartBindings`·`RawCampaignTutorialOneBindings`는
  E520에서 끝났으며 실제 Client 소유자 대신 덕타입으로 시험할 수 있다).
