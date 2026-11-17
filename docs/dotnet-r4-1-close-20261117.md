# dotnetpj 0.600 — R4-1 마감 감사와 R4-2 진입 조건

2026-11-17, HJOW-Athlon. R4-1 공통층의 완료·잔여를 코드 근거로 확정하고 R4-2 진입 조건을 적었다. 제품 코드 변경은 없다. R4 진행 중·M1 0/34·새 경로 플레이 불가는 유지한다.

## R4-1 마감 판정

공통층 이식과 장기 결선은 닫혔다. 풀(0.594·0.595)·공장(0.596·0.597)·공간(0.598)·건물(0.599) 공유, 삭제 분배 라우팅(0.573·0.576~0.584·0.593), 후처리·정기 본체(0.575·0.592·0.587~0.591), 생성 본체(0.589·0.591), 재생성·수명 실제 결선(0.590)이 live 장면 바이트 동일까지 입증됐다.

명시 잔여는 R6 장치가 필요한 것만 남는다. 근거는 코드에 있다.

- 개별 삭제 몸체 3종(`RawDaisDeletion`·`PlayerPreDestroy` 본체·`NotifyAviaryHome` 본체)은 DN에 본체 클래스가 없고(0.593 장치 경계만 둠), 필요 장치가 미션 흐름에 없다. 다이스 본체는 `SquidProcessHost.FromForm` 경유 실제 프로세스가 필요하고(`RawSurfaceWorld.cs:1550`은 월드 경로에서만 쓴다), Player 본체는 Player 기록 pin 뷰가, Aviary 본체는 썬·맨 오 워·데빌메이커 Aviary 프로세스 본체가 필요하다.
- 장기 실행기(Unpop·Pop·Display·Graph·Host)는 미션 흐름(`Netstorm.Client*`)에서 호출이 0건이다. `UnpopExecutor`는 구성 때 `DisplayExecutor`를 요구하고(`RawSurfaceStorage.Creation.cs:80`), 표시 장치가 미션 흐름에 연결된 곳이 없다. `GraphExecutor`가 쓰는 장기 Graph는 미션 저장소가 `Acquire`되지 않아 ResetTable 초기화를 거치지 않았다(미션 흐름의 `Acquire` 호출 0건).
- 장기 전송 장치(`BindTransmissionDevices`)·파생 Runtime pin도 호출자가 없어 검사에서만 연결된다.

## R4-2 진입 조건

R4-2(적재 중 postPop)의 완료 기준(덤프 owner·type·graph·totals 행 일치)은 postPop 파이프라인이 있어야 판정할 수 있다. 미션 흐름의 현재 상태는 다음과 같다.

- 지형 Pop은 장치 미연결 때 기록만 한다(`HostSession.cs:927` "Pop 장치가 없으면 기록만 한다").
- 요새 Pop은 사제·포스필드 두 몸체만 실제 결선하고 일반 적재는 R4-2 잔여로 둔다(`HostSession.cs:1091` "일반 적재는 R4-2에서 같은 장기 공간에 연결한다").
- 저장소 `PopExecutor`·`DisplayExecutor`·`GraphExecutor`는 미션 흐름에서 호출이 0건이다.

따라서 R4-2의 첫 단계는 Pop 공간 등록 switch-on·표시 장치·그래프·장부 결선이며, 그 결과물(지면·받침 소유자 스탬프·농지 타입·그래프 번호·totals 집계)을 같은 live 장면 대조로 확인한다. 메뉴 경로 복원과 L2 본 대조는 그 뒤다.

## 검증

제품 코드 변경이 없으므로 기존 증거를 유지한다. 집중 Client(HostSession 계열)27/27·Core(Fence|CppPortMap|Surface|Dais) 470/470·Assets 366/366 통과(0.599와 동일, 재실행으로 확인), Release 빌드 0경고·0오류다. Client 전체는 1236/1247이며 실패 11건은 기존과 같은 사전 존재(환경) 집합이다. L2는 `일치 0/34`, 데모 dn 3 미연결 유지다.

PORT_MAP은 1021행(L1 445·port 571) 유지, 대응 cpp 단위 438·fixture 319개를 유지한다. 다음은 R4-2 첫 단계(Pop 공간 등록 switch-on)이다.
