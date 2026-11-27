# Native GameAux 헤더10칸의 웹 경계 — WB04-E1451

2026-11-27 HJOW-Athlon. E1440 뒤 이름이 다르거나 별도 owner에 있는24개 중 **헤더10개를 분류했다.** 이 단위는 소스 판정이며 제품 기능을 새로 구현한 작업이 아니다. 나머지 역할10·Regular 이벤트2·진단1·기록 재생1의14칸은 후속 판정으로 남긴다. Native129개 이름과 웹 GameAux의105개 이름이 겹친다는 집계는 전역 주소/수명 동일성의 증거가 아니다.

| Native 칸 | 현재 웹 소비자와 판정 |
| --- | --- |
| `battleStartPeerHeader` (`1b/1/38`) | `NativeJoinTestBattle.cpp:15`의 개발자 이름4개 관문 뒤 직접 수신하는40BYTE 지역 버퍼용이다. 실제 일반 미션 시작 헤더와 합치지 않는다. 웹 `client-campaign-transport.js:13`은 Meta의1a/1b 전투 시작을 제외하므로 웹 제외다. |
| `contentIncomeHeader` (`5a/1/5`) | `raw-campaign-session-contents.js:121`의 원격 수입은 활성 Player와 실제 peer가 있어야 한다. 실제 peer가 없으면 반환하고 로컬 수입 알림은 별도 경로다. 실원격 peer 송신은 멀티플레이 제외 범위라 Native 생성 헤더 owner도 제외한다. 5a 공통 수신 메타데이터/로컬 알림을 삭제하거나 제외한 것은 아니다. |
| `gunProcessHeader` (`3a/1`) | `raw-gun-process.js:551`의 typed writer 인자로 표현한다. 값의 현재 Native 참조를 웹 GameAux에 연결하지는 않았다. |
| `flyerProcessHeader` (`39/1`) | `raw-flyer-process.js:367`의 typed writer 인자로 표현한다. |
| `sunAviaryHeader` (`38/1`) | `raw-sun-aviary-process.js:326`의 typed writer 인자로 표현한다. |
| `windAviaryHeader` (`74/1`) | `raw-wind-aviary-process.js:305`의 typed writer 인자로 표현한다. |
| `rainAviaryHeader` (`73/1`) | `raw-rain-aviary-process.js:287`의 typed writer 인자로 표현한다. |
| `bombTimerHeader` (`7d/1`) | `raw-bomb-effect-timer.js:192`와 `raw-bomb-timer-packet.js:51`의 typed writer 인자로 표현한다. |
| `bombTimerInitialLength` (DWORD0) | Native writer의 초기0/하한4와 달리 typed Session은22로 작성한다(`raw-campaign-session-transmission.js:121`). 실제 peer가 있으면 `finalizeBombTimerPacket`이 대상 수에 맞게22+2n으로 확정한다. peer가 없을 때22를 그대로 전달하는 기존 typed 콜백도 현재 검사가 명시한다. **중간 헤더가 같다는 판정은 아니다.** |
| `pathPacketHeader` (`30/1/length0`) | `createPathDataHeader()`(`raw-path-data.js:44`)를 실제 motion owner가 한 번 소유하고(`raw-campaign-session-motion-runtime.js:86`), 같은 Path 송신에 전달한다(582줄). 공용 Native 주소와 다른 typed owner 수명이다. |

위 표의 두 BYTE 배열은 Native 생성자 메타데이터의 message/protocol 순서이며 wire의 첫 두 BYTE 순서와 혼동하지 않는다. 실제 웹 Storage에는 `usesNativeTransmissionDevices` 설치가 없으므로 Session의 `#native()`는 false다(`raw-campaign-session-transmission.js:66`). 선택적인 Native 분기를 검사에서 주입할 수 있다는 사실을 실제 앱이 Native 장치를 쓰는 것으로 세지 않았다. typed에서는 각 실제 writer/지역 예약이 있고, Native에서는 serializer/common buffer를 요구하는 두 경계를 이미 분리한 구조다. 따라서8개는 현재 웹의 typed 표현 차이로 판정한다. Native GameAux 헤더를 시험에서 임의 변경했을 때 모든 웹 writer가 같은 현재 주소를 읽는 계약은 제공하지 않는다.

집중126/126 통과·실패/취소/건너뜀/todo0이다. 공중3종의 writer/reader·원본 Path fixture384행과 실제 Session 타이머 선언/송신 대상 선택을 포함하지만, 새 공중 유닛 전용 실제 플레이나 모든 헤더의 실제 네트워크 동등성을 입증하지 않는다. 제품 JS·번들·fixture·CPP/원본은 변경하지 않았다. CPP `RawGameAuxState.h`는 마지막 `8defb347c` 뒤 변경0이다.

[소스 해시·이름 집계·잔여14·검사 식별자](webpj-e1451-evidence/provenance.json). 원문은 무시된 `webpj/extracted/e1451-header-focus.log`에 보존한다. 완료 뒤 주기는v26=4/4·v22=4/8이므로 Node26 전체를 다음에 수행한다.
