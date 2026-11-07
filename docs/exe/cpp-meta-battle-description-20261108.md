# 1132 — 일반·메타71h 전체 설명 수신과 실제 표시 연결

제품1132 / 표시·파일·제품버전11.01이다. 이전 단위는 `739b04e5 / 20261108-94 CP`로 커밋됐다.

`0040b0d0/0040b330/004b49c0/0042cad0`의 전체 원문·다음 함수 경계까지 기계어와71h 등록/보호 PE 길이WORDffff를 대조했다. 일반 Session과 MetaConnectionRuntime이 같은 observer·State 부모를 등록한다. 현재 CS가0이면 실제 자식/packet 본문을 읽지 않고 반환하며 mode/sender/known 관문을 추가하지 않는다. 송신 observer는 FF길이 진단 뒤 현재길이를 다시 읽어 WORD2=len+10을 쓴다.

자식은 active가 없으면 로그만 반환한다. 길이>41이면 로그 뒤 현재packet번호의 전투 표시만 갱신한다. 정상은 캐시 길이의 DWORD/BYTE를 복사하고 현재packet길이+f8에NUL을 쓴다. 길이0이면f8을 쓰며f9를 강제로 빈 문자열로 만들지 않는다. 그 뒤 현재f9부터NUL까지7ASCII 제어 문자만 공백으로 바꾼다. 꼬리 지우기/locale 분류/빈 길이 정규화를 추가하지 않는다.

복사·정제 뒤 같은 기록+20 SID의 실제 가상88을 호출하고 현재packet4를 다시 읽어 기존479c40으로 특정 전투만 갱신한다. 전체 Poll/다른 참가자 갱신이나 가짜 표시로 대체하지 않는다. 바인딩만으로 World/CS를 요구하지 않고 선택된 효과 위치에서 실제 현재 장치를 빌린다.

사용자 지시대로 신규 단위/MP/게임/통신 검사는 실행하지 않았다. 메타 객체 입력·일반 선택 캡처·나머지 CS 수신/참가자 생성은 후속이다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 컴파일을 전체 메타 장면·MP 실행 성공으로 쓰지 않는다.

다음은 메타 객체 입력/일반 선택 캡처 접합/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출 → 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI이다. 전체 목표는 미완료다.

Netstorm.exe 12,791,296바이트/SHA-256 `e24bade2324b5eb85349348ff5f4785096496a289ec5967002bcb5143d71b65c`, R.exe 669,696바이트/SHA-256 `a9086b97dc30ca25e49e42d148bd9c62c28d26d9d330f09a8ffa914aedaa4026`다. 보고서는 `cpppj/meta-battle-description-20261108.json`, 원문/기계어·빌드·정적 감사는 `extracted/cpp-resume-1132/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 생성 소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
