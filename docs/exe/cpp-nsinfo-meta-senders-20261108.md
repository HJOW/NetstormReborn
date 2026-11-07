# 1108 — NSInfo 입력용 실제 상태/자료 송신

제품1108 / 표시·파일·제품버전11.01이다.1107(`cd5dc8f8 / 20261108-32 CP`) 뒤 입력 caller가 필요로 하는 실제1fh 상태 writer와0042bfb0 자료 송신/0040ae80 관찰기를 MetaConnectionRuntime의 같은 TCP11에 연결했다. 생성/빌드만으로 소켓을 열지 않는다.

상태 writer는 실제0040a300/12BYTE 헤더 뒤 caller가+4/+8과 CS/버튼 등 부분효과를 쓰게 하고 같은 SendMutable(11)을 호출한다. 기존 NotifyMetaMaster도 이를 사용한다. 같은 현재 자기 CS 기록 캡처→헤더→현재 subscriber→flags|2000h의 CS 저장/패킷 저장→송신 관문 순서를 유지하며 실패 시 앞선 쓰기를 되돌리지 않는다. 준비/시작/추방의 실제 UI 호출은 후속이다.

자료 송신은 signed count>=160에서 원문처럼 반환한다. 정상0..159에서는 실제 요청 count+7만ff로 채우고67h/version1·기본 길이4·BYTE키·WORD자료 길이를 쓴다. 원문rep movsd 뒤 남은BYTE 순서로 실제 source를 복사한다. 실제 전송 관문을 통과할 때만67h 관찰기가 WORD5의ffff를 진단하고, 복귀 뒤 현재 WORD를 다시 읽어 헤더 길이+7을 확정한다. 클라이언트 진단줄은0xa07이며 R전용2564를 재사용하지 않았다. 보호 PE의 descriptor67h/version1/BSS WORD0·상태 길이WORD12를 대조했다. 음수 크기/실제 source 부족은 호스트 예외로 드러내며 원문 메모리 오염을 재현하지 않는다.

**NSInfo 세 Handle·메타 참가자 메뉴/카메라와 새 송신의 UI 호출은 후속이다.** 67h는 실제 observer/version/길이만 등록했으며 새 수신기를 합성하지 않았다. CS 수신/참가자·전투섬 생성·카메라 Rig/HTML→1dh/28~33 등록/1095 송신 상태 호출, 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI도 남아 전체 목표는 미완료다.

NetstormCpp·NetstormRoot Release/x64 빌드 종료0·경고/오류0이다. 원문/기계어의 관문·초기화·폭/복사·관찰기·실제 동일 TCP 장치를 정적으로 대조했다. 보호2800파일 불변·새fixture/C++/MP/게임/통신/전체검사/34미션/ZIP 실행0이다. 사용자 제외에 따라 새 MP 검사는 실행하지 않았다. 전체 입력/메타 화면/State19 실행 성공이 아니다.

Netstorm.exe 12,546,560바이트/SHA-256 `f1c46baf4bcc18f8be2c568b7cc543312ab29877064942773bf18b116d27e6ab`, R.exe 669,696바이트/SHA-256 `d4f41e91d8b13aa32f53954130150ae43b229f23d63d509c249ba3cc8619ccd5`다. 보고서는`cpppj/nsinfo-meta-senders-20261108.json`, 원문/기계어·빌드·정적 감사는`extracted/cpp-resume-1108/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 재생성/생성소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
