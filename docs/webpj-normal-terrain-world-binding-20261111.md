# webpj 일반 지형 월드 연결(RawNormalTerrainWorldBinding) — WB04-E302 (2026-11-11, 4-B)

3.1절 4-B 의 **RawNormalTerrainWorldBinding**(client, 기준선 318줄 = `.cpp` 195 + `.h` 123; RawFortWorldLoadBinding 의 선행으로 3.1절에 추가)을 `core/raw-normal-terrain-world-binding.js` 로 옮겼다. 앞 단위의 정상 지형 부품(Context·Growth·Stamp·FarmCluster·단일 배치, E285~E296)과 `RawSurfaceWorld`(E299)·`RawTerrainWorldBinding`(E301)을 한 월드의 소비자로 조립한다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- **구성** `new RawNormalTerrainWorldBinding(world, chunks, islands, random, crt, contextGlobals, stampState, placementState, farmState, puzzlePiece, placement, reward, carrier, diagnostics)`: 다른 월드의 Carrier·누락된 필수 진단 다섯(context/stamp/region/farm/postpass)·CRT/puzzlePiece 상자 누락은 월드를 바꾸기 전에 거부한다. 같은 타입/지역 전역(지면 TYPE·edge TYPE·noRegion)이 어긋나거나 지면/edgeFarm TYPE 의 실제 생성자 주소가 없으면 거부한다. 단일 배치(`RawFortRandomPlacement`)와 군집(`RawFarmCluster`)을 만들고 월드 후처리에 Carrier 몸체를 등록한다.
- `generate(request)`: **일반** 분기 전용(군도·미복원 부모는 거부). 청크 범위가 내부 1..15 이고 right/bottom 이 left/top 보다 클 때만, Context → 성장(runNormal) → MarkEdges → Stamp → Normalize → context 해제 → 섬별 군집 순서이며 마지막 초기 지면 SID 를 돌려준다.
- `bindGenerationInputs(inputs)` + `generateOriginal(owner, bounds, region)`: 실제 상위(004c1510). 후보 캐시(`enabled && battle && !mission && 섬 수 == 1`) → context/성장 → PATCH layout 이 0 이고 후보이면 군도 분기(두 spacing 을 따로 조회해 확장폭·span, 확장 bounds, 흩뿌리기, 목표 넓이(백분율 0 이면 두 난수 추출), 로그, 사각 성장, 플레이어 1..8 배치 정리, gap/희소 정리), 아니면 일반 성장 → 도장/정규화 → 군도이면 DWORD 켜기 → 섬별 군집. 군도/확장 상자는 군집 상태와 **같은 상자**여야 하며 다른 상자로 다시 연결하면 거부한다.
- `postprocessFarms(region)`(일반 전용), `patternCount(record)`(현재 puzzlePiece 프레임과 `CanonDecoder` 가 통과한 칸), `makeNormalTerrainFarmWorldHooks`(청크 지역 signed BYTE·현재 섬 owner·가상 소유자 분배·월드 Pop/Unpop/Repop; 섬 런타임/HP 장부가 다른 월드이면 효과 전에 거부).
- 모든 참조는 소비자보다 오래 살아야 하며 CRT·puzzlePiece·군도 상자는 사용 위치마다 현재 값을 읽는다. CD(10.72)는 옮기지 않았다.
- **기준선 뒤 변경**(`f7f58f5a`: placementState 필드 접근자 `IslandType()`/`EdgeFarmType()`): 웹 상태가 이미 `currentIslandType()`/`currentEdgeFarmType()` 로 현재 값을 읽는 것과 같다(3.6절 ①).

## 2. 시험으로 찾은 결함

청크 항목의 `side` 표현이 모듈마다 달랐다: 타입 모델과 E292 `RawTerrainGrowth` 는 글자 문자열, 실제 `RawIslandBuilder`(E222)는 BYTE 코드 번호를 쓴다. Fort 적재 장면(E303)이 Builder 가 놓은 청크로 성장을 돌리자 `Invalid terrain side` 로 멈췄다 → `raw-terrain-growth.js` 가 번호면 글자로 바꿔 `connectionMask` 에 넘긴다(대응표 E292 행 메모 갱신).

## 3. 검사 (`tests/normal-terrain-world-binding.test.mjs` 9건)

원본 관측 파일이 없는 단위 계약 검사이다. cpppj `NormalTerrainWorldBindingTests.cpp` 의 세 장면(두 섬의 실제 성장·도장·군집 후처리와 CRT 캐시의 첫 요청만 소비·독립 FrameAt 사전 관찰과 게임 난수/CRT/캐시 대조 / 새 배치의 121 생성·owner·HP·게임 난수 / 누락 효과·미복원 분기 거부)을 10.78 로 옮기고, 군도 후보 상위의 관계식 스모크(spacing 두 번 조회·span·백분율 환산·플레이어 배치 호출·군도 DWORD)와 요청 범위·전역·구성 검사, 패턴 셈 범위, 배치 훅의 월드 몸체, 후보 조건 조합(하나라도 거짓이면 일반)을 더했다. 입력은 합성 지면 97·edgeFarm 121·puzzlePiece 107 과 합성 SHP 이다.

**변형 확인**(88개): 처음 65개가 남았고 보강 뒤 **26개**가 남았다. 모두 분류한다:
- **동등/하위 오류와 같은 클래스** — 청크 범위 `bottom > 15`(하위 `setBounds`/성장이 같은 `RangeError`), 패턴 번호 범위(하위 6비트라 표 68개 안에 항상 있어 도달 불가), `넓이 0` guard(유효 청크 범위에서 0 이 안 됨), 가상 소유자 분배 래퍼(지면/121 은 기본 분배와 같음).
- **이 장면에서 관찰되지 않는 연결** — MarkEdges/Normalize(경계 모양이 이 장면의 프레임 소비를 바꾸지 않음), 도장의 `chunkRegion`/진단 두 개(모든 칸에 유효 지역이 있어 noRegion 조회·진단이 불리지 않음), 첫 record 선택(순서·부호·없음; 두 지역의 record[0] 이 같아 패턴 칸 수가 같음), 패턴 회전(회전해도 칸 수가 같음), 섬 선택 필터·섬 수 부호·군집 사각 곱(retype 대상 셀이 같아 결과가 같음).
- **군도 상위 산술의 원본 oracle 없음** — 확장폭·확장 bounds·시드 계수·넓이·목표 범위/두 번째 난수·gap/희소 정리 호출: cpppj 에도 상위 합성의 원본 fixture 가 없고 부품 몸체는 `terrain-arch-helpers-x86.tsv` 등으로 따로 검증됨. 관계식 스모크만 있으며 L2 비교(4-D)에서 확인한다.

## 4. 남은 경계

- 3.1절 4-B 는 33/35 이다. 다음: client RawFortWorldLoadBinding(E303), 4-B 적재 통합 시험.
