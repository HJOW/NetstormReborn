# webpj 다이스 추가 그림·메타 배율/추상색 추가 Draw(RawDaisDraw·RawAuxiliaryDraw) 이식 — WB04-E84

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 여든네 번째 부분이다. 다이스(Dais) 165 의 별도 TYPE 169 추가 그림 요청(`o/RawDaisDraw`, 004484b0)과 메타 배율 표시(`o/RawAuxiliaryDraw`의 `DrawScaledMetadisplay`, 00479340)·고정 추상색 추가 Draw(`DrawAbstractAdditional`, 00485f90)를 cpppj 기준선 `1f00a25a` 에서 옮겼다. 마지막 VFX 출력 경계는 훅이 받고 슬롯은 바꾸지 않는다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-dais-draw.js` — `isPatchDais(raw)`·`drawDaisAdditional(raw, extraFrameCount, draw)`·`PATCH_DAIS_EXTRA_TYPE`(169)

- `isPatchDais`: 슬롯의 가상 표가 0x507950 이고 TYPE 이 165 일 때만 참이다(짧은 슬롯은 거부). `drawDaisAdditional`: 현재 프레임 DWORD(+36)를 signed 로 읽어 TYPE 169 의 현재 전체 프레임 수로 나눈 signed 나머지(`-0` 없음)를 `draw(169, 프레임)` 에 넘긴다. 다이스가 아니거나 프레임 수가 0 이하이거나 소비자가 없으면 거부하며, dead/추상/로컬 관문은 497280 소비자 몫이다.

`www/core/raw-auxiliary-draw.js` — `drawScaledMetadisplay(raw, x, y, hooks)`·`drawAbstractAdditional(raw, x, y, hooks)` (`hooks = { shape(TYPE), draw({ shape, frame, x, y, abstract, scale }) }`)

- `drawScaledMetadisplay`: +26 DWORD 의 숨김 비트 0x4000 이면 프레임/SHP 장치 없이 조기 반환한다. 아니면 정수 배율 `(+26 & 0x3fff) << 16` 에 분수 WORD(+8)를 더하고 0 이면 1배(0x10000)로 바꾼 뒤 현재 프레임(+36)을 읽고 현재 TYPE(+10)의 SHP 를 조회해 원색 배율 출력을 요청한다.
- `drawAbstractAdditional`: 소유자/dead/visible 관문 없이 현재 프레임과 TYPE 의 SHP 를 읽어 고정 추상색 치환표(abstract true, scale 0)로 한 번 출력한다. 출력 분기에 도달했을 때만 훅을 요구한다.

## 2. 검증

- `tests/dais-draw.test.mjs`(2건): `dais-draw-x86.tsv` 768행의 signed idiv 부호/감김(프레임 DWORD·프레임 수 조합, 슬롯 불변)과 소비자 요청이 같고, 다른 VT/TYPE·손상된 프레임 수·소비자 누락·짧은 슬롯 거부와 음수 프레임의 `-0` 방지를 확인한다.
- `tests/auxiliary-draw.test.mjs`(2건): `auxiliary-draw-x86.tsv` 1728행(Scaled 864·Abstract 864)의 출력 요청 `추상,SHP,프레임,x,y,배율,배율`과 SHP 조회 `TYPE,토큰`이 같고 슬롯이 변하지 않으며, 숨김 조기 반환은 훅 없이 성립하고 도달한 분기의 누락 훅·짧은 슬롯을 거부하고 배율 합산(0→1배, 정수 3+분수 0x8000=0x38000)을 확인한다.

최초 실패와 교정: 새 모듈은 위 fixture 2496행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 497280/VFX 소비자(추상/로컬 관문, 치환표 적용)와 SHP 토큰 조회는 `RawShapeToBuffer`·표시 단위에서 연결한다. 같은 계열의 템플 Draw(`RawTempleDraw`, 264행)는 보호 자산(TYPE baseFrame·FrameTable)이 필요해 자산 시험 장치와 함께, 마나 추가 Draw(`RawManaDraw`)는 전용 fixture 가 없어 후속으로 남긴다.

## 4. 다음 시작점

`RawFenceTarget`(선행 `RawProjectileGeometry`)→`RawGunBlocker`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
