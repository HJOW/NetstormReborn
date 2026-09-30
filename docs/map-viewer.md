# 개발용 맵 뷰어

원본 `.fort`의 저장된 오브젝트를 16×11px/칸 좌표계로 표시한다.
원본 데이터 탐색은 기존 `GameDataLocator`를 사용한다 (`NETSTORM_DATA` 지정 가능).

저장소 루트에서 실행:

```powershell
dotnet run --project src/Netstorm.Game -- --map savetheisland
dotnet run --project src/Netstorm.Game -- --map thewarbegins
dotnet run --project src/Netstorm.Game -- --map originals/d/b0.fort
dotnet run --project src/Netstorm.Game -- --map savetheisland --language korean
```

`--map`이 없으면 기존 애니메이션 확인 화면이다. 파일을 명시하면 해당 파일을 읽고,
맵 이름을 지정하면 설정의 `fortSpec`으로 경로를 만들고 공통 `GameFileSystem`에서 찾는다.
조회 순서는 원본 정적 분석과 같은 데이터 폴더 디스크 → 이름순 `*.tarc` → 보조 폴더다.
느슨한 파일 탐색과 아카이브 이름 검색은 대소문자를 무시한다.
설정은 options → setup의 첫 일치 우선 규칙을 사용하고, 맵 팔레트는 `battlePal`·`GamePalSpec`에서 선택한다.
타입 정의도 공통 파일 시스템에서 읽는다. 언어 선택·미션 대체와 검증 범위는
[실행 자산·설정·언어 연결](runtime-resources.md)에 정리했다.

* 방향키 또는 마우스 우클릭 드래그: 카메라 이동
* **전체화면에서 마우스를 화면 끝(맨 바깥 1픽셀)에 대면 카메라 이동** (원본과 같은 규칙·속도 곡선, 월드 밖으로는 나가지 않음) — [분석](exe/edge-scroll.md)
* 마우스 휠: 0.25~4배 확대
* Home: 시작 카메라로 복귀 — 원본 미션 시작 화면처럼 플레이어 1 사제 칸 기준점을 창 중심에서 (+13, +9) 떨어진 곳에 둔다.
  논리 해상도가 1024×768 이면(4:3 창이거나 `--wide letterbox` 일 때) 사제가 원본 캡처와 같은 (525, 393) 에 그려져, 뷰어 캡처를 원본 캡처와 바로 겹쳐 볼 수 있다 ([측정](screens/dissolved-alliance-start.md) 2절).
  안내 영역이 4줄로 늘어(높이 128) 화면 맨 위 128 논리 픽셀은 안내가 덮는다 (사제 위치 계산은 안내 높이를 보정하므로 그대로).
  와이드 시야 확장에서는 화면 중심이 넓어진 만큼 사제가 가운데 쪽으로 온다
* F4: 현재 사제 위치로 화면을 옮긴다. 튜토리얼 1의 첫 단계 신호로도 처리한다.
* G: 진단용 청크 윤곽 표시 전환
* 오브젝트 기준점 근처에 마우스: 타입, 좌표, 영역, 소유자, 다리 값 표시
* Esc: 종료

## 게임 세션과 미션 모드

뷰어는 규칙 상태를 직접 갖지 않는다. 입력은 **게임 세션**(`BattleSession`, [core-rules.md](core-rules.md) "게임 세션")에 명령으로 넣고,
세션이 고정 틱(24Hz)으로 진행한 결과(오브젝트·다리·Storm Power·알림)를 그린다.

| 실행 방법 | 세션 시간 | 생산 규칙 (기술 허용 표·덱 등록·재충전·회수 금지) | 시작 Storm Power·지식 |
|---|---|---|---|
| `--mission 이름` (예: `tutorial1`, `tutorial2`) | 계속 흐름 | 켜짐 | 미션 머리의 `myStartMoney`·`myTech`, 전투 옵션 덮어쓰기(튜토리얼 2) |
| `--map 이름` | P·B 모드가 켜진 동안만 흐름 (맵만 볼 때 저장된 다리가 무너지지 않도록) | 꺼짐 = **시험 모드** (규칙 조건만 맞으면 어떤 유닛이든 놓는다) | 맵의 `Money` 섹션 |

