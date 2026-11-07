# webpj Form 분배기·Content 삭제 수명·보상 경제 — WB04-D5b

2026-11-07, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3(WB04-D5b~E)의 첫 단위다. D5a의 Form 호스트 위에 다음 네 모듈을 옮겼다.

- **Form 분배기** `SquidFormDispatch`: 여러 Form 클래스가 섞인 종속 체인의 중첩 삭제를 현재 VT로 분배한다.
- **Content 삭제 수명** `ContentFormLifetime`: 커서 취소와 너겟 수입 경제를 처리한다.
- **보상 경제** `SquidReward`: Content 수명이 의존하는 보상 계산이다.
- **분산 SP 저장소** `ScrambledSpStore`: 보상이 쓰는 패치판 SP 저장 장치다.

화면 변경과 앱 수명 연결은 없다.

## 1. 근거

- 작업 시작 HEAD는 `6b700418`이고, 작업 중 다른 세션의 CP 커밋으로 `11fbb988`까지 진행했다. 이식 원본은 모두 **기준선 `1f00a25a`** 판(`git show 1f00a25a:…`)이다. 같은 작업 트리에 있던 다른 세션의 미커밋 변경은 근거로 쓰지 않았다.
- 근거 소스(cpppj, 기준선 뒤 변경 없음 — `port-map.mjs` 보고 0):
  - `o/SquidFormDispatch.cpp/.h`
  - `o/ContentFormLifetime.cpp/.h`
  - `o/SquidReward.cpp/.h`
  - `o/SpStore.cpp/.h`의 `ScrambledSpStore`(`GameRandom`은 WB04-C1에서 `random.js`로 옮김)
  - `o/RawRiftHitPoints.cpp`(D4에서 옮긴 TYPE HP를 보상이 공유한다)
- 근거 검사(cpppj, 기준선 판):
  - `RewardTests`·`RewardSpBitsTests`·`RewardHitPointDiagnosticsTests`
  - `RewardTypeNumberTests` 1~6
  - `RewardCurrentTypesTests`의 보상 부분
  - `ContentLifetimeTests`
  - `FormTakeTests`의 `FormClassLifetime_CurrentVtableKeepsGumpAndContentEffects`
  - `DependCreateCurrentTests`의 `DependCreateCurrent_UnregisteredClassDoesNotUseDifferentRawTypeRoute`
- 원본 관측(`cpppj/tests/fixtures`, 복사하지 않고 직접 읽음, 10.78 행만). 해시는 manifest의 LF 정규화 SHA-256 앞 12자리다.

| manifest id | 파일 | 사용 행 | SHA-256 | fixture 커밋 |
| --- | --- | --- | --- | --- |
| `reward` | `reward-x86.tsv` | originals 2400행 | `a1fedbc77eca` | `40cf5167` |
| `sp-add-bits` | `sp-add-bits-x86.tsv` | 504행(10.78 전용) | `4b3e88cafe53` | `58839028` |
| `rifttype-hit-points` | `rifttype-hit-points-x86.tsv` | originals 579행(초기 번호 음수 행 제외, cpppj와 같은 선택) | `58197e3d412b` | `804b87a5` |
| `content-lifetime` | `contentlifetime-x86.tsv` | originals 198행 | `5e701e7b56e3` | `6c9cfce9` |
| `content-cursor-width` | `content-cursor-width-x86.tsv` | originals 28행 × 2경로 | `1dbbcddbf61e` | `46e65408` |

## 2. 파일과 계약

### `www/core/sp-store.js` — `ScrambledSpStore`(0040eba0/0040ec50/0040edb0/00491680)

- 소유자 0~8의 SP 32비트를 32개 배열(각 9 DWORD)에 흩어 둔다. 값의 k번째 비트는 배열 `permutation[k]`의 소유자 DWORD에서 `(offset+k)&31`번째 비트다.
- 쓰기 때마다 같은 DWORD의 다른 비트에 게임 난수 잡음 5개를 OR한다.
- 초기화 순서(게임 난수 소비 순서 보존):
  1. 재시드
  2. offset(난수 100)
  3. 항등 순열과 추적 offset 쓰기
  4. 32번 교환(난수 32)
  5. 다시 항등 순열
  6. 32개 배열 확보(채움값 또는 할당 입력 함수)
