# webpj 표면 소유자·SID 건전성 검사·정적 SID 목록·TYPE 요구 에너지 이식 — WB04-E26

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 스물여섯 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawRegionOwner`·`o/RawSidRobust`·`o/RawStateSidLists`·`o/RawTypeRequiredEnergy`(+ `client/AiTypeRequiredEnergy`의 getter 부분)(10.78)을 옮겼다. 모두 원본 PE 관측 fixture가 있고 외부 효과는 훅/getter로 주입하며 도달하지 않은 장치는 요구하지 않는다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

| 모듈 | 원문 | 내용 |
| --- | --- | --- |
| `www/core/raw-region-owner.js` | 0046f710/0046f770 | `RegionOwner(pool, surfaces, () => 섬 타입)`. `atCell(x,y)`는 256×256 밖·표면 0·현재 섬 타입이 아닌 슬롯이면 0, 아니면 소유자 BYTE(+34). 표면 번호가 풀 용량 이상이면 `RangeError`. `atPoint(x,y)`는 원본 순서대로 `+0.99999f → 64비트 절삭 하위 32비트 → float 저장 → 다시 절삭`(유한하지 않은 값은 0)으로 칸을 고른 뒤 정수 조회. 섬 타입은 getter라 매 조회 현재 값 |
| `www/core/raw-sid-robust.js` | 004ac040/004ac140 | `squidIsRobust(object, live, diagnostics, hooks)`: 진단 type을 범위 검사보다 먼저 쓰고 lastRobust를 0으로 쓴 뒤 하한(base+50)/상한(base+capacity×50)을 검사(밖이면 1), live면 가상 표 DWORD 0/`0xdddddddd`/`0xcdcdcdcd`(2)·상태 bit0(4)·bit1(8)·TYPE 0(0x10), live가 아니면 표 0(2)·bit0 없음(4). `isSidRobust(sid, live, …)`: signed 1..65534 밖이면 false, live 0이면 풀을 읽지 않고 true, checking이 켜져 있으면 실패 시 `assertion('resolve( sid ).isRobust()', 0x218)` 뒤 진단이 복귀하면 true. 주소 폭은 `original32`(DWORD 감김, 기본)와 `host`로 구별. SID 번호에 세대 비트를 더하지 않음 |
| `www/core/raw-state-sid-lists.js` | 00472830/00472880/004728a0, 0044fb20 | `registerStateSidList`(첫 빈 칸에만)·`unregisterStateSidList`(중복 포함 모든 일치 칸)·`resetRegisteredStateSidLists`(매 칸의 현재 포인터를 읽어 비영 헤더마다 개수 쓰기, 쓰기 장치는 도달했을 때만 요구). `RawStateSidLists`: 정적 목록 여덟 개(공장 500·인간형 30·아웃포스트 100·공급원 256·간헐천 100·사제 16·거주 구역 50·신전 20)와 아홉 헤더(다중선택 `0x5caf28` 포함) 등록. `resetCount(header, selection)`은 다중선택만 빌린 Client 필드 `selection.multipleCount`를 씀. `prepareGeneralWorld()`는 일반 fresh 의미만 같은 배열 주소에서 비움 |
| `www/core/raw-type-required-energy.js` | 0049c3b0, 0049b0d0 | `typeRequiredEnergy(type, definition)`: 'mana' 속성(ASCII 대소문자 무시)을 순서대로 모두 읽어 마지막 정상 문자열(비문자열 `TypeError`, 첫 NUL 뒤 버림, 9바이트 초과 `RangeError`), 비어 있고 group≠10이며 제외 플래그(`flags2&0x200000`)가 없으면 level 규칙으로 원소 글자표(`wrtsWRTS.?>`+NUL)에서 보충, 비ASCII 거부, ASCII 소문자. `bindAiTypeRequiredEnergy(definitions, currentTypes)`: 188 TYPE/116 정의 계약, 자산 70..185만 정의를 읽고 내부·빈 TYPE은 정의 없음, 매 조회 현재 TYPE |

## 2. 검증

`tests/region-sid-lists.test.mjs`(10건)와 `tests/type-required-energy.test.mjs`(3건). 원본 10.78 관측은 `cpppj/tests/fixtures`를 직접 읽는다.

- `region-owner-x86.tsv`: 두 PE 1632행 중 10.78(originals) 816행(정수 칸·float snap·CRT 절삭·TYPE/상태/소유자). originalCD 816행은 범위 밖.
- `sid-robust-parent-x86.tsv` 1012행: 두 부모의 모든 실패 비트·번호 경계·선택 장치 호출 순서와 진단 재진입·32비트 주소 감김 전부. 그 밖에 미선택 번호/live/checking의 풀 비요구, 실제 호스트 풀의 물리 재사용(세대 비트 없음), TYPE 읽기가 논리 범위보다 앞섬.
- `state-sid-lists-x86.tsv` 66행(등록·해제·개수 정리 각 22행): 표·헤더 전체 출력과 개수 쓰기 순서. 정적 용량/헤더 순서, 복제 등록·쓰기 중단 prefix·짧은 표 거부, 개수 정리의 데이터/용량/등록표 보존과 일반 fresh 정책.
- `type-required-energy-x86.tsv` 2352행(음수/98 level·문자표 NUL·제외/group10·명시 문자열) 전부. 순서 있는 대입, 비문자열/9바이트 초과/비ASCII/표 밖 theme 거부, AI getter의 188/116 계약과 현재 TYPE 재조회.

최초 실패와 교정: 원본 관측 fixture 재생은 모두 처음부터 통과했다. 손으로 쓴 level 2 기대값(`ws`)만 틀려서 고쳤다. 원문은 level≥0에서 글자를 한 번 더하고 level 2에서 글자+`s`를 또 더하므로 `wws`가 맞다(실제 PE 2352행이 같은 결과). 테스트 기대값만 고쳤고 모듈은 그대로이다.

## 3. 경계

- `StateSidLists`의 실제 `Client` 정리(`ResetClientStateWorldStorage`)·`RawSurfaceStorage` 대여·사제 접두(`RawPriestPostPop`)·다중선택 소유 Client 필드 연결(`ClientGameplayGlobals`)은 해당 단위를 옮길 때 대조한다. 여기서는 `resetCount(header, selection)`이 `selection.multipleCount`를 쓰는 계약만 확인했다.
- `TypeRequiredEnergy`의 보호 자산 116개 정의로 188 TYPE 전체를 읽는 `Protected188Bindings`는 자산 적재 단위에서 대조한다(`AiTypeRequiredEnergy`는 getter 부분만 옮겨 `partial`).
- `RegionOwner`의 실제 Session/미션 adapter 연결은 해당 단위에서 한다. CD 10.72는 범위 밖이다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체(분배·장부·훅 연결), `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등). 작은 후보는 `RawEnergySupply`(`energy-supply-x86.tsv`)·`RawTotalMade`·`RawProjectileGeometry`·`RawMemStream`·`RawFileName` 등이다.
