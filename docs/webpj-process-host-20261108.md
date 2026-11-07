# webpj Process 호스트·Regular·Kernel 기준선 정렬 — WB04-D5c

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 D5c 단위다. cpppj 기준선 `1f00a25a`(제품985)의 `o/SquidProcess`·`o/RawProcessAttach`·`o/RawBaseProcessAttach`·`o/RawProcessRemoval`·`o/RawAuthorityState`와 `o/Kernel`의 진단 부분을 웹으로 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 이 문서는 Node v24.21.0, HJOW-Athlon에서의 기록이다.

## 1. 옮긴 것

| 웹 모듈 | cpppj 근거 | 내용 |
| --- | --- | --- |
| `www/core/squid-process.js` | `o/SquidProcess.cpp/.h` | `SquidProcess`(form/parent/pid/host, 가상 통지 `onFormDestroy`·`onParentDying`·`onAttached`·`notify`), `RegularProcess`(event·payload(float32)·count·time(double), `resetTask`·`setNextTime`·`setInitialCount`), `SquidProcessState`(시각·권한·서버·타입 순환·geyser/일반/공유 TYPE·상한·ProcessForm TYPE, 독립 값 또는 `bindAuthority`/`bindNow`/`bindGameAux`로 장기 전역 대여), `createProcessHostRuntime`(현재 World 처리기 묶음), `SquidProcessHost`(`attach`·`reattach`·`addRegular`·`addSharedRegular`·`attachRegular`·`refreshForm`·`kill`·`isRegistered`·`fromForm`·`notifyDependency`·`findEvent`·`findEventMasked`·`runRegular`·`hooks`·`owns`·`hasOnlyRegularProcesses`·`drainRuntimeBindings`·`setRegularHandler`·`setFormTransmit`·`bindFormFactory`, 장기 호스트는 `SquidProcessHost.borrowing`), `scheduleBridgeFall`·`hasScheduledBridgeFall` |
| `www/core/raw-process-attach.js` | `o/RawProcessAttach.cpp`, `o/RawBaseProcessAttach.cpp` | `attachProcessForm`(004b1440 파생 +74 접두: 부모 변경 진단 0xa77 → 서버이면 lookup → writer 크기 진단 0xa7f/0xa81 → 송신 → flags\|0x40 공통 부착), `attachBaseProcess`(타입 진단 0x1c → 필드 쓰기 → 부모 free/dead 조기 반환 → Kernel 등록 → 새 Form이면 Create·타입 진단 0xab7/0xab8·TYPE/PID·공통 부착(flags\|0x80)·가상 통지, 지정 Form이면 free일 때만 Take/타입·PID/부착, 부모·PID 일치 진단 0x5e/0x5f) |
| `www/core/raw-process-removal.js` | `o/RawProcessRemoval.cpp` | `removeAttachedProcesses`(004b27a0: 후보의 next를 삭제 전에 저장, 프로세스 TYPE 범위 10..70 미만·현재 상한 미만, 정확한 타입 또는 전체를 flags 0x40으로 Kill) |
| `www/core/raw-authority-state.js` | `o/RawAuthorityState.h` | 서버/권한 두 논리 전역(초기 true) |
| `www/core/kernel.js` (보완) | `o/Kernel.cpp/.h` | 기준선 정렬 — 진단 장치(`bindDiagnostics`: 소진 때 요약→상세→단언 0x7a, 빈 슬롯 삭제 단언 0x82), 슬롯 0의 조회·삭제(프레임 실행·Clear 제외), 슬롯 0 덮어쓰기에서 밀려난 객체 보관, 소멸 중 같은 슬롯 재삭제 거부, `drainOwnedForShutdown(select)`(= `dispose`) |
| `www/core/squid-form-dispatch.js` (보완) | `o/SquidFormDispatch.cpp/.h` | `bind(SquidProcessHost)` — Process 클래스 등록. 혼합 종속 체인의 중첩 삭제가 Process까지 포함 |

C++ 소유권 전용 장치(`unique_ptr` 이동·`weak_ptr` 대여 식별)는 JS 객체 동일성과 `createProcessHostRuntime`의 `activeCalls` 카운터로 옮겼다. 호출 위치마다 현재 World 처리기 묶음(삭제기·Regular 처리기·자산 훅·전송 경계·Pop 깊이·진단·공장)을 조회하고, 진행 중 호출 수를 올린다.

## 2. 검증

원본 x86 기대값은 `cpppj/tests/fixtures`를 직접 읽는다(복사 없음). 10.78 행만 쓰고 CD 10.72·10.37 행은 범위 밖이다.

| 검사 | fixture(행) | cpppj 대응 검사 |
| --- | --- | --- |
| `squid-process.test.mjs` | `process-x86`(10.78 시나리오 전체, 연산마다 풀 수치 13개·슬롯·프로세스 필드·사건·해제 목록, 독립 권한과 **빌린 권한** 두 번 재생), `process-attach-current-local`(24), `baseprocess-attach-current-local`(24) | ProcessTests·ProcessAttachCurrentTests·BaseProcessAttachCurrentTests |
| `process-form.test.mjs` | `processform-x86` Attach(288), `processretake-x86`(194) | ProcessFormTests(Attach 부분)·ProcessRetakeTests |
| `process-transmit.test.mjs` | `processtransmit-x86`(10.78) | ProcessTransmitTests |
| `regular-current.test.mjs` | `regular-current-find`(1232), `regular-current-constructor`(32; 외부 장치가 시각을 바꾸는 32행은 별도 관찰) | RegularCurrentTests |
| `raw-process-removal.test.mjs` | `process-removal-filter`(64) | rawProcessRemoval_native_mixed_form_filter |
| `process-class-current.test.mjs` | (독립 입력) | ProcessCreationCurrent_IndependentHost…, ProcessClassCurrent_ActualHostAndDispatchIgnoreTypeAliases(32조합)·NullParentNotice… |
| `kernel-diagnostics.test.mjs` | (독립 입력) | KernelDiagnosticCurrentTests 5장면(Client 접합 제외) |
| `content-form-lifetime.test.mjs` (보완) | — | ContentLifetime_SharedMixedFormDispatch의 Process 부분 |

