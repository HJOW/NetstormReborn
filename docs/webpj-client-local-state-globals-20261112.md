# webpj Client 수명 State/로그인 전역 — ClientLocalStateGlobals (WB04-E335, 2026-11-12, 4-D A 구획)

3.1절 4-D A 구획의 client **ClientLocalStateGlobals**(기준선 `.h` 132줄, 기준선 뒤 +48줄 모두 반영)를 `core/client-local-state-globals.js` 로 옮겼다. E325 의 `ClientConnectionGlobals` 가 빌리는 owner(`clockSynchronization().connection`)이며, 앞 단위(E325·E331~E333)와 같은 "참조를 빌려 주는 소유자, 값 사본 없음" 방식이다. 이 소유자가 보유하는 기준선 뒤 새 파일 타입 몇 개(작은 보유 구조)를 함께 옮겼다.

## 1. 규칙

- **ClientLocalStateGlobals:** 새 Client 에서만 최초 BSS 0 또는 PE 초기값을 만들고 Session 대여/반납·State 전환으로 되돌리지 않는다. 모든 접근자는 호출마다 **같은 객체**(DWORD/Double/Int32/Bool 상자, 배열, 뷰)를 돌려준다.
  - 상자: DWORD 40개(`DwordBox`, PE 초기값: `joinTestBattleCommand` 0x0200002b·`viewMetaCommand` 0x02000013·`firstMenuPending` 1·`tellTips` 1·`tipNumber` 1·`scriptBreak` 1·`restoreOwner`/`restoreSubscriber`/`cachedPeer*` 0xffffffff·`hudModeCommand` 0x0200003d·`addIslandCommand` 0x02000022·`metaFirstReset` 1, 나머지 0), 벽시각 기한 double 7개(`activityEnd`·`captureEnd`·`connectEnd`·`loginEnd`·`reconnectSaveEnd`·`localServerSearchEnd`/`RetryEnd`, 최초 0), `invalidTerritory`(Int32 127), 불리언 `savedGame`·`simulconnect`(`BoolBox`).
  - 블록/버퍼: State 다섯 블록 `blocks()`(36DWORD `Uint32Array`×5, 서로 겹치지 않음)와 `state()`(`{ old, phase, immediate, current, next }`, `RawLocalMissionState` 가 그대로 받음)·`zoneWords()`(DWORD 2칸)와 앞 6BYTE 뷰 `zone()`·`metaZoneRecords()`(7344BYTE)·`playerNoMakeWarnings()`(255 DWORD)·`previousHandle()`/`testBattleAddress()`(각 64BYTE).
  - 뷰: `clockSynchronization()`({ connection(005c8bf4)·replyMode(005c8c00)·gameTicks(0055b4ac) } 접근자 — `resetClockSynchronization` 입력과 같은 형태), `ambientSound()`({ deadline(=activityEnd), kind, forced } — `stepAmbientSound`/`resetAmbientSoundDeadline`), `metaZones()`({ firstReset, records } — `clearMetaZoneRecords`), `battleByte()`(005c85d0 의 하위 BYTE — 쓰기는 나머지 3BYTE 를 보존하며 `battleTerritory()` DWORD 와 같은 저장소).
  - 보유 타입: `helpWord()`(ClientHelpWord)·`monitorTime()`(MonitorTimeState)·`clockSamples()`(ClockSynchronizationData)·`zonePackets()`(RawPacketCopies).
- **ClientHelpWord** (기준선 뒤 새 파일, M7.1): 최초 null 인 helpWord 캐시. `interpret(configured)` 는 null/undefined/빈 문자열이면 false 이고 캐시를 건드리지 않는다. 비어 있지 않으면 기존 캐시와 ASCII 대소문자 무시로 같을 때 옛 표기를 유지(교체 횟수 `generation` 불변)하고 다를 때만 교체한다. 한글 등 ASCII 밖 문자는 접지 않는다(원문 `_stricmp`). 원문 표시 바이트 부호화(`EncodeDisplayText`)는 웹이 JS 문자열을 그대로 쓰므로 적용하지 않는다.
- **MonitorTimeState** (`RawMonitorTime.h`): `initialized`(DWORD)·`deadline`(double)·`monitor`/`alert`(signed int32). 시간 알림 본체(`StepMonitorTime` 등)는 채팅 관리자 단위와 함께 옮긴다.
- **ClockSynchronizationData** (기준선 뒤 새 파일 `RawClockSynchronization.h` 의 데이터만): `latencyMilliseconds`/`sent`/`received`(DWORD)·`next`(double)·70×32BYTE `samples`. 본체(서버 응답 등록·Commander 연결·수신·Poll)는 시간 동기화 단위에서 옮긴다.
- **duplicateRawPacket / RawPacketCopies** (기준선 뒤 새 파일 `RawPacketCopy`, 4dece0): packet 이 없으면 아무것도 하지 않고 빈 결과, 아니면 BYTE 1 의 항목을 pin 하고 observer 를 방향 1 로 알린 **뒤** WORD 2 의 현재 길이를 읽는다. 목적지가 없고 capacity 0 이면 새 할당(present), 그 밖은 signed `capacity < length` 이면 빈 결과, 목적지가 있어야 present 이다. 낮은 주소부터 DWORD 단위 복사 뒤 남은 BYTE 를 복사하고(겹침은 원문 순서) 물리가 모자라면 RangeError 이다. `RawPacketCopies` 는 token 을 1 부터 재사용 없이 발급(0 은 NULL, DWORD 끝에서 소진 진단), `duplicate`(소유 할당만 장부에)·`adopt`(인계)·`acquire`(없으면 오류)·`release`(0 무시, 한 번만)이다.

