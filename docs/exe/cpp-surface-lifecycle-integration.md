# 섬 삭제·다리 지연 낙하·연결 객체 정리 통합

2026-10-08, 호스트 **VM-W11-CODEX**. 1차 기준은 10.78이다. 마지막 디컴파일 수행 PC는 **VM-W11-CODEX, 2026-10-08**이며 이번에는 같은 PC의 기존 내보내기를 사용했다. 새 디컴파일·원본 게임/복사본·클론 창 실행은 없었다.

[RawSurfaceLifecycle](../../cpppj/src/o/RawSurfaceLifecycle.h)는 이미 복원된 파생 삭제 훅을 공통 삭제와 연결하는 호스트 분배기다. [SurfaceLifecycleTests](../../cpppj/tests/SurfaceLifecycleTests.cpp)는 실제 Factory·Pop·소유자·연결·삭제·프로세스 모듈을 함께 실행한다. 이전의 독립 기계어 대조 입력 수 **131,542개**는 그대로이며 이번 통합 검사를 새 x86 입력으로 더하지 않는다.

## 연결과 실행 순서

`RawSurfaceLifecycle::Hooks()`를 `SquidProcessHost`의 asset 훅으로 공급한다. ProcessHost는 ProcessForm과 종속 체인 처리를 먼저 받고 자산 삭제만 이 분배기로 넘긴다. 분배기는 타입 번호 대신 raw 슬롯의 실제 가상 표 기록값으로 아래 몸체를 선택한다.

| 객체 | preDestroy | postDestroy |
|---|---|---|
| bridge | `RawBridgeLifecycle::PreDestroy`, 실제 연결 타입 155 | `RawBridgeLifecycle::PostDestroy` |
| island | `RawIslandLifecycle::IslandPreDestroy` | 공통 훅 |
| noIsland | 공통 훅 | `RawIslandLifecycle::SurfacePostDestroy` |
| 그 밖의 자산 | 공통 훅 | 공통 훅 |

다리의 `DestroyLink`는 최종 `ProcessHost::Hooks()`를 포함한 실제 삭제 경로로 재진입한다. BasePre/BasePost는 공통 훅을 정확히 한 번 수행하고, walker 낙하와 파편/소리는 각각 외부 경계로 전달한다. 공통 선택 조회/form 해제 콜백을 보존하며 누락된 훅·잘못된 연결 타입·다른 풀은 구성 때 거부한다. 내부 콜백이 `this`를 참조하므로 분배기 복사/이동을 금지했다. 내부 다리 탐색기를 중첩 재사용하지 않아야 한다.

통합 검사는 다음 흐름을 실제 raw 풀에서 수행한다.

1. 9칸 noIsland를 행 우선으로 Pop한다. F가 받침과 종유석을 생성하고 H가 전체 소유자를 3으로 맞춘다.
2. 오른쪽 경계에 실제 K 다리를 Pop한다. 파생 postPop의 연결 순회가 표면 칸과 다리를 참조하는 bridgeConnector 하나를 생성한다. 이 시점에는 두 참조가 살아 있으므로 `LinkNeedsDestroy`는 거짓이다.
3. 받침 `(30,30)`을 실제 Destroy한다. 파생 preDestroy가 중심 `(28.5,28.5)`으로 Regular 이벤트 `0x2692`를 예약한다. payload는 `7196`이다. 받침의 공간/통계는 해제되고 표면·종유석·연결 객체는 남는다.
4. 다음 Kernel 프레임에서 실제 다리 이벤트 처리기를 실행한다. 오른쪽에 살아 있는 다리가 있으면 O 첫 프레임 56의 새 객체를 만들고, 옛 다리를 삭제한 뒤 같은 `(31,29)`에 Pop한다. 소유자·수명 4·부모 단어·extra의 0x10 비트를 복사한다.
5. 옛 다리의 dead 비트가 켜진 preDestroy에서 `LinkNeedsDestroy`가 참이 되고 연결 객체를 실제 Destroy한다. 공통 Unpop/반납이 연결 해시를 비우고 공통 통계를 감소시킨다. 부모 다리의 삭제는 실행 중 Regular의 ProcessForm도 정리한다.

