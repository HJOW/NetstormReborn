# Boardcoord 현재 각도 부모와 방향 소비자

단위820을 완료했다. 현재 제품은 **820 / 표시11.01**이며 바람 caller 잔여로 checking24를 유지한다. 호스트와 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**으로 같다. AGENTS.md·LEFT_JOBS.md·dotnet 인계를 다시 읽었다. GUI/실제 게임/MP/사용자 제외 검사는 하지 않았다.

RawBoardAngle이 패치0041cdb0/CD0043f4e0의 실제 fmod→음수 보정→끝각/진단을 공유한다. 패치의 double 중간값과 CD의 float 저장·signed 비트 비교를 구별한다. 실제005022a0은43b3fffd/359.9999F다. 입력±0은 원문CRT처럼+0이며 음수 정수배수의 나머지-0은 보존한다. 원문의 별도 double modulus 전역은 일반360, 집중 입력720이며 float360/361과 독립이다.720 입력은 결과 주입 없이 진단에 도달하기 위한 명시 전역 입력이다.

Storage의 같은 현재 checking/원문 진단 장치를 BoardAngleDiagnostics 값 뷰로 전달했다. 방향 helper0041ce20의 네 실제 소비자는 배리어 교차004568b0(RawFenceTarget),썬 아처 회전00467940,관측각0046aaf0(RawGunTarget),아이스 반사00482080이다. 네 경로와 썬 아처 발사00467c80 종료각·공통 크로스보우 회전을 연결했다. Fire는 진단 복귀 뒤 현재 finishAngle/tolerance를 읽는다. 기존 aggregate 입력은 새 필드를 구조체 끝에 추가해 보존했다.

실제 x86 CPU의 보호 PE/CRT1088행과 재관측1088행이 일치했다. x87 제어0x027f(53비트),binary32 끝각/부호0/작은 음수/큰 유한값·modulus360/720·전체 checking DWORD·진단 중 checking 변경을 포함한다. 진단30회이며 모든 반환은 실제 Native fstp qword의 binary64다. CPU golden SHA-256은 `3082bb54f64cf04fd178da9705e056235436df8a755bd5c661f7daba767ff7a8`이다. [구조화 보고서](../../cpppj/board-angle-checking-20261104.json).

처음 Unicorn의 실제 기계어1088행을 사용했을 때 큰각64행이 달랐다. 실제CPU 관측기는 Unicorn과112행(큰각64/음수 정수배수-0 48)이 달랐다. 초기005022a0의0 추정과+0 임시guard를 제품에서 교정했으며 실제CPU 자료를 golden으로 선택했다. 기존 커밋fixture를 바꾸거나 실패 입력을 제거하지 않았다. 최초 Unicorn TSV/보고서와 차이는 `extracted/cpp-resume-820/unicorn-*`에 그대로 보존했다. Unicorn 도구는 이제 진단 중간 산출물만 extracted에 생성하며 제품 golden이 아니다.

`tools/native_board_angle_probe.cpp`는 별도32비트 콘솔이다. 원본 PE를 파일 읽기 전용으로 읽고, 관측기 자신의3MB 빈.payload에410000..61ffff 바이트를 그대로 배치한다. 다른 OS 맵을 반납하지 않고 원본 시작/GUI/IAT 초기화를 실행하지 않는다. 실제 부모/CRT만 호출하고 진단 UI만 callback이다. 최초 VirtualAlloc 주소충돌·min/max 컴파일 오류·.text 링크 경고·누락 metadata의 접근 위반을 보존했고 최종.payload 방식은 경고/오류0이다. 프로브 빌드 명령은 원시 `probe-build-payload.log`와 보고서의 설정을 따른다. VS18 vcvars32 뒤 /std:c++20 /O2 /Gy /MD /W4 /EHsc /utf-8,BASE400000/DYNAMICBASE:NO/FIXED/CONSOLE/INCLUDE:_peScratch를 사용한다. 재검증은 `python tools/observe_board_angle_hardware.py --verify`다.

Release 빌드5회는 종료0이며 최종 경고/오류0이다. 집중 C++32개(새3)가 최종 실패0으로 통과했다: 각도3/아처4/총표적5/배리어5/반사8/World7. 최초 Unicorn64실패·중간 Python 괄호 오류 뒤 같은64실패·CPU golden 적용 뒤 임시guard48실패·수정/최종 로그를 모두 보존했다. 기존 보호2746/fixture926개는 불변이고 새CPUfixture1개 포함2747/927개다. EXE SHA-256은 `05e90bbf4049446c540a073fd6517c41a97ce3ea9d05e3c7f58d2f5a17990488`다.

직접 세 번째 caller인 바람004dd2b0은 아직 미연결이다. 썽크004dd330은 Client00439af0에서 Movie 갱신 잠금 없는 가지의004180f0 뒤/004182c0·다리 수명·위치 음향 앞에 호출된다. 현재Client에 이 호출이 없고 BurningFrameState.wind는0이다.821에서 x005e18a8/y005435b0/angle005e18b0의 실제 초기값/쓰기를 같은 Client 수명·GameRandom·입자 소비자에 연결한다. 바람 caller까지 연결하기 전0041cdb0을 완료로 세지 않는다.

53비트·유한 입력을 전체 x87 확장 정밀도/비유한 CRT 오류·OS/FILE/SEH로 확대하지 않는다. 실제 공격/포획/희생/승리·MP·공중3종 전용·환경 변화 없는 물리 캡처 재시도·전체CTest/71대상/34미션/ZIP은 수행하지 않았다. 전체 목표는 미완료다.
