# 일반 지형 성장·초기 도장·edgeFarm의 실제 월드 연결

## 범위와 현재 상태

- 기준 완료 커밋은 **5b10ce8 (`20261014-01`, 단위09)**이며 당시 Release 경고/오류0·전체1010검사가 통과했다. 이번 새 소비자의 검사 결과와 구분한다.
- 작업 전에 최신 `AGENTS.md`와 두 `LEFT_JOBS`를 다시 읽었다. 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다. `normalterrainbinding` 보조 내보내기는 root의 순차 readOnly 실행으로 패치2/CD4 모두 성공했다.
- 새 `RawNormalTerrainWorldBinding`은 기존 완료된 성장·초기 도장·단일 배치·군집을 한 `RawSurfaceWorld`에 연결한다. 기존 모듈·원본·CMake·LEFT·CoreData·GameWorld·dotnetpj는 이 작업에서 편집하지 않는다.
- **새 독립 원본 실행 입력0, 새 assert 관찰0, 게임/GUI 실행0**이다. 실제 소비자 검사3개를 준비했으며 공유 빌드/집중 실행은 root가 진행한다. 아래 검사는 현재 준비 단계이며 성공으로 세지 않는다.
- 전체 `IslandBuilder/Fort` 부모, archipelago, 미션 로더의 실제 호출 설치 및 캠페인 완주 동일성은 완료하지 않았다. 새 API는 호출자가 원본 분기를 확인한 `Normal`만 실행한다.

## 원본 호출 순서

| 작업 | 10.78 | CD 10.72 | 이번 연결 |
|---|---:|---:|---|
| 상위 섬 생성 후처리 | `0046da70` | `00490530` | 일반 지형 뒤의 현재 섬별 postpass만 연결 |
| 일반/군도 wrapper | `004c1510` | `0049e620` | 정상 분기만 연결, 전체 wrapper 완료 아님 |
| context constructor | `004c0390` | `0049cdd0` | 기존 `RawTerrainContext` |
| 일반 성장 | `004c10a0` | `0049e160` | 기존 `RawTerrainGrowth::RunNormal` |
| 경계 문자 표시 | `004c09a0` | `0049d890` | 같은 context의 `MarkEdges('x','=')` |
| 초기 도장 | `004c0640` | `0049d2c0` | 기존 `RawTerrainStamp::Stamp('x')` |
| 도장 뒤 문자 정규화 | `004c0ad0` | `0049d9a0` | 같은 context의 `NormalizeAfterStamp` |
| CD 후처리 묶음 | 위 세 호출 | `0049e4c0` | 정확히 경계→도장→정규화만 호출 |
| 현재 섬 개수 | `0046ee50` | `004bd4f0` | 같은3204BYTE의 `RawIslandRuntime::Count` |
| 첫 지역 record | `00432430` | `004047c0` | 현재 `ChunkMap`의 Y행/X청크 순서·signed BYTE 비교 |
| 청크 Bounds | `00432210` | `004040d0` | 기존 `ChunkMap::Bounds`, 판본 진단은 별도 필수 callback |
| cell rectangle 변환 | `0040d900` | 상위 인라인 | 유효 청크 각 경계를16배해 float로 전달 |
| 패턴 유효 칸 수 | `004bfde0` | `0040b9d0` | 현재 puzzlePiece 프레임+기존 `CanonDecoder` |
| edgeFarm 군집 | `0040e560` | `0042cd80` | 기존 `RawFarmCluster::Place` |
| 실제 단일 배치 | `0040db90` | `0042c3d0` | 기존 `RawFortRandomPlacement::Place` |

`Generate`는 context를 생성하고 성장→경계→도장→정규화를 수행한 뒤 context를 해제한다. 그 다음 상위의 현재 섬별 군집 postpass를 실행한다. context의 문자/지역/mapping 배열은 같은 저장소이며 성장 결과를 다른 지형 마스크로 대체하지 않는다.

상위 postpass는 번호0부터 **매번 현재 signed Count**를 다시 읽는다. `selectedRegion == 현재 noRegion` 또는 `selectedRegion == 번호`인 항목에 첫 record→Bounds→cell rectangle→패턴Count→군집 순서로 호출한다. Exists를 이용한 추가 필터를 넣지 않는다. 삭제된 항목의 원본 진단과 계속 진행은 필수 callback으로 구분한다. CD에는 첫 record의 missing/found 및 Bounds의 found 진단이 더 있다. 원본 첫 record가 없는 경우의 실제 영6BYTE 반환은 보존한다.

