# webpj 3-E 판정-전투 — WB04-E263 (2026-11-10)

3.1절 3-E의 다섯 번째 항목 **판정-전투** 18단위를 판정했다. 방법과 ①/②/③ 기준은 [판정-공통층 문서](webpj-judge-common-20261110.md)와 같다: 이식 원본은 기준선 `1f00a25a` 판이고, `git diff --ignore-cr-at-eol 1f00a25a HEAD -- <단위 파일>`(18단위 `.cpp/.h` 약 330줄 전부)과 새 API 의 소비자를 읽어 단독 미션의 웹 결과가 바뀌는지만 판정했다. 판정 기준 커밋은 착수 때 `HEAD` `93d891ac`(20261110-117 WB)이며 행 base 는 단위 파일을 마지막으로 바꾼 커밋이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 판정 표

| 단위 | 판정 base(기준선 뒤 커밋) | 기준선 뒤 변경 | 판정 | 처리·시점 |
| --- | --- | --- | --- | --- |
| RawBombEffect | `01ec27ae` (1) | `ObserveBombEffectPacket`: 송신일 때만 DWORD -1 진단 뒤 현재 low WORD 로 길이 확정 | ③ | **4-C**(패킷 송신 observer). 독립 typed 길이 마무리 불변 |
| RawBombEffectTimer | `2b5ee903` (1) | `WritePacket` 의 선언 길이가 DWORD 입력, protocol/message 선택 | **②** | **재반영**: 웹 `writePacket(packet, declaredLength, protocol, message)` |
| RawBombRegular | `01ec27ae` (1) | Native 현재 TYPE reader 생성자·`sharedBoss`·`currentLocalOwner`·`targetBuffer`(방금 확보한 실제 DWORD 100) | ③ | **4-D**. 독립 typed 불변 |
| RawBombStrike | `473e1247` (1) | `currentBoss`/`SpecialType`/`FirstEffectType`/`HailEffectType` 를 소비 위치마다 읽는 getter | ③ | **4-D**. 독립 typed 불변 |
| RawBombTargets | `d0e7fe1d` (1) | 현재 TYPE+44 사거리를 직접 읽는 생성자 | ③ | **4-A**(RiftType 새 칸 `range`)·4-D. 독립 typed 의 명시 `ranges` 불변 |
| RawBombTimerPacket | `2b5ee903` (1) | `WriteBombTimerPacket` 선언 길이가 `uint32` | **②** | **재반영**: DWORD 로 비교(4 미만이면 4)한 뒤 하위 WORD 만 기록 |
| RawArcherAnimation | `02b00b12` (1) | `currentType`(실제 Sun/Wind Archer TYPE 주소)을 생성/부모 검증/가상 getter 가 공유 | ③ | **4-D**(애니메이션 프로세스 TYPE 두 칸·`RawCampaignArcherBindings` 이식과 같은 시점) |
| RawCannonFire | `02b00b12` (1) | `currentType`(Sun 캐논 00541258 대여) | ③ | **4-D** |
| RawCrossbowAnimation | `02b00b12` (1) | `currentType`(00541288 대여, 수신 공장과 공유) | ③ | **4-D** |
| RawGunAnimation | `02b00b12` (2) | 수신 생성자·공통 읽기/쓰기·VT 별 writer 선택·`currentSunCannonType`·`host.AnimationProcessType` | ① + ③ | 앞의 셋은 **E113 이 이미 이식**(웹 `initializeReceivedGunAnimation`/`readGunAnimationProcessData`/`gunAnimationUsesFirePacket`/`writeGunAnimationProcessData`, fixture `gunanimation` 1280행). 나머지는 **4-D** |
| RawGunHit | `230528e3` (1) | `hooks.applyOverride`: Rain 45f940 의 source+c0 전용 진입을 generic 표적 관문보다 먼저 선택 | ③ | **7-P2**(총·투사체)·4-D. 미설치 typed 경로는 기존 본문 그대로 |
| RawGunTarget | `a84509c9` (1) | `friendlyFire` 가 `RawCheckingFlag`(현재 55a490 DWORD 비영 뷰)로 바뀜 | ① | 웹은 truthiness 판정이라 동일 |
| RawIceCannonFire | `02b00b12` (1) | `state.Type()`(현재 부모 TYPE 주소) | ③ | **4-D** |
| RawMissile | `cc562ccf` (1) | Sun 캐논/Sun 아처/윈드 아처/썬더/레인 캐논 역할 DWORD 를 발사자 비교 위치마다 읽는 shared getter | ③ | **4-D** |
| RawAcidFence | `800ff40c` (2) | `friendlyFire` 비영 뷰, 레인/썬/윈드 플라이어·벌룬 TYPE 의 shared getter | ① + ③ | 뷰는 ①, getter 는 **4-D** |
| RawFenceCreate | `d0e7fe1d` (1) | `RawFenceMarkerCreate` 의 중심 getter 생성자(별도 GunTarget 상태 없이 004adea0 중심 공유) | ③ | **4-D** |
| RawFenceMarkerClear | `d0e7fe1d` (1) | 중심 getter 생성자 | ③ | **4-D** |
| RawFenceNeighbors | `d0e7fe1d` (1) | 현재 TYPE+44 사거리·동맹 DWORD getter 생성자와 공개 `Center` | ③ | **4-D** |

