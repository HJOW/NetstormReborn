# Vortex 진단 복귀와 현재 Player·목록 후처리

단위854 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치다. 직전853은 `e7427079`이고 현재제품은854/11.01이다. 전체 목표는 미완료다.

PATCH004dc700과세 wrapper004dce40/004dd020/004dd1f0을 다시 읽었다. 실제 가상 진입의 일괄 Validate를 제거하고 Damageable/공통 부모 뒤 도달한 진단/장치만 요구한다. owner0은 식 `getPlayerId() != INVALID_PLAYER_ID`·Vortex.cpp:259, 없는 섬은 `islandList.exists( islandId )`·Vortex.cpp:281을 실제 서비스에 전달한다. 정상 복귀하면 현재 owner를 다시 읽어 물리 Player0~8 기록과 섬 등록을 계속하며 무조건 throw로 바꾸지 않는다. Player0의 물리 기록과 실제 유효 owner1~8의 의미를 구별한다. owner9 이상은 호스트 물리 경계에서 거부한다. 명시 Validate/Activate와 CD의 기존 사전/진단 중단 계약은 유지한다.

음향 뒤 WORD0은004ac220의 물리 store→현재 Bridge DWORD 조건부 통지를 사용한다. 두 신전 목록은00414450의 signed count/capacity 관문 뒤에만 현재 물리/중복 검색을 사용하고 처음 count에 저장한 다음 현재 count를 증가한다. 가득 찬 목록은 과거 저장소에 접근하지 않는다. 음수 실제 저장/손상 물리 범위는 도달한 호스트 경계에서 명시 실패한다. 선택 Form SID의 전체 DWORD를 확인해 상위 비트를 잘라 다른 SID로 바꾸지 않고, 실제 WORD TYPE 오류도 조용히 무시하지 않는다. 현재 TYPE의 프레임 쓰기/원소 wrapper는 free/contained가 바뀌어도 같은 물리 슬롯을 사용한다. 삭제 iterator는 다음 SID를 효과 전에 선택하고 current boss/로딩은 원문의 소비 위치에서 읽는다. 다른 TYPE/모드 전역 전체 공유는 별도 잔여다.

새 격리 관측기는 현재 정밀 함수표만 사용한다. 실제 세 wrapper/공통 부모·WORD setter·중복 목록을936행에서 두 x87 정밀도로1872회 실행했고 전체를 재관측해 TSV가 byte 일치했다. 정상723행/명시 진단 중단213행·진단 입력426행이다. raw50 전체/목록 stale·현재 전역·9 Player 전체 Adler와0/1/8의12DWORD/원문 trace를 수집했다. Damageable·음향/섬/소유자·내용물/예약/진단은 명시 장치이며 실제 하위 전체 실행으로 확대하지 않는다. 첫 Native 실패는 옛 owner 추출 경로가 이 PC에 없었던 것이고 두 번째는 선택 Form이 호출한49a840의 허용 범위 누락이었다. 두 소스/실패 로그를 보존하고 현재 정밀 범위를 직접 로딩하며 실제49a840 범위를 추가해 세 번째/재관측을 통과했다. 전체 바이너리를 실행 허용하지 않았다.

새20개(순수19+실제Session1) 포함 집중161개/28묶음을 준비했다. 새 Native936행과 기존 양판본 Vortex528행/현재 Player576행을 기대값 수정 없이 재생하며 미도달 장치·선택 실패의 부분 효과·signed 공유 중복 목록·전체 선택 SID·현재 수명/프레임·삭제 iterator의 cached next를 확인한다. 실제 Session은 명시 일반 슬롯/flags2의 진단 복귀 뒤 현재 owner8의 실제 Player 두 DWORD 쓰기만 확인하고 입력을 복구한다. 실제 섬 등록/다른 신전 삭제나 의식·승리 재현은 하지 않는다. 첫 집중161개는160개 통과/새 Native 재생1개 실패(1단언)였다. 검사 장치가 이전 행에서 free가 된 슬롯을 AllocatedBytes로 재준비하고 명시 Owner 장치에도 할당 관문을 적용한 차이를 각 입력 초기화/물리 BYTE 쓰기로 고쳤다. 해당1개만 재실행해936행 전체가 통과했고 최종161개 실패0이다. fixture 입력/기대값·제품 코드는 첫 빌드 뒤 변경0이다. 제품/두 집중 대상 첫 빌드와 교정 검사 대상 빌드 모두 종료0·컴파일 경고/오류0이며 핸들71486/69770 종료·자식 해제를 확인했다. 소요는67.6943886초/22.176484초다. 첫4소스와 교정4소스·모든 실패 로그를 보존했다.

기존2760개/fixture940개는 불변이며 새 fixture1개 포함 보호2761개/fixture941개다. 첫4소스/원문·원시는 Git 제외 `extracted/cpp-resume-854/`에 보존한다. 기존779의51함수와 별도 부속 진단이라 checking12·완료부모36/부분3 유지다. 다른 현재 가상20/전체 Pop·TYPE/모드 전역 및 LEFT_JOBS 전체 잔여는 남는다. 실제 게임·Frame/Process 틱·공격/포획/희생/승리·공중3종 전용·신규MP/통신·진단 UI·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복을 제외한다.

현재 제품 EXE SHA-256은 `fc8862fd3e29288f814fcb7bca5f27c28d5eed76eda12ccd7f4a6fd75844689a`다. [검증 JSON](../../cpppj/vortex-current-20261104.json)에 최초/교정 실행·Native 준비 실패·원문/소스/fixture SHA와 검증 한계를 보존했다. 단위 인계/로컬 커밋 뒤855로 이어가며 목표는 active다.
