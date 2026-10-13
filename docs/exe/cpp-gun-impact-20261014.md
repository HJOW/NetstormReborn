# 실제 충돌 플레어 생성과 프레임 수명 (2026-10-14)

현재 분석 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. `gunimpact` 정밀 readOnly 내보내기는2026-10-13에 두 판본9/9 종료0으로 완료됐다. 보호 원본10.78/CD10.72는 읽기만 했으며 게임·GUI·OS·DirectDraw를 실행하지 않았다. `--read-game`은 자산 바이트를 stdout으로 읽는 CLI다.

## 실제 원본 몸체

| 항목 | 10.78 | CD10.72 |
| --- | --- | --- |
| 시각 효과 producer | `0045a750` | `0047c8a0` |
| anim Handle | `0045a6e0` | `0047c810` |
| anim ctor/vtable | `0045ae00`/`00509758` | `0047c880`/`00503b30` |
| 지면 충돌 사슬 | `00470ac0` | `00455b40` |
| 좌표 제한 | `0041d0d0` | `0043f950` |
| 실제 Kernel count | `00471ab0` | `004ed650` |
| 실제 Regular ctor | `00496e80` | `0048efd0` |
| 공유 난수100 | `004558c0` | `0048cbd0` |

실제 자산은 `anim`160이며 표시 typename은 **Flare(플레어)**다. 이미 복원된 `SquidFactory` 생성자 recipe를 소비한다. TYPE 번호를 base Squid로 대체하지 않는다. producer는 억제 double이0이고 추가 억제 DWORD가0이며 실제 Kernel 등록 수가 **30,001 미만/CD3,001 미만**일 때만 진행한다. 원본 count는 slot0을 포함한40,000/CD4,000칸을 실제 순회하며 유효 실행 입력은 예약 slot0을0으로 둔다. C++의 대응 Kernel 용량은39,999/CD3,999다.

생성 순서는 Clamp→`SquidFactory.Create(160,2)`→해당 글자 구간 첫 frame 직접 저장→가상 Pop(x,y,0)→new40/Regular(event,parent,interval)이다. Regular의 원본 flags는0x50이며 부착 뒤 현재 gameTime·event·payload·count0을 기록한다. 최초 frame 저장은 SetFrame 호출이 아니므로 표시/Unpop을 앞당기지 않는다. Native/C++ 대조는 의미 있는 Form/부모/시간/event/payload/count를 검사하며 C++ 객체의 물리 메모리가32비트 원본과 같다고 주장하지 않는다.

anim Handle은 event의 첫 frame/개수 표를 읽고 signed count가0 이상이고 개수 미만일 때 실제 `SquidFrame.Set(first+count,0)`을 호출하고 payload를 반환한다. 범위 밖이면 가상 Destroy(0)을 호출하고-1.0f를 반환한다. 부모를 삭제할 때 종속 Regular Form도 같은 Host/Kernel에서 제거해야 한다. -1 반환을 임의의 timer 종료로 바꾸지 않는다.

## 독립 원본 관찰

`decomp_gunimpact_oracle.py`는 **2,274개 고유 정상 반환**, 각 판본1,137개를 관찰한다. 판본마다 Handle1,008·producer72·Clamp27·anim ctor1·Kernel count5·ground24개다. assertion/OS 호출은0이며 생성 접두 정지로 끝낸 행은 없다. actual Handle/SetFrame·현재 해시 단계·타입 query·Kernel 전체 배열 순회·Regular ctor·공유 난수 몸체를 직접 실행한다.

Factory/heap/Pop/부착/Dude/가상 삭제·표시 요청은 명시 관찰 경계다. Native attach는 Form71/부모50을 공급한다. C++ fixture의 Regular 의미 상태는 별도 실제 SID/Host에서 `AddRegular`를 실행해 대조한다. 실제 공용 월드의 공간·생성·삭제 완료는 아래 소비자 검사와 구분한다. 현재 독립 구간/SHP는 빈 구간과 존재 구간을 포함하는 합성 입력이다.

초안에는 예약 slot0까지 채운 마지막 capacity 입력1개/판본이 있어2,276행이었다. 실제 Kernel 계약인 slot0=0과 마지막 유효 등록 번호39,999/CD3,999로 교정해 현재2,274개에 포함하지 않는다. 이를 신규 관찰로 합산하지 않는다.

