# 개발용 맵 뷰어

원본 10.78의 키 배정은 [사용자 녹화에서 읽은 조작키 표](gameplay/input-controls.md)에 별도로 정리했다(2026-10-02). 아래 클론의 F4 사제 이동·F7 화면 끝 스크롤 설정·Space 일시정지 등은 원본 키 배정과 다르며 이번 분석에서 변경하지 않았다.

2026-10-01: 기본 실행은 **메인 메뉴와 캠페인 1-1 플레이**다. 1-1에는 마우스 생산·등록·워크샵 업그레이드·채집·이동·포획 버튼과 미니맵을 연결했고 개발용 규칙 변경은 차단했다. 아래의 P/[ ]/K 등은 `--map` 또는 다른 개발용 미션 조작 설명이다. [1-1 사용법·범위·추정·검증](gameplay/campaign-one.md). 2026-10-01: 1-2 Master of Whirligigs도 공개 — [1-2 문서](gameplay/campaign-two.md).

2026-10-01 **포대 전투** 추가: 미션에서는 브리핑을 닫은 뒤 자동 공격한다. 맵만 열었을 때는 **F3**으로
전투와 세션 시간 진행을 켜고 끈다. 손상 체력 막대·선택(T) 체력 수치·탄·Vander Tower 번개·사제 기절 고리·포획/구속 상태·알타의 룬 진행을 표시한다.
신전 완공·파괴 뒤 섬 원소·소유자색도 갱신한다. `--script "combat 1; wait 1"`로 검증할 수 있다.
전투 공식·탄속 등의 근사와 미구현 범위는 [전투 구현 문서](gameplay/combat.md), 새 영상 근거는
[YouTube 캠페인 3 관찰](videos/youtube-act3-combat.md)을 참고한다. F3은 현재 개발용이며 원본 단축키와 다르다.

원본 `.fort`의 저장된 오브젝트를 16×11px/칸 좌표계로 표시한다.
실행 데이터는 출력의 `game-data/`에 자동 포함된다. `GameDataLocator`는 `game-data/`·저장소의 `assets/game-data/`를 찾으며 별도 데이터는 `NETSTORM_DATA`로 지정할 수 있다. [데이터 구성](../assets/README.md).

저장소 루트에서 실행:

```powershell
dotnet run --project src/Netstorm.Game -- --map savetheisland
dotnet run --project src/Netstorm.Game -- --map thewarbegins
dotnet run --project src/Netstorm.Game -- --map assets/game-data/d/b0.fort
dotnet run --project src/Netstorm.Game -- --map savetheisland --language korean
```

2026-10-03 **커스텀 맵 시험 전투와 화면 요소**([TEST01 화면 요소 대조](videos/test01-visuals-20261003.md)):

```powershell
dotnet run --project src/Netstorm.Game -c Release -- --test-battle TEST01                 # 원본 Edit → Game → Test Battle 처럼 플레이 화면으로 연다
dotnet run --project src/Netstorm.Game -c Release -- --test-battle TEST01 --dump-objects  # 맵 오브젝트·세션 오브젝트·지면 영역(테마·소유자)을 콘솔에 쓴다
```

* `--test-battle <맵>` 은 캠페인에 공개되지 않은 맵(`d/<맵>.fort` + `.english`)도 생산 창·미니맵이 있는 플레이 화면으로 연다. 원본처럼 AI 색 덮어쓰기(`aiNColor`)는 적용하지 않는다.
* 플레이 화면에 반영한 원본 요소: 소유자 색 테두리(받침 포함), 화면을 따라 스크롤하는 미니맵(1px = 2칸, 누르기·끌기), 그림자, 가이저 증기·워크샵 레벨 그림·신전 회오리·풍선 흔들림, `hotFootRatio` 기준점, 원본 방식 배치 미리보기(흰 발자국 사각형·원소 아이콘·노란 비용·안내 글·고정 캐논 사거리 반짝임), 선택 괄호와 체력 막대, 브리핑의 글 사이 그림.
* **커서와 칸**: 커서가 가리키는 칸은 화면에 보이는 그 칸이다(이전에는 한 칸 왼쪽·위였다). 들고 있는 유닛의 발자국은 원본처럼 오른쪽 아래 칸이 (커서 열, 커서 행 + 1 + 타입의 `height`) 에 온다 — 골렘은 커서 두 행 아래가 발밑이다.

