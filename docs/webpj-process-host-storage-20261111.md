# webpj 장기 ProcessHost 저장소(RawProcessHostStorage) — WB04-E297 (2026-11-11, 4-B)

3.1절 4-B 의 **RawSurfaceWorld** 항목을 풀면서 먼저 필요한 선행 부품 **RawProcessHostStorage**(client, 기준선 143줄 = `.cpp` 75 + `.h` 68)를 `core/raw-process-host-storage.js` 로 옮겼다. RawSurfaceWorld 가 장기 Host/Kernel/상태와 짧은 현재 World 처리기 대여를 이 저장소로 나누어 쓴다(체크 목록에 RawProcessHostStorage·RawSurfaceStorage 두 선행 항목을 추가했다). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `new RawProcessHostStorage(pool, types, authority, gameAux)`: pool 은 10.78 `SidPool`, types 는 같은 저장소의 TYPE 표, authority 는 `RawAuthorityState`(없으면 독립 값), gameAux 는 `GameAuxState`(없으면 독립 값). `state`(`SquidProcessState`)는 권한/보조 TYPE 소유자를 빌려 매번 현재 값을 읽고(pollType 은 `gameAux.geyserType` 별칭), `bindDevices(kernel, clock)` 가 실제 Client Kernel 과 프레임 시각 `{ game }` 을 pin 하면 `state.now` 가 `clock.game` 을 읽는다. `kernel` 은 외부 장치가 없을 때 독립 `Kernel(39999)` 을 한 번만 만들고, `host` 는 첫 사용에 `SquidProcessHost.borrowing` 장기 Host 를 한 번 만들어 같은 객체를 유지한다.
- `bind(runtime)`: 현재 World 의 처리기 묶음(`createProcessHostRuntime`)을 **단일** 대여한다 — 현재/레거시 대여가 이미 있거나 삭제기 Pool 이 다르거나 공장 Pool 이 다르면 현재 바인딩을 바꾸기 전에 거부한다. 반환 `Use` 는 `release()`(C++ `~Use`; 자기 슬롯만 반납, 멱등)·`drainOwnedForShutdown()`(자기 대여가 만든 프로세스만)·`clearForState()`(현재 대여의 실제 `Kernel.clear` 한 번이 정상 복귀했을 때만 `ClearPass`; 호출 동안 `activeCalls` 로 중첩 반납을 막음)·`returnForState(pass)`(같은 저장소/대여의 증빙, 진행 중 호출 없음, Regular 이외 잔류 없음을 모두 확인한 뒤 현재 슬롯을 비우고 같은 Kernel 을 pin 한 `Returned` 발급; 모든 거부는 증빙/대여를 소비하기 전)를 가진다.
- `reclaimForAllocation(sid)`: SID 소진 복구가 현재 World 의 가상 삭제(`destroyDependent`, flags 0)를 요청하고 호출 동안 대여를 붙잡는다(현재 World 가 없거나 `asset.emit` 이 없으면 거부). raw 외부 Kernel 의 레거시 회수 대여(`bindLegacyAllocationReclaim` → `release()`)가 살아 있으면 그 함수를 호출한다.
- `dispose()`(C++ 최종 소멸): 독립 Kernel 은 모든 프로세스를, 외부 Kernel 은 이 Host 소유 프로세스만 반납하고(Host/Pool 은 그때까지 유지) 장치/Host 를 비운다. 웹에는 소멸자가 없으므로 대여/레거시 대여/저장소 최종 소멸은 명시 호출이다(멱등). 증빙(`ClearPass`/`Returned`/`Use`)은 비공개 표식 없이는 만들 수 없다.
- 기준선 뒤 cpppj(+13/-2 `.cpp`, +10 `.h`)의 장기 기본 장치 보관(`BindLifecycleDefaults`/`BindFormFactoryDefaults`/`BindFormTransmissionDefaults`), fire/rotate 애니메이션 TYPE 참조, `ClearCall` 의 전체 owner pin 은 Session 조립용 Native 연결이라 **3.6절 ③ 으로 4-D 에서 반영**한다(판정 커밋 `417f22d2`; 대응표 base 를 올림).

## 2. 검사 (`tests/raw-process-host-storage.test.mjs` 11건)

원본 관측 파일이 없는 단위 계약 검사이다. cpppj `ProcessHostStorageTests` 의 장면을 RawSurfaceWorld 없이 저장소 수준으로 옮겼다(World 대신 처리기 묶음을 대여).
- 잔류 Regular 가 단일 Clear 뒤 old 대여 반납과 Pool reset 을 넘어 같은 PID 로 남고 새 대여의 처리기를 쓴다(일반/공유 Regular 두 factory), 무효 부모(free/dead/void)와 예약 시각, 다른 저장소의 증빙·처리기·장치 거부와 소비한 증빙 재사용 거부, 실제 Client 시각/권한/보조 TYPE 의 직접 빌림과 반납 뒤 시각 유지, 독립/외부 Kernel 최종 소멸과 Host/Pool 수명, Regular 이외 잔류의 반납 거부와 PID 보존, 진행 중 Regular 효과/Clear 중 이전 증빙 재진입 거부('진행 중'), SID 소진 복구와 레거시 회수 대여, 구성 입력/증빙 위조 거부.
- 보강 2건: 낡은 대여/증빙이 새 대여의 현재 슬롯·프로세스에 영향을 못 줌(release 정체성, drain 정체성, 같은 Pool 의 다른 저장소가 같은 처리기 묶음을 빌려도 증빙 저장소 정체성 거부, release 한 대여의 반납 거부), 장치 pin 규칙(대여 중 같은 주소 재확인, 레거시 대여 중 새 장치 거부, 독립 Kernel 이 이미 있으면 교체 거부, `usesKernel(null)`, 독립 Kernel 용량 39999, 소멸 뒤 새 Kernel).
- 변형 확인 55개 중 처음에 14개가 살아남았고 보강 뒤 **1개만 남았는데 동등 변형**이다: `bindDevices` 의 "현재 대여 중" 관문 — 대여(`bind`)가 독립 Kernel 을 이미 만들어 두므로 다른 장치 거부가 항상 먼저 일어난다.
- 기억할 차이: 웹 Host 는 현재 대여에 `regular` 처리기가 없으면 원본 기본(payload 그대로 반환)을 쓴다(WB04-D5c 의 기존 규칙). 현재 대여가 **없는** 때만 유효 부모의 Regular 사건을 거부한다.

## 3. 남은 경계

- `RawSurfaceStorage`(Pool/해시/공간/그래프 저장소의 장기 수명)가 이 저장소를 `Processes()` 로 지연 생성해 쓴다(다음 항목). 3.1절 4-B 는 28/33 이다(선행 두 항목 추가).
