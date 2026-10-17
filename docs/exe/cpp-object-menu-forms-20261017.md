# cpppj 단위140 — 같은 Session의 객체 Form과 실제 메뉴 수명

기준은 `ed49269`이며 현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 디컴파일 날짜는 **2026-10-16**이다. 새 디컴파일·원본 게임 실행·원본 파일 수정은 없다. 전체화면 DirectDraw 수정은 제외한다.

## 실제 연결

Session이 기존 **GumpFormHost**를 같은 Pool/Factory/RawSquidDestroy에 한 번 구성하고, 같은 SquidFormDispatch에 Form3 경로를 등록한다. 기존 DependForm/ContentForm/ProcessForm과 섞인 종속 체인의 삭제는 같은 분배기를 사용한다. `GumpForms()`는 설치 전에 거부한다. 부모 삭제 알림의 UI 소비자가 없으면 실제 Notify 시점에서 명시 오류를 유지한다.

RawButtonControls의 `ObjectNotice()`는 기존 MakeGumpFormNotice로 **알림 당시의 등록 ID**를 해석하고 ReleaseGumpObject의 기본 가상+4를 통해 Kill만 요청한다. UI 노드를 즉시 반납하지 않는다. Kill의 기존 deleteForm은 현재 Client/Session 소비자가 같은 Form3을 공통 삭제로 반납한다. 부모 삭제 중 같은 Form이 다시 삭제되는 경우도 기존 dead/깊이/next 보존 규칙을 재사용한다.

RawMenuControls의 현재 호스트 getter는 각 Form 생성·소유 ID 쓰기·객체 부착마다 기존 BindMenuGumpForms를 다시 연결한다. 별도 UI 풀이나 게임 SID/등록 ID 변환표를 만들지 않았다. 기존 MenuGump의 객체부착 경계475cf0→현재 메뉴 가상+3c→MenuGumpWidgets.AttachObject가 이제 실제 Form(type3,flags2) 생성, 소유 등록 ID, 부모 체인, UI object/Form SID 필드, 실제 부착까지 도달한다.

`OpenAttached`는 caller가 공통 Open/위치/475cf0으로 이미 부착한 루트를 기록한다. controlled 스택이0인지와 루트 부착을 확인하며, 메뉴를 마우스 점에 다시 부착하지 않는다. 거부 결과는 루트0으로 표현한다. 이 입구가 자체 제목이나 파생 메뉴를 대신 작성하지 않는다.

실제 Client 서비스는 같은 UI의 ObjectNotice를 Session에 공급한다. Form 효과는 매번 현재 world를 조회한다. ClearWorld는 기존 world/Session이 살아 있는 동안 실제 팝업과 Form을 먼저 닫고 나서 world 포인터/Kernel 소유권을 해제한다. 이전 Session의 호스트 포인터를 UI가 캐시하지 않는다.

## 검증

새 집중 검사2개를 포함한 `CampaignObjectMenus_` **6개가 모두 통과**했다.

- 자연 설치한 사제에서 실제 공통 Open→기존 사제+a8 작성→같은 Pool의 Form3 부착을 실행했다. 새 Form의 현재 부모/등록 ID/기존 종속 머리를 확인한다. 실제 About 행을 원본 Gump 입력으로 클릭해 **0x02000017 정수 명령 큐**까지 진행한 뒤 메뉴를 닫는다. Form 반납, 부모 머리 및 기존 첫 종속 슬롯의 **전체 원문 복구**를 확인했다. 제목은 명시 검사 입력이고 실제4ada70·4ad780이나 Client 도움말 명령 본문을 실행한 검사는 아니다.
- 명시적으로 생성한 실제 Form3 부모에 메뉴를 붙이고 **실제 world.Destroy(parent)**를 실행했다. 자식 Form 알림→UI Kill→중첩 Form 삭제→SID 반납 뒤에도 UI 노드/등록은 남고, 이후 Cleanup에서만 해제되는 것을 확인했다. 부모는 명시 Form3이며 일반 자산의 모든 파생 삭제를 검증한 것으로 확대하지 않는다.

기존 RawMenuControls4·RawButtonCursorBoundary4·ObjectMenuOpen3·FormTake4·FormLifecycle4·GumpLifetime9를 합친 **28개 추가 집중 검사도 통과**했다. 처음 지정한 GumpForm 필터는 일치 검사0으로 실패해 올바른 FormTake/FormLifecycle 필터로 실행했으며, 0개 실행을 성공으로 세지 않았다.

Release 빌드1·2·3은 모두 종료0·경고0·오류0이다. 빌드2는 현재 SHA를 위한 LF 고정 뒤, 빌드3은 About 실제 입력/원문 복구 검사 추가 뒤 실행했다. 최신 실행 파일의 클론 진입 회귀 **5회·실패0**, `originals_unchanged=true`다. 캠페인1-1·1-2는 브리핑→Play Mission→월드, tutorial1·2·3은 브리핑 범위이며 사용자 자연 완주나 새 객체 메뉴의 자연 진입 검증이 아니다. 로그 접두부는 `extracted/goal-object-menu-forms-20261017-`다.

최종 **전체 CTest는1,571개·실패0·종료0·213.23초**다. 원본 근거 감사58종은 **47종 성공·기존11종 실패·새 실패0**이며 기존11종은 단위138 기록을 따른다. 현재 제품을 가리키던 **기존22보고서·54개 핀**만 갱신하고 각 integration_updates에 과거값을 보존했다. 기존 GumpFormHost/GumpFormBinding/메뉴 가상 부착/GumpLifetime의 원본 몸체 및 기대 fixture를 수정하지 않았다. 새 CPU 기대값을 생성하지 않았다.

보호 실행 파일2개와 tarc의 SHA3개를 확인했고, 새 연결 코드/보고서의 UTF-8와 LF를 고정했다. AGENTS/원본/DN과 기존 fixture 및 CMake는 변경하지 않았다. 연결 보고서는 `cpppj/campaign-object-menu-forms.json`이다.

## 다음 연결

UberGump의 임시 우클릭 메뉴는 아직 교체하지 않았다. 실제 제목4ada70·4ad780의 type/count/owner/sid4인자, 현재 VT별 파생+a8/+ac, 실제 Client DWORD 명령 소비자와 시간을 정지하지 않는 원본 우클릭 동작을 이어야 한다. 생산/경제/AI/로그인된 로컬 전송과 대상11스테이지 자연 완주·승패·저장·다음 미션·전체 동일성은 미완료다.
