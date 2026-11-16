# World의 전체 Runtime 호출 수명 (WB04-E626)

기준선 `225553df`의 `RawSurfaceWorld.cpp:347..370`, `SquidPostPop`의
세 공개 후처리, `RawSquidContainedUnpop`의 getter/분리/UI 통지와
Factory·Process의 기존 대여 진입에 실제 전체 owner 보유를 연결했다.
대상 C++ 소스의 기준선 뒤 변경은 0이다. 착수 때 작업 폴더에 있던
미커밋 초안을 대조·교정해 이번 단위로 마쳤다.

## 구현과 검증 범위

- 같은 World가 설치한 Factory/Display/Pop/Process 대여에 pin 함수를
  전달한다. 현재 다른 World의 대여를 다시 찾아 수정하지 않는다.
  독립 Kernel의 Host와 나중에 설치하는 종속 Unpop에도 전달한다.
- 공통/가상 PostPop과 독립 Activate는 원문의 공개 진입 첫 효과부터
  전체 owner를 보유한다. Activate의 사전 검사·깊이 쓰기도 그 안이다.
  getter·Detach·NotifyUi·Unpop의 중첩은 각각 원문 범위를 유지한다.
- JS에는 C++ 지역 shared_ptr의 자동 소멸이 없어 `runtime-pin.js`가
  실제 GameUse 발급 핸들만 식별하고 finally에서 멱등 반납한다.
  기존 pin이 돌려주던 일반 강한 owner의 임의 release 메서드는 부르지
  않는다. 호출 중에는 현재 Runtime 반납을 거부하고, 정상/예외 끝에는
  호출 수가 0으로 돌아간다. World 종료는 먼저 pin을 분리한다.
- 미설치 Native 장기 묶음을 빈 구현으로 만들지 않았다. 이 단위는
  이미 설치한 실제 대여에 한정하며 전체 Session/Load를 완료하지 않는다.

신규 6건은 실제 ClientGameplayGlobals·GameUse·World·Factory·Host·
PostPop·종속 Form 링크로 정상/예외 반환, 중첩, 실행 중 반납 거부,
pin 실패의 선행 순서와 바이트/깊이 불변을 대조한다. 전용 원본 fixture나
C++ pin 전용 시험은 없어 원문 소유권 계약을 검사했으며, 기존 원본
후처리/공장/종속 분리 fixture의 소비 검사를 전체 검사로 재생한다.
manifest806항목은 유지하고 신규 fixture 소비자로 등록하지 않는다.

최초 신규 5건은 4통과/1실패였다. 검사 입력의 Create는 원문 pin을
선택하지 않으므로 실제 Take 경로로 고쳤다. 코드 대조 중 Activate의
pin 범위가 사전 검사보다 늦다는 점도 교정하고 실패 순서 검사를 더했다.
신규 6건은 모두 통과했다. Node v22.23.3·v26.13.0 전체는 각각
5,972건 통과, 실패·취소·건너뜀0이다. 로그는
`extracted/webpj-e626-node22.log`·`webpj-e626-node26.log`에 보존했다.
`port-map.mjs`는1,160행/오류0이며 `git diff --check`도 오류0이다.
v24는 사용자 지시에 따라 생략하고 호환을 가정한다.

## 잔여와 다음 작업

외곽 Session 생성자4318..4330의 실제 callback/공개 수명과 Load가
다음이다. Native 장기 실행기·건설 Runtime/Client 공개는 각 설치 구간에서
이어 연결한다. 앱 session0·실제 미션 플레이 불가 경계는 유지한다.
원본/cpppj 게임 창·자연 플레이·사제 포획/희생·승리와 공중3종 새 전용
생성/생산/행동 검사는 하지 않았다.
