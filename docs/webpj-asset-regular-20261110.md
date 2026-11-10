# webpj 자산 정기 처리 분배 이식(RawAssetRegular) — WB04-E255

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-C의 일곱째 항목이다. cpppj 기준선 `1f00a25a`의 `o/RawAssetRegular`(초기 요새 자산의 가상 표 +0x5c 재정의: 거주 구역·간헐천·신전·Factory의 정기 처리와 공통 Damageable 사건)와, 기준선 뒤 cpppj 가 이 파일에서 분리한 공유 본문 `o/RawResidenceRegular`·`o/RawFactoryRegular`(새 파일)를 함께 옮겼다. 게임·브라우저 실행과 디컴파일은 하지 않았다(Node 단위 검사만).

## 1. 기준선 뒤 변경 판정 (3.6절)

`git diff --numstat 1f00a25a HEAD`는 `RawAssetRegular.cpp +70/−32`·`.h +16/−2`·`AssetRegularTests.cpp +22/−0`, RawAssetRegular 를 바꾼 커밋은 `11fbb988`(20261107-66, 신전 51h 수신과 상승 완료의 지형 저장)·`da2bf5b8`(-68, 신전 명령과 ViewMeta 저장)·`ed20738b`(20261108-233, 장기 간헐천 너겟과 Damageable Regular 접합)·`3c5f5092`(20261110-34, Factory 정기와 현재 업그레이드)·`7fa2b600`(-38, 거주 구역 정기와 Fort Dude 생성 부모)·`c102cdbf`(-88, 신전 정기 네 가상표와 상승 본문의 장기 소유자 연결)이고, 새 파일의 Fort Dude 비교는 `ae4448e1`(-40)이 정정했다. 기준선에 없던 새 파일 둘(`RawResidenceRegular`·`RawFactoryRegular`)과 그 시험(`ResidenceRegularTests`·`FactoryRegularCurrentTests`) 및 fixture(`factory-regular-current-x86.tsv` 656행)가 생겼다. 기존 fixture(`assetregular-x86.tsv`·`residence-coordinates-x86.tsv`)는 기준선 뒤 바뀐 적 없고, 10.78 typed 결과는 기준선과 같다(1,054행·864행 재확인).

- **② 재반영:** 모두 단독(싱글플레이) 결과를 바꾸지 않는 구조 분리이지만, 장기 owner(Client)가 같은 본문을 현재 전역으로 부르므로 웹에 같은 진입을 둔다.
  - 거주 구역: 사건 본문을 `runResidenceRegular`로 분리(현재 난수·지정기·신전 목록·설정·장식 TYPE을 소비 위치마다 조회, 최대 조사 수는 signed 비교 기록), Fort 전용 Dude 생성 부모 `runResidenceDudeSpawn`.
  - Factory: 사건 본문을 `runFactoryRegular`로 분리(현재 레벨 getter `virtualCount`를 첫 프레임·프레임 수 조회 앞에서 각각 한 번 읽음, 0 범위 난수는 시드를 한 번 소비한 뒤 오류).
  - 신전: 사건 본문을 `runVortexRegular`로 분리(독립 typed 호출과 Native 가 같은 코드). 공유 Fort 모드 getter(`sharedFortMode`)·상태 WORD 통지 훅(`hooks.state`)·억제 getter(`suppressLeaveRequest`)·Descend 훅(`hooks.descend`).
  - 간헐천: `runNativeGeyserRegular`(현재 가상 +88 표시 → 현재 비용/프레임 → 후행 Damageable), 공통 `damageRegular(sid, count, native)`(native 는 지면 피해 뒤에 HP 장치를 요구).
- 행 base 는 판정한 `c102cdbf`로 올렸다. 두 새 파일은 대응표에 행이 없어(재기준선 때 등록) 웹 모듈을 이 행의 web 칸에 함께 적었다. 호출자(client Session 의 자산 장기 owner)와 훅의 실제 몸체 연결은 4-D 몫이다.

## 2. 옮긴 것

