# webpj 3-E 판정-기타 — WB04-E264 (2026-11-10), 3-E 마감

3.1절 3-E의 마지막 항목 **판정-기타** 6단위를 판정했다. 이 항목으로 3-E(기준선 뒤 cpppj 변경 판정, 순서3 마감 관문)의 여섯 항목이 모두 끝났다. 방법과 ①/②/③ 기준은 [판정-공통층 문서](webpj-judge-common-20261110.md)와 같다: 이식 원본은 기준선 `1f00a25a` 판이고, `git diff --ignore-cr-at-eol 1f00a25a HEAD -- <단위 파일>`(6단위 약 150줄 전부)과 새 API 의 소비자(`grep`)를 읽어 단독 미션의 웹 결과가 바뀌는지만 판정했다. 판정 기준 커밋은 착수 때 `HEAD` `ba0a999b`(20261110-119 WB)이며 행 base 는 단위 파일을 마지막으로 바꾼 커밋이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 판정 표

| 단위 | 판정 base(기준선 뒤 커밋) | 기준선 뒤 변경 | 판정 | 처리·시점 |
| --- | --- | --- | --- | --- |
| RawFlare | `6a136e89` (1) | `state.native`(TYPE/할당 사전검사 생략)·억제 시각 getter(005c85f0)·난수 owner getter(00532710) 생성자 | ③ | **4-D**. 독립 typed 참조 계약 불변 |
| RawIceTower | `56f3c80f` (1) | 완성형/성장형/noIsland TYPE 의 shared 주소(`Ice()`/`Growing()`/`NoIsland()`)를 원문 선택 위치마다 읽음 | ③ | **4-D**. 독립 typed 입력은 기존 값 |
| RawPathRuntime | `5318fdd6` (1) | `friend class RawPathData` 선언 한 줄 | ① | C++ 접근 제어(실제 네트워크 decoder 가 같은 raw 배열을 직접 복원). **6-P** `RawPathData` 이식 때 웹 대응을 본다 |
| RawWindAviaryProcess | `0b1b7c73` (4) | `friendlyFire` 비영 뷰·`FlyerDiedIn`(Native 통지의 같은 AP2/dirty4 본문)·`WritePacket` 의 force/protocol/message 선택 | ① + ③ | 뷰는 ①. `FlyerDiedIn` 은 **4-D**, 동기화 force 는 **4-C** 패킷 writer. 독립 typed 기본값(force=false, 0x74) 불변 |
| RawWindAviaryState | `8b15b357` (1) | `ReadProcess`(0045b210 의 실제 수신: 선택된 객체 저장이 없는 NULL this 도 표현, 필드 물리 범위 밖은 명시 오류) | ③ | **4-C**. 내부 미션 전송으로 서버→클라이언트 수신이 지나가므로 패킷 수신 이식 때 한 묶음. typed 읽기는 같은 본체를 공유해 결과 동일 |
| client/RawChatChannels | `ce5d8878` (2) | `SanitizePlayerName`(0042b0e0 이름 정제를 자유 함수로 분리)·`Forget`(00445110: 같은 ID 의 100기록 모두 0) | ① | 소비자가 `NativeMetaParticipantCreate`·메타 참가자 전체 제거라 메타(웹 제외). `NameChanged` 는 같은 정제를 40BYTE 로 호출해 결과 동일(NUL 없는 40BYTE 는 이제 명시 오류이나 `Add` 가 +27 에 NUL 을 쓰므로 도달하지 않음) |

합계: ② 0·① 2(RawPathRuntime·RawChatChannels)·①+③ 1(RawWindAviaryProcess)·③ 3 — 6단위가 모두 판정됐다. 이 묶음도 웹 코드와 시험을 바꾸지 않는다(문서·대응표만).

## 2. 3-E 전체 결과 (E259~E264)

