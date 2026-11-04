# 보호막 Regular의 현재 TYPE·도달 장치와 진단 복귀

단위864 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이며 원본과 정밀 산출물을 변경하지 않았다. 직전 커밋은 `2c25ad04 / 20261104-51 CP`, 현재 제품은864/11.01이다. LEFT_JOBS 3절4번의 보호막 Regular 별도 진단/수명을 이어 갔다.

## 원문과 실제 연결

PATCH `00448e00`은 사건1에서 `004afc90(this,1,0)`을 호출하고0.08f, 사건2에서 현재 `005412d0` 사제 TYPE와 raw x/y로 `004b1fa0`을 호출한다. 검색 결과가 없을 때만 자기 가상10의 destroy(0)를 호출하고-1, 있으면0.01f를 반환한다. 다른 사건은 현재 checking `005e4794`가 비영일 때 `0`, `.\Dais.cpp`,0x479로 진단한 뒤0을 반환한다. `0050161c`의 PE 바이트 `30 00`으로 표현식0도 확인했다. CD `004848c0`에는 마지막 checking 진단이 없다.

PATCH `RawForcefieldRegular`에서 일괄 객체 Validate와 구성 시 모든 advance/destroy 요구를 제거했다. 도달한 장치만 복사해 호출하고 진단 정상 복귀 뒤에도0을 반환한다. 진행·삭제·진단 중 처리기 소유자가 반납돼도 선택 callable은 현재 호출을 마치며 뒤에서 raw/상태를 다시 읽지 않는다. 효과 누락은 선택 위치에서 명시 오류이며 앞선 검색/콜백 쓰기를 되돌리지 않는다. 실제 장치의 물리 SID 검사는 해당 장치의 계약이며, 사건1/미지원 사건 앞에 별도 슬롯 읽기를 넣지 않는다.

보호막 검색은 `ForcefieldRegularState::sharedPriestType`으로 접두/HP와 같은 `movement.priest` DWORD를 빌린다. 다른 살아 있는 owner 주소를 이미 대여한 상태는 구성 때 거부한다. 실제 Session은 `findTypeAt`을 `world.Find`에 연결해 기존 `RawBridgeConnect::FindTypeAt`의 현재 TYPE·타입별 검색 단계·공간 해시를 재사용한다. Session의 진단은 현재 Client 서비스를 선택 시점에 복사해 호출한다. PATCH 보호막 Regular 분배는 실제 VT `00507770`만 쓰고 TYPE167을 추가 관문으로 두지 않는다. 사제 등 다른 파생 분배는 이 단위 범위 밖이다.

독립 기존 finder는 훅이 비어 있으면 종전 flags0 입력 계약을 유지하며 CD의 기존 구성/객체 사전 계약도 유지했다. 실제 Session의 typed finder와 그 독립 경로를 같은 범위로 세지 않는다. 원문에 없는 CD guard 제거·표 밖 TYPE의 원문 메모리 자체 일치·모든 Regular/전체 Pop을 완료한 것은 아니다.

## 검증과 교정

제품+두 집중 대상의 첫 Release 빌드는651.641초, 수동 guard 교정 뒤 집중 대상만 증분 빌드는11.325초로 둘 다 종료0·경고/오류0이다. 로그 BOM(UTF-16 포함)에 맞춰 경고/오류를 감사했다. 빌드 중 입력 소스 변경0이다. 검사 뒤 교정 검사 파일1개의 줄바꿈만 저장소 원래LF로 복구했으며 논리 소스 변경은 없다.

새 순수8개·실제 Session2개는 **첫 실행 전부 통과**했다. 순수 입력은 미지원 사건의 raw 미조회, advance만의 선택/처리기 반납, 현재 DWORD 전체와 raw float의 typed 검색 인자, 검색 이후 destroy 누락의 부분 효과, 삭제/진단 중 처리기 반납, 현재 checking 상위 비트·원문 진단 인자, 미연결 진단, CD 계약을 대조한다. 큰 TYPE 번호는 명시 typed 장치의 인자 관측이며 실제 World의 표 밖 TYPE 허용 증거가 아니다. Session은 창 없이 `thewarbegins`를 로드해 실제 VT/현재 Client 진단과 실제 사제 위치의 유지 검색 반환만 호출했다. Frame/Process 틱·행동·자기 삭제는 실행하지 않았다.

6묶음 고유31개 중 최초에는 기존 `ForcefieldLifecycle_RequiresBindingAndDiagnosesInvalidEvents` 1개/2단언이 실패했다. 첫 실패는 보호막 접두와 Regular 구성 전, 미연결 PATCH Pop이 공간 쓰기 전에 무변경 거부한다고 기대한 부분이다. 842 이후 PATCH Pop은 원문 현재 부모의 부분 효과를 유지하므로 첫 실패 뒤 남은 깊이가 두 번째 단언도 실패시켰다. 이번 Regular 변경에 도달하지 않는 기존 노후 guard다. 무변경 사전 거부를 `base.Validate` 위치로 옮기고 같은 파일의 설명만 고쳤다. **제품 추가 수정0·fixture/기대값 변경0**이며 해당5개를 재실행해 전부 통과했다. 나머지26개는 최초 통과 결과를 유지한다.

최종31개/6묶음 실패0이다. 기존 보호막 원문3,624행(접두384/Regular3,240), 실제 합성 공간/프로세스 자기 삭제·외부 삭제, 접두9개, 사제 현재 접두6개/실제 Session1개를 포함한다. 원문 재생·합성 수명과 실제 게임 관측을 구별한다. 새 Native/fixture0, 기존 보호2,761개/fixture941개 불변이다. checking12=부모6+부분3+MP3·완료부모36/부분3은 유지한다.

실제 게임/GUI·진단 UI, 포획/희생/특정 적 공격/승리, 공중3종 전용, 신규 MP/통신, 물리입력/PCM/CD, 전체71/CTest/34미션/ZIP 반복은 실행하지 않았다. 현재 EXE SHA-256은 `c83b6833fd20e265c1db208a495e023d849ce417911a8c2b436806a970abdf40`(10,586,112바이트)다. 원문·PE 문자열 확인·첫/교정 입력·로그·보호 해시는 Git 제외 `extracted/cpp-resume-864/`, 커밋되는 상세는 [검증 JSON](../../cpppj/forcefield-regular-current-20261104.json)에 있다.

## 다음 단위

865 첫 후보는 `SquidPostPop::TotalCost`다. PATCH 기계어 `004b0e72..004b0e97`은 현재 raw TYPE로 별도 정수표 `0059ab50[type]`를 읽고 번호*23을 뺀 뒤 현재 누적값에 더한다. 현재 구현은 현재 TYPE float 비용을 다시 인코딩하므로 로딩 이후 float 쓰기에 반응한다. 두 기존 `CurrentTypeBindings` 수동 기대도 그 차이를 반영하고 있어 원문 근거와 함께 바로잡을 대상이다. 이번 빌드 중에는 해당 제품/검사를 바꾸지 않았다. 다른 TYPE 번호 사본·Teleport 생성 함수·다른 Regular/가상20·전체 Pop과 LEFT_JOBS 나머지는 계속 남는다. 전체 목표는 active다.