* `--mission`은 미션 스크립트에서 맵(`loadFort`)과 시작 조건을 읽어 연다. `--map`과 함께 쓸 수 없다.
* `Space`: 세션 일시정지·재개. `K`: 생산 규칙 켜기/끄기. 일시정지 중에도 명령을 내리면 한 틱만 진행해 결과를 보여 준다.
* 오른쪽 위 상자에 Storm Power(원본 색 규칙 ≤1000 빨강, ≤2000 노랑), 게임 시각(틱), 생산 규칙 상태, 미션 제목이 표시된다.
* **튜토리얼 1·2는 세션이 단계 처리를 한다**(`TutorialStages`, [core-rules.md](core-rules.md)). 튜토리얼 1은 F4/다리 배치 → 다리 8·19칸 → 가이저 연결 → 200·600 SP로 G까지 진행한다. 저장 맵에 없는 연습 가이저와 받침을 시작 시 생성한다(위치는 근사).
  가이저 위에 마우스를 두고 **H**를 누르면 사제가 반복해서 결정을 수확·전달한다. 전달당 200 SP다. 다리는 B 모드에서 놓는다. 다리 8·19칸 기준은 현재 살아 있는 내 다리 칸 수로 근사한다.
* 튜토리얼 2에서 미션 머리의 `techAllowed`·`denySalvage`는 시작 값이고 원본은 튜토리얼이 진행 중에 바꾼다([mission-header-flags.md](exe/mission-header-flags.md)).
  템플을 지으면 단계 B에서 Sun Workshop이 허용되고, 유닛 네 개를 놓으면 단계 H에서 회수 금지가 풀린다. 오른쪽 위 상자에 현재 단계와 선택한 오브젝트가 나오고, 단계가 넘어갈 때 해당 미션 스크립트의 안내 창이 열린다.
  단계 C·F는 **선택한 템플**을 본다: `T` 키로 커서 칸의 오브젝트를 선택/해제한다(`--script`는 `select x,y`·`select none`).
  튜토리얼 3~6의 단계 처리는 아직 없다.

### 튜토리얼 안내 창

`TutorialTell` 이벤트의 섹션을 원본 미션 스크립트에서 읽어 제목·본문·`$Button=` 버튼을 표시한다. 창이 열려 있는 동안 세션 시간과 지도 입력은 멈춘다. MORE/BACK 버튼은 스크립트의 다른 섹션을 열고, OK는 창을 닫는다. 마지막 단계의 Leave Tutorials는 미션 화면을 떠나고 Next Tutorial은 지정된 다음 미션을 연다.
단계 처리 객체가 아직 없는 튜토리얼 3~6도 시작할 때 A. 안내를 연다. 이후 단계가 자동으로 넘어가지는 않는다.

* F8: 현재 단계의 시작 안내를 다시 연다. MORE/BACK으로 이동하거나 보정 안내(`NotVortex` 등)를 본 뒤에도 단계 시작으로 돌아간다.
* Enter·Space 또는 좌클릭: 선택한 버튼 실행. Tab·좌우 방향키: 버튼 선택.
* 마우스 휠·상하 방향키·PageUp·PageDown: 긴 본문 스크롤. Esc: 안내 창 닫기. 창이 닫힌 상태의 Esc는 게임 종료.
* `<h1>`~`<h4>`, `<p>`, `<br>`, `<i>`, `<c>` 등의 제목·간격·강조를 표시한다. 원본 그림 명령(`<!...>`)은 현재 `[그림: 이름]` 자리표시자로 보인다. 원본 창의 그림·정확한 배치 재현은 후속 작업이다.

튜토리얼 2의 A~I 단계 안내와 버튼은 원본 스크립트로 정적 검사했다. 그래픽 창의 실제 배치·마우스 입력은 이번 작업에서 실행 검증하지 않았다.

검증용 명령 스크립트 `--script "명령; 명령"`: 시작할 때 세션 명령을 차례로 실행하고, 결과를 콘솔에 출력한다.

