# webpj Factory 연구(RawFactoryResearch) 이식 — WB04-E217

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "자원·생산·건설 명령" 묶음의 일부다. Factory 의 지식(연구) 선택·서버 연구 수신·생산 내용물 통지(453cc0·4547d0·453d70·453b50→475220)와 패킷 0x58 의 16 BYTE 기록을 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawFactoryResearch.cpp/.h`)·시험(`FactoryResearchTests`)·fixture(`factory-research-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 루트에서 확인).
- 근거 커밋: 소스 `7c51717d`, fixture `7cbcd998`.
- 원본 fixture: `factory-research-x86.tsv` 216행(10.78 전용 관측): Learn 32·Input 72·Receive 16·Notice 96. 입력은 검색 결과·남은 슬롯·attach flags·kind·수정키·sender·부모 상태/TYPE flags2·권한·내용물 flags/reason/loading, 출력은 반환·productionDirty 카운터·전체 호출 순서다(송신 16 BYTE hex, 도움말/진단 메시지 원문 hex 포함).
- 공유 장치: D5a 등 기존 `SidPool`(물리 슬롯 읽기)만 사용한다. 새 공유 장치는 만들지 않았다.

## 1. 옮긴 것

`www/core/raw-factory-research.js`(DOM 없음, 10.78 전용)

- `writeFactoryResearchCommand(parent, kind, now, packet)`: 패킷 0x58 의 16 BYTE(프로토콜 1·선언 길이 16·부모 SID WORD·kind 최종 BYTE·mode 0·현재 시각 double 비트)를 기록한다. 16 BYTE 보다 짧은 저장소는 거부하고 그 뒤의 호스트 버퍼는 보존한다.
- `RawFactoryResearch(pool, types, state, hooks)`: 10.78 풀과 188칸 TYPE 표를 확인한다. `state` 는 `{ loadingDepth(), productionDirty: { value }, boss(), now(), outpostType() }`(효과 뒤에도 최신 값을 다시 읽음), `hooks` 는 `find`(4ac9f0)·`slots`(4752b0)·`destroy`·`create`(4afa70)·`send`(채널 9)·`help`·`flare`(45a6c0)·`refresh`(44f7c0)·`assertion`·`upgradeReceive`·`inputLog` 이다. 선택한 하위만 그 위치에서 요구한다.
  - `handlesNotice(parent)`: 실제 가상 표 0x00509148(Factory)·0x00509210(아웃포스트 wrapper)만 통지 대상으로 본다.
  - `learn(parent, kind, attachFlags)`(453cc0): `find(…,16,kind,0)` → 결과가 client SID 범위(5..14999)면 destroy(0) → `slots` 재조회 → 결과가 있었는데 남은 슬롯이 0 이하이면 진단 `slotsRemaining > 0` → 남은 슬롯이 1 미만이면 반환 → 같은 종류가 다시 검색되지 않으면 `create(parent, kind, 0, 16, attach, 0)`.
  - `input(parent, code, packedArgument)`(4547d0 03000080): 수정키를 뺀 값이 03000080 이 아니면 미복원 오류. 상위 signed WORD 가 종류이며 0 이면 도움말 `researchHelp`(1)을 부르고 1 을 돌려준다. 아니면 슬롯 조회 → 입력 로그 → 슬롯이 양수이면 **로그 뒤의 현재 시각**으로 패킷을 만들고 learn(attach 16) 뒤 채널 9 로 송신한다(로그가 슬롯을 바꿔도 처음 양수 판단 유지).
  - `receive(sender, packet)`(453d70): 8 BYTE 미만은 거부. 부모 state 의 free/dead(비트 3)이면 무시, kind 0 이면 mode 1 일 때만 업그레이드 수신(`upgradeReceive`, 미공급이면 오류), 현재 TYPE 의 flags2 0x4000·owner==sender·비 아웃포스트일 때만 권한 확인(없으면 진단 `boss` 뒤 다시 읽어 여전히 없으면 반환) 후 learn(attach 0).
  - `notice(parent, form, reason)`(453b50→475220): reason 1 이고 form 이 있고 내용물 flags 비트 16 이면 플레어(parent, 6, 2) → 같은 kind 재검색 → 다른 client 예측 내용물이면 삭제. 이어서 `loadingDepth()` 가 0 이고 flags 비트 16 이면 productionDirty 를 올리고, reason 1 이면 부모의 abstract/astral 이면 진단 뒤 소유자의 Deck 재계산(`refresh`)을 요청한다. form 0 도 물리 0번 슬롯을 읽는다.

## 2. 검증

`tests/factory-research.test.mjs`(3건)

- 원본 관측 재생: 216행 전부(Learn 32·Input 72·Receive 16·Notice 96). 실제 `SidPool`(24000, 서버)에 Factory(SID 50, 가상 표 0x509148, TYPE 85)와 내용물(SID 100)을 raw 로 준비하고 반환·dirty·호출 순서를 대조한다.
- cpppj 계약 2건: 입력 로그가 슬롯과 시각을 바꿔도 처음 양수 판단을 유지하고 패킷은 로그 뒤 현재 시각(33.75)을 쓴다, 패킷 기록 16 BYTE·짧은 저장소/수신 본문 거부·미공급 업그레이드·가상 표 판정·플레어 누락 시 dirty 불변.

최초 실패와 교정: 모듈은 처음부터 216행과 계약 2건이 모두 통과했다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(cmp 일치).

- 실패한 변형: signed WORD 종류를 unsigned 로, client 상한 15000→15001, 슬롯 양수 판단 `>0`→`>=0`, 남은 슬롯 `<1`→`<0`, dirty 증가에서 loadingDepth 관문 제거, 수신 sender 비교 반전(각 1~2건).

## 3. 경계와 남은 것

- 패킷 0x58 분배기 등록 `RegisterFactoryResearchCommand`(관찰자·버전 1·길이 16·sender 전달)와 cpppj `FactoryResearch_PacketDispatchAndMissingEffects` 의 분배 부분은 `RawPacketDispatch`(순서4, 미이식) 이식 뒤로 미룬다. 이 단위는 그 시험의 비분배 부분(패킷 기록·VT·누락 하위)만 옮겼다.
- 실제 월드에 연결하지 않았다. 호출 지점(Factory 메뉴 입력·내용물 통지 분배)과 실제 검색/삭제/생성/송신/Deck 재계산 하위는 순서4~6 몫이다(PORT_MAP session 0).

## 4. 다음 시작점

WB04-E218 은 3.1절에서 고른다. 같은 묶음의 `RawFactoryUpgrade`(예측 그림/완료/종속 그림 정리)·`RawFactoryUpgradeCommand`(요청 00453bf0·수신 00453d70 업그레이드 분기; 모두 기준선 뒤 변경 없음)를 권한다. `RawFactoryResearch::Receive` 의 업그레이드 수신(`upgradeReceive`)은 `RawFactoryUpgradeCommand.receive` 가 채운다.
