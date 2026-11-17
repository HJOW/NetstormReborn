# dotnetpj 0.596 — 미션 표 저장소 설치(공장 일원화 바탕)

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-1 잔여 중 공장·실행기 일원화의 바탕을 두었다. 미션 표 3종을 장기 저장소에 독립 사본으로 넣어 저장소를 타입 보유 NativeStorage로 만든다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

`ClientHostGame.StartMission`은 미션 표 3종(타입·프레임·셰이프)을 세션 장치에 연결하던 순서 그대로, 요새 객체 장치 뒤에 `InstallMissionStorageTables`로 `_missionStorage`에 같은 입력을 넣는다. 저장소는 복제본을 소유하고(`OriginalTypeRecord.Clone`·`RiftTypeFrames` 복제·`SquidDisplayShape.Clone`), 풀 바이트는 바꾸지 않는다. 설치는 미션당 한 번이며 중복·크기 불일치를 거부한다. 검사에서 같은 입력을 직접 넣을 수 있게 공개 정적 메서드로 두었다. 저장소 공장은 전환(다음 단위) 전까지 쓰지 않는다.

## 검증

최종 **Client 신규 2검사**(`HostSessionStorageTablesTests`) 통과다. 실제 자료로 읽은 미션 표 3종의 개수 일치·풀 바이트 불변·원본 변경 뒤 저장소 사본 불변(독립 소유)·크기 불일치와 중복 설치 거부를 확인한다.

집중 Client(HostSession·SessionStateDump·HostWorldTypeFill·PoolBorrow·PoolTransition·StorageTables)20/20 통과·Core(Fence|CppPortMap|Surface|Dais) 470/470 통과·Assets 366/366 통과·Release 빌드 0경고·0오류다. Client 전체는 1229/1240이며 실패 11건은 기존과 같은 사전 존재(환경) 집합이고, Core 전체 회귀는 시간이 걸려 이번 단위에서는 돌리지 않았다(마지막 전체 측정 0.589 직전 2842/2846에서 PortMap 중복 해소 뒤 자료 부재 3건만 남음). L2는 호출자 결선 뒤에도 덤프 불변이므로 재수집하지 않았다. **일치 0/34·데모 dn 3 미연결** 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP은 1021행(L1 445·port 571) 유지다. 이번 단위는 이미 대응된 파일(`ClientHostGame.cs`·`RawSurfaceStorage.cpp` 대응 `HostSession.cs` 범위)의 호출자만 바꿔 행을 더하지 않았다. 대응 cpp 단위 438·fixture 319개를 유지한다. 다음은 저장소 공장 전환(세션 공장→저장소 공장)이다.
