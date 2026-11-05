# 보상 HP 조회의 현재 진단 복귀 연결 (단위900)

2026-11-05 HJOW-Athlon에서 작업했다. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 전체 cpppj 목표는 진행 중이다.

## 변경 범위

`SquidReward::TypeHitPoints`가 독자적인 debug 예외와 번호 기반 HP/4 계산 대신 단위898의 `ReadRiftHitPoints`를 호출한다. 처음 선택한 물리 TYPE와 표 기준을 유지하고, 원문 getNum을 소비하는 시점의 현재 표 기준·checking·dais·약화·사제 번호를 읽는다. 진단이 정상 복귀하면 같은 TYPE this의 현재 HP로 계속한다. 선택된 진단 장치가 없으면 기존처럼 명시 오류로 멈추며 성공 값을 대신 반환하지 않는다.

실제 `RawCampaignSession`은 같은 Client의 기존 assertion 서비스를 보상 HP에 연결한다. checking은 이미 연결된 같은 AiDebug DWORD이며, 약화/사제/제단/dais는 기존 GameAux의 장기 주소를 유지한다. 진단 callable은 선택 시 복사하므로 콜백이 자기 연결을 교체해도 호출 중 소유 자원이 먼저 파괴되지 않는다.

PATCH 객체 최대 HP004adcb0은 일반 TYPE 또는 다이스의 현재 제단 TYPE를 선택하고0049fce0을 부른다. CD004aed30은 같은 분류를 판본별 필드에서 읽고00444720을 부른다. 기존 `MaxHitPoints` 경로를 통해 공유 getter를 사용하며, 진단 도중 raw TYPE/제단 번호/표 기준이 바뀌어도 이미 선택한 getter this를 다른 TYPE로 교체하지 않는다. 독립 사본과 CD의 기존 `BindCurrentTypes` 거부 계약은 유지한다.

## 검증 범위

기존 양판본 x86 getter fixture1160행을 보존한다. 새 보상 어댑터 검사는 그중 **1158행**의 독립 원문 반환/진단을 직접 대조한다. 초기 getNum이 음수인2행은 번호로 TYPE를 선택하는 보상 API의 입력 범위 밖이라 이 어댑터 검사에서 제외하며, 기존 공유 getter1160행 검사는 그대로 실행한다. 진단 뒤 표 기준이 바뀌는 유효 입력은 포함한다. 기존 보상 모드의 약화 bool에는 원문 DWORD의 영/비영 분기만 대응시키며 전체 raw DWORD 저장의 동일성을 주장하지 않는다.

추가 검사는 다이스 최대 HP의 선택된 제단/진단 뒤 현재 약화와 표 재대여, 누락/자기 교체 진단 callable의 수명과 선택 조건이다. HP 숫자/메모리 어댑터만 검사하며 실제 게임·사제 포획/희생·특정 적 공격·승리·MP는 실행하지 않는다. 기존 원문 입력/기대값은 수정하지 않고 새 Native 관측/fixture를 만들지 않는다.

Release 제품/집중 빌드1명령은 종료0·경고/오류0이며, 소스 정리 도우미와 snapshot 저장이 끝난 뒤 같은 명령에서 빌드를 순차 실행했다. 빌드 중 소스 편집이나 다른 빌드 시작은 없었다. 최종2,852소스의 정규화 해시는 빌드 직전 snapshot과 같다.

새3/기존17의 **고유20사례·4명령/20실행이 첫 실행부터 실패0**이다. 기존 현재 TYPE5/장기 번호7/공유 getter5를 함께 실행했다. 보상1158행과 직접 getter1160행은 같은 동결 원문 결과를 각각의 경로에서 대조한 것이며 새 원본 실행으로 합산하지 않는다. 현재 원문 caller 목록은 PATCH19호출점/11부모, CD19호출점/12부모다.

기존 보호2,777개/957fixture가 모두 불변이고 새 fixture는0개다. 원시 로그/소스 snapshot/보호 해시/원문 C/호출자 감사는 `extracted/cpp-resume-900/`(Git 제외·현재 PC 전용)에 보존한다. 커밋 증거는 [JSON](../../cpppj/reward-hit-points-diagnostic-current-20261105.json)이다. 활성74프로젝트 및 과거 전체/GUI/34미션/ZIP 기준은 유지한다.

제품은 **900 / 표시·파일·제품11.01**,10,715,648바이트, SHA-256 `bf13702d217cf22aec0906d5a36af397dcb65e4f42c25dc1fd60cbb249d23a40`다.

## 잔여

전송의 maxHp lambda, SpriteDraw 막대, RawBurningSources와 월드 설치 전 Help 등 다른 HP 조회는 별도 감사·연결 대상이다. 이번 보상 연결을 전체 HP caller 또는 Factory/Pop 완료로 세지 않는다. checking8(부모3/부분2/MP3), 완료부모39/부분4와 전체 목표를 유지한다. 이어 다른 HP caller·CD Take/contained/Form·State/파일·객체UI/Edit·Windows·무검사MP와4~8차를 계속한다.
