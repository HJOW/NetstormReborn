# 1157 — 공통 Pop·신전 정기 Handle의 현재 checking 마감 대조

2026-11-08 / HJOW-Athlon. 제품은1156/11.01이며 이번 단위는 정적 감사다. 직전 커밋18e3aadd.

M3.1의 `004b02d0`는 기존 `RunSquidPopParent`에, M3.2의 `004dc220`는 `RawAssetRegular::Vortex`에 이미 연결돼 있었다. 후자는 일반 마우스 입력 부모가 아니라 신전의 정기 Handle(event0~4/default)이다. 전체 본체를 다시 작성하지 않고 원문 C/PE와 실제 Client→World→Session 대여를 확인해 오래된 checking 미완료2개를 제거했다.

Client는 같은 signed checking DWORD를 SurfaceStorage에 바인딩한다. 실제 World는 해당 뷰/TYPE·진단·로그를 Pop에 전달하고, Session은 같은 장기 owner를 다시 빌린다. RawCheckingFlag의 복사는 값의 bool snapshot이 아니라 원래 owner를 가리키며 관문에서 현재 DWORD를 읽는다. Pop은 현재 슬롯/void/contained→좌표 보정→부분 spot/side 효과→현재 grid 후보→checking 진단/복귀/같은 로그→현재 SHP/해시/표시/firstPop/Activate 순서다. 실패·겹침에서 앞선 부분 효과를 되돌리지 않는다. 기존 원문 진단과 현재 World 장치를 모두 확인했다.

신전의 event0 상승 진행, event1/2 현재 상태 애니메이션·실제 난수, event3 이탈 상태/통지·섬/실제 요청, event4 상태4→현재 checking 또는 signed count>4→실제 지역/현재서버·명령 큐→상태0, default Damageable 분기를 대조했다. 현재 Session의 assetState.checkingForms는 같은 Client DWORD를 빌린다. 네 가상표의 +5c는 세 직접4dc220과 하나의4dcf00→4dc220 점프다. 풍 계열을 다른 가짜 Handle로 만들지 않는다. PE의 event4 비교 상수도4.0이다. 실제 State/상승·이탈/Descend 장치가 같은 Session에 연결됨을 확인했고, 끝단 World 전환/카메라·HTML Caller 마감은M3.3에서 계속한다.

제품 소스 변경0, 새 빌드/단위/MP/게임/통신/전체/ZIP 실행0이다.1156의 NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0 이후 EXE 두 개가 바뀌지 않았고 보호2800파일도 불변이다. 기존 집중 검사 코드를 읽었으나 재실행하거나 새 통과로 세지 않는다. 보조 C/ASM은 Git 제외 `extracted/cpp-resume-1157/`에 보관한다.

Netstorm.exe 13,169,664바이트/SHA-256 `76ff76230af03c4c189d9e7de5e5bd6adfaf3f6aaf1d3a27f6f55f12f2a0722d` / R.exe 670,208바이트/SHA-256 `4cfdfe3f46384090bbd9328959d3aebda4844b29556273bb0f8f4f1d7963c9be`

현재 checking 목록은0개이며 M3~M8의6묶음/전체 목표는 미완료다. 다음 M3.3에서 실제 Descend/메타전환 요청·같은 World/카메라·HTML serial 후속을 확인한다. 마지막 전체 디컴파일2026-11-03/HJOW-Athlon, R전용2026-11-07/같은 호스트이며 생성 원문은 수정하지 않았다.
