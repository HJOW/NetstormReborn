# F7 섬 소유자 표식 토글 (WB04-E1090)

미션 중 F7과 게임 메뉴 View ▸ "Hide Island Ownership - F7."이 섬 소유자 표식(`useBanners`)을 뒤집고 월드를 다시 그린다. 이전에는 F7이 입력 큐에 들어가지 않았고 메뉴 항목은 비활성이었다.

## 구현

- **값의 소유자.** 원본 `005319c0`처럼 현재 `useBanners` 값은 소유자 색표 저장소(`ClientColorSortInput.ownerTables.enabled`)가 가진다. 메뉴 항목의 체크 상태(`config('useBanners')`)와 F7 분배기(`useBanners`)가 같은 값을 읽는다.
- **쓰기.** `_applyCampaignConfig(key, value)`가 설정 한 줄의 공통 적용 경로다. 설정 층의 키는 Settings가, 그 밖은 게임 데이터가 저장하고, `useBanners`는 추가로 `ownerTables.setEnabled(value, worldRedraw)`로 소유자 색표를 다시 만들고 월드를 다시 그린다(원본 `onChanged` → `43bc40 InterpretOwnerColorOptions`). 메뉴의 설정 명령(`0x6d`, `키=값`)과 F7(`writeBanners`)이 이 경로를 함께 쓴다.
- **입력.** F7(`0x760000`)을 브라우저 키 큐와 입력 프레임 허용 목록·표시 키 분배기(`dispatchViewKeyInput`)에 더했다. 정지 중에는 소비만 한다.

## 검증

새 `e2e/banners-key.spec.js`(Chromium): F7이 `useBanners`를 뒤집고 전체 다시 그리기를 요청하며, 한 번 더 누르면 원래 값으로 돌아오고, View ▸ Hide Island Ownership 항목이 활성이며 같은 값을 뒤집는다. `game-menu-bar.spec.js` 4건(설정 토글·음량 경로)도 통과한다.

## 남은 경계

Tutorial1에는 섬 소유자 표식이 그려지는 객체가 없어 화면 차이는 확인하지 못했다. 표식이 있는 미션(AI 섬)에서의 화면 비교는 남아 있다.
