# 1151 — M2 선행 Client 패킷 기록 파일과 실제 송수신 연결

제품1151/표시·파일·제품버전11.01이다. 직전은1150(`d479b400 / 20261108-119 CP`)다. **M2 전체 완료가 아니다.** 실제 Open을 호출할1ah/1bh 부모와 가변 Zone 소유권/State7 연결이 남아 있다.

ClientPacketRecord는 원문005e4770 FILE/005e4774 enabled·0054dbac 요청 DWORD를 같은 Client 수명에 둔다. 생성은 현재 FrameTime의 실제 owner와 사용자 홈 정책만 pin하고 파일/폴더를 만들지 않는다. 메타/World/Session을 바꾸어도 요청/기록 파일을 초기화하지 않는다. 요청 DWORD는 원문 최초0이며 임의 Config 키/새 CLI로 기록을 강제로 켜지 않는다.

4debe0은 이전 enabled0→FILE 닫기/0→enabled1→같은 이름w+b다. 자동 파일은 사용자 요구에 따라 Saved Games/Netstorm/Diagnostics 아래로만 옮긴다. 명시 UserDataRoot가 없으면 Open에서 기존 Resolve 정책을 사용하고 설치 폴더로 돌아가지 않는다. 사용자 폴더 확보/물리 오류는 명시 호스트 경계이며 fopen 실패는NULL FILE/enable1을 보존한다. Close는 요청 DWORD를 바꾸지 않고 destructor는 자기 FILE만 닫는다.

4dec30은 진입 연결의 주소+4 DWORD와 sender를 보유하고, enabled0에서 아무 파일도 쓰지 않는다. enabled1/NULL FILE는 원문 NULL 쓰기 위치의 호스트 오류다. 실제 방향 BYTE→현재 FrameTime.wall DOUBLE→현재 number의하위DWORD→주소DWORD→senderDWORD→현재 packet WORD+2 길이의 본문 순서로6회 fwrite하고 flush한다. 패킷 선언 길이 읽기는21BYTE 접두 뒤다. 다섯 번째 전달 길이를 본문 길이로 대체하지 않고 fwrite/flush 실패로 앞선 부분 효과를 롤백하지 않는다. 현재 클론의64비트 프레임 번호를 원문 기록의32비트로 명시 축소한다.

메타 수신의 빈 record callback을 제거했다. 일반 LAN/내부/편집도 같은 owner를 빌리며 방향0 기록은 기존 Session::RecordConnectionPacket/관측보다 먼저다. 기존 wireTypes/관찰 callback은 유지한다. 실제 receive 분배기의 type>9/집합 내부 observer 순서를 바꾸거나 별도 수신을 합성하지 않는다.

PacketDispatchHooks에 선택적 sent 장치를 추가했다. 기존 독립 장치는 빈 채로 같은 동작을 유지한다. TCP/내부/편집은 원문4dff00처럼 실제 observer/복사·버퍼 쓰기 뒤 해당 원본 packet을 방향1로 기록하고, TCP Flush의 합성 type4를 중복 기록하지 않는다. TCP 실제 쓰기 실패는 원문처럼 기록 전 반환한다. UDP4e00e0은 실제 쓰기/실패의 종료 통지 뒤에도 기록하는 원문 순서를 지켰다. 같은 TCP/UDP CallGuard가 기록 callback 중 실제 노드 저장소를 pin하며 장기 기록 owner도 강한 참조로 빌린다.

R의 별도 시계/기록 본체는 바꾸지 않았다. 근거로 읽었지만 Client 시계와 R 시계를 공유하는 것으로 대체하지 않았다.

4debe0/4dec30/4dff00/4e00e0 C/PE와 수신 callsite를 대조했다.6회 fwrite/후행 현재 WORD 길이·flush, 공유 실제 프레임 wall/low32번호·진입 주소/sender, disabled/NULL FILE/부분 효과·사용자 홈, 가상 송신의 observer/복사 뒤 기록·UDP 실패 후 기록·같은 장기 owner를 확인했다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0, 보호2800파일 불변이다. 새 C++ 단위/MP/게임/통신/기록 파일/전체/ZIP 실행0이다. 원시 근거/빌드는 Git 제외 `extracted/cpp-resume-1151/`에 보존한다.

Netstorm.exe 13,108,736바이트/SHA-256 `d9f8ef197c79ccab31346960ca8b73dc418c05528554c42fced6aa0343236338` / R.exe 669,696바이트/SHA-256 `c9512cc6932d241b562b9920ef820885c498e57194b01f58248157fbaf2aabdb`

마지막 전체 디컴파일2026-11-03/HJOW-Athlon, R전용2026-11-07/동일 호스트이며 생성 원문을 수정하지 않았다.

**다음 M2.2:** 4dece0의 실제 가변 Zone 복제 owner를 현재/다음 State+80 소비자와 연결한다. 이후1ah/1bh의 checking/상태·선택적Open·현재 문자열/실제10·11 연결 정리/상태 직접 실행을 연결한다. 단독 미션86BYTE 생성기를 메타 가변 복제로 대신 사용하지 않는다. 전체3차 잔여는 여전히M2~M8의7묶음이다.

첫 빌드는 종료0·오류0·경고2건이었다. UDP 길이의명시 uint16 변환과 앞선1150 안내 hook의지역이름중복을 교정한 뒤 최종 재빌드가 종료0·경고/오류0으로 완료됐다.1150의 첫 빌드 로그에도 C4456 1건이 있었음을 확인해 당시 보고서에 이를 추가했다. 당시 최종 증분 빌드의 경고0과 첫 컴파일의 경고1을 구별한다.

435220의 실제 설정 writer도 확인해 소리 옵션 Read 전에 Requested만0으로 재설정하도록 연결했다. 이미 열린 FILE/enabled는 그대로 유지한다. 이 writer를 포함한 최종 빌드가 위 기록이며 경고 수정 직후의 별도 성공 로그도 build-before-options-writer.log에 보존했다. 추가 활성 요청 writer 유무는 M6에서 전수 판정한다.