패턴Count는 record 첫 BYTE의 하위6비트를 패턴 번호로, 상위2비트×2를 방향으로 사용한다. 원본은 현재 puzzlePiece 타입으로 `CanonDecoder`를 생성해 유효 칸만 센다. 이번 소비자는 같은 월드의 현재 프레임과 기존 영역 패턴 표/decoder를 재사용한다. 빈 셀과 누락 프레임은 기존 decoder가 건너뛰며 record의 flag/seed를 패턴 수로 오해하지 않는다.

## 같은 월드의 실제 효과

`RawSurfaceWorld::Core()`에서 얻은 동일 Pool/hash/spots/타입/프레임/SHP/Factory/Owner/Pop/장부를 빌린다. `MakeTerrainStampWorldHooks`는 원문 그대로 재사용한다. 초기 도장은 실제 Factory→owner0→직접 프레임→Shape Pop→공통 후처리→같은 raw 지역WORD setter 순서다. 새 소비자가 생성자나 프레임/HP/공간/소유자 효과를 기본값으로 대신 제공하지 않는다.

`MakeNormalTerrainFarmWorldHooks`는 기존 `MakeFortRandomPlacementHooks`로 전체 MayPlace·Factory/Create·실제 Construct·HasHitPoints·TypeHitPoints를 연결한다. 그 뒤 같은 `ChunkMap`, 실제 `RawIslandRuntime::Owner`, `MakeOwnerDispatch`, 월드의 `Unpop/Repop/Pop`을 공급한다. Pool뿐 아니라 HP의 장부 참조도 같은지 검사한다.

실제121 생성자는 패치 **00462630→0050a8e8**, CD **00416d80→00500b10**이다. 두 표의 `+0x20`은 Damageable 후처리다. 월드 기본 구성만으로 이 파생 Pop은 허용되지 않으므로 **같은 Pool의 완료된 `RawCarrierPostPop`을 필수 인수로 받아 명시 등록**한다. 실제 Damageable→공통 장부와 지면 owner 효과를 호출자가 연결해야 한다. 존재하지 않는 Carrier `+0xcc` 효과를 noop으로 채우지 않는다.

반면 실제 타입97 isle·98 edge·107 puzzlePiece의 양판본 생성자 슬롯은 원본부터0이다. 지면97은 기존 실제 Factory의 원본 base 생성 계약을 실행한다. 이는 미복원121 생성자를0으로 치환하는 동작과 구별한다. 새 소비자는 현재 타입의 ctor 주소가 원본 타입표의 실제 값과 같은지 확인하며,121의 임의0 주소는 거부한다.

기존 전체 배치 pipeline의 private 입력 span은 새 API에서 조회할 수 없다. 따라서 caller는 같은 Core의 타입/SHP/hash/spots/장부와 같은 불변 프레임 입력으로 pipeline을 구성해야 한다. Pool identity를 확인하지만 private span identity까지 감사한 것으로 주장하지 않는다. 이번121 검사에서는 특수 vortex 지역 제한을 조회하지 않으므로 해당 목적의 섬 목록 사본을 만들지 않는다. 군집/owner/count는 실제3204BYTE 목록을 직접 사용한다.

## 난수와 캐시 수명

| 저장소 | 실제 소비자 | 연결 계약 |
|---|---|---|
| 성장 caller 로컬 seed | 섬별 청크 방문 번호×`0x10e3`+record seed | 게임 전역 난수를 소비하지 않음 |
| 군집 caller 로컬 `GameRandom` | 첫 record seed BYTE×77, 중심Y/X | 기존 군집 몸체에서만 유지 |
| 공유 게임 `GameRandom` | 초기 Choose 및 단일 배치 random-frame | 동일한 참조를 성장/도장 후 단일 배치에 전달 |
| 별도 CRT holdrand | 초기 도장99cache | 첫 FrameAt에서만99회 소비 |
| 수명이 긴 `TerrainStampState::variants` | 후속 초기 도장 | 다음 context에도 같은cache 재사용 |

CD 로컬 PRNG의 디컴파일 prototype이 this를 생략한 것은 원본에서 전역 난수를 사용한다는 뜻이 아니다. 앞09 실제 ECX/로컬 seed 감사와 완료 몸체를 그대로 따른다. 초기 도장 cache를 소유권 도장 `RawRegionPaint`의 별도99cache와 합치지 않는다.

## 새 소비자 검사3개

