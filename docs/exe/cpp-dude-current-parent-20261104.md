# Dude의 현재 부모와 signed population

단위859 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치이며 산출물 변경0이다. 직전858은 `fe3d121d`이고 현재제품은859/11.01이다. 전체 목표는 미완료다.

PATCH00450cd0은 최초 flags1에서004ac220으로 물리 QA WORD5101을 먼저 쓰고 현재 BYTE TYPE와 Bridge DWORD가 같을 때만004214a0 통지를 실행한다. 이후 현재 population count를 저장하고 capacity와 signed 비교한다. 기계어00450cf4의 jge에서 미만인 경우만 보존 index의 실제 배열에 SID를 쓰고00450d1a의 inc로 그때의 현재 count를 증가시킨다. 다음00450d40은 현재 TYPE getter004ac550의 +150 DWORD, 즉 I Run 첫 프레임을 읽어 실제 SetFrame(sid,first,0)에 전달한다. I가 없다는 -1이나 코드 개수는 이 wrapper의 추가 거부 관문이 아니다. 마지막은 공통004b0d30이다. flags0/2/4는 이 몸체에서 raw/목록/프레임 장치 없이 공통 부모만 호출한다.

PATCH의 전수 생성자 검사와 실제 몸체 앞 Validate·TYPE/free/contained 조건을 제거했다. 물리 QA 저장 뒤 현재 Bridge 통지와 같은 Session 목록/Frame/부모를 연결했다. 현재 Count/Capacity/Storage/SetCount binding을 사용해 사본 vector/count를 읽지 않는다. signed 비교에서 선택한 음수 index/짧은 배열/TYPE 표 밖은 실제 역참조 위치의 호스트 물리 경계로 거부하고 앞선 QA/등록 효과는 보존한다. 선택한 callable은 사본으로 유지한다. 명시 Validate의 기존 무효과 사전 계약과 CD004e9230의 기존 구성/동작/할당 계약은 유지한다.

생성00450a20은 현재 signed count/capacity와 그 뒤 현재 차단 전역을 읽고 실제 Factory(currentDudeType,2)→직접 frame0→가상 Owner→전달 좌표의 Pop0 순서다. Native는 그 앞에 목록 전체/TYPE·VT 쌍/유한 좌표 검사를 하지 않는다. PATCH에서 이를 제거해 실제 동일 Factory/Owner를 계속 사용한다. signed 음수 count도 생성 관문은 통과할 수 있으며 실제 Pop의 선택된 population 저장 경계와 구별한다. 디컴파일의 unaff_retaddr 표현은 기존에 교정한 실제 x/y/owner 인자 순서를 유지한다. CD004e8b30의 기존 계약은 별도다.

새19개(순수18+Session1) 포함95개/14묶음을 확인했다. 기존 양판본 Dude288행은 실제 Factory/Owner/Pop/Frame/장부의 전체 raw/표시·목록을 재생한다. 새 검사는 현재 VT/물리 QA·Bridge DWORD/통지·signed 비교/현재 배열·보존 index와 현재 count 증가·현재 Run/-1 전달·선택 실패 앞 효과·실제 생성/Owner/Pop 및 명시 Validate/CD 차이를 확인한다. 순수 프레임 경계 검사는 인자 전달을 관찰하며 SetFrame 전체 의미를 새로 증명하는 것이 아니다. 실제 Session은 TYPE74의 flags0 물리 VT와 TYPE70의 최초 QA/population/I64 Frame을 같은 장치로 관찰한다. 초기 StoreLevel로 같은 표시 단계만 준비하고 실제 Frame::Set을 호출하며 Frame/Process 틱·행동·의식/포획/희생/공격/승리는 실행하지 않는다. 변경한 목록의 전체 stale 배열/count는 Session 정리 전에 복구한다. Session Bridge 통지 선택 자체는 새 실행하지 않고 순수 계약/기존850/QA setter 원문 근거로 대조한다. Release 제품/두 집중 대상1회는 종료0·컴파일 경고/오류0이고 집중95개(새19)/14묶음은 첫 실행 실패0이다. 핸들93947 종료/자식 해제·115.8294077초와 첫6소스 불변을 확인했다. 빌드 중 소스 변경/교정 재실행0이다.

첫6소스·현재 정밀 원문7개/실제 기계어3개는 Git 제외 `extracted/cpp-resume-859/`다. 새 Native 관측/fixture0·보호2761개/fixture941개 기준·기존 입력/기대값 변경0이다. checking12·완료부모36/부분3은 유지한다. 섬/표면/Teleport 등 다른 현재 가상20/전체 Pop 및 후속 단계는 남는다. 실제 게임·사용자 직접 확인·공중3종 전용·신규MP/통신·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복을 제외한다.

기존 보호2761개/fixture941개는 모두 불변이다. 현재 EXE SHA-256은 `92a0dd7f7b60d57ccbf5a0fc1fe43a18c99993e768dd0c6a103d3b3e02949f85`다. [검증 JSON](../../cpppj/dude-current-parent-20261104.json)에 첫 소스/원문/기계어/기존 fixture SHA·실제 검사 개수/종료값과 한계를 보존했다. 단위 인계/로컬 커밋 뒤860으로 이어가며 목표는 active다.
