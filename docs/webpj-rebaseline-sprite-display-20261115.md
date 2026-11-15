# Sprite 표시 경계 재판정

WB04-E505는 후보 `225553df`의 `SquidPop` 변경을 **② 웹 반영 필요**로 판정했다. 또한 E259가 웹 제외로 판정했던 `SquidUnpop`·`SquidDisplay`의 일반 Sprite 분기를 다시 판정했다. `RawCampaignSession`과 `RawSurfaceStorage`가 일반 Sprite `0x50cc08`을 처리하고 `ClientRendererFrame`이 같은 실제 Sprite 표시 갱신을 연결하므로 메타 화면 전용으로 묶을 수 없다.

Pop과 Unpop의 지원 판정은 메타 Sprite `0x50c9f0` 및 일반 Sprite `0x50cc08`을 두 표시 플래그에서 허용한다. `SquidDisplay.validate`는 이 전용 경로에서 공통 프레임 검사를 건너뛰고, `update`는 현재 슬롯을 선택 장치 `metaRefresh`에 직접 전달한다. 8c도 같은 갱신 본문으로 이어진다. 장치가 없으면 해당 위치에서 명시 오류다.

두 신규 단위 `ClientMetaSpriteRefresh`·`NativeMetaSpriteRefresh`도 exclude-mp에서 port로 정정했다. 이 파일의 일반 Sprite 갱신은 사각형 확대·dirty·슬롯 BYTE 35 및 Form/Gump 갱신을 수행한다. **그 실제 본문과 Client/World 연결은 아직 미이식**이며 4-D/5-C에 남는다. 메타 화면 전용 부분은 제외한다. 지원 판정이나 훅만으로 게임 화면이 완성된 것은 아니다.

기존 10.78 Pop 2,456단계·표 83행·Unpop 1,300행·표시 및 간헐천 관측을 보존한다. 추가 검사는 두 가상 표와 두 표시 플래그, 공통 프레임 검사 생략, 장치의 단일 조회, 같은 슬롯 전달과 장치 누락을 확인한다.

첫 전체 검사에서 기존 Unpop 83행 검사가 원본 공통 표시 마스크만 기대해 새 전용 Sprite 지원 두 건을 거부했다. 원본 fixture와 마스크는 변경하지 않고 Pop 검사와 마찬가지로 별도 복원한 경로를 기대값에 포함한 뒤 관련 검사를 세 Node 버전에서 다시 통과시켰다.
