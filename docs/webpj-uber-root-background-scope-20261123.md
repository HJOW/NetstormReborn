# Uber 루트의 배경 GIF 메시지 자식(0xb0)·NSInfo 자식의 웹 범위 판정 (WB04-E1243)

E1224 읽기 전용 감사가 `RawUberRootControls` 행에 남긴 두 차이 — ① 0xb0 배경 GIF 메시지 자식이 웹 Uber 루트에 없다, ② 지식(NSInfo) 자식 공장이 없다 — 가 웹에서 도달하는지 소스로 판정했다. **둘 다 웹 범위 밖이며 행을 닫는다.**

## 근거

1. **0xb0 메시지 자식은 배경 이름이 있을 때만 만든다.** 원문 `UberRootGump` 생성(웹 `ui/uber-root-gump.js:119`, cpppj `RawUberRootControls.cpp:33`의 `Allocate(0xb0)`)은 `0x565e70`(현재 배경 이름 NUL 문자열) 첫 BYTE가 0이 아닐 때만 배경 GIF를 메시지 자식으로 붙인다. 이 문자열은 State 3(`native-main-menu-state.js`)·State 6(`native-battle-return-state.js`)이 `titleMenu`·`victory`·`defeat`로 쓰는 메인 메뉴 배경 이름이다.
2. **웹의 메인 메뉴는 이 자식 없이 같은 배경을 직접 그린다.** 웹 메인 메뉴(`ui/main-menu.js`)는 Uber 루트의 메시지 자식 대신 배경을 직접 그리며, 영어 4:3 메인 메뉴 M1 8장면은 cpppj 오라클과 알려진 구름 배경 등 차이 영역 밖에서 픽셀 차이 0이다(0.1절). 클릭하여 시작 뒤 title 그림 세 장(E1184)도 같은 방식이다.
3. **미션 HUD의 Uber 루트는 배경 이름이 0이다.** 웹 `ClientUberHud`의 현재 전역 읽기는 `0x565e70`을 0으로 두고 할당 분기(`#allocate`)는 루트 크기와 구분선 크기만 받으며 0xb0은 명시 오류다. 원본·cpppj도 미션 HUD에서 0xb0을 만들지 않고(없으면 `BackgroundGifs` 미설치 예외가 모든 미션을 막았을 것이다) 모든 캠페인 31·데모 3·스커미시 시작과 장시간 진행(E1121~E1125)이 이 경로를 지난다.
4. **`victory`·`defeat` 배경은 단독 미션에서 쓰이지 않는다.** 결과 DWORD가 1·2가 되는 경로는 멀티플레이 전투 종료뿐이다(E1238).
5. **NSInfo 자식은 멀티/메타 준비 창이다.** HUD 모드 4(지식 NSInfo 공장·`HudNsInfoMeta`)는 단독 미션에서 쓰이지 않는다(E1171 [판정](webpj-nsinfo-scope-20261122.md)). `RawUberRootLookup.current`·`UberRootGump.recreate`는 호출처가 없다.

## 결론

- `RawUberRootControls.cpp` 행은 partial → port로 닫고 비고에 위 근거를 더했다. 인계 5-C의 'Uber 허브' 체크를 완료로 바꿨다.
- 멀티플레이 메타 메뉴·`victory`/`defeat` 결과 배경을 구현할 때 `BackgroundGifs` 장치와 0xb0 할당·메시지 이벤트 연결을 함께 옮긴다.
- 이 단위는 코드를 바꾸지 않았다(소스 판독 판정). 기준선 뒤 cpppj 변경 없음.

## 대응표 정리

partial 단위 30 → 29, 줄 수 20,436 → 20,275(`python -X utf8 tools/port_inventory.py`).