| 명령 | 동작 |
|---|---|
| `construct 타입 x,y` | 사제가 건물(템플·워크샵·알타·아웃포스트)을 짓기 시작 |
| `harvest x,y` | 그 칸의 가이저에 사제를 보내 반복 수확 |
| `home` | F4 화면 복귀와 같은 튜토리얼 1 신호 |
| `place 타입 x,y` | 생산 창의 유닛을 배치 |
| `register 타입` | 그 유닛을 받을 수 있는 첫 워크샵에 지식으로 등록 |
| `salvage x,y` | 그 칸의 내 오브젝트를 회수 |
| `wait 초` | 게임 시간을 진행 |
| `select x,y` / `select none` | 그 칸의 오브젝트를 선택 / 선택 해제 (튜토리얼 2 단계 C·F가 선택한 템플을 본다) |
| `allow 타입` / `deny 타입` | 기술 허용 표를 바꾼다 (단계 처리가 없는 미션에서 손으로 재현) |
| `denysalvage 0\|1` | 회수 금지를 바꾼다 (같은 용도) |
| `rules 0\|1` | 생산 규칙 끄기/켜기 |

튜토리얼 2 흐름 확인 (2026-09-30): 관찰한 Storm Power 10,000 → 5,000(템플) → 4,200(워크샵) → 유닛 300씩 → 회수 75와 같은 값이 나온다.

```powershell
dotnet run --project src/Netstorm.Game -- --mission tutorial2 --window 1024x768 --script "construct windVortex 40,32; wait 17; construct sunFactory 52,32; wait 11; register sunArcher; place sunArcher 43,32; wait 2; place sunArcher 38,35; wait 1; denysalvage 0; salvage 38,35" --screenshot extracted/screens/session-tutorial2.png --screenshot-frames 45
```

콘솔에는 `Wind Temple 완공`(17초), `Sun Workshop 완공`(28초), 등록, 배치(−300)×2, 회수(+75)가 차례로 나오고 화면의 Storm Power는 3,675다.
(단계 처리가 생긴 뒤로 `allow sunFactory`는 필요 없다. 단계 H는 유닛 네 개를 놓아야 오므로 이 짧은 스크립트는 `denysalvage 0`으로 회수 금지를 손으로 풀었다.)

단계 처리로 A→G까지 걷는 확인(2026-09-30, Linux): 템플 완공 → `TutorialTell B.` → 워크샵 완공 → `C.` → 템플을 2초 선택 → `NotVortex` → 등록 → `D.` → 첫 배치 `E.` → 둘째 `F.` → 템플 4초 선택 → `G.`.
단계 H 전에는 회수가 `회수 금지 상태`로 거부된다. 스크립트: `construct windVortex 40,32; wait 17; construct sunFactory 52,32; wait 11; select 40,32; wait 2.5; register sunArcher; place sunArcher 43,32; wait 2; place sunArcher 38,35; wait 2; select 40,32; wait 4.5; place sunArcher 41,36; wait 1; salvage 41,36`.
좌표는 첫 번째로 유효한 칸이라 섬 가장자리에 붙는다. 건설 시간(템플 16초·워크샵 10초)은 관찰값(사제 이동 포함)을 그대로 쓴 임시 값이다([ConstructionTimes](../src/Netstorm.Core/Simulation/ConstructionTimes.cs)).

## 배치 시험 모드

**P** 로 켜고 끈다. 플레이어 1 로 워크샵 생산 유닛을 놓고 사제가 짓는 건물을 지어 보며 게임 규칙 코어([core-rules.md](core-rules.md))의 판정을 확인한다.

| 입력 | 동작 |
|---|---|
| `[` / `]` | 타입 선택 (유닛 27종 → 건물: 템플·워크샵·알타·아웃포스트, 원소·레벨 순) |
| 커서 | 커서 칸을 기준점(발자국 오른쪽 아래 칸)으로 판정 |
| 좌클릭 | 판정이 통과하면 명령을 넣는다 — 유닛은 배치(자리 점유, Storm Power 차감, Generator 는 공급원), 건물은 건설 시작(건설 시간 뒤 완성: 템플이면 섬 소유·다리 공급, 워크샵이면 등록 가능) |
| `F` | 고른 유닛을 받을 수 있는 첫 워크샵에 지식으로 등록 ("Put Knowledge into Production") |
| `Delete` | 커서 칸의 내 오브젝트 회수 (비용의 25%) |
| `C` | 빈 섬이 내 섬과 다리로 연결되었다고 강제로 가정 (다리 연결 판정을 건너뛰고 싶을 때) |