- 회전: offset을 signed DWORD로 3 늘린다(9600 초과면 한 번 뺀다). 이어서 모든 DWORD를 ROL3한다.
- 사제 추적 offset(0054d4e0)은 `{ value }` 소유자로 **첫 초기화 전에만** 연결한다.
- 게임 난수는 같은 `GameRandom`을 빌린다.

### `www/core/squid-reward.js` — `SquidReward`·`SquidRewardState`(0044c2f0 외, 10.78)

- **비용 곡선:** 로딩 비용표(`ftol(cost+번호*23)−번호*23`)의 정수 단가를 float로 바꿔 (개수+1)배한다.
  - 현재 제단/다이스 번호면 개수를 하나 줄인다.
  - 별칭(성장형 레인 바리케이드)은 첫 가상 개수 뒤의 **현재 슬롯 TYPE**으로 판정한다. 별칭이면 두 번째 가상 개수 뒤의 현재 레인 바리케이드 비용을 쓴다.
- **삭제 보상:** `(비율×비용)`의 DWORD 곱을 100으로 나눈다(0방향 절삭).
- **해체 환불:** 너겟 외에는 1/4 배율을 곱하고 남은 HP에 비례한다.
  - 절반 HP 모드면 HP 절반 미만은 0이다.
  - 최대 HP 0이면 0이다.
  - PATCH는 환불 TYPE을 **가상 개수 전에** 저장한다.
- **지급:** 패치판은 로컬/다른 소유자 모두 SP 가산을 쓴다. 그 뒤 현재 샘 풀에 `비용−지급`을 더한다.
- **SP 가산 `addSpBits`:** SP 읽기 → (고정 모드면 delta = 194162 − SP) → float 합 저장 → delta 절삭 DWORD를 AI 지갑 → SP 표시 갱신 순서다.
  - **NaN·무한대는 비트 단위로 처리한다.** 첫 NaN 피연산자의 quiet 비트를 전파하고(`7fa12345→7fe12345`), 무한대 상쇄의 결과는 실수 부정값 `ffc00000`이다.
  - `addSp(수)`는 JS NaN 페이로드가 없으므로 `7fc00000`으로 본다.
- **AI 지갑:** `bindPatchWallet`이 있으면 그 처리기(실제 brain+2d8)를 쓴다. 없으면 AI가 부착된 소유자 1~8의 독립 지갑에 더한다(DWORD 감김, 하한 0x4a6).
- **최대 HP:** D4의 `readRiftHitPoints`를 같은 레코드와 그때 현재 표 기준으로 쓴다.
  - 다이스 부류는 선택한 제단 TYPE 기록을 끝까지 유지한다.
  - 진단 장치는 호출 순간 선택한 것을 끝까지 쓴다.
- **`SquidRewardState`:** 독립 값형 필드를 가진다. `share(name, owner, key)`나 `bindGameAux(aux)`로 빌린 칸은 **매 소비 위치에서** 소유자 값을 읽는다.
  - 빌릴 수 있는 칸: 비율·자원·사제 1/4·특별 TYPE 6종. 절반 HP 모드는 World 칸이라 따로 share한다.
  - 장부는 `SquidPostPopState`와 같은 객체로 받고, 이 모듈은 `localOwner`와 `aiAttached[0..8]`만 읽는다.

### `www/core/content-form-lifetime.js` — `ContentFormLifetime`·`ContentLifetimeState`(004aeeb0)

