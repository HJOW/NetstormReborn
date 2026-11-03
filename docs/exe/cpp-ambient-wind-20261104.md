# Client 바람 갱신과 현재 연소 입자 owner

단위821을 완료했다. 제품은 **821 / 표시11.01**이며 마지막 전체 정밀 디컴파일/현재 호스트는 **HJOW-Athlon / 2026-11-03**이다. AGENTS.md·LEFT_JOBS.md·dotnet 인계를 읽었고 원본/기존 fixture를 변경하지 않았다. 실제 게임/GUI/포획/희생/특정 적 공격/승리·MP·공중3종 전용·사용자 직접 확인 검사는 수행하지 않았다.

패치004dd2b0(썽크004dd330)/CD004503f0의 실제 전역 최초값은X0/Y1/angle0이다. 두 감쇠는 float0.9가 아니라 보호 PE의 **double0.9**를 곱해 float로 저장한다. 같은 GameRandom.Next(100)을 한 번 소비하고 결과<10일 때만65536난수와 float±0.06 범위의 잡음을 소비한다. X를 저장한 뒤 fpatan(-y,x)·원문double 도 계수·현재 Boardcoord 각도 부모를 거쳐 angle을 float로 저장한다. 잡음 없는 경우 angle과 두 번째 난수는 보존한다. 각도 세 직접caller0041ce20/00467c80/004dd2b0를 완료해 checking은 **23=부모15+부분5+MP3**이다.

실제 Client439af0의 Movie 갱신 잠금<=0 가지에서 같은 ClientRandomGlobals.Game을 사용한다. 게임 시계 정지/메뉴 여부로 임의 생략하거나 delta로 감쇠를 스케일하지 않는다. 장기 RawSurfaceStorage.wind는 World Acquire/반납으로 재시드/초기화하지 않는다. 실제 Session→Particles.BindWind→BurningFrameState.sharedWind→입자별 현재 가속 읽기가 같은 owner를 사용한다. 독립 입력은 기존 wind 값형 계약을 유지한다.

실제 x86 CPU에서 양판본1920행과 재관측1920행이 일치했다. 각 관측은421632회 원문 부모 호출이며1920개의 최종 x/y/angle binary32·게임난수·checking/진단을 비교한다. 시드0/55/상위비트 등8종·1/2/7/64/1024프레임·작은/큰 유한x/y·미갱신angle의NaN 비트 보존을 포함한다. Native 진단은0이며 전체 checking DWORD는 보존된다. golden SHA-256은 `8d653588b6e676c85f7cd292677699df4a3702bb6ab268b5697caea19b04c999`이다. 원문 바이트는 기존 CPU 프로브의 전용.payload만 사용하며 원본 시작/GUI/IAT 초기화는 실행하지 않는다.

집중 C++17개(새3)가 실패0으로 통과했다: 바람3/기존연소4/각도3/World7. 새로운 현재 바람 검사는 처리기 생성 뒤 owner를 바꾸고 실제 양판본 연소 전체 출력426행과 대조한다. 기존 연소426행과 각도1088행도 통과했다. World 두 번 대여/반납 뒤 같은 바람 주소/현재값·동일 난수의 연속1/2프레임을 실제CPU 결과로 확인했다. Release1회·x86 프로브1회는 종료0·경고/오류0이다. 최초 제품/Native 검사 실패는 없고 통계용 Python 괄호 오류1회만 수정했다.

새로 확인한 근접 공백은 **004180f0 주변음/날씨 scheduler**다. 원문은 Wind 앞에서 wallclock 기한과 현재 종류/설정을 읽고 같은 게임 난수를 소비한 뒤 thunderQuietDistant/eagleScream/whale·선택 flash 요청을 한다. 현재Client에는 아직 없으므로 전체Client 난수 소비 순서 완료를 주장하지 않는다. 다음822에서 이 실제 부모/호출 순서를 연결한다.004182c0의 monitorFormat 시간 메시지도 별도 UI/Console 잔여다.

기존2747/fixture927개는 불변이고 새 CPU fixture1개 포함2748/928개다. 원본/기존 기대값 변경0이며 [구조화 보고서](../../cpppj/ambient-wind-20261104.json)에 소스/Native/검사/보호 해시가 있다. EXE SHA-256은 `897a5603557980ed7931fe62e7bd230314abc5a3962a0a7f80f5ea2a6207327f`다. 원시는`extracted/cpp-resume-821/`이다. x87 관측은0x027f/53비트이며 전체64비트 확장 정밀도/비유한 CRT 오류·OS/FILE/SEH·물리/PCM을 확인한 것으로 확대하지 않는다. 전체CTest/71대상/34미션/배포ZIP은 실행하지 않았다. 전체 목표는 미완료다.
