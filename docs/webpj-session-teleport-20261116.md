# Session 현재 TYPE 생성자·Teleport 결선 (WB04-E585)

`225553df`의 `RawCampaignSession.cpp` 1463..1483행을 같은 기반 owner에
연결했다. `RawCampaignDevastation.inc` 10..23행 중 타이머가 도달하는 펜스
Rebuild 부모와 빈 타이머 장치도 선택 생성자에 필요해 앞당겼다. Session.cpp/.h·Devastation.inc·
RawGameAuxState.cpp/.h·RawBombEffectTimer.cpp/.h는 기준선 뒤 변경 0이다.
cpppj 수정·디컴파일·원본/클론 게임 실행은 하지 않았다.

## 구현·소유 관계

- 최초 flags1에서 현재 `StreakProcessType` DWORD를 읽고 물리 TYPE 범위를
  확인한 뒤 `constructorAddress`를 한 번 선택한다. 고정 객체134/process12로
  대신하지 않는다. 비최초 호출은 잘못된 현재 TYPE도 읽지 않는다.
- 마나/사정거리 cdecl은 같은 실제 Host와 각각의 현재 부착 TYPE을 사용한다.
  부모 SID와 지정 Form0을 전달한다. 실제 Form1개·표시8개가 부착된다.
- 빈 BombTimer는 실제 Pool/TYPE/현재 시계 getter와 Unpop/Repop/표시/펜스
  장치를 빌린다. 생성자는 Init·기한 설정·부모/Kernel 부착을 하지 않는다.
  반환값은 같은 owner의 `unstartedBombTimers`가 보관한다.
- Path cdecl은 호출 시 현재 `motion.paths`를 선택한다. 지원한 주소의 결과는
  `unstartedPaths`에 보관하고 Init/부착/행동은 실행하지 않는다. 실제 Motion
  생성 순서의 설치는 후속이며, 검사는 같은 owner 자료로 실제 몸체를 조립했다.
- 그다음 같은 Factory의 bare 생성자, 마지막으로 기존 입자/내장 Form/NULL
  생성자 분배기를 사용한다. 진단이 현재 TYPE/슬롯을 바꾸어도 선택한 함수를
  다시 읽지 않는다. 진단이 중단되면 앞선 쓰기와 장부 깊이를 보존한다.
- 연소는 같은 Client 바람 X·약화 HP bool·사제/제단 TYPE의 현재 getter를
  빌린다. 다른 owner로 재연결하면 거부한다. 펜스 타이머도 실제 장기
  `sunFenceType` DWORD(00541278, 초기136)를 사용하도록 GameAux에 추가했다.
  이 값은 보호 PE 초기값·현재 TYPE 이름 전수 검사로 대조했다.
- World 프로세스를 먼저 정리한 뒤 접두를 해제하고 미부착 Path의 가상
  소멸자를 실행한다. 두 미부착 결과 목록은 실제 모듈보다 먼저 반납한다.
  부분 결선/반복/반납 후 호출을 성공으로 바꾸지 않는다.

JS 엔진의 치명적 메모리 소진은 C++의 `bad_alloc`처럼 복구 가능한 오류로
노출되지 않는다. 이를 흉내 내려고 일반 RangeError나 하위 예외를 삼키지 않는다.
이번 검사는 메모리 소진을 유도하지 않았다.

## 검증

신규10검사는 cpppj `CampaignObjectMenuTests`의 TeleportCurrent·
TeleportStubCurrent·Form/Asset/Path/BombTimer/CursorConstructorCurrent 계약을
실제 앞 생성 구간과 같은 월드에서 대조한다. 전체 Session Load 검사의 대체가
아니며 저장 미션 재대조는 4-D D/E 설치 뒤다. 원본 x86 Path 전체 이미지·
입자/커서/타이머 관측은 기존 하위 검사에서 계속 대조한다. 새 fixture는 없다.
manifest804항목은 유지하고 GameAux 관련3항목 계산 메타데이터를 갱신했다.

첫 집중 실행20건은15통과/5실패였다. 시험 입력에서 NULL 생성자 주소와 두
자산의 VT를 추정했고, 커서의 실제 `fields.bytes`를 건너뛰었으며 아직 없는
owner의 anchor/contained를 사용했다. 기준 소스의 실제 주소/VT와 기존 공개
필드로 교정하고 같은 Pool의 실제 Anchor/Contained/Motion을 조립했다.
두 번째19/20의 누락된 외부 효과 함수는 성공 함수 대신 명시 거부 장치로
보충했다. 최종20/20 통과이며 제품 동작이나 기존 단언을 약화하지 않았다.

최종 전체는 Node22(v22.23.3)·Node24(v24.22.3)·Node26(v26.13.0) 각각
**5,571건 통과, 실패·취소·건너뜀0**이다. Node24는 이 PC에 보존된
`extracted/webpj-oracle/node24/node_modules/node-win-x64/bin/node.exe`를
사용했다. 대응표1,160행·오류0이며 상태는 port842·partial35·todo129·제외154,
4-D29/44로 그대로다. 웹 JS129,572줄·검사755파일이다. 전체 로그는
`extracted/wb-e585-node22.log`·`wb-e585-node24.log`·`wb-e585-node26.log`,
대응표 로그는 `wb-e585-port-map.log`다(Git 제외).

## 남은 경계·다음 시작점

Session과 Devastation 자격/타이머 부분은 계속 partial이다. 실제 펜스
Rebuild·Motion의 생성 순서 설치, 상태 타이머의 주문 시전/수신/만료 통합,
전체 Load·Login·앱 수명·L2/L3는 잔여다. 새 공중3종 전용 관측·생성/생산/
행동·포획/희생·전투 승리 검사는 하지 않았다.

다음은 `RawCampaignSession.cpp` **1484행** `services.archer` 값 복사부터
Archer/Missile·IceRicochet 결선이다. 그 뒤 Vortex/이동·건물과 나머지 실제
모듈·외곽 공개·같은 Template TypeNames/Objects/Contents·Load로 이어간다.