- **preDestroy 순서:**
  1. 현재 커서 **DWORD 전체**가 이 Form이거나 부모 WORD가 자기 번호면 커서를 취소한다.
  2. kind(+18)가 현재 너겟 TYPE이고 부모가 dead가 아니면 수입 알림을 낸다. 로컬 소유자면 로컬 알림(명령 005429c4), 다른 소유자면 boss일 때 원격 패킷이다.
  3. 선지급 억제(+22)가 0이면 AI 지갑 선가산(비용 절삭)을 한다.
  4. 현재 owner·비용·localOwner를 다시 읽어 SP를 가산한다.
  5. 부모 중심(발자국 절반, float 저장) 위치 소리를 낸다.
  6. 공통 pre 깊이를 반납한다.
- 효과 뒤에는 커서·boss·너겟 번호·비용·부모 owner·슬롯을 소비 위치에서 다시 읽는다.
- **기타 효과:** postDestroy는 깊이를 반납한다. 부모 release 통지는 빈 가상 함수다. 종속 삭제는 같은 공통 삭제기로 재진입한다.
- **클래스 판정:** 사용 중/dead 슬롯은 현재 Content VT로, 반납된 슬롯은 TYPE 기록(6)으로 판정한다.
- `reward`를 주면 지갑/SP는 그 실제 저장소를 쓴다. 다른 Form/자산은 `fallback`으로 넘긴다.

### `www/core/squid-form-dispatch.js` — `SquidFormDispatch`

- **루트 삭제:** TYPE이 자산(70 이상)이고 자산 가상 삭제(예: 다리 재정의)가 연결돼 있으면 그쪽으로 보낸다. 그 밖은 공통 몸체다.
- **공유 훅의 분배:**

  | 효과 | 보내는 곳 |
  | --- | --- |
  | 전송·선택 해제 | 자산 경계 |
  | 종속 삭제 | 루트 재진입 |
  | Form의 pre/post/release·Unpop | **현재 VT**가 고른 클래스 호스트 |
  | 자산 효과 | 자산 경계 |

- TYPE BYTE를 클래스 대신 쓰지 않는다. 선택된 클래스 호스트가 없으면 다른 경로로 숨기지 않고 오류를 낸다. 이미 수행한 dead 표시와 pre 깊이는 보존된다.
- `bind`는 Depend·Gump·Content 호스트만 받는다. Process 호스트는 D5c에서 추가한다.

### D4 수정 — `www/core/squid-factory.js`의 `readRiftHitPoints`

- 사제 1/4 몫이 음수 0일 때(`-1/4`) JS `-0`을 돌려주던 결함을 `| 0`으로 고쳤다. TYPE HP 579행 대조에서 처음 실패한 1행으로 발견했다.

## 3. 검증

- **원본 관측 대조:**
  - 보상 2400행, SP 가산 비트 504행, TYPE HP 579행은 `tests/squid-reward.test.mjs`에서 대조한다.
  - Content 수명 198행, 커서 폭 28행×2경로는 `tests/content-form-lifetime.test.mjs`에서 대조한다.
  - 비교 대상은 효과 당시 전체 슬롯, 통지, 관측 슬롯 전체 바이트, 풀 메타, Adler, 공통 깊이, SP 저장소의 난수 상태·배열 Adler·offset, 지갑·샘 풀, 표시/가상 호출 수다.
- **최초 실패와 교정:**
  - TYPE HP 1행(`0,-1,…,quarter`)이 `-0`으로 실패했다. D4 `readRiftHitPoints`를 교정했다.
  - SP 가산의 NaN/무한대 비트는 관측 분석 단계에서 156행의 NaN 행을 확인했다. 그래서 처음부터 비트 단위로 구현했다.
- **계약 검사(cpppj 옮김):**
  - 저장소 왕복·거부·회전·추적 offset
  - 보상의 입력 거부, SP 효과 뒤 현재 비율·자원, 사제 1/4 빌림, 다이스 최대 HP의 선택 제단 유지, 진단 장치 누락/교체
  - 특별 TYPE 번호 기본값·빌림, 제단/다이스, 레인 바리케이드 별칭·너겟 전액, 두 번째 가상 개수 순서, 환불 TYPE 저장 순서, 현재 표/로딩 비용표
  - Content의 실제 SP/지갑 통합, boss·너겟 번호 재조회
  - 혼합 Form 분배(Process 제외), 현재 VT 분배(TYPE 별칭 3종), 미등록 클래스 오류, 연결 계약
