# webpj State 정리 소형 몸체 다섯 개 이식 — WB04-E11

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 열한 번째 부분이다. cpppj 기준선 `1f00a25a`의 작은 `o/` 단위 다섯 개(모두 10.78)를 한 번에 옮겼다. 각 몸체가 10~20줄이고 독립 원본 관측이 있으며 서로 의존하지 않는다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

| 모듈 | 원문 | 내용 |
| --- | --- | --- |
| `www/core/raw-clock-sync-reset.js` | 004c1f20 | `resetClockSynchronization({connection, replyMode, gameTicks})`: 세 DWORD를 이 순서로 0으로 쓴다(005c8bf4·005c8c00·0055b4ac). 인접 송신·수신 카운터와 큰 명령 큐·시계 배열은 건드리지 않고 값을 읽지도 복제하지도 않는다. |
| `www/core/raw-state-zone-reset.js` | 004bfd60 | `resetStateZone(zone)`: DWORD(+0)→WORD(+4)만 0. 4 BYTE 미만이면 첫 store에서, 6 BYTE 미만이면 둘째 store에서 `RangeError`이고 완료한 prefix는 보존한다. +6/+7은 읽거나 쓰지 않는다. |
| `www/core/raw-state-bridge-reset.js` | 0043b540, 00422490 | `resetStateOwnerColors(colors)`: 앞 9 DWORD만 0..8(열째 칸 보존, 9칸 미만이면 쓰기 전 거부). `resetStateBridgeDecayScan(scan, period, now)`: 기한=`period(float)+now(double)`을 먼저 계산(now를 float로 줄이지 않음)한 뒤 first=15000·last=23001·cursor=15000·next 순으로 쓴다. |
| `www/core/raw-ritual-refresh.js` | 449d98~449dc5 | `refreshLocalRitualData(hooks)`: core → templateOwner → coreData(owner) → templateOwner(다시 읽기) → snapshot. 장치 누락은 그 호출 위치에서만 거부하고 앞선 효과는 되돌리지 않는다. |
| `www/core/raw-bulwark-hit.js` | 48ef20 | `rawBulwarkHit(pool, types, victim, amount, owner, attacker, general)`: 공격자가 0이면 일반 피해(44c8e0)로, 공격자 슬롯의 TYPE `flags2 & 0x100000`(비행체 genus)이면 피해 없이 0. 공격자의 free/dead/extra/owner는 검사하지 않고 반환 DWORD를 좁히지 않는다. |

## 2. 검증 (`tests/state-resets.test.mjs`, 9건)

| 검사 | 근거 |
| --- | --- |
| 시간 동기화 64행(96 BYTE 두 영역 전체, 두 정밀도·반복) + 빌린 DWORD만 쓰고 이웃 보존 | `clock-sync-reset-x86.tsv` |
| Zone 10행 + 길이 0..8 경계(정확히 6 BYTE, prefix·canary) | `state-zone-reset-x86.tsv` |
| 다리 64행(소유자 색 9칸·붕괴 스캔 212 BYTE 네 영역과 store 주소/폭) + 색 표 물리 경계·열째 칸 보존 | `state-bridge-reset-x86.tsv` |
| 의식 갱신 54행(두 현재 owner·세 자식·각 중단 부분 효과) + 장치 누락이 앞선 효과를 남김 | `ritual-refresh-x86.tsv` |
| 불워크 492행(효과·두 슬롯 전체·반환 DWORD) + 장치 누락·표 크기 계약 | `bulwark-hit-x86.tsv` |

최초 실패와 교정: 구현 직후 9건이 처음부터 통과했다. 기대값은 원문 열만 썼다.

## 3. 경계

- cpppj의 `ClientStateStorageCleanup`/`RawSurfaceWorld` 통합 검사(실제 Client 물리에서 색 표·스캔·Zone 초기화, 대여/반납 순서)는 해당 Client/World 조립 단위에서 대조한다.
- `RawSquidPoolReset`(월드 Pool 정리)은 `SidPool.reset`과 삭제 기록 지우기 위에 얹는 부모라 전용 관측(`squid-pool-reset-x86.tsv`)을 쓰는 별도 단위로 남겼다.
- CD 10.72 경로는 범위 밖이다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`(Graph 소속 관리; 공통 PostPop의 선행), `SquidPostPop` 공통 본체, `RawSquidPoolReset`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
