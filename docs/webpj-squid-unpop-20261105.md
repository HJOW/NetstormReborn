# webpj 공통 공간 해제(Unpop 부모)와 spot genus — WB04-D1

2026-11-05, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3(WB04-D)의 첫 하위 단위다. WB04-D(Form/Process 생성·삭제·Regular)는 모두 객체를 공간에서 빼는 공통 해제 위에 올라가므로, 그 바닥인 **공통 공간 해제 부모(원본 004afe50)**와 **spot genus 보정(004afd30)**, **건물 옆면 해제·다리 표면 통지**를 먼저 옮겼다. 화면 변경은 없다.

## 1. 근거

- 시작 트리 `c68e673b`. 다른 세션이 cpppj `SquidFactory`·`SquidForms`·`ContentFormHost` 등을 작업 중이라(미커밋) 이번 단위는 그 파일들에 의존하지 않는 바닥부터 골랐고, 근거는 모두 커밋본이다(작업 사본과 같음을 확인).
- 근거 소스: cpppj `o/RawSquidUnpopParent.cpp`(`ea848459`)·`.h`(`6ff267a1`), `o/Squid.cpp`의 `EffectiveGenus`(`65927742`), `o/RawBuildingSpace.cpp/.h`(`edc7db85`), `o/RawCheckingFlag.h`(현재 진단 DWORD를 소비 시점마다 읽는 뷰).
- 원본 관측: `cpppj/tests/fixtures/squid-unpop-parent-hardware-x86.tsv`(`6ff267a1`, 1300행), `hash-x86.tsv`의 Genus 3216행(WB04-B에서 남긴 행). 장면 구성은 cpppj `tests/SquidUnpopCheckingTests.cpp`(`fbdcb4df`)와 같다.

## 2. 파일과 계약

| 파일 | 내용 |
| --- | --- |
| `core/squid-space.js` (신규) | `effectiveGenus(flags2, x, y, width, height, cellX, cellY)`, `INTERIOR_GENUS_MASK`, `RawBuildingSpace`(surface·unpopSide·notifySurface·bindCurrentBridgeType·bindNotifyDiagnostics·validateSpace) |
| `core/squid-unpop.js` (신규) | `runSquidUnpopParent(pool, hash, spots, sid, flags, checking, hooks)` |
| `core/sid-pool.js` | `physical(sid)`: 상태와 무관한 물리 슬롯 쓰기 대여(아래) |

- **effectiveGenus:** 좌표를 float32로 만든 뒤 double로 0.9999f를 더해 0 방향 절삭한 오른쪽 아래 기준 발자국에서, 안쪽 칸이고 건물군 genus(`0x50444200`)이면 `genus|8`이다. 지붕 genus(`0x400000`)는 가운데 열의 위쪽 절반을 안쪽으로 보지 않는다. 해시 버킷의 단순 절삭과 다른 변환이다.
- **runSquidUnpopParent(10.78):** Form/contained/유효성 진단 → free|void면 끝 → void 비트 → 섬/다리면 미니맵 통지 → 발자국 y/x 순서로 각 칸에서 현재 TYPE·좌표를 다시 읽어 spot 비트 해제(검사 진단 0x617)·건물 옆면·표면 진단(0x629) → contained면 종속 종단으로 끝 → 단계 0~3의 현재 버킷 체인에서 자신을 빼기(SetNext 진단 `n != getSid()`) → 미발견 진단(0x65d) → 표시 갱신. `checking`은 원본 진단 DWORD를 돌려주는 함수이고 진단 뒤 바뀐 값을 그 자리에서 다시 읽는다. `hooks.type`은 같은 레코드 객체를 돌려줘야 한다(원본은 첫 TYPE을 참조로 들고 있어 진단 중 발자국 변경이 보인다).
- **부분 변경 보존:** 선택된 장치(`assertion`·`miniMap`·`side`·`contained`·`update`)가 없으면 그 시점에 오류이고, 이미 쓴 void·spot·체인 변경은 되돌리지 않는다.
- **물리 슬롯 대여:** 원본 해제 몸체는 진단 복귀 뒤 free 슬롯이나 표면 후보 슬롯도 그대로 읽고 쓴다. WB04-A 풀의 `allocated()`는 free 슬롯을 거부하므로 `physical(sid)`를 추가했다(cpppj의 `SquidUnpop`/`RawBuildingSpace` friend 접근과 같음). 일반 객체 쓰기는 계속 `allocated()`를 쓴다.
- **RawBuildingSpace:** 188칸 10.78 타입 표만 받는다. `unpopSide`는 표면 플래그·섬 확인 없이 유효 SID의 표면 WORD 낮은 바이트 비트 1/2를 지우고(높은 바이트 유지) 통지한다. 통지는 현재 다리 TYPE이고 매몰이 아니면 번호((WORD>>3)&15)가 7을 넘을 때 현재 진단이 비0이면 `num >= 0 && num <= 7` 진단을 부른다. Pop 쪽 옆면(`PopSide`·`HasIsland`)은 WB04-E에서 옮긴다.

