# webpj 다이스·제단 수명 RawDaisLifecycle (WB04-E369, 2026-11-13, 7-P3 신전·제단·의식 부품 다섯 번째 항목)

3.1절 7-P3 항목 "RawDaisLifecycle 192줄(+15; 기준선 집계 190)"을 `core/raw-dais-lifecycle.js`(`RawDaisLifecycle`·`DaisLifecycleState`·`makeDaisLifecycleHooks`)로 옮겼다. 선행 부품은 모두 있었고(E367 `RawDaisRegular`·E368 `RawAltarSacrifice`/`RawDaisDeletion`), 이 단위가 새로 만든 공유 부품은 `DaisRegularState` 의 접근자 둘(`playerRecords` getter, `grantFlagWriter(owner)`)뿐이다. 기준선 뒤 cpppj 변경은 `4ee213870`(`20261109-95 CP`) 하나로, 제단 전처리와 다이스 삭제 후처리가 고유 삭제 실행기(`RawDaisDeletion`)에 위임하도록 바뀐 것이며 3.6절 ③(공통 Native 장기 연결)로 판정해 반영했다(PORT_MAP base `4ee21387`). cpppj 시험과 fixture(`DaisLifecycleTests.cpp`, `daislifecycle-current-x86.tsv`)는 기준선 뒤 변경이 없다.

## 1. 규칙

- **생성:** 풀(10.78)과 여섯 협력 몸체(발자국·신전 의식·제단·Carrier 후처리·Damageable 전처리)가 같은 풀이고 TYPE 표가 188칸이어야 한다. 효과 훅은 생성 때 요구하지 않고 도달한 훅만 요구한다(없으면 `다이스 수명의 선택된 실제 장치 미연결`).
- **`handles`/`validate`:** 최종 가상표가 다이스(0x00507950)이거나 제단(0x005076b8)일 때만 대상이다. `validate`(공간 쓰기 전 검사)는 다이스 TYPE+다이스 가상표 또는 제단 TYPE+제단 가상표 짝만 허용하고, 다이스일 때만 owner 0..8 범위와 "시간 억제가 아니고 게임 시각이 0 이며 flags 하위 3비트가 있으면 저장 개수 ≤ 3" 을 본다.
- **다이스 최초 Pop (`postPop`):** ① 시간 억제가 아니면 — 게임 시각 0·flags 하위 3비트면 저장 개수(0..3)의 다음 첫 프레임 표 `{3, 64, 120, 0}` 을 `setFrame(sid, 값, 0)` 으로 쓰고, 그 뒤 owner 의 획득 플래그 BYTE 를 `(가상 Count − 1) & 0xff` 로 갱신한다(쓰기 칸은 가상 Count **앞에서** 고정 — `grantFlagWriter`). ② flags 2 → owner(≠0)의 두 번째 SID 쌍 두 칸, flags 1 → 첫 쌍 두 칸에 이 SID 를 쓴다. ③ flags 1 이면 권한이 있을 때 로딩 중 단계가 0 이 아니면 가상 Unpop → 가상 Count → 단계 첫 프레임 표 `{0, 3, 64, 120}` 을 프레임 DWORD 네 바이트에 **직접** 쓰기(표시 setter 없음) → Repop 을 하고, 그다음 현재 제단 TYPE 을 만들어 owner 지정·다이스 좌표로 전체 Pop 한다. ④ 로컬 owner 이고 로딩 밖이면 지면 조회 → 섬이 있고 번호가 Player 의 홈 섬과 다르면 옵션 `noShowWarnCantUpgrade`·억제를 본 뒤 `WarnCantUpgradeAltarOffHomeIsland` 를 말한다. ⑤ 항상 공통 Damageable 후처리로 끝난다.
- **제단 최초 Pop:** flags 1 일 때만 로컬 21 정기를 예약(`reserveRegular`)하고 성공하면 현재 저항 지연 초(float32)를 payload 로 부착한 뒤 공통 Damageable 후처리로 끝난다.
- **제단 전처리 (`altarPreDestroy`):** 제단만 받는다(다이스 거부). 고유 삭제 실행기 `RawDaisDeletion.altarPre` 에 위임한다 — 같은 살아 있는 다이스 삭제(삭제 flags 에 0x400000 이 없을 때) → 발자국 안 추상 룬 정리(권한이 있거나 클라이언트 번호) → 전체 Damageable 전처리.
- **삭제 후처리 (`postDestroy`):** 실제 Player 기록을 묶은 다이스는 `RawDaisDeletion.daisPost`(기록을 단계마다 다시 빌림)에, 독립 입력 배열 경로와 제단은 이 모듈에 맡긴다. 독립 경로는 현재 owner(≠0)의 두 번째 쌍이 이 SID 와 정확히 같으면 두 칸을, 첫 쌍도 같으면 두 칸을 0 으로 지운 뒤 전체 Damageable 삭제 후처리를 flags 그대로 부른다.
- **상태 `DaisLifecycleState`:** `loading`(Carrier 후처리의 로딩 깊이)·`gameTime`(실제 로그인 종료 벽시계 시각, Process 시계 아님; 실제 Client 는 `bindGameTime({ value })` 로 장기 값을 빌림 — C++ `double&` 대응)·`players`(독립 입력 owner 0..8 의 SID 쌍 네 칸 + 홈 섬, 16비트 저장).
- **새 접근자:** `DaisRegularState.playerRecords`(묶은 실제 Player 기록 9칸 또는 null — C++ 공개 멤버 대응)와 `grantFlagWriter(owner)`(획득 플래그 BYTE 쓰기 대상을 호출 시점에 고정한 쓰기 함수 — C++ `GrantFlagSlot` 참조 반환 대응; 가상 Count 가 기록/배열을 다시 묶어도 고정한 칸에 쓴다).
- **훅 공장:** `makeDaisLifecycleHooks(pool, frame, host, factory, owner, hooks)` 는 `Frame.set`·`Factory.create`·`Owner.set`·`Host.addRegular`(로컬 Regular)를 연결한 새 묶음을 돌려준다(풀이 다르거나 몸체가 없으면 거부, 나머지 훅 보존, 원래 묶음 불변). Pop·공통 파생 삭제·UI·지면 조회는 호출자가 실제로 연결한다.

