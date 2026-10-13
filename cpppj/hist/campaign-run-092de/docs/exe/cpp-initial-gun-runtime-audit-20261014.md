# 실제 초기 GunProcess 연결 읽기 감사

현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. 2026-10-14의 현재 C++와 `extracted/refined/{originals,originalCD}/Netstorm.c`를 읽었다. 주 복원 대상은 패치 **10.78**이며 CD **10.72**의 주소/필드 차이도 기록한다. 이 문서는 새 원본 격자나 초기9개 C++ 검사 성공을 주장하지 않는다. 기존 GunProcess 2996 보고서의 당시 `VM-W11-CODEX`/SHA와 기존 Animation/Missile 관찰은 보존한다.

## 초기9개 생성·부착

`RawAssetPostPop.createGunProcess(parent)`를 같은 `RawWorldCore.processes`의 `CreateGunProcess(host,logic,parent,34)`에 연결할 수 있다. 실제 경로는 `RawGunProcess::Initialize` → `AttachGunProcess` → `SquidProcessHost::Attach(flags0)`이다. 이 factory는 `GunProcessHooks.attach`를 호출하지 않는다. 단독 `RawGunProcess::Construct`를 사용하는 소비자만 이 훅이 필요하다.

현재 실제 TYPE의 `group`을 사용해야 한다. sunArcher71은 group0, sunCannon128은 group1이다. 원본 생성자는 두 값 이외를 assert한다. 초기 필드는 target0, stage0, 현재 game now, 현재 group, dirtyff다. flags0 서버 부착은 전송을 요구한다. `ProcessFormTransmission.writeData`에서 실제 `GunProcess::WritePacket`으로 작성하고 `transmit`에서 해당 실제 바이트를 저장/송신해야 한다. Host의 누락 예외를 flags10/50으로 우회할 근거는 없다.

생성 뒤 같은 Kernel의 첫 stage0 프레임은 자연스럽게 stage1로 간다. 부모가 live이고 extra abstract/astral/blocked가 없으며 진단 전역이 꺼진 경우 이 첫 단계는 다른 Gun 효과 훅을 호출하지 않는다. 다음 stage1부터 실제 검색 경계를 요구하므로 초기 생성 성공을 전투 완료로 세지 않는다.

## 같은 Core에서 공급 가능한 경계

| 경계 | 현재 공급안과 실제 범위 |
|---|---|
| 생성/부착 | `CreateGunProcess(Core.processes,logic,parent,34)`; `Core.pool/types/kernel/processState`와 실제 packet writer를 공유한다. |
| Form 재전송 | `fields.Form()`을 `Core.processes.FromForm`으로 찾아 실제 GunProcess인지 확인한 뒤 `RefreshForm`을 호출한다. 0041c010/CD0048f8d0의 Unpop0→같은 부모 Attach0/전송을 재사용한다. |
| 표적 검색/발사 가능 판정 | 동일 pool/hash/types/shapes의 `RawGunTarget` + `MakeGunTargetHooks`. 검색과 MayFire만 연결하며 이 `shoot` 훅은 실제 Missile 생성 경계가 아니다. |
| 발자국 중심 | `RawGunTarget::Center(sid)` 또는 같은 TYPE footX/footY를 사용하는 기존 실제 중심 몸체다. |
| 공간 프레임 갱신 | 같은 `Core.unpop/pop/display/frame`을 사용한다. 원본 Unpop0→현재 frame 쓰기→Repop40의 실제 파생 PostPop 연결을 유지한다. |
| 표적 감시 | 같은 pool/destroy의 `DependFormHost.Create(target,fields.Form())`, `Remove(fields.Form(),target,0)`를 사용한다. 서버 DependForm 전송과 삭제 통지를 같은 Form 분배기에 연결해야 한다. 초기0 표적에는 새 쌍을 만들지 않는다. |
| 현재 Animation 조회 | 부모의 실제 Form 체인을 원본 순서로 순회하여 process35/36의 첫 일치를 반환한다. 00469070/CD004349e0은 fire/rotate 두 TYPE 번호를 비교하며 Kernel 전체를 임의로 검색하는 근거가 아니다. |
| Animation 중지/완료 | 현재 Form→실제 GunAnimation/ArcherAnimation 확인 후 `Core.processes.Kill(flags)`; 각 `Make*AnimationHostHooks`가 Gun 완료 Notify와 flags10 삭제를 이미 연결한다. |
| Animation 생성 | 먼저 `MakeGunAnimationProcessHooks`(128), 그 위에 `MakeArcherAnimationProcessHooks`(71)를 합성한다. 반대 순서로 호출하면 캐논 adapter가 Archer fallback을 덮는다. 실제71은 group0라 stage2 회전을 건너뛰고 stage5 Fire로 간다. |
| 투사체 생성 | `MakeGunMissileHooks`/`MakeArcherMissileHooks`의 동일 Host·`RawMissile` factory다. Missile의 표시자산 생성/Pop/표시·피해·impact·삭제 훅까지 같은 Core로 연결된 경우에만 실제 생성 완료다. |
| 차단 읽기 | 원본 부모 extra bit20: 패치 raw+40, CD raw+35를 읽는다. 복사된 전역 차단값으로 대체하지 않는다. |
| 난수 | 같은 `GameRandom` 상태의 실제 LCG를 한 번씩 전진한다. float 구간은 `upper16*(1/65536)*abs(max-min)+min`의 넓은 반환값이다. 별도 RNG나 CRT rand를 만들지 않는다. |

