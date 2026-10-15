# 로컬 간헐천의 실제 Player 배치 사각형

작성일과 마지막 정밀 디컴파일 PC는 **2026-10-15, VM-W11-Codex**다. AGENTS와 양 LEFT를 확인했고 `reverse-engineer-anything` 지침에 따라 보호 PE/기존 정밀 프로젝트를 읽었다. 원본 게임/GUI/OS 호출은0이며 보호 원본·AGENTS·dotnetpj를 변경하지 않았다.

`RawPlayerGeyserBounds`는 PATCH10.78의 `0048f5c0 → 00432430 → 0048f550 → 00432050 → 0040d900`을 복원한다. 실제 미션의 **같은 ChunkMap·Player9·TYPE188·프레임188**을 빌리며 기존 `TerritoryPatterns`, `CanonDecoder`, `kNeighborCells`를 재사용한다. `RawIslandRuntime::Bounds`나 현재 섬의 외곽 `ChunkMap::Bounds`를 대신 쓰지 않는다. 이 원본 사슬은 IslandRuntime의 Bounds를 조회하지 않는다.

## 원본 근거

읽기 전용 선택 export 계획은 `tools/ghidra/player-geyser-bounds-functions.json`이며, 결과는 `extracted/player-geyser-bounds/originals/creation.c`, `functions.tsv`다. 최초7함수 확인 뒤 실제 Dimensions/Canon/TYPE/CRT 하위까지17함수로 확장했다. 선택 export 수를 정상 원본 입력 수로 합산하지 않는다. `tools/ghidra/export_functions.ps1 -Name player-geyser-bounds -Edition originals -Refined`는 기존 정밀 프로젝트에서 재분석/저장 없이 실행했다.

| 주소 | 실제 역할 |
| --- | --- |
| `0048f5c0`, `0048f550` | 현재 Player flags/첫 지역, PlayerId 진단/주소차 계산 |
| `00432430`, `00432180` | 현재 청크 지도의 첫 signed 지역BYTE 일치, 실제 getter |
| `00432050`, `0041ca10` | 현재 미션/전투 분기의 배치 사각형과 옵션0~3의 간격 |
| `004bfd70` | 현재 영역 패턴과 실제 TYPE 발자국의 폭/높이 |
| `00425c20`, `00425860`, `00425700`, `00425b90` | 실제 Canon ctor/최초 셀 진행/셀 해석/발자국 Bounds |
| `0049a840`, `0049a940`, `0049a900`, `004e49c0` | 실제 TYPE getter/프레임 검색/발자국/CRT 절삭 |
| `0040d900`, `00431fc0` | 정수 청크 좌표의16배와 기존 기본 사각형 helper |

최종439입력에서는 기본 사각형 helper의 몸체 진입은 없었다. `00432050` 미션 분기는1/15를 실제 명령에서 직접 넣으므로 `DefaultChunkRect` 호출로 바꿨다고 주장하지 않는다. CD10.72는 이번 범위가 아니다.

## 현재 Player에서 결과까지

`Bounds(player)`는 다음 원본 순서를 보존한다.

1. 같은 raw Player의 `+0x50` flags BYTE에`0x22`가 없으면 진단한다. 진단 뒤 현재 Player의 `+0x78` region DWORD를 읽는다.
2. 같은 ChunkMap을 **y 바깥/x 안쪽**으로 조회한다. 각 항목의 island BYTE를 signed로 확장한 값이 region DWORD와 같은 **첫** 항목의 record6BYTE를 복사한다. 없으면6BYTE0이다. 영역 번호를 하위BYTE로 줄이거나 현재 소유자 목록에서 record를 재구성하지 않는다.
3. 원본 PlayerId getter의 metamap 진단을 처리한다.6BYTE record는 이 진단보다 앞에 값으로 보존된다. host API는 실제9기록의 정렬된 주소에 대응하는 Player0~8 번호만 받으므로 손상 포인터/비정렬 주소 차이 계산은 범위 밖이다.
4. `Placement`의 현재 전투 모드 진단, Player1~8 진단을 처리한 다음 현재 미션 모드를 다시 읽어 분기한다. 원본 진단은 호출한 뒤 계속하며, 진단 효과가 바꾼 현재 전역도 반영한다.
5. 정수 청크 사각형을 DWORD 감김의`<<4`로 변환하고 signed 정수를 float로 저장한다. 그 다음 left/top에서 ROM의 float4를 빼고 right/bottom에4를 더해 각각 float로 저장한다. 최초 record의 청크 점유 범위나 섬의 현재 외곽으로 교체하지 않는다.

