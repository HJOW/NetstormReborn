# 실제 IslandBuilder Place/PlaceAll과 현재 섬·청크 저장소

작업 및 마지막 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다. 작업 전에 최신 AGENTS.md와 두 LEFT_JOBS를 다시 읽었다. 완료 단위10 기준은 **a349e63 / 전체1023검사 성공**이며 이 문서의 새 독립 관찰은 단위11에 속한다. 보호 원본·AGENTS·기존 CMake/LEFT/GameWorld/RawSurfaceWorld는 수정하지 않는다.

## 원본 주소와 읽은 범위

현재 정밀 내보내기는 `extracted/islandbuilder/{originals,originalCD}/{creation.c,functions.tsv}`의 **20/20 함수**다. 주소 목록은 `tools/ghidra/islandbuilder-functions.json`에 있다.

| 역할 | 10.78 | CD10.72 |
|---|---|---|
| builder 최초 생성 | 0046d9b0 | 00490430 |
| 현재 지도 연결·기본 bounds·owner1 | 0046da20 | 004904d0 |
| 현재 지도와 전역 world 주소 비교 | Place 내부 비교 | 00490520 |
| 실제 단일 Territory 배치 | **0046dba0** | **00490750** |
| 실제 현재 Territory 순회 | **0046dd70** | **00490980** |
| 기본 청크 사각형 | 00431fc0 | 00403d20 |
| 청크 getter | 00432180 | 00404090 |
| 섬 청크 bounds | 00432210 | 004040d0 |
| 실제 Exists | 0046ee30 | 004bd4c0(004bd480 validId) |
| 실제 Add | 0046ee60 | 004bd500 |
| 실제 float Bounds setter | 0046f190 | 004bd900 |
| CanonDecoder 생성·Advance | 00425c20 / 00425860 | 0041fcf0 / 0041feb0 |
| assert FindNumber 검색 | 0049a940 | 004442f0 |
| CRT float→정수 | 004e49c0 | 004f161c |
| 현재 프레임 방향 getter | 00425c00 | 00420370 |

CD validId가 호출하는 **004bd4f0 Count**의 3바이트 전체 몸체는 현재 호스트의 기존 `extracted/island-runtime/originalCD`에서 읽기 재사용한다. 선택한 정확한 범위 `004bd4f0-004bd4f2`를 새 보고서 `reused_ranges`에 기록했다. 이것과 기존 CanonDecoder/ChunkMap/Runtime 실행 진입은 새 독립 완료 수에 더하지 않는다.

상위 저장 월드 **004b5f70/CD004330c0**의 전체 정밀 C를 읽은 경로는 각각 `extracted/refined/originals/raw.c`, `extracted/refined/originalCD/raw.c`다. 상위 순서와 Money/Badges/Technology 기존 추정 정정은 `cpp-fort-initial-world-order-audit-20261014.md`에 기록했다. 이번 단위는 그 부모 전체를 복원하지 않는다.

## 실제 Place 순서

1. builder 사각형의 네 필드 중 `-1`이 있으면 `cRect.isValid()` 진단 후 계속한다. 원본 null 지도 진단과 손상 포인터 실행은 호스트의 필수 참조 계약 밖이다.
2. 위치를 무시하지 않으면 record BYTE2의 낮은/높은 nibble을 X/Y 오프셋으로 읽는다. 무시하면 모두0이다.
3. 현재 지도 주소가 전역 world 지도 주소와 **동일할 때만** 실제 Runtime.Add를 먼저 호출한다. 반환 번호는 더하기 전 count다. 임시 지도는 전달 Territory DWORD를 그대로 반환한다.
4. 현재 left/top에 오프셋을 더하고 float32 원점을 만든다. 현재 puzzlePiece 타입·모양 low6·**방향 high2 그대로**를 실제 CanonDecoder에 전달한다. PatternCount 경로의 `high2×2`를 여기로 가져오지 않는다.
5. 첫 유효 칸부터 실제 `_ftol` 좌표의 현재 청크를 얻는다. 지역 BYTE, record6BYTE, 유효 칸 ordinal, 실제 프레임의 방향 BYTE를 쓴 뒤 Advance한다. 프레임을 찾지 못한 칸은 진단을 보고하고 건너뛰며 ordinal은 유효 칸에서만 증가한다.
6. 같은 world 지도일 때 전체 현재 청크에서 지역 번호의 bounds를 다시 구해 네 좌표를 `×16` float로 변환하고 **같은 32바이트 섬 기록**의 Bounds setter를 호출한다.