후속으로 [버튼 효과음·선택 사거리·Crossbow 방향](videos/record-play-details-20261003.md)을 연결했다. 공격 건물 선택과 배치에 같은 사거리 반짝임을 쓰고, Crossbow는 배치 전 우클릭으로 그림·60도 V 범위를 회전한다. 메인 메뉴·브리핑·도움말의 활성 일반 버튼은 원본 `button.wav`를 재생한다.

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
* Home: 시작 카메라로 복귀 — 원본 미션 시작 화면처럼 플레이어 1 사제 칸 기준점을 창 중심에서 (+16, +12) 떨어진 곳에 둔다.
  논리 해상도가 1024×768 이면(4:3 창이거나 `--wide letterbox` 일 때) 사제 칸 기준점이 원본과 같은 (528, 396) 에 놓여, 뷰어 캡처를 원본 캡처와 바로 겹쳐 볼 수 있다. 사제 그림은 `hotFootRatio` 때문에 (8, 2) 왼쪽·위인 (520, 394) 에 그려진다 (2026-10-03 TEST01 녹화의 건물 위치로 다시 맞춤 — [근거](videos/test01-visuals-20261003.md), 이전 값 (525, 393) 은 [캡처 측정](screens/dissolved-alliance-start.md) 2절).
  안내 영역이 4줄로 늘어(높이 128) 화면 맨 위 128 논리 픽셀은 안내가 덮는다 (사제 위치 계산은 안내 높이를 보정하므로 그대로).
  와이드 시야 확장에서는 화면 중심이 넓어진 만큼 사제가 가운데 쪽으로 온다
* F4: 현재 사제 위치로 화면을 옮긴다. 튜토리얼 1의 첫 단계 신호로도 처리한다.
* G: 진단용 청크 윤곽 표시 전환
* 오브젝트 기준점 근처에 마우스: 타입, 좌표, 영역, 소유자, 다리 값 표시
* Esc: 맵 시험 화면에서는 종료, 미션에서는 상단 Game 메뉴 표시·숨김

## 게임 세션과 미션 모드

뷰어는 규칙 상태를 직접 갖지 않는다. 입력은 **게임 세션**(`BattleSession`, [core-rules.md](core-rules.md) "게임 세션")에 명령으로 넣고,
세션이 고정 틱(24Hz)으로 진행한 결과(오브젝트·다리·Storm Power·알림)를 그린다.

| 실행 방법 | 세션 시간 | 생산 규칙 (기술 허용 표·덱 등록·재충전·회수 금지) | 시작 Storm Power·지식 |
|---|---|---|---|
| `--mission 이름` (예: `tutorial1`, `tutorial2`, `thewarbegins`) | 계속 흐름. 단 **안내·브리핑 창이 열려 있는 동안은 멈춘다**(미션을 열면 A. 브리핑이 먼저 뜨고 닫은 뒤부터 흐름) | 켜짐 | 미션 머리의 `myStartMoney`·`myTech`, 전투 옵션 덮어쓰기(튜토리얼 2) |
| `--map 이름` | P·B 또는 F3 전투가 켜진 동안 흐름 (처음에는 정지) | 꺼짐 = **시험 모드** (규칙 조건만 맞으면 어떤 유닛이든 놓는다) | 맵의 `Money` 섹션 |

