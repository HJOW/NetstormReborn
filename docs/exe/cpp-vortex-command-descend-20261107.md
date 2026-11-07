# 1051 — 신전 명령·ViewMeta 저장과 State19 요청

1050의51h/상승 완료에 이어 신전004dbac0의 실제 가상6c 명령,004dc220 event4와 일반 입력의004b3620/004b3760까지 연결했다. **State19의 실제 CS/목록/메타 부모는 아직 미연결**이다. 이 단위는 저장 뒤 다음 상태 요청까지이며 현재 메타맵 진입/Descend 완주 결과가 아니다. 신규 MP/단위/게임/통신 검사는 수행하지 않았다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이며 이번 산출물 변경은 없다.

보호 PE 이름표542b88의8BYTE 행과 실제 명령 DWORD를 대조했다. WipeOut=0300006f(542f00), Ascend=0300007c(542f68), Descend=030000a7(5430c0), **ViewMeta=02000013(542c20)**이다. 기존 event4의 애니메이션 토큰/세대번호라는 임시명은 실제 의미와 다르다. 실제 Client는 이제 Fort 지역을 조회하고 현재curServer의 상위WORD와 OR한 값을 현재 ViewMeta 명령의 두 인자에 넣어452fe0 큐에 예약한다. 일반 이름표에도 네 원문 이름만 연결했다. 새로운 메뉴/별칭이나 외부 서버 접속은 만들지 않았다.

가상6c는 네 실제 신전VT에서 공통 객체 부모보다 먼저4dbac0을 실행한다. WipeOut은 원문 진단0/2ea 뒤 처리1이다. 상태WORD0의 Ascend는 현재 inBattleMode 진단→확인WORD0의 첫 살아 있는 같은owner 목록 사제/현재free/유효하고 같은 표면 섬 검사다. 실패하면 신전SID|10000h의 **원문%d 숫자**를 global.param1에 쓰고PriestOffIsland를 표시한다. 확인WORD1이면 사제 검사를 생략하고 그 외 비영WORD는 처리1로 끝난다. 성공하면 상태1→현재 섬→시작51h/8BYTE→preparingAscend.wav의 원문 여섯 인자→현재 대상9 실제 mutable 송신이다. 상태0의 Descend는 같은 Kernel에 Regular event4/count0을 생성한다. 나머지/상태가0이 아닌 일치 명령은 공통 부모로 위임한다. 실제 입력은 y의 낮은WORD SID와 높은WORD 확인 인자를 함께 전달한다.

신전 event4는 상태4→기존 checking/float count4 이하 대기→0046f650/0046f5a0의 실제 지역getter→현재서버/ViewMeta 큐→상태0이다. getter는 float 절삭을 먼저 하고 Fort 진단→현재hash0→실제 표면 genus/지역WORD→섬 존재 진단→섬 기록의signed 지역BYTE 순서다. bridge/없는 표면의 지역없음과 invalidTerritory00540a30은 같은 초기127이지만 독립 전역으로 구별했다. 실제 Client의 ViewMeta 명령/invalidTerritory는 장기 LocalState 소유자에 한 번만 초기화한다. 기존 독립 fixture의 명시 훅은 fallback으로 유지하며 제품은새 실제 본체를 사용한다.

4d7627..4d7670의 기계어로 일반 ViewMeta 입력을 대조했다. signed 상위서버WORD가-1보다 작으면 목록getter 없이 무효 분기로 가고, 그 외 현재 시작 목록 개수와 비교한다. 무효이면19번 원문 로그 뒤 Config curServer에문자열0을 쓰며 저장 요청은 하지 않는다. 유효이면 낮은지역WORD로004b3760을 부른다. **입력의 상위서버WORD는 검증용이며 현재 선택 전역으로 덮어쓰지 않는다.** 기존 Client의 실제 Config/서버 목록/입력 큐를 사용한다.

004b3620은 현재Template read-only가 비영이면 경고하지만 계속한다. 현재Fort/invalidTerritory 진단→BattleTerritory 전체DWORD 저장→0초기화120BYTE 현재Territory 읽기→선택 지역flags의home bit4 OR→보관한6BYTE/현재owner의 지역 검색→같은 섬 지도6BYTE/생산 갱신→그 시점의 현재Template Territory 쓰기→그 시점 현재Template의 전체Fort snapshot이다. 지도 갱신004324e0은1050의 상승 부모와 같은 모듈로 공유하고 값 전달6BYTE를 진단 전 고정한다. 저장이 정상 복귀한 뒤 next[0]=19→+48 DWORD→+4c WORD→+44 현재서버를 쓴다. +4c 상위WORD 및 나머지144BYTE는 보존한다. 원문 추가 parent 실행/현재phase 확정/World 반납은 여기서 하지 않는다.

**다음:** State19의 CS 생성/초기화·메타 입력/표시/대여와 뒤 단계, 메타맵2VT·Outpost·나머지 endpoint/서버/writer·decoder다. checking잔여5는 해당 후속 부모가 남아 있어 유지한다. 실제 코드 연결을 LAN/상승/Descend 플레이 성공이나 폐기 서버 복구로 세지 않는다. 원본의 범위 밖 스택/메모리와32비트 물리 배치 자체는 호스트 경계다.

NetstormCpp Release **종료0·경고/오류0**, 실행파일 12,219,904바이트/SHA-256 `ac9dd246e2bf5c574fedbf90871cf08ae50d9cecb3e6d5e13bfbe480d89e6fd8`다. 보호 2800파일 불변·새fixture0·신규 MP/단위/게임/통신0이다.