* 판정 순서: (미션이면 기술 허용 표·덱 등록·재충전) → 섬 위치(내 섬 / 다리로 연결된 빈 섬 / 내 다리 끝, 남의 섬 불가) → 빈 자리 → Storm Power → 에너지.
* 아군 공급원(템플·Generator)의 공급 반지름을 원소 색 점선 원으로, 요구 에너지에 배정된 공급원을 선으로 보여 준다.
* 아래에 선택한 타입·판정 결과·**덱 상태**(다리 칸, 골렘, 등록한 유닛과 재충전 남은 시간)·알림·조작 키가 5줄로 나온다. 건설 중인 건물은 반투명 그림과 진행 막대로 보인다.
* **빈 섬 연결 판정(2026-09-30)**: 세션이 다리 격자에서 계산한다. 내 다리 연결망 하나가 내 섬과 그 빈 섬에 함께 닿으면 연결된 것으로 본다(근사, [BridgeReach](../src/Netstorm.Core/Bridges/BridgeReach.cs)).
  다리 끝은 "발자국 둘레에 플레이어 다리 칸이 있는 섬 밖 위치"로 근사한다.

검증용 명령(커서 대신 칸을 지정하고 카메라를 그 칸으로 옮긴다):

```powershell
dotnet run --project src/Netstorm.Game -- --map dissolvedalliance --placement windwalker --probe 124,126 --screenshot extracted/screens/placement-windwalker.png
dotnet run --project src/Netstorm.Game -- --map dissolvedalliance --placement bulf --probe 124,126 --screenshot extracted/screens/placement-bulf.png
```

## 다리 조각 시험 모드

**B** 로 켜고 끈다(배치 시험 P 와 동시에 켜지지 않는다). 게임 세션이 원본 규칙대로 플레이어 1 의 다리 칸(`BridgeTray`)을 채운다.
집기·되돌리기·놓기는 모두 세션 명령이고(`PickBridgePieceCommand` 등), 조각의 회전은 화면이 관리해 놓을 때 값으로 보낸다.
세션은 모든 플레이어의 다리 칸을 같은 전역 난수로 채우므로(원본도 난수 하나를 공유) 다른 플레이어가 있는 맵에서는 플레이어 1 의 조각 순서가 이전 뷰어와 다르다.
규칙은 템플이 있을 때만 1초마다, 칸 수는 전투 옵션 Bridge Slots, 5번째 추첨마다 한 칸 조각이다([bridge-pieces.md](exe/bridge-pieces.md)).
왼쪽 패널에 칸이 2열로 표시되고, 조각은 원본 `bridge.type` 프레임으로 그린다.

| 입력 | 동작 |
|---|---|
| `1`~`6` | 그 칸의 조각을 집는다 (들고 있던 조각은 칸으로 되돌린다) |
| `R` | 들고 있는 조각을 90° 회전 (원본의 오른쪽 클릭. 기본 시계 방향) |
| `C` | 반대 회전 켜기/끄기 (원본 C 키와 같다. 켜지면 R 이 반시계 방향) |
| `Backspace` | 들고 있는 조각을 칸으로 되돌린다 |
| 좌클릭 | 커서가 가리키는 왼쪽 위 칸(원본 측정 규칙 `BridgeCursor`)에 놓는다. 놓을 수 없으면 이유만 알린다 |