1. **ActualNormalGenerationAndFarmRetype**: 두 판본에서 같은 월드의 두 섬을 순서대로 성장→도장→군집 후처리한다. 원본 ctor0 지면과 실제121 ctor/vtable·HP·같은SID/일반 해시 체인·지역WORD·초기 장부·현재 섬owner를 확인한다. forcePlacement는0이며 실제 전체 MayPlace를 실행한다. 기대 난수는 같은 입력에 기존 `FrameAt`만 호출해 관찰하므로 새 독립 PE oracle로 세지 않는다. CRT99cache는 첫 요청만 소비하고 다음 context에도 재사용한다.
2. **ActualFarmCreateOwnerHpAndGameRandom**: 합성 random-frame121 metadata에 실제 단일 Place를 실행한다. 현재 섬owner2→실제 Owner/HP→프레임GameRandom1회→Shape Pop/장부/같은hash를 확인한다. 로컬 seed와 CRT는 유지된다. 이 입력 프로필은 실제 소비자 연결 검사이며 원본 자산 metadata 전체를 새로 확정한 것으로 해석하지 않는다.
3. **RejectsMissingEffectsAndUnrestoredBranches**: missing callback·다른 Pool/섬·군도/미복원 부모 및 실제 ctor 슬롯 불일치를 효과 전에 거부한다. 기존 decoder의 방향×2와 유효 패턴 수도 확인한다.

프레임/SHP는 이 소비자 연결에 명시한 합성 입력이다. 게임의 실제 자산 전체 로딩/모든 fringe/지역별 미션 배치를 완료한 검사는 아니다. API는 유효 내부 청크1..15만 수락하며 전체 부모의 손상 bounds/assert 경로를 복원했다고 주장하지 않는다. 로딩 깊이·실제 Carrier/HP/pipeline의 수명과 상태 동기화는 caller가 유지한다.

## 출처와 교정 lineage

`tools/analyze_normalterrain_world_binding.py --verify`는 **41개 기존 의존 파일의 전체 원문**, 기준5b10ce8 Git 원문 및 당시 fixture/report/tool SHA를 보존한다. Git LF와 작업 파일 CRLF는 각각 원문/SHA를 기록한 뒤 줄 끝 차이만 확인한다. 기존 Context224/Growth108/Stamp1560/Farm208/IslandRuntime2018의 원본 관찰은 모두 **이번 신규 합산0**이다. 원본 PE·기존 terrainworld10/10·새 helper2/4의 전체 파일 SHA도 기록한다.

공유 빌드 전 정적 감사에서 다음 검사 입력/기대를 교정했고 production/native는 변경하지 않았다.

- 일반 해시의 머리가 모든 대상SID라는 첫 기대를 실제 raw `+4 WORD` 체인 포함 검사로 교정했다. 이전 전체test/report는 `hist/normalterrai-4f241`에 보존한다. 최초 도구의 Windows 기본 CRLF 쓰기/역변환 실패도 전체 도구·출력으로 보존했고 최종 UTF-8 바이트 쓰기는 원래 LF를 유지한다. `analyze_normalterrain_binding_cpp_correction.py` 기본 실행이 한 검사 역변환을 감사한다.
- 합성121 flags1에 잘못 추가한 surface `0x800`을 제거했다. 원본 MayPlace의 island 후보 무시 조건은 `other island && placing nonsurface`이므로 정상 terrain-frame `0x200012`/random-frame `0x100012`를 사용한다. 이전 전체test/report/hash-lineage는 `hist/normalterrai-6a80b`에 보존한다. `check_terrain_profile_fix.py` 기본 실행이 입력 한 행의 역변환과 이전 해시 교정 출처를 감사한다.

세 감사기는 모두 현재 출처에 통과했다. 새 소비자 SourceSHA는 이전 전체 원문과 교정 연결을 통해 보존하며 원본 관찰 도구/fixture를 현재 C++ SHA만으로 덮어쓰지 않는다.

## 완료하지 않은 상위 분기

패치 wrapper의 군도 후보는 `00540a2c != 0 && 00594fbc != 0 && 00594fa0 == 0 && 현재Count == 1`이며 추가 `0052f7e5 == 0` 조건에서 군도 경로를 실행한다. CD의 **0049e5e0**은 `00535ae0 != 0 && 00540a20 != 0 && 0052e16c == 0 && 현재Count == 1`을 판정한다. 군도는 범위 확장·성장 밀도·플레이어 배치·후처리 전역 설정까지 포함하므로 이 일반 소비자는 실행하지 않는다. 상위의 inWorld 진단(`00490520`, CD world map `005484c0`, patch `0054c8a0`)과 전체 Fort 로드 역시 이 API가 완료한 범위가 아니다.

LEFT 갱신/공유 CMake/빌드/통합 검사/커밋은 root 전담이다. 이번 문서는 새 소비자 범위만 기록하며 게임 실행 가능성이나 캠페인1-1 전체 동일성 완료를 의미하지 않는다.
