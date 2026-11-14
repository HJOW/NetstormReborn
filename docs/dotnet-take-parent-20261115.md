# dotnetpj 0.543 — Take 부모와 현재 Factory/World 장치

2026-11-15, HJOW-Athlon. 직전0.542는51d5ea05d다. 기준은cpppj o/RawSquidTake.h/.cpp e1a617acd52d3308017308a1e5495ff8c8c9b194, o/SquidFactory.h/.cpp be567d46b5eca30d2a1421af78d49125add1ff6a, client/RawSurfaceWorld.cpp c102cdbf36fca4e8661d0699ea2ce5f2f44b2934다. 바이너리와 디컴파일 산출물은 변경하지 않았다.

## 같은 실제 Take 부모

RawSquidTake는 전체DWORD TYPE/객체 토큰을 유지한다. signed SID 범위 단락 평가→현재 상태/TYPE와 상세·복귀 진단→현재VT+48→판본별 상태 저장→현재 생성자→실제 반환 객체의 저장VT+2c 순서다. 생성자가 돌려준 객체는 할당 SID와 다를 수 있다. PATCH는 반환 객체의 상태/VT를 읽고 TYPE→상태를 쓰며, CD는 VT/상태를 읽고 상태→TYPE를 쓴다. 선택하지 않은 장치의 선검사나 할당/실패 롤백은 추가하지 않는다.

실제 RawSquidFactory.Take의 옛 TYPE/클래스 쌍·contained·void 선검사를 같은 부모로 대체했다. 현재 원문에는 Legacy모드 분기가 없으므로 별도 옛 동작 우회는 넣지 않았다. !free이면 void여도 실제 가상 해제를 호출한다. 네 실제 Form VT는 공통UnpopForm, 지원 자산VT는 같은Pool의 실제Unpop을 선택한다. 해제 뒤 현재 생성자를 다시 읽고 반환 객체의 실제VT가 Form이면 빈+2c, 자산이면 현재PostTake를 실행한다. 미연결 장치는 도달 위치에서 명시 실패한다. 상세 이름은 client→server 순서로 현재TYPE에서 첫NUL 앞을 복사한다.

## 실제 수명과 현재 World

공장은 현재 Runtime을 약하게 빌리고 실제 호출 동안 Runtime과 pin이 돌려준 외곽 owner를 강하게 보유한다. pin은 호출수 증가/게임 상태 쓰기보다 앞선다. owner의Dispose를 대신 호출하지 않는다. 명시 반납이 호출에 재진입하면 마지막 복귀까지 같은 대여를 유지한다. 현재 Runtime이 없으면 장기Unpop getter나 원래 실제 장치를 선택한다. 다른Pool/살아 있는 다른Runtime/필수 장치 누락은 결합 시 거부한다.

AttachForm/PopForm/UnpopForm도 같은 현재 대여를 소비한다. 실제World가 같은Unpop과 현재Session의ContainedDetach/ContentType, 실제Form 전송 경계를 소유한다. Session 대여가 끝나면 같은World의 기본분리로 돌아간다. Process 장치 반납은 Factory 대여를 반납하지 않는다. 전송 미연결을 빈 성공으로 대신하지 않는다.

## 원본 대조와 회귀

squid-take-parent-current-x86.tsv의 원본720행을 연결했다. 두 판본의 전체읽기/쓰기·복귀 진단·실제 반환객체·저장VT와 두payload를 비교한다. 독립 원본CD 입력은36BYTE, PATCH는50BYTE stride다. 이 primitive 통과를 CD 전체 관리풀/게임 완료로 세지 않는다.

추가9경우는 선택 실패의 선행 상태 저장, 실제void 해제와 현재 생성자/후처리, 불일치 상세/진단 뒤 클래스 변경, 미지원/null 생성자 실패, 현재Runtime 우선/지연반납/장기getter 복귀, 약한대여 만료, 외곽owner 보유/pin 실패, 실제World+Session 분리/현재TYPE7 생성자와 빈Form 후처리를 검사한다. 양판본 실제공장의 네Form VT/자산분배도 더해 원본재생1+추가9=집중10건이다. 기존합성4곳은 실제Unpop/void진단/명시free부모 입력을 공급하도록 고쳤다. 기존 creation482·derived3300 명령의 golden은 바꾸지 않았다.

반환객체를SID로대체·저장VT재조회 변이는 각1/9, void해제우회는4/9 실패로 검출하고 바이트/SHA를 원복했다. 전체 비증분Release0경고0오류 뒤 관련Core401과 전체Client일반1232는 통과했다(오디오실장치Explicit1 미실행). 최초Core400/401에서 드러난 기존합성1곳의 미연결Unpop 입력을 같은실제풀로 고친 뒤401/401이었다. 추가양판본공장검사는 기존CD Form가드에서1/1실패를 확인했고, 공통Forms의 판본표를 빌리도록 보정했다. 이 보정 뒤 최종전체비증분Release0경고0오류·관련Core402/402·각종료0이다. 이 단계에서는 전체Client를 반복하지 않았다. 앞선0.542 전체Core2370통과와 이번관련402통과는 구별한다. 자료는extracted/dotnet-take-0543-initial/·-mutations/·-final/(Git제외)에 둔다.

## 남은 연결

원문 Create 부모/현재생성자와Contained·Gump Create, 전체NativeStorage의 장기공장/World 소유자, 표면connector/파생Form 현행몸체, 제품 피해·해방·사제상위 배치/삭제와 Native활성화는 잔여다. 제품Host 프레임루프/Renderer/HUD/카메라가 미연결이므로 새경로는 플레이 불가, M1전체0/34다. 이번 수명/분리만 전체제품 완료로 세지 않는다.

PORT_MAP875행(L1 420+port455)에는 새7쌍만 더했다. 미이식 부분이 남은 기존Factory/World 전체base와 floor/reflected는 유지했다. fixture296개(신규1), 기존295개 행/해시는 불변이다. 새게임창/금지실제플레이/MP·보호파일/다른프로젝트/디컴파일 변경 없음. 목표active다.
