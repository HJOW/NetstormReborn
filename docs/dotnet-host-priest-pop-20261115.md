# dotnetpj 0.529 — 실제 호스트 사제 Pop·공유 프로세스·보호막 소리 결선

2026-11-15, HJOW-Athlon. `cpppj/src/client/RawCampaignSession.cpp`의 사제/소리 조립(1189~1230행), `RawSurfaceWorld.cpp`의 독립 typed Pop/전송 경계, `RawGameAuxState.h`의 현재 소리 역할DWORD47을 대조했다. 새 PORT_MAP 행은 `382ec33feca7a418dccd62c599abdbebb88e07e8` 기준이며 파일 전체 이식을 뜻하지 않는다. 디컴파일 산출물은 읽거나 변경하지 않았다.

## 제품 연결

`HostSession.PriestWorld`는 Player와 같은 풀·현재TYPE·프레임·공간 해시/점유 지도·장부·공장·HP·Owner·ProcessHost/Kernel에 사제/보호막 후처리를 등록한다. 같은 `SquidDisplay`를 Pop/Unpop/Frame에서 공유한다. 요새 로더는 지원하는 파생 사제/보호막을 이 실제 Pop에 전달하고, Owner와 loadingDepth도 실제 사제 상태에 전달한다. 다른 파생의 기존 미연결은 남아 있다.

실제 호스트 수명의 `RawStateSidLists.Priests`를 빌리고 독립 일반 월드 설치 때만 같은 배열을 초기화한다. Pool server와 사제 boss는 같은 현재 `ClientGameplayGlobals.Authority`를 읽는다. 회복 Form602는 기존 Player 프로세스와 같은 Kernel에 붙으며 현재 Player9·HP·실제 SP 회전/추적 주소를 읽는다. 반납은 이 Host의 프로세스만 먼저 비운 뒤 사제 등록과 Player9 대여를 해제한다. 미션 적재 도중 예외에서도 같은 순서로 정리한다.

현재 Pop에 실제 진단과 `RawSquidTransmission` writer를 연결했다. 공통 GameAux 전송 버퍼·현재TYPE HP·실제 Form 패킷 작성기를 사용한다. 독립 단일 미션의 Broadcast에서는 실제 직렬화 결과를 최근64건만 진단 보관한다. 원문 typed 독립 경로에 대응하는 것으로, 네트워크 전송을 수행했다는 뜻은 아니다. MP 대상 조회/송신은 선택 시 명시 실패한다. 진단 문장도 최근64건만 보관한다.

`HostMissionAudio`는 실제 이름 표·Windows 파일/DirectSound 장치·SoundPlayer를 소유하고, 보호막의 Sound47을 같은 ProcessHost에 부착한다. 낙하/보호막 안내도 같은 재생기를 사용한다. 역할47은 장기 GameAux 주소를 읽는다. 음소거 시 장치를 열지 않고 원문 Enabled/Initialized 관문에서 재생만 반환하므로 논리 보호막/사운드 프로세스는 실제로 생성된다. SoundProcessState는 현재 독립 typed 세션 수명이며 후속 Native 저장소 전체 조립으로 확대하지 않았다. Windows 외 장치 선택은 R8 Linux 작업이다.

## 검사와 실행에서 발견한 문제

초기 실제 실행에서 Frame의 Pop/Unpop display identity와 flags9 Pop의 전송 writer 미연결을 확인해 같은 실제 몸체로 교정했다. 일반31경로 첫 시작 검사는28성공/3실패였다. 실패 경로 bc1menu76·bc9ssun_bars76·breakingthrough는 저체력 사제가 보호막을 만들 때 소리 예약에 도달했다. 소리를 생략하지 않고 실제 Sound47 조립을 추가한 뒤 세 경로 모두 cpp/dn120프레임 정상 종료했다. 이 세 경로 L2는 여전히 불일치한다.

새 소비자 검사6개는 실제 자산으로 구성한다. 실제 thewarbegins 요새 로더의 두 사제/회복 Form, 공유 목록·장부·해시, 현재 boss/server, 회복 틱의 Player9/HP/SP 변경, 이 Host만의 반납, 음소거 저체력 사제의 보호막167·Regular1/2·실제 SoundProcess/Form47 생성과 정리를 확인한다. 검사 준비 중 Template 슬롯0으로 영역이 제외된 입력은 제품과 같은1로 교정했고, 보호막은 원문대로 Regular 두 개를 생성하므로 회복/소리와 합계4프로세스로 명시했다. 초기 변이 실행의 검사 using 누락 빌드 실패는 검출로 세지 않는다.

