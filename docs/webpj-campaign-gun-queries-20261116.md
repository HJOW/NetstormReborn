# 캠페인 Gun 현재 조회 연결 이식 (WB04-E565)

`client/RawCampaignGunQueries.cpp/.h`(기준선 `225553df`, 합계 59줄)의
`MakeCampaignGunQueryHooks`를 `webpj/www/core/raw-campaign-gun-queries.js`로 옮겼다.
기준선 뒤 cpppj 소스 변경은 없다. cpppj·보호 원본은 수정하지 않았다.
Archer의 실제 `shotContext`를 시험 장면에 공급하기 위해 7단계에서 4-D C 구획으로 앞당겼다.

## 구현과 연결

- 같은 Core Pool의 `RawGunMode`·`RawGunTarget`·`RawGunHit`와 TYPE 길이의 발사간격 표를 검사한다.
  Core의 참조 멤버는 C++ 값 인수처럼 복사하고, 하위 모듈·표·시각 상태 자체는 공유한다.
- 기존 훅의 감시/생성/발사 장치를 보존하고 현재 조회 일곱 개만 교체한다.
  - `checkForms`: 실제 `CheckRawObjectForms`가 거짓이면 원문 `parent.checkForms()` 진단.
  - `validateIslandOwners`: 같은 `neighbors.footprint`를 먼저 구한 뒤 현재 checking/noIsland TYPE과
    원래 finder/소유자 진단을 사용한다. 0xad 진단은 빈 성공으로 바꾸지 않는다.
  - `targetingMode`: owner 인자를 읽지 않고 같은 실제 모드 getter.
  - `findTarget`: 같은 실제 검색기. C++ float/DWORD/BYTE 인수 폭과 방향 DWORD→signed 비트 해석 유지.
  - `refreshRange`: 현재 Gun 게임 시각을 같은 실제 압축 시각 WORD setter와 공간 통지에 전달.
  - `shotContext`: raw→현재 중심→owner BYTE 캡처→여덟 DWORD 순서.
    raw 좌표 비트 두 개는 NaN payload까지 그대로, 중심은 실제 float32 저장 비트,
    현재 TYPE·모드·앞서 저장한 owner·마지막 프레임 DWORD 순서다.
  - `firingInterval`: 현재 슬롯 TYPE로 공유 float 표를 읽어 double로 승격한다.

`tests/campaign-gun-support.mjs`는 E564의 실제 단일 World 위에 cpppj `CampaignInstallSupport.h`와
같은 보호 `.type` 속성(range·height·theme·hpPerSec·delayBetweenShots)과 실제 프레임/SHP를 묶는다.
E561 자산 후처리를 활성화하고 같은 GunProcess writer·정기·E564 피격 체인을 연결한다.
Archer 애니메이션 생성은 아직 미연결이며 해당 단계가 선택되면 명시 거부한다.
전체 InstallScene/Session을 완성했다고 세지 않는다.

## 검증

전용 cpppj 시험이나 원본 x86 조립 fixture는 없다. 새 검사
`raw-campaign-gun-queries.test.mjs`의 소스 계약 8건이 **첫 실행 모두 통과**했다.

1. 다른 모드/표적/피해 Pool과 잘못된 간격 표 거부, 호출자 훅 보존.
2. 실제 썬 캐논의 여덟 DWORD 문맥, 같은 슬롯의 좌표·owner·방향 프레임 갱신.
3. 크로스보우의 현재 graph BYTE/astral 프레임 모드.
4. 현재 float 간격과 실제 압축 시간 WORD의 12.5→13.875 갱신.
5. 같은 실제 공간 finder의 적 제네레이터 검색, owner/동맹 변경 뒤 검색 결과 갱신.
6. 실제 Gun Form 체인과 부모 void 상태 손상의 원문 진단.
7. 실제 noIsland 공간 후보와 현재 footprint·checking·TYPE·owner의 0xad 진단.
8. 명시 장치 입력으로 확인한 ABI 폭·owner 캡처/프레임 읽기 순서·Core 참조 값 사본.
   raw 좌표 `0x7fa12345`와 `0x80000000`의 보존도 포함한다.

집중 coverage: 줄/함수 100%, 분기 95.65%. fixture manifest는 기존 801항목을 유지한다.
기존 하위의 GunMode·GunTarget·GunHit·CheckingDiagnostics 원본 fixture 대조는 전체 회귀에서
계속 수행하며 이번 연결에 없는 새 원본 관측을 만든 것으로 기록하지 않는다.
HJOW-Athlon에서 Node v22.23.3·v26.13.0 전체 각각 **5,328건 통과, 실패·skip 0**. 대응표 검사 7건·manifest 검사 3건 통과, 대응표 도구 오류 0이다. 로그는 extracted/wb-e565-node22.log·wb-e565-node26.log·wb-e565-focus1.log에 있다(Git 제외). Node v24 전체는 이번에 반복하지 않았다.

진행 수치는 대응표의 todo/partial 행과 재기준선 규칙 key 176개를 대조해 다시 계산했다. 미완료 171단위는 기존 92개·신규 79개다. 인계 0.1/0.2절에 남아 있던 옛 신규 99개 표기를 교정했다.

## 남은 경계

실제 Session 조립과 앱 수명에는 연결하지 않았다(PORT_MAP `port`).
다음은 4-D C 구획 `RawCampaignArcherBindings`: 같은 실제 `shotContext`를 공급하고
현재 장기 TYPE·발사/회전 패킷 수신/직렬화·공통 전송 버퍼도 함께 이식한다.
