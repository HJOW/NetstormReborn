# 경로 8종 cdecl의 고정 클래스 생성과 미시작 수명

단위885 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치·산출물 변경0이다. 직전884는 `7a4ef74b / 20261105-18 CP`, 현재 제품은885/11.01이다. 전체 Pop/전체 목표는 미완료다.

PATCH TYPE23/24/38/39/40/41/42/54의 wrapper는 각각0048be40/0048bf30/0048bed0/0048bea0/0048bf90/0048bf00/0048be70/0048bf60이다. 모두 인자를 읽지 않는 cdecl이며 `operator new(0x186f0)`→공통0048b2c0→자기 VT를 저장한다. 부모/Init/Attach/Kernel/Frame을 호출하지 않는다. VT는 각각0050e284/0050e3d8/0050e320/0050e2ec/0050e4c4/0050e354/0050e2b8/0050e458이다. 현재 TYPE 번호와 함수 주소의 클래스를 구분해야 하므로 번호를 다시 비교하지 않고 고정 주소에서 기존 RawPathProcess의 실제 종류/장치 조립으로 연결했다. 기존 CreateType의 독립 계약과 명령 경로는 유지한다.

공통 몸체는 base+4/+8/count를0, 시각5000QWORD와 nextTime을0으로 만들고 current/angle/finished/두 대상/helper/helper+4/deadFlag 및 tail 두DWORD를 지운다. 꼭짓점/방향/속도/나머지 padding은 미초기화 heap 입력을 보존한다. 기존 RawPathRuntime을 그대로 사용하며 새 실제 C++ 생성은 그 입력으로0을 선택한다. 부모 SID는 Teleport caller에 남은 미사용 인자이며 부모로 부착하지 않는다. 원문이 버린 반환 객체는 C++ Session의 unique 소유권으로 보관해 미시작 상태를 유지하고 motion/월드 참조보다 먼저 반납한다. 이는 사용되지 않는 원문 누수를 Session 정리에서 회수하는 호스트 수명 경계다. 실제 메모리 부족을 강제하지는 않았다.

[격리 도구](../../tools/decomp_path_constructor.py)는 실제8wrapper/공통 몸체를 실행하고 operator new만 외부 공급한다. heap0/A5×잔여 caller 인자0/65535와 ECX를 교차한32입력에서 **전체100080BYTE**·스택 보존/cdecl 복귀·앞뒤 보호 바이트를 확인했다. 별도8회 NULL 할당은 공통 생성자/객체 쓰기 없이0으로 돌아왔다. [새 fixture](../../cpppj/tests/fixtures/path-constructor-x86.tsv)는32개 전체 이미지를 RLE로 저장했고32+8을 재관측하여 일치했다. 첫 도구 probe의 저장 횟수 상수10018을 실제10016(배열10000+공통15+wrapper1)으로 고친 뒤 처음 fixture를 생성했다. C++ 결과를 Native 기대값으로 사용하지 않았다. CD 기존 독립 Runtime/생성 fixture와 이번 PATCH 주소 분배는 별도 범위다.

새 C++ 순수1사례는32개 전체 이미지를 비교한다. 새 실제 Session2사례는 설치 전 접근의 명시 예외·설치 후8종 전체 이미지/현재 MovementType/부모0/Form0/할당·Kernel 불변, TYPE61 한 항목의 함수 주소를8종으로 바꾼 실제 Teleport 공통 장부/깊이 복귀를 확인한다. 첫 실행은 새 공개 접근점이 Load 전의 미설치 motion을 역참조해 요약 없이 종료됐다. ConnectMotion이 Impl 생성자가 아니라 Load에서 실행됨을 확인하고, 공개 접근점의 설치 관문과 prefix의 선택 가능한 모듈 확인을 추가했다. 새 실제 생성 검사는 관문을 확인한 뒤 Load한다. 최초 비정상 종료의 정확한 OS 종료값은 당시 runner가 보존하지 못했으며 빈 로그/원인/교정 기록을 남겼다. 순수32행 첫 통과는 재실행하지 않았고 교정된 실제 연결/기존 회귀만 실행했다. **고유27개/8묶음(새3) 최종 실패0**이다.

Release 첫270.2923621초·교정129.0205733초 모두 종료0/경고·오류0이다. 컴파일 파일 교정은 Session.cpp와 새 Session 검사뿐이며 최종9파일 SHA는 교정 빌드 중 불변이다. 기존 fixture 입력/기대값 변경0·보호2765개/945fixture 불변·새1개 후 기준은 **2766개/946개**다. EXE는10658304BYTE, SHA-256 `9a7942ac20ee14758b99cff7de003438a9ec291e41107ad84f2540ea2bf30a87`다. [검증 JSON](../../cpppj/path-constructor-current-20261105.json)과 Git 제외 `extracted/cpp-resume-885/`에 기계어·최초 실패·최종 빌드/검사/보호 SHA를 남겼다.

checking10(부모4/부분3/MP3)·완료부모38/부분3은 유지한다. 남은 PATCH process 생성자는 UI50/51·timer57의3항목이며 자산 TYPE70 이상/다른 현재 TYPE/모드/전체Pop도 잔여다. 게임/GUI·전체 게임/AI/전투 Kernel·사용자 직접 확인·포획/희생/공격/승리·공중3종 전용/신규MP·전체71/CTest/34미션/ZIP 반복0이다. 다음886은 UI2/timer의 원문 ABI/초기화 차이와 기존 표시/타이머 몸체 연결을 이어간다. 목표는 active다.