## 2. 웹 범위

- 10.72(CD)는 범위 밖이다(판본별 필드 폭·오프셋·CD 전용 TYPE/가상표 계약·CD 호출 순서 차이를 옮기지 않았다). 실제 Pop·`SquidPostPop.setBody('dais', …)` 등록·`DamageablePostDestroy`/공통 파생 삭제 분배·UI/채팅은 호출자 훅이며 앱 수명에는 연결하지 않았다(`PORT_MAP` session 0). 이 모듈은 DOM 을 쓰지 않는다.
- `SquidPostPop.setBody('dais', lifecycle)` 가 요구하는 `handles`/`validate`/`postPop` 시그니처와 맞다(웹 `squid-postpop.js` 의 몸체 등록 방식). cpppj 의 `SetDaisPostPop`(기준선 뒤 `SquidPostPop` 변경)에 대응하는 연결은 `SquidPostPop` partial 항목(3.6절 ③)에서 이어받는다.

## 3. 검사 (`tests/dais-lifecycle.test.mjs` 2건 · `tests/dais-lifecycle-contract.test.mjs` 17건 · 장면 보조 `tests/dais-lifecycle-support.mjs`)

- **원본 독립 관측(10.78):** `daislifecycle-current-x86.tsv` originals 964행(모드 dpp 878·apre 48·dpost 24·app 10·apost 4)의 효과 순서(F 프레임·unpop/repop·C/owner/P 제단 생성·N/REG 21 예약·SURFACE/HAS/ISLAND/OPTION/CHAT 경고·destroy·postpop/post/B 마지막 공통 경계)·경계 직전 네 슬롯(5·7·8·9) raw 전체·아홉 Player 의 획득 플래그/SID 쌍 네 칸이 **독립 입력 배열과 실제 Player 기록 묶음 두 경로 모두 첫 실행에서 일치**했다(총 1,928회). 묶음 경로는 독립 배열을 `0xdead` 로 오염시켜 사본을 읽으면 반드시 실패하게 했고 출력 필드(+0x71, +0x8c..0x9c) 이외 Player 바이트가 보존됨을 전수 확인한다. 실제 `SquidFrame`/`SquidProcessHost`/`RawDamageablePreDestroy`/`RawCarrierPostPop`/`RawAltarSacrifice`/`RawFrameAdvance` 에 연결한 장면이다. 이 fixture 에는 로딩 중 Unpop/Repop 경로가 한 번도 없다(원본 관측 0건) — 그 경로는 계약 시험이 맡는다.
- **웹 계약 17건:** 생성 거부(협력 몸체/풀/188칸 표)·handles/validate(가상표·TYPE 짝·owner 범위·저장 개수 경계·flags/시각/억제 관문)·상태 상자 대여·`DaisRegularState` 새 접근자(고정 쓰기)·다음 첫 프레임 표와 표 밖 거부·획득 플래그 `count − 1`(0 은 255, 음수 훅 반환은 부호 없음)과 Count 재진입 시 쓰기 칸 고정(묶음/독립)·주입 훅 우선(suppressed/boss/count)·SID 쌍 등록(flags 1/2/3, owner 0)·**한 바이트를 넘는 SID(291)의 DWORD 폭**·로딩 중 단계 되돌리기(개수 0..3 표, Unpop 이 프레임 DWORD 를 오염시켜도 네 바이트를 덮어쓰고 나머지 바이트 불변, 개수 4·음수 거부, 로딩 밖/단계 0 건너뜀, 제단 TYPE·owner·좌표 인자)·홈 섬 밖 경고의 조건 순서(지면/섬/번호/옵션/억제 재읽기, 묶은 기록의 홈 섬 번호)·제단 21 예약(float32 payload)·제단 전처리/삭제 후처리(두 경로, 일부 쌍만 일치, 미러만 일치, owner 0/9, 제단은 쌍 불변, 비대상 가상표 거부)·훅 요구(15곳의 문구와 이전 효과)·훅 공장(연결·풀 불일치·원래 묶음 불변).

