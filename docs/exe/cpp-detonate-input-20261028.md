# Detonate의 원본 입력 종료와 클론 예외 경로 수정 — 단위521

2026-10-28, HJOW-Athlon. 작업 전 AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 확인했다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 이번에는 산출물을 변경하지 않았다. 앞선520은 커밋 `f8d6da53`으로 완료된 진행이다.

## 원본에서 확인한 동작

Detonate는 성장형 아이스 타워48eca0의 항목이 아니라, 주문16VT가 공유하는420b30 메뉴의 항목이다. command는5428c8의 `03000074`이며 정적 이름은 `DetonateBomb`이다. 메뉴는special/Editor가 모두0인 자기 소유 주문에 표시되고, event0 예약이 있으면 비활성화된다. 다른 모드에서는 같은 위치에 Turn To Obelisk를 작성한다.

**10.78에서 이 입력은5c 시전 요청을 보내지 않는다.** 일반 입력은release를 먼저 거르고, y의낮은WORD가 비영이면 현재 대상의 가상6c를 호출한다. 실제16VT의41e400은Turn To Obelisk만 처리한다. Detonate는4ae010→4ac9b0의 부모로 진행해0을 반환한다. 일반 입력은 이0을 무시하고 다음 사건으로 진행한다. 별도의 시전/피해/삭제를 추가하면 확인한 원본과 달라진다. 투하/주문 활성화의 별도5c 수신과 이 메뉴 입력을 합치지 않았다.

원본 일반 입력의Guide/Gump 소비 뒤 **4d6eca부터 다음 사건4da477까지**와 실제 세 가상 부모·AI 키·strchr·커서 활성 판정을 에뮬레이션했다. 원문768조건은16VT×누름/뗌/Ctrl/Shift×y의0/50/상위WORD 포함/낮은WORD0×세 모드다. 정상 사건 끝·원래ESP·전체raw50BYTE를 확인했다. 월드/전역 쓰기는 검사 훅에서 거부하며, 미허용 송신 함수에 진입하면 실패한다. 메뉴 없음460de0 조회만 명시 경계로0을 공급했다. 선택한 분기의 반복 검증도768조건 차이0, 추가 진단 감사도768조건/단정 진단0이다.

이 증거는PE의 초기 명령ID·정적 숫자 quick-set 문자열·비활성 커서 조건이다. Guide/Gump가 먼저 소비하는 경우,10.72, 임의로 변경한 전역/모든 입력 상태까지 확정한 기록은 아니다.

## 수정

`DetonateInput`이 원문release/낮은WORD0 관문과 실제 객체 입력 호출을 유지한다. 객체 부모가0을 돌려줘도 해당 일반 명령은 처리된 사건으로 반환한다. `UberGump::Event`에서 미연결 파생 명령 예외에 도달하기 전에 이 경로를 실행한다. 현재 선택·owner·메뉴 체인·표시 조건을 새 실행 조건으로 추가하지 않았다. 유효한 대상의 실제 하위가 없으면 무동작 성공으로 채우지 않고 오류를 유지한다.

수정 전 소스는Detonate를CampaignObjectMenuClient의 전달 목록에 넣지 않고, 후속 IsObjectMenuVariantCommand 진단에서 미연결 예외를 발생시키는 경로였다. 이번에는 별도 창에서 그 예외 자체를 재현했다는 주장을 하지 않는다.

## 검증과 한계

Release 빌드 경고/오류0. 등록3,352개 중 관련37개/실패0을 확인했다. DetonateInput3개·ObjectMenu20·ObjectCommand3·ScriptCommand3·ObjectConversion2·DialogCommand3·MissionGuideInput3이며 전체 검사는 반복하지 않았다.

- 독립 기계어768조건의 가상 호출/0반환·raw를 실제 RawObjectConversion/RawObjectCommand 부모와 비교했다. 누락 하위·알 수 없는 명령·release·y의낮은WORD도 별도 확인했다.
- 보호 자산의 실제Session에114 주문을 명시 생성하고, 실제Gump 메뉴 클릭→InputQueue→새 일반 입력→Session.CommandObject를 연결했다. Detonate 토큰과대상SID·부모0반환을 확인했고, 요청0·raw/돈 보존·event0 예약 부재를 확인했다. 이는 단위 장면이며 사용자가 자연 플레이로 주문을 얻은 증거나 별도 창 전체 경로 검증이 아니다.
- 별도 창 탐색2회는 모두 수락에서 제외했다. 첫 회는 브리핑의 실제 Play Mission 대신Go!를 가정했다. 두 번째는 Test Battle 전환 후 주문이 존재하지만frame0인 상태에서 world:bombExplodeMedium 좌표 장치를 찾지 못했다. 두 실행은종료1이며 실제Detonate 클릭까지 도달하지 않았다. 새 코드의 별도 창 수락 사례는 없다. 원시 로그와 시작PID/실행SHA는 `extracted/cpp-detonate-521`에 보존한다.

## 다음 대조

**단위522 정정:** raw_session의 두 번째 열은PlayerKnown이며 Editor가 아니다. 앞선 Editor1 판정은 보고 열 오독이었다. 실제 실행 원시는 보존했다. 저장 주문의frame0/선택 가능 여부를 원본 상태 전환·저장/기본 프레임·선택기와 대조해야 한다. 오류로 확정하거나 강제로 상태를0/프레임1로 바꾼 것은 아니다. 기존517의재적재 Spell frame2 관측과도 구별한다. 이 부분을 실제Detonate 클릭/폭발 성공으로 세지 않는다.

DigUp special 실제 입력, 일반 건물형 전체·신전/제단/워크샾의 소유자/레벨/포획 상태별 메뉴와 남은 편집 도구를 계속 점검한다. 공중3종은 새 생산/생성/우클릭 재현이나 전용 검사 없이 정적 제한을 유지했다. 연녹색 직사각형 분석·자연 완주/영문 부재 개선 보류도 유지한다. 원본/CD·AGENTS.md·기존fixture는 변경하지 않았고 원격push는 하지 않는다.
