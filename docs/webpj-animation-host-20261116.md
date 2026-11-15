# 애니메이션 실제 Host 공장·완료 수명 연결 (WB04-E566)

`RawCampaignArcherBindings`가 요구하지만 기존 프레임 모듈에서 남겨 둔 실제 Host 연결을 먼저 옮겼다.
근거는 기준선 `225553df`의 `o/RawGunAnimation`, `RawArcherAnimation`, `RawCannonFire`,
`RawCrossbowAnimation`, `RawIceCannonFire`의 `.cpp/.h`와 `SquidProcess.h::AnimationProcessType`이다.
다섯 `.cpp`는 기존 행 기준 `02b00b12`부터 `225553df`까지 같으며 기준선 뒤 변경도 없다.
cpppj·보호 원본·공통 인계는 수정하지 않았다.

## 구현과 연결

다섯 모듈에 원문의 `Make*HostHooks`와 `Create*` 공장을 이식했다.
현재 `GunProcess`의 실제 부모 TYPE을 검사하고, 같은 Pool/Host에 flags `0x10`으로 부착한다.
`SquidProcessHost.animationProcessType(fire)`는 E561의 장기 상태에서 현재 발사/회전 TYPE을 읽는다.
독립 기본값은 35/36이고 `GameAuxState`를 빌렸으면 다음 부착에서 바뀐 값을 사용한다.

완료 훅은 매 호출마다 Form의 현재 프로세스를 다시 찾는다. Gun 통지를 먼저 수행하고 실제
파생 애니메이션을 Host의 공통 파괴 경로로 없앤다. 틀린 파생이나 누락된 프로세스는 명시 오류다.
입력 훅 묶음을 값으로 복사하며 Archer의 `common`도 별도 묶음으로 보존한다.

- Gun: 회전2·발사5·원위치8, 부모의 현재 썬 캐논 TYPE, 실제 부착 결과의 파생 검사.
- Archer: 회전2·발사5. 발사 각·출력 비트 초기화는 성공한 부착 뒤에만 실행한다.
- Cannon/Crossbow/Ice: 현재 TYPE의 발사 프로세스와 각 파생의 실제 완료 수명.
- `makeGunAnimationProcessHooks`·`makeArcherAnimationProcessHooks`: 실제 생성 함수와
  현재 Archer TYPE 우선 분배, 값으로 캡처한 다른 파생의 fallback.
- `makeArcherMissileHooks`: 기존 실제 `makeGunMissileHooks`를 같은 Host/Pool로 대여한다.
  Missile 본체나 새로운 발사 결과를 합성하지 않는다.

기존 프레임/직렬화 몸체는 바꾸지 않았다. 이 단위는 캠페인 설치나 실제 Session 완성을 뜻하지 않는다.

## 검증

`tests/animation-host.test.mjs` 10건은 실제 `SidPool`·`Kernel`·`SquidProcessHost`·
`RawSquidDestroy`·`GunProcess`를 사용한다. 좌표/공간/표적 판정은 cpppj 시험과 같은 명시 외부 장치다.

- cpppj `GunAnimation_ActualHostNotifyAndLifetime` PATCH 부분.
- cpppj `ArcherAnimation_ActualHostNotifyAndLifetime` PATCH 회전/발사·부착 뒤 초기화 부분.
- cpppj `CannonFire_ActualHostNotifyAndLifetime`와
  `IceCannonFire_ActualHostNotifyAndSelfDeletion`.
- Crossbow 실제 완료 수명의 소스 계약, 다섯 공장의 잘못된 Pool/Gun/부모 및 부착 실패,
  현재 프로세스 TYPE·부모 TYPE 재조회, 콜백 사본·실제 파생 검사, 실제 공장/fallback 분배.

최초 집중 검사는 9건 중 3건이 실패했다. 시험 장면이 Gun의 표적 중심을 Archer와 같은 방향으로
주었고 `FrameCodeTable`에 문자열을 넘긴 것이 원인이었다. cpppj 장면의 Gun 중심 `[50,60]`과
실제 `{side:65,variant:0,flags:0}` 코드 입력으로 교정했다. 프레임 처리기를 수정하지 않았다.
이후 실제 생성 어댑터 성공 경로 1건을 추가했다.

기존 원본 fixture 대조까지 포함한 집중 검사 25건은 전부 통과했다.
다섯 애니메이션 모듈은 집중 coverage의 줄/함수 각각 100%다(분기 89.29~95.24%).
새 Host 시험은 원문 조립 계약이고 새로운 x86 관측으로 세지 않는다. 아래 기존 fixture의
내용·`cpppjBasis`는 보존했으며 manifest의 웹 모듈 변경 이력만 갱신했다(총801항목 유지).
행 수는 머리글을 포함한 manifest의 비어 있지 않은 행 수다. CD 행은 기존 시험처럼 제외한다.

| fixture ID | 행 수 | SHA-256 |
| --- | ---: | --- |
| gunanimation | 2,561 | `7ed89da7895a89b1d65efcbdbef0bddbc5855d525ef51b30d17305b3c7145d6f` |
| cannonfire | 5,535 | `b1090cbf8edc90b391c170a323ba0797163b572262c1ba527b2cf16236ab2e3a` |
| vanderfire | 418 | `be3c685abe10d7deaa88d0a7d1436855b377d3eed439743d6c9c3aa862f20d81` |
| icecannonfire | 3,586 | `fc77d37af375595b785c557f8e5a36f09ff4a120b8749a1a163ff1a72343d9ce` |
| gunarcher | 1,691 | `7882627526d66a7b5084902407788f83170f5c3da27745dfec0a3aa0cb77ab2b` |
| crossbowanimation | 2,639 | `d7658e001e1f9e85af7c4ac140e8d34acb96500e16f4880ce32eb8dd10ab75be` |

HJOW-Athlon에서 Node v22.23.3·v26.13.0 전체 각각 **5,338건 통과, 실패·skip 0**.
변경 후 집중 재확인(새 시험10·대응표7·manifest3) 20건도 통과했다.
대응표 도구는 1,160행 오류 0이며 port833·partial33·todo138·제외156을 유지한다.
로그는 `extracted/wb-e566-node22.log`·`wb-e566-node26.log`·`wb-e566-focus.log`·
`wb-e566-metadata.log`에 있다(Git 제외). Node v24 전체는 이번에 반복하지 않았다.

4-D 진행 개수도 재집계했다. `fa8c2931f` 보관 시점의 실제 완료 체크는 13개인데 당시 머리글은
12개로 적혀 있었다. 현행 완료 7개와 보관 완료 13개, 현행 미완료 16개를 합쳐 **20/36**으로
교정했다. 과거 기록은 보존하고 현행 0.2절과 단계 머리글만 수정했다.

## 남은 경계와 다음 시작점

다음은 `client/RawCampaignArcherBindings` 본체다. 같은 실제 Gun 조회기·Missile·애니메이션을
조립하고 장기 Sun/Wind TYPE 소유자, 발사/회전 수신, 현재 헤더와 공유 전송 버퍼를 연결해야 한다.
그 뒤 Session의 D 구획과 Native 장기 Storage의 E 구획에 설치한다.
4-D의 실제 L2/미션 화면·교전·자연 완주는 여전히 미검증이다.