* 들고 있는 조각은 커서 위치에 반투명하게 보이고, **놓을 수 없는 위치에서는 빨갛게** 보인다(원본은 순수 빨강 실루엣, 뷰어는 빨강을 곱한 반투명 그림). 아래 안내 줄에 "놓을 수 있음(연결 N)" 또는 불가 이유가 나온다.
* 배치 판정(2026-09-30, Core `BridgeGrid`): 섬 칸(본섬 미리보기·작은 받침)·다른 다리·다른 오브젝트 발자국과 겹치면 불가. 조각 밖을 향한 연결 방향이 섬 칸이나 내 다리의 마주 연결된 끝에 닿아야 한다. **초목이 있는 섬 가장자리에서는 시작할 수 없다**([섬 소유권 규칙](gameplay/island-ownership.md) 3번, 사용자 확인): 뷰어가 그리는 가장자리 초목(edgeFarm) 칸과 dropBlocking 오브젝트(건물·나무·신전·가이저 등) 발자국 칸 옆은 불가다([bridge-pieces.md](exe/bridge-pieces.md) 8.4절). 초목 칸 위치는 뷰어의 edgeFarm 미리보기(좌표 고정 근사)라 원본과 다를 수 있다. 영역 소유 조건은 아직 없다.
* 붕괴(2026-09-30): 모드가 켜진 동안 게임 시각 10초마다 다리 칸을 한 번씩 처리한다. 섬·다리 **끝에 있는 칸**(열린 쪽이 있는 칸)만 수명이 줄고, 접합 칸(갈래·모퉁이)은 그 끝 판자와 함께 줄어든다. 양쪽이 이어진 판자는 줄지 않는다. 5 아래에서 금 간 프레임, 0 이면 사라진다(저장 다리도 포함. 무너진 저장 다리는 그리지 않는다). 섬에서 떨어져 나온 5칸 미만 조각은 다음 처리에서 한꺼번에 사라진다. 단단한 끝 칸은 줄지 않는다. 규칙: [bridge-pieces.md](exe/bridge-pieces.md) 8.1절.
* 놓은 칸은 배치 시험의 "다리 끝"·빈 섬 연결 판정에도 쓰인다. 칸 패널의 조각은 원본 사이드바처럼 절반 크기로 그린다. 회전·커서 규칙은 원본 실행으로 확인한 것이다([bridge-pieces.md](exe/bridge-pieces.md) 4절).

검증용 명령(6초를 미리 흘려 칸을 채우고, 4번 조각을 회전 1 로 든 채 (118,122)에 둔다):

```powershell
dotnet run --project src/Netstorm.Game -- --map dissolvedalliance --bridges 6 --bridge-hold 4,1 --probe 118,122 --screenshot extracted/screens/bridge-tray-dissolvedalliance.png
```

2026-09-29 확인: 칸 6/6, 추첨 6회, 첫 조각은 한 칸, T 자·ㄱ 자 조각이 원본 프레임으로 이어져 보이고, 회전 1 의 4번 조각이 가로 막대 아래 가지 모양으로 그려진다.

배치 판정 확인 (2026-09-30, Bridge the Gap! 섬 오른쪽 가장자리 x = 61, 초목 규칙 반영 뒤 다시 확인):

```powershell
dotnet run --project src/Netstorm.Game -- --map bridgethegap --bridges 6 --bridge-hold 0,1 --probe 62,42 --screenshot extracted/screens/bridge-place-ok.png
dotnet run --project src/Netstorm.Game -- --map bridgethegap --bridges 6 --bridge-hold 0,1 --probe 62,47 --screenshot extracted/screens/bridge-place-vegetation.png
dotnet run --project src/Netstorm.Game -- --map bridgethegap --bridges 6 --bridge-hold 0,1 --probe 66,47 --screenshot extracted/screens/bridge-place-red.png
```

- 초목 없는 가장자리 (61,42) 옆 (62,42): "놓을 수 있음(연결 1)"과 원본 프레임.
- 초목(edgeFarm) 칸 (61,47) 옆 (62,47): 빨간 조각과 "불가: 섬 가장자리(초목 없는 곳)나 내 다리 끝에 이어지지 않음". 처음 확인 때는 이 위치가 가능으로 나왔으나 초목 규칙을 반영한 뒤 불가로 바뀌었다.
- 하늘 (66,47): 빨간 조각과 같은 불가 문구.

## 화면 설정 (전체화면·화면비·가장자리 스크롤)

맵 뷰어·스프라이트 뷰어·기본 확인 화면이 같은 화면 계층(`DisplayManager`)을 쓴다.
게임은 **논리 해상도**(원본 픽셀)로 그린 뒤 창에 늘려 표시하며, 16:9·16:10·4:3 을 지원한다.
계산 규칙은 `Netstorm.Core.Display.ScreenLayoutCalculator`(테스트 `tests/Netstorm.Core.Tests`)에 있다.

| 키 | 동작 |
|---|---|
| F11 · Alt+Enter | 전체화면 ↔ 창 모드. 전체화면은 **디스플레이 모드를 바꾸지 않는 테두리 없는 전체 화면 창**이라 원본의 "전체화면 저장 뒤 재실행 오류"가 생기지 않는다 |
| F10 | 와이드 처리: **시야 확장**(게임 동작, 기본) ↔ 4:3 레터박스(개발용) |
| F9 | 원본 해상도 높이 480 → 600 → 768 순환 (원본 640×480·800×600·1024×768 의 높이) |
| F7 | 가장자리 스크롤 켜기/끄기 (원본 `Edge Scroll in Fullscreen`) |