PlaceAll은 `[start,end)`의 실제120BYTE/20레코드를 파일 순서대로 다시 읽는다. BYTE1의 bit0이 켜지고 bit1이 꺼진 레코드만 현재 인덱스를 Territory로 Place한다. `start==end`는 `iStart != iEnd` 진단 후 빈 순회로 반환하며 역순 범위도 순회하지 않는다. 이름·소유자·지역별 정렬을 추가하지 않는다.

## 같은 저장소의 C++ 소비자

`RawIslandBuilder.h/.cpp`는 이미 완료된 `RawIslandRuntime.Add/Bounds/Exists`, `ChunkMap.At/Bounds`, `CanonDecoder`를 참조로 사용한다. 기존 축약 vector IslandList는 읽거나 복사하지 않는다.

- target/world의 주소 동일성은 `InWorld()`가 직접 비교한다. 같은 값의 임시 지도는 world가 아니다.
- owner DWORD와 현재 puzzlePiece 전역, builder bounds는 caller 수명의 참조다. C++ state의 기본값은 지도 연결이 끝난 `(1,1)~(15,15), owner1`이며 원본 생성 직후 null/미연결 상태의 standalone 실행 완료를 주장하지 않는다.
- Add는 현재 owner DWORD와 원래 owner BYTE를 각각 원본 폭으로 쓴다. 존재 DWORD·Territory BYTE·count만 함께 바꾸며 Temple/padding/기존 Bounds는 유지한다. 최종 Bounds만 실제 float 비트로 덮는다.
- Pool은 같은 실제 Runtime의 Pool 참조이며 독립 pool이나 섬 자료 사본을 만들지 않는다. 이 첫 청크 배치 단계는 Squid Factory/HP/Pop을 원본에서도 호출하지 않는다. 후속 실제 지면/edgeFarm 배치는 완료된 `RawNormalTerrainWorldBinding`이 담당한다.

## 두 판본의 진단 차이

| 경계 | 10.78 | CD10.72 |
|---|---|---|
| 홀수 high2 방향 | 그대로 `direction/2` 진행 | `(rot&1)==0` 진단 후 같은 순회 진행 |
| XY 청크 범위 밖 | XY를 합친 진단 **한 번** 후 clamp | 진단 없이 clamp |
| 프레임 방향·변형·번호 검색 실패 | line299 진단 후−1/skip | line0x123 진단 후−1/skip |
| 일치 청크 없는 Bounds | `(16,16,1,1)` 반환 | `found` 진단 후 같은 사각형 반환 |

새 builder의 진단은 필수 연결이다. 진단 없는 성공값/noop으로 미복원 효과를 대신하지 않는다. callback에 의해 자료가 변하는 모든 손상 경로를 이번 격자에서 관찰했다는 뜻은 아니다.

## 공유 디코더의 선택적 진단 확장

기존 RiftTypeFrames 검색은 실패 시−1을 반환하며 C++ CanonDecoder가 이를 건너뛰는 동작은 이미 있었다. 실제 assert FindNumber의 보고가 노출되지 않았으므로 부모와 협의해 패턴 생성자에 선택적 `std::function<void()> missingFrameDiagnostic={}`를 추가했다. 검색 직후 frame이−1이면 보고하고 기존 Advance를 계속한다. 기존 lookup/회전/셀/좌표/순회 몸체를 다시 작성하지 않는다. RiftType.h/.cpp 변경은0이다.

