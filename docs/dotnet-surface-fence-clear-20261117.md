# dotnetpj 0.589 — 펜스 연결선·표식 정리 본체와 장기 결선

2026-11-17, HJOW-Athlon. cpppj 제품 1314 기준에서 R4-1 잔여 중 개별 삭제 몸체의 펜스 정리 효과(`RawFenceLine`·`RawFenceMarkerClear`·장기 결선)를 이식했다. 원문 포팅·장기 결선·finder 연결을 잇는다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

원문 `o/RawFenceLine` 본체를 직역했다. 수직(J 0x4a)/수평(K 0x4b) 판별·100회 상한·포함 끝점·CRT 절삭(`signed64` 낮은 32비트)·정수 검색점 보정 순서를 원문대로 둔다. 삭제는 같은 타입·현재 프레임 방향 일치 후보만 가상+10(flags 0x40)으로 보낸다.

원문 `o/RawFenceMarkerClear` 본체를 직역했다. 다른 끝점 중심을 먼저 읽고 들어오는 모서리만 1..255로 clamp한 뒤 천장에 0.9999를 더해 절삭하는 검색 영역, QA 또는 packed QB 태그가 현재 부모인 표식(TYPE 90)만 가상+10(flags 0x40)으로 지우는 선택을 원문대로 둔다. 실제 flags 1 표면 후보는 건너뛰되 삭제 뒤 체인은 같은 finder가 미리 보관한 next로 읽는다.

장기 결선은 같은 장기 Pool/Hash/TYPE/프레임에 잇는다. 연결선 TYPE은 `FenceShieldTypeBox`(170, 00541300)·표식 TYPE은 `FenceMarkerTypeBox`(90, 005411c0)의 현재 상자를 공유하고, 표식 중심은 `FenceNeighbors().Center`와 같은 getter를 공유하며, 삭제는 실제 가상 삭제(`DestroyAsset`)에 연결한다. `RawFenceMarkerClear`의 `RawGunTarget` 직접 인자 생성자는 DN에 대응 타입이 없어 두 번째 생성자(중심 함수 직접 결합)만 두며, 판본·TYPE 크기·풀 단일성 계약은 유지한다. 0.587의 `FenceLifetime` 장치 묶음(`BindFenceLifetimeDevices`)에 이 두 정리를 실제 Host 조립에서 연결하는 일은 다음 단위에 남긴다.

## 검증

최종 **Core 신규 6검사** 통과다. 생성 계약(Table 크기 거부)·검색점 보정·100회 상한·연결선 방향 선택·프레임 범위·삭제 미연결 명시 실패·표식 영역 clamp·부모 태그 선택·저장소 재사용·역할 공유를 확인한다. 원본 fixture 기대값은 바꾸지 않았다.

집중 438/438(`Fence|Surface` 필터: 신규 6 포함) 통과·`CppPortMap` 2/2 통과·Assets 366/366 통과·Release 빌드 0경고·0오류다. Client 전체는 1222/1233이며 실패 11건은 작업 트리를 치운 상태에서도 동일해 사전 존재(환경) 실패다. Core 전체 회귀는 시간이 걸려 이번 단위에서는 돌리지 않았으며, 마지막 전체(단위 직전 측정) 2842/2846의 실패 4건 중 1건(`PortMapHasUniquePairs` 중복)은 이번 단위에서 해소했다. 남은 3건은 0.571~0.587과 같은 자료 부재(Burning originalCD 2건·PlayerTechnology NETSTORM_DATA 1건)이며 TRX는 보관하지 않는다. L2는 HostSession 미결선으로 덤프 불변이므로 재수집하지 않았다. **일치 0/34·데모 dn 3 미연결** 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP은 1010행(L1 445·port 560)이며, 신규 5행(`RawFenceLine.h/.cpp`→`RawFenceLine.cs`, `RawFenceMarkerClear.h/.cpp`→`RawFenceMarkerClear.cs`, `RawSurfaceFence.cpp`→`RawSurfaceStorage.FenceClear.cs`)을 더하고 중복 2행(구 `RawGameAuxState` 구 base 1행·`RawSurfaceDamageable.cpp→Damageable.cs` 완전 중복 1행)을 뺐다. 대응 cpp 단위는 434다. fixture 연결은 없어 319개를 유지한다. 다음은 펜스 수명 실제 이웃 결선·개별 삭제 몸체·HostSession 별도 풀 전환·피해/해방 상위 결선이다.
