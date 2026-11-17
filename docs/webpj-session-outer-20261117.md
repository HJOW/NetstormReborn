# 실제 외곽 Session 생성·공개 (WB04-E638)

기준선 `225553df`의 `RawCampaignSession.cpp` 4318..4330·4333..4335,
4158..4160·4822·6381..6382의 **22줄**을 연결했다. 해당 `.cpp/.h`는
기준선 뒤 변경 0이다. World/Core 두 조회는 실제 외곽 검사가 필요해
4D-6c에서 앞당겼고, Detach·pin은 각각 4D-3b·4D-5a 구간에서 앞당겼다.
기존에 이식한 Impl 모듈들을 같은 순서로 실제 생성하고, 최초 Template·
WorldRuntime·승패/AI 부품 뒤 외곽 callback을 연결한 다음 마지막에
`gameplayUse.publish(this)`를 실행한다.

같은 외곽 주소가 기술 획득·Vortex snapshot·현재 CoreData·세계 간헐천·
너겟 배치·로컬 의식 갱신의 대상이다. 로컬 의식은 현재 Core→현재 owner의
CoreData→두 독립 snapshot이며 실제 의식은 실행하지 않았다. 생성 자체로
Load를 호출하거나 loaded를 켜지 않는다. Client 앱에서 이 외곽을 소유하는
단계는4D-6·4D-7이다. PORT_MAP session 상태는 여전히 0이다.

소멸자 앞단은 World/건설 pin을 먼저 닫고, Login이 없어도 같은 서비스가
보관한 전송 대여를 detach한 뒤 실제 Impl을 반납한다. 실제 발급한 Runtime
pin으로 배송 중인 외곽은 수동 dispose도 거부한다. 전송 함수 표는 C++
optional과 같이 복사하며 공유 owner 참조를 유지한다. State 자식 반납
증빙/Withdraw 선택은4D-5a에 남겼다. 현행 정상 반납은 기존 Impl owner의
최종 대여 해제를 사용하며 이를 전체 State 소멸자 완료로 세지 않는다.

새 4검사는 실제 외곽을 구성한다. 동일 Client 전역에 두 세대의 Session을
순서대로 게시/반납하고, 다른 Subscriber·생성 중 Fort reader 실패 뒤
대여 회수와 재생성을 확인한다. 실제 World의 Form152 Take가 외곽 pin을
발급하고 정상/예외 복귀 모두 호출 수를 반납하는지 확인한다. 배송 중
dispose는 현재 Session을 유지하며 거부하고, 배송이 끝난 뒤 한 번만
detach한다. 모든 검사에서 loaded는 false다. 생성 후 정상 미션 Load와
실제 미션 snapshot/AI 통합은4D-2h·4D-10에 남는다.

최초4검사는2통과/2실패였다. Core aggregate 자체의 참조 동일성은 원문
값 반환과 맞지 않아 실제 Pool/장부 참조 비교로 고쳤고, 공개 Core getter도
6382의 값형 aggregate/같은 물리 참조로 교정했다. 또한 Player 할당 뒤
Fort reader가 실패한 경우에도 장기 Player base가0이 된다는 기대는
ClientPlayerRecords의 반납 계약과 달랐다. 할당 전 다른 Subscriber 거부는
base0, 할당 뒤 실패는 같은 장기 base를 보존하고 대여만 회수하는 것으로
구분했다. 관련16검사(외곽4·callback6·pin6)가 모두 통과했다.

구간 지도는 위22줄만 새 완료로 센다. 이전4D-1 배정에서 남은18줄 예산은
Load/State 반납 경계의 재대조 때 다시 집계하며, 검증 선행 대기인1b·1c와
별도로 다음 구현은4D-2a다. 생성자·소멸자 앞단1d 체크만 완료했다.
원본 fixture는 기존 의식 갱신 직접 소비 시험을 유지하고 관련 외곽 모듈만
manifest에 연결했다. 새 fixture/직접 소비자 등록은0이다. 보호 원본·cpppj·
공통 인계·README·AGENTS.md를 수정하거나 게임/브라우저 창을 실행하지 않았다.

최종 Node v22.23.3·v26.13.0 전체가 각각 **6,027건 통과**, 실패·취소·
건너뜀0이다. v24는 사용자 지시대로 생략하고 호환을 가정했다.
대응표1,160행 오류0·manifest823항목(신규0)·`git diff --check`를 확인했다.
최종 로그는 Git 제외 루트의`extracted-webpj-e638-node22.log`·
`extracted-webpj-e638-node26.log`다.
