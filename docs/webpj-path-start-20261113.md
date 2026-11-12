# webpj 이동 시작 RawPathStart와 이동 프로세스 선택 (WB04-E351, 2026-11-13, 6-P 경로 부품)

3.1절 6-P 셋째 항목 "RawPathStart 199줄(+2)"(기준선 `.h` 52 + `.cpp` 147 = 199줄, 기준선 뒤 `clock.Boss()` 반영 2줄)을 `core/raw-path-start.js` 로 옮겼다. 부모 객체가 "이 좌표로 가라"고 할 때 쓰는 **이동 시작 최상위 몸체**(패치 00489080)이며, E349~E350 에서 옮긴 경로 프로세스·검색·직렬화·정리를 한 번의 호출로 잇는다. 이 몸체의 `processType` 훅이 쓰는 작은 선택 함수(`RawPriestWorldBindings.cpp` 안의 `RawMovementProcessTypes`·`SelectMovementProcessType`, 원본 00488fe0)는 `core/raw-movement-process-types.js` 로 같이 옮겼다(사제 월드 바인딩 본체는 아직 todo → partial). 이로써 **6-P 세 항목이 모두 끝났다.**

## 1. 규칙

- **`RawPathStart.start(request)`:** `request = { parent, destination: { x, y }, replaceStart, helper, tailWord, tailByte, reserved }`. 순서는 다음과 같다.
  1. 부모 SID 가 0 이거나 풀 용량 이상이거나 목적지가 비유한이면 거부한다.
  2. 목적지를 `[0, 254]` 로 자른다. 부모 TYPE 으로 `hooks.processType` 이 프로세스 번호를 고른다. 부착 flags 는 Dude/마나 볼트/텔레포트 부모이면 `0x10`(client 번호 Form), 아니면 0(서버 번호 Form → 직렬화 송신)이다.
  3. flags 가 0 이고 `snapPriestDestination` 이고 부모가 사제이면 목적지를 올림 칸(float 0.9999 편향 뒤 절삭) − 0.5 로 바꾼다.
  4. 권한(`clock.boss`)이 있고 같은 종류의 기존 경로가 부모에 붙어 있으면(`lookup.find`) 그 두 대상(targetA/B)을 이어받고, 목적지 쪽이면 시작점을 반 칸 물린 뒤(`start ≤ 목적지` 인 축만) 기존 경로를 지운다.
  5. 프로세스 번호가 23/54/39/40/42/24/41/38 이 아니면 미복원 경계로 거부하고, 파생 종류(39/40/42/24/41/38)는 `hooks.createType` 이 있어야 한다. 생성한 프로세스가 같은 풀·같은 종류인지 확인한 뒤 `initializePath(parent, destination, start, 0)` → 대상 둘·완료 끔·명령 필드(helper·tail word·tail byte·목적지) 기록.
  6. 꼭짓점이 하나도 없으면 실패 정리: 부모 발자국(`startFootprint`) 주변의 **표면 단계를 제외한** 후보 중 genus `0x8c044200` 비트를 가진 것마다 현재 부모 위치/TYPE/소유자로 `hooks.clearBlocked` 를 부른다. 재귀 guard `state.retrying` 이 켜져 있으면 끄고 `null`, 아니면 하나라도 정리했을 때 guard 를 켜고 **한 번 재귀** 시작한 뒤 `null` 을 돌려준다(재귀가 성공해도 외부 호출은 `null`; 재귀가 성공하면 guard 는 켜진 채 남는다).
  7. `replaceStart` 이면 시작에서 연속으로 부모 올림 좌표와 **x, y 모두** 같은 꼭짓점의 좌표만 시작점(현재 부모 좌표)으로 바꾼다(시각/방향 보존).
  8. `metadata`(speed·turningSpeed)로 `prepare(0, …)` 하고 `RawPathProcess.attach` 한다(부모가 죽어 등록되지 않으면 오류). 마지막으로 부모 통지(`notifyStarted`)를 실행한다.