최초 실패와 교정: 구현 직후 가장 큰 재생(시나리오 재생·Attach 288·재수신 194·finder 1232)은 처음부터 통과했다. 실패한 것은 입력 구성 실수(새 풀의 부모 슬롯이 free라 patch 판 조기 반환이 일어남, `factory.create`한 부모가 void라 Regular가 실행되지 않음, 첫 프레임 시각 가정)였고 모두 검사 쪽을 고쳤다. 제거 필터 fixture의 raw 노드는 가상 표 0인데 현재 소스의 링크 getter/setter가 미복원 가상 표를 거부해, 웹 입력은 그 노드에 실제 Form(Content)/자산 가상 표를 넣었다(필터·순서·flags 비교는 같다).

## 3. cpppj 옛 검사와 기준선 소스의 불일치 (사용자 보고 대상)

기준선 소스 읽기로 보아, 아래 cpppj 검사 기대는 현재 소스와 맞지 않는 것으로 보인다. 웹은 **소스와 원본 fixture**를 따랐고 cpppj를 실행해 확인하지는 않았다(cpppj 수정은 CP 몫이다).

1. `process_attach_guards`의 `Attach(…, 46, parent, 0)`(전송이 필요한 flags)은 "등록 전 거부"로 `Kernel.Size()==0`·`typeRotation==7`을 기대하나, `AttachBase`에는 사전 검증이 없어(20261105-42 이후) Kernel 등록과 회전 전진 뒤 writer 미연결 위치에서 거부된다. 웹은 그 부분 효과(등록 1, 회전 8)를 그대로 검사한다.
2. `ProcessForm_RejectsInvalidNamedFormsBeforeRegistration`도 같은 이유로 "Kernel/회전 불변"을 기대하나 소스는 등록·회전 뒤 거부한다. 예약 헤드(`predictableFirst`)를 지정하면 거부되지 않고 부착된다.
3. `rawProcessRemoval_native_mixed_form_filter`의 raw 노드는 가상 표가 0인데 `FormLinkLayoutAtTable`이 미복원 가상 표를 거부하므로, 형제 노드를 건드리는 Unpop에서 던지게 된다(소스 읽기상).

## 4. 3.6절 판정 (기준선 뒤 cpppj 변경)

대상 행의 기준선 뒤 변경은 `SquidProcess.cpp/.h`뿐이다(`1cd2b05a`·`6fefcff5`·`ceb59671`·`02b00b12`). ①: `ReceiveSharedRegular`(LAN 전체 수신)·`WriteFormPacket`(참가자 월드 동기화)·`shared_`(실제 writer 가상 표 선택)·`ProcessFormTransmission.packet/writeFullData`는 웹 제외 경로이거나 단독 경로 결과를 바꾸지 않는다. ③: `fireAnimationType`·`rotateAnimationType`(035/036 발사·회전 애니메이션 프로세스 TYPE)은 애니메이션 소비자(`RawCampaignArcherBindings`) 이식 때 `GameAuxState`와 함께 추가한다. ②는 없다. 행 base는 `02b00b12`.

## 5. 아직 연결하지 않은 것과 경계

- 실제 World 조립(`RawSurfaceWorld`)의 장기 호스트 대여 장면(생성 공장 연속 대여·World 삭제·`ProcessHostStorage`)은 WB05다. 이 단위의 `borrowing`/`createProcessHostRuntime`은 독립 Pool/Factory/Kernel로 검사했다.
- Client 접합(procSumm.txt 요약 출력, PID 재작성 World 장면)은 Kernel 진단 장치만 옮기고 WB05에서 연결한다.
- 서버 전송의 실제 직렬화기(`WriteData`/`transmit`)는 입력 장치다(`ProcessFormTransmission`). 패킷 분배(`RawPacketDispatch`)와 processform의 Dispatch 행은 패킷 단위에서 옮긴다.
- 파생 프로세스(`Gun`·`Path`·`Construction`·새·Regular 파생 등)의 `notify`·`runFrame`은 각 단위에서 옮긴다. 기반 `notify`는 미복원이면 거부한다.
- 공중 3종은 정적 이식만 하며 전용 검사를 추가하지 않는다.

## 6. 다음 시작점

D5c로 3.2절 순서3의 `SquidProcessHost`·Regular 공통·Process 부착이 끝났다. 남은 것은 **WB04-E**(공간 spots/해시 등록·Graph/점유·Pop/PostPop·소유자/지역/장부·HP/프레임·내용물 생성과 소유자 마감)다. 대응표 base `1f00a25a` 이후 변경 보고가 있는 `todo` 단위는 이식 때 판정한다.
