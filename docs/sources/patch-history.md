# `PatchFixs.txt` — Ticonderoga Entertainment 비공식 패치 이력 요약

> 원문: `originals/PatchFixs.txt` (892줄, Windows-1252, "Ticonderoga Entertainment Official Patch Documentation").
> 순서: 최신(10.77/10.78) → 오래된 것(10.70 이전). 보유 exe 의 버전 판단은 [README.md](README.md) 2절.
> 클론 구현에 영향을 주는 항목만 주제별로 모았다. 버그 수정·서버·채팅 세부는 원문을 본다.

## 1. 버전 목록

| 버전 | 주요 내용 |
|---|---|
| 10.77/10.78 | `$Input` 숫자 버그, 알타 5000SP 반복 치트 수정, manabolt 아이콘을 보석으로, **셰이프에서 숨은 갈매기 그림 제거**, Zone 정보 시스템 |
| 10.75/10.76 | 주문 제외 옵션, 가이저 수·배치·재생성 옵션, **Resource Injections**, **Island Dynamics**(나무·폐허·기념비 무작위 생성), Game Type 옵션, 사용자 캠페인 메뉴, 주인 없는 다리는 미니맵에 회색. Ice Tower·Devil Maker·Acid Barricade 조정 |
| 10.73/10.74 | 승리 조건 코드 수정, 공중/지상 피해 구분, 비행체 목표 규칙, Arc Spire 가 비행체 공격 안 함. Arc Spire·Disc Thrower·Air Ship·Man o'War·Graviton 조정 |
| 10.72 | R.exe·setup.cfg 지원, Man o'War hp 175→165, Zone 메뉴 제거 |
| 10.71 V1.0~1.5 | Thunderstorm·Thunder Strike 주문, 새 소리, 편집기 다리 상태 메뉴(Cracked/Normal/Hard), **반시계 회전**, 튜토리얼 2·3·6 마나 표시 수정, Enemy Territory 난이도 |
| 10.70 V4.4~9.4 | 사제 Pray·주문서, Gravitation·Meteor·Hydra 계열, **섬 테마 그래픽(신전 원소별)**, 거주지·Edge Farm 테마, 치트 방지(**tarc 우선**), 채팅 도우미·길드, 스크립트 조건 연산자, 해상도 제거, 키 변경 |
| 10.70 이전 | 새 전투 옵션 다수, 새 다리 모양, 게임 타이머, 가이저 잔량 프레임, Send Storm Power, 주문 Whirlwind/Twister/Vortex, `$Input` 명령 |

## 2. 그래픽·화면

* **섬 테마 = 신전 원소** (10.70 V5.3 "different island textures for different themed vortexes", V5.4~5.5 wind·rain·thunder 섬 그래픽과 Edge Farm).
  V6.0: 원소 전환 시 그래픽 알고리즘 수정, **유닛당 프레임 수 상한 증가**(새 섬 테마용), **거주지가 섬 소유자·테마를 따라 바뀜**, Edge Farm 테마 수정, 신전을 회수했을 때 섬이 기본 테마로 돌아가는 버그 수정.
  → 우리 분석(신전 원소 → 지면 테마, 거주지 원소별 그림)과 일치 ([dissolved-alliance-start.md](../screens/dissolved-alliance-start.md)).
* 섬 테마 켜기/끄기: **Shift+F3** (V9.0), View 메뉴 Hide/Show Island Theme (V7.0).
* **1152×864, 1280×960 해상도 제거** (V9.0) → 한때 4:3 고해상도를 지원했다가 없앴다.
* 섬·다리가 소유자 색으로 표시 (10.70 이전), 주인 없는 다리는 미니맵 회색 (10.75).
* **가이저 잔량에 따라 프레임이 바뀜** (1 ~ 1/2, 1/2 ~ 1/4, 1/4 ~ 0) (10.70 이전).
* 상태 막대: 높이 줄임, 폭 = 유닛 폭, 마비·투명 타이머 막대 추가 (V9.0). 기도·시전 타이머 막대 (V4.4).
* 결정(nugget)이 공중에 떠 있지 않고 떨어짐 (V7.0).
* 스크린샷은 `ScreenShot` 폴더에 저장, 폴더가 없으면 충돌 (V6.4).