최종 전체 비증분 Release 빌드는 경고0·오류0이다. 솔루션 전체 회귀를 끝까지 실행해 Assets366/366·Client1224/1224 통과, Core2204중2203통과/1실패를 확인했다. 실패는 `PlayerTechnologyTests.ActualTypeBindingsAndStalledBoundary`의 필수 `NETSTORM_DATA` 누락이었다. 같은 빌드에 실제 자료 사본 루트를 지정한 재검사1/1은 통과했다. 제품/기대값은 변경하지 않았고, 재실행 오류 방지를 위해 인수인계 검증 명령에 자료 루트를 명시했다. 최초 솔루션 실행의 종료1을 숨기거나 최종 Core전수 단일 녹색 실행으로 합산하지 않는다. Client 실장치 Explicit1건은 미실행이며 일반 검사 통과 수에 넣지 않는다.

제품 요새 로더의 실제 사제 Pop 조건을 거꾸로 바꾼 대표 변이는 실제 로더 소비자1/1에서 사제 목록0으로 실패 검출했다. 원본 소스 바이트를 SHA-256 `910865004a2c3fa97a710d0f134f8c0e25c95a7a66760293cd764002f27c9515`까지 복원한 뒤 비증분 재빌드/전수 검사를 수행했다.

최종 일반31미션을 각120프레임, 기본 영어/창 모드·4:3·음소거·고정시각1760000000으로 단순 진입/대기했다. 31개 모두 종료0·world1덤프, 같은 실행 파일SHA `df27701f95a3cd153523000e10096c386d59926a08666a9e27cdac485658169b`다. originals1394파일의 이름/해시는 전후 동일하다. 임시 집계 코드가 read_dump의 행 목록을 세 값으로 unpack하려고 실패했지만 실제 종료/덤프는 유효했다. 최초 오류 집계는 collection-unpack-errors.json에 보존하고 기존31개 run.json/덤프를 재검증해 results.json을 교정했다. 재집계는 게임을 다시 실행하지 않았다.

최종 thewarbegins는1856슬롯(0.528의1854보다 회복Form2개 증가), Game2317693072·CRT296677442다. 같은 시드 cpp1941슬롯과의 공통1856·cpp_only85·dn_only0, 타입1633·프레임126·전체슬롯80·그래프240/255행 일치다. 전체 L2는 불일치다. bc1menu76·bc9ssun_bars76·breakingthrough의 별도 cpp/dn6실행도 모두 종료0이지만 비교 도구는 상태 차이 때문에 종료1이다. 최종31경로 재수집은 dn만 했으며 cpp전체31개의 새 paired 수집으로 세지 않는다.

로컬 자료는 extracted/dotnet-priest-pop-0529-final/, dotnet-l2-0529-sound/, dotnet-priest-pop-0529-mutation-fixed/, dotnet-0529-tests/다(Git제외). PORT_MAP810행(L1 387+port423), fixture278개 유지. 새 NB/외부 접속은 없다.

## 남은 경계

피해 전처리/삭제·해방·배치 모듈, 중립 사제 대피, burning 갱신, 다른 Walker 낙하, 너겟 보충, UI Tell/번역은 아직 실제 Host에 연결되지 않았다. 선택된 미연결 효과는 명시 예외이며 `ValidateReady`도 아직 실패한다. 제품 프레임 루프의 Kernel 실행/게임 시계·카메라 음향 뷰와 전체 소리 옵션 해석/변경·음악 통합은 R5/R6 잔여다. 이번에는 최초 sound/음량 설정만 연결했다. 따라서 시작 상태 연결과 별도로 단위에서 Kernel.RunFrame을 호출한 합성 검사를 실제 게임 플레이 완료로 세지 않는다. 실장치 재생은 기존 활성 출력 endpoint 부재로 미확인이다.

농지·다른 파생 postPop·AI·표시·M1 전체0/34 및 후속 단계는 잔여다. 사제 포획/희생·특정 적 공격·시나리오 승리·MP 재현은 수행하지 않았다. AGENTS.md·보호 원본/CD·README 개요·cpppj·LEFT_JOBS.md·webpj는 이 단위에서 변경하지 않았다.
