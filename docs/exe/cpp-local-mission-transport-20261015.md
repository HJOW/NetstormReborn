# 단독 미션의 Zone·로컬 연결·0x27 실제 분배

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. AGENTS와 양 LEFT를 다시 읽었고 같은 PC의 `extracted/refined/originals/{Netstorm.c,functions.tsv}`를 사용했다. 원본 게임·복사본·GUI·OS 소켓을 실행하지 않았다. 보호 원본·AGENTS·dotnetpj는 변경하지 않았다.

## 원본 순서와 이번 제품 경계

`004b51e0`은 현재 Fort 이름을 준비한 뒤 Zone을 만들고 pending State7을 예약한다. State7의 `004b4be0`은 Zone의 subscriber와 현재 mySubscriber를 비교하여 로컬 Player를 선택하고 이름/IP/subscriber를 설치한다. 뒤 State8/9는 로컬 서버ID9에 접속하고 `004b63d0`의0x27 로그인을 보낸다. 서버에서 받아들인 연결은 pending-1이며 실제 `CheckLogin`의 `004e27b0`이 이를 owner1로 옮긴다.

**`0054db50`은 설정 `simulconnect`다.** 단독 플레이 또는 zlocal이라는 이유로 이를 true로 만들면 `CheckLogin`의 실제 pending 재배치를 생략하여 원본 경로가 달라진다. 이번 모듈은 이 전역을 만들거나 수정하지 않는다. 통합 검사는 false인 실제 `RawPlayerParticipation::CheckLogin`을 사용한다.

새 `zacket/LocalMissionTransport`의 구현 범위는 다음과 같다.

| API | 실제 연결 범위 |
| --- | --- |
| `MakeLocalMissionZone` | 원본512바이트 예약 전체·네 슬롯 작성과 observer의86바이트 길이 확정 |
| `MakeLocalMissionLogin` | 상위가 공급한 실제 Fort 이미지 이후의 전체 예약 바이트/이름/subscriber/image와 최종 선언 길이 |
| `InstallLocalMissionZone` | 같은 Player9/PlayerWorldState/참조 전역에 실제 Zone 설치, 양수owner의0048f3b0 coreData 선택, Chat 이름/서버 시작의 필수 외부 소비자 |
| `RegisterLogin` | 같은 RawPacketDispatch의0x27/version1/정책0 등록. 실제 큰 로그인 처리기가 없으면 열기를 거부 |
| `Open`/`AssignConnection` | 서로 다른 실제 클라이언트9/서버 pending-1 저장소를 만들고, 실제 현재 연결을 owner로 이동 |
| `Send`/`Flush`/`Poll` | 현재 Player flags1과 실제 연결 존재 검사→바이트 큐→type4 집합 프레임→pending 먼저/등록 연결 번호순 수신 |
| `DispatchFrame` | 기존 단일 패킷 분배 및 원본004df740의 지연 없는 집합 버전/시각/기록/observer/receive/depth/종료 순서 |
| `Peer`/`Dispatch` | 같은 RawBattleBroadcast 송신과 건설/서버 응답 등록에 사용하는 실제 소비자 |

패킷을 저장하지 않는 송신 observer나 flags2만 쓰는 로그인 성공 callback을 제공하지 않는다.0x27의 실제 등록 몸체는 상위가 공급해야 한다. 새 전송은 두 실제 host 연결/바이트 저장소를 소유하며 TCP/SPX/Winsock 전체를 복원했다고 주장하지 않는다.

## Zone와0x27의 정확한 바이트

`004b51e0`의512바이트 예약은 `004df590`이 전부0xff로 채운다. 종류0x1a/버전1/초기 길이4, Zone+4=`0x82000004`, +8 WORD0, +10 DWORD0, +14의 numClients=1을 쓴다. `0040a220`의 각 슬롯은 **15+index×71**이며 slot BYTE/subscriber DWORD/flags DWORD/name+9/IP+41 순서다. 네 이름은 `zsimpson`, `jgreer`, `kdemarest`, `tbratton`이고 subscriber1..4/slot0..3을 모두 작성한다.

