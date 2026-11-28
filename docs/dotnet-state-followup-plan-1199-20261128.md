# 1.199 이후 State·HUD 재진입 선행

1.198 소스를 읽기 전용으로 조사한 후속 순서다. 구현/실행 완료가 아니며 기존 R5-4c3c → R5-4c4 → R5-4d → R5-3 → R5-5 → R6 순서를 바꾸는 판정도 아니다. State와 HUD 수명을 잇는 데 필요한 선행을 아래처럼 분할한다. 단위마다 팀 전체 제품/검사 소스를 동결한 뒤 통합 빌드하고, 정기 전체는 네 단위마다 수행한다.

## A. 기존 Native HUD 재사용

1. `ClientHostGame.RootFactories.cs`의 `EnsureRootFactories`는 현재 중복 예외를 낸다. 동일 장기 Storage/트리/문자 소유자를 검증하고 기존 공장 집합을 반환하는 경로가 필요하다. 새 NSInfo 노드 생성은 `CreateNativeKnowledgePanel`과 `RecreatePrimary`의 실제 소멸/생존 관문으로 분리한다. 살아 있는 첫 창의 중복 생성 오류를 약화하지 않는다. 첫 음향 소유자는 정상 LoadContent의 DrawStartupScreen/색표 준비에서 UI보다 먼저 만들어지므로 현행 정상 진입의 eager Audio 차단은 아니다.
2. `ClientHostGame.HudDividers.cs`·`MissionChat.cs`·`ProductionDeck.cs`·`Minimap.cs`의 Native 분기를 분리한다. 실제 Uber가 있으면 세 구분선과 각 패널이 현재 실제 owner와 같은지 확인하고 기존 기하/도킹을 재사용한다. 수동 구분선3개·채팅창 생성·slave 등록을 반복하지 않는다. Native 전체 공장이 작성하는 slave10개는 같은 생성자/HudDock가 소유한다.

| 현재 진입의 문제 | Native 경로의 다음 구현 |
| --- | --- |
| `EnsureMissionHudDividers`가 수동 별칭0을 보고3개를 새로 만듦 | 현재 Uber의 실제 세 구분선을 검증하고 수동 생성/임시 사건 전달기를 추가하지 않음 |
| `AttachMissionChat`가 수동 두 구분선·`_missionHudDock` 요구 및 무조건 Create | `root.Chat == NativeChatWindows.CurrentRoot` 검증 뒤 기존 Root에 `0x02000088,0,0` 숨김 사건 전달 |
| 생산 Ensure의 Resize가 Native 기하를 덮음 | 현재 `root.Production`과 실제 panel.RootToken 일치 검증·팔레트/예산 갱신만 수행 |
| 미니맵 Ensure가80×75/(0,H-75)를 강제 | 현재 `root.Minimap` 검증·팔레트/물리 통지/Refresh만 수행 |

미니맵 Construct 중복 관문은 유지한다. 실제 `KillGumps → Cleanup → ReplaceUber`가 이전 노드를 반납해야 한다. Native HUD 모드 전환도 실제 `HudDock.Handle(0x0200003d,…)` 호출자 순서를 따른다. 이 재사용 검증과 전체 State GUI 왕복은 별도다.

## B. 음악 채널과 State 외부 장치

1. `MusicChannelState` 실제96바이트와 Active/SetVolume/Close/Stop을 이식한다. 현재 음소거396행에 정지/닫기72행을 더한 `musiccontrol`468행(판본별156)을 우선 대조한다. 원시 상태의 패딩·이웃 필드는 보존하고 CLR 잠금은 배열 밖에 둔다.
2. Rewind/Read/Start로 `musiccontrol`5,292행을 닫는다. 명시 IO 경계로 검사할 수 있으며 실제 파일 Open/worker를 성공으로 합성하지 않는다.
3. 실제 열린 파일 토큰 소유자와 Channel.Open/EnsureBuffer로 `musicopen`1,872행 및 `musicselection`2,064행을 닫는다. 현재 선택 검증372행 외 나머지1,692행은 이때 확인한다. `WindowsSoundFiles.ReadWave`는 전체 표본을 읽고 즉시 닫으므로 스트리밍 파일 소유자로 대신할 수 없다. 기존 경로 resolver와 Winmm 규칙을 재사용하되 열린 파일은 재사용하지 않는 DWORD 토큰으로 관리한다.
4. 동일 WindowsSoundDevice/버퍼 토큰·COM ABI에서 음악용 버퍼 생성·개별 Release·HRESULT 상태/Restore·재생/쓰기 커서·두 구간 Lock/Unlock을 추가해 FillBuffer/`musicstream`5,088행을 잇는다. 이후 두 채널 초기화·500ms worker·종료 합류와 `musicruntime`4,800행을 잇는다.

