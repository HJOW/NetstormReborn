# 1027 — 세 아비어리 전체 수신과 같은 PID

10.78 Sun38h/Rain73h/Wind74h의 `0045e410/0045e5d0/0045e4f0`을 같은 Session/생성자/Kernel에 연결했다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이며 이번에는 원문과 보호 PE를 읽기만 했다. 이 수신 부모/decoder는 PATCH 범위이고 CD 별도 ABI와 섞지 않는다.

신규 free Form은 ff dirty 진단→실제 기본 생성자→decoder→현재 TYPE/명시 Form SID/flags10의 기존 BaseProcess.Attach 순서다. 기존 live Form은 dirty&1==0 진단 뒤 같은 raw PID의 outer 필드만 갱신하며 재부착/OnAttached/현재 시각 덮어쓰기가 없다. 명시 Form0·WORD/물리 풀 밖 DWORD SID는 호스트 표현 오류다. 각 원문 파일/행의 진단이 정상 복귀하면 처리도 계속한다.

Sun/Rain은 같은0045b0f0을 새 `ReadAviaryProcessData`로 공유한다. 부모DWORD는 별도 출력이고 dirty2는 raw16의8BYTE, dirty4는 raw24의4BYTE다. 현재 dirty/inner 소유권/counter는 보존한다. Wind는 기존 RawWindAviaryState.ReadProcess/45b210을 재사용해 다섯 필드의 원래 비트/폭/순서대로 복원한다. 계산 없이 복사하므로 double 비트와 미선택 바이트가 유지된다.

원문 decoder는 실제 객체 필드가 선택되지 않으면 this NULL에서도 반환한다. 후속 대조에서 이 경로를 확인해 빈 span을 허용하고 실제 선택된 저장에서만 호스트 오류로 드러내도록 보완했다. PID 미발견을 새 객체로 대신하지 않는다. Sun/Rain은 동일한 outer+20/48BYTE 배치와 decoder를 사용하므로 현재 두 클래스의 공통 필드를 직접 빌린다. Wind의 기존 전체 패킷 크기 선행 검사는 호스트 버퍼 경계로 유지한다.

세 TYPE005410f0/0054115c/00541160은 보호 PE의32/59/60을 가진 별도 장기 GameAux DWORD로 소유하고, 기존 Session의 생성/조회 참조와 수신이 같은 주소를 읽게 했다. decompiler의 extraout_ECX+20/+38 부착은 PE에서 실제 this/인자 순서를 확인했다. 세 패킷은 observer NULL/버전1/정책0/가변 길이이며 Editor 충돌 확인/반납 목록에도 포함한다.

첫/보완 Release 빌드는 각각 종료0·경고/오류0이다. 제품1027/11.01은12,014,592바이트, SHA-256은 `0cd3fcfb7f1acc438ee7eeef17ef2d0e157efef8f4537a2f3c62342a93f78e9b`다. 신규MP/단위/게임/통신·공중3종 전용 검사와 fixture 생성은0이며 보호2800파일 불변이다. 정적 복원/컴파일을 생산·비행·LAN 동작 통과로 세지 않는다.

다음은 비행39h/애니메이션3bh·3ch 등 나머지 전체 decoder와 재동기화/Activate 부모다. 내용물 통지의 PlayerKnown 실제 binding, 메타/Outpost/checking5, 한국어 후속, Edit 시나리오 UI도 남아 있다.
