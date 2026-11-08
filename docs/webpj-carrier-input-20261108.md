# webpj 운반물 입력(RawCarrierInput) 이식 — WB04-E97

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 아흔일곱 번째 부분이다. 운반물(Carrier)의 커서 평가(4277f0)·강조(4284a0)·클릭 명령 분배(42a360)·이동 확인 음향(426290)을 cpppj 기준선 `1f00a25a` 의 `client/RawCarrierInput` 에서 옮겼다. E95 의 `raw-carrier-drop-point.js`(좌표 snap)가 선행이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-carrier-input.js` — `CarrierInputState`, `RawCarrierInput(pool, types, audio, stamps, frame, state, hooks)`

- `evaluate(parent, point)`: 부모 TYPE genus(flags2)에 0x30000(이동/운반 계열)이 없으면 진단(0x7b6)을 부르고, 그 경우나 extra&0x20, 사제 HP/지면 검사(`immobile`)면 커서 6 을 전역에 쓰고 반환한다. 아니면 부모 현재 위치를 snap 한 자기 사각형(폭 0.999)에 점이 들어 있는지 본다. 안에 있으면 `insidePoint` 를 (-10000, -10000)으로 쓰고, 내용물이 있고 메뉴 생략 TYPE 이 아니면 내려놓기 위치(`dropPoint`, 내용물 종류)를 부른 뒤 6, 아니면 포획 후보가 있고 같은 Graph 면 8, 아니면 6 이다. 밖이면 후보를 찾고 genus 0x20000(비행)이 없으면 후보 없음일 때 제단(genus 0x200000이고 제단 허용이면 점을 제단 위치로 바꾸고 0x11)·연결 검사 실패 시 간헐천/귀환 장소(점을 그 위치로 바꾸고 3, 없으면 6)·연결되면 제단 없음 0x12/있음 3(점−3)을 정하고, 후보가 있고 (너겟이거나 내용물이 없고) 같은 Graph 면 0xd 로 덮은 뒤 현재 커서를 반환한다. 비행이면 후보 0xd → 간헐천 3 → 귀환 장소 3 → 제단 3(점−3) → 0x12 순이다. 점은 caller 객체를 직접 바꾼다(`highlight`/`commit` 은 값 복사).
- `highlight(number, point, force)`: 점을 snap 한다. number 0 이면 진단(0xce)만, 아니면 진단 이동점 표시가 켜졌을 때 표시 타입(`markerTypes[signed 카운터 % 개수]`)과 snap 점으로 표시·커서 갱신 후 카운터를 DWORD 감김으로 증가시킨다. force 가 없고 다중 선택이 아니면 `evaluate` 결과를 커서로 지정(`selectCursor`), 아니면 0x12 를 지정한다.
- `commit(number, point)`: 점을 snap 한다. number 0 이면 `highlight(0)` 만, 부모 extra&1 이면 무시한다. 비행 TYPE 은 점을 최소 (7, 5)로 올린 뒤 `evaluate` 한다. 커서 3·0xd·0xe·0x12 는 소유자가 로컬이면 확인 음향 후 이동 요청(`request`, 0), 8·9 는 포획(`pickup`)+선택 해제, 10 은 로컬이면 내려놓기 음향(TYPE dropSound) 후 운반물 내려놓기 요청(`dropCargo`)을 하고 6·0xb·0xc 와 같이 선택 해제, 0x11 은 제단 희생(조기 희생이면 로컬/원격 조기 처리, 아니면 희생 요청 후 로컬이면 확인 음향+제단 보관+희생 표시), 그 밖은 진단(0x179)이다. 소유자는 각 시점마다 슬롯에서 다시 읽는다.
- `acknowledge(parent, index)`: 같은 TYPE 이 같은 프레임에 이미 소리를 냈으면 무시하고 아니면 장부(`stamps`)를 갱신한다. 사제는 index 를 0..1(범위 밖이면 난수(0,2000)/1000), 골렘은 0..4(난수(0,4000)/1000)로 이름 소리(`namedSound`), 나머지는 TYPE 이동 음향 핸들(`sound`)이다.
- 도달한 훅이 없으면 오류이다(`CarrierInputHooks` 대응 함수 31개: immobile·content·candidate[{sid, nugget}]·dropPoint·graph·connected·surface·geyser·returnPlace·daisAt·validGeyser·altarAllowed·localOwner·multiple·checking·earlySacrifice·assertion·highlight·selectCursor·select·request·pickup·dropCargo·earlyLocal·refreshCursor·earlyRemote·showSacrifice·marker·random·namedSound·sound). `frame` 은 `{ value }` 또는 getter 이다.

## 2. 검증

- `tests/carrier-input.test.mjs`(2건): `carrier-input-x86.tsv` 454행(10.78 전용 단일 판본, 53/64비트 열 동일). 입력 33 DWORD(연산 0 Evaluate/1 Highlight/2 Commit/3 Acknowledge·genus·flags·extra·소유자·로컬·즉시 이동 불가·내용물·후보·Graph·연결·표면·간헐천·귀환·제단·허용·조기 희생·다중·표시·강조 힘·부모 번호·초기 커서·스탬프·프레임·인덱스·TYPE·점 x/y·콜백 변이 8종)로 반환 코드·효과 순서와 모든 인자(I 진단/H 이동 불가/C 내용물/Q 후보/D 위치/G Graph/L 연결/S 표면/Y 간헐천/V/R 귀환/A 제단/F 허용/E 조기/M 다중/U 선택/B 강조/X 커서/T 요청/P 포획/N 내려놓기/W·J·K 희생/Z 표시/O 갱신/? 난수/a 이름 소리/s 소리)·caller 최종 점 8바이트·전역 여섯 값·TYPE 두 플래그와 프레임 장부·여섯 슬롯 전체 raw 가 같다. 계약: 미연결 하위·판본/TYPE 표 크기/음향 장부/장부 크기/SID 범위 오류를 해당 분기를 쓰는 시점에 거부한다.

최초 실패와 교정: 첫 실행에서 시험 장치의 이름 소리 훅이 골렘 인덱스 4 에서도 이름 표를 조회해 실패했다. 원본/C++ 은 인접 인덱스 4 를 표 조회 없이 "@1" 로 쓰므로 시험 장치를 그에 맞게 고쳤다(제품 코드는 그대로). 이후 454행 모두 통과했다.

## 3. 경계와 남은 것

- 실제 월드(포획 후보 426900·Graph 462c80·연결 488340·표면 40d9d0·간헐천 4b1fa0·귀환 장소 4263a0·제단 4b2070 조회·사제 지상 고정·분산 요청 427340/428e00/427070·UI/커서/음향 장치)는 훅 경계이다. 이동 요청의 서버 처리와 `RawCarrierMove`/`RawCarrierTarget`/`RawCarrierChoice`/`RawCarrierEscape` 본체는 해당 단위에서 한다.

## 4. 다음 시작점

`RawCarrierMove`·`RawCarrierTarget`·`RawCarrierChoice`·`RawCarrierEscape`(운반물 이동/표적/선택/탈출), `RawBalloonFrame`·`RawPathRuntime` 계열(보행/경로), `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
