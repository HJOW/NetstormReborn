# 캠페인 피격·충돌 연결 이식 (WB04-E564)

`client/RawCampaignHitBindings.cpp/.h`(기준선 `225553df`, 합계 223줄)를
`webpj/www/core/raw-campaign-hit-bindings.js`로 옮겼다. 기준선 뒤 cpppj 변경은 없다.
Session 생성자가 사제 월드 연결 다음에 무조건 구성하므로 7단계에서 4-D C 구획으로 앞당겼다.
cpppj 소스·보호 원본은 바꾸지 않았다.

## 구현

- 같은 World Core와 사제/HP Pool을 검사하고, 실제 내용물 조회 → burning → 사제 피격 →
  일반 피격/방어 → 신전 피해 → 주변 Dude 통지 → 충돌 효과를 구성한다.
- 사제 피격은 현재 Host의 605 사건 조회, 604/605 사건 전체 취소, 현재 프레임 지정,
  사제 HP/공간 전환, burning, 같은 감시 Form 호스트를 연결한다.
- 일반 피격은 같은 가상 삭제/표시/현재 지면을 연결한다. 실제 +9c 표 83개와 윈드 타워
  `0x50ead0`의 방향 방어를 분배하며 나머지 방어는 명시 외부 장치가 필요하다.
- `damageHooks`는 일반 → 특수 파생 → 사제 훅 합성 순서다. 특수 파생은 실제 +98이 신전
  `4dba40`·불워크 `48ef20`·제단 `448b70`인 경우만 선택한다. 제단의 현재 방향 11/12는
  무피해로 1을 반환하고 나머지는 일반 피해다.
- `damageResult`는 victim VT를 읽은 뒤 attacker owner BYTE를 읽고 실제 여섯 +98 몸체의
  반환을 DWORD로 전달한다. TYPE 명칭·void 훅 반환을 근거로 0/1을 추정하지 않는다.
- 충돌 효과는 실제 Factory·Kernel·Regular·Pop·프레임·가상 삭제를 공유한다.
  `missileHooks`는 같은 GunHit과 현재 Missile 시각을 피해/시각 갱신과 충돌에 연결한다.
- `handler`는 충돌 anim만 추가한다. 공통 삭제를 쓰는 썬 디스크(83/132), 빈 투사체
  (80/81/130), anim VT만 같은 표면 수명 Pre/Post와 실제 가상 삭제로 보낸다.
  다른 자산·다른 효과는 기존 fallback을 유지한다.
- C++ 값 캡처처럼 외부 방어/삭제 함수 사본을 고정한다. 소비자인 E563 사제 연결도
  함수 묶음을 소유하도록 보강했다(상태/장기 몸체 참조는 계속 공유).

## 검증과 최초 실패

전용 원본 x86 조립 fixture와 cpppj 전용 시험은 없다. `CampaignInstallSupport.h`의
관련 구성을 `tests/campaign-hit-support.mjs`에 옮겨 보호 TYPE 188칸·실제 프레임/SHP로 만든
`RawSurfaceWorld`와 E563 사제 연결, 같은 HP·SP·ProcessHost·Kernel을 조립했다.
전체 캠페인 InstallScene의 이식 완료를 뜻하지 않는다.

`raw-campaign-hit-bindings.test.mjs`의 소스 계약 14건과 사제 연결의 함수 사본 보강 1건을
추가했다. 사제+피격 집중 검사 총 26건 통과:

- 사제 HP 변경·604/605 중복 사건 전체 취소·기본 프레임·HP1 반환과 실제 보호막 생성
- Base/일반 피해·source owner BYTE·서버 삭제와 미지원 VT/fallback
- 현재 지면·Player 주 섬·실제 시각의 경고 순서/기한
- 불워크의 현재 비행 genus, 윈드 타워 실제 방향 방어
- 제단 L/M 무피해와 다른 방향 일반 피해, 다이스의 다른 실제 VT/HP 판정
- 건설 중 신전의 실제 내용물 Form 조회와 끝 시각/표시
- burning 현재 Form 조회/실제 Host 삭제와 외부 생성 경계
- Dude의 현재 task Regular 생성/현재 시각·외부 burning 요청과 미복원 경로 거부
- 일반 방어 외부 경계·제단 프레임 물리 범위, 함수 사본 고정
- 실제 충돌 anim 생성/프레임/삭제·Missile 현재 시각·공통 자산 삭제/fallback

최초 실행 전 시험 파일 객체 괄호 1개를 고쳤다. 첫 실제 집중 실행은 4/11 통과였으며
제품 코드 이식 결함이 아닌 시험 장치/입력 오류를 다음처럼 교정했다.

1. RawSurfaceWorld의 필수 프로세스 송신 경계를 명시 거부 함수로 공급했다.
2. 일반 표적의 raw HP 100이 실제 최대 HP 절반보다 작아 의도하지 않은 burning 생성
   경계를 선택했다. HP 감소/방어/경고 시험 입력은 10000으로 명시하고 고정 기대 HP도
   각각 9997/9991/9995 등으로 맞췄다. 낮은 HP의 burning 생성 경계는 별도 시험으로 유지했다.
3. Kernel의 실제 API `get(1)`로 예약 프로세스를 조회하고, 공통 자산 삭제는 완전한
   Form/선택 훅이 있는 `world.destroy`로 진입하도록 장치를 고쳤다.
4. TYPE164 제단과 TYPE165 다이스에 같은 +98이 있다고 가정한 시험을 교정했다.
   실제 표에서 `448b70`은 TYPE164의 VT에만 있다. 완성 다이스는 일반 HP 보유 판정이
   거짓이라 0을 반환한다. 두 실제 생성자의 구분을 별도로 단언했다.

이후 집중 13건 범위 coverage는 줄 99.41%·분기 97.65%·함수 90.91%였다.
추가 함수 사본 시험까지 사제+피격 26건이 통과했다. 실제 교전·특정 적 공격이나 자연 플레이를
실행한 결과로 세지 않는다.

HJOW-Athlon에서 Node v22.23.3·v26.13.0 전체 각각 **5,320건 통과, 실패·skip 0**. 대응표 검사 7건·manifest 검사 3건 통과, 대응표 도구 오류 0이다. 로그는 extracted/wb-e564-node22.log·wb-e564-node26.log·wb-e564-focus8.log에 있다(Git 제외). Node v24 전체는 이번에 반복하지 않았다.

## 남은 경계

- 웹 Storage는 아직 Native 전송 장치 경로가 없다. Native 장기 Storage의 현재 Dude 경로
  TYPE 대여는 4-D E 구획에서 설치한다. 독립 typed 경로의 기본 24는 원문과 같다.
- 실제 Session 생성/Load와 전체 InstallScene, Archer·Motion·Path 등 나머지 연결이 남았다.
- 다음 시작점은 4-D C 구획의 `RawCampaignArcherBindings`와 해당 생성자가 요구하는
  `RawCampaignGunQueries` 등이다.
