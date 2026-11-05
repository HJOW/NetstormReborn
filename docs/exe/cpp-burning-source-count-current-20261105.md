# 연소점 HP 조회·정수식과 진단 복귀 연결 (단위902)

2026-11-05 HJOW-Athlon에서 작업했다. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 전체 cpppj 목표는 진행 중이다.

## 원문과 구현

PATCH00424110/CD004a5d30은 선택된 피해 슬롯의 signed DWORD/WORD HP를 최대HP getter **전에** 저장한다. 당시 raw TYPE의 같은 물리 this를 실제0049fce0/CD00444720으로 읽으며, 진단 복귀 뒤에도 이미 저장한 HP와 TYPE this를 다른 피해/TYPE로 교체하지 않는다. 후단의 flags2 조회004ac200/CD004abae0은 같은 선택 슬롯의 **현재 raw TYPE**를 다시 읽는다. 따라서 진단이 raw HP/TYPE나 컨트롤러의 피해 번호를 바꿔도 사전 HP와 현재 TYPE 플래그가 함께 사용된다.

`RawBurningSources`를 공유 `ReadRiftHitPoints`에 연결했다. 현재 checking/진단 장치는 기존 victim 상태를 사용하고, 사제/dais/약화 전역은 같은 Client 수명의 GameAux 주소를 빌린다. 실제 `RawCampaignSession`이 `RawCampaignParticles::BindHitPointTypes`로 이 두 TYPE 주소를 연결한다. Session 지역 보상 객체를 입자 수명 밖에서 캡처하지 않는다. 기존 독립 입력의158/165와 값형 약화는 미연결 fallback으로 유지한다.

새 `BurningSourceCount`는 원문의32비트 계산을 실행한다. `half = wrap32(maximum * 2) / 4`이며 단순 maximum/2로 바꾸지 않는다. PATCH는 `wrap32(HP*5) / wrap32(maximum-half)`를6에서 빼고, CD는 `wrap32(HP*5) / wrap32(half-maximum)`에6을 더한다. 결과와 `wrap32(width+height)/5`의 작은 값을1 이상으로 제한한다. 모든 signed 나눗셈은0 방향으로 절삭한다.

0 나눗셈과 INT_MIN/-1은 원문의 x86 IDIV 실패 위치에서 각각 명시적 domain/overflow 오류로 보고한다. 아직 도달하지 않은 flags2의 강제1로 실패를 숨기지 않는다. 계산 뒤 현재 TYPE의0x400을 읽어 목표를1로 바꾸고, 높이3 미만의 이른 복귀는 실제 픽셀/행 주소를 요구하지 않는다. 호스트 이미지 주소 범위 검사는 해당 메모리가 필요한 뒤쪽 경계에 둔다. 입자 선택/추가·제거·난수/행 표식의 기존 정상 경로는 보존한다.

## 원문 관측

[격리 도구](../../tools/decomp_burning_source_count.py)는 양판본의 실제 연소점 부모, HasHP/TYPE/flags2 조회, 실제 HP getter/getNum과 정수식을 실행한다. robust의 반환1과 외부 진단 UI의 명시 정상 복귀/메모리 변경만 장치 경계다. 유효한 피해 SID50·높이2에서 목표 행 저장 후 복귀하므로 새 원문 관측에서는 입자 루프·난수·픽셀·실제 게임/공격/MP를 실행하지 않는다. CD의 기존 C++ 유효 SID 앞단을 신규 robust 복원으로 세지 않는다.

각판본2,826·합계 **5,652행**을 최초 동결한 뒤 재관측 결과가 같았다. 정상4,764행, 실제 CPU0 나눗셈864행, signed 몫 범위 초과24행이다. INT_MIN/큰 양수의 곱셈 감김, CD WORD 해석, 폭/높이 합 감김, 약화/현재 번호, 진단 뒤 HP/TYPE/피해 번호/선택 TYPE HP 변경과 후상태를 구별한다. 기대값을 C++ 수식으로 만들지 않았다.

관측 도구 준비 중 #DE 뒤 에뮬레이터의 다음 입력 복귀가 두 번 실패했다. 입력마다 초기 CPU 문맥을 복원하고 실제 interrupt0을 관측해 오류 위치/제수를 기록하도록 수정한 뒤 최초 동결과 재관측을 완료했다. 이는 원문 나눗셈 실패를 정상 반환으로 보정한 것이 아니다. 입자/픽셀 루프는 기존 동결 fixture 검사와 구별한다.

Release 제품/집중 빌드1명령은 종료0·경고/오류0이다. 정리 도우미 종료→2,858소스 snapshot→CMake를 같은 명령에서 순차 실행했고, 빌드 중 소스 편집/다른 빌드 시작은 없었다. 최종 소스는 빌드 직전 snapshot과 같다.

새3/기존14의 **고유17사례·5명령/17실행이 첫 실행부터 실패0**이다. 새5652행의 실제 선택기 정상/IDIV 실패와 선택/HP·TYPE·피해 번호/전역/진단 후상태, 장기 사제/dais 예외, 자기 교체 진단의 캡처 수명을 검사했다. 기존 Source4/Corner3/피해 앞단2/공유 HP5를 함께 실행했다. 기존 연소점 선택/추가·제거/300시도·폴백/행 표식/난수 관측은 모두 불변이다.

기존 보호2,778개/958fixture가 불변이고 새fixture1개로 다음 기준은 **2,779개/959fixture**다. 기존 입력/관측 기대값은 수정하지 않았다. 원시 로그/소스 snapshot/보호 해시/정밀 원문·ASM은 `extracted/cpp-resume-902/`(Git 제외·현재 PC 전용)에 보존한다. 커밋 증거는 [JSON](../../cpppj/burning-source-count-current-20261105.json)이다. 활성74프로젝트/과거 전체·GUI·34미션·ZIP 기준은 유지한다.

제품은 **902 / 표시·파일·제품11.01**,10,722,304바이트, SHA-256 `f4928b0224ae18e7eb77028764150b0f9f52b7c7e08a43ef29adde36ab7931be`다.

## 잔여

다음은 회복 경로의 현재 사제 TYPE/조회 순서와 월드 설치 전 Help 등 다른 HP caller다. 원문 회복00494580/CD0040ccd0은 사제 번호005412d0/CD0051cbbc를 매 조회에서 읽지만 현재 C++는158 고정을 사용한다. 전체 HP caller/Factory/Pop 완료로 세지 않는다. checking8(부모3/부분2/MP3), 완료부모39/부분4와 CD Take/contained/Form·State/파일·객체UI/Edit·Windows·무검사MP 및4~8차의 전체 목표를 유지한다.
