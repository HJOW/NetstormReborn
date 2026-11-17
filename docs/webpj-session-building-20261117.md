# Session 건설 설치·캡처 (WB04-E639)

기준선 `225553df`의 `RawCampaignSession.cpp` 3668..3771·3772..3776의
109줄과 캡처에 필요한 공통 `DrawTempleFrames` 3659..3667의9줄, 합계
**118줄**을 옮겼다. 해당 `.cpp/.h`는 기준선 뒤 변경0이다. 공통 신전
그림9줄은4D-4b에서 현재 단계 선행으로 앞당겼다. 기존 계획4D-2a에
섞여 있던 ConstructionArrival/ManaBolt Arrival/Regular는 실제 C++에서
3788·3796·3805, 즉 `ConnectMotion`에 있으므로4D-2b로 정확히 옮겼다.

같은 실제 owner·World/Pool·ProcessHost와 현재 HP/음향을 빌린다. 일반/
전용 색표 토큰과 실제 Client 공용 ownerTables를 구별한다. 특별 색표는
같은 현재 primary를 한 번 선택해 계산 전체에서 보관한다. primary가
명시되었으나 연결되지 않은 경우 COL 팔레트 성공으로 바꾸지 않는다.
건설/연소 캡처는 실제 ShapeToBuffer/SHP·Factory/다이스/신전/간헐천/
발사기 전용 추가 그림과 원본 오류 경계를 사용한다. 기존 파편의 일반
건물 자격 함수를 공유 export로 바꾸었으며 TYPE/VT 집합은 유지했다.

RawCheckingFlag는 기존 `captureCampaignFlag`로 당시의 pointer/지역
값을 복사한다. 원래 struct에 나중에 다른 값을 대입하더라도 이미 복사한
pointer는 바뀌지 않는다. 간헐천 수신은 각 진입 위치에서 그 당시 플래그를
다시 복사하고 기존 handler에 같은 packet을 넘긴다.

새8검사는 같은 실제 생성자 부품을 구성한 owner에 설치한다. 별도 근사
월드를 만들거나 실제 owner의 loaded를 켜지 않는다. 외부 화면/음향/
선택 입력만 C++ headless fixture처럼 공급한다. 설치 전/반복/반납·같은
Pool/Host·일반 SHP 전체 픽셀·공용 색표 참조·primary1회 선택·지연 캐시·
플래그 복사·현재 packet·다이스 전용169 그림·신전123/124/125의 그림/
레인 조건부 음향을 확인했다. 신전은 프레임 자료 입력만 읽었고 의식이나
게임 내 상승을 실행하지 않았다. 공중3종 전용 생성/행동 검사는 추가하지 않았다.

명시 abstract bit1 부모를 실제 같은 World에 Pop한 뒤 건설30/진행음47을
붙였다. 시작12.5/끝22.5에서 실제 ConstructionProcess를 수동 실행해
17.5의HP350/700과22.5의HP700/700, Form 삭제·음향 삭제·abstract 해제/
재등록·Host 최종 정리를 확인했다. 별도17BYTE packet의 end 부분 수신도
같은 기존 Form/PID의 객체만 바꾸었다. 전체 미션 적재/Kernel 프레임/
사용자 건설 입력/Client 앱 플레이를 검증한 것은 아니다. 정상 Load와
빌더 이동/도착 전체 검사는4D-2h·4D-10·6단계에서 재대조한다.

최초 부품 probe는 미배치 부모로 시작해 원문 부모 유효성 검사를 받았다.
같은 World의 Pop과 실제 건설 abstract bit1 입력을 사용하도록 고쳤다.
색상 abstract bit16과 건설 bit1, HP+26과 상태WORD+12를 구별했다.
contained 조회의 네 번째 인자가 삭제 선택임도 확인하고 관측 조회는
remove=false로 고쳤다. 실제 ProcessForm 삭제는 명시 선택 UI 입력이
필요해 C++ fixture와 같이 현재 선택0을 공급했다. 기존 오류를 빈 성공으로
대체하지 않았다. 신전 타입을 잘못 추정한 probe는 가상표 거부를 관측했고
실제 자산 이름으로123/124/125를 확인했다.

최초 정식7검사는6통과/1실패였다. C++의 `definition.specialFrames.baseFrame`
이 JS에서는 평탄한 `definition.baseFrame`이므로 실제 결선을 교정했다.
다음 실행에서는 windBattery75의 baseFrame=-1을 직접 그리려던 검사 입력이
실패했다. 이 타입의 extra2는 기본 그림 생략+빈 추가 그림이 맞으므로,
일반 현재 프레임의 비투명 전체 픽셀과 extra2의 전체 투명을 따로 확인했다.
정적 검토에서는 primary를 항목마다 재조회하던 코드를 한 번 선택하도록
고쳤고 실제 getter 호출1회 검사를 넣었다. 마지막 신전 자료 검사까지
신규8건·관련 집중37건이 통과했다. 보호 관측/기대값은 수정하지 않았다.

기존13개 직접 원본 소비 항목의 관련 모듈/범위만 갱신했다. 새 fixture/
직접 소비자 등록은0이며 원본 소비 검사는 그대로 유지한다. 공통 인계·
cpppj·원본/CD·README·AGENTS.md는 수정하지 않았고 게임/브라우저 창을
실행하지 않았다. 다음은4D-2b ConnectMotion 앞부분이다.

최종 Node v22.23.3·v26.13.0 전체가 각각 **6,035건 통과**, 실패·취소·
건너뜀0이다. v24는 사용자 지시대로 생략하고 호환을 가정했다.
대응표1,160행 오류0·manifest823항목(신규0)·`git diff --check`를 확인했다.
최종 로그는 Git 제외 루트의`extracted-webpj-e639-node22.log`·
`extracted-webpj-e639-node26.log`다.
