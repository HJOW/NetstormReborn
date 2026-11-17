# dotnetpj 0.595 — HostSession 별도 풀 제거(호출자 결선)

2026-11-17, HJOW-Athlon. cpppj 제품1314 기준에서 R4-1 잔여 중 HostSession 별도 풀을 제거하고 호출자를 NativeStorage 풀에 결선했다. 세션 수명 일원화의 호출자 결선이며, 공장·실행기 일원화는 다음 단위다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## 구현

`ClientHostGame.StartMission`이 미션 저장소(`RawSurfaceStorage`, 경량 생성자)를 소유하고 그 풀을 `HostSession`에 빌린다. 세션이 저장소를 소유하고 월드가 대여하는 원문 구조(`RawCampaignSession.cpp` 991행·`RawSurfaceWorld.cpp` 152행)의 첫 단계다. 빌린 풀의 별도 생성 코드를 지웠으므로 개발 호스트 미션 흐름에는 별도 풀이 없다. 풀 바이트·해시·덤프 행은 별도 풀과 같다(아래 검증).

## 검증

최종 **Client 신규 2검사**(`HostSessionPoolTransitionTests`) 통과다. 실제 자료로 thewarbegins live 장면(지형 생성+요새 객체 적재)을 조립해 대조한다. 별도 풀 장면과 저장소 풀 장면의 사용 슬롯 수·풀 전체 바이트·게임 난수가 완전히 같고, 저장소 풀 장면끼리도 결정적이다. L2 전체 대조는 Windows 오라클·표시 실행이 필요한 PC에서 한다.

집중 Client(HostSession·SessionStateDump·HostWorldTypeFill)18/18 통과·Core(Fence|CppPortMap|Surface|Dais) 470/470 통과·Assets 366/366 통과·Release 빌드 0경고·0오류다. Client 전체는 1227/1238이며 실패 11건은 0.589~0.594와 같은 사전 존재(환경) 집합이다. Core 전체 회귀는 시간이 걸려 이번 단위에서는 돌리지 않았으며, 마지막 전체 측정(0.589 직전 2842/2846)에서 PortMap 중복 해소 뒤 자료 부재 3건(Burning originalCD 2건·PlayerTechnology NETSTORM_DATA 1건)만 남았다. L2는 호출자 결선 뒤에도 덤프 불변이므로 재수집하지 않았다. **일치 0/34·데모 dn 3 미연결** 유지다. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP은 1021행(L1 445·port 571) 유지다. 이번 단위는 이미 대응된 파일(`HostSession.cs` 대응 `RawSurfaceStorage.cpp`)의 생성자·호출자만 바꿔 행을 더하지 않았다. 대응 cpp 단위 438·fixture 319개를 유지한다. 다음은 공장·실행기 일원화(세션 수명 일원화 계속)다.
