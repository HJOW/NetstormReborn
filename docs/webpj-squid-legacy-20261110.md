# webpj Squid 저장 필드·SquidSpatial 소비자 확인과 분류 판정 — WB04-E246

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-B의 넷째·다섯째 항목(`Squid`·`SquidSpatial`)을 한 단위로 판정했다. 두 항목은 새 웹 코드를 쓰는 일이 아니라 **실제 소비자를 기준선과 현재 cpppj 에서 확인해 이식 범위를 확정하는 일**이다. 코드 변경은 `tools/port-map.mjs` 의 분류(`exclude-legacy`)와 대응표·문서뿐이고, 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다. 기준선 뒤 cpppj 변경은 `Squid.cpp/.h`·`SquidSpatial.cpp/.h`에 없다(저장소 루트 `git diff --numstat 1f00a25a HEAD`가 빈 결과).

## 1. 소비자 확인 결과

`git grep` 으로 기준선(`1f00a25a`)과 현재 작업 트리 모두에서 `o/Squid.h`·`o/SquidSpatial.h`·그 이름들의 사용처를 추적했다.

| 단위 | 사용처 | 판정 |
| --- | --- | --- |
| `Squid::EffectiveGenus` (원본 004afd30) | `RawGraph`·`SquidPop`·`SquidUnpop`·`RawSquidPopParent`·`RawSquidUnpopParent`(모두 실제 raw 경로) | 웹이 이미 `core/squid-space.js` 의 `effectiveGenus` 로 이식했고 `tests/squid-unpop.test.mjs` 가 `hash-x86` Genus 3,216행과 `+0.9999` 보정·범위 밖 거부 계약을 대조한다. **Squid 의 웹 소비자 몫은 이것이 전부다** |
| `struct Squid` 저장 필드·`Face`·`Advance` | `client/GameWorld.h`(`std::vector<o::Squid> objects_`)와 `client/UberGump.h` 뿐. 시험은 `WorldTests.cpp`(`World_*` 이동 시험) | 옛 `GameWorld` 어댑터의 이동 모델이다. `GroundGrid`/`GroundCell`/`CellPoint`/`kNeighborCells`(TerrainBuilder, 4-B)와 `FortContentList`(Template, 4-B)가 선행이다. **4-B 로 이월**한다(아래 2절) |
| `SquidSpatial`(`SpatialObject` 포함) | `o/CMakeLists.txt` 등록, `SpatialTests.cpp`, `PopTests.cpp` 마지막 시험뿐. production 소비자 없음 | raw 풀 이전에 원본 공간 갱신(Pop/Unpop의 spot·해시 등록)을 `std::map` 모델로 복원했던 옛 단위다. 같은 갱신을 `SquidPop`/`SquidUnpop`(raw 풀)이 대체했고 웹은 그 raw 구현(`squid-pop.js`·`squid-unpop.js`, `pop-x86`·`unpop-x86` 계열 fixture 대조)을 이식했다. **웹 이식 대상에서 제외**한다 |

## 2. 처리

- **새 분류 `exclude-legacy`:** raw 풀 이전의 옛 단독 모델로 production 소비자가 없고 raw 구현이 대체한 단위다. `tools/port-map.mjs` 의 `CLASSES`·`RULES`(`legacy-spatial`, 이름 `SquidSpatial`)에 추가하고 `--write` 로 대응표를 다시 만들었다. 제외 분류이므로 상태는 `-`이고, 분류 개수가 port 851·adapt 44·exclude-mp 6·exclude-edit 55·exclude-host 27·**exclude-legacy 1**이 됐다. 옛 fixture `spatial-x86.tsv`(238 시퀀스·878 전이)는 웹이 대조에 쓰지 않는다. 같은 기계어 갱신은 raw 경로의 fixture 가 이미 대조한다.
- **`Squid` 행:** `status` 는 `partial` 을 유지하고 메모를 "남음: 옛 struct Squid 저장 필드·Advance/Face(GroundGrid/CellPoint 의존, GameWorld 소비) → 4-B" 로 바꿨다. 3.1절 항목은 이 판정으로 닫고 4-B 단계 설명(3.2절)에 이월을 적었다. 웹에 `GameWorld` 를 이식하기로 하면(4-B 영역 B) 그때 `Squid` 와 `GroundGrid` 를 함께 옮기고, 이식하지 않기로 하면 같은 분류 판정을 한다.
- `Squid` 가 `GroundGrid` 의 `Path()`(8방향 최단 경로 어댑터)를 쓰지 않는다는 점, 그리고 `Advance` 의 이동 식(걸음 길이 `sqrt(2)`)이 원본 복원이 아니라 옛 어댑터의 근사라는 점을 cpppj 머리 주석으로 확인했다. 그래서 원본 동일성의 근거로 쓰지 않는다.

## 3. 검사와 한계

- 새 웹 검사는 없다. `tests/port-map.test.mjs`(분류 이름·규칙 이름 검사)와 전체 검사가 새 분류를 확인한다.
- 소비자 검색은 정적 검색이다. cpppj 의 동적 호출(함수 포인터 표 등)은 해당 파일에 `Squid`/`SquidSpatial` 이름이 없으므로 이 판정에 영향이 없다.

## 4. 다음 시작점

3-B 의 다음 항목은 `RawBridgeLifecycle`(WB04-D2 삭제 훅 뒤 남은 다리 가상 삭제 분배, 기준선 뒤 변경 +8)이다.