`www/core/raw-asset-regular.js` — `AssetRegularState`·`RawAssetRegular`(`handles`·`handle`·`handler`·`damageableHandle`·`damageRegular`)·`runVortexRegular`·`runNativeGeyserRegular`.
`www/core/raw-residence-regular.js` — `runResidenceRegular`·`runResidenceDudeSpawn`. `www/core/raw-factory-regular.js` — `runFactoryRegular`.

- 10.78 가상 표 기록값 8개(거주 구역 0x50f540, 간헐천 0x50da88, 신전 0x5196f0/0x5197a8/0x519860/0x519918, Factory 0x509148/0x509210)로 몸체를 고른다. 기록값만 비교하고 원본 주소를 호출하지 않는다. 부모 TYPE 이 표 크기(188) 이상이면 외부 효과 전에 `RangeError`, 모르는 표는 `TypeError`이다.
- 선택된 사건이 요구하는 외부 효과는 도달한 위치에서만 요구하고 없으면 `Error`로 거부한다(앞선 raw 변경은 되돌리지 않음). 무효 반환으로 대신하지 않는다.
- 장치 경계: 프레임 지정(`SquidFrame.set`)·한 번 감싸기 진행(`RawFrameAdvance`)·게임 난수·`RawSquidFinder`는 이미 옮긴 웹 모듈을 쓰고, 표면 통지·소유자 지정·주민 Dude 생성·간헐천 표시·신전 Ascend 정리·이탈 알림·섬 번호·이탈 요청·Descend/지역·피해 지면·가상 HP·부착 프로세스 조회·건설 시작·Factory 업그레이드(그림·완료·플레어)는 훅이다.
- 상태: `AssetRegularState`는 cpppj `AssetRegularState` 대응이다. `checkingForms`는 값 또는 소비 위치마다 현재 DWORD 를 돌려주는 함수(C++ `RawCheckingFlag`의 현재 뷰)이고, 비영 판정이다. `sharedAuthority`(boss)·`sharedFortMode`(함수)는 장기 owner 의 현재 값을 빌린다.
- 거주 구역의 쓰기는 원문 `const_cast` 위치만 물리 슬롯(`pool.physical`)에 하고, 신전·간헐천의 WORD/프레임 쓰기는 할당 슬롯(`pool.allocated`)에 한다(원본 `AllocatedBytes`와 같음).

## 3. 검증 (`tests/asset-regular.test.mjs`, 33건)

| fixture (10.78 사용 행) | 대조 내용 |
| --- | --- |
| `assetregular-x86.tsv` 1,054행 (SHA-256 `manifest.json`) | 거주 구역 261·간헐천 511·신전 257·Damageable 단독 25행의 반환 float 비트·게임 난수 상태·최대 장식 조사 수·하위 효과 호출 순서·슬롯 50 전체·이웃 프레임 세 개. 같은 행을 독립 호출·공유 권한(반대 fallback)·공유 신전 목록으로, 거주 구역 261행과 신전 프레임 사건 99행을 따로, 간헐천 511행을 Native 분리 본문으로, 신전 257행을 Native 진입(공유 Fort 모드·상태 통지·억제 getter·Descend)으로 다시 재생 |
| `residence-coordinates-x86.tsv` 864행 | 별도 원본 관찰의 축 교차/경계 좌표(해시 1단계 체인을 다시 등록한 장면)의 결과·난수·최대 조사·호출·슬롯·이웃 프레임 |
| `factory-regular-current-x86.tsv` 656행 | Factory 정기의 현재 두 레벨 조회·프레임 조회·실제 난수·0범위 예외(오류 128행)·최종 슬롯 |