작성된 네 슬롯이 모두 설치되는 것은 아니다. 복제 직전 observer `0040a1e0(1)`이 길이를 **15+numClients×71=86**으로 바꾸므로 State의 실제 Zone 이미지는 첫 슬롯86바이트다. `LocalMissionPacket::storage`는512바이트를 보존하고 `Bytes()`는 이86바이트만 반환한다. 현재 Fort 이름 준비 `00460980`과 pending State의144바이트 예약/이미지 수명/sequence 증가는 이 순수 작성 API의 외부 부모에 남긴다.

0x27은 원본이 예약한 **FortImage 크기+67**바이트 전체를0xff로 채운다. subscriber는+8, NUL 포함 nameLen BYTE는+12, FortImage 크기 DWORD는+13, 이름은+17에 쓰고 이미지 위치는 **17+nameLen**이다. `0040a450(1)`의 최종 선언 WORD는 **17+nameLen+FortImageSize의 하위 WORD**다. 예약 바이트와 송신 바이트를 합치지 않는다. 원본의 길이 축소/손상된 문자열 때문에 실제 host 저장소 밖에 접근하는 경우는 명시적으로 거부한다.

`MakeLocalMissionLogin`은 Template 생성/현재 Fort 선택/파일 읽기/이미지 저장/Template 정리 몸체를 실행하지 않는다. 실제 상위 부모가 이 절차로 얻은 이미지와 현재 subscriber/handle을 공급해야 한다. 독립 기계어 관찰에서는 이 하위들을 명시 외부 경계로 대체했고 그 호출 순서를 fixture에 그대로 보존했다. C++ 작성 검사는 이 외부 절차 이후의 전체 작성/송신 바이트를 대조한다.

## 현재 Zone 설치와 실제 연결

`InstallLocalMissionZone`은 현재 server/boss를 켜고 pendingStamp/Zone+10/+4를 실제 참조 전역에 쓴다. earlyCaptureTimer가 정지 상태인지 진단한 뒤 현재 loginTimeout+120과 현재 double 시각으로 예약한다. numClients1의 slot0/subscriber1이 현재 mySubscriber1과 일치하면 같은 `PlayerWorldState`의 templateOwner/battleOwner/known을1로 쓰고 실제 Player+6c의10바이트를 현재 coreData로 선택한다. Player flags에는 로컬8만 추가한다. 이름 복사→실제 Chat 이름 등록→현재 subscriber 재조회/저장→IP 복사→선택 flags200 순서다. **flags2나 Ready40은 이 설치가 쓰지 않는다.**

Chat callback 뒤의 subscriber와 다음 반복의 numClients는 원본 위치에서 다시 읽는다. 새 원본 관찰2개는 callback이 첫 슬롯subscriber를9로 바꾸고 numClients를2로 늘리는 입력을 사용했다. 원본과 C++ 모두 Player1의 subscriber9 및 Player2의 subscriber42/이름을 같은 바이트 순서로 설치해야 한다. 알려진 UI 이름 정제/등록은 다른 단위의 실제 `RawChatChannels`에 연결할 수 있다. 이 모듈은 Chat 성공을 합성하지 않는다.

실제 로컬 연결의 새 수명에는 다른 pending이 없으므로 첫 서버 연결은-1이다. 클라이언트9는 buffered이고 서버는 `AssignConnection(-1,owner)`에서 buffered로 전환한다. 재배치는 같은 `PacketConnection`의 sender와 등록 위치만 바꾸며 Player 연결 flag를 직접 쓰지 않는다. `Send(1,...)`은 실제 현재 Player flags1이 없으면 원본처럼 생략한다. `Send(9,...)`의 실제 바이트는 flush 전 출력 큐에 남는다.

Flush는 원본0xff 첫 BYTE를 유지하고 kind4/전체 WORD를 쓰는 집합 프레임을 만든다. 집합 수신은 **현재 시각 기록→버전 정책→record→현재 observer(0)→현재 receive** 순서다. 기존 단일 분배에서 버전 정책 뒤에 시각을 읽는 순서와 구분한다. callback의 sender/등록 변경은 다음 내부 패킷에 반영하고 예외에도 depth를 복구한다. 지연 분배/중첩 집합, TCP/SPX/OS 접속 실패/재시도/종료 전체와 손상 버퍼의 원본 범위 밖 접근은 이번 지원 범위가 아니다.