`recovery-history/islandbuilder-decoder-baseline`은 기준 a349e63의 전체 **78파일**을 보존한다. 기존 CanonDecoder/RiftType·패턴/타입 디코더·검사·관련 전체 보고서/fixture/tool과 Git LF/작업 CRLF를 분리했다. `analyze_islandbuilder_decoder_lineage.py` 기본 실행은 두 공유 파일의 지정3부분씩을 정확히 역변환하고 이전 전체 원문을 확인한다. 이 공유 확장의 새 native 합산은0이다.

기존 NormalTerrainWorldBinding/TerrainStamp 등의 보고서는 당시 SHA를 그대로 보존한다. 기존 감사기의 현재 파일 동일성 확인에서 이 두 공유 파일의 차이가 드러나므로, 현재 연결은 **이 successor의 전체 역변환 감사와 함께** 읽는다. 이전 관찰의 출처를 새 현재 SHA만 적어 지우지 않는다.

## 독립 PE 관찰과 입력 교정

`tools/decomp_islandbuilder_oracle.py`는 Unicorn에서 두 PE의 좁은 실제 함수만 실행한다. 실제 builder 생성·지도 연결부터 Place/PlaceAll까지 실행하고, 명시 assert 보고 경계만 기록하여 원본 계속 실행으로 돌아간다. 지도/섬 효과를 callback이나 C++ 결과로 계산하지 않는다.

- 원본 world ChunkMap/IslandList는 실제 전역 inline 주소에서 직접 실행한다. 임시 지도는 별도 주소다.
- 실제 청크256×12BYTE와 섬100×32BYTE/count DWORD를 모두 저장한다. 바뀌지 않은 record와 Temple/padding도 RLE로 보존하며 재생에서 제외하지 않는다.
- 64개 저장 모양×high2 방향0..3×world/임시지도, owner/Territory의 DWORD·BYTE 폭, 위치 무시, 정상/역순 프레임, 경계 clamp/미지정 bounds/99번째 Add, 빈/부분누락 프레임 및 PlaceAll flag/빈/역순 범위를 관찰한다.
- x87 control `0x027f/0x037f`의 반환·보존 레지스터·스택과 전체 저장소/진단이 같은지 확인한다. 게임 창·OS 호출은0이다.

첫 표본에서 CD validId의 기존 Count helper 누락을 발견해 전체 helper 범위를 읽기 재사용했다. 첫 전체 실행은 미분류 FindNumber 진단 line299에서 중단됐고 전체 당시 tool/CPP와 실패 로그를 `extracted/islandbuilder-initial-full-failure`에 보존했다. 중단/표본 실행은 신규 완료 수에 더하지 않는다.

첫1156격자의 부분누락 profile3은 A방향을 제거했으나 패턴3의 실제 방향은 O/G/N이었다. G방향 제거 입력으로 교정해 실제 중간 누락의 진단과 나머지 유효 청크 쓰기를 함께 관찰했다. 이전 전체 CPP/tool/report/fixture는 `recovery-history/islandbuilder-partial-profile-initial`에 보존했다. 첫 역변환 감사에서 방향 코드만 바뀌고 A설명이 남은 것을 발견해 설명도 교정했으며 당시 전체 감사 보고서와 바이트 SHA가 일치하는 소스를 별도로 보존했다. 최종 재관찰의 **profile3만24행 변경**, 나머지 **1132행은 원본 전체 행 동일**다. 재관찰1156을 신규1156에 중복 합산하지 않는다.

## 최종 원본 계수와 검사 상태

| 판본 | 고유 입력 | 정상 입력 | assert 입력 | 두 정밀도 정상 복귀 |
|---|---:|---:|---:|---:|
| 10.78 | 578 | 534 | 44 | 1156 |
| CD10.72 | 578 | 280 | 298 | 1156 |
| 합계 | **1156** | **814** | **342** | **2312** |

