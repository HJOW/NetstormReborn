# 개발용 스프라이트 탐색 뷰어

`_shapes.shp`의 프레임을 원본 팔레트로 표시하고 대응하는 `.type` 클러스터 정보를 확인한다.
지면의 `rim`·`fringe` 변형과 건물의 본체·추가 레이어를 비교할 때 사용한다.

저장소 루트에서 실행한다.

```powershell
dotnet run --project src/Netstorm.Game -- --sprites isle
dotnet run --project src/Netstorm.Game -- --sprites fringe
dotnet run --project src/Netstorm.Game -- --sprites island
```

`--sprites` 뒤에는 `TypeLoadOrder`에 있는 타입 이름을 넣는다. 이름의 대소문자는 구분하지 않는다.
자산은 출력의 `game-data/`에 포함되며, 별도 데이터는 `NETSTORM_DATA`로 지정한다. 소스는 `assets/game-data/`에서 관리한다.
처음에는 실행 설정의 `battlePal` 팔레트를 적용한다. `--map`과 `--sprites`는 함께 사용할 수 없다.

| 추가 옵션 | 동작 |
|---|---|
| `--frame N` | 처음 선택할 프레임 번호 (격자의 `#N`) |
| `--palette 이름` | 처음 적용할 팔레트 (`GamePalSpec` 위치의 `.COL` 이름, 예: `suncannon`) |
| `--play` | 선택 프레임의 동작을 재생한 상태로 시작 |
| `--props` | 격자 대신 `.type` 속성 목록을 보여 주는 상태로 시작 |

| 조작 | 동작 |
|---|---|
| 위/아래 방향키 | 앞/뒤 타입 |
| 왼쪽/오른쪽 방향키 | 앞/뒤 프레임 |
| Page Up/Page Down 또는 마우스 휠 | 앞/뒤 20프레임 페이지 |
| Home/End | 타입의 처음/마지막 프레임 |
| 격자 클릭 | 해당 프레임 선택 |
| Space | 선택 프레임이 속한 동작 재생/정지 |
| `+` / `-` | 재생 속도 1~30fps (기본 10fps) |
| P / Shift+P | 다음/이전 팔레트 (디스크·아카이브의 모든 `.COL`) |
| Tab | 프레임 격자 ↔ `.type` 속성 목록 |
| Esc | 종료 |

오른쪽 패널에는 선택 프레임의 번호, 대응 클러스터 이름과 레이어, 원본 프레임 코드(측면·변형·번호),
클러스터 플래그, 동작 안 위치, 특수 프레임(기본·도움말·gump·base), 크기·기준점, 그림 참조를 표시한다.
셰이프 블록은 레이어별로 클러스터 전체를 담으므로 클러스터 수를 넘는 프레임은 `레이어 1`(그림자)로 표시된다.

**동작 재생 규칙**: 원본 로더가 만드는 프레임 코드의 측면·변형이 같은 클러스터를 파일 순서로 돌려 본다
([type.md](formats/type.md) "원본 프레임 코드 표"). 그림자 레이어 프레임을 고르면 같은 레이어 안에서 돈다.
재생 속도는 원본 틱이 확인되지 않은 개발용 값이며, 원본 동작별 속도·순서 전환(예: 공격 → 대기)은 재현하지 않는다.

**팔레트**: 게임 팔레트는 `GIFCLOUD.COL`이다. 유닛 이름의 `.COL`은 그래픽 제작용으로 추정되며 적용하면 색이 틀어진다.
읽지 못한 팔레트는 안내 줄에 빨간 글자로 표시하고 이전 팔레트를 유지한다.

속성 목록은 `typename`·`typeflags`·클러스터/레이어 수·특수 프레임 번호와 모든 속성을 키 이름순으로 두 열에 표시한다.
칸을 넘는 값은 `…`로 줄이고, 두 열을 넘으면 생략한 줄 수를 표시한다.
이미지가 아닌 특수 레코드는 디코딩하지 않는다. 페이지·타입·팔레트가 바뀔 때 이전 텍스처를 해제한다.

자동 캡처 예시:

```powershell
dotnet run --project src/Netstorm.Game -- --sprites isle --screenshot extracted/terrain/sprite-browser-isle.png
dotnet run --project src/Netstorm.Game -- --sprites dude --frame 8 --play --screenshot extracted/screens/sprite-play-dude.png
dotnet run --project src/Netstorm.Game -- --sprites priest --props --screenshot extracted/screens/sprite-props-priest.png
dotnet run --project src/Netstorm.Game -- --sprites sunCannon --frame 16 --palette suncannon --screenshot extracted/screens/sprite-palette-suncannon.png
```

`--screenshot`의 상위 폴더는 미리 있어야 한다. 캡처는 0.5초(30 × 1/60초) 뒤에 찍으므로 `--play` 캡처는 동작이 몇 프레임 진행된 화면이다.
