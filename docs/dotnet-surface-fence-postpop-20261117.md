# dotnetpj 0.592 — 펜스 후처리·정기 처리 본체와 저장소 결선

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-1 잔여 중 펜스 후처리·정기 처리 본체를 이식하고 저장소에 결선했다. 원문 포팅·장기 결선·finder 연결을 잇는다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

원문 `o/RawFencePostPop` 본체를 직역했다. 가상 표 00509470 확인·flags 1 접두(주제 3일 때 사건 0 예약·네 방향 이웃 스냅샷·owner 캐시(+2a) 기록·두 쌍 정리와 사건 3 예약·연결 수(+2e) 기록과 사건 2 예약)·모든 flags의 Damageable 마무리를 순서대로 보존한다. 각 예약의 독립 실패는 해당 생성자만 생략하고, `validateBase`가 비었으면 검사를 건너뛰며, 투영 위치는 공통 장부 검사에 그대로 전달한다. 0.1f payload는 비트(0x3dcccccd) 그대로 둔다.

원문 `o/RawFenceRegular` 본체를 직역했다. 가상 표 확인·bit20 우선 Damageable 위임·사건 0(프레임 전진·owner 캐시 비교 정리·연결 수 이중 조회와 기록·사건 2 예약)·사건 1 갱신·사건 2 재생성·사건 3(owner 캐시 보존 정리와 빈 x87 NaN 0xffc00000 반환)·그 밖 사건 Damageable 위임을 순서대로 보존한다. 프레임 전진은 `RawPathAnimation.Direction`과 J/K 구간 `Run().Count`로 계산하고, C++ idiv 검사는 `DivideByZeroException`(0 제수)·`OverflowException`(MinValue/-1)으로 옮겼다. `Handler` 분배기는 맡는 표만 처리하고 나머지는 fallback에 넘긴다.

장기 결선은 같은 장기 Pool·TYPE·프레임·지정기에 잇는다. 후처리의 주제는 TYPE+98 원소 번호, 이웃·연결 수는 `FenceNeighbors`, 연결선 삭제는 `FenceLine`, 사전 확인은 `PostPopExecutor().ValidateBase`, 마무리는 `ResourceCarrierPostPop().DamageablePostPop`에 연결한다. 정기 처리의 표식 삭제·연결선 삭제·갱신·재생성은 `FenceMarkerClear`·`FenceLine`·`FenceLifetime().Refresh`·`FenceRebuild().Rebuild`에, 프레임 지정은 `FrameExecutor()`에 연결한다. Damageable Handle은 R6 손상 정기 라우팅이 아직 없으므로 호스트 공급 장치(`BindFenceRegularDevices`)로 빌린다.

## 검증

최종 **Core 신규 10검사** 통과다. 판본·표·지정기 계약 거부·가상 표 확인·Validate 거부와 투영 전달·flags 0 접두 생략과 빠진 Damageable 명시 실패·flags 1 접두 전체 순서와 +2a/+2e 기록·bit20 위임·사건 1/2/3 분기와 빈 x87 NaN·사건 0 프레임 전진과 이중 조회·TYPE 범위·fallback 분배·저장소 재사용과 장치 순서 강제를 확인한다. 원본 fixture 기대값은 바꾸지 않았다. 포팅 과정에서 flags 1 접두의 예약(주제 사건 0)이 이웃 조회보다 앞섬을 원문 순서로 교정했다.

집중 464/464(`Fence|CppPortMap|Surface` 필터: 신규 10 포함) 통과·Assets 366/366 통과·Release 빌드 0경고·0오류다. Core 전체 회귀는 시간이 걸려 이번 단위에서는 돌리지 않았으며, 마지막 전체 측정(0.589 직전 2842/2846)에서 PortMap 중복 해소 뒤 자료 부재 3건(Burning originalCD 2건·PlayerTechnology NETSTORM_DATA 1건)만 남았다. Client 전체의 11건 실패는 0.589에서 작업 트리를 치워도 동일함을 확인한 사전 존재(환경) 실패다. L2는 HostSession 미결선으로 덤프 불변이므로 재수집하지 않았다. **일치 0/34·데모 dn 3 미연결** 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP은 1021행(L1 445·port 571)이며, 신규 5행(`RawFencePostPop.h/.cpp`→`RawFencePostPop.cs`, `RawFenceRegular.h/.cpp`→`RawFenceRegular.cs`, `RawSurfaceFence.cpp`→`RawSurfaceStorage.FencePostPop.cs`)을 더했다. 대응 cpp 단위는 438다. fixture 연결은 없어 319개를 유지한다. 다음은 HostSession 별도 풀 전환·피해/해방 상위 결선이다.
