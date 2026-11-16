# Factory 실제 World 대여 부모 (WB04-E592)

`225553df`의 `o/SquidFactory.cpp`235..278과 `SquidFactory.h`18..24,
`client/RawSurfaceWorld.cpp`167..173의 현재 공간/Form 대여를 연결했다.
행 base `be567d46` 뒤 Factory cpp/h 및 기준선 뒤 이번 cpp/h·World cpp
변경은0이다. cpppj의 FormTake·FormAttach/Pop/Unpop 부모 검사와 기존
웹 Factory 계약을 함께 읽었다. Runtime pin 자체의 전용 cpppj 검사나
원본 fixture는 검색에서 찾지 못했다. 새 검사는 실제 Pool/공통 Form/
공간 부모와 별도 호출 관측을 쓰는 소스 계약 대조다.

## 실제 선택·수명

- `bindUnpop`은 실제 장기 공간 조회기만 저장한다. 생성/void Take/Form
  해제에는 호출하지 않고 선택된 자산 VT+48에서만 현재 부모를 찾는다.
- `bindRuntime`은 같은 Pool·필수 장치4개와 동일 현재 대여를 검증한다.
  약한 참조만 저장하며 World가 실제 장치 객체를 보유한다. 반환 release는
  웹의 명시 종료에 필요한 C++ 약한 대여 만료 대응이다. 멱등이며 같은
  객체의 새 대여를 지난 release가 지우지 않는다.
- Take·PopForm·AttachForm·UnpopForm은 진입 시 현재 대여를 선택하고 pin을
  호출한 뒤 정상/예외 복귀까지 보유한다. pin 실패는 어떤 prefix보다 앞이다.
  자산 Take는 진단 중 대여를 반납/교체해도 처음 선택한 공간 부모를 쓴다.
  Form Take의 가상 부모는 `UnpopForm`을 새로 호출하므로 그때 현재 대여를
  다시 선택한다. 두 수명을 같은 것으로 합치지 않았다.
- Form의 전송은 명시 인수 → 선택된 현재 대여 → NoWorld 기본 장치다.
  내용물 TYPE/종속 분리도 같은 현재 대여를 쓰며 원문 링크/void/깊이 쓰기와
  진단 순서를 유지한다. 현재 장치 누락을 기본 장치로 대체하지 않는다.
- 실제 RawSurfaceWorld는 새 Factory에 공간 부모를 영구 저장하는 대신
  현재 대여를 연결한다. 생성 실패/일반 종료의 프로세스 정리 뒤 반납한다.
  종료 뒤 옛 Factory를 보유해도 과거 World의 Unpop을 기본값으로 쓰지 않는다.
  현재 World/Session 전체 pin 장치는 상위 실제 Runtime 공개 때 연결할 잔여다.

## 첫 실패와 검증

최초 관련89건 중83건 통과/6건 실패였다. 새 입력에서 클라이언트 SID를
진단 없이 서버 Take에 넣은2건, 그 범위 진단을 놓친 호출 순서1건,
클라이언트 Form의 SERVER_POP 플래그1건, 종속 분리 대신 공통 Unpop을
재귀 호출한2건이었다. 서버 SID/실제 종속 분리 부모로 입력을 수정했고
원문 진단/제품 관문은 완화하지 않았다. 새11건 및 기존 Factory·World와
묶은89건은 최종 모두 통과했다.

Node v22.23.3·v26.13.0 최종 전체는 각각 **5,651건 통과, 실패·취소·건너뜀0**이다. 대응표1160행/오류0도 확인했다. 새 fixture/원본
실행은 없고 기존806항목 중 관련12개 구현 추적만 갱신했다. JS130218행·
테스트762개이며 이식 상태845 port/36 partial/126 todo·잔여162모듈/
34373 C++행은 그대로다. 4-D의 선행 완료 항목을 포함해34/49다.
Node24는 사용자 지시로 생략하고22/26 통과 시 호환을 가정한다.

로그는 `extracted/wb-e592-focused.log`, `wb-e592-new.log`,
`wb-e592-node22.log`, `wb-e592-node26.log`, `wb-e592-port-map.log`다.

## 다음 시작점

Factory의 현재 대여 경계는 연결했지만 **Factory 자체는 아직 World 지역
소유**다. 다음은 실제 `RawSurfaceStorage::Factory`443..454와
`postPopState`/공유 깊이를 장기 소유로 전환하고, 현재 Unpop/Form 기본
장치도 같은 Storage 부모에 잇는 작업이다. 그 뒤 Storage311..325/
World174..179·302..304의 같은 GameAux2048BYTE/Host/진단을 쓰는
Squid 송신과 삭제 → Session1611 `RawWorldRuntimeHooks`로 간다.
별도 Factory나 임시 Form 송신 성공 함수를 만들지 않는다.

Session 생성자는1610까지이며 실제 Runtime 공개/Load/프레임·L2/L3는
남았다. cpppj·보호 파일 수정·디컴파일·게임/브라우저·자연 완주·실제
포획/희생·특정 적 공격/승리·공중3종 전용 관측은 하지 않았다.
