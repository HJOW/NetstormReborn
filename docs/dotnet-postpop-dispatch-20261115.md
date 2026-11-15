# dotnetpj 0.555 — 현재 PostPop 가상 분배

2026-11-15, HJOW-Athlon. cpppj 제품1313의 `SquidPostPop.cpp`에서 현재 이식된 파생/접두의 선택과 전체 호출 수명을 옮겼다. R4-1 장기 Storage의 실제 실행기 선행 작업이다. 새 경로는 여전히 플레이 불가이며 M1은0/34다.

## 구현

실제 VT로 다리·섬·표면·Teleport 접두와 등록한 Carrier/Damageable·전용 자산·기존 사제/Player/보호막을 분배한다. PATCH 접두는 TYPE/free를 추가 관문으로 쓰지 않으며 미선택 플래그는 장치를 요구하지 않는다. 다리는 최초 bit2를 먼저 쓰고 연결 장치를 선택한다. CD 독립 다리 접두는 +35와 기존 사전 실패를 유지한다.

현재 접두/전체 몸체를 지역 강한 참조로 선택하고, 선택된 Runtime owner는 전체 파생 호출/예외 복귀까지 KeepAlive로 유지한다. 단기 대여가 없는 전용 VT는 실제 장기 자원 분배를 요구하며 미설치를 공통 성공으로 가장하지 않는다. 실제 World는 같은 연결기의 다리/섬 접두·표면 몸체를 사용하고 프레임 callback도 실제 SquidFrame에 연결했다.

**아직 없는 Dude·Factory·Rune·Dais·Buried·Balloon·Flyer·Bomb·CarrierSteal 등의 typed 설치는 완료가 아니다.** 해당 모듈은 장기 자원 장치가 선택되는 위치에서 실제 몸체가 필요하다. 실제 Client/Session의 pin 대여와 NativeStorage 장기 실행기 보유·현재 Host 별도 풀 전환도 남아 있다. 큰 cpp/header 전체 기준을 올리지 않았다.

## 검증

신규33검사는 접두32검사(미선택 raw 생략·free/contained 첫 저장·자기 연결 반납·raw/VT 변경·부분 효과·명시 사전 검사·장기 장치 선택·Carrier/Damageable 전체 몸체·외곽 owner pin·CD 다리 오프셋)와 자산 원본818행의 실제 가상 분배 재생1검사다. 자산 fixture는 직접 호출과 가상 분배 양쪽을 그대로 대조하며 원본 기대값을 수정하지 않았다.

최초245검사 중1건은 다리의 옛 사전 실패 기대였고 원문 bit2 선행 저장에 맞춰 합성 기대를 바꿨다. 중간 빌드는 지연 중 직접 종료(MSB4166 기록) 후 단일 노드로 재개했으며 새 검사 상수명도 Stride로 바로잡았다. 후속277검사 중2건은 테스트 프레임에 JIT 임시 강한 참조가 남아 호출 뒤 수집을 가리는 검사 문제였다. 생성/호출 프레임을 NoInlining helper로 분리하고 호출 중 생존과 호출 뒤 수집 검증을 모두 유지했다. 다음283/283 통과 뒤 CD 오프셋 회귀1건을 추가했고, using 누락 컴파일 진단을 교정한 최종 **284/284** 통과다. Host Release 경고0·오류0이다. TRX는 `extracted/dotnet-postpop-dispatch-0555-initial/`·`-final2/`·`-final3/`·`-final5/`에 있다. 전체 회귀는0.546 기록을 유지한다.

L2는 동일 SHA cpp34 기준선을 재사용하고 dn31개를 새로 실행했다. 모두 종료0·수집 오류0·보호 원본1,394파일 전후 동일이다. 0.554 대비 상태 변화는 없고 파일 전체 동일14·frame행만 차이17경로다. 전체 일치0/34·데모dn3 미연결, 자료 `extracted/dotnet-l2-0555-fixed/`다. 사용자 제외 플레이와 MP 실행은 수행하지 않았다.

실제 첫 L2 실행은27경로가 실패했다. 개발 Host의 Fort 적재가 HandlesDerived를 사제/보호막 전용으로 사용하던 경계를 드러낸 결과다. 일반 표면까지 선택되자 아직 없는 접두를 호출했다. 해당 호스트는 실제 설치된 HandlesPriest/HandlesForcefield를 명시 선택하도록 바로잡았다. 일반 요새 적재의 R4-2 미완 결선은 그대로 기록하며 지원표를 줄이거나 빈 접두를 넣지 않았다. bc3acid_bar76 실자료 적재 회귀1건을 추가한 **Client 관련55/55**도 통과했다. 새 검사의 ReadOnlySpan을 Assert.NotEmpty에 넘긴 컴파일 오류는 실제 Count 검사로 고쳤다. Client TRX는 `extracted/dotnet-postpop-dispatch-0555-client-final/`이다. 첫 실패 실행/원본 감사는 `extracted/dotnet-l2-0555/`, 수정 후 새31개 실행은 `extracted/dotnet-l2-0555-fixed/`에 분리 보존했다. 위 L2 최종 결과는 수정 후 실행이다.

PORT_MAP913행(L1 431·port482), cpppj 대응415단위·fixture312개다. 분배 부분 대응2행을 더했고 기존 행/해시/순서와 floor/reflected는 유지했다. 새 cpp 계열 전체 완료로 세지 않아 R4·R6 누락 총량은 이전 단위 그대로다. 다음은 NativeStorage 장기 물리/장부 정체성과 공장·공간 실행기 보유, 실제 World 대여를 잇는다.
