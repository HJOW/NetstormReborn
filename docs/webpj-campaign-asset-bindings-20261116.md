# 캠페인 자산 연결 이식 (WB04-E561)

4-D 구획 C의 `client/RawCampaignAssetBindings`를 `webpj/www/core/raw-campaign-asset-bindings.js`로
옮겼다. 기준선 `225553df`와 HEAD의 cpppj 소스는 같다(기준선 뒤 변경 없음, 2026-11-15 확인).

## 옮긴 범위

- `RawCampaignAssetBindings(world, damageable, assets, random, islands, playerRecords, playerState, globals, effects)`:
  같은 풀의 Damageable/섬 목록, 188칸 TYPE·프레임·셰이프·원소 표, 9×0xb4 Player 기록이 아니면 어떤
  공유 상태도 쓰기 전에 거부한다. 같은 World의 권한 소유자(`RawAuthorityState`)를 자산·Gun·Regular
  전역의 `sharedAuthority`에 연결하고(이미 다른 소유자를 빌렸으면 거부), 자산 상태의 현재 Bridge TYPE
  getter를 같은 World의 `BridgeState`에 연결한다(C++의 주소 비교는 BridgeState별로 하나뿐인 getter의
  함수 동일성으로 옮겼다). 그 뒤 실제 모듈을 조립하고 현재 게임 시각·Fort 모드를 준비한다.
- 조립하는 원본 모듈: `RawGeyserAiRegistration`, `RawContainedLookup`, `RawFrameAdvance`,
  `RawGunProcess`, `RawAssetPostPop`(같은 Player 9기록과 현재 battleOwner getter), `RawFactoryPostPop`,
  `RawSquidLevel`, `RawAssetRegular`.
- 연결이 직접 만드는 훅
  - Gun: 표적 감시 제거/생성(`dependencies.remove/create`), 현재 Form 재전송(`refreshForm`),
    부모 Form 체인의 발사(35)/회전(36) Animation Form 조회와 중지, 부모 차단 비트(+40의 0x20),
    지연 난수(16비트 × 2^-16 × |high − low| + low).
  - 자산 PostPop: Regular 부착, GunProcess 생성(현재 `gameAux.gunProcessType`), 발사기 Animation 제거
    명령 큐(`archerNoticeCommand`), 지면 조회, 소유자 지정(가상 owner 효과가 없으면 `Core.owner`,
    신전은 효과 필수), 표면 통지, 간헐천 AI 등록, 한 점 finder, Unpop/Repop, TYPE 프레임 구간 DWORD
    (+0x130/+0x134는 현재 프레임 표의 'A'/'B' 첫 프레임), 섬 번호/존재/신전 등록, 삭제.
  - Factory: Regular 확보/부착, 삭제, 표시 갱신, 프레임 지정(`makeFactoryPostPopHooks`).
  - Regular: 표면 통지, 지면 조회, 소유자, 표시 갱신, 섬 번호(unsigned), 부착 Form 조회.
- 공개: `activatePostPop`/`dispose`(활성 등록만 해제), `prepareFrame`, `setLoadingDepth`,
  `bindPatchGeyserAi`, `setState`(Level 저장 → 기본 표시 갱신), `adjustFactoryFrame`, `handler(fallback)`,
  `transmission(fallback)`(Gun 프로세스만 지역 64BYTE 버퍼와 `sendGunPacket`, 나머지는 fallback),
  `postPop`·`regular`·`gun`.
- 고르지 않은 선택 효과는 성공/무동작으로 바꾸지 않고 그 효과가 호출될 때 명시 오류다(`RejectMissing`).

## 함께 추가한 것 (3.6절 ③ 소비자 단위 몫)

- `game-aux-state.js`: `gunProcessType`(005410f8, 34)·`fireAnimationProcessType`(005410fc, 35)·
  `rotateAnimationProcessType`(00541100, 36). 보호 PE `.data` 초기값과 프로세스 TYPE 이름표로 대조한다
  (`tests/game-aux-state.test.mjs`, 필드 53칸).
- `squid-process.js`의 `SquidProcessState.fireAnimationType`·`rotateAnimationType`(독립 35/36, 빌린
  GameAuxState에 그 칸이 있으면 현재 값). cpppj `02b00b12` 판정에서 미뤘던 두 칸이다.
- `raw-gun-process.js`의 `GunProcessState`(cpppj 구조체 대응: 독립 입력과 `sharedAuthority`·
  `shared*Type`·`allianceRead`). `globals()`가 `RawGunProcess`가 받는 현재 값 reader 묶음을 만든다.

## 옮기지 않은 것

