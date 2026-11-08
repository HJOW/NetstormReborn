# 1152 — M2.2a 전체 패킷 복제와 장기 Zone/State7 소비자

제품1152/표시·파일·제품버전11.01이다. 직전은1151(`af1fbf6c / 20261108-120 CP`)다. **M2.2a 완료이며M2.2/전체M2 완료가 아니다.** 다음은 메타 화면의 실제 일반 State7 부모다.

RawPacketCopy의 전체4dece0은 입력 포인터NULL이면 등록/헤더를 읽지 않고 반환한다. 현재 type의observer(1)를 같은 mutable 입력에 호출한 뒤 현재 WORD+2 길이를 읽는다. 목적지가NULL이고capacity0이면 그 길이를 자동 할당한다. 나머지는 signed capacity가길이보다 작으면NULL을 반환하고 외부 목적지는 그대로 사용한다. 원문 PE의 CMP/JG와 REP MOVSD→MOVSB에 맞춰 낮은 주소부터 DWORD 수와 남은0~3BYTE를 복사한다. 중첩 주소를 memmove로 바꾸지 않으며 부족 물리는 해당 DWORD/BYTE 접근 위치에서만 호스트 오류가 된다. 앞선 observer/필드 쓰기를 롤백하지 않는다.

결과는 반환 포인터의 존재와 byte span/소유자를 구별한다.0길이의 소유 할당도 원문NULL과 같은 것으로 세지 않는다. 호스트 vector의0길이 주소 자체 대신 present/할당 owner로 이 차이를 표현한다. std::shared_ptr pin은 C++ 저장소 수명 경계이며 성공ACK/원문 새 상태가 아니다.

RawConnectionRegistry::StoreConnectPacket의 기존 실제 Caller를 이 본체에 연결했다. 이전 버퍼 해제/slot0 뒤만 복제하고, 완성된 소유 할당을 기존 opaque token 장부에 넣은 뒤 slot을 공개한다. observer를 두 번 호출하거나 또 다른 byte 사본을 만들지 않는다. 원문 일반 목적지/용량 분기도 함께 복원했으며 실제 접속 Caller는 기존과 같은 자동 할당 분기다.

ClientLocalStateGlobals가 같은 RawPacketCopies를 장기 소유한다. 원문4dece0 자동 복제는 Duplicate, 이미 완성된 일반 LAN Zone은 Adopt로 같은 vector byte 할당을 이동한다. token0은NULL이고 성공한 실제 소유 할당만 단조 DWORD token으로 보관한다. Acquire는 현재 token의같은 바이트를 pin하고 Release는 해당 token을 한 번 반납한다. 임의 Session 반납으로 장부를 비우거나 호스트 포인터를DWORD로 자르지 않는다.

기존 일반 LAN 호스트의 요청은 고정1 대신 실제 가변 Zone token을 next+80에 쓴다. 기존 client/독립/복구 입력의벡터 계약과 단독 미션86BYTE 본체는 유지한다. 일반 State7은 정리 전 zonePending 캡처로 설치 여부를 고른 다음, 당시 current+80으로 실제 바이트를 읽어 기존 InstallLocalMissionZone에 전달한다. 복귀 후 **현재 current+80을 다시 읽어 free**하고, 실제 반납 뒤 current/next+80만0으로 만든다. immediate+80은 원문처럼 건드리지 않는다. 서버 시간 처리기 등록도 반납 뒤의 기존 위치다.

호스트 취소 소유권도 실제 free 뒤만0으로 표시해 후속 예외/취소가 같은 token을 다시 반납하지 않는다. 취소는 자기 token만 해제하고, Native 수신이 덮어쓴 다른 next7/Zone 표식을 자기 대기실 요청으로 지우지 않는다. 설치 중 오류이면 앞선 원문 효과와 아직 반납하지 않은 실제 소유권을 유지한다.

4dece0 C/PE의NULL/observer1/현재WORD/할당·signed 용량 비교/REP MOVSD→MOVSB를 대조했다. 같은 mutable 입력/전체 소유 할당·Registry 실제 Caller·Client 장기 token·일반 State7의현재 토큰 재조회/반납/예외 prefix·취소 소유권·미션 본체 불변을 확인했다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0, 보호2800파일 불변이다. 새 C++ 단위/MP/게임/통신/전체/ZIP 실행0이다. 원시 근거/빌드는 Git 제외 `extracted/cpp-resume-1152/`에 보존한다.

Netstorm.exe 13,112,320바이트/SHA-256 `d7ae8026d1f36e85934e1b0bd511bb41dca1bad17f49539805254eba69ed5793` / R.exe 669,696바이트/SHA-256 `c776f8f149ca792cfb979c2efe467af3edb28531f2fe28266945d677cfa0c4a2`

마지막 전체 디컴파일2026-11-03/HJOW-Athlon, R전용2026-11-07/동일 호스트이며 생성 원문을 수정하지 않았다.

**M2.2b 잔여:** 현재 메타 분배기의State7은 일반 Starting 장치/World 설치와 접합되지 않았다.1bh는 수신 안에서 현재7을 쓰고 Run을 직접 부른다. 현재MetaConnectionRuntime의pumping/cleanup 재진입 관문, cleanup 때 메타 owner 반납, 현재Player base와같은TCP9/등록표·실제 접속 장치를 함께 인계해야 한다. 보호를 없애거나 임의 다음Tick으로 바꾸거나 LAN 대기실 ACK를 만들어 대신하지 않는다.1ah/1bh 전체 checking 부모와 선택적 기록Open도 아직 남아 있다.3차 잔여는M2~M8의7묶음이다.
