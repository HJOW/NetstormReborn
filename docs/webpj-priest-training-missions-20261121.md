# Priest Training(bc*76) 아홉 미션 시작 연결과 전 미션 시험의 거짓 통과 수정 (WB04-E1157)

미션 도중 입력을 지켜보다가 "Death Bridge(bc5)"를 시작하면 사실은 데모(DemoStormRages)가 돈다는 것을 발견했다. 원인과 수정을 정리한다.

## 증상과 원인

- 캠페인 목록 Priest Training 의 아홉 미션(`bc1menu76`~`bc9ssun_bars76`)을 눌러도 미션이 시작되지 않았다. 콘솔에 `Netstorm: 미션 'bc5walkers76' 은(는) 아직 연결되지 않았습니다.` 한 줄만 남고 메인 메뉴로 돌아갔다.
- 앱의 시작 허용 목록(`_beginCampaignFromDialog`)에 Tutorial 1~6 과 세 캠페인 묶음 16개만 있고 Priest Training 9개가 없었다. 대본·fort 자료는 이미 있었다(`originals/netstorm.tarc` 안에 `bc1menu76`~`bc9ssun_bars76` 의 `.english`·`.fort` 아홉 쌍이 모두 있다. `originals/d` 의 느슨한 파일은 76 이 없는 다른 이름 `BC5Walkers` 등이라 눈에 띄지 않았다).
- **시험이 속았다**: `e2e/all-missions-start.spec.js`(31개 미션 시작)는 미션이 시작되지 않으면 메인 메뉴에서 45초 뒤 Auto-Demo 가 대신 데모를 띄우고 그 시계가 흐르는 것만 확인했다. 그래서 Priest Training 9개도 "통과"로 보였고 LEFT_JOBS 의 E1121 기록("31개 미션 진행")도 사실과 달랐다. 이번 점검의 처음 신호는 입력 없이 지켜본 bc5 가 약 6분 뒤 "NetStorm Demo — The Dark Prince of Rain…" 창으로 바뀐 것이었다.

## 수정

- `www/netstorm.js`: 시작 허용 목록에 `bc1menu76`·`bc2bridging76`·`bc3acid_bar76`·`bc4bow_rush76`·`bc5walkers76`·`bc6mows76`·`bc7sun_bars76`·`bc8fury76`·`bc9ssun_bars76` 을 더했다.
- `e2e/all-missions-start.spec.js`: `autoDemo: 0` 으로 Auto-Demo 가 가리지 못하게 하고, 미션마다 현재 Guide 의 대본 이름이 요청한 이름과 같고 데모 표시가 0 인지 확인한다. 허용 목록에서 빠진 미션이 있으면 이제 실패한다.

## 확인

- 전 미션 시작 e2e: 31개 미션 모두 요청한 대본(데모 아님)으로 시작해 시계가 흐름(6.2분 통과).
- 한국어 대본 `bc*76.korean` 도 이제 해당 미션에서 쓰인다(한국어 모드에서 시작).
- 객체 우클릭 메뉴 전수 점검(미션마다 객체 종류의 대표 하나를 우클릭, 임시 조사): The War Begins!·Surrounded·Vicious 와 Priest Training bc1·bc5 에서 선택 가능한 모든 객체가 메뉴를 열었고 콘솔 오류가 없었다.

- 장시간 진행(`campaign-long-run.spec.js`, E1158): Priest Training 아홉 미션을 150초씩 이어 진행해(26분) 오류 없이 통과했다. 이 e2e 도 `autoDemo: 0` 과 실제 대본 이름 확인을 더해 Auto-Demo 에 속지 않는다.
- 객체 우클릭 전수 점검 추가: bc8fury76·bc9ssun_bars76 도 선택 가능한 모든 객체가 메뉴를 열었고 오류가 없었다(bc6mows76 은 큰 장면에서 조사용 픽셀 스캔이 시간 초과, 게임 오류 아님).

## 남은 것

- Priest Training 미션을 끝까지 하는 흐름(승패 안내 뒤 `Again!`·`Next Mission` 등)은 이 단위에서 확인하지 않았다. 대본의 성공/실패 구역은 E1152 의 승패 흐름과 같은 경로를 쓴다.