- cpppj의 Native 장기 Storage 경로(`UsesNativeTransmissionDevices`: 장기 Factory 후처리 공유, 공통
  전송 버퍼, `SerializeProcessForm`/`BroadcastProcessForm`). 웹 `RawSurfaceStorage`에는 대응 개념이
  없어 항상 독립(typed) 경로이며 Client 장기 소유자가 생기는 4-D 구획 E에서 같은 자리에 추가한다.
- `islandTemple` 훅(기존 신전 assert 조회): cpppj에서도 CD(10.72) 후처리만 부른다. 10.78 전용인
  웹에서는 연결하지 않는다.

## 시험

cpppj에는 이 단위만의 원본 관측이나 전용 시험이 없고 `tests/CampaignInstallSupport.h`의
`InstallScene`이 이 연결로 저장 건물 후처리·Gun/Regular를 조립한다. 웹은 같은 방식으로 보호 10.78
TYPE·프레임 코드·SHP 물리 프레임 표로 실제 `RawSurfaceWorld`를 만들어 연결을 설치하는 장면
(`webpj/tests/campaign-asset-support.mjs`)을 두고 소스 계약을 시험한다.

`webpj/tests/raw-campaign-asset-bindings.test.mjs` 17건:

- 구성 거부(다른 풀·표 크기·Player 기록·권한 소유자·Bridge TYPE 소유자·현재 owner 9)와 구성 연결,
  `prepareFrame`/`setLoadingDepth`, 활성/해제.
- 썬 캐논 최초 Pop → 현재 005410f8 TYPE의 실제 `GunProcess` 부착과 31BYTE 전송, `transmission` 분배와
  버퍼 경계, 표적 감시·Animation Form 중지·체인 손상·Host 없는 Form, 차단 비트와 지연 난수,
  누락 총 효과 거부.
- 거주 구역(지면 owner·목록·예약 0/1/3·Damageable 사건), 간헐천(목록·AI 등록·비용 WORD·프레임 진행·
  처리기 fallback), 신전(소리 토큰·프레임 구간·섬 등록·같은 지역 신전 삭제 0x100000·Regular 사건),
  섬 번호 getter 경계(절삭·지도 밖·다리 genus·genus assert·TYPE 188·int32 밖), 발사기 명령 큐,
  Factory/아웃포스트(예약·Level·프레임 보정·지역 소유권·중복 경고), 누락 Factory·자산 효과 거부.

변형 확인: 자동 생성 135종. 1차 생존 24종 가운데 17종은 시험 보강과 죽은 코드 정리로 해소했고,
남은 7종은 동등 변형이다.

| 동등 변형 | 사유 |
| --- | --- |
| 난수 한계 65536 → 65537 | `next(limit)`는 상위 16비트(0~65535)의 나머지라 두 한계의 결과가 같다 |
| 좌표 하한 −2147483648 → −2147483649 | float32 좌표에는 두 값 사이의 수가 없다 |
| `#setOwner`·`typeDword`의 `type >= 표 길이` → `>` | 분배된 거주 구역/신전의 TYPE BYTE는 표 안이라 188에 닿지 않는다 |
| Animation Form 체인 방문 계수 시작 0 → 1, 증가 1 → 2, 한계 `>=` → `>` | 손상 체인 거부 시점만 한 걸음 달라지고 결과(거부)는 같다 |

## 시험 장면에서 확인한 사실 (다음 단위 참고)

- 건물 발자국이 최대 8칸이라 겹치는 자리의 두 번째 Pop은 공간 점유로 void에 머문다(후처리 없음).
  객체는 15칸 이상 띄운다. 넓은 지면(51×51)을 한 번에 로드하면 장면 하나에 8초가 걸리므로 객체
  자리에만 지면 조각을 깐다.
- Pop 도중 예외가 나면 공통 후처리 깊이가 남아 그 장면의 다음 Pop이 중첩으로 처리된다. 실패를 보는
  사례는 장면을 새로 만든다.
- 감시 Form 쌍을 만들려면 월드 runtime에 `formTransmit`이 있어야 한다.

## 남은 경계

- 앱 수명에는 연결하지 않았다(PORT_MAP `port`). 실제 외부 효과(총 조회/표적/발사, 신전 가상 owner,
  지역 번호, 소리, 명령 큐, Gun 패킷 송신)는 Session 조립(4-D 구획 D)에서 `RawCampaignGunQueries`·
  `RawCampaignArcherBindings`·`RawCampaignOwnership` 등과 함께 연결한다.
- cpppj `InstallScene` 전체(사제·피해·전투 연결 포함)의 웹 대응은 해당 바인딩 단위가 끝나는 대로
  `campaign-asset-support.mjs`를 넓혀 만든다.
