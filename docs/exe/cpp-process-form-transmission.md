# ProcessForm 서버 부착과 같은 raw 월드 연결

원본 10.78의 `004b1440`과 CD의 `004af380`은 서버 역할에서 프로세스의 `WriteData(buffer, parentChanged, 0)`를 **부모 체인을 변경하기 전에** 호출한다. 양수 길이는 공유 버퍼 송신 함수(`004e2e30`/`004c0420`)로 넘긴다. 음수 길이는 송신하지 않는다. 이후 `flags|0x40`으로 공통 부착과 Form Pop을 실행하여 공통 Form 전송을 억제한다. 길이 0 및 2047 초과의 원본 assert 경로는 호스트 계약 밖이다.

`SquidProcessHost`에 `ProcessFormTransmission`을 연결했다. flags0/0x80 부착을 임의의 로컬 flags로 바꾸지 않으며, 직렬화/송신 경계가 없으면 등록 전에 거부한다. 실제 생성은 Kernel 등록→Form 생성/Take→타입/PID→파생 WriteData/버퍼 송신→부모 체인→공통 Pop→OnAttached 순서다. `RefreshForm`은 `0041c010`/`0048f8d0`의 현재 raw 부모 조회→Unpop(0)→같은 부모 Attach(flags0)를 수행하며 OnAttached를 재호출하지 않는다. 이미 소유한 live Form 재수신과 구별한다.

초기 구현의 직접 Form Pop 전송은 실제 PE 실행 중 프로세스 vtable+8 진입을 통해 잘못된 해석임을 확인하여 교정했다. 초기 진단 로그는 `extracted/process-transmit-native-smoke*.log`에 보존했다. 기존 ProcessForm/Kernel/재수신 fixture와 생성기는 덮어쓰지 않았다.

`RawSurfaceWorld::Core`는 실제 pool/hash/spot/type/frame/shape/장부/생성/소유자/표시/Graph/삭제/Form/ProcessHost/Kernel의 참조를 제공한다. `BindRuntime`은 첫 객체 생성 전에 현재 자산/Regular/프로세스 전송/낙하/로딩 깊이 경계를 설치한다. 기존 Form 훅도 현재 선택과 자산 경계를 읽는다. `BeginLoad`와 `FinishLoad` 사이에서 실제 Fort 순서대로 생성·필드 저장·Pop할 수 있다. 외부 연결 모듈 해제 전에 `ClearProcesses`로 예약 객체를 먼저 해제해야 한다.

독립 실행기 `tools/decomp_processtransmit_oracle.py`는 각 PE의 서버/클라이언트, 신규/지정 free, 잔여 상태, 이웃 Form/자산, flags, 프로세스 타입, 양수/음수 WriteData의 입력을 만든다. 생성자/Allocate/Take/부착/공통 Pop/갱신은 실제 명령을 실행한다. 파생 WriteData·송신·OnAttached는 그 순간의 raw 바이트와 순서를 관찰하는 경계다. Kernel은 판본별 대표 입력을 두 x87 정밀도에서 실제 실행하고, 나머지 교차 입력은 번호1 응답 경계와 기존 독립 Kernel 근거를 사용한다. C++ 재생에서는 모든 입력의 실제 Kernel 등록을 실행한다. 게임·창·OS 호출은 없다.

전송의 고유 입력은 **376개**(두 PE 각각 94개 입력×부착/갱신 두 연산)이며 독립 SHA/정상 반환 감사와 C++ 두 재생 검사가 통과했다. 같은 월드 집중6개·기존 재수신5개도 통과했다. 새 Fort/후처리/Gun/Path와 함께 **전체928개·실패0**, CTest88.83초, 최종 Release 경고0/오류0이다. 근거는 `campaign-process.json`과 `tools/analyze_campaign_process.py --verify`에 보존한다. 마지막 디컴파일 수행 PC는 **VM-W11-CODEX, 2026-10-13**이다.

이 연결은 실제 캠페인 플레이 완료를 뜻하지 않는다. 현재 GameWorld의 비표면 객체·이동은 기존 어댑터에 남아 있으며, 실제 Fort 로더/Gun/Path·건설/전투/사제 운반·의식/승패를 같은 core에 이관해야 한다. DirectDraw 전체화면 수정과 dotnetpj 작업은 이번 범위에서 제외했다.
