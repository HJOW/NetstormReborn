# 실제 파편 수명과 맨 오 워 전투 삭제 연결

2026-10-17, **HJOW-Athlon**. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**으로 일치한다. 기존10.78 PE/정밀 함수표/보호 자산을 읽었다. 새 디컴파일·원본 게임/GUI 실행은 없다. CD 10.72의 새 관찰은 이번 범위에 포함하지 않는다.

원본 `004604a0`의 세 생성자와 `004881d0`은 공통 표시 목록에 parent0으로 먼저 등록하고, TYPE110을 flags2로 생성한다. Create 뒤 현재 source 좌표로 dummy를 Pop(flags16)하고 process16을 수동 부착한 뒤 `004b1510(form,dummy)`의 상호 의존을 만든다. 세 파생 생성자는 중력6/float0.24/float1.2를 저장하며 셋째는 중간 깊이에1을 더한다. 이후 현재 source의 그림을 캡처하고 실제 `00460000`으로 준비한 뒤 **현재 source signed BYTE 깊이**를 최종 저장한다.

`RawShrapnelLifetime`은 이 순서와 bitmap→입자→공통 등록/base→flags bit0 객체 반납을 복원한다. `ShrapnelVisualSystem`은 같은 RenderProcessSystem/Host/Kernel에 실제 process16을 등록한다. 생성 완료 전 Frame/Draw 재진입을 막고, 소진은 실제 Form 삭제(flags0)에 연결한다. 의존 쌍이 완료된 dummy는 원래 Form 삭제 사슬이 처리하며 별도 조기 삭제로 순서를 바꾸지 않는다. 부착 전 실패한 dummy만 별도 소유권 정리 경계를 사용한다.

`decomp_shrapnel_lifetime.py`는 실제 wrapper·세 생성자·등록·픽셀 묶음·두 난수·타이머·소멸을 실행했다. **216고유 입력**은 빈 목록/확장/구멍, signed 깊이, Create 중 source 좌표 변경, 부착 중 현재 부모 변경, 캡처 중 source 깊이 변경과 삭제 bit0을 포함한다. new/free·dummy Create/Pop·Host 부착·감시 쌍·캡처는 명시 외부 경계다. 캡처480입력과 픽셀 계산980입력의 실제 몸체 대조와 구별한다. C++ 생성92BYTE·용량 전체·두 난수·목록·요청 순서가 원문과 일치했다. 소멸 뒤 원본의 해제 포인터/용량과 C++에서 반납한 vector 표현은 같다고 주장하지 않으며, 해당 두 표현을 제외한 필드/등록과 실제 반납 순서를 비교했다.

TYPE110의 실제 VT50eb98은 +10=`004af780`, +14=`004b0950`, +18=`004b0840`, +20=`004b0d30`, +90=`004b02d0`이다. +34는 정밀 표에서 빠진 `0048f0a0`이며 PE의 **`8b016a00ff5010c20800`**10바이트를 확인했다. 이 함수는 두 통지 인수를 무시하고 현재 가상+10(flags0)을 호출한 뒤 ret8한다. 별도9고유 입력의 두 정밀도18정상 반환도 대조했다. 주 생성/삭제와 합한 정상 반환은 **882회**, 정밀도 차이는0이다. 새 월드 dependencyNotice 경계가 이 자산 가상 통지와 기존 ProcessForm 통지를 분배한다.

캠페인에서는 TYPE144의 실제 일반 소유자 색표와 현재 owner 색 번호를 `RawShapeToBuffer`에 연결한다. 원본 `00542414`의 PE 초기값은1이고 쓰기 대입이 발견되지 않아 TYPE 기준점 보정을 켠다. 줌은 현재 클라이언트의65536이다. 다른 타입의 전용 색표/추가 가상 그림은 명시적으로 미복원 경계로 남긴다. `RawCampaignParticles`는 같은 실제8비트 Renderer clip/pitch에 캡처 색 픽셀을 직접 쓴다. `CrtRandom::Borrow`가 **기존 Session crt DWORD**를 참조하므로 `RawGameRandomSeed`의 원본 시드 초기화와 `RawTerrainStamp`의 실제 소비에 이어 같은 스트림을 사용한다. 게임 난수와 합치거나 파편용 별도 시드를 만들지 않는다. 원본의 명시 srand에 따른 상태 대입은 그대로 보존한다.

