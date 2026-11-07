# webpj 일반 MakeAi·AI 설정(RawMissionAiSettings) 이식 — WB04-E32

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 서른두 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawMakeAi`·`o/RawMissionAiSettings`(10.78)를 옮겼다. [E21](webpj-script-command-inputs-20261108.md)의 `dispatchMakeAiCommandInput`이 부르는 실제 부모(대본 MakeAi)이며, 미션 시작 override 흐름과는 구별한다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-mission-ai-settings.js` — `RawMissionAiSettings`, `configListItem`

- `initialize(storage)` [0040ef80]: 실제 기본값이 쓰는 바이트만 바꾸고 나머지 caller 바이트는 보존(전체 지우기/공백 보충 없음). `+0`=0, `+0x1e`=기본 기술 `sunWalker;windBattery;sunCannon;sunBalloon;sunBlocker;sunArcher;windArcher`, `+0x220`=6.0f, `+0x224`=float(stuffRefreshRate), `+0x228`=3.0f, `+0x22c`=startingSp+4000(DWORD 감김), `+0x230/0x234`=3, `+0x238`=4, `+0x23c/0x240/0x244/0x24c/0x250/0x254`=0, `+0x248`=0xffff.
- `setAbility(storage, name, enabled)` [0040f030]: ASCII 대소문자 무시로 원본 13개 능력 이름의 비트(+0x248)를 켜고 끈다. 미등록 이름은 checking이면 assertion을 먼저, 그 뒤 항상 log를 부르고 **index 13 비트도 수정**한다.
- `apply(storage, owner)` [004c2d90]: 키 접두는 owner>0이면 `ai<owner>`, 아니면 `ai`. 값이 비면 이전 필드 유지. Name/Tech 문자열, 세 atof(`BridgeDrawRate`/`StuffRefreshRate`/`TimeBetweenMoves`, 비어 있지 않은 값에만 `parseFloat`), 일곱 atol(`StartMoney`·`GeyserAttachments`·`MaxInHand`·`Collectors`·`Enemy`·`NoTemple`·`Off`), `Color`(1..8만; none/미등록은 유지), `AllyList`(`;` 열 개, 양수는 shift 하위 5비트로 가산), `Ability`(열 개, 선두 `!` 한 글자만 끄기). `prepare(storage, owner)` [004c2d40]: 기본값 → 공통(-1) → owner를 같은 storage에 누적.

`www/core/raw-make-ai.js`

- `makeAi(owner, settings, reads, hooks)`: 전투 중이 아니면 설정/저장소에 접근하지 않고 끝. 600바이트에 `initialize`(40ef80 기본값만, 미션 override 없음) 뒤 **현재** missionPost가 0이면 `+0x220=0`·`+0x224=1.0f`·`+0x228=2.0f`·`+0x230=3`·`+0x234=6`. owner 0이면 follower=로컬 owner+1의 brain이 이미 있으면 끝, 아니면 `transferObjects(follower)` → `create(follower, settings)` → `clearObjects(0x2000)`. owner≠0이면 playerKnown을 읽고 `+0x1e`=0 후 알려졌으면 `stormPower(로컬)`을 `_ftol` 하위 DWORD로 `+0x22c`에 쓰고 `create(owner, settings)`.
- `transferMakeAiObjects(owner, hooks)`: 풀 한계와 로컬 첫 region을 진입 때 고정, 현재 소유자(로컬/중립)·비묻힘·flags 있는 객체 중 다른 섬이고 플레이어/다리 TYPE이 아닌 것을 로컬 소유였으면 두 카운터를 줄이고 `setOwner`. `flags&0x200000`이면 깊이 증가 → 한 번 더 `setOwner` → 감소(예외 때 감소를 합성하지 않음), `flags&0x200`이면 새 소유자의 첫 region을 Player 행(+0x78)에 쓴다. `clearMakeAiObjects(mask, hooks)`: 다음 후보를 flags 조회/가상 삭제보다 먼저 고르고 `flags&mask`이면 `destroy(sid, 0)`.

## 2. 검증 (`tests/make-ai.test.mjs`, 4건)

원본 10.78 관측(`cpppj/tests/fixtures`를 직접 읽음) 3362행:

- `make-ai-parent-x86.tsv` 770행: 원문 **600바이트 전체**(기본 설정·미션 post 0 덮어쓰기·SP 절삭)·`Init`/`Sp`/`Transfer`/`Create`/`Clear` 호출 순서·생성 인자 전체.
- `make-ai-objects-x86.tsv` 2592행: 현재 슬롯 50바이트·Player 첫 region·깊이·두 카운터(`Uint32Array` 감김)·삭제 mask와 `Flags`/`Region`/`Owner`/`Local`/`Built`/`Destroy` 호출 순서.

계약 검사: 전투가 아니면 비접근, 초기화 중단 뒤 앞선 쓰기(`settings[30]=77`) 보존, 삭제 순회의 선행 다음 후보 선택, 두 번째 `setOwner` 예외 뒤 깊이 증가 보존, AI 설정의 `apply`/`setAbility`/`prepare` 규칙(공통→owner 누적, 키 접두, 빈 값 유지, 가산 동맹, 색 이름, 느낌표 능력, 미등록 이름 진단 후 index 13, 목록 조각 읽기).

최초 실패와 교정: 원본 관측 3362행은 처음부터 통과했다. `apply`/`setAbility`용 계약 검사의 기술 문자열 슬라이스 길이(72 → 80)를 잘못 써서 한 번 실패해 고쳤다(모듈 변경 없음).

## 3. 경계

- `RawMissionAiSettings`는 `PORT_MAP`에서 `partial`이다. **원본 관측은 `initialize`만** 대조했고(위 770행의 설정 600바이트), `apply`/`setAbility`/`prepare`는 소스 계약으로 만든 검사이다. 미션 시작 override 관측(`MissionStartTests`)은 `MissionStart` 단위에서 대조한다.
- `atof`(`parseFloat`)·미션 키 조회(`get`)·AI 생성(417e50)·`4c27c0` 객체 이전의 Player/월드 카운터·가상 삭제는 훅으로만 주입했다. CD 10.72는 범위 밖이다.
- 일반 입력 분배(`MakeAiCommandInput`)와 이름표(`make-ai-names`)·입력(`make-ai-input`)·객체(`make-ai-objects`) 중 입력/이름 fixture는 E21과 대본 큐 단위에서 대조한다.

## 4. 다음 시작점

`CreateAtInput`·`DetonateInput`·`ProductionShortcuts`·`RitualChoiceInput`(미옮긴 하위 의존)과 `MissionStart`(AI 설정 override). 큰 묶음은 `RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체, `RawSquidPoolReset`, `RawBridgeConnect`, Player 나머지이다.