## 3. 입력 (키)

| 키 | 동작 | 버전 |
|---|---|---|
| R | 사제 선택 (V9.0) — V7.0 에서는 시계 방향 회전 | V9.0 |
| C | 반시계 회전 | V9.0 |
| ← → | 조각 회전 | 10.70 이전 |
| E | 다음 다리 조각 자동 선택 | 10.70 이전 |
| D | 마지막으로 지은 유닛을 하나 더 | V9.0 |
| P | 사제 선택 (한 번 더 누르면 사제 위치로 이동) | V7.0 |
| Y | 4번째 다리 조각 (V5.2 에서 다리 선택 제거 → V6.0 에서 4번째 조각 선택으로 다시 추가) | V6.0 |
| Shift+F3 | 섬 테마 표시 전환 | V9.0 |
| F12 | 소프트웨어 마우스(Safe & Jumpy Mouse) 전환 | README.DOC |
| Shift+2, Shift+3 | 영국 키보드에서 위치 저장 | V7.3 |

원판 조작은 [game-manual.md](game-manual.md) 4절과 [공식 PDF 선별 대조](pdf-manual.md#조작생산-절차).

## 4. 스크립트·텍스트 형식

* 헤더 크기: `<h1>` ~4, `<h2>` ~5, `<h3>` ~3, `<h4>` ~1 (`~숫자` = 글꼴 크기 코드). 영어 스크립트에서 큰 글꼴 `~4` 허용 (V7.0).
* **조건 연산자** (V7.0): `<?? X=Y>` 같음, `<??! X=Y>`·`<?!? X=Y>` 다름, `<?G`·`<?GE`·`<?L`·`<?LE` 비교, `<?G!`·`<?GE!`·`<?L!`·`<?LE!` 부정 — 구현 완료된 `MissionConditions` 와 대조할 것 ([mission-script.md](../formats/mission-script.md)).
* 메뉴 목록 명령 접두어 `%` (`@` 명령과 비슷) (10.75), 키워드 `#` 접두어 (V6.4), 스크립트 명령 `Warning`·`DontChatTo`·`RefreshPlayers`·`$Input`·`$Zsend`.
* 채팅 서식: `~u`(연초록) `~p`(연분홍) `~a`(연주황) `~s`(연보라) `~d`(청록) 색, `~U` 밑줄, `~S` 취소선, `~[C.#]` 팔레트 색, `~[B.#]` 배경색, `~[E.XxY]` 엠보스 위치, `~[S.소리]` 소리, `~[Iicon.D0]` 아이콘.
* `.type` 명령 추가: `casttime`, `praytime`, `manacost`, 주문 지속 시간·생성 수 조정 명령.
* `help.english`·`tell.english` 갱신, BlueCheese 의 Priest Training 미션 추가 (V9.2).

## 5. 유닛·주문 수치 변화

원판 매뉴얼 → 1997-10 README.DOC → 패치 순서. PDF에 적힌 선별 비용과 현재 값의 차이는 [PDF 비용 대조](pdf-manual.md#원판-비용과-보유-패치판의-차이)에 정리했다. 최종값은 현재 `.type` ([game-manual.md](game-manual.md) 7절 표).

| 대상 | 변화 |
|---|---|
| Sun Barricade | README.DOC: 200sp, 1400 hits, **Level 1 (Sun 1)** (원판 Level III 600sp) → 현재 `.type` 300 · 700hp · level 1 |
| Whirligig | README.DOC: hits 25 |
| Dust Devil | README.DOC: damage 12 |
| Acid Barricade | README.DOC: hits 1400 / 10.75: 연결되면 공격하는 비행체를 기둥 위로 빨아들임 / 이전: 지상 1×1, 공중 4×4 격자 파괴 |
| Ice Tower | README.DOC: 600sp / 10.75: 비용 600→800, hp 800→1060 / 10.73: Devastation 두 번에 섬까지 완전 파괴 |
| Man o' War Pool | README.DOC: 600sp / V6.2: hp 175→200 |
| Man o' War | V6.2: 200→175, 10.72: 175→165, 10.73: 165→150, 피해 16 (V9.4) |
| Bulf | README.DOC: hits 800 |
| Arc Spire | README.DOC: 1600 → V1.5: 1300→1200 → 10.73: 1200→1000, DPS 50→45 / 3×3 범위 피해, 여러 목표 / 10.73: 비행체 공격 안 함 |
| Devil Maker | 10.75: hp 600→400 |
| Sun Disc Thrower | 10.73: hp 400→300, 지상 DPS 13→15, 공중 DPS 20 |
| Air Ship | 10.73: hp 800→655 |
| Graviton | 10.73: 시전 5→1, 범위 26→5, 비용 1000→500 (V1.1 에서 범위 26 으로 낮춤) |
| 새 주문 | Meteor → **Bombard** 1100 L2, Whirlwind 550, Twister 1100 L2, Vortex 1650 L3, Hydra 500, Hydra Wave 1000 L2, Hydra Flood 1500 L3 (V1.1), Thunder Strike 1800, Thunderstorm |
| Rank | 원판: Rank 마다 +25% → 10.70 이전: "Rank no longer gives a player a advantage" |

※ 주문의 "L2·L3" 는 패치 문서의 레벨 표기이며 `.type` 의 주문 level 값(예: Vortex 1, Whirlwind 3)과 순서가 반대인 경우가 있다 → 주문 level 의 의미는 exe 확인 필요.

## 6. 전투 옵션 (BattleMaster)

10.70 이전에 Starting Money(5000/6500/7000)·Map Size·Map Density·Geyser Amount·Amount per Geyser·Map Mode·Spells·Watcher·Auto Alliance·Player Handicap 추가,
10.75 에 Resource Injections·Game Type·Island Dynamics·Geyser Placement·Geyser Respawn·주문 제외 추가. Geyser Amount 에 High/Extreme 추가 (V6.0).
exe 의 옵션 표와 대조: [battle-options.md](../exe/battle-options.md).

## 7. 파일·설정 관련 (분석 결과와 대조 필요)

* **10.70 V5.3 "Netstorm defaults to the tarc files rather than loading the hard drive files first"** (치트 방지).
  → 우리 정적 분석([vfs.md](../formats/vfs.md))은 "데이터 폴더 디스크 → tarc" 순서였다. 적용 범위(모든 파일인지, 일부 종류인지)를 exe·동적 분석으로 다시 확인해야 한다.
* V9.0 에서 **CFG 명령 다수 제거**: `cheat`, `debugCheat`, `noBridgeDeath`, `noIslandDeath`, `noBridges`, `kaboom`, `stuffRefreshRate`, `simpleBridgeReplace`, `alwaysUseBridgeCanon`, **`bridgeDrawRate`**, `dontImportShapes`, `buildOnEnemyBridge`, `quickVortex`, `quickBuild`, `allowEarlyCapture`, `printMoney`, `priestPickupPriest`, `archipelago`, `superAlly`, `inventoryLimit`, `useSystemTimerClock` 등.
  → `options.cfg` 에 `bridgeDrawRate`·`stuffRefreshRate` 가 남아 있어도 패치판 exe 는 무시할 수 있다 (exe 의 설정 키 등록 목록으로 확인).
* 치트·디버그 메뉴 제거, 스크립트 명령 `ChangeMoney`·`CreateAt`·`SetOwner`·`GetFullDeck` 등 보안 강화.
* `R.exe`(자동 업데이트), `TMaker.exe`(모든 레벨을 가진 요새 생성), `nsLaunchC.exe`(온라인 접속 실행기).