합계: ② 재반영 2(RawBombEffectTimer·RawBombTimerPacket, 하나의 변경)·① 1(RawGunTarget)·①+③ 혼합 2(RawGunAnimation·RawAcidFence)·③ 13 — 18단위가 모두 판정됐다.

## 2. ② 재반영 (선언 길이 DWORD)

cpppj 는 상태 타이머 패킷 writer(41e280)의 선언 길이 매개변수를 `uint16_t` 에서 `uint32_t` 로 바꿨다(원문은 DWORD 입력). 웹 `raw-bomb-timer-packet.js` 는 `declaredLength & 0xffff` 로 먼저 16비트로 잘랐기 때문에 `0x10001` 같은 값이 4 로 올려졌으나, 실제로는 DWORD 비교(`max(4, 0x10001)`)를 거친 뒤 하위 WORD 만 기록되어 `1` 이 남는다. 웹을 `Math.max(MIN_DECLARED, declaredLength >>> 0)` 로 고쳤고 `raw-bomb-effect-timer.js` 의 `writePacket` 이 `protocol`(기본 1)·`message`(기본 0x7d)를 전달한다.

## 3. 검사와 변형 확인

- 신규 시험 1건: `tests/bomb-effect-timer.test.mjs` 의 `bomb-timer-packet: 선언 길이는 DWORD 로 비교하고 하위 WORD 만 쓴다`(0→4, 3→4, 4→4, 0x1234→0x1234, 0x10001→1, 0x10010→0x10, 0xffffffff→0xffff). 옛 `& 0xffff` 로 되돌리면 실패하고 다시 적용하면 통과한다. 기존 원본 fixture 대조(`bomb-effect-timer` 5건)는 그대로 통과한다.
- 단위 검사: `node --test tests/bomb-effect-timer.test.mjs` 통과.

## 4. 대응표·문서 반영

- `PORT_MAP.tsv` 18행의 base 와 메모를 갱신했고 `node webpj/tools/port-map.mjs` 오류 0, 18단위의 `cpppj 변경` 보고가 사라진다.
- LEFT_JOBS.webpj.md 3.6절 표에 ③ 묶음 2행(4-D 전투 부품 Native 역할/TYPE getter, 4-C·4-A·7-P2 시점의 패킷 observer·TYPE 사거리·Rain 오버라이드)을 더했다. [대응표 문서](webpj-port-map-20261106.md) 9절 표에 이 묶음을 더했다.

## 5. 남은 경계와 다음 시작점

- 다음은 3-E 마지막 항목 **판정-기타**(6단위: RawFlare·RawIceTower·RawPathRuntime·RawWindAviaryProcess·RawWindAviaryState·client/RawChatChannels)이다. 이 항목을 마치면 3-E 가 닫혀 순서3(M2 선행 부품)이 마감된다.
