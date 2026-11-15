# 재기준선 신규 게임 규칙·스커미시 33단위 판정 (4-R ②-C)

후보 `225553df`에서 기준선 `1f00a25a` 뒤에 생긴 단위 33개를 Git
트리와 호출 위치로 확인했다. 아래는 모두 **port**이며, 코드를 옮겼거나
세션에 연결했다는 뜻은 아니다. 단계 표기는 실제 소비자와 후속 요구를
기준으로 한 배정이고 Tutorial1에서 먼저 요구하는 부분은 4-D에서 처리한다.

| 단위 | 확인한 소비자와 배정 |
| --- | --- |
| `AssetDeletionVtables` | `RawSurfaceAssetDeletion`의 실제 생성자별 삭제 가상표. E498 장기 삭제 연결과 함께 4-D에서 처리한다 |
| `ClientConstructionDevices`, `ClientConstructionGem`, `ClientConstructionGlobals`, `ClientConstructionPlacement`, `ClientConstructionRandomPlacement`, `ClientConstructionRegions` | 같은 Client·월드의 배치·영역·보상·난수 장치. Tutorial1 시작에 필요한 부분은 4-D, 입력·건설과 나머지는 6에서 연결한다 |
| `ClientHelpWord`, `HudPlayerButtonDraw` | `ClientLocalStateGlobals`의 도움말 키와 `RawMenuControls`의 PlayerInfo 버튼 표시. 각각 도움말·HUD 소비자 단계에서 연결한다 |
| `RawAviaryProcessData`, `RawBombHardener`, `RawBombHeal`, `RawBombTreason`, `RawManoSpell` | `RawCampaignSession`과 장기 Host가 사용하는 아비어리 직렬화·주문 분기. 월드 적재에 필요한 부분은 4-D, 주문·전투 후단은 7에서 연결한다 |
| `RawDaisDeletion`, `RawDaisProcessData`, `RawRitualRegular`, `RawVortexDeparture`, `RawCampaignVortex` | 같은 장기 저장소·Session의 다이스 삭제/처리·의식·신전 이탈. 4-D 선행분과 7의 실제 의식 연결을 구분한다 |
| `RawDecorationRegular`, `RawFactoryRegular`, `RawPriestRegular`, `RawResidenceRegular`, `RawSpriteRegular` | 장기 `RawSurface*` 소유자와 Session이 재사용하는 장식·공장·사제·거주 구역·Sprite 정기 본문. Tutorial1 선행분은 4-D, 나머지는 해당 소비자에서 연결한다 |
| `VortexCommandInput` | 게임 입력의 신전 상승·하강·파괴 분기. 메타 화면 전용 `viewmeta` 분기는 웹 제외이며 일반 입력 연결은 6·7에서 한다 |
| `AiAllianceSide`, `AiDifficulty` | 11차 적·동맹 판정과 AI 난이도. 캠페인과 스커미시별 설정은 순서 10에서 반영하며 그 전에는 임의 난이도 배율을 적용하지 않는다 |
| `SkirmishSetup`, `SkirmishStartIslands` | 12차 설정 화면·시작 대본과 시작 섬 후보. 웹은 순서 10에서 Outpost와 함께 이식한다 |
| `RawCentralIsland`, `RawIslandPartition`, `RawBattleIsland`, `RawBattleNuggetPlacement` | 일반 전투에 있던 생성기지만 `BuildSkirmishWorld`와 건설 난수 경로가 공유한다. 중앙 군도·참가자 섬·자원 배치는 스커미시 순서 10에서 연결한다 |

E498의 29개, E499의 33개와 이번 33개를 합쳐 신규 176개 중 95개를
판정했고 **81개**가 남는다. 메타 수신기·전송 및 일반 전투 시작 등은
단독/스커미시 도달을 더 확인해야 하므로 이번 목록에 넣지 않았다. 기존
이식분의 변경 37개 판정과 대응표 기준선 전환은 별도다.

Git 트리의 신규 단위 33개와 앞선 두 묶음과의 중복 부재를 확인했다.
현행 대응표 검사는 984행·오류 0건이고 Node v22.23.3·v24.22.3·
v26.13.0 전체 검사는 각각 4,966건 통과·실패 0이다. 이 단위에서 제품
코드를 바꾸거나 게임을 실행하지 않았다.