- **부모 통지:** 텔레포트/마나 볼트/Dude 부모는 빈 몸체(반환)다. 사제는 목적지 정수 칸의 finder(표면 단계 제외)에서 **마지막으로 만난** 간헐천을 targetA 로 기록하고(체인 순서 그대로, 매몰된 것은 건너뜀), 사건 0x25c·0x25d 를 각각 처음부터 다시 찾아 전부 지우고 `metadata.defaultFrame` 으로 프레임을 지정한다(`SquidFrame.set(parent, frame, 0)`). Walker/웜/Bulf/벌룬 프로세스는 간헐천 기록만 하고 반환하며, 그 밖의 부모는 미복원 경계로 거부한다.
- **`startFootprint(point, footX, footY)`:** 확장 입력(`point - (foot - 1)`)만 먼저 `[1, 255]` 로 자르고, 원래 점이 `0 < x, y < 256` 이 아니면(한 축이 0 이거나 256 이상이어도 점 전체가 무효) 확장 입력만 영역으로 쓴다. 왼쪽/위는 절삭, 오른쪽/아래는 올림 칸이다. 비유한 좌표·음수 발자국은 거부한다. 직접 시험하려고 순수 함수로 내보냈다.
- **`RawMovementProcessTypes`·`selectMovementProcessType(types, selection, type)`:** 현재 TYPE 전역(sunWalker 133·empty 187·windWalker 137·rainWalker 156·teleport 134·bulf 135·dude 70·priest 158·manaBolt 105)과 내장 프로세스 번호(worm 40·walker 38·balloon 42·teleport 23·bulf 41·priest 54·fly 39·dude 24)를 매번 현재 값으로 읽는다. 실제 Session 은 장기 소유자(`{ priest: { priestType }, roles: { templeProductType, emptyCarrierType, rainWalkerType, bulfType, dudePathType } }`)를 빌려 읽고 쓰며, 소유자가 없으면 독립 지역 값이다(`copy()` 는 독립 저장소). 선택: 타입 표 크기(188)·범위 확인 → sunWalker/empty → 웜 40, windWalker/rainWalker → Walker 38, genus 0x20000(벌룬) → 42, teleport → 23, bulf → 41, dude → dude 프로세스, priest → 54, manaBolt → 비행 39, 그 밖 → 웜 40.

## 2. 웹 범위와 기준선 뒤 변경

- 10.72(CD) 분기(실패 정리 생략·시작점 보정 없음·CD 의 worm 특수 반환이 표 조회 앞·미지원 타입 assert)는 옮기지 않았다(웹은 10.78 만).
- 기준선 뒤 변경(3.6절 ①): `clock.boss` → `clock.Boss()`(공유 권한 소유자)는 `SquidProcessState.boss` 로 옮겼다(PORT_MAP base `5318fdd6`). `RawMovementProcessTypes` 의 소유자 대여 확장(`RawGameAuxState` 역할 TYPE 칸)은 소유자 객체의 getter 로 옮겼다(PORT_MAP `RawPriestWorldBindings` 행은 base `4b6efc0e`, partial).
- 상태(`state`)는 `{ clock, dudeType, teleportType, manaBoltType, priestType, geyserType, snapPriestDestination, retrying }` 객체를 호출마다 현재 값으로 읽고 `retrying` 에 쓴다(복사하지 않음). 훅은 `{ ...hooks }` 사본으로 보관한다.
- 이 모듈은 DOM 을 쓰지 않는다. 앱 수명 연결(Session 이 `state`·`hooks` 를 실제 월드 전역에 잇고 부모 객체의 이동 명령이 `start` 를 부르는 일)은 4-D/6 단계 조립 몫이다.

## 3. 검사 (`tests/raw-path-start.test.mjs` 15건)

