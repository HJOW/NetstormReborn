# 실제 Edit 메뉴 연결 전의 확인된 접합 지점

2026-10-24 HJOW-Athlon에서 현재 제품을 읽기 검토했다. 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23이다. 단위383의 하위 모듈·World/Deck 단계와 실제 메뉴 완료를 구별하기 위한 후속 기록이며, 이 조사에서 GUI를 실행하거나 제품을 수정하지 않았다.

## 메뉴·선택·상태

- `UberGump.cpp`의 메인 Edit는 아직 `Tell,NotImplemented`다. 명령 조회/활성 판정/raw 메뉴 허용/문자열 처리에 Fort 선택 분배가 없다.
- `TellMapMenus`는 ReadFort `0x020000ca`와 행별 문자열 토큰을 생성한다. 현재 `ExpandMenus`는 미지원 명령으로 비활성화한다. 목록·열·체크 처리기는 재사용한다.
- `FortSelection`의 `DispatchFortSelectionCommand`/`QueueFortSelectionAction`/`SetPendingFortName`은 제품 호출자가 없다. 실제 Event의 Guide/Gump 소비 뒤 연결하고 event.y의 문자열 토큰을 그대로 전달해야 한다.
- 현재 `State::Post`는 선착순 한 건이고 원본 숫자 nextState는 덮어쓰기다. mode2→nextState1을 같은 실제144BYTE 상태 수명에 기록하고 다음 Tick에서 소비해야 한다.
- UberGump의 Storage/목록/FileSpec/pending 이름은 재사용한다. 실제 current Fort 이름/dirty/숫자 State 소유자는 아직 없으며 `RawFortCurrentName`도 제품 미연결이다. 논리 이름과 작업 파일 경로도 구별해야 한다.

## 파일과 월드 소유

`GameWorld`의 raw 생성자는 현재 로그인 또는 캠페인 `Load()`를 실행하고 deferConnection도 localLogin을 요구한다. ExistingEditor의 명시 지연 설치 분기를 추가해야 하며 로컬 연결로 우회하지 않는다.

`EditorFiles`는 UberGump 또는 Client 수명으로 소유한다. 보호 원문을 읽어 작업본을 Prepare한 뒤 **현재 작업본의 실제 바이트**를 입력으로 사용한다. 기존 작업본을 새 원문으로 덮지 않는다. Session의 최초 생성자 입력은 ExistingEditor, 같은 Storage/Client Badges/conversion, 실제 작업 파일 경로이며 저장은 실제 ReadOnlyForts 참조를 빌린다.

파일 읽기→State/Notify→Subscriber/config/title→World 순서를 유지한다. Session의 실제 ID/handle과 `RawFortSubscriberLoad`를 연결하고 이름만 읽는 `ReadSubscriberName`으로 대체하지 않는다. 새 ID의 초기화 플래그와 CRT도 Client/스레드의 기존 소비자와 공유해야 하며 Session마다 새로 만들지 않는다.

`MakeCampaignClientServices`에는 아직 `fortTechnologyUi`가 없다. 실제 선택정보0042f540 표시 경계를 연결한다. World/Deck 완료만으로 최종 loaded를 설정하지 않는다. 캠페인 Load 후반의 필요한 실제 후처리와 Editor 외곽을 마친 뒤 커서/선택/객체 메뉴를 활성화한다.

## 후속 UI와 원래 자연 관문

HUD의 Production/Minimap, `PaletteChanged`, UI→Session 순서의 해제를 재사용한다. `ActivatePageAction`/`TryTell`/`RefreshWindowTitle`/`OpenGameMenu`와 합성 조건에는 mission_/missionRuntime_ 전제가 남으므로 실제 Editor 월드 상태에 맞게 분기한다. 가짜 MissionRuntime을 만들지 않는다. `RawGameMenus`의 Editor 하위 메뉴는 있지만 Save as/Add Island/Set All Bridge 입력은 아직 미연결 예외다.

AI는 별도 누락으로 판정하지 않는다. Editor도 ConnectMotion→ConnectAiMovement→ConnectAiRuntime을 구성하며, 현재 battle=false에서 원문 AI 프레임이 자연 반환한다. 설치 중에 프레임을 먼저 등록하지 않는 수명 순서가 필요하다.

팔레트 계산 본체는 이미 여러 모듈에 복원돼 있으므로 재사용한다. 실제 전환 이름 부모/GamePalSpec/arg2=1 장치 효과/dirty 수명과 설치 중 HUD productionSync의 경계는 [checkpoint 이후 분석](cpp-existing-editor-after-checkpoint-20261024.md)에 기록했다. 선택 외곽과 최초 원문 관측은 [기존 파일·설치 외곽](cpp-existing-editor-install-outer-20261024.md)을 따른다.
