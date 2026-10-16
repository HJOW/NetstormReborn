# 썬 바리케이드의 실제 Regular 제어와 기존 연결 표식 삭제

현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-16**으로 일치한다. AGENTS.md와 양 인수인계를 다시 읽고 단위82의 실제 연속 Kernel 실패에서 이어갔다. 보호 원본·AGENTS·dotnetpj를 변경하지 않았고 게임/GUI를 실행하지 않았다. 목표는 active·미달성이다.

## 실제 Handle와 남은 반환 경계

TYPE136 `sunFence`의 사용자 표시명은 **Sun Barricade/썬 바리케이드**다. `00509470+5c`는 Handle00457a50이다. `RawFenceRegular.*`로 아래 순서를 복원하고 실제 Session의 Regular 분배기에 연결했다.

- extra bit20은 사건 번호보다 먼저 실제 Damageable에 위임한다.
- 사건0은 현재 프레임을 먼저 보관하고 현재 글자 방향/구간 수를 읽는다. signed32 `(현재+1)%구간수`를 절대 프레임으로 지정하며 구간의 첫 번호를 더하지 않는다. 다음으로 현재 BYTE owner와 +2a DWORD 캐시를 비교한다.
- owner가 다르면 방향0/2/4/6의 최근접 SID를 **모두 모은 뒤** 현재 owner를+2a에 쓴다. 스냅샷의 각 이웃에 대해004566d0으로 기존 표식을 지우고, 그 효과 뒤의 현재 좌표로00455b20(mode0)을 호출한다. 마지막에 독립 new(0x28) 성공일 때만 같은 Kernel에 사건2/payload0.1f를 예약한다.
- 이어 활성 연결 수004573d0을 조회한 후 현재+2e를 읽는다. 다르면 **두 번째로** 연결 수를 조회해+2e에 쓰고 앞의 이웃 스냅샷/owner 캐시/삭제/예약을 반복한다. 두 변경 분기는 독립이며 첫 예약 효과 뒤의 현재 캐시도 보존한다. 반환은0.1f다.
- 사건1은004578f0 이웃 재Pop 갱신, 사건2는00457460 재생성을 호출한 뒤0.0f를 반환한다. 하위 몸체 자체는 아직 미복원이며 실제 Session에서는 구체적 오류로 유지한다. 다른 사건은 실제 Damageable에 위임한다.

**보호bit20 없는 사건3은 아직 미복원이다.** 원본 명령은 스냅샷/삭제/new/Regular생성 후 명시적인 float load 없이 반환한다. Regular 생성자00496e80도 자기 double 저장용 fld/fstp만 수행하며 반환 float를 정의하지 않는다. 호출자 x87 상태가 결과에 관여하므로 임의0/-1이나 성공 처리로 바꾸지 않았다. 새 관찰에서도 이 경로를 정상 반환 표본으로 세지 않는다. 보호bit20의 사건3 위임은 별도 정상 표본에 포함한다.

## 실제 기존 fencemark 삭제

`RawFenceMarkerClear.*`는004566d0을 복원한다. 같은 `RawGunTarget::Center`로 다른 끝점과 부모 중심을 읽고, 원본 Rect의 incoming corner1..255 clamp/min/max와 lower 절삭·upper0.9999f ceil을 적용한다. 실제 일반 finder flags1로 현재 해시를 순회한다. 현재 TYPE가 fencemark이고 QA WORD가 부모SID이거나 `(QB DWORD<<8)|graphBYTE`가 부모SID일 때만 가상+10(flags40)으로 삭제한다. 후보를 미리 모으지 않고 같은 finder의 저장 next와 현재 태그를 사용한다.

실제 Session은 동일 Pool/hash/TYPE/중심/`world.Destroy`를 연결한다. 삭제 효과가 다음 후보의 태그나 현재 노드의 next를 바꾸는 입력도 원본/C++에서 대조했다. 전체 가상 삭제의 별도 하위 몸체를 이번 원본 관찰에서 실행한 것으로 세지 않는다.

## 내부 표식90과 포스 필드170의 정정