## 새 작은 몸체/메타데이터 또는 명시 거부가 필요한 경계

- **targetingMode**: 004d4d00/CD00443110. 현재 frame 코드 첫 BYTE를 signed char로 읽고 `side*2-0x98`, 0..7 밖이면0이다. WindArcher140의 nonastral은 실제 aimDirection(raw+30)의 두 배다. 초기71/128만의 소비자는 같은 `Core.frames[type].Codes()[currentFrame].side`로 이 작은 몸체를 재사용할 수 있다. owner 인수를 공격 방향으로 오해하면 안 된다. 현재 코드에는 전용 공개 adapter가 없다.
- **shotContext**: 패치0045ae60의 8DWORD는 현재 raw x/y, 실제 발자국 중심, type, 위 direction, owner(raw+34), frame(DWORD+36)이다. CD는 owner+32/frame BYTE+34다. current mode/center를 같은 getter로 채워야 한다. 현재 GunTarget adapter가 이 문맥을 만들어 주지는 않는다.
- **firingInterval**: 실제 TYPE+4c float다. 현재 `RiftTypeRecord`에는 이 필드가 없다. 동일 실제 `.type` 속성/초기값 근거를 별도 자료로 공급해야 하며 synthetic2.25 같은 검사 입력을 실제 메타데이터로 사용하지 않는다.
- **GunTargetType**: range+44/theme+98/L~O run 첫번호(+15c)/heightExtension(+1e4, CD+1c4)가 필요하다. 실제 속성 파서와 `Core.frames`의 현재 Run을 사용해야 하며 빈 배열을 정상 타입 자료로 세지 않는다.
- **refreshRange**: 004d5000/CD004433e0은 단순 재그리기 함수가 아니다. 004d4e10/CD00443200을 통해 현재 game now를 압축하여 raw WORD+12의 bit1..를 쓰고 004214a0의 현재 공간 통지를 수행한다. 실제 helper/body 연결 전에는 예외를 유지한다. `display.Update`나 noop로 대체할 수 없다.
- **소리**: 캐논 회전은 실제 난수 파일 `sunCannonRaiseLower*.wav`를, 실제71 Fire는 `sasprung.wav/safired.wav/sarachet.wav/sunDiscThrowerWhoosh.wav`를 요청한다. 기존 실제 sound 소비자가 없으면 해당 도달에서 거부한다.
- **표적 코너/피해/impact**: 교차 레인 배리어 코너 선택, 실제 표적의 피해 가상 분배, 공통 효과와 소리·Dude 통지 하위를 각각 기존 모듈에 연결한다. 이미 검증한 모듈의 일부 callback을 noop로 바꾸어 전투 완료를 표시하지 않는다.
- **진단 훅**: checking=false인 실제 입력에서는 `validateIslandOwners/checkForms`가 호출되지 않는다. checking=true를 지원한다고 표시하려면 현재 소유자/체인 진단도 연결해야 한다. `showRanges=false`일 때만 GunTarget의 진단 그림 훅이 불필요하다.

실제 Archer 타입은 directions0/frameCount35다. 과거 합성 회전20/40 입력을 현재 TYPE 기본값으로 주입하지 않는다. 그 입력은 당시 독립 Animation 관찰 범위로 남는다. 현재 생성/부착을 연결한 뒤 실제 타이머·표적/감시·애니메이션·투사체 소비자까지 닫는 일은 후속 범위다.
