# webpj 객체 trace 재귀(RawObjectTrace) 이식 — WB04-E236

2026-11-10, HJOW-Athlon. 개발자 덤프의 진입점(0044dab0)을 옮겼다. 객체의 현재 가상 +0x70 진단을 부르고 모든 Form 자식을 재귀하며, 재귀 깊이와 로그 들여쓰기 전역을 맞추고, 끝에 `END <번호>` 줄을 기록한다. E230(`RawObjectTraceFields`)·E235(`RawObjectTraceMain`)가 만든 객체별 서식을 이 재귀가 실제 같은 풀 위에서 한 로그 장치로 연결한다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E235 에 이어 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawObjectTrace.cpp/.h`)·시험(`ObjectTraceTests`)·fixture(`object-trace-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스·fixture `e5efcf6f`(2026-10-29).
- 원본 fixture: `object-trace-x86.tsv` 103행(10.78 전용). 입력은 시작 번호·논리 끝 번호·재귀 깊이·들여쓰기·checking·메타맵·boss·로그 직후 변경 번호·노드 목록(`번호,TYPE,상태,head,next`)이고 출력은 가상 호출(`V:번호:깊이:들여쓰기`)·들여쓰기(`I:`)·로그(`L:레벨:서식 hex:깊이:들여쓰기`)·dead 부모 assert(`A:`)의 순서와 인자, 최종 깊이/들여쓰기다.
- 공유 장치: E235 `raw-object-trace-main.js`(`traceRawObjectVirtual`, 계약 시험의 실제 분배), `sid-pool.js`.

## 1. 옮긴 것

`www/core/raw-object-trace.js`(DOM 없음, 10.78 전용)

- `traceRawObject(pool, globals, hooks, sid)`: sid 가 0 이면 판본/장치 검사 없이 반환. 슬롯이 free 면 `sid <번호> is free` 한 줄만 기록. 아니면 현재 가상 +0x70(`hooks.trace`) → 깊이 +3(DWORD 감김)·들여쓰기 설정 → 진단 checking 이 켜져 있고 메타맵이 아니며 슬롯이 dead 이고 boss 가 아니면 dead 부모 assert 보고 → 부모 head 부터 next 고리를 따라 TYPE 0..69 후보만(70 이상은 건너뜀) 논리 끝 번호 미만이면 자식으로 재귀(끝 번호 이상을 만나면 순회 종료, free 자식도 개수에 포함) → 깊이 −3·들여쓰기 복구 → 자식이 있었으면 `~1~yEND <4자리>` 기록. next 는 외부 효과 전에 저장하고 가상 호출·로그가 바꾼 뒤의 head/깊이/count/checking 은 소비 위치마다 다시 읽는다.
- `globals = { depth: { value }, count: () => n, checkingDead: () => n, metamap: () => n, boss: () => boolean }`, `hooks = { trace, indent, log, deadParent }`. 선택된 장치 누락은 TypeError, 물리 범위 밖 SID 는 RangeError 로 거부하며 앞선 출력/깊이 변경은 되돌리지 않는다.
- 원문은 재귀/종속 고리에서 끝없이 실행한다. 호스트만 현재 경로의 부모 재방문과 형제 고리를 명시 오류로 거부하며, 다른 부모가 같은 정상 후보를 다시 참조하는 것은 막지 않는다.

## 2. 검증

`tests/object-trace.test.mjs`(3건)

- 원본 관측 재생: 103행 전부(변이 10종: 첫 가상 호출 뒤 head 교체·형제 next 교체·깊이 교체·free 로 바뀜·TYPE 70 으로 바뀜·끝 번호 변경, 두 번째 호출 뒤 next 교체, 로그 뒤 깊이 +2).
- cpppj 계약 포팅 1건 + 웹 보강: 미연결 `indent` 를 거부해도 앞선 가상 호출/깊이 유지, SID 0 은 장치 없이 반환, 부모 재방문(고리)·범위 밖 head 의 거부와 그때까지의 깊이/호출 수. 보강: 형제 고리, free 루트(한 줄만 기록), 다른 판본 풀 껍데기(SID 0 이면 반환·아니면 거부), 끝 번호 이상 후보에서 순회 종료, 다른 부모의 같은 후보 재참조 허용.
- 실제 연결 1건(cpppj `ObjectTraceMain_RecursiveVirtualDispatchSharesCurrentPool` 이식): 같은 풀에서 재귀 → E235 `traceRawObjectVirtual` 의 현재 가상 분배(자산 Main, Process, Content) → 공통/파생 로그 7건과 `END 0010`, 깊이·들여쓰기 0.

최초 실패와 교정: 모듈은 처음부터 103행이 모두 통과했다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 처음 변형 31건 중 7건이 살아남아 5건은 시험을 보강해 잡았다(깊이 상자가 값을 감지 않게 해 모듈의 DWORD 감김을 검증, 끝 번호 순회 종료, 같은 후보 재참조 허용, SID 0 의 판본 검사 생략, 다른 판본 풀 거부).

- 실패한 변형: TYPE 상한·깊이 증분·로그 레벨·필드 위치(next/head)·free/dead 비트, free 문구와 반환, 고리 검사(부모/형제), 깊이 감김, 들여쓰기 설정, checking/메타맵/dead/boss 조건, head 읽기, next 저장 시점, 건너뜀과 종료, 끝 번호 비교, 자식 수, 깊이 복구, END 문구/조건, 방문 집합 해제, 0 번 진입, 판본 검사(각 1건 모두 실패).
- 적용되지만 통과한 변형(동등 변형): ① 선택 장치 존재 검사 제거 — 함수가 아니면 호출 자체가 TypeError 다. ② 모듈의 슬롯 범위 검사 제거 — `SidPool.slot` 이 같은 RangeError 를 던진다.

## 3. 경계와 남은 것

- 개발자 덤프 명령과 로그 장치·들여쓰기 setter 의 실제 연결은 순서4 몫이다(PORT_MAP session 0). 이 모듈은 가상 +0x70 호출과 재귀/깊이/END 규칙만 소유한다.

## 4. 다음 시작점

WB04-E237 은 3.1절에서 고른다. 기준선 뒤 변경이 없는 단위(`RawTerrainGrowth`·`RawTotalMade`·`RawTutorialOne`)를 권한다.
