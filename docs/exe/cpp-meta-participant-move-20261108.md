# 1149 — M1.1 메타 참가자 이동과22h 수신

문서 정리1148(`12baa7c1 / 20261108-117 CP`) 뒤 새 계획M1.1을 구현했다. 제품1149 / 표시·파일·제품버전11.01이다. 직전 제품은1147이다.

4b4820의 메타 모드/CS0 State.cpp d3c 진단 뒤 전체42c2e0을 연결했다. 등록은 원문 빈 observer/version1/policy0/고정17BYTE이며 새 sender/known/phase/active 관문을 더하지 않는다. TCP의 이동된 input vector, zlocal의 이동된 queue vector와 UDP 소유 수신 frame을 사용한다. 기존 RawPacketFrame 집합 observer가 같은 mutable 버퍼를 넘기는 계약을 확인했고,22h의 packet5 bit8도 사본을 만들지 않고 원문 중간 위치에서 지운다.

진입CS/참가자 포인터, 초기owner/자기 전투/옛 전투, 전투 변경 여부와 NSInfo 필요 여부는 지역값으로 보유한다. 좌석/전투가 바뀌면 원문 packet8 표시의 추방 효과→당시 자기 subscriber의 kickedoff(priority1)→현재 전투0 진단→옛 좌석0/전투 표시/같은 CS 버전/기록master bit 제거→새 좌석 진단/쓰기·전투 표시/버전·현재 위치 thundercrack→현재known/현재owner의 anticipation 음악 순서다.

479cf0/478720 슬롯 효과는 현재 전투 active 진단이 복귀한 뒤도 invalid면 반환한다. 슬롯 범위 진단 뒤 실제 Pool 객체 TYPE/C 프레임/PE540b68의8자리 표/기존 RiftShapeFrame 포구를 읽고, 현재 Snapshot의 기본 칸 크기로 당시 객체 위치를 보정한다. 같은 Session.hits.Impact().Ground의 clamp→Dude→공유Next(100)→효과3/0.02를 호출하고 원래 보정 위치에 explodeSlot.wav를 재생한다. 지면 clamp 값을 음향 위치에 잘못 재사용하지 않는다. 기존 항해의 C프레임/포구 provider를 공통 함수로 옮겼으며 원문 토큰 순서를 정적으로 비교했다.

그 뒤 실제packet bit8을 지우고 WORD를 signed로 확장해 현재 기록flags와 XOR한 변경 마스크를 보유한다. dropPiece 음향 뒤에는 packet WORD를 다시 읽어 전체DWORD flags에 쓴다. 새 전투/슬롯·packet 좌표 또는 기존 좌석 좌표를 기록하고 실제 float가 달라진 경우만 Rig 목표를 갱신한다. Rig 뒤에도 같은 지역 좌표를 다시 쓰는 원문 꼬리를 보존한다. master 전환이면 같은 CS Battle+59의87BYTE를 기존41cb90 초기화 본체로 재설정하고, 현재packet 전투를 다시 읽어 기존42bfb0/67h/87BYTE 실제TCP11 송신으로 연결한다. global 옵션을 임의로 먼저 적용하거나 ACK를 합성하지 않는다.

전투 변경의 자기 참가자는42f3d0(0) 후 현재 전투가 있으면42f3d0(1), 다른 참가자는 필요할 때42f330 멤버 갱신만 수행한다. 기존 ClientChatRuntime의 접두/수신대상과 멤버 갱신을 분리하고 Clear8/known/현재CS/8슬롯/Add 루프의 토큰이 이동 전과 같음을 확인했다. 두 모드 호출의 기존 동작은 유지하며 멤버만 갱신할 때 불필요한 접두 변경을 만들지 않는다.

마지막은 조건부 NSInfo→현재packet 참가자ff 표시→현재48f190 metadata→현재CS의 같은 참가자42b490 이름표 서식이다. 이름표를 먼저 갱신하는 쪽으로 원문 순서를 바꾸지 않았다. 이후 frame/Poll이 새 서식을 표시한다.

4b4820/42c2e0/479cf0/478720/41d180/470ac0/41cb90/42bfb0 C/PE·고정17BYTE/실제 mutable 수신 소유·캡처 CS/owner·현재 flags 재조회·Snapshot 포구·같은 지면 효과/난수·Chat 자식/포구 provider 토큰 불변·후행 현재CS 이름표를 대조했다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0, 보호2800파일 불변이다. 새 C++/MP/게임/통신/자료조회/전체/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다.

Netstorm.exe 13,078,528바이트/SHA-256 `28e93e81919f32a49b3f4980a86600dd9dfa1658363f29c1dd4bd4ebf062fb85` / R.exe 669,696바이트/SHA-256 `2e7970ddad6990a87fb3e87b55f235471332bc62cfcb3a4c7e1d8bdecc6448b6`

최종 근거/빌드는 Git 제외 `extracted/cpp-resume-1149/`다. 마지막 전체 디컴파일은2026-11-03/HJOW-Athlon, R전용은2026-11-07/HJOW-Athlon이며 생성 원문은 수정하지 않았다. 실제 이동/장면 안정성 또는 x87/원문 스택/OS 실패 자체 일치를 실행 검증한 것으로 세지 않는다.

M1은 부분 완료이며 **다음은M1.2의25h 항해 오류/실제 객체 안내**다. M2~M8과K/S/기존 후순위도 유지한다. 전체 목표는 진행 중이다.

첫 빌드는 반환 형식 NativeMetaSeatHooks의 전방 선언 누락으로 종료1(오류 진단9건)이었다. 헤더 선언을 추가한 뒤 위 최종 재빌드가 종료0·경고/오류0으로 완료됐다. 첫 실패 로그도 같은 보조 폴더에 보존한다.
