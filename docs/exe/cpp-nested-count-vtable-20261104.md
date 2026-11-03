# 제단의 중첩 현재 가상80과 Session 소비자 공유

단위838 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. 직전837은 `09b6d969 / 20261104-19 CP`다.

PATCH0044a480/CD00483670은 같은 위치의 typed Dais를 한 번 찾고, SID가1..capacity-1이면 그 대상의 현재 VT+80으로 위임한다. owner/free/dead·생성자 쌍 필터가 없다. 기존 RawAltarSacrifice::Count의 temple_.Count 직접 호출을 선택 callback으로 바꿨다. 실제 Session은 항상 공통 현재 VT 분배기를 연결한다. 독립 fixture의 미연결 기본값은 실제 Dais VT일 때만 기존 Temple 몸체를 허용하며, 다른 VT는 명시 실패한다.

Impl::VirtualCount는 실제 지원83VT를 공통81/제단/다이스로 분배한다. RewardCount·Fort 저장·객체 정보/회수/변형·CoreRefresh·초기 runtime 및 이름/Factory 입력/업그레이드·AI·Factory 메뉴의 확인된 가상80 hook을 같은 몸체에 연결했다. FortSave의 명시 Has 공통 호출은 유지했다. Factory 전용 두 VT의 기존 caller guard도 유지한다. 초기 설치는 loaded가 아니라 선택 모듈 설치 여부로 검사한다.

활성 SID 집합은 Impl의 첫 멤버로 만들어 World/서비스보다 나중에 반납한다. 중첩 Count에서 같은 SID를 다시 밟으면 원문의 무한재귀를 호스트 오류로 거부한다. 성공적으로 등록한 호출만 RAII로 자기 표식을 지우므로 예외 뒤 정상 조회도 가능하다. callback은 raw this만 빌려 새 소유권 순환을 만들지 않으며 기존 Kernel 정리 동안 모듈이 살아 있는 수명 순서를 유지한다.

새 실제 Session/UI 검사2개는 TYPE165·프레임0·state레벨7을 유지하고 대상 VT만 base/Dais로 바꾸어 제단 Summary가 Level VII/I를 선택하는지 확인했다. 같은 대상의 VT를 Altar로 바꾸면 같은 SID 재귀 오류가 나며 두 객체 전체 바이트/기존 UI가 변하지 않고 복원 뒤 메뉴를 다시 열 수 있다. 예외는 완성된 메뉴의 builder callback 밖에서 선택했다. Pop/Frame/의식/포획/희생/전투 함수는 실행하지 않았다.

새2개와 기존 직접가상80/회수/Form/현재World 회귀를 합해 **집중8개 첫 실행 실패0**, Release1회 종료0·경고/오류0이다. 새 독립CPU/fixture는0이다. 명시 소비자의 원문 VT+80 연결은 정적으로 대조했으며 모든 소비자별 실제 실행을 검증한 것으로 확대하지 않는다.

**남은 경계:** RawSquidLevel.Has의 AllocatedBytes/free 거부와 RawTempleRitual.Count의 TYPE165 강제는839에서 읽기 정의역을 대조한다. setter 실제 쓰기·프레임/배열 밖·미등록 VT는 별도 호스트 경계다. Temple.Stage 등 내부 직접 Count까지 전체 완료로 세지 않는다. checking14=부모8+부분3+MP3·완료부모34/부분3은 유지한다.

제품838/11.01 EXE SHA-256은 `0e3ecf932974749ea930af4a457385100e977baf4e2d86249a37e0727383d139`다. 보호2759개/fixture939개 변경0이다. 원시는 `extracted/cpp-resume-838/`, 상세 해시는 [838 JSON](../../cpppj/nested-count-vtable-20261104.json)에 있다. 실제 게임/GUI·물리/PCM/CD삽입·포획/희생/특정 적 공격/승리·신규MP/통신·공중3종 전용 검사는0이며 전체71/CTest/34미션/ZIP을 반복하지 않았다. push/DN/WB 변경0이다.
