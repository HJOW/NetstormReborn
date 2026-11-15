# Guide 소멸 이식 (WB04-E524)

4-D 구획 B의 `client/CampaignGuideRelease`를 `webpj/www/core/campaign-guide-release.js`로 옮겼다. E344에서
"C·E 구획 뒤"로 막혔던 항목이며, C 구획의 `RawCampaignMissionStartBindings`·`RawCampaignTutorialOneBindings`
(E520)와 Guide 교체(E523)가 끝나 풀렸다. 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.

## 옮긴 범위

- `CampaignGuideRuntime`: 저장 토큰·Init 입력(미션 이름의 원문 부호화 바이트)·demo/started/단계의 유일한
  현재 미션 저장소(`gate`)·Start 상태·Mission+64 복귀 모드·실제 Tutorial 가상표 사용 여부(`missionType`이
  `tutorial`)·Start/Tutorial 실행기 자리. 생성은 번호·Start 효과를 앞당기지 않고, `dispose()`는 Tutorial →
  Start 순서로 참조를 반납한다.
- `releaseCampaignGuideMission`(00463ab0 → 004c3210 → 00483210): 이전 Guide의 지연 Fort 로드 요청 반납
  (`LoadMissionFort`·`LoadMissionWorld` 취소) → 실제 Tutorial이면 허용 표 전체 허용(첫 255칸)·회수/승급 금지·
  미션 번호·Early 분배 번호·단계·표식 0 쓰기 → 현재 Guide AI 반납 → 기본 Mission 해제(실행기 소멸·세대 토큰 0·
  Config 해제·`global.inMission` 0). 기본 Mission은 정상 꼬리와 예외 unwind에서 한 번만 해제한다.
- AI 반납: World를 빌린 상태이면 같은 Session의 Player 기록이 Client의 것과 같아야 하며 실제 참여 상태에서
  모든 AI를 반납한다. 이미 반납된 상태이면 Player 1..8의 `+b0` brain이 모두 NULL인지 확인하고 하나라도
  비NULL이면 가짜 빈 Player로 대체하지 않고 명시 오류로 둔다. 미초기화된 Player 물리(1620바이트 아님)도 거부한다.
- 소유 참조(`mission`·`runtime`·`request`·`missionNumber`·`worldMissionToken`·`nativeMission`)는 `{ value }`
  상자로 받는다(C++ 참조 인자 대응). 여섯 전역은 속성 getter/setter로 같은 DWORD를 빌린다.
- 실제 `Client`는 아직 없어 모듈 머리글에 최소 덕타입 계약을 적었다.

## 시험

`webpj/tests/campaign-guide-release.test.mjs` 4건(실제 `GameData`·실제 `TheWarBegins` 미션 스크립트·
Guide 교체 E523, Client/Session은 덕타입):

1. `CampaignGuideRuntime` 초기 입력(이름 `TheWarBegins`·tutorial 판정·데모/모드·토큰, 번호/Start 미생성).
2. World를 빌린 Tutorial End(cpppj `EndKeepsActualWorldInput...` 대응): 요청·실행기·미션·세대 토큰·번호·
   `global.inMission`·허용 표(255칸 허용·마지막 칸 보존)·금지 DWORD·단계 전역·AI 반납 한 번.
3. World 없는 경우(`NoWorldUsesActualPlayers...` 대응): NULL brain은 정상, 비NULL brain은 명시 오류이며
   unwind가 기본 Mission을 한 번만 반납하고 Player 기록은 불변, finish는 호출되지 않음.
4. 비Tutorial/런타임 없음은 기본 해제만, 미초기화 Player·Session Player 소유자 불일치는 거부.

## 남은 경계

- cpppj 시험의 실제 Client+GameWorld+UserInput 조립(선택·입력 경로 보존, State reset 증빙 소비 구별 등)은
  Session/Client가 생기는 4-D D·E 구획에서 같은 계약으로 재대조한다.
- 다음 시작점: 4-D C 구획의 `RawCampaignAssetBindings`·`RawCampaignConstructionBindings`, 이어서 D 구획.
