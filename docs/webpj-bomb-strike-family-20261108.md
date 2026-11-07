# webpj Bomb 주문 나머지(RawBombTargets·RawBombBlast·RawBombEffect·RawBombStrike) 이식 — WB04-E54

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 쉰네 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawBombTargets`(원본 41e4b0 대상 검색, 41e3c0 기본 반경, 41e460 중심), `o/RawBombBlast`(502370 폭발 계열 가상 bc 자격 41e9c0·c0 효과 41ea30), `o/RawBombEffect`(0x5f 주문 효과 수신 420bf0, 송신 observer 40aca0), `o/RawBombStrike`(네 직접 피해 주문 가상 표 502908·5029d0·502aa8·502b70)를 옮겼다. 이로써 Bomb 계열 몸체가 모두 끝난다(남은 `client/RawBombBlastBindings`는 월드 조립). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-bomb-targets.js` — `RawBombTargets(pool, hash, types, ranges, {boss}, {radius, eligible, assertion})`

- `defaultRadius(parent)`: 현재 TYPE+44 float 반경의 `_ftol` 하위 DWORD(signed). `center(parent)`: 좌표를 `+0.99999`(float 502310) 정수 절삭 뒤 `−0.5`, 1..255 제한한 캡처 중심(폭발과 공유).
- `find(parent, output)`: 권한이 없으면 복귀 가능한 진단 `0x130`을 먼저 부르고, 중심과 가상 b4 반경을 순회 전에 캡처한다. `[중심−반경, 중심+반경]`(왼쪽/위 절삭, 오른쪽/아래 +0.99999 스냅)을 실제 `RawSquidFinder` 네 단계 해시로 순회하면서 예약 SID(5..14999)는 건너뛰고, 후보의 **현재** TYPE 발자국·extra 비트 1·상태 비트 2를 걸러 `(좌표 − 발자국/2)`와 중심의 거리가 `발자국/2 + 반경` 이하이면 파생 자격(가상 bc)을 묻고 출력 헤더의 현재 `count < capacity`(signed)일 때만 실제 버퍼에 저장한다. 반환 기하는 중심 2·signed 반경·float 사각형 4.

`www/core/raw-bomb-blast.js` — `RawBombBlast(pool, types, frames, coordinates, state, hooks)`

- `handles(parent)`: 가상 표 502370. `mayTarget(target)`: 표면 genus(4)는 번호 0이면 참, 아니면 현재 프레임 코드의 보호 비트(0x40)가 꺼져 있을 때 참(프레임 범위 밖 거부); 성장 중 비 방해 TYPE(150)은 참; 그 밖은 HP 보유(dais 추상은 extra 비트 1)·허용 genus `0x80074200`.
- `apply(parent, target)`: 운반체(`+12`)면 무효과. 표면: 캡처 중심·대상 발자국 중심 거리가 `반경 + 발자국/2 − 1` 이내(비순서 포함)면 삭제(0x100048), 밖이고 권한이 있으면 끝 칸 변환(edge). 비표면: 피해 전에 대상 중심 캡처 → 사제(158)가 부모와 동맹이면 끝 → 성장 중 비 방해(150)는 권한이 있을 때 삭제(0x8100008), 그 밖은 피해 1000 → 캡처한 중심에 ground 효과.

`www/core/raw-bomb-effect.js` — `RawBombEffect(pool, types, state, hooks)`, `finalizeBombEffectPacket(packet, writing)`

