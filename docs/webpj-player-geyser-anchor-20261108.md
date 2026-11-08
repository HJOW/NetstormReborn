# webpj Player 간헐천 범위·배치 기준점(RawPlayerGeyserBounds·RawPlayerPlacementAnchor) 이식 — WB04-E57

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 쉰일곱 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawPlayerGeyserBounds`(원본 0048f5c0 → 00432430 → 00432050 → 0040d900, 00431ff0 원점 배치)와 `o/RawPlayerPlacementAnchor`(0048fdb0 배치 기준점, 0048fd30 거리 선택, 00462b80/00462bc0 그래프 조회)를 옮겼다. 이로써 cpppj Player 파일(`RawPlayer*`)은 모두 웹에 대응한다. 간헐천 범위가 쓰는 `o/CanonDecoder` 의 패턴 반복자와 발자국 범위(00425c20·00425860·00425b90)를 `www/assets/territory.js` 에 보강했다. 세 파일 모두 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기는 옮기지 않았다.

## 1. 옮긴 것

`www/assets/territory.js` — `CanonDecoder(frameCodes, pattern, direction, x, y, onMissingFrame, diagnostics)` (추가)

- 생성자가 첫 유효 칸까지 한 번 진행한다(그 앞 칸의 프레임 누락만 알림). 10.78 은 checking 이 켜졌으면 고른 프레임 코드의 금 간 비트(0x20)를 진단(Canondecoder.cpp 0x325)하고 복귀하면 계속한다. `bounds(footX, footY)` 는 현재 좌표 절삭에서 순회 개수 × 발자국을 뺀 정수 사각형(유효 칸이 없어도 마지막 좌표). 기존 `decodeTerritoryPattern` 생성기와 같은 회전/셀 해석을 쓴다. 비패턴 한 칸 경로·다리 패턴 추첨은 아직 옮기지 않아 대응 행은 partial 로 둔다.

`www/core/raw-player-geyser-bounds.js` — `RawPlayerGeyserBounds(pool, chunks, types, frames, players, state, assertion)`

- `bounds(player)`: flags 0x22 진단 → 현재 지역 DWORD 의 첫 일치 청크 record 를 값으로 복사 → metamap 진단 → `placement` → DWORD<<4 signed float ∓ 4.0.
- `regionRecord`(y 행/x 열·signed 섬 BYTE 비교), `originPlacement`(원점 6,6 + 패턴 폭/높이), `placement`(전투 아님·Player 1~8 밖 진단 뒤 계속 → 미션은 패턴 폭/높이 + 위치 nibble+1, 전투는 현재 mapSize 간격 3/4/5/6과 현재 방향표의 북부터 시계 방향 이웃·홀수 대각선 보정 3×3).
- 패턴 폭/높이: 호출 시점 puzzlePiece TYPE 의 프레임 표로 `CanonDecoder` 를 만들고(누락 프레임 진단 MissingFrame) 그 뒤의 현재 TYPE 발자국으로 범위를 구한다. 상태 `{ battle, mission, metamap, mapSize, puzzlePieceType, playerDirections, decoderDiagnostics }` 는 매번 다시 읽는다.

`www/core/raw-player-placement-anchor.js` — `RawPlayerPlacementAnchor(pool, types, surfaces, bookkeeping, additional, contained, state)`, `PlayerPlacementAnchorState`, `makePlayerAnchorQuery`

- `inspect`/`locate`: 무효 그래프 조기 반환 → owner 작업장 목록을 매번 다시 읽으며 genus(flags2 & 6)·종속 내용물(E56 `RawContainedFinder`, mask 0x10·kind == type)·현재 FullDeck(종속 조회 뒤 재조회)로 허용한 같은 그래프 후보 40개 → 후보가 있거나 종속 허용이 있었으면 추가 목록의 같은 owner·그래프 → 하나면 그것, 여럿이면 `nearest`.
- `graphAt`(그래프 비활성 254, 좌표 + float 0.9999 double 절삭, 표면 지도 SID 의 raw graph), `graphFor`(SID 0 은 checking 이면 Graph.cpp 0x93 진단 뒤 0, 표면은 자기 graph, 그 밖은 좌표 그래프), `nearest`(owner BYTE 일치 후보의 double 거리·float 최소 9999, 10.78 은 NaN 미채택).
- `PlayerPlacementAnchorState.bindFullDeck(getter)`: 실제 0054db14 DWORD 를 빌린다(기존 `fort-object-load.js` 의 `readFullDeck` 과 같은 getter 방식).

## 2. 검증

- `tests/player-geyser-bounds.test.mjs`(3건): `player-geyser-bounds-x86.tsv` 439행(Bounds·RegionRecord·Placement). 같은 청크 지도·9기록·TYPE 발자국/프레임·모드와 진단 효과 다섯 가지(지역·모드/간격·발자국·패턴 타입·record 변경)로 출력 16 BYTE·효과 뒤 현재 8칸·진단 순서를 비교한다. 계약: 진단 장치 누락 거부, 같은 Canon 진단 소유자의 금 간 프레임 진단 0x325(cpppj `CanonConsumers_PlayerGeyserPatternDimensions`, 웹은 같은 조건의 프레임 코드로 구성)와 checking 0 이면 진단 없음.
- `tests/player-placement-anchor.test.mjs`(4건): `playeranchor-x86.tsv` 10.78 1598행(L 1296·G 60·S 180·N 62)의 반환·임시 후보 순서·raw 128슬롯 Adler·표면 지도 Adler. 계약: 손상 자료의 지연 진단과 조기 경로·SID 0 진단 복귀, 공유 FullDeck 현재 값, 실제 `SquidOwner` 장부 → 기준점·contained 타입 재조회·그래프 비활성(cpppj owner/bookkeeping 계약의 기준점 부분).
- 기존 지형/영역 검사(`territory`·`terrain*`·`fort`) 87건이 그대로 통과한다.

최초 실패와 교정: 두 모듈 모두 원본 2037행 전부를 첫 실행에 통과했다. 기준점 재생의 지도 체크섬 바이트를 값이 바뀔 때만 다시 쓰도록 검사 보조만 고쳤다(3.6초 → 1.8초).

## 3. 경계

- cpppj `player_anchor_owner_bookkeeping_permission_and_terrain` 의 후보 권한(`RawCanonPlacementPermission`)·지형 권한(`RawCanonPlacementTerrain`) 연결은 해당 파일 이식 때 옮긴다(`makePlayerAnchorQuery` 가 그 필수 경계다).
- 실제 Client 의 checking·FullDeck·Canon 진단 소유자 연결과 로컬 간헐천 소비자(`RawFortFinishOwner` 등)는 WB05 월드 조립과 해당 이식 단위에서 한다.

## 4. 다음 시작점

종류별 PostPop 본체(Asset·Fence·Nugget·Buried·Carrier·Bomb·Factory·Priest·Rune)·`SquidDisplay`/`RawSquidDisplayParent`·`SquidFrame`·`RawSquidName`·`RawSquidDefense`·`RawSquidSelection`·후보/지형 권한(`RawCanonPlacementPermission`) 이다.
