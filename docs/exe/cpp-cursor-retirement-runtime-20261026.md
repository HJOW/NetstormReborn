# 커서 장기 상태와 AI·Player 링크 반납 준비본 통합

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 전체 디컴파일 날짜는 **2026-10-23**이다. 대상은 원본 10.78이며 이번에는 기존 디컴파일·PE의 정적 근거를 사용했다. 새 디컴파일이나 원본 게임 실행은 하지 않았다.

## 복원과 병합

- [443 준비본](prepared/cursor-owner-443-20261025/README.md)의 원시 입력 28개에서 결과 32개를 별도 디렉터리에 재현했다. Git 검사·적용 종료는 모두 0이고 정규화 SHA 32개가 보관 manifest와 일치한다.
- [444 준비본](prepared/state-runtime-retirement-444-20261025/README.md)의 입력 10개와 패치 SHA를 확인하고 별도 저장소에서 재현했다. 각 파일의 LF/CRLF를 보관된 결과 SHA와 일치하게 선택했다.
- 현재 HEAD `9f5768a4`의 제품·기존 검사와 원시 base·준비 결과를 삼방향 병합했다. 443의 CMake 두 파일과 편집 checkpoint 검사 끝부분에 추가 충돌이 있었다. 기존 ProcessHost 등록과 GameAux 검사 두 개를 보존하면서 새 등록·커서 검사 두 개를 함께 넣었다. 444는 충돌 없이 합쳤다. 준비본으로 현재 파일 전체를 교체하지 않았다.
- 기존 줄의 줄바꿈을 유지했다. 줄바꿈 정리 도중 `ConnectionActivitySessionTests.cpp` 열기에서 OSError 22가 한 번 발생해 같은 디렉터리의 임시 파일을 교체하는 방식으로 마쳤다. 원시 병합 자료와 첫 실패는 `extracted/cpp-cursor-retirement-443444-20261026/` 및 이번 실행 기록에 남긴다.

## 실제 동작의 범위

`ClientGameplayGlobals::CursorGlobals()`가 커서 목록 **30 DWORD**, signed 개수, 측정·배치·격자 상태를 소유한다. 실제 Session은 같은 주소를 빌린다. 준비 대여의 생성·실패·반납은 현재 상태를 초기화하지 않는다. 실행기·콜백·예약 타이머는 Session 수명에 남는다. 일반 대여의 초기화는 기존 호환 동작이며 원본 State cleanup을 실행한 것으로 세지 않는다.

원본 `00445aa0`은 Prepare 뒤와 각 가상 삭제 뒤 현재 개수를 재조회한다. `00446e70`은 잠긴 빈 커서에서도 Activate(1)과 Prepare·현재 프로세스 정리·목록 120바이트 초기화·잠금 꼬리를 실행한다. 첫 메뉴용 연결도 실제 카메라 snapshot과 호출 시점의 Renderer를 사용한다. 큰 DWORD를 낮은 WORD로 접어 다른 정상 객체에 접근하지 않으며, 호스트 SID 소비 직전에 명시 범위 검사를 한다. 단일 `CursorDropState::preview`의 SID16 경계는 남아 있다.

444는 원본 `resetAi(00413ae0)`와 `ClearPlayerLinks(0048fcd0)`의 **실제 정상 복귀**에서만 이동 전용 증빙을 발급한다. 같은 모듈의 수명·현재 상태·진행 중 호출을 확인한 뒤 증빙을 소비한다. 소비한 자식만 Session 소멸의 중복 정리에서 제외한다. AI 파괴/free 또는 링크 free가 실패하면 앞선 효과를 보존하고 정상 증빙을 발급하지 않는다. 다른 모듈·이동 뒤 원본 증빙·소멸 후 동일 주소 재사용·callback 중 재진입은 거부한다.

전체 State1 cleanup/prefix의 제품 coordinator에는 아직 연결하지 않았다. 개별 자식 반납과 Core(0)만으로 Player 고정뷰 대여를 먼저 반납하지 않는다. 기존 441 GameAux·442 ProcessHost·439 카메라·권한·FullDeck 연결을 유지한다. Edit 제품 진입과 전체 미션 완주를 이번 변경의 완료로 주장하지 않는다.

## 검증

