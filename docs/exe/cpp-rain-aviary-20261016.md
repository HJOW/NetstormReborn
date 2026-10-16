# cpppj 맨 오 워 풀의 후처리·생산 프로세스 복원 — 2026-10-16

현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-16**이다. 원본10.78의 TYPE143·`rainAviary`·표시 이름 **Man o'War Pool/맨 오 워 풀**을 대상으로 한다. 앞선 데모1 `DemoStormRages`의 Load 실패에서 이어 작업했다. 원본 게임/GUI 실행과 보호 원본 수정은 없다. DirectDraw 오류 수정은 AGENTS.md의2차 목표이며 현재1차 복원 범위에서는 제외한다.

## 복원한 원문

| 원문 주소 | 제품 동작 |
| --- | --- |
| 0045ed10 | 서버·편집/친군 사격·최초 플래그 관문, 기존 프로세스 조회/진단599, new48·생성·타입60/flags8 부착 뒤 Damageable 후처리 |
| 0045d810 | outer48BYTE/inner+32의 생성자 쓰기. 비서버 상태와 미정 padding/마지막 DWORD를 보존 |
| 0045dde0 | 별도 AP 상태 기계: 시작→2, 프레임11 뒤6, 생성 뒤5, 기한이 된10→2 |
| 0045d870→0045b2a0 | 실제 inner 인자 선택과 **메시지0x73·버전1** 상태 writer |

같은 계열의 휘리베이스 프로세스32를 대신 부착하지 않는다. `RawRainAviaryProcess.*`는 별도 타입60·생성자/Frame/writer이며 현재 raw 부모·Form·시각·서버/편집/친군 사격 값을 읽는다. 상태1의 시작 프레임은0, 사운드는 `manOWarBuild.wav`다. 상태2는0.20000000298023224초 간격으로 프레임을 올리고,11을 넘으면 상태6으로 간다.

상태6의 서버 분기는 실제 rainFlyer144를 생성하고 소유자/양방향 QA·현재 부모를 기록한 뒤 **초기 부모 좌표에서(-0.5,-2)**로 Pop하고 BaseFlyer 연결을 요청한다. 이 분기 뒤에는 비서버도 상태5/프레임0/현재 시각+3초로 갱신한다. 휘리베이스의 상태3·재전송 동작과 다르다. 상태3/4/5/7/8/9의 Frame은 원문처럼 별도 진행을 하지 않는다.

원본 패킷의0x73은0053278e와 실제 등록0045e5d0에서 확인했다. 휘리베이스의0x38로 바꾸지 않는다. 전체 dirty BYTE/부모 포함/force 조합과 미사용 출력 버퍼를 보존한다. 프로세스 수신기 전체와 멀티플레이 전송 복원은 이번 범위가 아니다.

## 같은 실제 세션 연결

`RawAssetPostPop`의 VT005099e8은 별도 Rain hook으로 분배하며 같은 세션의 Carrier/Damageable→공통 PostPop을 정확히 한 번 실행한다. 현재 fort는 `core.ownerMode.fort`, 친군 사격0055a490은 기존 `gunTargetState.friendlyFire`를 공유한다. 별도 색/Player/친군 사격 표를 만들지 않는다.

각 실제48BYTE 입력을 원문 생성자로 초기화하고 `RainAviaryProcess`를 같은 `SquidProcessHost`/Kernel의 타입60·flags8로 부착한다. 실제 Host의 Form/Parent를 같은 원문 이미지에 반영한 뒤0x73 writer와 기존 BattleBroadcast/장치 송신을 사용한다. 현재 writer 분배는 휘리베이스32와 맨 오 워 풀60의 서로 다른 패킷 버퍼를 구분한다.

데모1의 실제 원본 Fort를 로드하는 검사는 저장 맨 오 워 풀이 존재하고 각 객체에 실제 타입60 Form/같은 Kernel 객체가 붙으며 부모·Form·초기 상태1·송신 후 dirty0·후처리 깊이0임을 확인한다. 이후 기존 실제 Start를 호출한다. 장면을 간략한 대체 맵으로 바꾸지 않았다.

## 원본 대조와 검사

`tools/decomp_rain_aviary.py`는 원본9함수와 실제 타이머/QA/부모/첫 Process 조회를 실행한다. **1602개 고유 입력**은 생성18·후처리386·Frame174·writer1024다. 두 x87 정밀도 재실행은 동일하며 고유 입력으로 중복 합산하지 않는다. 전체48BYTE, 객체5개, 현재28BYTE 전역,64BYTE 패킷·반환 길이·호출 순서, 스택/비휘발/허용 밖 쓰기를 확인한다.

힙/부착/Damageable/프레임 지정/장치/플라이어 생성·소유자·Pop·BaseFlyer는 명시 외부 경계다. 개발자 Frame 진단은 원본 관찰에서 껐고 제품의 필수 경계 검사에서 별도로 누락 실패를 확인한다. 큰 하위 몸체까지 같은1602개에서 원본 실행했다고 주장하지 않는다.

집중 `RainAviary_`5개가1602행과 필수 경계를 대조해 통과했다. 실제 `CampaignDemoStart_`3개도 통과했다. 최종 Release 경고0·오류0, 전체 CTest **1/1·exit0, 내부1340개·실패0**, 검사131.57초·전체131.72초다. 기존8미션과 실제데모3개의 같은 Script/Session/Start 검사를 모두 포함한다. 최종 검증 소스는 재빌드하고 링크했다.

새1602입력과 인접 휘리베이스1734개의 SHA·Python 구문·UTF-8/문서 링크/공백을 확인했다. CMake 등록의 이전/현재 SHA는 요구에너지 보고서에 보존해 기존2352입력 감사도 확인한다. 상세 결과/범위는 `cpppj/rain-aviary.json`과 같은 단위의 `LEFT_JOBS.md`를 따른다. 로그는 `extracted/goal-rain-aviary-20261016-{native,build,focused,demo,final-build,ctest,tests,adjacent-replay}.log`다.

인접 `decomp_assetpostpop_oracle.py --verify`는 과거 보고서가 옛 제품/검사5파일과 CRLF2파일을 고정해 현재 트리에서 실패한다. 이 기존 보고서의 SHA를 현재 값으로 덮어쓰지 않았다. 원본PE/도구/선택 내보내기 바이트는 일치한다. 대신 같은 기존1636입력을 `extracted/rain-aviary-adjacent-20261016/`에 원본으로 다시 실행해 **현재 fixture와 줄바꿈 정규화 후 전체 바이트가 같음**을 확인했다. 이는 새 고유 입력이 아닌 기존 기대값 재검증이다. 과거 제품 SHA 감사와 현재 제품 C++ 비교를 구분한다.

## 다음 실제 진행 경계

BaseFlyer0045c1b0은 여전히 선택 시 명시 오류인 미복원 경계다. 실제 플라이어의 생성/이동/공격·귀환, 데빌 메이커, 썬 바리케이드 공격·Regular, AI 전략 및 GUI Start/StartDemo·Guide·미션 프레임 연결이 남는다. AP5에서의 생산 재시작 상위 호출자도 이어서 연결해야 한다. 데모3개 Load/Start 통과를 데모 자연 재생/전투/승패·지정 미션 자연 완주의 완료로 간주하지 않는다. 전체 목표는 active·미달성이다.