## 3. 검증

| 검사 | 결과 |
| --- | --- |
| genus | 원본 Genus **3216행**의 두 결과 열 일치(비트 8이 붙는 행 포함), +0.9999 보정·가장자리·범위 거부 |
| 공통 해제 | 원본 **1300행** 전체: checking·타입/상태/extra BYTE·x/y 비트·타입70 발자국, 풀 슬롯 0~128(6450바이트)·네 단계 해시(172544바이트)·spot 지도(65536바이트)의 Adler-32, 요청 순서(A 진단/M 미니맵/N 옆면/D·Q 종속/U 표시) |
| 계약 | 진단 장치 누락 때 void·spot·미니맵이 남고 표시 갱신 없음, contained 종단 누락 때 상태 12 유지, 옆면 해제 비트·다리 통지 진단·현재 다리 TYPE 공급자·풀/해시 불일치 거부 |

- 음성 대조: 임시 사본에서 ① contained 분기를 지우거나 ② spot 비트 해제를 빼면 1300행 대조와 부분 변경 검사가 실패함을 확인하고 임시 파일을 지웠다.
- 처음 구현은 1300행 검사에 약 18초가 걸렸다. Adler-32를 zlib처럼 5552바이트마다 나머지를 구하는 방식으로 바꿔 약 4초로 줄였고, 여러 길이(0·1·5551~5553·65536·172544)에서 매 바이트 방식과 같은 값임을 확인했다.
- 신규 `tests/squid-unpop.test.mjs` 5/5. 이식 추적 목록에 `squid-unpop-parent`를 등록하고 `squid-hash` 항목에 Genus 소비자(`squid-unpop.test.mjs`·`squid-space.js`·`Squid.cpp`)를 추가했다. 원본/CD 실행, analyzeManager, 디컴파일은 하지 않았다.

## 4. WB04-D 하위 단위 계획 (2026-11-05 기준, cpppj 진행에 따라 조정)

| 하위 | 범위 | 원본 관측 후보 |
| --- | --- | --- |
| [x] D1 | 공통 공간 해제 부모·spot genus·건물 옆면 해제 | `squid-unpop-parent-hardware-x86.tsv`, `hash-x86.tsv` Genus |
| [ ] D2 | `SquidUnpop` 처리기(표시 갱신 vtable 지원표·미니맵 dirty 큐·contained 해제 종단)와 공통 삭제기 `RawSquidDestroy` | `unpop-x86.tsv`·`unpop-vtables.tsv`, `destroy-x86.tsv` 등 |
| [ ] D3 | Form 부착·생성자(`RawFormAttach`·`SquidForms`·Depend/Process/Gump/Content Form 호스트) | `form-*-current-x86.tsv` |
| [ ] D4 | `SquidFactory` Create/Take와 `SquidProcessHost`·Regular(`AddRegular`/`FindEvent`) | `regular-current-*-x86.tsv` 등 |

D3/D4의 cpppj 소스는 지금 다른 세션이 바꾸는 중이다. 착수 시점의 커밋본을 기준으로 하고 이후 변경은 3.5절에서 반영한다.