첫 일치의 native 물리 배치는 `x*16+y`지만 검색은 y행/x열이다. 서로 바꿔도 한 영역의 record가 항상 같을 것이라고 가정하지 않는다. BYTE`255`/`128`은 각각 region`-1`/`-128`에 일치하며 region`255`/`128`에는 일치하지 않는다. 유효 지도에 존재하는 기본127 항목 역시 실제 첫 일치 후보다.

## 미션과 전투의 서로 다른 배치

미션 분기는 record의 하위6bit 패턴과 상위2bit 회전을 기존 Canon에 전달한다. `004bfd70`은 **회전값×2**를 Canon의 direction으로 넘기며, 이 계약을 원본 `RawIslandBuilder::Place`의 direction 인자와 혼동하면 안 된다. 현재 puzzlePiece 타입과 실제 프레임 표로 최초 Advance/프레임 검색을 실행하고, 현재 TYPE의 footX/footY를 `CanonDecoder::Bounds`에 넘겨 폭/높이를 구한다. 프레임을 찾지 못하면 진단하고 다음 셀을 계속 찾는 기존 Canon 동작도 사용한다. 패턴의 width/height만 읽어 하위를 생략하지 않는다.

배치 시작은 `x=1+(record[2]&15)`, `y=1+(record[2]>>4)`이고 끝점은 이 시작에 위 폭/높이를 더한다. 회전된 패턴 크기와 TYPE 발자국이 모두 영향을 준다. frame 부재로 유효 첫칸이 없어도 원본은 크기를 계산한다. current puzzlePiece 값은 helper 진입에서 보존하며 프레임 진단이 타입 전역을 바꾸더라도 이미 생성한 decoder의 TYPE와 발자국 조회 순서를 유지한다.

비미션 전투 분기는 현재 Player별 방향표 `0053181c`를 읽는다. Player1~8의 원본 초기 순서는 **4/0/6/2/7/3/5/1**이다. 현재 옵션 BYTE`0052f7e6`의0~3은 각각 간격3/4/5/6이며, 북부터 시계 방향인 기존 `kNeighborCells`의 짝수 방향으로 중심`(9,9)`에서 이동한다. 방향 lowbit가1이면 기존 방향의 수직 성분을 사용한 추가 이동을 한다. 원본은 간격을2번 또는4번 읽으며 마지막 사각형은 끝점에서2를 뺀 시작점이다. 현재 방향표를 입력 참조로 받아 기본 순서에 고정하지 않는다.

현재 모드·mapSize·puzzlePiece·방향표는 `PlayerGeyserBoundsState`의 참조다. Player 기록이나 지도를 프레임마다 복사하지 않는다. mapSize4이상/물리 방향표 밖/잘못된 TYPE와 정렬되지 않은 Player 포인터는 원본의 stack/메모리 범위 밖 실행을 흉내내지 않고 명시 오류다. 기존 Canon의 host 정수 산술 범위를 넘는 손상 발자국 역시 기존 오류 계약을 유지한다.

## 독립 실제 PE 검사

`tools/decomp_player_geyser_bounds.py`는 C++ 구현/출력을 읽지 않고 위17함수의 원본 기계어만 허용한다. 실제 Player, ChunkMap, TYPE와 프레임을 명시 가상 입력에 구성하며 **assert 보고만 외부 관찰 경계**다. PlayerId/배치/Canon/TYPE발자국/CRT/좌표변환을 결과 경계로 대체하지 않는다. 정상 EIP/ESP·비휘발 레지스터·x87 제어어`027f`/TOP0을 확인하고 허용되지 않은 실행/쓰기를 거부한다.

최종 **439고유입력·439정상반환**, 원본 진단137회다. 최초438입력에 record를 값으로 보존하는 진단 효과1개를 추가했고, 동일 입력의 재생성을 별도 고유 수로 더하지 않는다. 실제 몸체 진입은 Player bounds376, region record418, 현재 chunk getter27,188, PlayerId376, 배치397, Dimensions/Canon ctor/Advance/Bounds/TYPE발자국각356, 셀 해석644, 프레임 검색434, TYPE getter/CRT절삭각712, 청크변환376, 옵션 간격122회다. helper 호출 수를 고유 입력 수에 합산하지 않는다.

입력은64패턴×4회전 전체, Player1~8×옵션0~3×미션/비미션, 현재 방향표 변경, 분리된 map크기16×16/3×4/1×1, signed region 경계, 첫 y행/x열 중복/부재, TYPE 발자국1×1/2×3/0×1/−1×2, flags와 전투/메타맵 진단, 모든 frame 부재, direct 배치의 Player0/−1/9/INT_MIN/MAX 진단 후 미션 진행을 포함한다. 진단이 현재 region·미션 모드·mapSize·TYPE발자국·타입 전역·원본 record를 바꾸는 순서도 검증한다.

