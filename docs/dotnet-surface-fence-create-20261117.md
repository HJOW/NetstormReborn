# dotnetpj 0.591 — 펜스 생성 몸체와 재생성 실제 생성 결선

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-1 잔여 중 펜스 생성 몸체를 이식하고 재생성 생성 장치를 실제 본체에 결선했다. 원문 포팅·장기 결선·finder 연결을 잇는다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

원문 `o/RawFenceCreate` 두 본체를 직역했다. 표식(`RawFenceMarkerCreate`)은 방향 0/2/4/6 선택(동률 세로 우선)·들어오는 끝점만 clamp한 inclusive 사각형·4칸 버킷 변경 때만 기록하는 지점 생성을 보존하고, 생성 때마다 QA WORD(부모)·표면 통지·packed QB 태그(이웃 high8/low8 분리)·현재 owner·Pop(flags 0x40) 순서를 지킨다. 포스 필드(`RawFenceShieldCreate`)는 mode0과 같은 끝점 보정으로 정수 절삭 없이 float 지점을 그대로 Pop에 전달하며(100회 상한), 생성 직후 extra bit20이 서 있으면 즉시 종료하고, 현재 TYPE의 J/K 구간 첫 프레임을 직접 프레임 DWORD에 쓴 뒤 signed SID 비교로 owner를 고른다. 비유한 좌표·판본·표 크기·빠진 효과는 명시 거부한다. `RawGunTarget` 직접 인자 생성자는 DN 대응 타입이 없어 중심 함수 결합만 두며 계약은 유지한다(0.589 정리 본체와 같은 경계).

장기 결선은 같은 장기 Pool·TYPE·프레임·역할에 잇는다. 표식 중심은 `FenceNeighbors().Center`와 같은 getter를 공유하고, 생성·소유자·Pop·통지는 실제 저장소(`CreateAsset`·`SetAssetOwner`·`PopAsset`·`BuildingExecutor().NotifySurface`)에 연결한다. 재생성 실제 생성 결선(`BindFenceRebuildActualCreations`)은 재생성 장치 묶음의 두 자리(Markers·Shields)를 `FenceMarkers().Create`·`FenceShields().Create`로 채우며, 0.590의 수명 실제 결선이 요구하는 생성 장치 자리를 실제 본체로 메운다.

## 검증

최종 **Core 신규 7검사** 통과다. 판본·표·효과 누락 거부·표식 방향/경계/버킷·QA/태그/owner/Pop 순서와 새 SID 누락 명시 실패·포스 필드 100 상한·J/K 첫 프레임 쓰기·signed SID owner 선택·bit20 즉시 종료·TYPE 범위·저장소 재사용과 역할 공유·재생성 실제 생성 결선 순서 강제를 확인한다. 원본 fixture 기대값은 바꾸지 않았다. 포팅 과정에서 owner 선택 조건(`secondId < firstId`일 때 첫 owner)의 테스트 기대값을 원문으로 교정했다.

집중 454/454(`Fence|CppPortMap|Surface` 필터: 신규 7 포함) 통과·Assets 366/366 통과·Release 빌드 0경고·0오류다. Core 전체 회귀는 시간이 걸려 이번 단위에서는 돌리지 않았으며, 마지막 전체 측정(0.589 직전 2842/2846)에서 PortMap 중복 해소 뒤 자료 부재 3건(Burning originalCD 2건·PlayerTechnology NETSTORM_DATA 1건)만 남았다. Client 전체의 11건 실패는 0.589에서 작업 트리를 치워도 동일함을 확인한 사전 존재(환경) 실패다. L2는 HostSession 미결선으로 덤프 불변이므로 재수집하지 않았다. **일치 0/34·데모 dn 3 미연결** 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP은 1016행(L1 445·port 566)이며, 신규 3행(`RawFenceCreate.h/.cpp`→`RawFenceCreate.cs`, `RawSurfaceFence.cpp`→`RawSurfaceStorage.FenceCreate.cs`)을 더했다. 대응 cpp 단위는 436다. fixture 연결은 없어 319개를 유지한다. 다음은 개별 삭제 몸체(FencePostPop·FenceRegular)·HostSession 별도 풀 전환·피해/해방 상위 결선이다.