96바이트 배치: +00 버퍼·+04 크기·+08 활성/반복·+0c 파일·+10 길이·+18 duration8바이트·+20 읽기·+24 쓰기·+28 WAVEFORMATEX18바이트·+3c 데이터 시작·+40 정지 깊이다. +14/+3a 패딩과+44 이후를 보존한다. 기존 `SceneMusic.ChannelDuration`(+18 조회)을 재사용한다. 실제 SceneMusic은 같은 현재 이름·Select·기본 채널 duration·음향/정지 소유자·공통 게임 난수를 빌린다. 무음도 이름 선택·파일 조회·원문 난수 판단을 실행한다.

이어서 실제 옵션 재해석 부모와 Pause/음소거/장면 음악/루프 정지 등을 같은 Root의 `ClientStateWorldDevices`에 조립한다. 무Runtime·비저장·비메타의 미선택 장치를 앞당겨 요구하지 않되 선택된 경계를 빈 성공으로 채우지 않는다. 미션 반납은1.196의 실제 Runtime callback을 사용하며 새 HUD/장기 Renderer·음향을 폐기하지 않는다.

## C. 실제 완전 적재는 별도 R6 의존

`HostSession.Load`의 실제9단계는 유일한 NativeLoaded 작성자이며 제품에서 아직 호출하지 않는다. 현재 `worldLoad.Load()`와 `CreatePrepared()` 공개만으로 이를 완료 처리하지 않는다. 원문 주석의 “여덟”은 실제 배열9개와 달라 해당 소비자 작업에서 교정할 수 있다.

| 단계 | 재사용/누락 |
| --- | --- |
| Building | HostMissionConstruction 실제 장치가 HostMissionBurning 생성에 섞여 있어 설치 위치/순서 분리 필요 |
| Motion | `HostSession.Motion` 본체 존재, 원문처럼 RuntimeLoad 앞으로 이동 필요 |
| RuntimeLoad | BuildWorldLoad 재사용·BeginLoad→깊이1→campaign.Load→FinishLoadAfterGraph→깊이0 부모 필요 |
| Devastation | RawCampaignDevastation의 주문 시스템 조립 필요; 파편/DropIsland로 대신하지 않음 |
| CarrierDrop | Session 명령·운반 부모 누락 |
| PriestPrayer | RawPriestPrayer 본체는 존재, 실제 세션/수신/명령 조립 누락 |
| Builder | 원문 MakeBuilder의 RawConstructionBuilder 본체·세션 설치 누락 |
| Placement | 농지 배치와 별개인 실제 사용자 ConnectGameplayPlacement 부모 필요 |
| Cursor | 세션 본체 존재, Game 호출과 미리보기/Gump/음향/배치 후크 미완료 |

아홉 실제 단계가 정상 복귀한 뒤에만 Load가 NativeLoaded를 공개하고, 현재 Runtime·미니맵·덤프를 연결한다. R5 수명 검증과 이 R6 플레이 적재 완료는 구분한다. 기존 미니맵1픽셀은 CPP의 다리 생성 시점과 관련되고 DN live AI 호출/Bridges·Drops가 아직 없으므로 임의 픽셀 보정이나 난수 앞당기기로 완료 처리하지 않는다. 같은 프레임은 같은 게임 시각이 아니다.