assert 입력은 각 fixture 행의 **마지막 실제 진단 횟수가0이 아닌지**로 구분한다. PlaceAll에서 호출한 Place의 반복 진입은 별도 신규 입력이 아니다. 판본별 실제 primary 진입은 Place1176/PlaceAll60이며 그 합을1156 고유 입력에 추가하지 않는다.

최종 fixture SHA-256은 `8048e92b263b026d17d02ccf7ddf8b0a3fefa0240970c8581cf5a3ea2b6c60a4`, native 보고서 SHA-256은 `fdb47e354e6f4a6f2aa36d7900bea9bd3ed7904be79d96284a99c239425bb26c`다. 관찰 당시 source/tool/export/PE와 전체 fixture SHA는 `recovery-islandbuilder-evidence.json`에 고정했다.

최종 주석 감사에서 검사 scene의 주소 동일성을 설명한 한 줄에 혼합 문자가 남은 것을 발견해 한국어로 교정했다. `recovery-history/islandbuilder-comment-initial`에는 이전 전체 test/native report/부분프레임 lineage/두 도구를 보존한다. `analyze_islandbuilder_comment_lineage.py`가 한 주석의 정확 역변환으로 C++ 토큰 불변을 확인하고 현재 test SHA를 별도 successor에 고정한다. 생산·원본 입력·fixture 변경은0이다. 이전 두 감사기의 **TEST 한 파일만 교정전 전체 snapshot으로 연결**하고 나머지 현재 SHA 검사는 그대로 실행하므로 관찰 당시 소스를 현재 주석 SHA로 덮어쓰지 않는다.

현재 다음 출처 감사가 통과했다. comment 감사에는 기존 native/부분프레임 감사도 포함된다. Root의 단위11 최종 **Release 경고/오류0·새15검사/실패0·전체1038검사/실패0·고유집중34검사 성공**을 확인했다. 원본1156행 전체 재생/현재 섬·청크·일반 성장 소비자/필수 자료 거부의새Builder3검사는성공이다. `IslandBuilder_` substring필터는기존2검사도포함하므로총5실행이며이를새5검사로세지않는다. 최초집중32집계의교정전전체tool/log도root가보존했다. 최종통합로그와전체계수는root의단위11기록을따른다.

```text
python tools/analyze_islandbuilder_decoder_lineage.py
python tools/analyze_islandbuilder_comment_lineage.py
```

새 C++ 검사3개는 양판본1156행 전체 청크/섬/진단 재생, 실제 RawSurfaceWorld의 같은 Pool/Hash/Runtime/ChunkMap→Owner·원래 ownerBYTE·CountOwned·일반 Growth 소비, 필수 진단/실제120BYTE 범위 거부를 검사한다. 소비자 검사는 새 독립 PE 고유 입력으로 계수하지 않는다.

## 남은 경계

이 단위는 연결된 유효 ChunkMap과 실제 물리100기록·20Territory의 배치를 복원한다. null 지도·isValid0/길이0 지도·타입/프레임 포인터 손상·물리100개 초과 Add/손상 Territory 포인터는 호스트 계약 밖이다. 프레임 입력은 정상/역순/빈/부분누락 합성이며 실제 자산의 모든 metadata를 로딩한 검증은 아니다.

GameWorld의 축약 청크/vector 섬 초기화는 아직 이 실제 builder로 교체하지 않았다. 저장 Fort 부모·Template 초기화/초기 Squid Kernel 바인딩·Terr→Chaff·contents Pop·Money/Badges/Technology·군도 및 캠페인1-1 전체 원본 동일성은 후속 통합 범위다. 기존 Runtime2018/Queries8490/Growth108/NormalWorld3 및 다른 helper 관찰의 신규 합산은0이다. 인수인계/CMake/최종 통합 계수와 커밋은 root 전담이다.
