# 회복의 현재 TYPE·HP 순서와 Help HP 연결 (단위903)

2026-11-05 HJOW-Athlon에서 작업했다. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 전체 cpppj 목표는 진행 중이다.

## 원문과 구현

회복 PATCH00494580/CD0040ccd0은 난수/추적 접두 다음에 현재HP와 객체 MaxHitPoints를 비교한다. 회복할 경우 매 증분마다 현재 사제 TYPE 번호005412d0/CD0051cbbc를 저장한다. PATCH는 **번호→현재HP→TYPE HP**, CD는 **번호→TYPE HP→현재HP** 순서다. TYPE getter의 외부 효과로 번호나 HP가 바뀌더라도 해당 호출에 저장한 번호는 유지한다. 후보값을 두 번째 객체 MaxHitPoints와 비교하고, 상한 초과면 세 번째 객체 MaxHitPoints를 다시 읽으며, 그 외에는 증분 전체를 다시 계산한다. signed HP/6은0 방향 절삭, ADD는32비트 감김이다.

`ReadPriestRegenHitPoints`로 이 부모의 순서를 분리하고 실제 `RawPriestRegen`에 연결했다. 기존 선행 HP 읽기를 난수 접두 뒤 원문 위치로 옮겼다. 실제 Session은 같은 장기 GameAux의 사제 TYPE 주소를 연결하며, 독립 입력만 기존158을 사용한다. 대상 Actor의 TYPE158/VT는 그대로 확인하고 회복 분자의 TYPE만 현재 전역으로 선택한다. 기존 Setter/표시/연소 갱신·중립화·추적 후단을 사용한다.

월드 설치 전 Help의 asset.maxHitPoints 직접 반환을 제거했다. `MakeCurrentTypeHitPoints`는 호출 당시 표에서 TYPE this를 선택하고 공유 `ReadRiftHitPoints`에 넘긴다. 진단 뒤에도 같은 물리 TYPE를 유지하며 현재 표 기준·checking/약화·사제/dais를 다시 읽는다. 실제 Help는 메뉴 생성 전에 설치된 Client의 SurfaceStorage와 GameplayGlobals를 함께 소유 참조한다. 진단도 그 저장소의 실제 AssertDisplay를 사용한다. World/Session이 없어도 같은 getter이며, 다른 Help 이름/설명/플래그 공급자의 전체 복원을 뜻하지 않는다.

## 원문 관측 범위

[격리 도구](../../tools/decomp_priest_regen_hit_points.py)는 회복 HP 분기 내부 PATCH004945ee/CD0040cd15부터 Setter 진입 또는 회복 없는 후단까지 실제 명령을 실행한다. Actor/TYPE getter의 반환·HP/현재 번호 변경은 명시 장치다. 번호/HP 메모리 읽기·판본별 순서·비교·signed 나눗셈·ADD·현재 GetType 산술은 실제 명령이다. 실제 getter 본체/Setter/전체 회복 처리기·앞선 난수/추적·후단 표시/효과는 이 새 Native 관측에서 실행하지 않는다.

각432/총864행을 최초 동결하고 재관측 결과가 같았다. HP6종·객체 최대HP 반환3개 묶음4종·TYPE 최대HP 반환2개 묶음6종·외부 변경3종을 교차했다. 정상/음수·INT_MIN·큰 양수와 CD WORD 좁힘, 반복 getter와 현재 번호/HP 변경을 구별한다. 기대값을 C++ 식으로 생성하지 않았다. 도구 최초 관측 뒤 주석과 보고서 경계 설명의 잘못된 한자 표기 두 곳을 한국어로 고쳤으므로 최초/재관측 tool SHA는 다르며 fixture SHA는 같다.

Release 제품/집중 빌드1명령은 종료0·경고/오류0이다. 정리 도우미 종료→2,863소스 snapshot→CMake를 같은 명령에서 순차 실행했고, 빌드 중 소스 편집/다른 빌드는 없었다. 최종 소스는 snapshot과 같다.

새4/기존19의 **고유23사례·6명령/23실행이 첫 실행부터 실패0**이다. 새 회복 부모864행·UI reader의 기존 HP 관측1158행과 미선택 장치 경계를 대조했다. 실제 회복 처리기의 두 판본에서 Actor158을 유지하면서 분자 TYPE74→75 변경을 확인했다. 기존 전체 회복10사례·Help HTML2/요청2·직접 공유HP5도 통과했다. 실제 게임/GUI/진단 UI/신규 MP 검사는0이다.

기존 보호2,779개/959fixture는 불변이며 새fixture1개로 다음 기준은 **2,780개/960fixture**다. 기존 입력/기대값은 수정하지 않았다. 원시 로그/소스 snapshot/보호 해시/정밀 원문·ASM은 `extracted/cpp-resume-903/`(Git 제외·현재 PC 전용)에 보존한다. 커밋 증거는 [JSON](../../cpppj/priest-regen-help-hit-points-current-20261105.json)이다. 활성74프로젝트/과거 전체·GUI·34미션·ZIP 기준을 유지한다.

제품은 **903 / 표시·파일·제품11.01**,10,733,056바이트, SHA-256 `cafc290cf5d8979db4825f07c11d051f5cd9d906cbd88e4791e05daabfc474cd`다.

## 잔여

다른 HP caller의 실제 공급자와 CD Take/contained/Form·고정 Form 경로를 이어서 대조한다. 전체 HP/Factory/Pop 완료로 세지 않는다. checking8(부모3/부분2/MP3), 완료부모39/부분4와 State/파일·객체UI/Edit·Windows·무검사MP 및4~8차의 전체 목표를 유지한다.