최초2,274행 대조에서 초안 Clamp 하한0.5와 NaN 비교의 오류를 발견했다. 실제 PE의 하한은 `005012d0/CD00501764`의 **float1**이다. 패치는 unordered 비교에서 NaN을 보존하고 CD는 최종 상한255를 선택한다. 생산 코드·검사·oracle·fixture·전체보고서를 `recovery-history/gunimpact-initial`에 실제 바이트로 보존하고 이 명시 교정만 적용했다. `analyze_gunimpact_correction.py`는 역변환과 당시 전체 SHA·같은2,274fixture·실제 상수를 감사한다. 동일 입력 재방문이며 신규 고유 입력은0이다.

첫 공유 Release는 경고/오류0으로 빌드됐으나4검사 중3검사에서4checks가 실패했다. 두 전수 검사는 fixture의 suppressed 열10에 저장된 실제 문자열 **True/False**를 `stoul`로 읽어 예외가 났다. 이 입력 해석만 숫자1/0으로 교정하며 기존 suppressedTime 열9의 double raw64는 이미`stoull`로 읽는다. 실제 소비자는50.5/50.5의808/555픽셀이640×480 viewport 밖이었다. 원본1024×768 viewport를 공급하고 최종 장부/비용/깊이/dirty 단언을 각각 분리했다. 생산 코드·원본tool·2,274fixture는 불변이다. **교정 후 공유 Release 경고/오류0, GunImpact4검사/실패0**으로 양판본2,274전수와 실제 소비자가 통과했다.

첫 C++ 실패 직전의 전체source/header/test/oracle/fixture/report·당시 Clamp 도구/보고서와 실패로그9개를 `recovery-history/gunimpact-before-cpp-correction`에 보존했다. 현재 교정 도구의 schema2는 두 단계의 전체 역변환과 당시/현재 모든 SHA·원본 관찰 불변을 연결한다. 추가native/고유입력은0이다. 최초 실패로그는 `extracted/campaign-runtime10-GunImpact_initial-tests.log`다.

## 실제88프레임 소비자

두 판본 보호 `d/anim.type`는 A35/B23/C8/D22 총88클러스터다. 첫 번호는 A0/B35/C58/D66이며 원본 번호B09/D09의 공백도 실제 물리 목록 순서로 처리한다. `GunImpact_ActualAssetsFactoryPopRegularAndDeletion`은 같은 `GameAssets`/실제 SHP에서 구간을 읽고 D의22프레임을 처리하도록 작성했다.

이 검사는 같은 풀의 Factory→실제 SquidPop/표시/공통 postPop 장부→Host.AddRegular→현재 Kernel 틱→실제 SetFrame/Unpop/Repop→RawSquidDestroy/실제 SquidDestroyLifecycle/종속 Form 삭제를 연결한다. 실제 전역 타입 개수 증가/감소, Kernel 제거, 해시 해제, 깊이0, 같은 원본 프레임 수를 확인했다. 화면 영역 변경 요청과 삭제 보상/알림은 관찰 경계이며 창·오디오 출력이나 전체 UI 완료로 세지 않는다. 교정 후 같은 두 판본의 장부/비용/깊이/dirty 개별 단언이 모두 통과했다. 단위10 **a349e63**의 고유집중35·전체1023검사/실패0이며 실제캠페인 월드 조립/완주 완료는 아니다.

## 후속

`MakeGunImpactRuntimeHooks`는 실제 Factory/Kernel/Host를 소비하고 `MakeMissileGunImpactHooks`는 Missile의 barrier/표적 충돌, `MakeAssetGunImpactHooks`는 공통 Damageable ground 사건을 연결한다. 누락된 생성/삭제/Dude 반응은 예외다. ground의 Clamp→Dude→공유 난수100 소비→D 효과0.02 생성 순서도 원본 몸체로 관찰한다.

주변 Dude 몸체는 아직 별도 미완료다. 패치`00451470`은 finder 결과의 genus0x400에 대해`00450db0`을 호출한다. CD`004e8e70`은 별도 큰 몸체에서0x5107 Regular 생성/재예약·상태·경로 반응을 직접 처리하므로 두 판본을 동일 빈 알림으로 대체하지 않는다. 실제 Missile 월드 이관은 이 경계를 연결해야 한다. flight fieryTail11·피해 사망 corpse/폭발·소리 출력도 별도 후속이다.

CMake 등록 대상은 `RawGunImpact.cpp`, `GunImpactTests.cpp`, `NETSTORM_GUNIMPACT_FIXTURE`이며 현재4개 검사다. 원본/lineage 감사 명령은 `python -X utf8 tools/decomp_gunimpact_oracle.py --verify`, `python -X utf8 tools/analyze_gunimpact_correction.py --verify`다. 이전08·썬 캐논/썬 디스크 투사체의 완료 보고서는 수정하지 않는다. 캠페인1-1 전투/완주 동일성은 계속 미완료다.
