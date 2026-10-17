# cpppj 실제 캠페인 커서 수집과 표시 연결 — 2026-10-17

작업 PC는 **HJOW-Athlon**, 마지막 정밀 디컴파일 PC와 날짜는 **HJOW-Athlon / 2026-10-16**이다. 기준 커밋은 `4e00fc1`이다. 이번 단위135는 새 디컴파일이나 게임/GUI 실행 없이 기존 정밀 C와 보호 PE의 격리 CPU 관측으로 진행했다. 전체 목표는 계속 진행 중이며, 이번 연결만으로 사용자 자연 플레이를 완료했다고 평가하지 않는다.

## 실제 상태와 렌더러

`RawCampaignCursor`가 `RawCampaignCursorDisplay`를 소유한다. 표시는 같은 현재 CursorDrop, Lifecycle, Placement, Bridge, 격자, Pool, TYPE, spot, SP, PlayerKnown, 시각을 읽는다. PreviewCollection의 피드백은 수집 시마다 실제 SP와 난독화 오프셋을 다시 읽는다. 카메라 원점은 같은 `SetParticleView`에서 갱신한다.

Renderer는 각 dirty마다 원본 PreviewCollection을 한 번 호출하고 실제 body/aux 항목을 월드의 원본 제출 순서에 합쳐 같은 정렬기에 넣는다. dirtyFlags bit2(값4)의 UI 전용 수집은 커서를 추가하지 않는다. `SceneImage`는 마지막 항목 캐시를 사용해 피드백을 다시 실행하지 않는다. 개별 항목의 bounds는 전체 viewport를 유지해 마지막 작은 dirty가 전체 스냅샷을 잘라내지 않게 한다. 월드 반환 전 추가 수집 callback을 해제한다.

PreviewDraw는 원본 SHP, TYPE hotspot/footprint, 원본 깊이와 순서, 현재 소유자 색, 합법/불법 윤곽과 변환, 격자 및 글자를 사용한다. 비용/경고는 같은 실제 FontStore와 GumpTextDraw에 넘기며 현재 언어를 원본 좁은 바이트로 인코딩한다. 실제 취득의 문맥 word2는 글꼴 번호가 아니라 색이다. 경고 흰색/비용 노란색을 실제 팔레트에서 얻는다. labelFrames는 BSS0과 `wrtsWRTS.?>`의 11개 원본 프레임 순서다. 원본 설정 키 `theammode`의 누락 기본값은 1이며 94/97 색표는 모드0에서만 활성화된다.

## 원본 색표 관측

`tools/decomp_preview_colors.py`는 실제 전체 ColorSort `439de0`와 팔레트 생성 `43c010`을 3개 입력과 53/64비트 정밀도로 실행했다. Destroy/Illegal/LabelShadow 각 256바이트는 3행, 정상 반환12, 정밀도 간 차이0이다. 불법 색은 빨강 성분만 2.1배 한 뒤 1로 제한하며 최근접 비교의 float 최소값 저장을 보존한다. Destroy는 입력 빨강의 하위 BYTE, 그림자는 전부0이다.

`tools/decomp_owner_hsl_colors.py`는 원본 `43b5a0`/`43b750`의 전체 전용 변환을 111/141/137/94/97에 적용했다. 3팔레트×5종의 15행, 정상 반환763이며 53/64비트 결과를 각각 보존했다. 항등 9×256표와 할당 주소는 명시 입력이다. 부모 등록/해제 루틴 `43be30`/`43bc40` 전체를 새로 실행한 것은 아니다.

기본 정밀도는 CRT `4e81da`→`4f2a7b`→`4f2a49`와 실제 변환 함수 `4f2929`/`4f29bb`를 별도로 실행해 확인했다. 실제 반환, 스택, 보존 GP, 빈 x87 스택 및 정밀도53/최근접 반올림을 검사했다. Unicorn rawCW `0x23f`는 예약 bit6이 없어 canonical `0x27f`와 같다고 주장하지 않는다. 실제 팔레트의 97 한 행은 64비트 결과가 달랐고 이를 지우지 않았다. C++ 검사는 원본 기본53비트 출력을 기대값으로 삼는다. 두 보고서의 `instructions`는 실행기에서 호출마다 초기화하므로 **마지막 호출의 명령 수**이며 전체 합계가 아니다.

C++는 기존 일반 12종과 새 전용 5종을 함께 공급한다. owner0 항등과 원본 후보 색 목록, 원본 HLS 명도/편차, 첫 최소와 엄격 비교 순서를 보존한다. 원본 데이터 외 임의 색상 치환은 없다.

10.72 대조 주소는 ColorSort `458a90`, 팔레트 생성 `496530`, 전용 변환 `495ed0`/`495e10`, CRT 기본 정밀도 `4f4d10`, 수집/그리기 `43da30`/`43dbd0`이다. 이번 새 CPU 관측과 조립 검사는 10.78 전용이고 기존 10.72 fixture는 수정하지 않았다.

## 검증과 남은 범위

최종 Release 빌드3은 종료0/경고0/오류0이다. 집중13개(커서10/색표2/Renderer1)가 통과했고 새 검사는5개다. 전체 **1,547개/실패0**, CTest종료0/240.51초(전체240.54초)다. 원본 근거41종도 통과했다. 스테이징63파일/제품20개/기존33보고서의현재91핀 갱신/과거144핀 보존/보호3파일 SHA/UTF-8·LF·기존CMake접두·기존fixture불변을 감사했다. 로그 접두부는 `extracted/goal-cursor-display-20261017-`이며 새 원본 근거는 `cpppj/preview-colors.json`, `cpppj/owner-hsl-colors.json`과 두 새 fixture에 있다.

통합 검사의 준비75/GUI 반환8 Form/완성76 공급원은 기존 단위134의 명시 입력이다. 실제 생산 완료나 메뉴 Open의 제공 행을 대신 검증하지 않는다. 실제 Take/현재 카메라/원본 SHP/합법 윤곽/불법 경고/부분 clip/현재 SP 피드백/캐시/Cancel을 검증한다.

현재 Client의 일반 마우스 입력은 아직 임시 선택/이동 경로다. 실제 커서 명령 입력과 생산 Gump/금액/dirty, 부모 Gump 깊이의 전체 소비자, 실제 로그인된 로컬 전송 큐, 경제/AI/전체 Kernel, Early Missions1~6·캠페인1-1~1-2·데모3의 자연 완주/승패/저장/다음 미션 및 전체 동일성은 남아 있다. 전체화면 DirectDraw 수정은 요청대로 제외한다.
