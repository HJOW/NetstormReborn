# 1168 — M6-b2 Client 0bh 버전조회와 실제 응답

2026-11-08 / HJOW-Athlon. 이전 CPP1167/1916bd66, 표시·파일·제품11.01이다. Client 정밀2026-11-03·R2026-11-07/같은 호스트의 기존 생성C·보호PE를 사용했으며 재디컴파일/생성 소스 변경은0이다.

Client 기본 등록에 원문409fe0 observer/version1·policy0/가변길이/4b3af0 수신을 연결했다. 같은 세 실제 생성 경계의 InstallClientPacketDefaults를 사용하므로 Meta에만 국한되거나 짧은 Session에 묶이지 않는다. observer는 sending0에서 packet을 읽지 않고, 송신 때 DWORD+8의-1을0x36c에서 진단한 뒤 현재 lowWORD+12를 선언WORD에 쓴다. 0ch의 별도0x38c 진단과 구별한다.

수신 본문은 sender만 사용한다. 요청 payload·connection·phase·known·server 조건을 새로 검사하지 않는다. RawGameAuxState에 원문0052ed54/55의 현재0c/1 BYTE와00544c24의 BSS WORD0을 보관하고 수신 시 읽는다. 이 값은 원문0ch 공용 생성 metadata이며 World/Session 교체 때 다시 초기화하지 않는다. 원문에 없는 Config키/활성 writer를 추가하지 않았다.

4b3af0→df590은 실제50BYTE 전체를FF로 초기화하고 현재 message/protocol&7f/최소길이4를 쓴 뒤 WORD10/78만+4/+6에 쓴다. +8 extraDataLenDWORD는ffffffff 그대로다. UI11.01과 wire10.78을 구별하며0/12/50으로 정상화하지 않는다. 생성한 같은 지역 mutable 예약을 현재 Registry.SendMutable에 한 번 넘긴다. 실제 현재 sender/Player/NULL 관문 뒤 선택된 가상송신에서만1167의0c observer가 실행된다. 음수 pending의 비트를 보존하며 별도 송신 전 관찰/가짜 연결·ACK를 만들지 않는다.

현재0c observer는0x38c 진단이 정상 복귀한 뒤 lowWORD를 새로 읽는다. 버퍼가 그대로면ffff+12의 하위WORD가11이 되어 실제 선언은11이다. 진단이 중단되면 뒤 길이쓰기/전송은 수행되지 않는다. 이 동작은 원문 근거를 따른 것이며 로그인 성공이나 정상 응답임을 의미하지 않는다. 기본 callback은 장기 aux/Registry/Player owner만 보유하고 Session/World/Meta this를 캡처하지 않는다.

추가 대조에서 원문4a6990의 debugBurst는 같은0c metadata를 사용하는63000BYTE×3 송신임을 확인했다. 0054dbc4는 BSS0이고435220의reset0 외에 활성 비영 writer를 발견하지 못했다. 현행 Login의 지역burst0/비영 미복원throw는 **M6-n의 남은 코드 복원**으로 보존한다. 이번 버전조회 연결을 debugBurst 전체 복원으로 세지 않는다. 28h 로그인 응답4a6920은 별도 경로다.

원시는 extracted/cpp-m6-version-query-prep/의 함수C·전체range PE·주소참조/BSS 근거와 extracted/cpp-resume-1168/의 range별SHA·빌드·보호해시다. 최종 NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0, 보호2800파일 불변이다. 새 C++단위/MP패킷/게임/통신/ZIP 실행0이며 사용자 지시로 MP 실행 검사는 생략했다. 제품과 원문 해시는 [보고서](../../cpppj/client-version-query-20261108.json)를 따른다.

M6-b1 기본 등록과 b2 버전조회 연결을 마쳤다. 다음 M6-c 원격3fh 건설 통지의 lazy decoder/Place/실제 등록·동일 버퍼 writer를 진행한다. M6-c~n/M7/M8와 한국어·시나리오UI는 미완료다.
