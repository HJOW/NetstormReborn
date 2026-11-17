# dotnetpj 0.597 — 저장소 공장 전환(세션 공장→저장소 공장)

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-1 잔여 중 저장소 공장 전환을 했다. 미션 표를 넣은 저장소의 장기 공장을 세션 장치 빌더에 그대로 쓴다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

`HostSession.BuildTerrainPopDevices`·`BuildFortObjectDevices`에 선택적 외부 공장 인자를 두었다. 넘기면 같은 풀 소속인지 확인하고 세션 전용 공장 대신 그대로 쓰며, 소유자의 바인딩(세션 HP·진단)은 다시 얹지 않는다. `ClientHostGame.StartMission`은 표 설치 뒤 두 장치를 `storage.FactoryExecutor()`로 통일한다. 저장소 공장은 표시·전송 장치 없이도 구성되며(전송 Form 부착만 조건부로 건너뜀), 미션 표(0.596 설치)를 읽는다.

## 검증

최종 **Client 신규 3검사** 통과다. 실측 thewarbegins live 장면(지형 생성+요새 객체 적재) 대조로 저장소 공장 전환 전후의 풀 바이트·난수·사용 슬롯이 완전히 같음을 확인했고, 저장소 표 ZOrder 변이 장면은 바이트가 달라져 빌린 공장이 실제 생성을 주도함을 확인했다. 다른 풀 공장 결합은 거부한다.

집중 Client(HostSession·SessionStateDump·HostWorldTypeFill·PoolBorrow·PoolTransition·StorageTables)23/23 통과·Core(Fence|CppPortMap|Surface|Dais) 470/470 통과·Assets 366/366 통과·Release 빌드 0경고·0오류다. Client 전체는 1232/1243이며 실패 11건은 기존과 같은 사전 존재(환경) 집합이다. Core 전체 회귀는 시간이 걸려 이번 단위에서는 돌리지 않았다(마지막 전체 측정 0.589 직전 2842/2846에서 PortMap 중복 해소 뒤 자료 부재 3건만 남음). L2는 호출자 결선 뒤에도 덤프 불변이므로 재수집하지 않았다. **일치 0/34·데모 dn 3 미연결** 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP은 1021행(L1 445·port 571) 유지다. 이번 단위는 이미 대응된 파일(`HostSession.cs` 범위·`ClientHostGame.cs`)의 빌더만 바꿔 행을 더하지 않았다. 대응 cpp 단위 438·fixture 319개를 유지한다. 다음은 실행기 일원화(장기 Host·Display·Unpop 결선)다.
