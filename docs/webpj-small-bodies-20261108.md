# webpj 소형 몸체 네 개(플라이어 전처리·펜스 타입 부모·사제 희생 요청·캐리어 채취 통지) 이식 — WB04-E14

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 열네 번째 부분이다. cpppj 기준선 `1f00a25a`의 작은 `o/` 단위 네 개(모두 10.78, 독립 원본 관측 보유)를 한 번에 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

| 모듈 | 원문 | 내용 |
| --- | --- | --- |
| `www/core/raw-flyer-pre-destroy.js` | 0045e7e0 | `RawFlyerPreDestroy(pool, {destroy, damageable}).preDestroy(sid, flags)`: 슬롯 `+8` WORD(DWORD나 종속 head로 넓혀 읽지 않음)가 비0이면 그 대상을 가상 삭제(`destroy(target, 0)`)한 뒤, callback 안에서 raw가 바뀌었더라도 원본 this/flags로 Damageable 전처리(`0044b4b0`)를 한 번 호출한다. |
| `www/core/raw-fence-type-parent.js` | 457890→457460(Rebuild), 457e20→4578f0(Refresh) | `runFenceTypeParent(raw, sid, currentType(), checking(), operation, hooks)`: checking이 비0이고 슬롯 타입 BYTE가 현재 타입 전역 DWORD와 다르면 원문 진단(`fence.getType() == sunFenceType`, `\Ns\O\Fence.cpp`, Rebuild 0x60f/Refresh 0x602)을 요청한다. 복귀 뒤 checking은 다시 보지 않고 슬롯 BYTE(zero-extend)와 전역 DWORD를 다시 읽어 같으면 하위 `body(sid)`를 실행한다. |
| `www/core/raw-priest-sacrifice-request.js` | 4498e0, 494eb0 | `allowed(dais, priest)`는 현재 Dais 자격 판정을 그대로 전달. `begin(priest, dais)`는 너겟 내용물 제거(현재 너겟 종류)→의식 명령(다리 건설 아님)→사제의 현재 owner BYTE가 현재 로컬 owner와 같으면 `sacrifice.mus` 요청. callback 뒤 owner를 다시 읽는다. |
| `www/core/raw-carrier-mining.js` | 429800 안의 40ac20·4e2e30·4266c0 | `RawCarrierMining(pool, header, {broadcast, receive}).notify(geyser)`: 현재 헤더(protocol bit7 제거, message, length 하위/상위)와 지면 SID 6 BYTE를 한 번 작성해 같은 버퍼를 송신→로컬 수신에 전달한다. 길이를 4 이상으로 보정하지 않고 지면은 미리 검사하지 않는다. |

## 2. 검증 (`tests/small-bodies.test.mjs`, 6건)

| 검사 | 근거 |
| --- | --- |
| 플라이어 96행(WORD 대상·재진입 변경 3종·네 슬롯 200 BYTE) | `flyer-predestroy-x86.tsv` |
| 펜스 타입 432행(두 부모의 진단/복귀 뒤 raw·전역 재판정) + 선택된 경계/DWORD 타입 폭 | `fence-type-parent-x86.tsv` |
| 사제 희생 128행(호출 순서·현재 owner 재독·반환) | `priest-sacrifice-request-x86.tsv` |
| 채취 통지 864행(두 정밀도 열 동일성 포함) + 늦은 경계/판본 | `carrier-mining-x86.tsv` |

최초 실패와 교정: 펜스 모듈의 원문 파일 이름 상수 `'\\Ns\\O\\Fence.cpp'`가 셸로 쓰는 과정에서 역슬래시가 줄어든 채 저장되어 첫 실행에서 진단 인자 비교가 실패했다. 소스를 고쳐 432행이 통과했다(검사가 의도대로 잡아냈다). 나머지는 처음부터 통과했다.

## 3. 경계

- cpppj `FenceTypeParent_DiagnosticReturnUsesActualChildrenAndSameRaw`(실제 `RawFenceRebuild`/`RawFenceLifetime` 자식 연결)는 두 자식 단위를 옮길 때 대조한다.
- `RawCarrierMining`의 `CarrierArrival`(채취 조건·너겟 생성), 사제 희생의 `RawDaisRegular::CanBridge`와 Dais 의식 명령, 플라이어의 Damageable 전처리 몸체는 각 단위가 연결한다.
- CD 10.72 경로는 범위 밖이다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, 공통 `SquidPostPop` 본체, `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