- `receive(packet)`: 부모가 free/dead면 끝. 운반체가 살아 있으면 시전 프로세스 종료 + 부모 현재 TYPE 비용(`_ftol` 하위 DWORD)을 누적(공유 자원 DWORD면 그 unsigned 비트를 다시 읽는다). 최초 대상 수 검사(음수/버퍼 밖 거부) 후 배열 확보(실패는 오류), 매 반복 현재 패킷의 대상 수를 다시 읽어 free/dead 아닌 대상에 부모 현재 가상 표로 `apply`, 최초 한도까지 저장. 권한이 있고 부모 TYPE이 117/118이면 종류를 먼저 캡처, 40바이트 확보 성공 시 지연 객체 생성(실패해도 객체 0으로 예약 인자·부착은 이어짐). 마지막으로 현재 TYPE 소리와 현재 가상 표 삭제(0x100048), 최초 배열은 정상/예외 모두 한 번 해제.
- `finalizeBombEffectPacket`: 쓰기일 때만 현재 대상 수(≤100, 버퍼 안)로 선언 길이 WORD를 `10 + 2×수`로 확정.

`www/core/raw-bomb-strike.js` — `RawBombStrike(pool, types, random, state, hooks)`

- `handles`/`mayTarget`: 네 표만 받고 자격은 현재 HP·free/dead를 보지 않는다(첫 표는 `genus & 0x120000` 마스크 값을 그대로 반환). `apply`: ① 502908 생성+pop 뒤 특별 종류(138)가 아니면 피해 2000, 맞고 권한이 있으면 삭제(0x100008) ② 5029d0 dead면 끝, 소리 → 저장한 운반체가 살아 있으면 빔 → 피해 500 ③ 502aa8 dead면 끝, 두 번 투영한 기둥(수명 5)+소리, 난수 1~4회 반복·3분의 2 확률로 피해 100~499(피해 전 HP<1 중단, HP<피해 중단) ④ 502b70 난수 1~49회·절반 확률로 생성/pop 뒤 피해 20~49(초기 dead 관문 없음).

## 2. 검증

- `tests/bomb-targets.test.mjs`(2건): `bomb-targets-x86.tsv` 300행. 실제 네 해시 순회·기하 비트·자격 호출·100칸 배열 전체·출력 헤더·13슬롯·현재 발자국(두 정밀도). 계약: 미도달 자격 요구 안 함, 물리 버퍼 범위, 구성 오류.
- `tests/bomb-blast.test.mjs`(2건): `bomb-blast-x86.tsv` 2082행 + `bomb-blast-dais-x86.tsv` 64행. 자격/직접 효과/합성 호출, 변형 1~11, 반환·효과·네 슬롯·권한·발자국. 계약: 하위 누락·잘못된 프레임/owner·다른 풀.
- `tests/bomb-effect.test.mjs`(4건): `bomb-effect-x86.tsv` 1676행(10슬롯·패킷·누적·두 전역). 계약: 패킷 경계, 확보 실패/누락/예외 해제, 송신 observer, 공유 자원 DWORD(`GameAuxState`)의 unsigned 재조회.
- `tests/bomb-strike.test.mjs`(2건): `bomb-strike-x86.tsv` 1420행(반환·효과·다섯 슬롯·공유 난수·권한). 계약: 미도달 효과, 하위 누락, 계열별 초기 dead 관문 차이.

최초 실패와 교정: 네 모듈 모두 5542행 전부를 첫 실행에 통과했다.

## 3. 경계

- 실제 피해(가상 98)·삭제(가상 10)·지면 효과(470ac0)·다리 끝 칸 변환(421f70→4215d0)·생성/pop(4af530/가상 90)·소리·빔(472700)·투영(497220)·기둥(472770)·대상 검색 전달/지연 객체 생성/부착은 월드 조립에서 공급한다. 가상 b4 반경/bc 자격의 파생 분배는 호출자가 연결한다.
- cpppj `RawBombBlastBindings`(client)는 위 몸체를 세션에 연결하는 어댑터로 월드 조립 단위에서 대조한다.

## 4. 다음 시작점

Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState`·`RawPlayerGeyserBounds`·`RawPlayerPlacementAnchor`), 종류별 PostPop 본체(Asset·Fence·Nugget·Buried·Carrier·Factory·Priest·Rune), `SquidDisplay`/`SquidFrame`이다.
