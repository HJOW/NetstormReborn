# Carrier의 내용물 교대 표시와 Regular 사건

작업 단위145. 기준 `2c79a9b`, 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**이다. 새 디컴파일·원본 게임/창 실행·DirectDraw 수정은 없다. 단위144에서 표시 기한 프로세스를 완료한 뒤 그 호출자인42a750을 복원했다.

`RawCarrierRegular`는491 내용물 교대 표시,492 지형/경로/회피,493 시전 기한,5105 낙하 위임 및44b900 Damageable fallback의 전체 부모 순서를 보존한다. 실제+5c가42a750인 여섯 VT만 자동 분배하며 사제494580 등 다른 파생은 caller가 원래 사건을 먼저 처리해야 한다. TYPE나 genus만으로 파생 처리를 확장하지 않는다.

491은 mask20/1 순서로 각각 내용물을 조회한 **뒤** `float(1.5/payload)`와 signed count의 float 저장값을 비교한다. 기한을 초과하거나 부적합 TYPE이면 앞서 발생한 효과를 되돌리지 않고0을 반환한다. 두 종류를 count 하위비트에 맞춰 번갈아 표시하며, 내용물이 하나라도 있으면 마지막에 현재 부모 가상88을 갱신한 뒤 기본 Handle로 넘긴다.

새 자산은 현재 kind/flags2로 생성한 뒤 새 자산의 현재 TYPE 기본 프레임을 읽는다. 부모의 현재 z-order에서1을 빼고 extra bit8을 켠다. 내용물 WORD는 생성 뒤, 부모 owner는 WORD 효과 뒤, 부모 float 좌표는 owner 효과 뒤에 읽는다. y에서 종류별 원문 float0.8/1.2를 뺀 값을 float로 저장한 뒤41d0d0으로 제한한다. Pop 뒤 new18의 성공 여부에 따라428700 타이머를 생성하며 할당 실패가 앞선 자산을 지우거나 표시 갱신을 생략하게 만들지 않는다.

493은 FindContained(1,0,0)이0이어도 원문처럼 물리0번 슬롯의 WORD kind를 읽는다. TYPE+1b8은 **casttime**이며 RiftType 로더의 `piVar9[0x6e]` 저장에서 확인했다. casttime*10의 DWORD 감김을 signed count와 비교한다. 일반 RiftTypeRecord를 바꾸지 않고 같은 번호의 보충 표를 받아 프레임/시전 시간을 공급한다. Session이 실제 정의를 읽어 이 표를 구성하는 것은 다음 연결 작업이다.

NaN 좌표 검증에서 디컴파일 표현의 오류를 교정했다. 첫 구현은 C 표현 `1<=value ? value : 1`을 따랐고 새 원문 검사에서224개 check가 실패했다. 실제41d0d0의 하한 `test ah,41/jne`, 상한 `test ah,5/jp`와 원본 관측은 unordered NaN을 보존한다. 제품을 `value<1 ? 1 : value` 및 `255<value ? 255 : value`로 고쳤다. 도구/입력/기대 fixture는 변경하지 않았다. 초기 실패 로그 focus1/focus2를 보존하고 최종 focus에서 원문837행이 모두 통과했다.

독립 도구 `decomp_carrier_regular.py`는 보호 PE의42a750·41d0d0·49a840·4ac550을 전체 실행한다. 큰 내용물 검색/생성/WORD/Owner/Pop/할당/타이머/표시/492 지형·이동·회피/5105 낙하/Damageable는 명시 입력이다. 실제 caller `fstp float32`까지 실행해 반환을 관측한다. 원본 80비트 중간값 자체의 동일성을 주장하지 않는다. GP/stack canary/ret12·SEH FS0 복구·x87 제어와 빈 스택, 허용 코드/읽기/쓰기 범위를 검사했다.

새 관측은 **837입력·1,674정상반환·241,652명령·53/64 정밀도 차이0**이다. mask/종류/기한/음수·NaN·무한 payload, signed count, callback의 현재 TYPE/owner/좌표/VT 변경, 할당 실패, casttime 감김,492 권한·지형·경로 존재,5105 전용 반환과 전체 기본 float 반환을 포함한다. 별도 VT fixture는 보호 PE에서 직접 읽은83VT의+5c DWORD다. 기존 fixture/원본을 수정하지 않았다.

집중 검사는 원문 전체/83VT 분배/늦은 경계 요구의3개다. 세 개 모두 최종 통과했고 실제6개 VT와 다른77개 파생의 fallback 보존을 확인했다. 실제 Session/Timer 연결이나 사용자 자연 선택/우클릭 완료 검증은 아니다. 최초 세 Release 빌드는 모두 종료0이며 최종 빌드3은 경고0·오류0이다. 검증 중 실패는 빌드 실패가 아니라 수정 전 NaN 기대 불일치다.

다음 우선 작업은 같은 Session의 현재 시계와 SoundProcessState.processType으로 타이머를 연결하고, 실제 자산 생성/소유자/Pop/삭제와 기존 RawAssetRegular.DamageableHandle을 이 부모에 연결하는 것이다. InstallContents.checkCarrier의 잘못된426fc0 지면 검사를 실제426080 미리보기 예약으로 교체한다. 사제259/25a/25b/25c/25d를 무조건 기본 Carrier로 보내지 않는다. 492의4225b0/429010과5105의426ef0 큰 하위 몸체는 이번 관측에서 명시 경계다. 미복원 하위를 빈 성공으로 대체하지 않는다.

CD10.72의42a750 정밀 대응은 현재 없다. 좌표 제한은43f950, 낙하 helper는4e4ff0 대응을 기록하지만 이번 제품은 PATCH188 전용으로 CD를 거부한다. 실제 Client 입구가 바뀌지 않아 클론 창 진입을 반복하지 않았다. Early Missions6·캠페인1-1/1-2·데모3의 자연 완주/승패/저장/다음 미션과 전체 동일성, 생산/경제/AI/로그인된 로컬 전송은 계속 미완료이며 전체 목표는 active다.

로그 접두부는 `extracted/goal-carrier-regular-20261017-`, 원본 보고서는 `cpppj/carrier-regular.json`, 제품 연결 보고서는 `cpppj/campaign-carrier-regular.json`이다. 전체 회귀/최종 보호 및 인수인계 결과는 아래에 추가한다.

최종 전체 CTest는 **1,586개·실패0·종료0·215.85초**(CTest215.88초)다. 최종 빌드3/집중3개와 같은 바이너리의 전체 검사이며 빌드/전체검사는 겹쳐 실행하지 않았다. 38163/73516/67331/30180/63613은 모두 종료했다.

원본 감사63종은 **52성공·기존11실패·새실패0**이다. 기존11종은 직전 단위의 SHA 불일치9종/로컬 정밀 산출물 부재2종이다. 기존38보고서58현재핀만 갱신하고 과거값/이력을 보존했다. 보호SHA3·기존CMake접두·UTF-8/LF·기존fixture/AGENTS/원본/DN 불변을 확인하고 인수인계를 마친 뒤 `20261017-45 CP 캐리어의 내용물 교대 표시와 원본 사건 처리 복원`으로 로컬 커밋한다. push하지 않는다.