## 독립 검증

[관찰 도구](../../tools/decomp_local_mission_transport.py)는 현재 전체 정밀 함수표에서 검토한 불연속 범위만 실행한다. 알 수 없는 첫 진입을 통과시키지 않는다. 원본 격리 관찰 **66고유입력**은 Zone2/로그인24/설치8/집합32다. 중간32/34개 생성 결과는 이66개에 포함되며 합산하지 않는다.

- Zone:512바이트 전체 stack과 observer 이후86바이트 복제, 원본 외부 Fort 준비/할당 순서 및 선택 State/sequence 출력.
- 로그인:이름0/8/31/49바이트·이미지1/7/256바이트·서로 다른 이미지 입력에서 예약 전체와 최종 송신 전체.
- 설치:Player9 전체1,620바이트·현재 owner/template/known/server/boss/Zone/double 기한 및 설정/시간/이름/서버 시작 순서. callback 중 현재 subscriber/count 변경2개 포함.
- 집합:실제004df740/004df240/004df010/수신 observer, 버전0/1/2와 정책0/1/2/4/8, callback 중 sender 변경 및 닫힌 연결에서 반환값·flags/sender/received/heartbeat/depth·모든 시계/기록/처리기/종료 효과.

모든 정상 호출의 cdecl 스택과 EBX/ESI/EDI/EBP 보존을 확인했다. CRT 진단 출력/IP 문자열 포맷, 시간값, 파일/Template 이미지, 힙, 등록 receive, 연결 종료/반납, 이름 등록과 서버 시작은 명시 외부 대체다. 원본 게임이나 원본0x27 전체 handler를 실행한 관찰이 아니다. [fixture](../../cpppj/tests/fixtures/local-mission-transport-x86.tsv)와 [보고서](../../cpppj/local-mission-transport.json)에 근거/SHA/제한을 보존하고 `--verify`를 통과했다.

새 C++ 집중 검사는 `LocalMissionTransport_Native66`, `LocalMissionTransport_ZonePendingLoginAndReply`, `LocalMissionTransport_SelectedBoundariesAndDepth`다. 실제 Zone→같은 Player→실제 CheckLogin→pending 재배치→실제0x27/응답 바이트 분배를 검사하며 원본 큰 worldLoad를 가짜 callback으로 완료 처리하지 않는다. CMake 등록 뒤 **Release 경고0·오류0, 전송3개를 포함한 집중33개 실패0·전체 CTest1/1·내부1,264개 실패0·210.62초**를 확인했다. 로그는 `extracted/goal-chat-transport-final-{build,focused}.log`, `goal-chat-transport-{ctest,tests}.log`다. 새 native66개와 현재 소스 SHA 감사도 통과했다. 게임/GUI 실행은0이다.

## 다음 Session 연결

같은 Session의 `RawPlayerParticipation`에 로컬 전송의 `AssignConnection`을 공급하고, 같은 패킷 분배기에 실제0x27 로그인 몸체를 등록한다. 현재 Config의 simulconnect를 실제 getter로 읽고 Zone 설치/현재 subscriber 선택을 선행한다. 그 뒤 `MakeLocalMissionLogin`이 받은 실제 이미지의 송신/flush/poll이 원본0x27 분기를 지나 `RawCampaignWorldRuntime::Load`를 호출하게 옮긴다.

0x27의 기존 사람 참여 bit2 저장→CheckLogin→원본 Player 데이터/응답/전송→새 로그인에서만 worldLoad→**그 시점의 현재 localOwner** Ready40→connected/dropped 수에 따른 Begin, 이후 login timer/사제 상태/진행GUI 종료와 Guide Stage3는 계속 상위 구현 대상이다. 자연 미션 완료/자동 AI 전략/자동 Outcome을 이 전송 단위만으로 활성화하지 않는다. Early Missions·캠페인1-1~1-2 자연 완주와 원본 동일성은 미완료다.
