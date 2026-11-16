# Session의 세 Aviary 생성 구간 결선 (WB04-E609)

`225553df` 생성자1665..1667의 ConnectSunAviary2074..2103,
ConnectWindAviary2034..2073, ConnectRainAviary2105..2126를 같은 순서로
옮겼다. 대상 Session.cpp와 세 기존 몸체/필드·FlyerProcess·WindCircle의
기준선 뒤 변경은0이다. 기존 가상 몸체를 수정하지 않았다.

## 같은 현재 참조와 Host

- 같은 실제 Pool/Host에서 현재 첫 프로세스를 찾고 공통 Damageable
  후처리·현재 프레임/범위 진단·소유권/Pop·위치 음향을 사용한다.
  중복 진단은 원문 세 식/행에서 현재 AI 진단 함수를 호출하며, 다른
  미연결 진단은 선택된 위치에서 명시 오류다. Wind의 생성/Pop flags는0,
  부착 flags는8이며 별도48BYTE/80BYTE 필드 래퍼를 유지한다.
- 서버·편집·시계와 fort/boss를 같은 현재 owner에서 읽는다. checking과
  친군사격 RawCheckingFlag의 사본도 원래 전역 owner를 계속 읽는다.
  독립 typed의 역할은 Sun32/73, Wind59/138/139, Rain60/144다.
  실제 Client의 Native 장기 대여는 후속 결선에 남았다.
- Sun의 현재 Form 갱신은 같은 Host의 실제 파생 객체와 필드 동일성을
  검사한다. 홈 통지의 직접 그림 쓰기는 같은 Unpop/Repop과 현재48BYTE를
  사용한다. Wind 종료도 같은 Host의80BYTE를 검사해 Kill한다.
- Wind의 높이/발자국은 현재 TYPE/SHP, 원주 보정은 실제13번 SHP 헤더의
  float와 signedWORD26/30을 사용한다. 후설치 Flyer의 같은104BYTE·speed
  float 비트·0.5 인자·기본 생성/부착 뒤 전용 표 쓰기를 코드로 연결했다.
  Flyer 몸체는 생성자1669에서 설치하므로 앞선 선택 호출은 명시 오류다.
- 기존 JS Fields가 새 메모리를 확보한다. JS 신규 Uint8Array의0 초기값은
  플랫폼 할당의 특성이며, 별도 memset이나 가짜 CRT 미정 바이트 생성기는
  추가하지 않았다. 원문 생성자가 쓰지 않는 필드는 기존 몸체처럼 유지한다.
  malloc의 미정 힙 바이트를 재현했다고 주장하지 않는다.

## 검증과 남은 경계

신규3건은 첫 실행에서 통과했다. 원문 생성 순서/반복/반납,
생성 중 Pool/게임 난수 무변경과 Session 미공개, 현재 전역 참조/역할,
지역 대입 뒤에도 같은 checking/친군사격 owner를 확인했다. 유닛/생산
프로세스를 생성하지 않고 getter와 조립 상태만 확인한다. 사용자가 제한한
공중3종의 새 전용 생산/생성/우클릭/행동 관측·검사를 추가하지 않았다.
기존 부품의 관측 시험은 전체 회귀에 그대로 포함하지만 새로운 실제
Session 생산/비행 검증으로 세지 않는다. 직접 신규 fixture는0개이며
manifest는806항목을 유지한다.

Node v22.23.3·v26.13.0 전체는 각각5,864건 통과했고 실패·취소·건너뜀은0이다.
두 실행의 exit0과 최종 요약을 확인했다. 대응표 오류0, UTF-8 6파일·구문/diff
검사도 통과했다. JS132,027줄/시험779파일이며 이식 수치는846/39/122·
session0을 유지한다. Node24는 사용자 지시로 생략했다. 실제 미션/Load·원문 World 분배와
Flyer 전체 결선·Native 장기 역할·실제 생성/생산은 후속 경계다. 다음은
생성자1668 ConnectArcSpire,1669 ConnectFlyer,1670 Fence,1671 GemReward다.
그 뒤1672 Buried/전체 자산 삭제/Regular 분배/World 게시1825로 간다.

보호 원본/cpppj 수정·디컴파일·게임/브라우저 실행·실제 사제 포획/희생·
특정 적 공격·승리 재현을 하지 않았다. 로그는 extracted/wb-e609- 접두의
initial·port-map·node22·node26이다.

2026-11-16 사용자 요청으로 이 단위 검증·인계 문서·로컬 커밋 후 목표를
일시 중지한다. 재개 첫 지점은 위1668 ConnectArcSpire이며,1669의
Flyer 내부 Frame/Target과 현재 SID 진단·방송 helper도 함께 대조해야 한다.
