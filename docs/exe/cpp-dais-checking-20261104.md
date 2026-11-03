# 다이스 checking 진단 복귀와 내부 현재 가상 Count

단위840 기록이다. 현재 호스트와 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**으로 일치한다. 디컴파일 산출물은 변경하지 않았다. 직전839는 `21cf8caf / 20261104-21 CP`다. 사용자는 이번 단위의 검증·문서·로컬 커밋을 마친 뒤 목표 일시 정지를 요청했다. 841 이상의 새 단위는 시작하지 않는다.

CanBridge00449710/CD00481910에서 실제 Carrier가 참조하는 내용물 finder를 사제 genus 진단 전에 시작한다. 첫 후보와 이미 읽은 next를 보관하고, 패치의 `priest.getGenus() & gPRIEST` / `.\Dais.cpp:0x140` 진단 정상 복귀 뒤 현재 TYPE/genus·owner·후보 mask/kind·사제 종류를 읽는다. 패치는 현재 제단 owner 행과 원래 owner 열의 동맹 DWORD가0인지 확인하고, CD에는 이 동맹 조건과 패치 진단을 추가하지 않는다.

내용물 finder004b2270의 checking→dead→현재 boss→allowDead 순서와 `\Ns\O\Squidfinder.cpp:0x13c` 진단 복귀를 실제 같은 상태에 연결했다. 이 함수에는 form finder004b22d0의 metamap 조건이 없다. 진단 장치가 정상 반환하면 현재 부모 head를 처음 읽는다. 진단이 바뀐 장치의 수명을 끝내더라도 호출 중 선택한 함수는 복사본으로 유지한다.

BridgeCommand00449900/CD00481ab0은 실제 PathStart가 NULL일 때 현재 checking으로 `p` / `.\Dais.cpp:0x18e`를 진단하고, 정상 복귀해도 즉시 반환한다. 부속 MoveAwayDepth00427d30은 현재 mapper/form/Host의 실제 경로를 조회한다. 최초 depth로 `\Ns\O\Carrier.cpp:0x87b/0x87c` 조건을 판정하고 마지막 반환은 현재 depth를 다시 읽는다. 진단 중 경로가 삭제되면 원문의 해제된 포인터 읽기를 재현하지 않고 명시적인 호스트 수명 오류로 거부한다.

Stage00447890/CD004812a0은 입력 프레임(-1이면 현재 프레임)을 먼저 저장하고 현재 가상+80을 한 번 읽는다. 판본별 4 DWORD 표와 DWORD 뺄셈의 signed32 해석을 사용한다. 실제 PE의 정적 표를 별도로 확인했다.

| 판본/보호된 원본 | VA | DWORD 표 |
| --- | --- | --- |
| 10.78 `originals/Netstorm.exe` | `00532620` | `{1, 3, 64, 120}` |
| 10.72 `originalCD/NETSTORM.EXE` | `0052e2b4` | `{10611, 3, 64, 120}` |

Altar EscapeFrame004486d0/CD00483040은 방향과 TYPE를 가상 호출 전에 저장하며 count0에서만 현재 가상 Count를 사용한다. Dais StartSacrifice004483c0과 FinishSacrifice00449b30의 확인된 가상+80도 같은 Session 분배기에 연결했다. Finish의 첫 Count·owner 저장·두 번째 Count 순서를 유지한다. 이 효과 함수들은 정적 대조·컴파일만 확인했으며 실행하지 않았다. Stage count>=4, EscapeFrame count0의 계산 방향이16 DWORD 범위를 벗어나는 입력, 미연결 가상 장치 등은 명시적인 호스트 오류다.

실제 Session은 현재 boss·방향별9×9 동맹표·canonical 사제 종류·checking·진단 서비스를 제공한다. Temple Stage와 Dais 내부 Count는 기존 메뉴/저장과 동일한 VirtualCount를 사용한다. 제단의 중첩 Count는 같은 대상 SID를 보존한다. 독립 fixture에는 가상 호출 장치를 명시하여 기존 원시 기대값을 바꾸지 않았다. 실제 Session에서 사제 의식/희생 진단을 재현했다고 확대하지 않는다.

새 검사6개는 자격 판정, 진단 전 first/next 저장과 복귀 뒤 현재 head/TYPE/owner/kind/동맹, 현재 권한과 실제 NULL PathStart 반환, 빈 실제 Path 객체의 depth 진단 복귀, 양판본 Stage0..5 경계와 프레임 선행 저장에 한정한다. 게임 자산 생성/Frame/의식/포획/희생/피해/음향/예약 등 행동 장치는 호출하면 실패한다. 실제 경로 객체는 초기 빈 상태의 깊이 조회용이며 성공 이동이나 게임 프레임은 실행하지 않는다. 기존 TempleRitual/DaisRegular/AltarSacrifice 검사 소스3개는 장치 연결의 컴파일만 확인하고 전체 효과 검사는 실행하지 않았다.

집중 **17개(새6개) 첫 실행 실패0**이다. 새 Dais6·현재 Count getter3·현재 가상 Count 메뉴5·실제 메뉴 회수/닫기/현재 World3을 선택 실행했다. 최초 빌드는 protected `SquidProcess::Pid()` 접근 C2248 1건으로 실패했고 소스와 로그를 보존했다. 현재 Form+0x12 DWORD PID를 읽도록 수정하여 수명 검사를 유지했으며 public API를 추가하지 않았다. 수정 후 Release 빌드는 종료0·경고/오류0이다. 새 원문 CPU 관측/fixture0이며 보호2759개/fixture939개는 검사 뒤에도 모두 불변이다.

현재 제품840/11.01 EXE SHA-256은 `33fcd2b5e91edeb4a4a2308984792156f6c83ea7ebdb2f483aa3121e6a8e48e0`다. 원시는 `extracted/cpp-resume-840/`에 있으며 Git 제외·작업 PC 전용이다. 상세 빌드/검사/소스/최초 오류/보호 해시는 [840 JSON](../../cpppj/dais-checking-20261104.json)에 있다.

CanBridge와 BridgeCommand의 두 checking 부모를 완료 집계하여 **잔여12=부모6+부분3+MP분류3·완료부모36/완료부분3**이다. 부속00427d30은 기존51부모 집계에 따로 추가하지 않는다. 나머지 메뉴/Edit·State/파일·Factory 자체 진단·공통 Pop·Windows 호환성·MP/4~8차는 미완료로 남긴다. 이번 단위 문서·커밋 뒤 목표를 일시 정지하며 새 작업을 진행하지 않는다.

실제 게임/GUI·물리/PCM/CD삽입·포획/희생/특정 적 공격/승리·신규MP/통신·공중3종 전용 검사는0이며 전체71/CTest/34미션/ZIP을 반복하지 않았다. push0·이번 CP 단위의 DN/WB 수정/커밋 포함0이다. 마무리 중 확인한 별도 세션 dotnetpj 미커밋 변경은 보존하고 이번 커밋에서 제외한다.