| 항목 | 단위 | 문서 | ② 재반영 | ③ 이월 시점 |
| --- | --- | --- | --- | --- |
| 판정-공통층 | 22 | [E259](webpj-judge-common-20261110.md) | 2(ContentFormHost 현재 가상+50 부착·`removeStateSidList`) | 4-A·4-B·4-D·5-B |
| 판정-Player·사제·신전 | 12 | [E260](webpj-judge-player-priest-20261110.md) | 3(RawPriestPreDestroy·RawPriestRegen·RawVortexPreDestroy) | 4-D·6 |
| 판정-섬·다리·자원 | 6 | [E261](webpj-judge-island-resource-20261110.md) | 0 | 4-B·4-D |
| 판정-건설·운반·Dude | 15 | [E262](webpj-judge-construction-dude-20261110.md) | 0 | 4-C·4-D·6 |
| 판정-전투 | 18 | [E263](webpj-judge-combat-20261110.md) | 1(상태 타이머 패킷 선언 길이 DWORD) | 4-A·4-C·4-D·7-P2 |
| 판정-기타 | 6 | 이 문서 | 0 | 4-C·4-D·6-P |
| 합계 | **79** | | **6건** | |

- ② 재반영 6건(웹 코드 변경 + 시험): `ContentFormHost` 현재 가상+50 부착, `removeStateSidList`(RawStateSidLists), 사제 PreDestroy 목록 제거 공유, 사제 회복의 TYPE 158 검사 제거, 신전 PreDestroy 의 signed outpost 반복, 상태 타이머 패킷 writer 의 선언 길이 DWORD. ② 가 있는 단위는 7개이고 나머지 72단위는 ①(결과 영향 없음)이거나 ③(독립 typed 결과는 같고 실제 Client/World 의 Native 조회기·getter·본체·wire 경로가 4-A~4-D·5-B·6·7-P2 에서 연결될 때 반영).
- ③ 은 `LEFT_JOBS.webpj.md` 3.6절 표에 시점별로 적었다: **4-D**(Native World 대여·가상 분배·공유 getter 묶음), **4-C**(건설/폭탄/아비어리 패킷 writer·observer·수신), **4-B**(Fort 적재 전역·섬 후처리 로딩 깊이·NoWorld 섬 조회), **4-A**(Config 문자열 버퍼·타입 레코드 새 칸 6개), **5-B**(표시 카메라 getter), **6**(입력 소비자의 단독 도달 재확인), **7-P2**(Rain 오버라이드).
- 남은 판정 대기: client UI 4단위(`ClientMain`·`UberGump`·`MenuGumpWidgets`·`ScrollingTextGumpWidgets`)는 4-D·5 연결 때 판정한다.

## 3. 대응표·문서 반영

- `PORT_MAP.tsv` 6행의 base 와 메모를 갱신했고 `node webpj/tools/port-map.mjs` 오류 0, 6단위의 `cpppj 변경` 보고가 사라진다. 3-E 전체(79단위) 중 `o/` 단위의 `②/③ 판정 필요` 보고가 0이 되었는지는 아래 4절에서 다시 센다.
- LEFT_JOBS.webpj.md 3.6절 표에 ③ 묶음 1행(4-D·4-C 플레어/아이스 타워/아비어리)을 더했다. [대응표 문서](webpj-port-map-20261106.md) 9절 표에 이 묶음을 더했다.
- **순서3 마감 정리**: 3.1절 3-B~3-E 완료 목록을 [보관본](../LEFT_JOBS.webpj.old.md#archive-webpj-20261110-through-wb04-e264)으로 옮기고, 0.2절의 4-A 를 **다음**으로 바꿨다.

## 4. 남은 경계와 다음 시작점

- 다음은 순서4 **4-A 자료·파일·설정·타입 표**(3.2절)이다. 착수 때 3.1절에 4-A 체크 항목 목록을 펼친다(`o/` 15단위: adapt 8·Config*·ConfigInterface*·OriginalText*·Xlat*·RiftType*·RiftTypeTable*·TypeParser*, client M 영역). 3-E 에서 정한 4-A 몫 ③(ConfigInterface 문자열 버퍼·RiftType 새 칸 6개·`defaultFrame`)을 같은 단계에서 반영한다.
