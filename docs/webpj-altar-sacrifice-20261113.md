# webpj 제단 몸체 RawAltarSacrifice (WB04-E368, 2026-11-13, 7-P3 신전·제단·의식 부품 네 번째 항목)

3.1절 7-P3 항목 "RawAltarSacrifice 263줄(+24)"을 `core/raw-altar-sacrifice.js`(제단 몸체·훅 공장)로 옮기고, 이 몸체가 쓰는 두 선행 부품을 함께 만들었다: `core/raw-ritual-regular.js` 에 제단 정기 분기 `runAltarRegular`·`AltarSacrificeState`(E367 에서 룬/다이스 분기를 만든 같은 모듈)를, 새 `core/raw-dais-deletion.js` 에 다이스·제단 고유 삭제 실행기 `RawDaisDeletion` 을 추가했다. 둘 다 cpppj 에서는 기준선(20261106) 뒤 새 파일(`RawRitualRegular`·`RawDaisDeletion`, `20261109-95`·`20261110-52 CP`)이라 PORT_MAP 의 대응표 밖이다. `RawAltarSacrifice` 자체도 두 커밋(정적 `FinishOnAltarForms`·실제 Player 기록 대여·`boss`/`count` 훅·현재 전역 읽기)을 반영했다(PORT_MAP base `21bb2550`).

## 1. 규칙

- **제단 정기 (`runAltarRegular`, 44a5e0):** 모든 사건에서 먼저 제단 위치의 다이스를 조회한다. **20/22** 는 제단 방향이 14(희생 중)일 때만 ±1 프레임 진행(22 가 구간 끝에 닿으면 +1 로 되돌림)하고 아니면 0, 진행이 끝나지 않으면 payload. **21** 은 권한이 있고 유효한 포획 사제가 있으면 원래 owner(하위 7비트)·다이스 owner(둘 다 ≤ 8)의 동맹이 있을 때 자동 해방 후 -1 → 방향 15 면 payload → 난수(0..99999)/1000 < 탈출 확률(40)이면 역진행 후 22, 아니면 20 으로 0.08 예약 → 다이스가 있고 방향이 10 이 아니면 사슬 소리(난수 0..4)와 저항 소리(난수/1000, 0..2)를 다이스 좌표 − 3.0 에서 → 반환은 난수 실수(0..2) + 저항 초(float32). **23** 은 payload, **24** 는 다이스 방향 10 이면 권한이 있을 때 다이스 삭제(0x400000)·-1(없으면 payload), 진행 중이면 이탈 프레임(count<3 이거나 난수 몫<50)·실수 < payload 일 때 붕괴 소리, 구간 끝이면 포획 사제 삭제(8)·애니메이션 제거·폭발 소리·다이스 K/제단 M 프레임 후 3.0. 그 밖의 사건은 공통 Damageable 로 보낸다. 다이스 SID 는 1..capacity-1 만 유효하다.
- **제단 몸체 (`RawAltarSacrifice`):** 생성자는 PATCH 풀과 여덟 협력 부품(발자국·룬·신전 의식·Carrier·다이스·프레임 진행·Host·Damageable 전처리)의 같은 풀, 188칸 표를 확인한다. `handle` 은 실제 제단 가상표(0x005076b8)만 받는다. `daisAt`(기준점 typed 다이스 조회: 좌표 절삭 → 현재 다이스 TYPE genus 의 표면 비트로 시작 단계 → 첫 일치)·`count`(조회한 다이스의 가상 +0x80, 없으면 0; 장치가 없으면 실제 다이스 가상표에서만 신전 의식 count)·`escapeFrame`(count 0~2: 방향 글자 첫 번호 + count 로 프레임 지정, count 0 은 방향을 `14 − 가상 count`(DWORD 감김, 0..15 밖은 거부)로 다시 정함 / count ≥ 3: 프레임 없이 구간 길이 × 1000 까지 난수만 소비).
- **해방:** `releasePriest` — 제단 조회 → 진단 `altar.isValid()`(0x3cb) → checking 이 켜졌고 제단 방향이 12·11 이면 진단 0x3cc → 권한이 없으면 진단 `boss`(0x3cd) → (진단이 정상 복귀하면 계속) 포획 사제가 있으면 생성(사제 TYPE, flags 0)·표면 WORD 지정(현재 다리 TYPE 이면 통지)·owner 0·제단 중심 +0.5 에 Pop·5초 효과·원본 삭제 → `priestFree.wav` → 강한 사제가 있으면 604 취소. `releaseFromAltar` — 자동 해방 부모 몸체(진단 0x578 → 다이스가 있으면 해방 → 제단 Unpop → **P 첫 번호를 프레임 DWORD 네 바이트에 직접 쓰기(표시 지정 없음)** → Repop).
- **삭제 전처리:** `daisPreDestroy` 는 실제 다이스 가상표만 받아 `RawDaisDeletion.daisPre` 를 부른다: extra 비트 1·8 이면 건너뛰고(공통 전처리만), 아니면 (삭제 flags 에 0x400000 이 있을 때) 제단 24 Form 완료 → 강한 사제 604 중지 → 발자국 안 제단 삭제(0x48) → owner 1..8 갱신 → 공통 Damageable 전처리.
- **고유 삭제 (`RawDaisDeletion`):** `altarPre`(다이스 삭제 → 발자국 안 추상 룬 정리: 권한이 있거나 클라이언트 번호 5..14999 → 전체 Damageable)·`daisPre`·`daisPost`(Player 기록의 두 칸 +0x94/+0x8c 을 비우고 +4 칸을 채우되 기록은 단계마다 다시 빌리고 owner 는 주소 계산마다 다시 읽음)·정적 `findTypeAt`/`findTypeAtPoint`(4b1fa0 점 조회)·정적 `finishForms`(제단 Form 사슬을 다음 링크를 먼저 저장하며 돌고 Regular/Shared 24 프로세스마다 희생 완료; 사슬 손상·process 종류·Kernel 누락은 거부).
- **훅 공장:** `makeAltarSacrificeHooks(pool, frame, host, factory, assets, random, hooks)` 는 `Frame.set`·`Host.addSharedRegular`·`Factory.create`·`RawAssetRegular.damageableHandle` 과 현재 게임 난수(정수 `low + next(high−low)` int32, 실수 `next(65536)/65536 × |high−low| + low` float32)를 연결한 새 묶음을 돌려준다(풀이 다르면 거부).