Session의144 전용 Pre는 현재+8 WORD 그림자를 먼저 가상 삭제하고 기존 Damageable Pre로 진입한다. Post는 실제 Damageable Post다. 폭발 `004605d0`은 억제/Kernel 한도를 검사하는 `0045a840`의 기존 anim160(variant1,float0.04)을 먼저 실행하고 실제 파편을 만든다. 붕괴는 variant1 파편이며 원래 collapse.wav/explosion.wav/priestFree.wav의 위치/전역 요청도 연결한다. TYPE144를 공통 Pre/Post whitelist로 처리하지 않는다.

첫 실제 연결 검사는 같은232프레임에서 부모 플라이어 프로세스의 홈 통지 미연결에 도달했다. 맨 오 워 풀 VT5099e8+34의 실제 `0045b5b0`은 서버일 때 현재 process60을 찾아 stage10/dirty6을 쓰고, 현재 시각을 먼저 저장한다. 정수 게임 난수 `[10000,15000)`를1000으로 **정수 나눗셈**한10..14초 지연을 다시 더한 뒤 현재 Form을 갱신한다. `RawRainAviaryProcess::FlyerDied`와 Session의 실제 finder/Host/RefreshForm으로 연결했다. `decomp_rain_home_notice.py`의36고유 입력·두 정밀도72정상 반환·차이0은 서버 관문/dirty/0시드·분수 시각의 전체48바이트·난수·갱신을 보존한다. finder/lookup/refresh는 명시 경계이며 정상 프로세스 NULL 역참조를 성공 입력으로 합산하지 않는다.

Release 최종 수정 빌드는 경고0·오류0·종료0이다. 원본 파편3개·기존/새 맨 오 워 풀6개·실제 파괴/픽셀/의존1개·데모1 자연400프레임1개를 합쳐 집중11개가 통과했다. 실제 파괴 검사는 자연 생성 본체를 선택하고, 현재 owner의 원본 그림 전체·한 Frame·Renderer 픽셀과1028바이트 pitch의 행 padding·Host.Kill0→Form/더미 반납·삭제 깊이0을 확인했다. 검사 UI의 선택 상태 누락은 명시 선택0으로 보완했다.

전체 CTest 내부 **1453개 모두 통과·실패0,210.49초,종료0**이다. `LastTest.log`의 실제 내부 건수와11개 목표 미션의400프레임 완료를 확인하고 `extracted/goal-shrapnel-lifetime-20261017-ctest-output.log`로 보존했다. 새216/36 및 기존980/480/2352행의 SHA 감사를 통과했다. TYPE 제품/기존 기대값은 보존했고 실제 o/tests CMake2개의 전후 핀만 이력으로 갱신했다. 새 파일 UTF-8/LF·한국어 주석/공백을 확인했다. 모든 빌드·원본 실행기·검사 핸들은 종료했으며 로그 접두사는 `extracted/goal-shrapnel-lifetime-20261017-`다.

사용자 플레이 관점의 다음 빈 부분도 현재 소스에서 확인했다. `UberGump::OpenObjectMenu`는 사제의 Construct 항목을 아직 비활성화하고, `GameWorld`의 실제 입력 명령 연결도 사제 이동에 한정된다. 건설/경제·다리·커서·원본 메뉴 입력을 실제 UI에 연결해야 한다. 캠페인1-2의 사용자가 생성하는 다른 플라이어도 `otherFrame/command/finish` 미복원 경계가 남아 있다. 400프레임 검사를 승패/저장/다음 미션·장시간 AI 전략이나 사용자 완주 증거로 합산하지 않는다. 전체 목표는 active이며 사용자 플레이·자연 완주·전체 그래픽/음향 동일성과 구별한다. 전체화면 DirectDraw는 이번1차 범위에서 제외한다.
