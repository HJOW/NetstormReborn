# 기본 Form 네 cdecl 생성자의 지정 슬롯 단일 저장

단위884 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치·산출물 변경0이다. 직전883은 `d2091391 / 20261105-17 CP`, 현재 제품은884/11.01이다. 전체 Pop/다른 생성 분배와 전체 목표는 미완료다.

PATCH004ae760/004ae3d0/004ae9f0/004aea40(TYPE1/2/3/6)은 첫 인자SID로 `poolBase+SID*50`을 계산하고 그 물리 주소의 가상 표DWORD만 쓴다. CD004af920/004af5c0/004afd60/004afe00은 stride36과 CD 가상 표를 사용한다. 함수는 새 슬롯을 할당하거나 TYPE BYTE·상태·payload를 초기화하지 않는다. 따라서 Teleport가 이 주소를 선택했을 때 첫 인자는 새 Form의 부모/할당 flags가 아니라 덮어쓸 **현재 물리 SID**다. 이를 기존 Create/Take로 바꾸지 않고 SquidForms.ConstructVtableAt으로 연결했다. 함수 주소로 선택하므로 선택 TYPE 번호를 다른 기본값으로 바꾸지 않는다.

SID0/예약/free/dead도 원문 물리 범위 안이면 그 DWORD를 그대로 쓴다. SID0의 첫 DWORD가 특수 풀 머리와 겹치는 의미까지 보정하지 않는다. 보존 범위는 **해당 DWORD 밖의 모든 바이트와 타입BYTE/할당 카운터**다. 실제 소유 메모리가 있는 SidPool의 주소 범위와 원문 null pool/32비트 포인터 감김은 구분한다. Native 반환값은 같은 물리 포인터이며 현재 Teleport caller는 이를 사용하지 않고 공통004b0d30으로 계속한다. 새 Form/Kernel/입자를 만들지 않는다.

[격리 도구](../../tools/decomp_form_constructor.py)는 양판본8함수×SID7개(0/4/5/50/23001/32767/65534)=56입력을 실제 PE에서 실행한다. memory-write hook으로 목표 주소의 단일DWORD 외 쓰기를 거부하며 cdecl 스택/뒤 인자 보존과 반환 포인터를 확인한다. 임의 payload·TYPE74·free/dead 상태를 공급한 [새 fixture](../../cpppj/tests/fixtures/form-constructor-x86.tsv)의56행을 재관측하여 일치했다. CD의 정밀 C 본문이 없는 일부 주소도 실제 entry 기계어와 PE 실행으로 확인했다. 게임/창/할당 API는 실행하지 않았다.

C++ 새 순수1사례는 양판본56슬롯의 전후 전체 바이트/카운터를 재생한다. 새 실제 Session1사례는 같은 Teleport VT에서 현재 TYPE1/2/3/6의 함수를 각각 선택하여 같은SID의 VT가 바뀌고 raw TYPE74/할당 수·Kernel/renderer 수가 유지되며 공통 TYPE74 통계+1/깊이0으로 돌아가는지 확인한다. 검사의 종료 수명에서는 명시 입력의 원래 VT를 복원하고 이후 잘못된 TYPE/VT 조합의 Form 행동은 실행하지 않는다. 고유24개/6묶음(새2)의 첫 실패0이다. 기존 Teleport/NULL/EvilRay 실제 생성 회귀도 통과했다.

Release1회 944.5956204초·종료0·경고/오류0이다. 최종8파일은 빌드 중 불변·추가 제품/검사 교정0·기존 fixture 기대값 변경0이다. [검증 JSON](../../cpppj/form-constructor-current-20261105.json)과 Git 제외 `extracted/cpp-resume-884/`에 원문 기계어/실행·빌드/검사 로그와 SHA를 보존했다. 기존 보호2764개/fixture944개는 불변·새1개 후 기준은 **2765개/945개**다. 현재 EXE는10656768BYTE, SHA-256 `8f92587213fd90d774f1a8ac0db34bb4d77f412afc9048a7e57b0716a8f9528b`다.

checking10(부모4/부분3/MP3)·완료부모38/부분3은 유지한다. 경로8/UI2/timer1의 process11항목과 자산 생성자·다른 현재 TYPE/모드·전체Pop은 잔여다. 게임/GUI·사용자 직접 확인·포획/희생/공격/승리·공중3종 전용/신규MP·전체71/CTest/34미션/ZIP 반복0이다. 다음885에서 경로 생성자의 고정 클래스/ABI와 부착 없는 결과 수명을 이어간다. 목표는 active다.
