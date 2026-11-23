# CampaignCursorClientEffects partial → port 판정 (WB04-E1256)

대응표 `cpppj/src/client/CampaignCursorClientEffects.cpp` 행(74줄)에 남아 있던 E1223 감사의 세 가지(화면 갱신 동등성·한국어 번역 폭 동등성·`drop.record` 도달성)를 실제 확인과 판정으로 닫아 행을 `partial` 에서 `port` 로 바꿨다(partial 26→25단위, 18,333→18,259줄, port 982).

## 1. closePopups 뒤 화면 갱신 동등성 → 실제 확인

원본은 `RequestClosePopups` 뒤 `common.changed()` 로 화면 갱신을 요청한다. 웹의 `closePopups`(`ClientGumpRoot`)는 팝업 노드를 `lifetime.kill` 하고 앱의 `changed` 장치(`invalidateAll` + `_displayDirty`)가 다음 프레임에 다시 그린다. **`e2e/mission-object-commands.spec.js` 새 시험**(실제 Chromium, The War Begins!)이 픽셀로 확인한다: 채팅 도우미 팝업이 차지하는 논리 사각형(실제 경계에서 4px 안쪽)을 팝업 전·열린 뒤·`_closeCampaignPopups` 뒤로 읽어, 팝업이 영역의 40% 넘게 바꾸고(열림), 닫은 뒤에는 열린 상태와 25% 넘게 다르며(지워짐) 팝업 전과는 10% 미만만 다르다(애니메이션 오차 허용). 닫은 영역이 다음 프레임에 원래 모습으로 돌아온다.

## 2. 한국어 번역 폭 동등성 → 같은 부호화 경로라 일치

원본의 우려는 Windows-1252 바이트 대 표시 코덱 부호화에서 폭이 달라질 수 있다는 점이었다. 웹은 한국어를 유니코드 글꼴 경로로 다루며, **측정**(`core/campaign-cursor-client-effects.js` `lifecycle.measure` → `fonts.encode(text)` + NUL)과 **그리기**(`render/raw-campaign-cursor-display.js:93` → `fonts.encode(`${text}\0`)`)가 같은 `FontSet.encode`(유니코드 글꼴이면 UTF-16 코드 단위)를 쓴다. 새 Node 시험(`tests/campaign-cursor-client-effects.test.mjs`)이 실제 한국어 번역표의 경고 다섯 문구(E1249 에서 더한 항목)로 측정 값과 그리기 경로의 글자 배열 측정값이 같음을 고정한다.

## 3. `drop.record` 는 개발자 키 없이 도달하지 않음

`makeCampaignCursorClientEffects` 의 `drop.record` 는 앱 client 에 `recordDeveloperBuildPlacement` 가 없으면 오류를 던지는 선택 장치인데, 원본에서 이 기록은 건설 기록기(`recorderActive`)가 켜졌을 때만 호출된다. 건설 기록 재생·기록(RawBuildReplay·ClientBuildReplay)은 E1242 가 개발자 전용으로 판정해 웹 범위 밖으로 닫았고 기록기를 켜는 개발자 키가 웹에 없어 이 경로는 도달하지 않는다.

## 시험

- 새 e2e 1건(위 1)과 새 Node 시험 1건(위 2). 관련 Node(`campaign-cursor-client-effects`·`raw-campaign-cursor-display` 등)와 대응표·manifest 시험 통과.

## 남은 일

- 원본 실행으로 비교한 것은 아니다(웹 실제 동작·소스 판정).
- 5-B 의 나머지 행은 RawProductionControls 뿐이다.
