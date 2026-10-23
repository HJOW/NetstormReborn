# HideBuildings 표시/선택 및 ReloadTheams 연결점

기존 정밀 원문은 `VM-W11-Codex / 2026-10-23`의 `extracted/refined/originals/Netstorm.c`다. 같은 보호10.78 PE와 기존 SpriteDraw/지역 소유권 본체를 재사용한다. 새 디컴파일/실제 게임/GUI/설치는 하지 않았다.

## HideBuildings

원본 `00498220`은 `0059a8ac`을 그림자 분기(원문122360), 일반 본체 분기(122376), 파생 가상 본체 분기(122441)에서 조회한다. 숨김 상태에서는 그림자를 생략하고, `005424b4`의 TYPE flags2 마스크 또는 TYPE `005412e8/005411a8`에 해당하는 본체를 Abstract 색으로 표시한다. dead/abstract/로컬owner/가상Draw의 별도 관문은 원본 분기대로 유지된다.

이 본체는 기존 `client/SpriteDraw.cpp`의 `SpriteDrawState::specialMode`로 복원돼 있다. `RawCampaignFactoryDisplay::Sprites(...,localPlayer,hideBuildings)`가 같은 현재 메뉴DWORD를 이 필드에 공급하도록 연결했다. 기존 `filteredFlags=2421574144`, `wideOwnerType=164`, `unfilteredType=84`와 실제 TYPE/풀/색 표/가상Draw를 재사용한다. 원본은 건물 그림 자체를 전부 없애는 동작이 아니므로 단순 스프라이트 삭제로 대체하지 않는다.

`GameWorld::BindHideBuildings(current)`는 실제 `0059a8ac`의 현재 조회기를 설치한다. Client는 이 월드를 소유하는 동안 살아 있는 메뉴DWORD의 getter를 연결해야 한다. 초기 입력은 원본 BSS0이며, 빈 조회기를 명시 설치하는 요청은 거부한다. 매 Sprites 호출과 매 선택 후보의 관문에서 현재 값을 읽어 토글 값을 복사해 보관하지 않는다.

선택의 원문 `004d5840`(168112~168116)은 `hide==0 || (!(currentType.flags2&mask) && currentType!=164 && currentType!=84)`일 때만 나머지 후보 판정을 계속한다. GameWorld::Pick의 기존 Bounds/근접거리 후보 앞에 이 관문을 추가했다. 같은 Session의 현재 raw TYPE와 현재 TYPE flags2를 읽는다. 기존 선택의 전체 hash/거리/후보/Carrier/추가 조건은 여전히 근사 구현이므로 이 변경을 `004d5840` 전체 복원으로 표시하지 않는다.

새 `HideBuildingsTests.cpp`는 실제 thewarbegins 월드의 RainTemple SID15002를 사용해, 한 번 바인딩한 같은 DWORD의 0→최상위비트 비영→0 변경이 실제 본체 픽셀과 선택 후보를 바꾸고 복원하는지 검사한다. 현재 raw TYPE84/164의 제외 및 빈 getter 거부도 확인한다. 부모가 요청한 빌드 금지 경계 때문에 이 신규 검사는 이 작성 시점에 컴파일/실행하지 않았다. 부모의 CMake 등록/제품 빌드/집중 검사 대상이다.

## ReloadTheams

원본 `004d62b0`의 `00542b54` 분기(169172~169187)는 Challenge `00594fc8`이 영점일 때만 두 목록을 순서대로 훑는다.

| 순서 | 원본 포인터/현재 signed count | 현재 Session 저장소 |
| --- | --- | --- |
| First | `0055a70c / 0055a714` | `assetState.outposts` |
| Second | `005e189c / 005e18a4` | `core.bookkeeping.factories` |

각 반복은 현재 목록의 현재 SID를 읽고 같은 슬롯의 float X `+0xe`, Y `+0x12`로 `00455380(x,y,0)`을 호출한다. 별도 사본/정렬/중복 제거/영역별 합치기를 하지 않는다. 하위 호출 뒤에는 현재 count와 다음 SID를 다시 읽는다.

`00455380`은 기존 `RawRegionOwnership::Update`와 `RawRegionPaint`로 복원돼 있고, 현재 Session이 소유하는 `RawCampaignOwnership::UpdateRegion(x,y)`가 같은 Pool/hash/프레임/난수/실제 두 목록/현재 theammode에 연결한다. 새 지역 알고리즘을 만들 필요가 없다. Session의 실제 저장소를 여는 좁은 getter/Update 입구와 GameMenuActions의 First/Second/reloadAt을 연결하는 작업은 동시 작업자가 담당한다.

이 문서는 메뉴 효과 연결 범위이며 전체 메뉴/HUD/선택 동일성 및 미션 전체 완주 완료를 뜻하지 않는다.
