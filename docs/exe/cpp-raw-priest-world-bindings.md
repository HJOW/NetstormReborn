# 사제·보호막의 단일 raw 월드 연결

작업 PC와 마지막 정밀 디컴파일 PC는 **VM-W11-CODEX**다. 2026-10-13의 현재 `extracted/refined/`와 기존 복원 모듈을 읽고 `priestworldevents`를 패치6/CD4개 함수 읽기 전용으로 내보냈다. 마지막 디컴파일 수행 PC: **VM-W11-CODEX, 2026-10-13**. 원본/클론 GUI 실행, 보호 원본 변경, 커밋/푸시는 하지 않았다.

`client/RawPriestWorldBindings`는 기존 복원 몸체를 같은 풀·해시·spot·표면 SID 지도·타입/프레임·생성자·소유자·프레임 지정·공통 postPop·보상·난수·ProcessHost에 연결한다. 전체 게임 플레이의 완료를 의미하지 않는다. CMake 등록/최종 빌드/아래 통합 검사 결과는 root 작업 단위의 최종 근거로 기록한다.

## 공유와 생성 순서

`RawPriestWorldCore`의 참조 자료와 `RawPriestWorldState`는 바인딩보다 오래 살아야 한다. `surfaces`는 원본과 같이 **같은 hash의 `Entries(0)` 자체**여야 하며 생성자가 주소 identity를 확인한다. 내용이 같아도 지도 사본은 거부한다. `priests.priests.entries`는 월드 초기화 시 실제 확보 용량을 설정한다. 살아 있는 사제 목록과 `ownerState`를 MissionOutcome/사제 finder에 그대로 전달한다. 별도 복사 사제 목록을 만들지 않는다.

1. 공통 raw 월드와 `RawPriestWorldBindings`를 생성한다. 바인딩 생성자가 실제 `RawPriestPostPopTail`/`RawForcefieldPostPop`을 공통 postPop에 등록한다.
2. 공유 Form 분배기에 `bindings.DestroyHooks(기존 자산 삭제 훅)`을 제공한다.
3. 동일 Kernel의 ProcessHost에 `bindings.RegularHandler(기존 다른 자산 사건 분배)`를 제공하고 Form 분배기에 호스트를 등록한다.
4. `BindProcesses(host)`로 실제 Regular/Form/Kernel 경로를 연결한다. 이 이후에 로딩 Pop이 가능하다.
5. `bindings.Spawn()`을 실제 `RawDamageableRelease`에 연결하고 그 Release를 실제 `RawDamageablePreDestroy`에 연결한다. `BindDamageable`로 사제의 Carrier→Damageable 삭제 흐름을 연결한다.
6. 실제 배치 파이프라인의 `RawPriestPlacement`를 `BindPlacement`로 연결한다. 전투/해방을 허용하기 전에 `ValidateReady()`를 실행한다.

다른 풀의 모듈과 중복 연결은 쓰기 전에 예외로 거부한다. ProcessHost/Damageable/Placement가 없으면 실행 경계가 이를 명시 진단한다. 외부 Kernel은 바인딩을 해제하기 전에 비워야 한다. 바인딩 소멸자는 공통 postPop의 자신이 등록한 두 포인터를 해제한다.

## 실제로 조합한 몸체

- 사제 일반 Pop: `RawPriestPostPop` 목록/회복 0x25a 예약→`RawCarrierPostPop`/Damageable 지면 claim→실제 공통 장부→`RawPriestState` 현재 HP/spot/extra→실제 낙하/보호막 확보 또는 삭제.
- 보호막 확보: `RawPriestForcefield`의 실제 일반 finder→factory(flags 2)→현재 소유자→실제 Pop→두 Regular 예약→소리/로컬 안내 경계.
- 낙하: 실제 `RawPriestFall` 보호막/SharedRegular 0x25b/Unpop(0)/Repop(0x800)/소리→현재 방향 J 진행 또는 J 시작→실제 표면 SID 확인→E 도착. J/E 시작 프레임은 `RiftTypeFrames::Run`에서 읽는다.
- HP 변경/회복: `RawPriestState::SetHitPoints`와 `RawPriestRegen`의 실제 절반 HP 경계·원본 정수 1/6 회복·최대 HP·순서/난수·추적. 다른 Regular 사건과 같은 번호를 타입/vtable로 구별한다.
- 사제 소유자: `RawPriestOwner`의 로딩/전투·원래 소유자 low 7비트·로컬 표식 재정의. 다른 타입은 같은 공통 `SquidOwner::Set`을 사용한다.
- 사제 삭제: 실제 목록 압축/보호막 삭제→Carrier 현재 종속 finder→실제 Damageable preDestroy/해방→공통 깊이 감소. 실제 Form 분배가 회복/낙하/보호막 Regular를 함께 지운다.
- 사제 해방: `RawPriestSpawn`의 실제 132개 나선 후보/WORD 복사/소유자/Pop→Carrier 검사. 후보 허용은 `RawPriestPlacement`의 실제 검사다.

