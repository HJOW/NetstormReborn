# 생산 Deck 재계산 연결 이식 (WB04-E522)

4-D 구획 C의 `client/RawCampaignProductionBindings`를
`webpj/www/core/raw-campaign-production-bindings.js`로 옮겼다. 기준선 `225553df`와 현재 cpppj 소스가
같음을 확인했다. E215가 이월한 "재계산/Factory 상품 추가 시험 3건"과 원본 10.78 관측 242행을 이 단위에서
처음 대조했다.

## 옮긴 범위

- `refresh(owner)`/`refreshCurrent()`(원본 0044f7c0): Deck 토큰 → Fort/Battle 모드 진단(둘 다 꺼져 있으면
  `modeAssertion`, 없으면 거부) → 미션이 아니고 `fullDeck`이 비영이면 자연 반환 → 기존 항목의 power를
  255로 표시하고 가중합 0 → `getTeleportTechFree` 설정이 비영이면 Mog 153 활성 → 공통 Factory 목록을
  매 반복 현재 개수/DWORD로 순회(현재 owner의 신전 genus는 허용표가 허락할 때만 기본 생산 133, 일반
  Factory flags1은 상품 추가) → Compact → signed power가 양수인 항목의 chance BYTE를 DWORD 감김 합산.
- `appendFactoryProducts(factory, deckToken)`(004752b0 mode 0): 용량(`factoryCapacity`: 두 Factory 가상 표
  `0x509148`/`0x509210`만, BYTE 상위 3비트 + 2) → 같은 풀의 내용물 순회에서 표시 비트(`0x10`)가 있는
  항목의 kind를 허용표 조회 후(금지여도 Deck 추가를 거르지 않음) Deck에 활성화하고 distinct 종류별로
  남은 자리를 줄인다. `deckToken` 0이면 Deck을 바꾸지 않는다.
- `contains`(0044f970)·`indicateTechnology`(004929e0)·`allowed`·`pool`·`deck`.
- 생산 상태(`mission`/`fullDeck`/`freeTechnologyType`/`templeProduct`)는 사용 때마다 현재 값을 읽는
  객체이며 속성 getter로 장기 소유자의 DWORD를 빌릴 수 있다(cpppj의 `const&` 참조 대응).

## 시험

`webpj/tests/raw-campaign-production-bindings.test.mjs` 4건:

1. 원본 관측 `production-refresh-x86.tsv` **242행 전부**: 매 행 새 보호 188 TYPE·실제 Player/Factory/
   내용물/SP 통지 월드를 만들어 반환값·352바이트 Deck 전체·요청 순서(`Q:getTeleportTechFree`, `A:331`)가
   같고 Player 9기록·풀 전체·금지 표가 변하지 않음을 확인했다. **첫 실행에서 242행 전부 일치**했다.
2. 허용표·Factory 목록(개수 0)·설정·templateOwner의 현재 값을 호출마다 다시 읽는다.
3. 모드 진단이 쓴 장기 `fullDeck` DWORD를 다시 읽어 자연 반환을 고르고, 0이면 설정을 질의해 항목이 사라진다.
4. 웹 전용 거부: 다른 풀의 Core/Player/Deck, 미복원 Factory 가상 표, 범위 밖 Factory SID(0·65535),
   진단/설정 소비자 미연결, `deckToken` 0의 Deck 불변.

`webpj/tests/fixtures/manifest.json`에 `production-refresh` 항목(원본 10.78, 242행)을 추가했다
(`fixture-manifest.mjs --write --only production-refresh`).

## 남은 경계

- 실제 `ClientGameplayGlobals`(장기 fullDeck DWORD 소유자)는 아직 없어 시험은 속성 getter로 같은 형태를
  재현했다. 앱 수명에는 연결하지 않았다(PORT_MAP `port`).
- 표시 Gump 갱신은 별도 실제 명령 소비자의 책임이다(`RawProductionControls`, 5-B/6).
- 다음 시작점: `RawCampaignAssetBindings`·`RawCampaignConstructionBindings`.
