# Player의 현재 부모와 선택 장치

단위858 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치이며 산출물 변경0이다. 직전857은 `eae6fd03`이고 현재제품은858/11.01이다. 전체 목표는 미완료다.

PATCH004905d0은 현재 owner BYTE가 가리키는 실제 Player+9c DWORD에 SID를 먼저 기록한다. flags1/현재 local owner DWORD 일치/비Fort 관문 뒤40BYTE 확보를 시도하고 성공한 경우만 Regular1/5.0f 생성자를 호출한다. 이후 flags1/현재 special 비영이면 같은 물리 슬롯 frame DWORD를 직접1로 쓰고 공통004b0d30으로 간다. 실제 몸체 앞 Validate와 생성자의 전수 장치 검사를 제거했다. 현재 VT 조회/직접 기록·프레임은 free/contained/TYPE와 무관한 같은 물리 슬롯이다. callback 뒤 raw/전역을 복구하거나 처음 선택한 조건을 재검사하지 않는다. 실제9기록 밖 owner와 SID 풀 밖은 원문 진단 대신 호스트 물리 경계에서 거부한다. 명시 Validate와 CD00407cc0의 기존 사전 계약은 유지한다.

Create00490250은 PlayerKnown 진단 뒤 현재 special의 y와 전달 owner+10의 x를 저장하고 현재 playerType로 typed Find를 실행한다. 중복 진단이 복귀하면 다시 현재 playerType로 Factory(flags2)→가상 Owner→직접 물리 frame0→저장한 좌표의 Pop0 순서로 간다. getter0048fc60은 Known0에서 owner/기록을 읽지 않고0이며0048fc90은 진단 복귀 뒤 명시 owner의 현재 기록을 읽는다. 필요한 하위만 도달 위치에서 요구하고 선택한 callable 수명을 유지한다. Session의 기존 단순 level0 finder를 같은 World typed Find004b1fa0로 교정하고 미연결 Player 진단을 같은 Client 서비스의 원문 expression으로 연결했다. 현재 Client 진단 API는 expression만 전달하므로 원문의 파일/행 표시 전체 일치로 확대하지 않는다. CD00407400/00406d90/00406dc0와 전수 구성/할당 경계는 유지한다.

새16개(순수14+Session2) 포함67개/10묶음을 확인했다. 기존 Player 양판본1680행은 실제 Factory/Owner/Pop/ProcessForm/Kernel/9기록을 전수 재생하며 실제 World/Host 검사도 유지한다. 새 순수 검사는 물리/선택 관문·기록/프레임/공통 실패 앞 부분 효과·현재 owner/special/type·진단 정상 복귀/생성 순서·호스트 범위/CD 차이를 확인한다. 실제 Session은 TYPE74의 Player VT에 flags0의 free/contained 기록과 최초5초 Regular를 연결하고 기록을 복구한다. 다른 Session은 Known 진단 복귀가 바꾼 현재 TYPE152/special 좌표18,10을 사용해 실제 생성/Owner/Pop/기록/frame1을 관찰한다. Frame/Process 틱/Regular 처리·의식/포획/희생/공격/승리는 실행하지 않는다. Release 제품/두 집중 대상1회는 종료0·컴파일 경고/오류0이고 집중67개(새16)/10묶음은 첫 실행 실패0이다. 핸들56452 종료/자식 해제·528.3839485초 소요와 첫6소스 불변을 확인했다. 빌드 중 소스 변경/교정 재실행0이다.

첫6소스·원문 Player8개와 다음 Dude 참고2개는 Git 제외 `extracted/cpp-resume-858/`다. 보호2761개/fixture941개 기준이며 새 Native/fixture/기존 입력·기대값 변경0이다. checking12·완료부모36/부분3은 유지하고 Dude·섬/표면/Teleport 등 나머지 현재 가상20/전체 Pop과 후속 단계는 남는다. 사용자 직접 확인·공중3종 전용·신규MP/통신·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복을 제외한다.

기존 보호2761개/fixture941개는 모두 불변이다. 현재 EXE SHA-256은 `32b715ad55714dd70cbe8f414308d7285e585c152f36f63d0c3b8dfd28eeb422`다. [검증 JSON](../../cpppj/player-current-parent-20261104.json)에 첫 소스/원문/기존 fixture SHA·실제 검사 개수/종료값과 한계를 보존했다. 단위 인계/로컬 커밋 뒤859로 이어가며 목표는 active다.