## 2. 웹 범위

- 10.72(CD)는 범위 밖이다(판본별 필드 폭·CD 전용 소리·기술 분기를 옮기지 않았다). 실제 UI·음향·삭제(`world.Destroy`)·Pop/Unpop·애니메이션 제거는 호출자 훅이며, 다이스 수명(RawDaisLifecycle)과 `DamageablePostDestroy` 연결은 다음 단위가 이어받는다. 이 모듈은 DOM 을 쓰지 않는다.

## 3. 검사 (`tests/altar-sacrifice.test.mjs` 17건 · `tests/dais-deletion.test.mjs` 6건 · 공용 장면 `tests/dais-support.mjs`)

- **원본 독립 관측(10.78):** `altarsacrifice-x86.tsv` 1,435행(모드 handle 1,080·pre 288·escape 50·free 8·count 5·release 4)과 `daisprogress-x86.tsv` 의 제단 정기 handle 1,056행의 반환 float 비트·효과 순서(F 프레임·destroy·N/S 예약·A/G 음향·X 애니메이션·unpop/repop·C/owner/P/E 해방·R 갱신)·난수 상태·Player 결과 DWORD·다섯 슬롯 raw(pre 모드는 마지막 공통 Damageable 진입 직전 경계)·제단 24 Form 조회 횟수가 **모두 첫 실행에서 일치**했다. 같은 풀의 실제 `RawFrameAdvance`/`SquidFrame`/`SquidProcessHost`/`RawDamageablePreDestroy`/`RawCarrierPreDestroy` 에 연결한 장면이다.
- **웹 계약:** 제단 정기 순수 분기(20/22 방향·진행 되돌림·23/그 밖·21 자동 해방의 권한·포획 사제 범위·동맹 인덱스 8/9 경계·탈출 확률과 20/22 선택·사슬/저항 소리표와 난수 범위·다음 지연·24 의 빈 프레임/이탈/붕괴/최종 붕괴와 TYPE 표 밖·SID 1·불규칙 좌표 바이트), 제단 몸체(생성자 거부·handles/handle 분배·daisAt 좌표/TYPE 범위/표면 단계·count·escapeFrame 의 count 별 프레임/DWORD 감김/난수 상한·releasePriest 진단 세 곳·생성/WORD/owner/Pop/효과/삭제/소리 순서·releaseFromAltar 의 P 첫 번호 네 바이트·daisPreDestroy 의 flags/extra 관문·훅 공장), 고유 삭제(생성자·altarPre 의 flags/죽은 다이스/추상 룬 영역 경계/권한·클라이언트 번호·daisPre 관문/순서·daisPost 의 호출 횟수와 owner 재읽기·기록 범위·typed 조회·finishForms 의 사슬 순회와 오류).

## 4. 변형 확인과 남은 경계

- 연산자·상수 변형 136(제단)+101(삭제)+130(정기 분기) = 367회를 적용했다. 1차(관측 대조만)에서 66·78·41개가 남았고 대부분 **시험의 공백**(관측 장면의 한 가지 자산·owner·난수 흐름에서 가려지던 인자·경계·바이트 폭)이라 위 웹 계약 시험으로 잡았다.
- 남은 동등 변형 13개: 임시 `DataView` 크기 4→5 세 건(`setUint32/getFloat32` 만 씀)·좌표 절삭 하한 −2147483648 → −2147483649 두 건(그 사이에 float32 가 없음)·`#valid` 의 `sid < capacity` → `<=`(조회가 돌려주는 SID 는 항상 실제 슬롯)·`runAltarReleaseParent` 의 checking 값 1→2(비영 판정)·해방 사제 WORD 폭 2→3(`setRawSquidWord` 가 하위 16비트만 씀)·Player 기록 `offset > length` → `>=`(바로 이어지는 `length − offset < 4` 가 같은 경우를 거부)·프레임 DWORD 상위 바이트 시프트 24→25(그만한 코드표가 없어 어느 쪽이든 범위 오류)·정기 분기의 초기 `center` 배열 값(다이스가 있을 때만 쓰고 그때 덮어씀).
- 시험이 먼저 실패한 곳은 모두 시험 쪽이었다(제단 발자국 영역을 다이스 것으로 착각한 좌표, 정상 Form 이 지워지며 부르는 정리 훅이 금지 훅이던 입력, 해시 사슬에 남은 표 밖 TYPE 이 finder 에서 먼저 거부되는 입력, 제단 24 에서는 읽지 않는 제단 방향을 기대한 입력). 모듈은 관측 대조에서 바뀌지 않았다.
- 다음 7-P3 항목은 **RawDaisLifecycle**(192줄 +15; RawAltarSacrifice 선행은 끝남)이며, `RawDaisDeletion.altarPre/daisPost` 와 `DaisContentNotice`·`DamageablePostDestroy` 를 쓰고 `DaisRitualConsumer`(최초 Pop → 포획 부모 알림 → 제단 24 → 전체 삭제) 통합 시험과 함께 다룬다.