## 4. 변형 확인과 남은 경계

- 연산자·상수 자동 변형 170개 + 140~262줄 재확인 79개 + 의미 변형 19개를 적용했다. 1차에서 14개가 남았고(관측이 한 SID·한 프레임 값만 써서 가려지던 DWORD 폭·프레임 쓰기 바이트·`repop` 인자), 계약 시험으로 잡았다.
- 남은 동등 변형 8개: 임시 `DataView` 크기 4→5(`setUint32/getFloat32` 만 씀)·`#playerField` 의 `field > 4` 방어(비공개 호출자는 0..4 만 부름)·같은 곳의 `PLAYER_PAIR_AT + field * 4`(묶은 기록에서 SID 쌍을 읽는 경로는 닿지 않음 — 묶은 다이스 삭제는 고유 삭제 실행기가 처리)·`#setPlayerField` 의 `field > 3` 방어·`second`/`secondMirror` 칸 맞바꿈 두 건(두 칸에는 항상 같은 값을 쓴다)·프레임 직접 쓰기의 시프트 8→9(단계 표 값이 모두 256 미만)·고유 삭제 실행기에 넘기는 제단 TYPE 인자(`altarPre`/`daisPost` 는 읽지 않음).
- 시험이 먼저 실패한 곳은 모두 시험 쪽이었다(`null` 인자를 기본값 연산자가 되돌림, 단계가 0 이 아닌 프레임은 63/119/179 뿐이라 14 를 쓴 입력, 억제 훅이 참이면 조회 효과가 기록되지 않는 기대, 실제 Frame 훅이 짝수 번째 갱신을 기록하는 효과). 모듈은 관측 대조에서 바뀌지 않았다(미사용 상수·초기값만 정리).
- **통합 시험 이월:** cpppj `DaisRitualConsumerTests`(최초 Pop → 포획 부모 알림 → 제단 24 → 전체 삭제를 한 월드에서 이어감)는 `SquidPostPop` 의 다이스 몸체 연결(`SetDaisPostPop` 대응)·`ContentFormHost`/`ContentFormLifetime`/`SquidFormDispatch`/`DamageablePostDestroy` 조립이 필요해 이 단위에 포함하지 않았다. 해당 연결은 `SquidPostPop` partial 항목(B 구획)과 7 단계 연결에서 함께 다룬다.
- 다음 7-P3 항목은 **RawVortexAscend**(partial 181줄 +4; RawIslandRuntime 을 직접 읽는 두 번째 생성자)이다.
