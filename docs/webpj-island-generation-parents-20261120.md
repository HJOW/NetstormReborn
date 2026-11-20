# 섬 생성 부모 셋: 군도 분리·중앙섬·전투섬 (WB04-E1104)

12차 스커미시가 쓰는 MP식 월드 생성(무작위 중앙섬 → 참가자별 시작 섬)의 `o/` 부모 세 개를 이식했다. 앞서 있던 `RawIslandBuilder`(영역 패턴 배치)·`RawIslandRuntime`(섬 목록)·정상 지형 생성기·`RawFortWorldLoad` 위에서 호출 순서와 선택 규칙만 묶는 얇은 모듈들이며, 장치(지형 생성·Graph·객체 적재 등)는 훅으로 받는다.

## 구현

- [raw-island-partition.js](../webpj/www/core/raw-island-partition.js) `RawIslandPartition.split()` (원본 0046f7d0·0046ed50·0046ec00): 해시 0의 256×256 표면 표를 y 행 → x 열 순서로 훑어, 현재 지면 TYPE이고 지역이 0인 첫 칸마다 `add(0, 0xff)`로 새 섬을 만들고 그 지면의 raw X/Y(float32 오프셋 14/18)를 x86 `_ftol`처럼 절삭(64비트 범위 밖·비유한은 0, 안은 하위 32비트)한 좌표에서 flood 한다. 연결 마스크는 프레임 코드의 방향 글자(`connectionMask`)이며 북→동→남→서 순서를 호스트 작업 스택의 역순 push로 보존하고, 방문 기록은 실제 raw 지역 WORD다. 경계를 누적해 `setBounds`로 쓰고 flood가 끝날 때마다 월드 다시 그리기 통지를 부르며, 프레임 번호나 현재 TYPE이 표 밖이면 거부한다. 만든 섬 수를 돌려준다.
- [raw-central-island.js](../webpj/www/core/raw-central-island.js) `RawCentralIsland.build(record)` (원본 0046e140): 진단 4개(전투 모드 아님 0xea·위치 0 아님 0xeb·섬 목록 비어 있지 않음 0xec·owner 0 아님 0xed; 진단 뒤 복귀하면 계속) → 원점 배치 범위 → 장기 중심 기록(청크 경계 ×16의 float32 평균) → `RawIslandBuilder.place(record, 현재 invalidRegion, 위치 무시)` → 지형 생성 → 권한이고 terrainEnabled가 0일 때만 거주 구역 → archipelago가 0이 아닐 때만 군도 분리 → Graph 재구성(false).
- [raw-battle-island.js](../webpj/www/core/raw-battle-island.js) `RawBattleIsland.build(owner)` (원본 0046d9b0→0046da60→0046e2c0): owner 0이면 진단(0x10f) 뒤 계속 → Template의 Territory 20×6바이트 → 플래그 바이트(+1)의 0x04 비트가 켜진 첫 영역(없으면 0번) → Player별 배치 범위 → `place(record, 선택 번호, 위치 무시)` → 섬 번호 기록 → 지형 생성 → 권한이면 그 영역 한 개의 객체 적재 → Graph 재구성(false).
- 셋 모두 10.78 판본·섬 목록과 같은 풀을 생성자에서 확인하고, 필요한 훅이 없으면 효과 전에 거부한다(CD 10.72 분기는 웹 범위 밖).

## 검증

- cpppj에는 이 셋을 직접 검사하는 시험이 없어(Session 통합뿐) 새 `tests/island-generation-parents.test.mjs` 8건이 원문 소스의 순서·계산을 독립 기대값으로 대조한다. 실제 `SidPool`·`ChunkMap`·`RawIslandRuntime`·`RawIslandBuilder`와 합성 프레임으로: 전투섬은 플래그 영역 선택(2·5 중 2, 없음 0, 7, 19)·호출 순서(boss 여부)·월드 지도에 놓인 칸 수와 범위·위치 바이트 무시·owner 0 진단·필수 훅 거부; 중앙섬은 네 조건의 호출 순서(거주 구역·군도 분리 조건)·중심 기록 값(104, 136)·섬 목록의 owner/영역 번호(127)·진단 순서(0xeb→0xec)·필수 훅 거부; 군도 분리는 y→x 스캔 순서로 만든 지역 번호·경계·add 인자·다시 그리기 횟수·다른 TYPE 칸 비방문, 십자 섬의 방문 순서(중심→북→동→남→서), 아래쪽 칸에서 시작해 북쪽으로 번지는 세로 섬, NaN·2³²·1e30·음수 좌표의 `_ftol` 절삭, 범위 밖 프레임(음수 포함)·TYPE 거부, 생성자 검사.
- 변형 확인 약 35종(연결 방향 네 개·방문 TYPE 검사·시작 행·프레임 하한·다시 그리기 통지·절삭 방식·좌표 필드·경계 누적·add 인자·중심 계산·거주 구역/군도 분리 조건·owner 0 진단·플래그 바이트 위치·첫 선택 중단·위치 무시 인자·객체 범위·섬 번호 기록)이 모두 시험 실패로 잡힌다. 동등 변형은 `let raw = …; raw = raw` 한 가지다.

## 남은 일 (12차 나머지)

- E1105~: 세션 연결 — `BuildSkirmishWorld`(Template 지역 슬롯에 시작 섬 fort 읽기·`ReadSkirmishTerritory`의 영역 선택·`PlayerBounds.placement/originPlacement`·군도 밀도 규칙·간헐천 예산 스커미시 참가자 수)와 빈 월드 fort로 하는 미션 시작(`BeginMission("Skirmish")`)·AI 난이도/동맹 적용·항목 기본값 켜기.
