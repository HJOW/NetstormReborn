# 같은 Storage의 장기 Factory·공유 상태·기본 부모 (WB04-E593)

`225553df`의 `client/RawSurfaceStorage.cpp`143..168(종속/공간 해제),
402..407(건물),414..428(표시),443..454(공장), 일반 Acquire의 공통
상태 값 대입과 `RawSurfaceWorld.cpp`69·161..173의 실제 대여를 연결했다.
`o/SquidDisplay.cpp`78..79·162..229의 현재 view 읽기도 포함한다.
Storage/World의 행 base `14fd017c`, Display/PostPop의 `225553df` 및
기준선 뒤 이번 cpp/h는 변경0이다. cpppj 파일/보호 자료는 수정하지 않았다.

## 같은 owner와 실제 기본 부모

- `postPopState`와 안정된 깊이 별칭은 Storage가 보유한다. Factory·World의
  후처리/소유자·같은 TotalMade 세 표·GameAux 총비용·장기 SID 목록이
  이 상태를 사용한다. Factory를 World마다 새로 만들지 않는다.
- 일반 Acquire만 기존의 새 값 대입을 같은 상태 주소에서 호환한다.
  깊이/생산dirty/AI 표시/독립 owner 목록을 초기화하며 공유 장부/비용
  연결은 유지한다. 준비 대여와 반납은 현재 상태를 재설정하지 않는다.
- Factory는 실제 현재 TYPE·HP/사제1/4·종류·fakeSurface·진단과 같은 깊이를
  읽는다. 기본 공간/종속 부모는 선택 지점에서 같은 Storage의 한 Unpop/
  Building/ContainedUnpop을 조회한다. 생성/HP/void Take는 World·표시
  대여나 공간 실행기를 미리 요구하지 않는다.
- 현재 Session 종속 분리는 Storage가 약하게 빌리고 World의 같은 현재
  Factory 대여도 그 종단을 쓴다. World 종료 뒤에는 기존 Session 함수를
  붙잡지 않고 실제 Storage 기본 부모로 돌아간다. Source의 독립 내용물
  통지는 `RawContentParentNotice`이고 실제 Native 내용물/입력은 별도다.
- 같은 Display는 짧은 World 표시 대여 또는 장기 Client 표시 장치를
  선택한다. 대여 검증/명시 release는 E592의 약한 수명 규칙과 같다.
  정상/예외 복귀까지 선택한 Runtime을 pin하고 값형 view 복사도 그
  호출 안에서 마친다. 첫 표시 구성에 현재 카메라가 없으면 오류다.
- 메타 갱신만 현재 함수가 비었으면 Client 장치를 선택한다. 일반 view/
  억제/dirty는 현재 대여를 우선하며 누락을 기본 성공으로 바꾸지 않는다.
  changed는 현재 World에만 알리고 별도 Client 효과를 만들지 않는다.
- World는 두 공장/표시/공간 실행기를 만들지 않고 같은 Storage 참조를
  사용한다. 실패/일반 종료의 자기 대여 반납, 실제 프로세스 정리 뒤
  후처리 Runtime 장치 분리를 연결했다. World의 setDisplay는 현재
  값형 카메라를 갱신하며 표시기의 getter는 그때 값을 읽는다.

## 현재 카메라의 읽기 순서

옛 `setView` 사본만 쓰는 구현을 현재 getter에 연결했다. Bounds는
원문처럼 카메라X → X 배율 → 카메라Y → Y 배율 → 폭 배율 → 높이
배율의 여섯 위치에서 읽는다. 원본 clip 위치도 각각 현재 viewport다.
본그림 dirty가 카메라를 바꾼 경우 그림자의 다음 경계는 바뀐 값을 쓴다.
독립 값형 view 및 `bindView(null)` 호환은 유지했다. 5-B의 이 선행
③ 항목은 완료지만 실제 프레임 카메라/Renderer/메타 상위 결선은 남았다.

## 근거·최초 실패·검증

기존 cpppj RawSurfaceStorageTests·Factory/Form·Display 부모 검사와
보호 자산을 쓰는 웹 검사를 함께 읽었다. 이번 신규17건은 실제 Pool/
Form/Storage와 독립 카메라/호출 기대값을 쓰는 소스 계약 대조다.
전용 원본 Runtime/장기 Factory 관측은 새로 수집하지 않았다.

초기 기존67건 중66건이 통과했다. E592에서 상위 장기 기본 부모가
없음을 검사했던1건은 이제 실제 Storage 기본 Take 성공으로 바꾸었다.
제품 관문을 완화하지 않았다. 신규14건을 처음 합친81건 중7건은
검사에서 표면 타입 번호를 연결하지 않은 입력 오류였다. 실제 캠페인
번호82/94/95/155/157을 지정해 고쳤다. dirty 후 카메라 검사 추가 때는
슬롯 길이를76, 이어0x50으로 잘못 넣었다가 실제50바이트로 교정했다.
최종 신규17건·Factory/World/Storage/Display/Session 기반 관련140건이
모두 통과했다.

cpppj의 옛 `TotalMadeSharingLeavesPlayerFactoryBindingsWorldLocal`
검사는 준비 World에서도 새 owner 목록을 가정한다. 현행 cpp 소스는
bookkeeping을 Storage의 참조로 보유하고 준비 대여/반납에서 이 목록을
초기화하지 않는다. 이번 이식은 현행 소스의 장기 상태를 따랐으며
그 옛 검사의 네이티브 통과를 주장하지 않는다. CPP 검사를 실행하거나
원본 동작의 새 결론을 낸 것이 아니다.

Node v22.23.3·v26.13.0 최종 전체는 각각 **5,668건 통과, 실패·취소·건너뜀0**이다. 대응표1160행/오류0도 확인했다. 기존806항목
중 관련7개(owner/postpop/bridgedecay/display/간헐천·마나·볼트 부분표시)의
구현 추적을 갱신했다. fixture SHA/행 수와 원본 관측 자체는 그대로다.
JS130388행·검사763개, port845/partial36/todo126·잔여162모듈/
34373 C++행은 같고 4-D 선행 완료를 포함해35/50이다.
Node24는 사용자 지시로 생략하고22/26 통과 시 호환을 가정한다.

로그는 `extracted/wb-e593-initial.log`, `wb-e593-new.log`,
`wb-e593-focused.log`, `wb-e593-node22.log`, `wb-e593-node26.log`,
`wb-e593-port-map.log`다.

## 남은 경계와 다음 시작점

다음은 같은 장기 Factory/HP·Host·GameAux2048BYTE/헤더·진단을 사용하는
Storage311..325의 실제 `Transmission/BindPopRuntime/TransmitPop`과
World174..179·302..304의 송신/삭제 → Session1611 Runtime이다.
Session 생성자는1610까지이며 실제 Runtime 공개·Load·프레임/L2/L3는
아직 없다. Graph/Pop/PostPop/Frame/Owner 등 나머지 지역 실행기의
Storage 소유권 전환도 잔여다.

Native 전송 장치·직렬화 전역/내용물 종류·기본 Form 송신/writer는 그
상위 결선에서 함께 옮긴다. 이번 독립 기본 종단을 Native 전체 완료로
세지 않는다. 실제 미션은 아직 플레이할 수 없다. 게임/브라우저·자연
완주·실제 포획/희생·특정 적 공격/승리·공중3종 전용 관측은 하지 않았다.
