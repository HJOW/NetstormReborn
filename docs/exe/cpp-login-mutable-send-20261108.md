# 1166 — M6-a 실제 mutable 송신과 기본 관찰기 연결

2026-11-08 / HJOW-Athlon. 이전 CPP1165/aa91f53b, 표시·파일·제품11.01이다. 마지막 클라이언트 정밀 디컴파일2026-11-03/같은 호스트, R2026-11-07/같은 호스트이며 생성 소스는 바꾸지 않았다.

1165는 네 관찰기 본체만 복원했으며 실제 일반 writer는 const Send로 관찰기를 우회했다. 이번에는 로컬 버퍼를 소유한 MissionLogin/Server/Fort의 세 send hook을 mutable span으로 바꾸고 실제 Login의 SendMutable→선택 backend→현재 연결/Player 관문→observer→현재 선언WORD→wire 경계를 연결했다. 수신 relay/settings28h/pendingReply/broadcast의 기존 const 계약은 유지한다. const_cast로 수신 입력을 수정하지 않는다.

2ah는 원문4a7240처럼 image+20의 전체 예약을 넘기고 실제40a540 뒤 image+17의 wire를 선택한다. 2bh는48f9f0의100BYTE 예약과 원래 초기 길이를 보존한다. 두 writer의 선행 길이쓰기/범위 축소를 제거했다. BroadcastConnectionPlayer도 별도 사본을 만들지 않고 같은 로컬 예약을 현재 Registry.SendMutable에 넘긴다. 67h/81h는4a7800/4a6e70→4df590의 최소 초기 선언WORD4와 전체94/44BYTE 예약을 구별한다. 81h의 현재 lowWORD+2 감김 뒤 shift, 진단 정상 복귀 뒤 필드 재조회는1165 본체를 재사용한다. 같은 server hook의72h도 기존 metadata 관찰기40b110이 실제 송신 뒤 길이를 확정하도록 선행 계산을 제거했다.

Login이 없는 Editor/Meta에서도 Player 상태 방송이 선택될 수 있다. ClientCampaignTransport의 공용 InstallClientMissionSendObservers를 일반 coordinator 생성/재연결 후 BindSession 전, Meta TCP/UDP 공유표 생성 후, EditorLocalTransport 생성 후 Open 전에 연결했다. 네 관찰기는 같은 Client storage와 type만 보유하고 Session/World/Meta this를 캡처하지 않는다. receiver/version/길이 등록을 새로 만들지 않는다. Login의 임대 등록은 기존 snapshot/반납 계약을 사용하므로 반납 뒤 기본 관찰기가 남는다. Meta의 중복67h 관찰기만 제거하고 기존 버전/길이/수신 본체를 유지했다.

RawMissionClientPackets의 상태 없는 observer 본체를 공용 함수로 분리해 기본표와 Session이 동일 수식을 사용한다. 고정28/29/84/6e는 실제 빈 방향 관찰기로 유지한다. standalone FinalizeMissionFortPacket도 진단 복귀 뒤 현재 lowWORD를 다시 읽는다. 기존 네 검사 파일은 전체 예약을 받아 실제 관찰기 뒤 wire를 캡처하도록 어댑터만 맞췄다. 원본 기대 TSV·사례·기대값 변경과 새 검사 추가는0이다.

원문 근거는 네1165 관찰기와4a7240/48f9f0/4a6e70/4a7800/4df590/40b110의 전체C 및 PE다. 이번 원시 native-evidence.json은 함수표의 모든 inclusive range를 각각 읽고 개별 SHA와 연결 SHA를 저장한다. 과거1161/1162/1164 일부 peSha256은 entry부터 함수size만큼 읽은 연속 window였다. 원래 값과 그 의미를 보존하고 정확한 비연속 range 근거를 세 JSON에 추가했다. 생성C는 이전에도 전체였고 해당 구현 명령 branch는 기존 window 안에 있었다. 일부 후미/SEH 복원 구간까지 새로 대조했으며, 해시 범위 정정을 새 제품 기능 구현으로 세지 않는다.

최종 Release/x64의 NetstormCpp·NetstormRoot·기존 netstorm_tests 코드는 모두 컴파일 종료0·경고/오류0이다. 기존 검사 target의 dependency인 help probe도 빌드만 했으며 실행하지 않았다. 보호2800파일은 전수 SHA가 동일하다. 새 C++단위/MP패킷/게임/통신/ZIP 실행은 모두0이다. 컴파일 성공은 로그인 동작 검증이 아니며 사용자 지시로 MP 실행 검사는 생략한다. 바이너리 해시와 빌드별 로그는 [기계 보고서](../../cpppj/login-mutable-send-20261108.json), 원시는 extracted/cpp-resume-1166/에 보존한다.

M6-a는 부품1165와 실제 caller/기본표1166으로 완료했다. 다음 M6-b는 원문 C3 일곱 수신과 관찰 전용 metadata의 장기 기본 등록이다. M6-b~n/M7/M8과 한국어·시나리오UI는 남아 있으며 전체 목표 완료가 아니다.
