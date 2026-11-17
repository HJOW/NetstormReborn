# dotnetpj 0.590 — 펜스 재생성 본체와 수명 실제 이웃 결선

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-1 잔여 중 펜스 재생성 본체를 이식하고 수명 장치를 실제 본체에 결선했다. 원문 포팅·장기 결선·finder 연결을 잇는다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

원문 `o/RawFenceRebuild` 본체를 직역했다. 부모 extra 9 진입 검사·네 방향 0/2/4/6 순서·동맹 최근접 이웃·사거리 판정·대상 extra 0x21 건너뜀·표식 생성 뒤 현재 owner 재조회·포스 필드 생성을 원문 순서대로 둔다. 끝점 좌표는 형식화한 사본을 보관하지 않고 각 호출 직전에 현재 raw(+14/+18)에서 읽는다. 비정렬 DWORD·float 비트 조립을 원문대로 보존한다.

장기 결선은 같은 장기 Pool·이웃·사거리에 잇는다. 이웃은 `FenceNeighbors().Neighbor`의 동맹 검색, 사거리는 `FenceNeighbors().WithinRange`의 현재 판정을 공유한다. 표식·포스 필드 생성(`FenceMarkerCreate`·`FenceShieldCreate` 미이식)은 호스트 공급 장치(`BindFenceRebuildDevices`)로 빌린다.

수명 실제 결선(`BindFenceLifetimeActuals`)은 수명 장치 묶음의 네 실제 자리(Neighbor·ClearMarkers·ClearLine·Rebuild)를 같은 저장소의 `FenceNeighbors`·`FenceMarkerClear`·`FenceLine`·`FenceRebuild` 본체로 채운다. 동맹 읽기·재생성 생성 장치가 먼저 연결되어 있어야 하며, 수명 본체 생성 전 한 번만 호출한다. `FenceLifetime` 본체의 지연 조회 구조 덕분에 결선 뒤 생성되는 수명 본체가 실제 본체를 그대로 쓴다.

## 검증

최종 **Core 신규 7검사** 통과다. 판본·효과 누락 거부·차단 부모 건너뜀·네 방향 순서·표식→포스 필드 순서와 현재 owner·사거리 밖/차단 대상 건너뜀·호출 때 빠진 효과 명시 실패·저장소 재사용과 실제 이웃 사용·수명 실제 결선 순서·재사용·풀 공유를 확인한다. 원본 fixture 기대값은 바꾸지 않았다.

집중 447/447(`Fence|CppPortMap|Surface` 필터: 신규 7 포함) 통과·Assets 366/366 통과·Release 빌드 0경고·0오류다. Core 전체 회귀는 시간이 걸려 이번 단위에서는 돌리지 않았으며, 마지막 전체 측정(0.589 직전 2842/2846)에서 PortMap 중복 해소 뒤 자료 부재 3건(Burning originalCD 2건·PlayerTechnology NETSTORM_DATA 1건)만 남았다. Client 전체의 11건 실패는 0.589에서 작업 트리를 치워도 동일함을 확인한 사전 존재(환경) 실패다. L2는 HostSession 미결선으로 덤프 불변이므로 재수집하지 않았다. **일치 0/34·데모 dn 3 미연결** 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP은 1013행(L1 445·port 563)이며, 신규 3행(`RawFenceRebuild.h/.cpp`→`RawFenceRebuild.cs`, `RawSurfaceFence.cpp`→`RawSurfaceStorage.FenceRebuild.cs`)을 더했다. 대응 cpp 단위는 435다. fixture 연결은 없어 319개를 유지한다. 다음은 개별 삭제 몸체·HostSession 별도 풀 전환·피해/해방 상위 결선이다.