- **웹 전용 계약 1개:** 별칭 판정이 첫 가상 개수 뒤 현재 슬롯 TYPE을 읽는지 확인한다. 음성 대조에서 이 위치를 잡는 검사가 없어 추가했다.
- **음성 대조 16종:** 한 곳씩 바꿔 실행했고(scratchpad 도구, 실행 뒤 원본 복원) 14종이 검출됐다.
  - 검출된 변이:
    - NaN 전파 제거, 고정 SP 모드 무시
    - 별칭 판정에 저장 TYPE 사용(웹 계약 검사 추가 뒤 검출), 환불 TYPE 저장 순서
    - 지갑 하한 제거, 샘 풀 누적 제거, 절반 HP 경계
    - SP 잡음 개수
    - 커서 하위 WORD만 비교, 지갑 뒤 owner 재조회 생략, 선지급 억제 무시, 원격 알림 boss 무시
    - 중심 footY/footX 교환
    - 분배를 TYPE BYTE로
  - 검출되지 않은 2종:
    - **실수 부정값 대신 JS NaN:** x64 V8의 기본 NaN이 우연히 `ffc00000`이어서 결과가 같다. ARM 등에서는 `7fc00000`이 되므로 명시 규칙을 유지한다.
    - **분배기의 종속 삭제를 클래스 호스트로 넘김:** 현재 세 호스트 조합에서는 결과가 같다. contained 자산은 공통 몸체가 루트로 받지 않으므로 자산 재정의 경로로 갈 수 있는 종속이 없다.
- **전체 검사:** Node v24.21.0, **463/463 통과**(WB02-H 437 + 이 단위 신규 26개). 기존 cpp-oracle 등을 포함한 전체이며, 실패·건너뜀은 0이다.
- 브라우저·원본/CD·cpppj 게임 실행, analyzeManager, 디컴파일은 하지 않았다. cpppj 단위 검사 EXE도 판정에 쓰지 않았다(4절).

## 4. 남은 경계

- **ProcessHost(D5c):** `SquidProcessHost`와 `bind(processes)`가 남아 있다. cpppj `ContentLifetime_SharedMixedFormDispatch`의 Process(자기 삭제 프로세스·Kernel) 부분도 이때 더한다.
- **삭제 장부(WB04-E):** `SquidDestroyLifecycle`과 보상을 잇는 cpppj `reward_lifecycle_*` 3개는 삭제 장부 이식 때 옮긴다. `SquidPostPopState` 전체도 같다.
- **사제(WB06):** `RewardCurrentTypes`의 사제 부분(`RawPriestState.GroundImmobile`)은 사제 이식 때 옮긴다.
- **일반 피해 후처리(WB09):** `RewardTypeNumber_GeneralPostSharesCurrentRainExclusionAndOverride`(`RawDamageablePostDestroy`)는 일반 피해 후처리 때 옮긴다.
- **세션 연결(WB05/WB06):** 다음 연결은 아직 없다.
  - SP 저장소 재시드 원천(004559d0 시각)
  - 사제 추적 offset 소유자(0054d4e0)
  - 실제 지갑(`bindPatchWallet`, brain+2d8)
  - 수입 알림/원격 패킷/위치 소리 장치
  - Client의 커서·boss·현재 너겟 번호 getter
  - `SquidRewardState.bindGameAux`와 World 절반 HP 모드
  - 같은 World TYPE 표·로딩 비용표 대여