`PlayerGeyserBoundsTests.cpp`는 같은 입력에서 출력 **16BYTE 전체**(record6BYTE 뒤의 stale10BYTE 포함), 현재 모드/옵션/타입/발자국/Player region8필드, 진단 순서를 fixture와 직접 비교한다. 기대 범위를 C++ 구현으로 다시 만들지 않는다. 두 번째 검사는 필수 진단 누락 거부다.

루트 통합 **Release 게임/검사 빌드는 경고0·오류0**, 집중 **2개·실패 체크0**이다. 빌드 로그는 `extracted/run47-49-build.log`, 집중 로그는 `extracted/player-geyser-bounds-root-focused.log`다. 별도 중복 빌드는 하지 않았다. 최종 `python -X utf8 tools/decomp_player_geyser_bounds.py --verify`의439행 파일 SHA/입력 계획/정상 반환 감사도 통과했다(`extracted/player-geyser-bounds/final-verify.log`). 문서를 포함한 소유8파일 UTF-8/공백 오류0을 확인했다. 이 모듈의 직접 GUI 검증은 없으며 다른 단위의 본문/UI 검사를 여기에 합산하지 않는다.

보호 PE SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`다. fixture/보고서는 `cpppj/tests/fixtures/player-geyser-bounds-x86.tsv`, `cpppj/player-geyser-bounds.json`에 있다.

## 같은 Session의 실제 생성 연결 경계

읽기 전용 조사 시점의 Session은 같은 raw 월드·Player9·TYPE/프레임/spot/hash를 이미 소유한다. `RawSurfaceWorld::Create/Pop`과 `Core().factory`가 동일 풀을 사용하며, 최초 Pop의 firstPop 정규화 뒤 `RawAssetPostPop::Geyser`가 전체 장부/자원 WORD/Regular/AI 등록을 실행한다. 이 경로는 최근 생성5목록과 별개다.

아래 실제 연결은 아직 미공급이다.

- `RawCampaignSession::Impl`에는 전용 최근5목록·`LocalGeyserState`/자원 pool·`RawLocalGeyser`/현재 Player bounds 소유자가 없다. 기존 `assetState.geysers`는 전체 장부이며 최근 목록으로 넘기면 안 된다.
- noIsland의 실제 `RawConstructionConfirm → RawConstructionNotice → RawConstructionClear/Place`는 개별 몸체와 같은 풀 어댑터가 있지만 client Session에서 아직 생성하지 않는다. `metadata.frames`/현재 SP/owner/TYPE/raw 풀을 기존 어댑터에 공급하고, 실제 메시지`0x3f` 전송/자기 통지 처리 및 최종 refresh도 연결해야 한다. Terrain의 직접 생성으로 우회하지 않는다. `RawBattleBroadcast`는 이미 단일 Session에 있으나 ConstructionNotice의 실제 packet 공급은 별도다.
- `world.Destroy(empty84,0x400000)`는 현재 hits 분배의 CommonAsset83/impact에 포함되지 않는다. `RawCampaignHitBindings::Destroy`는 다른 자산을 `services.destroy`에 넘기고 `CampaignClientServices`는 이 파생 경계를 아직 공급하지 않는다. 따라서 일반 base 삭제로 임의 확장하지 말고 실제 empty84의 가상 삭제/Pre/Post를 확인해 연결해야 한다.
- Geyser PostPop은 현재 Player AI 포인터를 번호순으로 조회한다. `RawGeyserAiRegistration`은 첫 nonnull AI의 실제 미복원 요청을 예외로 남긴다. AI 포인터를0으로 덮거나 그 통지를 무시하여 생성 완료로 만들면 안 된다.
- `RawTutorialOne` C는 실제 BuiltForGenus4가18보다 크면 `Advance` 뒤 반환값과 무관하게 생성한다. D는 같은 전용 최근 목록/현재 신전Graph와 비교한다. 생성 성공 뒤에만 임의로 단계를 올리거나 최근 목록을 전체 장부로 바꾸면 원본 순서와 다르다.

이 모듈/독립 검사는 Session 실제 설치·Early Missions/캠페인1-1~1-2 완주·건설/다리/수확·운반·진행 저장·UI/음향 동일성의 완료 검증이 아니다. 요청에서 제외한 DirectDraw 전체화면 오류는 이번 범위에 없다.