Source1은 2,027파일을 동결했다. 첫 빌드는 579.418초 뒤 종료 1이었다. 현재 main의 `CameraViewTests.cpp`에 남아 있던 `lifecycle.previews[0]=Sid{50}` 대입 한 곳이 DWORD 목록에 맞지 않아 C2440이 발생했다(템플릿 문맥을 포함한 오류 출력 5행). 준비본에 포함되지 않은 후속 439 카메라 검사의 입력형 전환 누락이다. 같은 번호 50을 DWORD로 대입하도록 한 곳만 교정하고 Source2를 새로 동결했다. 기존 검사 조건·기대 숫자·Native fixture는 바꾸지 않았다. 최초 로그는 `extracted/cpp-cursor-retirement-443444-build1.log`와 `build1-result.json`에 보존했다.

두 번째 빌드(16.862초)는 신규 `ClientCursorGlobalsTests.cpp`가 사용하는 `MakeRawSurfaceStorage` 선언의 직접 include 누락(C3861)으로 종료 1이었다. `client/RawSurfaceWorld.h`만 포함하도록 교정하고 Source3를 동결했다. 검사 입력·조건·기대값은 그대로다. 두 번째 실패 로그도 별도로 보존한다.

Source3의 세 번째 빌드는 27.913초·종료0·경고/오류0이었다. 첫 집중 검사는 430관측 중 신규 Session 검사 세 개가 실패했다. AI 초기 동맹의 외부 Chat 통지, 실제 Deck 할당 직후 바이트, Client와 동일한 Subscriber 소유자의 공급 누락이다. 제품의 거부 관문을 완화하지 않고 신규 검사 세 개의 외부 fixture만 보완했다. 기존 검사와 신규 검사의 입력·기대 숫자·조건을 유지했다. Source4 재빌드는 52.059초·종료0·경고/오류0이다. 첫 집중 실패는 `focused1/`에 보존했다.

최종 Source4의 **집중 검사 422고유/430관측·실패0**, 신규20개 전부가 통과했다. 실제 출력 이름을 현재 등록 이름과 대조하고, 확인된 stderr 진단만 메모리에서 분리했다. 원시 로그는 바꾸지 않았다. 변경된 기존 검사298개 중297개의 숫자 값은 동일하다. 나머지 카메라1개에는 준비본의 장기 주소·현재 snap·실제 Cancel 추가 단언이 있으며, 기존 fraction·좌표 조건을 보존했다. Native fixture 변경은 없다.

동일한 최종 게임 SHA **`423d0911af400e47de0e8cc44c43e7dc14de6ab6a50148411ff7a84d3814c4e6`**로 메뉴 GUI 두 실행, 자연 Early2, 전체 CTest를 통합 검증했다. **CTest 내부3,027개·실패0**이며 CTest 시간은355.65초다. 이전 최종 통합의3,007개 실제 이름을 보존했고 실제 추가20개가 이번 신규 목록과 정확히 일치한다. 보호1,821파일, 동결2,027소스, 네 실행 파일과 실행 도구의 SHA를 단계 전후에 확인했으며 원본 전체 집합과 두 설정 파일의 복원도 통과했다.

Early2는 실제 생산 타이머1·상태 주입false·초기 진행 주입false로 A~I 안내, 신전·워크샾·지식·생산·발사기·회수 환불75·종료SP3075·다음Tutorial3 초기SP2000과 같은 프레임의 다음 안내를 확인했다. 기존 완료값1은1로 유지했다. 다음 브리핑 캡처의SP1700은 설치 후 생산이 진행된 시점이다. 첫 Bridges 안내, Campaign 목록, Capture The Priest! 브리핑의 세 화면을 육안 확인했으며 원본과의 픽셀 동일성 검사로 확대하지 않는다.

[최종 실행 보고서](../../cpppj/cursor-retirement-runtime-20261026.json)에 각 시도의 로그·Source SHA·실행 파일·실제 검사 목록·통합 실행·육안 확인을 기록했다. 원시 통합 자료는 `extracted/cpp-cursor-retirement-443444-batch1/`에 있다. 작업 중 반영된 별도 문서 커밋 `1eddb0e8`은 국·영 README만 변경했으며 이번 제품 소스나 동결 실행에는 영향을 주지 않았다.

## 남은 작업

이 통합으로 준비본443·444의 병합·검증을 완료했다. Native State1 전체 cleanup/prefix와 실제 제품 반납 coordinator, 437 사제4leaf 및 Prepare의 권한 사본, 436의 잔여 SID16 변환, 438의 전체 Deck 명령 부모/nullable+ac, 단일 Drop.preview DWORD 경계는 후속이다. Edit 제품 진입·편집·저장과 전체 Early/Priest Training/캠페인/데모 자연 완주·진행 저장·원본 완전 동일성은 계속 미완료다.