## 명시적으로 남은 외부 몸체

공간/가상 삭제/claimGround는 root의 동일 월드 몸체를 공급해야 한다. 소리 파일 로딩/프로세스·로컬 안내·소유자 무상태 통지·다리 무상태 통지·생산 종속 후처리는 효과 경계로 받는다. 필수 계산 경계에 빈 lambda나 고정 허용값을 넣지 않는다.

회복의 `PriestRegenHooks`는 fullHP에서도 patchAudit를 호출한다. **00491680의 실제 ScrambledSpStore 회전**을 연결하고 현재 Offset을 `state.regen.patch.observedSequence`에 동기화해야 한다. SP 초기화는 원본의 expected sequence도 같은 Offset으로 시작한다. 독립 SP 회전 복원은 별도 작업 단위의 근거를 따른다. 이 통합 검사는 실제 Rotate와 두 sequence를 공유하고 SP가 유지되는지 확인한다.

실제 SP 초기화 후 추적 gate는 첫 실제 배열 포인터에 해당하는 nonzero 호스트 토큰, sentinel은 항등순열 첫 signed WORD의0으로 동기화한다. gate의 기본0을 실제 월드에 남기면 원본보다 자주 changedSample을 쓰므로 초기화와 같은 경계에서 설정해야 한다. 콘솔 통합 검사의 gate0은 명시 합성 전역 입력이다.

`shape(00488fe0/CD 0047d020)`는 형상 번호가 아니라 **이동 프로세스 타입 선택**이다. `RawMovementProcessTypes`와 현재 타입 genus로 원본 분기를 연결했다. `occupied(004b1a90/CD 004eb310)`는 이미 복원된 `RawContainedLookup::Find`에 연결하여 현재 종속에서 선택된 프로세스가 존재하는지를 검사한다. 이 두 callback은 바인딩 내부의 실제 몸체로 교체한다. 패치는 기본 worm 프로세스40을 반환하며 CD의 미지원 타입은 원본 assert 경로를 예외로 진단한다. balloon genus 분기는 나머지 특수 타입보다 앞이지만 두 walker 특수 타입보다 뒤다.

현재 보호 PE에서 확인한 mutable 타입/프로세스 전역은 다음과 같다. 상태의 값을 바꾸면 다음 선택에 그대로 반영한다. 두 판본의 자산 번호는 같은 로딩 배열을 따른다.

| 선택 | 자산 번호 | 프로세스 번호 | 패치 타입/프로세스 전역 | CD 타입/프로세스 전역 |
| --- | ---: | ---: | --- | --- |
| sunWalker | 133 | 40 | 0054126c / 00541110 | 0051cb58 / 0051ca00 |
| 추가 빈 타입 | 187 | 40 | 00541344 / 00541110 | 해당 분기 없음 |
| windWalker | 137 | 38 | 0054127c / 00541108 | 0051cb68 / 0051c9f8 |
| rainWalker | 156 | 38 | 005412c8 / 00541108 | 0051cbb4 / 0051c9f8 |
| balloon genus | 현재 타입 | 42 | 00541118 | 0051ca08 |
| teleportEffect | 134 | 23 | 00541270 / 005410cc | 0051cb5c / 0051c9bc |
| bulf | 135 | 41 | 00541274 / 00541114 | 0051cb60 / 0051ca04 |
| dude | 70 | 24 | 00541170 / 005410d0 | 0051ca5c / 0051c9c0 |
| 사제 | 158 | 54 | 005412d0 / 00541148 | 0051cbbc / 0051ca38 |
| manabolt | 105 | 39 | 005411fc / 0054110c | 0051cae8 / 0051c9fc |

`damageable(0044c060/CD 00462480)`의 type+0xd4는 **숫자 damageEffect**다. `RiftTypeRecord`/숫자 속성 읽기를 추가하고 바인딩에서 free 또는 damageEffect 0이면 원본처럼 후속 burning 조회/효과 없이 반환한다. 실제 보호 `netstorm.tarc`의 priest/forceField/residence 타입은 해당 속성이 없고 초기0을 유지한다. sunCannon/altar는1이다. nonzero의 전체 burning 몸체는 명시 callback으로 남는다. 기존 fixture는 전체 Parse의 +0xd4 결과를 저장하지 않아 새 관찰을 재사용했다고 주장하지 않는다.

중립 사제의 `neutralAction(00491db0/CD 0040d030)`은 같은 소유자 지형 객체 찾기/부족한 HP·일시 속도 증가 이동→±20의 전역 float 난수 후보→지역 일치/실제 이동 프로세스 생성이다. 빈 lambda로 바꿀 수 없다. 패치의 일시 속도 복원은 이전 speed를 ftol한 정수를 float로 쓰므로 원래 소수 float를 그대로 복원하는 것과 다르다. 실제 원본 호출/정밀도를 후속에서 대조해야 한다. `measurement(0040ee90)`은 SP 표시가 없으면 실제 PE float0, 있으면 현재 로컬 SP를 읽는다. 아직 필수 외부 callback이며 현재 일반 경로의 remaining 0일 때는 읽지 않는다. 중립의 이동/운반/의식 및 nonzero damageEffect 건물의 burning까지 완료하려면 이 후속 몸체를 연결/복원해야 한다.

