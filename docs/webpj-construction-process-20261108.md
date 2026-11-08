# webpj 건설 프로세스(RawConstructionProcess) 이식 — WB04-E132

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백서른두 번째 부분이다. 건설 프로세스(원본 0x38바이트 객체, 프로세스 타입 30) — 건설 시작·진행·완료·form 삭제 통지 — 를 옮긴 `o/RawConstructionProcess`(cpppj 기준선 `1f00a25a`)이다. 선행은 기존 웹 모듈의 `SquidProcess`/`SquidProcessHost`/`Kernel`(프로세스 호스트), `SquidReward`(최대 HP), `setRawSquidHitPoints`, `raw-form-linked.js`(Form 연결 진단)이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(HP 16비트 폭, 진행음 조건의 비트 비교, `ResetEnd` 의 진행 HP 계산)는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-construction-process.js`

- `createConstructionProcessFields()` — `{ dirty(0xff), start, end, sound, additional }`.
- `ConstructionProcessState` — 경계 호출 뒤에도 다시 읽는 전역: `now`·`processType`(30)·`soundProcessType`(47)·`localPlayer`·`boss`·`pushGenus`·`daisType`(165)·`quickBuild`/`quickVortex`/`checking`(현재 DWORD 비영 뷰: 숫자·bool 또는 현재 값을 돌려주는 함수; 사용 위치마다 읽음)·`sharedBoss`/`currentProcessType`(실제 Client 값을 빌리는 함수).
- `RawConstructionProcess(pool, types, state, hooks, host)` — 필수 경계 15개(`reserve`·`effect`·`loadSound`·`reserveSound`·`attachSound`·`refresh`·`maxHitPoints`·`updateDisplay`·`removeContained`·`playAt`·`readySound`·`play`·`unpop`·`repop`·`push`, `attach` 는 host 가 없을 때만)와 타입 188칸·host 풀 일치를 생성 때 확인한다. `types` 는 사본이 아니라 현재 표를 읽는다.
  - `start(parent, duration)` (00443fe0→00443c40): 확보 실패면 효과 없음 → 표시 효과(지속 0/−0 이면 즉시 건설 `I`, 아니면 `E`) → 시작=효과 뒤 현재 시각, 끝=지속+현재 시각(지속 0 은 10초) → 부착(host 가 있으면 실제 `ConstructionProcess` 를 flags 0x10 으로 붙이고 부착된 프로세스의 필드를 쓴다. free/dead 부모에는 붙지 않고 남은 쓰기를 계속) → 지속 시간이 1초보다 크면(실수 비교, NaN 은 소리 없음) `jimBuild.wav` 조회·소리 프로세스 확보(경계가 현재 소리 필드를 바꿀 수 있음)·부착 → 갱신 요청.
  - `run(fields, parent, kill, currentForm, currentParent)` (004428c0): 선택된 `checking`(비영)에서는 현재 Form 조회→연결 진단(`formRobust`)→거짓이면 원문 진단 `getForm().isRobust()`/`\Ns\O\Construction.cpp`/0x1c4(복귀하면 `checking` 을 다시 검사하지 않고 계속) 뒤, 현재 부착 부모 조회 → 최대 HP 경계 → `ftol((현재−시작)/(끝−시작)×최대)`를 0~최대로 제한해 부모 HP(DWORD)에 쓰고 표시 갱신 → 표시 경계가 뒤집었을 수 있는 abstract 비트를 다시 읽어 abstract 가 아니면 `kill(0)`(프로세스가 사라졌으면 종료) → HP 가 최대이고 권한(빌린 boss 의 현재 값)이 있으면 `kill(0)`. `quickBuild` 이거나(`quickVortex` 이고 부모 genus 가 vortex 0x200) 바로 최대이다.
  - `complete(parent)` (00443e20): 최대 HP 와 HP 를 처음 읽은 값으로 끝까지 쓴다 → contained 삭제(진행음 프로세스) → 갱신 → 로컬 소유이고 예측 번호가 아니면 `buildDone.wav`·타입 준비 소리 → (권한·확정 번호·HP 가 최대·abstract) Unpop → void 가 아니면 원본 assert 로 거부 → abstract 비트만 끄고 재등록(flags 9) → genus 가 `pushGenus` 와 맞으면 유닛 밀어내기. 각 경계 뒤 부모의 소유자·좌표·타입·전역을 다시 읽는다.
  - `formDestroyed(parent)` (00444030): `complete` 뒤 부모가 free/dead 가 아니고 dais 타입이 아니면 HP 를 최대로 채운다.
  - `resetEnd`/`resetEndFromForm` (00443dc0): 최대 HP 경계 뒤 끝 시각을 현재+10초로 바꾸고 변경 비트 4 만 켠다. 후자는 host 의 form→Kernel 조회로 건설 프로세스를 찾고, 빈 슬롯·다른 프로세스·오래된 form 은 필드 쓰기 전에 거부한다.
- `ConstructionProcess` — Kernel 이 소유하는 실제 프로세스. `runFrame` 이 `run` 을 호출하고(Kill 뒤에는 미리 읽어 둔 host·번호·주소로만 등록 여부를 묻는다. 원본이 해제된 객체로 둘째 Kill 을 부르는 동작은 옮기지 않음), `onFormDestroy` 가 `formDestroyed` 를 호출한다.
- `makeConstructionProcessHooks(pool, reward, effects, hooks)` — 최대 HP 를 같은 풀의 `SquidReward` 로, 유닛 밀어내기를 `effects.displace` 로 잇는다(풀 불일치 거부, 밀어내기의 반환값은 원본처럼 버림). `makeConstructionProcessNoticeHooks(pool, process, hooks)` — 건설 배치 통지의 시작(`start(sid, 지속 단정도 비트)`)을 `process.start` 에 잇는다.

## 2. 검증

`tests/construction-process.test.mjs`(13건):

1. `constructionprocess-x86.tsv` 의 `originals` 956행(`originalCD`·`original1037` 은 다른 판본이라 제외) — 부모 raw·전역·64비트 시각·확보 결과·경계 변동 17가지(`Mutate` 0~16: 효과 뒤 시각/타입 변경, 부착 뒤 시각 변경, 소리 필드 변경, 갱신 뒤 로컬 변경, 최대 HP 경계의 시각/HP 변경·둘째 호출 반환 변경, 표시 경계의 abstract 뒤집기, 권한 뒤집기, contained 삭제 뒤 소유자 변경, Unpop 의 abstract 바이트 변경, 재등록 뒤 좌표/genus 변경, 완료음 뒤 준비 소리 변경)에서 사건 순서와 인자·프로세스 필드(dirty·시작·끝·소리)·부모 슬롯 50바이트 전체가 같다.
2. 계약: 필수 경계 누락·`attach` 없음(host 가 있으면 허용)·타입 표 크기·다른 풀의 host 거부.
3. 확보 실패의 효과(객체 확보 실패는 `A:56` 만, 소리 프로세스 확보 실패는 소리 항목을 남기고 갱신까지 진행), 지속 시간 0/−0·1초·NaN 의 기본 시간/진행음, Unpop 이 void 를 켜지 않으면 abstract 를 풀지 않고 거부, 통지 어댑터(단정도 비트 0 → 기본 10초 건설, 다른 풀 거부).
4. 실제 host/Kernel/삭제기/보상 모듈(타입 100 은 HP 300 factory)에서 `process.start` → 프레임마다 HP 75 → 시각 10 에 `D;X:47;R;W:buildDone.wav;G:85;U;Q:9;B:100:1` 순서로 완료·프로세스 제거·HP 300·abstract 해제·form 해제, abstract 가 아닌 부모의 둘째 Kill 생략, 권한 없음은 프로세스 유지·`quickBuild` 는 한 프레임에 종료, 죽은 부모에는 붙지 않음, 다른 풀 보상 거부, 빌린 boss 를 진행과 완료 각 시점에 다시 읽음.
5. `construction-form-parent-x86.tsv` 64행 — 현재 `checking`/Form 조회·연결 진단·진단 복귀 접두(결과·checking·current Form·사건 문자열[진단 인자 바이트 포함]·부모 조회 횟수)가 같다.
6. 실제 Form/부모 체인에서 분리된 체인은 진단 뒤 복구되어 기존 HP 진행만 이어진다(연결 검사 2회·진단 1회).

최초 실패와 교정: 새 모듈은 956행 + 64행 fixture 와 실제 host 통합 시험을 첫 실행에 통과했다. 시험 쪽 입력 열 수 확인식만 한 번 고쳤다.

## 3. 경계와 남은 것

- `makeConstructionProcessHooks` 의 `effects`(유닛 밀어내기 `RawConstructionEffects.displace`)는 해당 모듈 이식 뒤 연결 시험한다(여기서는 `null` 로 둔 경로와 풀 불일치 거부만 확인).
- 건설 계열의 나머지(`RawConstructionArrival`·`Notice`·`Effects`·`Local`·`Confirm`·`Request`·`Clear`·`Data`·`Builder`·`Place`·`Command`·`Packet`·`Selection`)는 이 프로세스의 `start` 를 어댑터로 이어 쓴다.

## 4. 다음 시작점

`RawConstructionArrival`(운반 도착 판정, 이제 선행 `start` 있음)·`RawConstructionNotice`·`RawConstructionEffects` 등 건설 계열, Missile Host 팩토리(`RawGunProcess` 이후), `RawGunImpact`(SquidFrame/Host 장면 필요), `RawWindAviaryProcess`·`RawWindFlyer`, 비행체, 번개 수명/프레임/화면, `RawPathProcess` 계열(커널/Form 장면 선행), `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