- **원본 독립 관측:** `pathprocesstype-x86.tsv` 의 `originals` 376행(모든 타입 번호와 벌룬 genus 교차의 선택 결과)과 `pathstart-x86.tsv` 의 `originals` 256행(부모 134/158 × 장면 profile 0~7 × 권한 × 기존 경로 × 시작점 교체 × 대기 사건 — 실제 BFS·Host·RawPathData 직렬화·SquidFrame 에 연결한 결과: 성공 여부, 생성 C·정리 K·Init Pop P·겹침 정리 B 의 공유 효과 순서, 명령 필드 7개, 꼭짓점 문자열, 부모 프레임, 남은 사건 0x25c/0x25d 개수, 재귀 guard)가 **첫 실행에서** 일치했다(장면 조립 입력을 고친 뒤).
- **cpppj 시험 이식:** `rawPathStart_native_process_type_priority`, `rawPathStart_native_creation_search_server_and_priest`. 사제는 서버 번호 Form 으로 한 번 송신하고 기본 프레임을 두 번 갱신하며 0x25c/0x25d 를 모두 지운다.
- **웹 계약:** 입력 검사(부모 SID 0/용량/비유한 목적지)·생성자 연결 검사·파생 종류의 `createType` 요구와 종류/풀/생성 실패 거부·미지원 종류 거부, 목적지 clamp(0.5/254.5 경계), Dude/마나 볼트/텔레포트의 client 번호 Form, 사제 스냅의 세 조건, 완료 표시/대상 초기화, 시작점 교체의 x/y 동시 일치, 실패 정리의 genus 비트(최상위 포함)와 표면 단계 제외, Walker/웜/Bulf 의 간헐천 기록(표면 단계 제외·마지막 후보)과 미지원 부모 거부, 부착 거부 오류, `startFootprint` 열한 경우, 훅 사본, 비유한 부모 좌표, 선택 함수의 우선순위·전역 대여·값 사본·입력 검사.
- 첫 실행의 실패는 모두 시험 구성 쪽이었다: 검색 시험이 쓰는 프레임 표는 평면 배열이고(프레임 코드 표 객체가 아님), 월드 객체에 풀 참조가 없었고, 마지막 간헐천은 체인 순서상 50 이다(머리 52 → 매몰된 51 → 50). 모듈은 바뀌지 않았다(`startFootprint` 만 순수 함수로 분리해 내보냄).

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 134회(Start 1차 75개 + 재확인 32개 + 4개, 선택 모듈 20개 + 3개)를 적용했다. 1차에서 Start 28개가 남았고 모두 **시험의 공백**이었다. 다음을 보강해 잡았다: 목적지 clamp(0.5·254.5·음수·300), 부모 용량 경계(32768), Dude/마나 볼트/텔레포트의 client 번호 Form, 사제 스냅의 세 조건(스냅 전역·flags·사제 TYPE), 시작 뒤 완료 표시/대상, 시작점 교체의 x/y 동시 일치, 실패 정리 genus 비트와 표면 단계 제외, 파생 종류의 간헐천 기록(표면 단계 제외), 부착 거부 오류, 훅 사본, 비유한 부모 좌표, 발자국 계산 열한 경우(확장 클램프 1/255·한 축 0·256 이상·발자국 x/y 혼동·올림 끝). 선택 모듈은 `< TYPE_COUNT` 대 `!==`(189칸 표)·사제 DWORD 저장·`copy()` 의 Dude 번호가 남아 보강했다.
- 남은 동등 변형 2개: ① 기존 경로 삭제의 `kill(old, 0)` 대 `0x10` — 서버 Host 의 공통 삭제가 `0x10` 을 더해 cleanup 에 넘기므로 결과가 같다. ② `old.pool !== pool` — `host.fromForm` 은 같은 호스트의 프로세스만 돌려주므로 다른 풀의 `RawPathProcess` 가 올 수 없다(방어선).
- 앱 수명 연결과 Session 의 부모 가상 이동 명령(예: 사제 이동 명령이 `start` 호출)은 4-D/6 단계 조립 몫이다. `RawPriestWorldBindings` 본체(사제 월드 바인딩)는 다른 client 단위들이 선행이라 이월한다.
- 3.1절 6-P 셋째 항목을 이 단위로 닫는다. 다음은 **7-P1 비행 유닛 부품**의 첫 항목 RawFlyerProcess(238줄 +53)이다.