`SetLoadingDepth`는 사제 owner/shield/Carrier와 회복의 로딩 차단을 함께 갱신한다. `SetAuthority`는 HP/낙하/Carrier/contained finder를, `SetLocalPlayer`는 원래 소유자의 로컬 표식과 보호막 안내 조건을 동기화한다. 다른 월드의 공통 권한/로딩/SP/소유자 전역도 root에서 같은 변경 경계로 갱신한다.

## 검사 범위

`RawPriestWorldBindingsTests.cpp`의 새7개는 원본 PE/Python/GUI 없이 두 판본의 실제 풀/factory/소유자/공간/display/frame/postPop/보상/Form/Kernel과 실제 Carrier/Damageable 해방을 조합한다. fullHP 사제 로딩, 재Pop 회복 중복 방지, 실제 SP회전 Regular 틱, 이동 불가 보호막의 finder/예약, 낙하 SharedRegular/도착 E, 사제/보호막/전체 예약 삭제, 실제 해방 생성/WORD 원래 소유자, 절반 HP 회복의 공간 전환/보호막 해제, damageEffect 0 자연반환/숫자 속성, 다른 풀/중복 연결/미복원 사건 거부를 검사한다.

처음 집중 실행은7개 중5개 실패/2개 통과였으며 `extracted/priestworldbindings-focused-before-surface-alias.log`를 보존했다. 사제 shadow 플래그에 대한 합성 SHP 물리 프레임을 main 개수만 제공한 입력 오류를 바로잡았다. 동시에 독립 검토에서 발견한 별도 surface 지도 사본을 제거하고 실제 base 자산의 Pop으로 hash0/spot을 같이 등록했다. 이후 같은 hash0의 제거/복구 변이를 낙하가 다음 사건에서 읽는지와 동일 내용 사본의 생성 거부를 검사한다. 수정 뒤 빌드/집중 실행 결과는 아래 최종 기록으로 갱신한다.

이동 선택은 독립 `decomp_priestworldevents_oracle.py`가 보호 PE의 실제 body/getter를 실행한 **2,872개** 입력(패치1,504/CD1,368)을 재생한다. 전체 유효 타입·balloon genus·프로세스 전역 변경·사제/windWalker 전역 충돌을 포함한다. CD 진단 보고만 명시 대체하며 결과 선택은 실제 원본이 계산한다. 재생성은 새로운 관찰 수에 중복 합산하지 않는다. `recovery-priestworldevents-evidence.json`의 SHA/입력/실제 실행 목록 감사로 확인한다.

기존 fixture로 복원한 종속 조회/SP/HP/낙하/다른 모듈의 재사용에는 새 관찰 수를 더하지 않는다. 신규 관찰은 위 이동 선택2,872개뿐이며 배치 forcePlacement 분기는 명시 입력으로 사용한다. 사제 중립 하위 몸체와 실제 소리 장치는 이 콘솔 검사의 범위 밖이다. 캠페인 1-1 로딩/건설/전투/운반/의식/완주 및 전체 원본 동일성은 root의 실제 월드 연결과 후속 실행 대조로 검증해야 한다.

## 최종 결과

- 공유 Release 증분 빌드 Exit0·경고/오류0을 root가 확인한 뒤 `RawPriestWorldBindings` 집중7개 **실패0**을 확인했다. 로그 `extracted/priestworldbindings-focused-after-surface-alias.log`. 수정 전5실패 입력 로그를 보존한다.
- 실제 두 PE 이동 선택 **2,872개**의 C++ 대조가 위 집중 검사에 포함된다. 최종 재생성/감사는 `extracted/priestworldbindings-movement-{oracle,verify}-surface-alias.log`이며 `python tools/decomp_priestworldevents_oracle.py --verify`가 현재 구현/정밀 body/보호 PE/fixture SHA와 입력 범위를 감사한다.
- 본 연결부/검사/도구/목록/두 문서와 수정한 타입 정의의 UTF-8을 확인하고 새 C++ 파일의 의도하지 않은 CJK 문자/변경 공백 검사를 통과했다. 전체 CTest와 실제 캠페인 월드/GUI 연결의 결과는 root의 후속 작업 단위 기록을 따른다.
- 읽기 전용 Fort 후속 분석은 [원본 Fort 월드 로딩 분석](cpp-raw-fort-world-loading-analysis.md)을 따른다. 기본 소유자/미션 모드, Terr→Chaff 생성 순서, 사제 프레임0, Pop9 뒤 실제 contained 재귀를 연결해야 한다.
