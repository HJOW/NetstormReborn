# 988 — 참가자별 월드 동기화와 대상 지정 송신

2026-11-06, HJOW-Athlon. 마지막 정밀 디컴파일은 같은 호스트의 2026-11-03 자료다. 사용자 지시에 따라 멀티플레이는 원문 근거로 구현하며 신규 단위 검사·게임/소켓/실제 통신 검사는 실행하지 않는다.

## 원문 재대조

`004ac260`은 디컴파일에서 송신 인자 추적이 잘못됐으므로 원본 PE의 `004ac260..004ac32e`를 Capstone으로 읽어 확인했다. 대상은 첫 인자, selector는 두 번째 인자다. selector가-2가 아니면 +54에 buffer와 세 번째 인자를 전달하고, 양수 반환 뒤 첫 인자가-1이면 방송, 그 외는 `0048f4b0(target)`의 현재 Player 접속 비트 뒤 `004e2dc0(target,buffer)`로 보낸다. 네 번째 flags는 이 분기에서 읽지 않는다. 대상별 송신을 기존 방송 Pop으로 대체하지 않았다.

`004ae360`의 기계어도 대조했다. ProcessForm+54는 현재 부모가 client SID인지 진단한 뒤 현재 PID 객체의 writer+8에 `(buffer,부모변경0,세 번째 force 인자)`를 전달한다. force를 부모 변경 bool로 합치지 않는다. 기존 Gun/Path/비행/Aviary/Timer writer가 작성한 실제 버퍼를 새 `ProcessFormTransmission.packet`으로 조회하고 같은 공통 송신 버퍼에 복사한다. 별도 방송을 실행하지 않는다. WindAviary의 기존 원문 force 경로도 클래스 API에 전달한다.

공유 Regular `00496e00`은 `clientSidAllowed`가0이면0을 반환하고, 비영이면 현재 헤더와 부모 WORD·double 예약 시각·event·payload·count를28BYTE에 쓴다. PE 헤더는 message7e/protocol1/length28이다. 원문이 쓰지 않는+4/+5는 보존한다. 실제 일반/공유 생성자의 writer 구별을 Regular 객체에 보존하며 raw TYPE 변경만으로 추측하지 않는다. 일반 Regular는 원문의 빈 writer다.

## 실제 월드 연결

- `RawPeerWorldSync`는 `004a7350`의 client SID 허용1→`00433100(1)`→현재 target→서버SID15000..23000의 surface 먼저/그 외 다음 순회를 수행한다. free bit1 없음·TYPE>69만 적용하고 TYPE.flags1+e8의0x800을 읽는다. owner 캐시는 두 모드 변경 때 각각-99로 초기화한다.
- `004a6d00`의 child WORD+6/sibling WORD+4 사슬을 전송 효과 뒤 재조회한다. 서버15000..23000은 전송/재귀, client5..14999는 TYPE<70 및 현재 `00541164` 공유 Regular인 경우만 전송한다. 임의 owner/free 필터를 추가하지 않는다. 잘못된 사슬의 순환은 호스트 물리 범위 오류로 거부한다.
- 마지막 순회는 TYPE.flags2+ec의0x10000인 walker를 현재 +48(0)/+4c(0)로 공간에서 해제하고 재등록하는 것이다. **986 함수표의 'process 가상 송신' 해석을 정정한다.** PE의 실제 walker VT는 `004afe50`/`0041c0d0`이며 기존 `RawSurfaceWorld::Unpop/Repop`을 사용한다.
- 정상 종료 때만 clientSidAllowed=0이다. 예외가 발생하면 앞선 전역·부분 송신·공간 효과를 임의 되돌리지 않는다. 시계 동기화 token과 별개인 실제 syncTarget을 같은 Client GameAux 전송 상태에 보관한다.
- Session 서버의 `synchronizePeer`를 현재 World의 전체 부모에 연결했다. Player 접속 비트와 실제 현재 per-ID 송신 장치를 사용하며 NULL 연결은 원문처럼 생략한다.

## 검증 범위와 다음 작업

Release 첫 빌드가 종료0으로 끝났다. 최종 검토에서 Path가 다른 writer로 넘긴 뒤 마지막 버퍼 선택 표식을 갱신하도록 보완했고 최종 빌드도 종료0·경고/오류0이다. EXE는11,567,616바이트, SHA-256 `3fd6728ca763774f55ba6c8f166438ba15331ce6baa43620e95fbe797c443367`다. AGENTS.md·원본/CD·fixture2800파일은987에서 새로 고정한 SHA와 모두 일치했다. `cpppj/peer-world-sync-20261106.json`이 기계 판독 기록이다.

신규 MP/단위 검사·통신·게임 실행은 하지 않았으며 LAN 플레이 검증 완료로 표현하지 않는다. 실제 전체 MP/원격 수신 decoder 및 재접속·상태/메타/UI와 일반 전투섬/중앙섬은 남는다. 현재 분배에서 지원하지 않는 파생 process writer는 빈 성공 대신 선택 위치에서 기존 명시 오류를 유지한다. 사용자 확인인 공중3종 전용 실행도 하지 않았다.