`schedule → bridge destroy → link destroy → 파편 요청 → 낙하 소리` 순서를 확인한다. 삭제 전후 깊이와 postPop 중첩 깊이는 0으로 돌아오며 비용·타입별 통계·해시·spot·SID 여유 수를 함께 검사한다. 받침 삭제가 표면 9칸과 종유석을 포괄 제거하는 명령은 별도로 구현하지 않았다.

## 검사 결과와 범위

10.78/CD 각각 정상 끝 칸 교체, 고립 다리 삭제, hard 프레임 유지, flags `0x1000` 보존, noIsland 삭제의 walker 낙하 분배를 검사한다. hard는 K 프레임 46과 수명 4를 유지하며 Regular/연결도 남는다. 뒤따르는 실제 부모 삭제가 둘을 정리한다. `0x1000`은 받침 삭제 시 예약만 막으며 이후 다리 삭제의 연결 정리는 그대로 수행한다. 별도 검사로 미연결 훅·풀/타입 오류와 공통 분배 계약을 확인했다.

- x64 Release **경고/오류 0**. 전체 콘솔 CTest **246개·실패 0**(기존 240 + 새 6, 테스트 60.22초·CTest 전체 60.27초).
- 관련 감사 **7종 모두 통과**: bridgeeffects, bridgeevent, bridgeconnect, islandlifecycle, islandpostpop, setframe, process. 기존 도구·fixture·내보내기·PE SHA/경계 기록을 확인했다. 새 PE 실행 관찰은 생성하지 않았다.
- 로그: `extracted/build-surface-lifecycle.log`, `extracted/ctest-surface-lifecycle.log`, `extracted/audit-surface-lifecycle.json`(Git 제외).
- 최초 검사에서 SID 여유 수 기대값에 받침 반납 한 개가 빠져 8개 확인이 실패했다. 기대값을 바로잡았으며 최종 검사에서는 실패가 없다.

Graph는 비활성이다. 다리 프레임 코드는 실제 `bridge.type` 추출값이며 다른 프레임과 SHP 크기는 합성이다. 표시 갱신·표면 진단·삭제 보상·walker 낙하 몸체·파편 생성·소리 출력은 기록/외부 훅이다. walker 객체는 합성 raw 입력이며 표면 삭제/Unpop/탐색/통계 갱신은 실제 모듈이다. GUI raw GameWorld와 Kernel의 게임 시각/루프 연결은 남아 있다.

## 다음 작업

**2026-10-08 후속 완료:** [실제 SHP/표시 연결](cpp-frame-display-integration.md)과 [Graph 활성 통합·다리 가상 삭제](cpp-surface-graph-integration.md)를 완료했다. 다음은 raw GameWorld/GUI·Kernel 게임 루프 연결이다. 아래 설명은 이 문서 작성 당시의 인계다.

`SquidFrame` 표시 콜백을 실제 `SquidDisplay::Update`에 연결하고 실제 SHP 크기를 공급한다. 이어 Graph 활성 상태에서 끝 칸 생성/삭제/Pop·받침 생성을 통합한 뒤 raw GameWorld/GUI와 Kernel 게임 루프를 연결한다. 장시간 변이 검사는 기존 인수인계대로 별도 작업이다.

근거: [다리 삭제 훅](cpp-bridgeeffects-reconstruction.md), [다리 이벤트](cpp-bridgeevent-reconstruction.md), [연결/소유자 전파](cpp-bridgeconnect-reconstruction.md), [섬 삭제 훅](cpp-islandlifecycle-reconstruction.md), [noIsland 최초 등록](cpp-islandpostpop-reconstruction.md), [객체 부착 프로세스](cpp-process-reconstruction.md).
