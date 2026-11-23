# Priest Training 7 완료 안내의 Training Menu 단추 연결 (WB04-E1245)

[다음 작업 1 ⑦](../LEFT_JOBS.webpj.md#next)에 남아 있던 "Priest Training 7의 Training Menu 단추가 웹에서 아무 일도 하지 않는다"를 사용자 결정(2026-11-23, "연결해 주세요")에 따라 `bc1menu76` 연결로 닫았다.

## 원인

- 훈련(보너스 도전) 대본 가운데 `bc2bridging76`~`bc9ssun_bars76` 여덟 개의 훈련 메뉴 단추(`bc1menu76` 자신에게는 없다)는 모두 `MissionBegin,BC1Menu76`(접미 `76`, 웹 자산 `bc1menu76`)을 쓴다. 번역 폴더의 훈련 대본 한국어 파일 전체를 읽어 센 결과 이런 단추 줄은 16개(대본당 완료·실패 구역 각 하나)이고 그 가운데 접미가 빠진 줄이 하나이다.
- 단 하나, `bc7sun_bars76`(Priest Training 7, 썬 바리케이드)의 **완료 구역 `[@300]`** 의 첫 단추만 접미가 없는 `$Button=Training Menu,MissionBegin,BC1Menu` 이다(실패 구역 `[Failed][ai1templedead]` 의 같은 단추는 `BC1Menu76`). 영어 원본 대본도 같은 철자이므로 번역 오류가 아니라 **원본의 오타**이다.
- 원본 실행 파일은 이름 그대로 `d` 폴더의 10.72판 loose 파일 `BC1Menu.english`(와 `BC1Menu.fort`)를 읽는다. 이 폴더에는 같은 10.72판 훈련 사슬(`BC2Bridging`·`BC3Acid_Bar` 등, 단추 `Another!` → `MissionBegin,BC1Menu`)이 함께 있다. 웹 자산(netstorm.tarc)에는 76 접미판 대본만 있어 `_beginCampaignFromDialog` 의 허용 목록에 못 들어 "아직 연결되지 않았습니다" 콘솔 안내만 남기고 아무 일도 하지 않았다.

## 결정과 변경

사용자 결정은 세 가지 중 "`bc1menu76` 으로 연결"이다(10.72판 loose 사슬을 자산으로 복사하는 방법과 그대로 두는 방법은 쓰지 않았다).

- `www/core/mission-name-alias.js`(웹 전용 신규): `MISSION_NAME_ALIASES = { bc1menu: 'BC1Menu76' }` 와 `resolveMissionBeginName(argument)`. 별칭 비교는 대소문자·앞뒤 공백을 무시하고, 별칭이 아니면 입력을 그대로 돌려준다(문자열이 아닌 값은 빈 문자열).
- `www/netstorm.js` `_beginCampaignFromDialog`: 인자를 `resolveMissionBeginName` 으로 먼저 푼다. 허용 목록·`_markSkirmishRun`·`_lastCampaignMission`·`Guide.begin` 은 풀린 이름(`BC1Menu76`)을 쓴다. 대본이 적은 미션 이름을 받아 `Guide.begin` 을 부르는 곳은 이 함수뿐이다(스커미시·데모 시작은 고정 이름, 다시 시작은 위에서 캡처한 풀린 이름을 쓴다. `netstorm.js` 의 `Guide.begin` 호출 4곳을 검색해 확인).
- `tools/port-map.mjs` `WEB_ONLY` 에 새 모듈을 등록했다(www 의 모든 모듈은 대응표 web 칸이나 이 목록에 있어야 한다). PORT_MAP 행은 바뀌지 않는다(partial 29단위·20,275줄 그대로).

## 시험

- `tests/mission-name-alias.test.mjs` 5건: 별칭 해석(대소문자·공백·이미 76 접미인 이름·누락 인자), 별칭 표가 동결·소문자 열쇠임, **번역 폴더의 훈련 대본 줄을 직접 읽어** 접미 없는 `BC1Menu` 단추가 `bc7sun_bars76.korean` 한 줄뿐이고 나머지 15개는 별칭이 풀어 주는 철자(`BC1Menu76`)와 같음, 별칭을 푼 뒤 모든 훈련 `MissionBegin` 이름이 연결된 훈련 미션 아홉 개 안에 있음.
- `e2e/mission-priest-training-flow.spec.js` 새 시험(실제 Chromium): Priest Training 7을 목록에서 시작하고 `[@300]` 구역을 열리게 한 뒤(대본은 "시작 후 300초 경과 + 열린 창 없음"이 조건이라 300초를 기다리는 대신 `runtime.gate.startSeconds` 를 301초 앞당긴다 — `raw-mission-frame-gate.js` 의 판정식과 같은 값) 단추 `['Training Menu', 'Restart', 'FINISH THEM!', 'Leave Missions']` 를 확인하고 Training Menu 를 눌러 `bc1menu76` 이 시작되며(브리핑 `['Technology available','Ok']`, 데모 아님) 시계가 흐르는지 본다. **배선(`resolveMissionBeginName` 호출)을 `String(argument ?? '')` 로 되돌리면 이 시험은 브리핑 단추를 기다리다 시간 초과로 실패**함을 확인했고 복원했다. 같은 파일의 기존 Death Bridge 시험도 통과했다.
- 대응표·manifest 시험 15건(별칭 시험 포함) 통과. 전체 회귀는 주기(0.5절 5)에 따라 E1246 뒤 Node 26 으로 돈다.

## 남은 일

- 완료 구역의 다른 단추(`Restart`=`Tell,TryAgain`, `FINISH THEM!`=`MissionBegin,BC8Fury76`)는 이번에 바꾸지 않았다(앞의 시험은 단추 목록만 확인한다).
- 원본이 읽던 10.72판 `BC1Menu` 사슬(`BC2Bridging`~`BCSSun_Bars`)은 웹에 없다. 필요해지면 다음 작업으로 다시 정한다(자산 복사 시 한국어 번역과 `prepare-assets.js` 의 `KOREAN_CAMPAIGN_MISSIONS` 목록·허용 목록을 함께 넓혀야 한다).
- 자연 완주(5분을 실제로 버텨 `[@300]` 이 열리는 것)와 원본 실기 대조는 하지 않았다.
