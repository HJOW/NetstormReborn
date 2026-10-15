# 건설 확정의 실제 메시지와 가변 길이

2026-10-15, 현재/마지막 정밀 디컴파일 PC는 **VM-W11-Codex**다. 기존 정밀 프로젝트에서 PATCH constructionplace23함수/confirm11함수를 읽기 전용으로 다시 내보냈다. 이는 이 PC에서 누락된 선택 export를 만드는 작업이며 원본 파일/프로젝트를 변경하지 않는다. 실제 원본 게임/GUI/네트워크는 실행하지 않았다.

건설 확정 `00444590`은 `004df590`에 현재 message63/protocol1/가변 선언 길이0과 **512바이트 스택 버퍼**를 넘긴다. 길이전역은실제`004445f1 MOVZX WORD [00544c58]`로읽는다. 실제 helper는512바이트를 전부 `ff`로 초기화하고 protocol의 bit7을 지우며, 선언 길이는 unsigned 최대(4,현재WORD)다. 이어 실제 CanonDecoder 순서로 Factory SID를 만들고 `0040a840`의 `packet+27+index*2`에 SID WORD를 쓴다. 타입·좌표·방향/인자/owner·double시간의두DWORD·개수·flags/quality를 기록한 뒤 같은 버퍼를 송신과 자기 통지 처리에 전달한다.

`WriteConstructionPacket`은 이 전체512바이트를 보존한다. 버퍼 뒤의 caller 바이트는 쓰지 않는다. 메시지 등록값은 caller의 현재 `ConstructionPacketHeader`를 사용하고 전송 직전 길이와 임시 선언 길이를 합치지 않는다.

`FinalizeConstructionPacket`은 원본 `0040a800`이다. sending0이면 아무 바이트나 진단도 바꾸지 않는다. 비영이면 현재 BYTE sidCount를 읽어 `count*2+27`을 선언 길이에 쓴다. count255이면 원본 assertion callback 뒤에도537을 기록한다. 이 helper는 실제 송신 계층이 선택하는 별도 단계다. **Write 반환512를 네트워크 전송 길이나 현재 header 길이라고 주장하지 않는다.** 단독 캠페인의 NULL 연결 표를 거치는 broadcast와 이후 자기 통지는 이 차이를 유지해야 한다.

호스트는512미만의 출력 버퍼/정상19개를 넘는 SID목록을 효과 전에 거부한다. 원본 assert 이후 스택을 넘겨 쓰는 손상 경로와 실제 소켓 전송 구현은 완료 범위가 아니다. 현재 Session 설치/Confirm→Notice→Clear/Place의 연결은 후속 작업이다.

`tools/decomp_construction_packet.py`는 앞선 Confirm oracle에서 **메시지 생성 경계만 실제004df590 몸체로 확장**하고512바이트 전체를 실제 send 호출 위치에서 캡처한다. Canon/프레임검색/SID목록쓰기/모든 필드쓰기와 헤더 초기화는 실제 명령이다. Factory/송신/자기통지/기록기와 count255 assertion은 명시 관찰 경계다. 기존 도구/fixture는 바꾸지 않았다.

독립 원본은 **86고유입력/정상반환**이다.72개는 plain/noIsland/island/다리0/25/빈목록×방향3×현재헤더4의512바이트이며,14개는 count0/1/9/19/20/254/255×sending0/1 최종화의전체512바이트다. cdecl/thiscall스택/비휘발레지스터/x87제어어와허용된쓰기/실행범위를확인했다. 최초실행은누락된constructionplace선택export를읽지못해중단됐고두선택export를재생성한뒤정상86개를얻었다. 최초실패로그는`extracted/construction-packet-native.log`, 정상로그는`extracted/construction-packet-native2.log`다.

새 C++ 집중2개는 전체512바이트·호출자뒤5바이트보존·count255진단/후속계산과물리계약을검사한다. 최초18체크실패는길이전역을DWORD로해석한제품오류였다. 실제`MOVZX WORD`를확인해공개필드/읽기폭을WORD로교정했다. helper자체의최소4처리를바꾼것이아니다. 최초자료는`extracted/construction-packet-focused.log`에보존했다. 교정뒤 **Release게임/검사경고·오류0·집중2개실패0**이다(`extracted/construction-packet-build2.log`, `construction-packet-focused2.log`). 같은86개재생성은새고유입력으로합산하지않는다. 실제Session설치/GUI/전체회귀로세지않는다.

원본/도구/정밀함수표/fixture/현재C++소스의SHA는`cpppj/construction-packet.json`에있으며`python tools/decomp_construction_packet.py --verify`로감사한다. 목표active·Early Missions/캠페인1-1~1-2완주동일성미완료·DirectDraw전체화면오류제외를유지한다.