- cpppj 개별 시험(`AssetRegularTests` 10.78 분·`ResidenceRegularTests`·`FactoryRegularCurrentTests`)을 모두 옮겼다: Factory 업그레이드 분배/상위 플레어, 첫 Regular 가 현재 Kernel 에서 시각/payload 를 혼동하지 않고 호출되는지(실제 `SquidProcessHost`+`Kernel`), 미연결 효과/부모/다른 풀 거부, 거주 구역 Dude 생성 부모의 관문과 조건부 작업, 지연 선택. CD 10.72(originalCD) 행과 CD 전용 assert 는 범위 밖이다.
- 경계 계약(변형 시험에서 살아남은 입력): 가상 표 여덟 값의 이웃 값 거부와 신전 네 부류·Factory 두 부류의 분배, 부모 TYPE 188, 진행기가 다른 지정기에 연결된 경우와 표 크기, 신전 event2 의 하위 5비트(15/16/30/31/63/32), 편집 모드 event1 의 구간 길이(불균등 코드 표), 이탈 event3 의 섬 번호 BYTE 축소·상태 bit0·boss 임계 3/23, 상태 통지 훅 순서(2 → 4 → 0)와 WORD 직접 쓰기 배제, event4 의 checking 어느 비트·함수 값과 토큰 부호 없음, 건설 재시도의 extra bit0·부착 번호·0 길이 시작, Factory 레벨 상위 3비트·getter·0..15 범위·0 프레임 범위 난수 소비, 거주 구역의 설정 훅·원소 표 범위·signed 최대 조사 수, 조사 중심의 10..246 자름(양 끝)과 영역 ±1/위 3/아래 min(float(y+32),246), 중심 좌표 CRT 범위 오류(비유한·2^63), 장식 TYPE/프레임 범위 오류, 변경 뒤 조사 수, 첫 소유자 일치 신전만 읽기, Dude 수 signed 비교·Fort getter·source 0 진단 복귀.

최초 실패와 교정:

- 모듈은 처음부터 fixture 세 종(1,054·864·656행) 전부를 통과했다. 실패 2건은 검사 쪽 입력이었다 — 실제 `SquidProcessHost`는 Form 생성에 TYPE 1·2·3·6 의 생성자 주소가 필요한데 합성 TYPE 표에 없었다(`typeConstructorAddress`로 채움), 미연결 거부 문구 정규식이 실제 문구와 달랐다.
- 변형 157개(raw-asset-regular 80·raw-residence-regular 62·raw-factory-regular 15)에서 150개를 탐지했다. 처음 37개가 살아남아 위 경계 계약을 추가했다. 그 과정에서 (a) 변형 도구가 모듈 머리 주석에 같은 숫자(가상 표·지연 비트)가 있어 주석의 첫 일치를 바꾸는 가짜 생존을 만들었음을 확인하고 변형 대상 문자열에 코드 문맥을 넣었고, (b) 타입 범위 거부 변형은 아래 `SquidFrame`의 같은 낱말 오류 문구로 통과했음을 찾아 정규식을 `자산 Handle 타입 범위`로 좁혔다.
- 남은 7개는 동등 변형이다: event4 `fround(signedCount) <= 4`와 `signedCount <= 4`(정수→float 변환은 단조라 4 이하 판정이 같음), 생성자 `advance.pool`·`setter.pool` 검사(진행기 생성자가 이미 같은 불변을 강제하는 중복 방어), 최대 조사 수 갱신의 `<=`와 `<`(같으면 같은 값 대입), 거주 구역 예약값의 `Math.fround`(표본 ≤65,535×20 은 22비트라 항상 float 로 정확), 좌표 절삭의 하한 비교 상수(double 로 −2^63 과 −(2^63−1) 이 같은 수)와 `ToInt32`/`BigInt.asIntN` 구현 차이.

## 4. 경계와 다음 시작점

- 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0). 훅의 실제 몸체(표면 통지·소유자 지정·Dude 생성·간헐천 표시·신전 Ascend/알림/섬/이탈 요청/Descend·피해 지면·가상 HP·건설 시작·Factory 업그레이드)와 공유 권한·Fort 모드 getter·신전 목록·현재 설정은 4-D 에서 잇는다.
- 이 단위로 `RawAssetRegular`는 PORT_MAP `port`(base `c102cdbf`)다. 3-C의 다음(마지막) 항목은 `RawLocalGeyser`(271줄, +74)다.
