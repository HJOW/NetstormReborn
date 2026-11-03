# 공통 PathProcess Halt의 현재 진단과 자기 삭제 복귀

단위832의 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이며 직전831 정적 분류는 `c3b7f8f0 / 20261104-13 CP`다. 현재 제품832/11.01이며 checking16=부모10+부분3+MP분류3,완료부모32/완료부분3이다.

10.78 공통00489ff0은 최초 부모를 저장하고 지역 deadFlag를 설치한 뒤 부모 가상+38을 호출한다. 자신이 삭제됐다면 현재 checking이 비영이며 결과가12가 아닌 경우 `result == haltALREADY_DEAD`, `\Ns\O\Pathprocess.cpp`,0x2d5(725) 진단을 호출하고 정상 복귀시true다. 기존 제품의 무조건 예외는 현재 checking과 정상 진단 복귀를 보존하지 못했다. 새 부모는 호출 전에 복사한 현재 checking 뷰/진단 장치와 객체 외부의 같은 수명 감시만 사용해 삭제된this를 다시 읽지 않는다.

살아 있다면 결과10 뒤 저장 부모의 현재 flags40 비트20을 읽어11로 바꾼다.11은 현재boss 또는 현재Form의 signed 로컬 범위(4<Form<15000)에서 같은 Host에 flags0 제거를 요청하고true다. 나머지는 현재boss→저장 부모의 현재TYPE genus30000→현재TargetB/TargetA 순서로 기존 실제 RawPathRetarget를 선택하며 새 이동이 자신을 삭제하면true다. 살아 있으면 최초/강제정지 후 결과가10인지에 따른 bool을 반환한다. 부모 가상/진단 후 현재 권한·TYPE·경로 부모를 다시 읽되 최초 부모 포인터와 재지정의 현재 경로 부모를 혼동하지 않는다.

두 번째 `!deadFlag`, 같은 파일0x2f2(754)는 정상 직렬 원문에서 미도달이다. 첫 가상 호출에서 살아남은 뒤 경로+186ec에0을 쓰고, 그 뒤 가능한 하위 호출은 **004ac200→0049a840**뿐이다. 이들은 rawTYPE를 읽어500배/현재59ab20 주소를 더하고TYPE+ec genus를 읽는 순수 조회다. 다른 쓰기/진단/콜백이 없다. 조건과 target 포인터 산술도 필드를 쓰지 않으므로0 비교는 항상 거짓이다. 실제CPU2288행에서 이 진단0개도 확인했으며 인위적인 getter 부작용으로 정상 게임에 없는 진단을 만들지 않았다. 불법 동시 메모리 변경·SEH/손상 포인터는 별도다.

RawPathProcess 패치Halt는 새 RunPathHaltParent를 사용하고 기존 CD 경로는 유지한다. 실제 Session.process.checking은 같은 Client AiDebug.Checking DWORD에Bind하고 RawCampaignPathEffects→RawCampaignPathBindings→각 생성 경로의 진단을 같은 services.ai.assertion에 연결한다. 기존 RunFrame/공통 도착·Walker/Bulf/Balloon의 같은 Halt와 실제 내용물/후보/새 이동을 계속 사용한다. CD의 같은 진단을 임의 추가하거나 패치 함수를 그대로 실행하지 않는다.

독립 Windows x86 관측은 실제00489ff0과 순수GetGenus/TYPE·실제 재지정 분기를 실행하며 부모 가상/진단/제거요청·내용물/후보/새 이동만 명시 외부 경계다.2288행/삭제 후 반환값 진단258행·재관측이 정확한 바이트로 일치했다. fixture SHA-256은 `ca7661b77324eb0b47cddf86211902d96e6fe282eee683ebf0cf251c648a2db8`다. 원문 지역stack 포인터 주소 자체와 raw 이미지 중간의 주소값 동일성은 비교하지 않고 같은 삭제 통지/현재 필드·호출을 비교한다.64비트 호스트는 기존 shared watch로 원문 지역 수명 감시를 안전하게 보존한다.

첫 프로브 컴파일은 종료0이나 선택된 하위 조회/후보/새 이동을Cdecl로 연결해 실행3221225477이었다. 실제 기계어의 ECX 수신자/stack cleanup에 맞춰 관측기5개를fastcall thiscall 경계로만 고쳤다. 최초 빌드/실행·단일 기본행 진단 로그를 보존했으며 실패 fixture는 생성하지 않았다. 제품/원본/기존 fixture를 실패에 맞춰 변경하지 않았다.

새 C++ 검사는 순수 부모의 CPU2288행에 기존 실제 RawPathRetarget를 연결한다. 별도의 실제 Client checking/Kernel/Host 검사는 일반134 부모의 가상Halt가 경로를 실제 제거한 뒤 현재 진단/복귀/미재접근을 확인한다. 실제 사제 포획·희생/특정 적 공격/승리·사용자 확인·신규MP/공중3종 전용·GUI/게임/통신 프레임·물리/PCM 재시도·전체71/34미션/CTest/ZIP은 실행하지 않는다. 원시는 `extracted/cpp-resume-832/`이며 최종 Release 빌드/프로브2회는 종료0·경고/오류0이며 집중8개(새3) 실패0이다. 기존 재지정4801행/프레임120행/도착24행도 통과했다. 실제 Session 회복 수명 회귀는 프레임 없이 설치/정리 연결을 확인한 것이며 신규 Path 진단을 실제 Session에서 발생시켰다는 뜻은 아니다. 기존2756개 불변·새 fixture 포함2757개/937개다. EXE SHA-256은 `092ae555c8f48ecc317d35cb895fda655e78a9020d8e3a8f470133822dac7904`다. [832 JSON](../../cpppj/path-halt-checking-20261104.json)에 결과/보호 해시를 기록했다. 다음833은 공통 도착0048b340의 현재 checking/isRobust·자기 삭제/가상 도착 반환과 실제 공간 소비자다.
