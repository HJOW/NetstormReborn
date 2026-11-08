# webpj Carrier 낙하·포획·내용물 통지·너겟 전달(RawCarrierFalling·RawCarrierPickup·RawCarrierContentNotice·RawCarrierTransfer) 이식 — WB04-E73

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 일흔세 번째 부분이다. 운반체(Carrier)의 위험한 다리 판정·가속 낙하·포획 사제 해방(`o/RawCarrierFalling`), 일반 포획 요청/수신·기존 내용물 제거(`o/RawCarrierPickup`), 내용물 가상 통지(`o/RawCarrierContentNotice`), 미등록 187 캐리어의 주기 Halt와 너겟 전달(`o/RawCarrierTransfer`)을 cpppj 기준선 `1f00a25a` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기와 10.37 행, x87 확장 정밀도(64비트) 관측은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-carrier-falling.js` — `RawCarrierFalling(pool, types, frames, surface, players, boss, checking, priestType, mapLimit, hooks)`

- `needsEscape(x, y)`(4225b0): 좌표에 float32 0.9999 를 더한 `_ftol64` 하위 signed 칸이 0..255 안이고 그 칸의 표면 머리가 있으며 현재 TYPE genus 에 비트 4(다리)가 있을 때 현재 프레임 코드의 flags & 0x20 을 반환한다(아니면 0).
- `fall(부모, 개수)`(426ef0): signed 개수가 100 을 넘으면 권한자가 아닐 때 1, 권한자는 포획 사제(mask 0x20, 현재 사제 종류)를 찾으면 해방한 뒤 삭제하고 −1. 개수가 10 이면 +35 BYTE 를 20 으로 쓰고, 그 밖에는 가속 낙하 y(`(개수×0.5+1)×0.0625+y`)를 254 로 제한(NaN 보존)해 Unpop → Pop(flags 0x40) 후 0.05를 반환한다.
- `releasePriest(부모)`(426c80): 진단 전역이 켜져 있는데 권한자가 아니면 원문 진단(Carrier.cpp 3bf), 권한자가 아니면 끝. 권한자는 포획 내용물을 찾고 현재 사제 종류로 생성해 WORD 를 옮기고 소유자 0 을 지정한 뒤 이전 소유자의 Player 기록 +0xa8/+0xac 복귀 좌표가 내부 좌표가 아니면 진단(3d0)을 요구하고 Pop → 낙하 가상 → 포획 내용물 삭제한다. `validPoint(x, y)`(40e800)는 엄격한 내부 좌표 비교(0 < x, y < 현재 지도 제한 float)이다.

`www/core/raw-carrier-pickup.js` — `CarrierPickupState`, `RawCarrierPickup(pool, types, contained, boss, single, state, hooks)`

- `request(부모)`(428e00): 현재 지점의 후보(426900)를 조회하고 후보가 없거나 권한자가 아니면 끝. 단독 미션이 아니면 후보의 genus 를 확인한 뒤 권한을 다시 읽어, 9바이트 패킷(프로토콜 하위 7비트, 메시지, 길이 WORD, 부모 WORD, 후보 WORD, 후보 TYPE)을 대상 9 로 전송한다(요청에서 로컬 수신하지 않음). 요청 헤더는 `{ message(0x4f), protocol(1), length(9) }` 이다.
- `receive(소유자, flags, 패킷)`(426520): 길이 9 미만은 거부. 최초 부모/대상 WORD 를 보관하고 권한자가 아니면 원문 진단(Carrier.cpp 20c) 뒤 권한을 다시 읽어 여전히 아니면 끝. 대상/부모가 free 또는 dead 이거나 TYPE 이 패킷과 다르면 끝. genus 0x2000 이면 오벨리스크 보상 객체로 변환한 뒤 입력 큐에 넣고, genus 0x2100 이면 기존 내용물을 제거(`discard`)한다. 대상을 임시 dead 로 만들고 부모에 같은 종류의 ContainedForm 을 생성·부착한 뒤 dead 만 되돌리고, 대상의 종속을 다음 항목을 먼저 읽으며 새 내용물로 옮기고 대상을 삭제한다. `discard(부모)`(4264d0)는 현재 첫 mask 1 내용물의 종류를 보관해 삭제하고, 삭제 효과 뒤의 현재 특정 타입(silentType)과 같지 않으면 전역 보충을 부른다.

`www/core/raw-carrier-content-notice.js` — `RawCarrierContentNotice(pool, types, state, hooks)`

- `handles(부모)`: 부모의 현재 가상 표가 고르는 내용물 통지 진입점이 `00429560`(WB04-D5a `RawContentParentNotice.nativeEntry`)인지 조회한다. 알 수 없는 VT 는 거부한다.
- `notice(부모, 내용물, reason)`(00429560): 저장 게임이면, 로딩 중이면 전부 생략한다. 내용물이 dead 가 아니고 kind 의 genus 에 0x100(폭탄)이 있고 폭탄 억제가 꺼져 있으면 부모 중심점의 두 화면 투영(16/11배·0.5·`_ftol64`·카메라 감김)으로 번개(5.0)와 `thunderCrack.wav` 를 내고, 부모 소유자가 로컬이면 40바이트를 확보해 내용물 생성(타입 +6c 추가 DWORD)과 1.5초 타이머를 설정한다. 그렇지 않고 부모 소유자가 로컬이고 내용물이 현재 사제 종류이며 dead 가 아니면 타입 +7c 추가 DWORD 의 소리를 낸다. 이어서 reason 비트 0·+20 WORD·genus 0x208100 이면 Carrier 지면 처리, reason 비트 1 이고 내용물이 사제 종류이면 현재 부모 위치에 genus 0x400000 객체도 현재 156 타입(rainWalker)도 없을 때 `priestFree.wav` 를 낸다. 모든 전역과 raw 는 각 원본 읽기 위치에서 다시 읽는다.

`www/core/raw-carrier-transfer.js` — `CarrierTransferState`, `RawCarrierTransfer(pool, hash, types, boss, editor, localOwner, state, hooks)`

- `halt(부모)`(42ac60): 모든 미등록 187 캐리어가 공유하는 카운터를 DWORD 로 1 올린 뒤 signed 로 4 로 나눈 나머지(음수 나머지를 정규화하지 않음)로 바꾸고, 0 이면 `transfer`, 아니면 공통 Halt(42a5f0)를 부른다.
- `transfer(부모)`(42a0d0): 권한자이고 부모에 첫 mask 0x20 내용물이 없고 편집 모드가 아니어야 한다(아니면 10). 현재 좌표의 한 칸 일반 finder 를 만든 뒤 공유 cooldown(signed 양수일 때)을 하나 줄이고, cooldown 이 0 인 동안 후보를 돌며 같은 소유자의 미등록 187 후보가 현재 너겟 타입 내용물을 가지면 마지막 기증자를 기록하고 cooldown 5, 10바이트 전달 명령을 로컬에 먼저 수신한 뒤 송출하고 기증자의 이동을 제거, 간헐천 후보로 이동 명령, 내용물 kind 의 genus 0x8000 이면 귀환 요청 후 12 를 반환한다(없으면 10).
- `receive(소유자, flags, 패킷)`(426830): 길이 8 미만은 거부, 내용물/부모가 free 이면 끝, 아니면 내용물을 떼어 부모에 붙이고 미리보기 예약(426080)을 부른다. 선행 두 인자와 packet+8 은 읽지 않는다.

## 2. 검증

- `tests/carrier-falling.test.mjs`(3건): `carrier-falling-x86.tsv` 9884행(다리 판정/낙하/사제 해방/내부 좌표의 반환 DWORD, 호출 순서 M/K/C/W/O·o/D·d/U/P·p/F·f/A, 부모/포획 Form/새 사제 raw, 권한, 사제 종류; 효과 안에서 가상 표·권한·종류·내용물 WORD·플레이어 기록을 바꾸는 변이 포함), `carrier-falling-vtables.tsv` 8행(사제/캐리어/보호막의 서로 다른 낙하 슬롯 선택 근거), 선택된 하위가 필요해질 때의 미연결 거부와 물리 프레임 범위 진단.
- `tests/carrier-pickup.test.mjs`(2건): `carrier-pickup-x86.tsv` 2657행(요청/서버 수신/기존 내용물 제거의 호출 순서 P/S/I/V/Q/F/B/C/D/U/A, 최종 현재 전역 8칸, 아홉 물리 슬롯 raw 전체, 송신 패킷; 실제 `RawContainedFinder` 로 종속 이동), 선택하지 않은 효과 비요구와 짧은 패킷·도달한 경계 누락·연결 오류 거부.
- `tests/carrier-content-notice.test.mjs`(2건): `carrier-content-notice-x86.tsv` 1731행(고정 입력 21열 → 외부 효과 호출 순서 C/B/S/A/N/T/P/H/G/I/F, 효과 뒤 현재 전역 9칸, 부모/내용물 슬롯 50바이트 둘), 실제 파생 VT 만 분배하고 읽지 않아야 할 분기·선택된 필수 효과 누락을 구별.
- `tests/carrier-transfer.test.mjs`(2건): `carrier-transfer-x86.tsv` 1110행(전달/주기 Halt/수신의 호출 순서 F/D/A/P/S/M/X/G/V/H/C, 최종 상태 7칸, 네 물리 슬롯 raw, 송출 패킷; 실제 `RawSquidFinder` 와 공간 해시 사용), 주기 카운터가 부모별로 분리되지 않고 signed 감김을 보존하며 도달하지 않은 효과를 요구하지 않음.

최초 실패와 교정: 낙하·내용물 통지·너겟 전달은 첫 실행에 통과했다. 포획은 요청 헤더의 필드 순서를 잘못 옮겨(cpppj `FormPacketHeader` 는 메시지 BYTE 가 먼저, 프로토콜 BYTE 가 다음) 상태 문자열이 어긋났고, 모듈의 기본값과 시험 입력의 순서를 교정해 해결했다.

## 3. 경계와 남은 것

- 다리 후보 조회(426900)·오벨리스크 변환(4620c0)·입력 큐(452fe0)·ContainedForm 생성·부착·사제 생성/해방, 번개(00472770)·소리·타이머 생성, 이동 제거·간헐천 선택(429420)·이동 명령(427600)·귀환(428fd0)·공통 Halt 본체는 해당 파일 이식 때 연결한다.

## 4. 다음 시작점

`RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission` 이다.
