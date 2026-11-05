# webpj 세션 장기 상태 2단계: 현재 TYPE·모드·장부 전역 — WB04-C2

2026-11-05, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서2(WB04-C)의 두 번째 하위 단위다. 원본 10.78에서 생성·Pop·보상·HP·지형 등 여러 소비자가 같은 주소를 직접 읽고 쓰는 **게임 보조 전역**(현재 TYPE 번호·Form/예약 번호·모드·장부·기한)을 클라이언트 수명의 한 객체로 옮겼다. 앱/세션 연결과 화면 변경은 없다.

## 1. 근거와 범위 조정

- 시작 트리 `176ef6e7`. 착수 때 `node webpj/tools/fixture-manifest.mjs --write`로 C1 항목(`game-random-seed`·`game-clock`·`terrain-tiles`)의 `webCommit`을 `176ef6e7`로 갱신했다. cpppj 변경 보고는 3.5절 7곳 그대로다.
- 근거: cpppj `src/o/RawGameAuxState.h`의 **커밋본 `886f2d23`**(작업 사본과 같음을 확인). 다른 세션의 cpppj 미커밋 변경(Form/Factory 쪽)은 근거로 쓰지 않았다.
- **원본 직접 대조:** 보호 원본 `originals/Netstorm.exe`(SHA-256 `a305414c…d236`)의 PE 섹션 표로 각 원본 주소를 파일 위치로 바꿔 `.data` 초기값과 BSS(파일 자료 밖, 0)를 읽었다. 파일을 읽기만 했고 실행하지 않았다. 47필드+장식 4칸이 모두 cpppj 헤더 값과 같았다.
- **Regular 공통은 WB04-D로 옮김:** cpppj에서 일반/공유 예약(Regular)은 `SquidProcessHost.AddRegular/AddSharedRegular/FindEvent`로, SID 풀에 ProcessForm을 만들고 Kernel에 등록하며 삭제기(`SquidUnpop`·`RawSquidDestroy`)와 묶는다(`tests/RegularCurrentTests.cpp`, `regular-current-*-x86.tsv`). Form/Process 생성이 먼저 있어야 하므로 의존 관계에 따라 WB04-D에서 이식한다. 이번에는 Regular가 읽는 번호 전역(`regularProcessType` 46·`sharedRegularProcessType` 61·`processTypeEnd` 63)만 옮겼다.

## 2. 사용 계약

`webpj/www/core/game-aux-state.js`의 `GameAuxState`. 필드 표 `GAME_AUX_FIELDS`(이름·원본 주소·폭·초기값·대응 타입 이름·설명)와 `WORLD_DECORATION_FIELDS`가 기준이다.

```javascript
// 클라이언트 생성 때 한 번 만들고 생성·Pop·보상 등 소비자가 같은 객체를 빌린다.
const aux = new GameAuxState();
aux.priestType;          // 158 (원본 005412d0)
aux.totalCost = -5;      // signed DWORD 검사 뒤 저장
aux.worldDecorationTypes[3]; // 185 Monument, 같은 Uint32Array 저장소
aux.resetForGeneralWorld();  // 원본 일반 월드 준비의 fresh 입력만 되돌림
```

- **폭:** `u32`(0~0xffffffff), `i32`(signed 32비트), `f32`(Math.fround), `f64`, `bool`. 범위 밖·형식 오류는 쓰기 전에 거부한다. 자원 DWORD는 `resourcePoolBits`/`storeResourcePoolBits`로 같은 DWORD를 unsigned 비트로도 다룬다.
- **장식 네 TYPE:** treeTwo 100·RUIN 184·treeThree 101·Monument 185(원본 순서). 생성 부모가 반복 중 현재 Monument를 다시 읽으므로 같은 `Uint32Array`를 빌려 준다(원소 쓰기는 원본 메모리처럼 32비트로 감김).
- **되돌리기:** `resetForGeneralWorld()`는 cpppj의 일반 월드 fresh 입력(ordinaryGeysers·totalCost·resourcePool·refundPercent 25·allowAnyCapture·archipelago·lost X/Y·geyserType 122·priestQuarterHp)만 되돌린다. 원본 정리 전체가 아니며 현재 TYPE·Form/예약 번호·기한·군도 밀도는 유지한다. `resetLostPositionForCleanup()`은 정리의 두 저장(Y 다음 X)만 -10000.0(0xc61c4000)으로 쓴다.
- **TYPE 번호와 웹 타입 표:** 26칸이 같은 번호의 웹 이름과 맞는다. 예: 158 `priest`, 122 `geyser`, 97 `isle`, 85/87 `sunFactory`/`rainFactory`, 46 `regularProcessType`, 61 `sharedRegularProcessType`, 31 `baseFlyerProcessType`, 1/2 `DependForm`/`ProcessForm`. `processTypeEnd` 63은 프로세스 타입 끝(10+53)이다.
- **범위 밖:** cpppj의 `RawSquidTransmissionState`(공통 전송 버퍼/압축 전역)는 실제 객체 송신 소비자와 함께 판단한다. 실행 중 writer(타입 적재·Start·원문 정리)가 바꾸는 값은 각 소비자 단위에서 대조한다. CD 10.72 주소는 다루지 않는다.

## 3. 검증

| 검사 | 결과 |
| --- | --- |
| 원본 PE 초기값 | 47필드+장식 4칸 모두 `.data` 초기값/BSS 0과 일치(보호 원본 SHA 확인 뒤). double 4칸은 8바이트 전체 비교 |
| 타입 표 이름 | TYPE 번호 26칸이 웹 `TYPE_LOAD_ORDER`·`PROCESS_TYPE_NAMES`·내장 Form 이름과 일치 |
| 계약 | 폭 검사·거부 시 값 유지·자원 비트·장식 저장소 대여·인스턴스 독립·일반 월드 준비/정리 되돌리기 범위 |

- 음성 대조: 임시 사본에서 사제 TYPE 초기값을 159로 바꾸면 PE 대조·타입 이름·계약 검사가 모두 실패함을 확인하고 임시 파일을 지웠다.
- 신규 `tests/game-aux-state.test.mjs` 4/4. 이식 추적 목록에 새 종류 `original-binary`로 `originals/Netstorm.exe`를 등록했다. 전체 결과는 인계 문서 0.1절에 기록한다.
- 원본/CD 실행, analyzeManager, 디컴파일은 하지 않았다.

## 4. 다음

- WB04-C3: 입력/예약 큐(cpppj `client/InputEvent.h`의 `InputQueue`, 명령 예약·취소 순서)와 게임 시계·Kernel의 프레임 순서.
- WB04-D: 이번에 옮긴 `processFormType`·`dependFormType`·`regularProcessType` 등을 빌리는 Form/Process 생성자와 Regular(`AddRegular`/`FindEvent`, `regular-current-*-x86.tsv`).
- 앱 연결: `GameAuxState`도 `ClientRandomGlobals`처럼 첫 실제 소비자와 함께 클라이언트(앱) 수명에 만든다.