* **와이드 화면은 시야 확장으로 동작한다 (2026-09-29 사용자 결정). 레터박스 화면은 게임에서 쓰지 않는다.**
* **시야 확장**: 같은 배율에서 맵을 옆으로 더 보여 준다. 논리 높이는 고른 원본 해상도의 높이(기본 768)이고 논리 폭이 화면비에 맞춰 늘어난다 (16:9 → 1365×768, 16:10 → 1229×768, 4:3 → 1024×768). 4:3~16:9 밖(21:9·5:4)은 가까운 한계 화면비로 제한하고 남는 곳을 검게 둔다.
* **4:3 레터박스 (개발용, 게임 옵션 아님)**: 원본 해상도 그대로 4:3 영역만 쓴다. 1920×1080 에서 가운데 1440×1080, 좌우 여백 240px, 배율 1.40625 — 원본 로컬 플레이 영상과 같다. 뷰어 캡처를 원본 캡처와 1024×768 로 겹쳐 볼 때만 쓴다.
* 정수 배율이면 점 샘플링, 아니면 선형 보간으로 늘린다 (글자도 함께 늘어나 흐려질 수 있다 — 게임 UI 를 만들 때 원본 해상도 기준 글꼴로 다시 검토).
* 창 제목과 화면 맨 위 넷째 줄에 `화면 1920×1080 (16:9) → 논리 1365×768 ×1.406 · 시야 확장 · 전체화면` 처럼 현재 배치가 표시된다.
* 마우스 좌표는 논리 좌표로 바뀌어 뷰어에 전달되므로 배율·여백과 상관없이 호버·클릭 위치가 맞는다.

설정은 사용자 설정 폴더(Windows `%APPDATA%\NetstormReborn\settings.json`, Linux `$XDG_CONFIG_HOME` 또는 `~/.config` 아래)에 저장된다.
원본 `options.cfg` 는 건드리지 않는다. 폴더는 환경 변수 `NETSTORM_SETTINGS_DIR` 로 바꿀 수 있다.
**시작 안전장치**: 시작할 때 `StartupInProgress` 를 켜 두고 첫 프레임을 그린 뒤 끈다. 켜진 채 전체화면 설정이 읽히면(직전 시작이 화면 초기화에서 끝남) 창 모드로 시작하고 안내를 띄운다.

명령줄 옵션 (아래를 쓰면 그 실행은 설정을 저장하지 않는다): `--fullscreen`, `--windowed`, `--window 1920x1080`,
`--wide extend|letterbox`, `--view-height 480|600|768`, `--no-edge-scroll`.

검증용 PNG를 저장하고 자동 종료:

```powershell
dotnet run --project src/Netstorm.Game -- --map savetheisland --screenshot extracted/screens/fort-map-savetheisland.png
# 화면비·전체화면 확인 (스크린샷 실행은 설정을 저장하지 않음)
dotnet run --project src/Netstorm.Game -- --map savetheisland --window 1920x1200 --screenshot extracted/screens/wide-16x10.png
dotnet run --project src/Netstorm.Game -- --map savetheisland --window 1920x1080 --wide letterbox --screenshot extracted/screens/letterbox.png
# 저장 프레임 수 변경 (기본 30): 가장자리 스크롤처럼 시간이 필요한 동작을 찍을 때
dotnet run --project src/Netstorm.Game -- --map savetheisland --fullscreen --screenshot-frames 15 --screenshot extracted/screens/edge.png
```

원본 팔레트와 본체 레이어를 사용하는 정적 뷰어다. 저장 프레임이 없는 타입은
`default` 클러스터를 선택한다. 단 `randframe` 거주지(Residence)는 원본 캡처처럼 영역 신전 원소의 그림
(해·비·바람·번개별 residence 그림) 중 lit 프레임 하나를 좌표로 고정해 고른다 (원본은 무작위, 2026-09-28).
건물 기준점에 xmin/ymin을 더해 그린다.
게임의 정확한 깊이 정렬은 미분석이므로 표면 우선, y·x 순서로 표시한다.

