# dotnetpj 0.599 — 저장소 건물 공간 전환(실행기 일원화)

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-1 잔여 중 건물 실행기를 저장소로 통일했다. 세션 장치 빌더가 외부 건물 공간을 빌리고 호출자가 저장소 실행기를 연결한다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

`HostSession.BuildFortObjectDevices`에 선택적 외부 건물 공간 인자를 두었다. 넘기면 같은 풀·해시 소속인지 검증하고 세션 전용 생성을 없앤다. `ClientHostGame.StartMission`은 저장소 섬 타입을 세션 지형 값(97)으로 맞춘 뒤 `storage.BuildingExecutor()`를 넘긴다. 섬 타입은 `HasIsland` 판정에 쓰이므로 동기화가 필수이며, 다리 타입은 양쪽 82로 같다. 저장소 건물 실행기의 진단·다리 바인딩은 세션과 같은 값으로 둔다.

## 검증

최종 **Client 신규 2검사**(`HostSessionPoolTransitionTests`에 추가) 통과다. 실측 thewarbegins live 장면(지형 생성+요새 객체 적재)으로 풀·공장·공간·건물을 모두 저장소에서 빌린 장면이 별도 장면과 풀 바이트·난수·사용 슬롯이 완전히 같음을 확인했다. 섬 타입 선택은 `HasIsland` 직접 대조로 입증했다(97이면 참·0이면 거짓). 다른 풀·해시의 건물 공간은 거부한다. 현재 live 적재는 Pop 공간 등록을 하지 않아(R6) 섬 타입 차이가 장면에 드러나지 않으며, 동기화는 R6 Pop 등록 때 효력이 난다.

집중 Client(HostSession·SessionStateDump·HostWorldTypeFill·PoolBorrow·PoolTransition·StorageTables)27/27 통과·Core(Fence|CppPortMap|Surface|Dais) 470/470 통과·Assets 366/366 통과·Release 빌드 0경고·0오류다. Client 전체는 기존 11건 실패(사전 존재 환경 집합) 외에 통과하며, Core 전체 회귀는 시간이 걸려 이번 단위에서는 돌리지 않았다(마지막 전체 측정 0.589 직전 2842/2846에서 PortMap 중복 해소 뒤 자료 부재 3건만 남음). L2는 호출자 결선 뒤에도 덤프 불변이므로 재수집하지 않았다. **일치 0/34·데모 dn 3 미연결** 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP은 1021행(L1 445·port 571) 유지다. 이번 단위는 이미 대응된 파일(`HostSession.cs`·`ClientHostGame.cs` 범위)의 빌더만 바꿔 행을 더하지 않았다. 대응 cpp 단위 438·fixture 319개를 유지한다. 다음은 R4-2 적재 중 postPop 결선(지면·받침 소유자 스탬프·농지·장부·그래프)이다.