원본 `005411c0=90`은 **fencemark**, `00541300=170`은 **fenceShield(Force Field/포스 필드)**다. 기존 Session은 두 번째 값을90으로 연결하는 복원 오류가 있었다. 현재는 `fenceNodeType=90`을 태그 삭제에, `fenceShieldType=170`을 기존 RawFenceLine의 mode0에 각각 연결한다. 기존 선 삭제 검사의 실제 후보도170으로 정정하고, QA가 같아도 별도90을 포스 필드로 삭제하지 않는지 확인한다. 과거99개 검색점 fixture/보고서는 보존한다. [명칭 대응표](../unit-source-name-map.md)에도 추가했다.

## 독립 관찰과 검증의 범위

`tools/decomp_fence_regular.py`는 실제 Handle·TYPE/방향 getter의 현재 정밀 범위를 실행했다. **1372고유 입력**(사건0 1305·사건1/2 위임18·bit20/기본 Damageable위임49)을 두 x87 정밀도에서 **2744정상 반환**으로 관찰한다. 이웃·선/mark삭제·연결수·각 할당/예약·프레임·Damageable 등 큰 하위는 명시 외부 입력이다. 전체 다섯 슬롯250BYTE/반환 float/모든 호출 순서를 C++3개에서 전수 대조했고 누락/부분 쓰기 검사1개도 통과했다. 첫 연결 조회가 캐시를 바꾸는 입력으로 C++ 피연산자 평가 순서에도 의존하지 않게 했다.

`tools/decomp_fence_marker_clear.py`는 기존 표식 선택/중심/Rect/CRT/**실제 일반 finder**/태그 getter를 실행한다. 수평/수직/대각/역방향×QA3비트×packed3비트와 삭제 뒤 변경3개로 **259고유 입력/518정상 반환**이다. 전체 여섯 슬롯300BYTE·실제 Begin 영역·삭제 인수/순서를 C++에서 비교한다. 이 관찰의 외부 경계는 가상 삭제 하위 효과다. 실제1×1 발자국/중앙 좌표 입력이며 전체 보드 경계/모든 발자국 크기의 전수 증거가 아니다.

새 고유 입력은 합계 **1631개**이며 두 정밀도 반복을 입력 수에 더하지 않는다. 펜스 관련 집중 **17개 모두 통과**한다. 실제 연속 Kernel 검사는 Tutorial1~5/캠페인1-1/1-2의7개가400프레임을 통과하고, Tutorial6/GuardMyBack은 사건0·기존 표식 삭제를 지나 **사건2의00457460 재생성**에서 멈춘다. 데모1의 rainFlyer Pop, 데모3의 다른 총 animation 실패도 유지한다. 성공 예외/건너뜀/임시 객체로 우회하지 않는다.

전체 GUI·Guide/미션 프레임·AI전략·실제 사용자 명령·공격/승패·저장/다음 미션·그래픽/음향 동일성은 후속이다. cpppj2차 목표의 DirectDraw 오류 수정 필수 항목은 유지하며 현재1차 미션 복원에서만 제외한다. 로그는 `extracted/goal-fence-regular-20261016-{native,build,final-build,focused,focused-final,frames,frames-final,ctest}.log`, 실제 표식 관찰은 `extracted/goal-fence-marker-clear-20261016-native.log`, 출처/정상 반환 감사는 `cpppj/fence-regular.json`·`fence-marker-clear.json`이다.

최종 Release 경고0·오류0, 전체 CTest **1362개 중1358통과·4실패**, 검사117.22초/전체117.28초다. 새6개와 기존 검사에서 회귀는 없고 실패4개는 위 실제 미션의 남은 경계다. 새1631/요구에너지2352 SHA·Python구문·UTF-8·새 주석·로컬링크·공백을 감사했다. 과거 buried 보고서의 Session/CMake3파일 및 fence-line 보고서의 변경 검사1파일 SHA는 그대로 보존한다. 해당 차이와 현재 연결/전체검사 결과를 새 fence-regular 보고서에 별도로 기록했고 원본 fixture/과거 SHA를 덮어쓰지 않았다.
