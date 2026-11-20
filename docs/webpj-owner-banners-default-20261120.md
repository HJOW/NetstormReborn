# 섬 소유자 표식 기본값과 설정 저장 (WB04-E1093)

미션 화면의 섬 테두리가 원본과 다르게 주황색이던 차이를 고쳤다. 원본 스크린샷(`Bridge the Gap - Started.png`, `The War Begins! - Started.png`)은 섬 가장자리에 소유자 색(내 섬은 파랑)의 가는 테두리가 보이지만 웹 화면은 그 테두리가 없는 주황 가장자리였다.

## 원인

원본의 `Client::InterpretOwnerColorOptions`(`ClientMain.cpp`)는 설정이 바뀔 때마다(`configuration_.onChanged`) `useBanners`(F7 "Hide Island Ownership")를 `ConfigInterface::GetInt("useBanners")`로 읽어 소유자 색표 저장소에 적용한다. 키가 없으면 `GetInt`는 0을 돌려주므로 기본은 0, 곧 섬 소유자 색 표식을 보이는 쪽이다. 웹은 소유자 색표 저장소의 PE 초기값(1, 표식 숨김)을 그대로 쓰고 설정 값을 한 번도 적용하지 않았다.

## 수정

- **설정 적용.** `_syncClientOwnerColorOptions()`가 게임 데이터의 `useBanners`(없으면 0)를 소유자 색표 저장소에 적용한다. 색표 준비 직후와 설정 변경 통지(`_onSettingChange`·`_onConfigWrite`)마다 부르며, 준비 전에는 원본처럼 무시된다. 구름·도움말 단어 동기와 같은 자리다.
- **설정 저장.** `useBanners`를 설정 키(`toggle`, 기본 0)로 올려 F7과 View 메뉴 선택이 `netstorm.settings`에 저장된다(원본 `options.cfg`의 같은 키). 설정 키가 12개가 되어 `popup.test.mjs`·`first-run.spec.js`의 기대 목록을 갱신했다.
- **정리.** E1090의 `_applyCampaignConfig`는 설정 쓰기와 통지에 맡기고 `useBanners`일 때 전체 다시 그리기만 요청한다.

## 검증

- Chromium(`banners-key.spec.js`): 기본 `useBanners`가 0이고 F7이 1로 바꾸며 설정 저장소에 남고 한 번 더 누르면 돌아온다. 같은 조작을 View ▸ Hide Island Ownership가 한다.
- 화면: Tutorial1 시작 화면의 섬에 파란 테두리가 원본 스크린샷처럼 보인다. 5개 Chromium 파일 9건(첫 실행·좌측 HUD·F7·Reset All·게임 메뉴막대)이 통과한다.

## 남은 경계

원본 스크린샷과의 전체 화면 픽셀 비교는 아직 하지 않았다(카메라 원점과 Tutorial1 이후 상태 정렬이 필요). 소유자가 여럿인 섬(AI 섬)에서의 색표 대조도 남아 있다.
