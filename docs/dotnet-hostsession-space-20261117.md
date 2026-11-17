# dotnetpj 0.598 — 저장소 공간 공유(해시·점유지 일원화)

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-1 잔여 중 공간 해시·점유지를 저장소와 공유했다. 풀(0.595)·공장(0.597)에 이어 세션 수명 일원화를 공간에까지 잇는다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

`HostSession.BuildTerrainPopDevices`에 선택적 외부 해시·점유지 인자를 두었다. 넘기면 쌍으로 빌려 세션 전용 생성을 없애고, 요새 장치(`_fortPop`·`_fortBuilding`·그래프 재구성)가 같은 필드를 공유하므로 자동으로 따라간다. 크기가 다르면 거부한다. `ClientHostGame.StartMission`은 `storage.Hash`·`storage.Spots`를 넘긴다. 해시는 풀 비종속 공간 색인이라 풀 검사가 필요 없고, 공장(풀 검사 유지)과 같은 호출 순서를 쓴다.

저장소 공간 실행기(`Building`·`Unpop`·`Pop`·`Display`·`Graph`)의 직접 전환은 아직 하지 않는다. 저장소 해시·점유지가 이번 단위에서야 live 상태가 되므로, 그 실행기들의 결선은 공간이 살아 있음을 전제로 다음 단위에서 잇는다.

## 검증

최종 **Client 신규 2검사**(`HostSessionPoolTransitionTests`에 추가) 통과다. 실측 thewarbegins live 장면으로 풀·공장·공간을 모두 저장소에서 빌린 장면이 별도 장면과 풀 바이트·난수·사용 슬롯이 완전히 같음을 확인했고, 해시·점유지 한쪽만 빌리거나 크기가 다르면 거부함을 확인했다. 저장소 풀·공장·공간 장면끼리는 앞 단위에서 결정성을 확인했다.

집중 Client(HostSession 계열)25/25 통과·Core(Fence|CppPortMap|Surface|Dais) 470/470 통과·Assets 366/366 통과·Release 빌드 0경고·0오류다. Client 전체는 1234/1245이며 실패 11건은 기존과 같은 사전 존재(환경) 집합이고, Core 전체 회귀는 시간이 걸려 이번 단위에서는 돌리지 않았다(마지막 전체 측정 0.589 직전 2842/2846에서 PortMap 중복 해소 뒤 자료 부재 3건만 남음). L2는 호출자 결선 뒤에도 덤프 불변이므로 재수집하지 않았다. **일치 0/34·데모 dn 3 미연결** 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP은 1021행(L1 445·port 571) 유지다. 이번 단위는 이미 대응된 파일(`HostSession.cs`·`ClientHostGame.cs` 범위)의 빌더만 바꿔 행을 더하지 않았다. 대응 cpp 단위 438·fixture 319개를 유지한다. 다음은 공간 실행기 일원화(장기 Building·Unpop·Pop·Display·Graph 결선)다.