* `--mission`은 미션 스크립트에서 맵(`loadFort`)과 시작 조건을 읽어 연다. `--map`과 함께 쓸 수 없다.
* 미션 중 Esc → `Game`을 클릭하면 목표 다시 보기(F8), Restart Mission, Leave Mission, Quit Game을 고를 수 있다. Leave Mission 확인 창의 Main Menu는 메인 메뉴로 돌아가고, Replay Mission은 같은 미션을 처음부터 다시 연다. Continue Mission은 확인 창을 닫는다. Restart Mission도 같은 미션을 다시 로드하며 첫 안내와 시작 Storm Power를 복원한다. 1-1에는 화면의 메뉴 버튼도 있다. [원본 실행 관찰](screens/README.md#19-replay-missionrestart-mission-전환-2026-09-30-windows-hjow-athlon).
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

**캠페인 초기 브리핑(2026-09-30):** `--mission thewarbegins` 같은 캠페인 미션도 미션 스크립트의 `[A.]` 섹션을 브리핑 창으로 먼저 연다(제목·인용·본문, 버튼 **Review Knowledge / Play Mission**). **브리핑을 닫기 전에는 세션 시간이 0에서 흐르지 않는다**(사용자 규칙 2, [안내·브리핑 창과 게임 시간](gameplay/dialog-pause.md)). Play Mission(또는 Enter·Space)으로 닫으면 시간이 시작되고, F8·Game 메뉴의 목표 다시 보기로 다시 열 수 있다(다시 열어도 그동안 시간은 멈춘다). Restart/Replay Mission은 미션을 다시 열어 브리핑부터 시작한다. 오른쪽 위 상자에는 창이 열려 있는 동안 `· 안내 창(시간 정지)`이 표시된다.

* `Review Knowledge`(`ShowTechnology`)는 브리핑 위에 **지식 창**을 겹쳐 연다. 미션 화면에서는 **F6**(View Netstorm Knowledge)도 같은 창을 연다(2026-10-01 원본 화면대로 재구현). SUN·WIND·RAIN·THUN. 네 행에 행 머리 칸(원소 이름·원소 기호)과 유닛 카드(유닛 모습·이름)가 놓이고, 마우스가 올라간 카드는 어두워진다. 카드는 맵 `.fort` Technology 의 지식 + 배운 지식이며 행 안 순서는 `.type` group 순서다. 카드를 누르면 **상세창**(도움말 삽화, Alignment·Class·Hits·Range·Damage·Cost in Storm Power·Energy to Build, `help.english` 본문 스크롤, Back·OK)이 열린다. 격자는 F6·Esc·Enter·Space·격자 바깥 클릭으로 닫고, 상세창은 Back·Esc·Backspace로 격자, OK·Enter·Space·F6으로 모두 닫는다. 원본처럼 **시계를 멈추지 않는다**(녹화에서 창이 열린 채 SP 가 늘었다 — [ShowTechnology 분석](exe/show-technology.md)). 원본 돌 질감·금색 모서리와 작은 버튼을 공유 UI 스킨으로 표시한다([외관 적용](screens/clone-ui.md)). Damage 는 원본 계산을 찾지 못해 Shooter 는 `?`로 둔다.
* 미션 스크립트 버튼 `MissionAbort,1`(성공 창 Leave Missions)은 곧바로 미션을 떠나고, `MissionAbort,0`은 Leave Mission 확인 창(Main Menu·Replay Mission·Continue Mission, 원본 `tell.english` [ABORT]와 같은 구성)을 열며, `MissionRestart`는 미션을 다시 시작한다(exe `FUN_00463e40`). Core의 포획·희생 이벤트가 해당 미션 스크립트 섹션을 알리면 안내 창을 열고, 창이 있는 동안 시간을 멈춘다. 이벤트 조건과 근사는 [희생 의식 구현](gameplay/sacrifice.md)에 기록했다.
* 보이는 본문이 없고 `<$Config,…>` 설정 명령만 있는 `[A.]`(대회용 `tnronguide` 스크립트)는 빈 창이 미션을 멈춰 세우지 않도록 열지 않는다.
* 원본은 미션 시작 뒤 다이얼로그 없이 10프레임이 지나야 브리핑이 뜨고 닫은 뒤 7프레임 뒤에 시간이 재개되지만, 클론은 0초 지점에서 즉시 열고 닫는 즉시 재개한다(차이는 0.2초 안팎으로 추정).

* F8: 현재 단계의 시작 안내를 다시 연다. MORE/BACK으로 이동하거나 보정 안내(`NotVortex` 등)를 본 뒤에도 단계 시작으로 돌아간다.
* Enter·Space 또는 좌클릭: 선택한 버튼 실행. Tab·좌우 방향키: 버튼 선택.
* 마우스 휠·상하 방향키·PageUp·PageDown: 긴 본문 스크롤. 안내 창에서 Esc는 원본처럼 반응하지 않는다. 창을 닫은 뒤 Esc는 미션 메뉴를 연다.
* `<h1>`~`<h4>`, `<p>`, `<br>`, `<i>`, `<c>` 등의 제목·간격·강조를 표시한다. 원본 그림 명령(`<!...>`)은 현재 `[그림: 이름]` 자리표시자로 보인다. 창 바탕·모서리와 작은 글씨·버튼은 [원본 UI 외관 적용](screens/clone-ui.md)에 따라 표시하며, 인라인 그림·정밀 배치는 후속 작업이다.

튜토리얼 2의 A~I 단계 안내와 버튼은 원본 스크립트로 정적 검사했다. 그래픽 창의 실제 배치·마우스 입력은 이번 작업에서 실행 검증하지 않았다.

## 수송 사제 포획과 알타 의식

선택한 수송 유닛은 기절한 적 사제를 클릭하면 그쪽으로 이동해 싣는다. 사제를 실은 채 내 완성 알타를 클릭하면 운반하고, 선택한 내 사제로 알타를 클릭하면 의식에 필요한 자리로 이동한다. 운반 중 `D`를 누른 뒤 빈 칸을 클릭하면 사제를 내려놓는다. 지도에는 `운반 중`·`제단에 묶임` 표식과 알타 위 다섯 룬의 진행 상태가 나온다. 의식 중 내 사제를 다른 곳으로 보내거나 내 사제가 기절·포획되면 의식은 깨지지 않고 멈추며(포로는 알타가 파괴·판매될 때만 풀린다), 알타 글자가 주황색 `멈춤(사제 복귀 필요)`로 바뀐다. 사제를 다시 알타로 보내면 이어진다. 의식을 마치면 5,000 SP를 받는다. 상세한 측정·추정 구분은 [sacrifice.md](gameplay/sacrifice.md)를 참고한다.

**이동·낙하 상태(2026-10-01):** 이동 중 길이 끊겨 우회할 수 없으면 `길 막힘·대기`를 표시하고 같은 명령을 유지한다. 다리를 복구하면 자동으로 재개한다. 발밑 다리·받침 자체가 사라지면 지상 수송은 제거되고 사제는 `허공에서 기절` 상태가 된다. 이 사제는 비행 수송으로 포획하거나 그 칸에 다리를 놓아 회복시킬 수 있다(HP가 절반 이상). 최대 HP로 허공 기절한 사제도 보호막 고리와 체력 막대를 그린다. 저장된 수송 유닛 본체도 현재 이동 좌표로 표시한다. 건물 회수·파괴 시 받침·개발용 지면·절벽을 숨기며 새로 배치한 건물형 유닛 받침은 원본 그림으로 표시한다. [동작과 추정값](gameplay/movement-pathing.md).

조작 순서는 다음과 같다.

1. 수송 유닛 위에 커서를 두고 `T`를 눌러 선택한다.
2. 기절한 사제를 클릭해 포획한 뒤, 목적지 알타를 클릭한다.
3. 내 사제도 `T`로 선택해 알타를 클릭한다. 실제 이동 경로가 이어지면 둘 다 알타 주변으로 이동하고 의식이 시작된다.
4. 내려놓으려면 수송 유닛이 선택된 상태에서 `D`를 누르고 비어 있는 칸을 클릭한다.

검증용 `--script`에는 `capture tx,ty px,py [ax,ay]`, `deliver tx,ty ax,ay`, `altar ax,ay`, `drop tx,ty x,y`를 지원한다. 좌표는 맵 칸이며 `capture`는 선택한 수송 유닛 좌표, 사제 좌표, 선택적 알타 좌표 순서다. `--script`의 좌표는 각 명령이 실행되는 시점에 엔티티를 다시 조회한다.

검증용 명령 스크립트 `--script "명령; 명령"`: 시작할 때 세션 명령을 차례로 실행하고, 결과를 콘솔에 출력한다.

| 명령 | 동작 |
|---|---|
| `construct 타입 x,y` | 사제가 건물(템플·워크샵·알타·아웃포스트)을 짓기 시작 |
| `harvest x,y` | 그 칸의 가이저에 사제를 보내 반복 수확 |
| `home` | F4 화면 복귀와 같은 튜토리얼 1 신호 |
| `combat 0 또는 1` | 자동 포대 전투 끄기/켜기. 시간 정지는 Space |
| `place 타입 x,y` | 생산 창의 유닛을 배치 |
| `register 타입` | 그 유닛을 받을 수 있는 첫 워크샵에 지식으로 등록 |
| `salvage x,y` | 그 칸의 내 오브젝트를 회수 |
| `wait 초` | 게임 시간을 진행 |
| `select x,y` / `select none` | 그 칸의 오브젝트를 선택 / 선택 해제 (튜토리얼 2 단계 C·F가 선택한 템플을 본다) |
| `allow 타입` / `deny 타입` | 기술 허용 표를 바꾼다 (단계 처리가 없는 미션에서 손으로 재현) |
| `denysalvage 0\|1` | 회수 금지를 바꾼다 (같은 용도) |
| `rules 0\|1` | 생산 규칙 끄기/켜기 |
| `capture tx,ty px,py [ax,ay]` | 수송 유닛을 사제에게 보내고, 선택적으로 포획 뒤 알타까지 운반 |
| `deliver tx,ty ax,ay` | 운반 중인 사제를 알타로 옮겨 묶음 |
| `altar ax,ay` | 내 사제를 알타 옆으로 이동 |
| `drop tx,ty x,y` | 수송 유닛이 지정한 빈 칸에 운반 사제를 내려놓음 |

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

2026-10-03: 아이스·썬더 캐논을 들고 있을 때 **우클릭은 북→동→남→서 회전**이다(역회전 설정이면 반대 순서). 방향별 원본 그림을 미리 보여 주며 놓은 방위는 고정된다. 개발용 `--script`는 `place rainCannon 100,120 1`처럼 마지막 방위 값(0=북, 1=동, 2=남, 3=서)을 받을 수 있다. 세 캐논의 충전·발사 그림, 아이스 타워 재성장, 썬 바리케이트 방어선의 근거·한계는 [TEST01 추가 판독](videos/test01-combat-20261003.md)에 있다.

## 다리 조각 시험 모드

**B** 로 켜고 끈다(배치 시험 P 와 동시에 켜지지 않는다). 게임 세션이 원본 규칙대로 플레이어 1 의 다리 칸(`BridgeTray`)을 채운다.
집기·되돌리기·놓기는 모두 세션 명령이고(`PickBridgePieceCommand` 등), 조각의 회전은 화면이 관리해 놓을 때 값으로 보낸다.
세션은 모든 플레이어의 다리 칸을 같은 전역 난수로 채우므로(원본도 난수 하나를 공유) 다른 플레이어가 있는 맵에서는 플레이어 1 의 조각 순서가 이전 뷰어와 다르다.
규칙은 템플이 있을 때만 1초마다, 칸 수는 전투 옵션 Bridge Slots, 5번째 추첨마다 한 칸 조각이다([bridge-pieces.md](exe/bridge-pieces.md)).
왼쪽 패널에 칸이 2열로 표시되고, 조각은 원본 `bridge.type` 프레임으로 그린다.

| 입력 | 동작 |
|---|---|
| `1`~`6` 또는 `Q` `W` `A` `S` `Z` `X` | 그 칸의 조각을 집는다 (들고 있던 조각은 칸으로 되돌린다). 글자 키는 원본 매뉴얼의 단축키로, 2열 칸의 행 순서(Q W / A S / Z X)다 |
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

맵 뷰어·스프라이트 뷰어·메인 메뉴가 같은 화면 계층(`DisplayManager`)을 쓴다. 메인 메뉴 옵션에서 해상도·창/전체화면·음량을 조절하며 [현재 공개 범위와 임시 정책](gameplay/campaign-one.md)을 따른다.
게임은 **논리 해상도**(원본 픽셀)로 그린 뒤 창에 늘려 표시하며, 16:9·16:10·4:3 을 지원한다.
계산 규칙은 `Netstorm.Core.Display.ScreenLayoutCalculator`(테스트 `tests/Netstorm.Core.Tests`)에 있다.

| 키 | 동작 |
|---|---|
| F11 · Alt+Enter | 전체화면 ↔ 창 모드. 전체화면은 **디스플레이 모드를 바꾸지 않는 테두리 없는 전체 화면 창**이라 원본의 "전체화면 저장 뒤 재실행 오류"가 생기지 않는다(AGENTS.md 2026-10-03: 이 문제는 클론에서 발생하지 않아야 한다) |
| F10 | 와이드 처리: **시야 확장**(게임 동작, 기본) ↔ 4:3 레터박스(개발용) |
| F9 | 원본 해상도 높이 480 → 600 → 768 순환 (원본 640×480·800×600·1024×768 의 높이) |
| F7 | 가장자리 스크롤 켜기/끄기 (원본 `Edge Scroll in Fullscreen`) |

* **와이드 화면은 시야 확장으로 동작한다 (2026-09-29 사용자 결정). 레터박스 화면은 게임에서 쓰지 않는다.**
* **시야 확장**: 같은 배율에서 맵을 옆으로 더 보여 준다. 논리 높이는 고른 원본 해상도의 높이(기본 768)이고 논리 폭이 화면비에 맞춰 늘어난다 (16:9 → 1365×768, 16:10 → 1229×768, 4:3 → 1024×768). 4:3~16:9 밖(21:9·5:4)은 가까운 한계 화면비로 제한하고 남는 곳을 검게 둔다.
* **4:3 레터박스 (개발용, 게임 옵션 아님)**: 원본 해상도 그대로 4:3 영역만 쓴다. 1920×1080 에서 가운데 1440×1080, 좌우 여백 240px, 배율 1.40625 — 원본 로컬 플레이 영상과 같다. 뷰어 캡처를 원본 캡처와 1024×768 로 겹쳐 볼 때만 쓴다.
* 정수 배율이면 점 샘플링, 아니면 선형 보간으로 늘린다 (글자도 함께 늘어나 흐려질 수 있다 — 게임 UI 를 만들 때 원본 해상도 기준 글꼴로 다시 검토).
* 창 제목과 화면 맨 위 넷째 줄에 `화면 1920×1080 (16:9) → 논리 1365×768 ×1.406 · 시야 확장 · 전체화면` 처럼 현재 배치가 표시된다.
* 마우스 좌표는 논리 좌표로 바뀌어 뷰어에 전달되므로 배율·여백과 상관없이 호버·클릭 위치가 맞는다.

### 프레임 속도 요구사항 (AGENTS.md 2026-10-03)

AGENTS.md는 **"새 클론 게임에서는 30, 60, 120프레임을 지원해야 한다. 30프레임 지원을 먼저 구현하고, 60 및 120프레임 지원은 후순위로 둔다"**고 정했다. 기존 게임은 요소마다 애니메이션 프레임이 달랐던 것으로 추정된다고 함께 적혀 있다.

* **현재 상태(미구현):** 화면은 수직 동기(모니터 주사율)에 맞춰 그리고, 프레임 속도를 고르는 옵션·설정·명령줄 인자는 없다. 진단용 `NETSTORM_NOVSYNC=1`(수직 동기 끄기)만 있다.
* **구현 기준(제안):** 여기서 "프레임"은 화면 갱신 속도로 해석한다. 규칙은 24Hz 고정 틱(`FixedTimestep`)으로 돌고 화면은 `VisualCell` 보간으로 그리므로, 그리기 속도를 30·60·120으로 바꿔도 게임 속도·검사합은 달라지지 않아야 한다. 30프레임 상한을 먼저 넣고 설정 파일에 저장한다.
* **애니메이션:** 원본은 요소마다 간격이 다르다(가이저 증기 약 24Hz, 신전 회오리·피해 연기 12Hz 등 — [animation-timing.md](videos/animation-timing.md)). 30프레임에서는 24Hz 애니메이션이 화면 프레임과 맞지 않아 간격이 고르지 않게 보일 수 있으므로, 구현 때 원본 영상과 나란히 비교한다.
* 이 절은 요구사항 기록이며 코드는 아직 고치지 않았다.

설정은 사용자 설정 폴더(Windows `%APPDATA%\NetstormReborn\settings.json`, Linux `$XDG_CONFIG_HOME` 또는 `~/.config` 아래)에 저장된다.
원본 `options.cfg` 는 건드리지 않는다. 폴더는 환경 변수 `NETSTORM_SETTINGS_DIR` 로 바꿀 수 있다.
**시작 안전장치**: 시작할 때 `StartupInProgress` 를 켜 두고 첫 프레임을 그린 뒤 끈다. 켜진 채 전체화면 설정이 읽히면(직전 시작이 화면 초기화에서 끝남) 창 모드로 시작하고 안내를 띄운다.

명령줄 옵션 (아래를 쓰면 그 실행은 설정을 저장하지 않는다): `--fullscreen`, `--windowed`, `--window 1920x1080`,
`--wide extend|letterbox`, `--view-height 480|600|768`, `--no-edge-scroll`, `--no-sound`, `--no-music`.
성능 진단(2026-10-03): `--perf`는 1초마다 프레임 수·갱신/그리기 평균 시간과 구간별 시간(지면 `terrain`·오브젝트 `objects`·세계 `world`·UI `ui`·그리기 호출 제출 `batchEnd`)을 콘솔에 쓰고 화면 오른쪽 아래에 FPS를 띄운다. 환경 변수 `NETSTORM_NOVSYNC=1`은 수직 동기를 끈다 — 원격 데스크톱처럼 표시 주사율이 낮은 환경에서 프레임이 그 주사율(예: 32Hz)에 묶이는지 확인하는 진단용이다. UI 자동 검사(`--ui-script-file`)에는 `assert-detail 값`(선택 유닛·내 이동형 유닛의 방향/이동 상태, 예: `assert-detail selected=priest`, `assert-detail priest:C:moving`)이 있다.

TEST01 후속 검증: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/clone_test01_smoke.ps1`은 4:3 두 크기·16:9·16:10에서 영어/한국어를 번갈아 검사한다. 선택·워크샵 레벨·미니맵 클릭/끌기·Shift+숫자 저장/숫자 복귀·배치·아이스/썬더 캐논의 배치 전 우클릭 회전을 확인하며 설정은 별도 폴더에 둔다. UI 명령 `move-center dx,dy`는 논리 화면 중심 기준으로 커서를 옮기고 `assert-detail-not 값`은 상태에 해당 문자열이 없음을 검사한다. 추가 상태는 `camera=x,y`, `workshops=타입:레벨,...`, `placement=allowed|실패명|none`, `cursor=타입|none`이다. [결과와 시각 차이](videos/test01-verification-20261003.md).
시작 팁(2026-10-02): 메인 메뉴로 시작하면 Options "Tell Tips at Startup"이 켜져 있을 때 원본 "Did You Know?" 창을 연다. `--no-tips`는 이번 실행에서만 끄고, `--ui-script-file` 자동 검사는 `--tips`를 줄 때만 연다.

**소리(2026-10-01)**: 원본 `sound/*.wav`·`music/*.mus`를 그대로 재생한다. 미션에서는 원소 곡 4개를 원본 순서(wind → rain → thunder → sun, 첫 곡 난수)로 돌리고 내 희생 의식 동안 `sacrifice.mus`를 요청한다. 메뉴는 `ser22.mus`다. 효과음은 다리 금·붕괴·놓기·회전, 건설 완료, 지식 창, 포획과 의식에 연결했다. 메인 메뉴 옵션의 효과음·음악 켜기/끄기·볼륨(1~5, 기본 3·2)을 현재 재생에도 즉시 적용한다. 소리 장치가 없으면 무음으로 계속 실행한다. 규칙: [music.md](exe/music.md).

검증용 PNG를 저장하고 자동 종료 (`--knowledge [타입]`을 붙이면 지식 창 — 타입을 주면 그 상세창 — 을 연 채 시작한다):

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
Whirlibase에서 생성한 Whirligig는 원본 회전 프레임으로 이동·표시한다([사용 범위·추정](gameplay/flyers.md)).
다른 스프라이트의 플레이어별 색상 변환, 그림자, 기타 애니메이션, 컨테이너 내부 오브젝트,
바리케이드 광선도 아직 그리지 않는다.

현재 원점은 저장 좌표를 사용하는 미션 기준 (1,1)이다. 일반 요새를 여는 경우에도 이 기준으로
미리보기하며, 원본 요새 편집 원점 (6,6)이나 전투 재배치를 재현하지 않는다.
분석 근거는 [영역 배치 분석](exe/territory-layout.md)에 정리했다.
지면과 다리에 대한 검증 범위는 [다리 저장 프레임·지면 생성 분석](exe/terrain-and-bridges.md)을 참고한다.