- **cpppj 쪽 입력 차이(실행 확인 안 함, 정적 판단):** cpppj `CursorPreviewDword_ContentComparisonNative56KeepsAllBits`는 Form 슬롯 VT를 0으로 둔 채 `PreDestroy`를 부른다.
  - 기준선 `IsContentForm`은 `b08059ac`(2026-11-06)부터 현재 VT로 판정한다. 따라서 기준선에서 이 검사는 "Content preDestroy 슬롯 오류"로 실패할 것으로 보인다. 검사 추가(`46e65408`, 10-26)가 VT 판정 도입보다 앞서고, 도입 커밋은 이 검사를 고치지 않았다.
  - 빌드된 `cpppj/build/bin/Release/netstorm_file_tests.exe`는 `b08059ac` 이전(11-06 15:13) 빌드라 판정에 쓰지 않았다.
  - 웹은 실제 Content 생성자 VT를 넣어 같은 커서 관측을 재생한다. cpppj 쪽 수정 여부는 사용자/CP 작업 몫이다.
- **CD 10.72:** CD 판의 SP 전역 float·WORD HP·너겟 무효과 경로는 웹 범위 밖이다.

## 5. 3.6절 판정 — 이식분이 있는 행의 기준선/판정 뒤 cpppj 변경

단위 착수 때 `port-map.mjs`가 "②/③ 판정 필요"로 보고한 5행을 판정하고, base를 판정 커밋으로 올렸다. 모두 LAN 3차 작업과 개발자 기능의 공유 파일 변경이며 ②(지금 반영)는 없다.

| 행 | base | 판정 |
| --- | --- | --- |
| `client/ClientMain.cpp` | `44e53bdc` | ③ 메인 루프의 연결 감시(439ad8)·전투 자원 프레임(439b34)·재접속 주기 저장 단계, 기록 재생 전역·커서 기록기 식별자, 시간 보정 시계 바인딩은 WB05 세션/메인 루프 연결 때 단독 도달과 단계 순서를 재확인한다. 힙 진단 로그·Monitor AutoRestart는 ① |
| `client/UberGump.cpp` | `11fbb988` | ① LAN 대기실·재접속·Multiplayer 옵션/메뉴(웹 제외). ③ `ShowLoginProgress`의 단독 안내(Initializing./Starting Mission...)·`PollBattleResources`(486bc0)·F7/Ctrl+F6 건설 기록 재생 입력은 WB05 로그인·WB06 입력 이식 때 |
| `o/GameClock.cpp/.h` | `3e9dc2c7` | ③ 시간 동기화 보정(0055b4ac) 바인딩과 `(ms−origin−보정)` 감산. 보정 0이면 기준선과 같은 결과다. WB05에서 시간 패킷(5번)의 단독 도달을 확인할 때 반영한다 |
| `o/RawFormPacket.cpp/.h` | `b8cadde1` | ③ 삭제 패킷 writer의 헤더 선기록·client SID 진단 정상 복귀는 패킷 writer 이식 때 반영한다. 웹이 가진 VT 4종 칸은 ① |
| `o/RawGameAuxState.h` | `11fbb988` | ③ 새 장기 전역 칸(StateNotifyType 82·Spell/Income·프로세스 TYPE·건설 헤더·미션 설정 7칸·Quick 건설 등)은 각 소비자 이식 단위에서 보호 PE 초기값과 함께 추가한다. 기존 웹 칸의 값/폭은 바뀌지 않았다 |

## 6. 검사 수치와 다음 단계

- `squid-reward` 17개(약 3.2초), `content-form-lifetime` 9개(약 1.2초).
- PORT_MAP 상태 변화:
  - `SquidReward`·`ContentFormLifetime`: todo→port
  - `SpStore`: partial→port
  - `SquidFormDispatch`: todo→partial(Process 등록 남음)
  - 합계: port 44·partial 85·todo 767
- manifest는 69항목이다(새 원본 관측 5개).
- 다음은 **D5c**다: `SquidProcessHost`·Process/BaseProcess 부착·Regular 공통, Depend 생성의 Process 직렬화 부착 접두, 분배기 `bind(processes)`와 혼합 분배의 Process 부분.
- D5c를 시작할 때 `node webpj/tools/fixture-manifest.mjs --write`로 이 단위의 `worktree` 표시를 실제 커밋으로 갱신한다. 대상은 새 5항목과, `squid-factory.js` 수정으로 바뀐 D4 항목들이다.