## 2. 웹 범위와 기준선 뒤 변경

- 기준선 뒤 +48줄(12커밋, c551d159 까지)은 모두 반영했다: M7.1 helpWord, DebugBurst, JoinTestBattle 명령, 메타/로컬 서버 검색/Peer 캐시 칸, 시간 동기화 70표본, Zone 소유 token 장부, 메타 구역 표, 경고 표 255 등. 새 필드는 값 보관만 하고, 메타(LAN)·로컬 서버 검색·Join 시험 전투를 소비하는 분기는 웹 제외 오류로 막힌 소비자가 쓴다(① 보관 / ③ 소비자 단위 몫). 종료된 서버의 security.dat/명령줄 검증 writer 는 `userUpdated` 에 연결하지 않는다(사용자 지시).
- `RawPacketCopies.duplicate`/`adopt` 의 실제 소비자(State+80 가변 Zone 복제)와 `RawClockSynchronization`·`RawMonitorTime` 본체는 3.6절 ③ 표(4-D~ 행)로 넘겼다. PORT_MAP: `ClientLocalStateGlobals.h` port(보유 타입 모듈 세 개를 `;` 로 묶음)·`RawMonitorTime.cpp` partial.
- C++ shared_ptr 수명(Zone 소유 장부)은 JS GC 로 대체한다(① 판정). 이 모듈은 DOM 을 쓰지 않는다. 앱 수명 연결(Client 가 한 번 만듦)은 4-D 조립 몫이다(PORT_MAP session 0).

## 3. 검사 (`tests/client-local-state-globals.test.mjs` 28건)

- **cpppj 시험 이식:** `ClientHelpWordTests` 7건(키 부재/빈 값, 이미 비NULL 유지, 대소문자 유지·교체, 비NULL 빈 문자열 구별, 한글 정확 비교, 상태 전역의 단일 캐시). 한글 표시 바이트 시험은 JS 문자열 비교로 옮겼다. 나머지 `ClientLocalStateGlobalsTests`(12건 전부)는 `RawCampaignSession`/Login 대여를 쓰므로 Session 조립 단위(4-D D)에서 옮긴다.
- **웹 계약:** PE 초기값 전체·상자 폭(DWORD 부호 없음·Int32 signed·불리언)·접근자 주소 고정, 모든 상자가 서로 다른 칸임(이름마다 고유 값을 쓰고 전부 다시 읽음), State 블록 정체성과 비중첩 및 `RawLocalMissionState` 수용, 시간 동기화 뷰와 `resetClockSynchronization`/`ClientConnectionGlobals` 수용, 주변음 뷰와 `resetAmbientSoundDeadline`, 메타 구역 뷰와 `clearMetaZoneRecords`, Zone 앞 6BYTE 뷰, battle BYTE/DWORD 공유(상위 BYTE 보존), 두 Client 독립, `MonitorTimeState`/`ClockSynchronizationData` 필드 폭, `duplicateRawPacket`(null 패킷, observer 방향 1 뒤 현재 길이, 256BYTE 이상 WORD, 외부 목적지·capacity 0/부족/signed, 목적지 없는 길이 0, 반환 뷰 길이, 남은 BYTE, 물리 부족·타입 범위 밖, 겹침 순서, 등록 대여 핀 두 겹과 반납·예외 시 반납), `RawPacketCopies`(token 순서·NULL 0·획득/반납·인계·소진).
- 첫 실행에서 25건이 모두 통과했고(모듈은 시험 뒤 보강 없이 그대로), 변형 확인 뒤 3건(판별력 보강)을 더했다.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 180개(LocalState 115·PacketCopy 36·HelpWord 16·MonitorTime 6·ClockData 7)를 돌려 첫 실행에서 51개가 남았다(대부분 같은 초기값을 가진 두 상자의 접근자 교환·State 블록 이름 순서(내용이 모두 0 이라 deepEqual 로는 구별 못함)·시간 동기화 getter·battle BYTE 마스크·256BYTE 길이·핀 반납 등). 시험을 보강한 뒤 **3개가 남았고 모두 동등**하다.
  1. `RawPacketCopies.duplicate` 가 장부에 `copy.allocation` 대신 `copy.bytes`(할당 전체를 보는 같은 길이의 뷰)를 넣는 변형: 내용과 길이가 같아 관측 차이가 없다.
  2. `acquire` 의 `!token ||`: token 0 은 장부에 없으므로 `found === undefined` 와 같다.
  3. `ClientHelpWord.interpret` 의 `this.#value !== null &&`: 캐시가 null 이면 `equalsIgnoreCase('', 비어 있지 않은 설정값)` 이 길이 차이로 false 라 같다.
- 3.1절 4-D 는 6/27 이다(A 구획: ClientFortGlobals·ClientPlayerRecords/Runtime·ClientCoreDataGlobals·ClientCameraGlobals·ClientAiDebugGlobals·**ClientLocalStateGlobals** 여섯 줄 완료). 다음: A 구획 `ClientSpGlobals`/`ClientNsInfoGlobals`, `ClientStateWorld*` 묶음(선행 확인 필요 항목은 3.1절 줄에 사유를 적는다).
