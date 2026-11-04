# timer57 기본 cdecl의 빈 목록과 기한0

단위886 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치·산출물 변경0이다. 직전885는 `38742606 / 20261105-19 CP`, 현재 제품886/11.01이며 전체 목표/Pop은 미완료다.

PATCH00420ad0은 new40 뒤00420960을 호출한다. 공통 몸체는 base+4/+8을0, VT0050330c, 별도 new400의 목록 포인터, capacity100/count0/endTime QWORD0만 쓴다. +20 flags/+24는 기존 heap을 보존하며 현재 게임 시각·부모·TYPE 번호·Host·Init을 읽거나 호출하지 않는다. 기존 BombEffectTimerProcess(delay,members,flags)는 원문 별도 Init 계약으로 현재 시각+지연을 계산한다. 이를 delay0으로 재사용하면 cdecl 기한0과 달라지므로 **ConstructEmpty와 별도 기본 생성 overload**를 연결했다. 미작성 flags는 명시 heap 입력으로 받고 실제 C++ 공장은0을 선택한다. 반환 객체는 같은 현재 월드의 장치를 빌리지만 부착하지 않는다.

Teleport의 현재 TYPE 함수가00420ad0이면 SID/둘째0은 미사용 cdecl 인자이며 공통 후처리로 계속한다. 버린 결과를 Session의 unique 소유권으로 보관하고 Init/Kernel/타이머 Frame을 추가하지 않는다. 공개 MakeUnstartedBombTimer와 같은 내부 MakeEmptyBombTimer를 사용한다. 실제 C++ vector 확보 실패는 bad_alloc을 cdecl NULL 경계로 처리한다. 원문 내부 new400의 NULL은 capacity100/null목록인 객체를 그대로 반환한다. 해당 불완전 원문 메모리 상태와 C++ vector 예외는 별도 호스트 한계이며, 물리 할당 실패를 강제하지 않았다. 원문 미사용 +24/padding·예약 대상 배열의 바이트 자체를 C++ 의미 필드의 동일성으로 주장하지 않는다.

[격리 도구](../../tools/decomp_bomb_timer_constructor.py)는 PATCH 두 몸체를 실제 PE에서 실행하고 두 operator new만 공급한다. heap0/A5×미사용 caller/ECX0/65535×시각0/100.25/NaN의12입력에서 전체 객체40BYTE/미작성 배열400BYTE·스택/SEH 원상복귀·앞뒤 보호 바이트를 관찰했다. 현재 시각 read hook은 전부0회다. 별도 외부/내부 NULL4회도 원문 정상 복귀를 확인했다. [새 fixture](../../cpppj/tests/fixtures/bomb-timer-constructor-x86.tsv)의12행과 NULL4회를 다시 실행해 일치했다. CD0045f760은 별도 인라인 몸체이며 baseVT00500d68→timerVT005025d0/new40/new400/빈100칸/기한0/flags 미작성을 정적으로 기록했다. CD 실행/새 타이머 이식은 하지 않았다.

새 C++ 순수1사례는12입력의 의미 필드/heap flags/부모0/Form0/풀 카운터와 생성 후 시각 변경을 대조한다. 실제 Session 새2사례는 Load 전 기본 생성/기한0/미부착과 설치 후 TYPE61의 현재 함수 주소를 timer57로 선택한 Teleport 공통 장부·깊이/할당·Kernel·renderer 불변을 확인한다. 기존 비멀티 수치 Frame4416행과 경로/기본Form/EvilRay/Teleport 실제 연결도 통과했다. **고유29개/9묶음(새3) 첫 실패0**, Release1회 280.3150798초·종료0/경고·오류0이다. 최종10파일은 빌드 중 불변·추가 교정/기존 기대값 변경0이다.

기존 보호2766개/946fixture 불변·새1개 후 기준은 **2767개/947개**다. EXE는10660864BYTE, SHA-256 `afe3fba5b7385b3ad32a497fd307f7a0cb3a36ebb53262bb97dc9a588ccae5ac`다. [검증 JSON](../../cpppj/bomb-timer-constructor-current-20261105.json)과 Git 제외 `extracted/cpp-resume-886/`에 원문 관찰/양판본 정적 근거·빌드·검사·소스/보호SHA를 보존했다.

checking10(부모4/부분3/MP3)·완료부모38/부분3은 유지한다. 남은 PATCH process 생성자는 UI50/51 두 항목이며 자산70 이상/현재 TYPE writer/다른 모드·전체Pop은 잔여다. MP7d 수신은 기존 초기화 생성자를 유지하며3차에서 무검사 검토한다. 주문 시전·게임/GUI/전체 AI·전투 Kernel·포획/희생/공격/승리/사용자 직접 확인·공중3종 전용·신규 MP/패킷/통신·전체71/CTest/34미션/ZIP 반복0이다. 다음887은 이미 Impl에서1119/1134에 준비한 실제 마나/사정거리 표시 두 몸체의 함수 주소 분배다. 목표는 active다.
