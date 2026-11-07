# 1098 — 전체 메타 Poll과 참가자 목록 갱신

제품1098 / 표시·파일·제품버전11.01이다.1097(`c065c9e9 / 20261108-03 CP`)에 이어00479760 전체와0047cab0, 종속 finder를 라이브러리에 컴파일했다. 00593150 참가자 목록 슬롯도 같은 ClientChallengeGlobals가 BSS0부터 장기 소유한다. **실제 Pool/가상 객체/Gump·CS 수신 장치 접합과0047dd60 전체 장면 생성,1dh/28~33 등록은 미적용**이다.

Poll은 PlayerKnown→1096 최초 보정→현재 카메라 배경 분모→카메라+0C DWORD/화면flags10h 관문→필요시 viewport dirty→참가자1..39→전투1..9→현재 카메라 관문 재조회→존1..50 순서다. 카메라+0C는 float 비교로 바꾸지 않아 음수0 비트도 비영으로 판단한다. 개별 참가자 갱신의 SID/raw+0B bit0 관문과 특수BYTE0, 개별 전투 갱신의 특수BYTE10은 이 Poll에 추가하지 않는다. 현재 실제 필드/클리핑과 Gump serial/가시성/기존 float 위치를 사용한다.

기계어 전체 대조로 디컴파일 C의 지역변수 추정 두 곳을 보완했다. 존 owner 카운터는 `esp+14`에서 별도 증가하며 투영 높이4/0과 합쳐지지 않는다. 종속 이름표의 가시성은 **방금 존 투영이 돌려준 `esp+20`의 clipped**를 사용한다. 과거 전투 지역값으로 판단하지 않는다. 양수 상태/자기 존 아님/marker 비영 관문 뒤 owner→항상unpop→현재flags에 따른높이/정규화투영→항상move를 수행한다.

종속 finder는 원문 checking/메타모드/dead2/권한의 진단 관문→현재head→next 선저장→TYPE<70을 따른다. 004b1bd0은 모든 Form 판정이 아니라00541074의 **전체DWORD 타입과 같은지**를 검사하며,004b1bf0은10이상/현재 signed Process 상한 미만이다. 이 둘을 제외한 후보의raw+12 Gump handle을 조회해, 존재하는 실제 노드의visible만 변경한다. 새 free/dead 필터나 링크 정규화는 추가하지 않았다. 물리 밖/순환은 호스트 예외로 거부하며 같은 물리의 호출 전체 생존을 요구한다.

참가자 목록은 최초0이면 반환하고, 재그리기 뒤00593150을 다시 읽어 현재 노드의visible을 켠 뒤 현재float x/y·parent0/mode0으로 부착한다. 재그리기가 목록 슬롯을 바꾸는 원문 효과를 보존한다.1097 문서/보고서의 raw+0B bit0 이름도 모호한 dead 표기에서 실제 비트 관문으로 정정했으며 조건은 바꾸지 않았다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0이다. 여섯 원문 함수/관련 전체PE·순회 상한/스택 별칭·현재표/포인터 재조회와 소스를 정적 대조했다. 보호2800파일 불변·새fixture0이며 새C++/MP/게임/통신/새R·원본R/전체검사/34미션/ZIP 실행0이다. 컴파일한 본체를 실제 메타 표시/Poll 접합 또는 MP 동작 검증 완료로 세지 않는다. Netstorm.exe 12,397,568바이트/SHA-256 `0cae209ca27eaad1a75bbad0508984192e1f25302c4e360a494abbf81ca6736b`,새R.exe 669,696바이트/SHA-256 `e979f036b8abf59d30daf0f9b1e652e109e55cbb79d0671c9558f2f79e001451`다.

다음은0047dd60 전체·CS 실제 수신/가상표·같은 Pool 객체/카메라/Gump/HTML 장치를 완성해1095송신/1096투영/1097갱신/1098Poll과1dh/28~33 등록을 연결하는 작업이다. 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오UI도 남아 전체 목표는 미완료다.

보고서는`cpppj/meta-display-poll-20261108.json`, 원문/PE·빌드/정적감사는`extracted/cpp-resume-1098/`(Git제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 재디컴파일/생성소스 변경0이다. 사용자 지정 검사 제외·폐기 검증/공식서버 미접속·원본 보호를 유지하고 AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