다리는 저장된 본체 프레임을 그대로 표시한다. 지면은 원본 연결 패턴과 시드 성장 흐름을
바탕으로 생성한 **미리보기**이며 원본과 픽셀 단위 일치를 보장하지 않는다.
`fringe` 플래그가 있는 지면에는 원본 방향 폴백과 y+4칸 기준점을 적용한 별도 절벽을 표시한다.
전투 모드에서 제외되는 `unlit` 변형은 사용하지 않는다. 절벽의 변형 선택과 깊이 정렬,
가장자리 타일은 원본의 원소별 유효 프레임 범위에서 선택한다. 개별 변형의 전역 난수
선택·그림자·받침 섬의 동적 생성은 추가 검증이 필요하다.
본섬 안쪽 `AA` 타일은 원본처럼 원소별 36프레임에서 3×3 그림을 구성한다.
변형 번호는 원본의 99항목 난수표 생성·좌표 선택식을 고정 시드로 재현한다.
실제 게임은 시간 시드와 그 뒤의 전역 난수 소비 순서에 따라 다른 변형을 고를 수 있다.
2026-09-28 후속 수정으로 본섬 밀도 시드를 원본의 청크별 초기화에 맞췄다.
Save the Island!·The War Begins!의 본섬 마스크는 독립 Python 재현 결과와 전체 칸이 일치하며,
두 원본 캡처에 경계를 겹쳐 형태를 대조했다. 픽셀 단위 외관 일치를 보장하는 검증은 아니다.
대조 이미지 생성 방법과 수치는 [본섬 마스크 분석](exe/terrain-and-bridges.md)에 있다.
후속 작업에서 본체 프레임 계산을 셰이프의 레이어별 저장 순서에 맞춰 수정하여
가이저·다중 레이어 건물이 그림자 영역의 프레임을 선택하던 오류를 해결했다.
저장된 `noIsland`가 같은 소유자의 완전한 3×3 묶음이면 `island` 윗면과 `islandStalag` 하단 바위로 표시한다.
오른쪽 아래 칸을 두 스프라이트의 공통 기준점으로 사용하며 일반 지면·절벽과 중복하지 않는다.
Save the Island!의 26개, The War Begins!의 13개 받침을 저장된 논리 칸과 대조했다.
받침 색상은 플레이어 번호와 별개의 색상 번호로 선택한다. 현재 개발용 색상표는 제공된 미션 캡처에
맞춘 플레이어 1 청록(7), 플레이어 2 빨강(2)이며, 중립·미지정 소유자는 P09를 사용한다.
일반 `isle` 지면에도 원본의 소유자별 팔레트 인덱스 변환표를 적용한다. 신전 소유자가 없는 지면은
원본 인덱스를 유지한다. 이 변환으로 본섬 윗면의 갈색 테두리가 청록색으로 바뀐다.
흰 돌출 장식은 원본 `edgeFarm` 스프라이트다. 원본의 `matchframe` 규칙에 따라 선택된
`isle` 칸을 같은 번호의 `edgeFarm` 프레임으로 대체한다. 개발용 뷰어는 영역 청크당 20개를
목표로 가장자리 칸을 좌표 기반 순서로 선택한다. 원본은 전역 난수와 배치 가능 여부를 사용하므로
장식의 개별 위치는 미리보기이며 원본 캡처와 정확히 일치하지 않는다.
일반 맵의 플레이어 설정을 읽어 색상표를 복원하는 기능은 미구현이다.
특수 프레임은 위치 표식으로 표시하며, 불완전한 `noIsland` 묶음은 기존 지면 미리보기에 남긴다.
다른 스프라이트의 플레이어별 색상 변환, 그림자, 애니메이션, 컨테이너 내부 오브젝트,
바리케이드 광선도 아직 그리지 않는다.

현재 원점은 저장 좌표를 사용하는 미션 기준 (1,1)이다. 일반 요새를 여는 경우에도 이 기준으로
미리보기하며, 원본 요새 편집 원점 (6,6)이나 전투 재배치를 재현하지 않는다.
분석 근거는 [영역 배치 분석](exe/territory-layout.md)에 정리했다.
지면과 다리에 대한 검증 범위는 [다리 저장 프레임·지면 생성 분석](exe/terrain-and-bridges.md)을 참고한다.
