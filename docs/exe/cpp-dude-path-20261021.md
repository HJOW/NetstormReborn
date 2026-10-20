# DudePath24의 실제 경로·현재 속도·부모 연결

2026-10-21 단위244. 시작 커밋 `c6931d8e`. 현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**로 일치한다. 기존 정밀 자료와 보호 PE를 읽었으며 새 디컴파일은 없다. 원본/CD·AGENTS·dotnetpj는 변경하지 않는다. 실제 게임/C++검사/CTest/전체 회귀는 사용자의 기존 지시대로 통합 검증에 이월한다.

## 확인한 원본과 새 독립 관측

- 10.78 `0048bf30`은 `new(0x186f0)`와 공통 `0048b2c0` 후 VT `0050e3d8`을 설치한다. 사제54/VT50e458과 비교한13슬롯은 부착+2c만 다르다. Init48bb10, Run48bb40, Arrive48bd90, Halt489ff0, cleanup48aa00, Prepare48a200은 기존 복원 몸체와 같다.
- 부착48bab0은 현재 부모 타입과00541170(70)을 비교하고, 41bd20에005410d0(24)·부모SID·flags·지정Form을 전달한다. 경로 시작48a 계열은 Dude에 flags10을 사용한다. 부모70/VT508cc8의 +40=빈4437f0/+44=빈480260이므로 사제의 간헐천 선택·프레임 초기화 통지를 수행하지 않는다.
- **제품 작성 전** `tools/decomp_dude_path.py`로 생성자 전체와 부착 전체를 격리 관측했다. **30입력·두 정밀도60정상 반환·61614명령**이다. 생성자6입력은 세 heap 패턴과 할당 성공/실패, 부착24입력은 현재 부모70/71/158·flags0/10/50/80·지정Form0/65535다. 생성자는 전체100080바이트 Adler32, 부착은 실제 전체 인자와 복귀 진단 순서를 기록한다. 잘못된 부모의 원본 assert는 복귀 경계이며 C++의 엄격한 예외 정책과 구분한다. 지정65535는 인자 전달 관측이며 제품 풀에 그 번호를 실제 부착한 검사가 아니다.
- `new`, Host41bd20, 복귀하는 진단은 명시 효과 경계다. 실제 GP/retN/canary/x87/명령·읽기·쓰기 범위를 확인했다. 게임/OS/창은 실행하지 않았다. 공유 Init/Run/Arrive 전체를 이번에 다시 관측했다는 뜻도 아니다. 기존 공유 경로의 근거는 그대로 보존한다.
- 동결 원문은 `cpppj/dude-path-native.json`, `cpppj/tests/fixtures/dude-path-x86.tsv`, `dude-path-vtables.tsv`다. CD10.72 부착은 정밀 매칭에서4804f0, 공유 Init/Run은480550/4805d0이다. 이번 제품의 Dude24는10.78만 허용한다.

## 실제 Session 연결

`RawPathRuntime/RawPathProcess`에24의 정확한 VT와 부모70 부착 검사를 추가했다. 사제/웜의 실제 공유 Init·보행·정지 프레임·도착·cleanup을 사용하며 다른 걷기 모형을 만들지 않는다. `RawPathStart`는24 생성자를 허용하고 기존 경로 취소·명령 보존·BFS·Prepare·로컬Form 부착을 그대로 수행한다. Dude 부모의 두 빈 시작 통지에서 즉시 반환한다.

`RawCampaignPathBindings`의 `StartDude/CurrentPath`는 현재70/VT508cc8과 이동24를 확인하고 실제 같은 Host/Form을 찾는다. 부모 Halt/Arrival은 단위243의 같은 `RawDudeBehavior`로 연결한다. Session의 `hitEffects.dudes.pathEndpoint`는 현재 Runtime.Point(Count−1), `startPath`는 같은 StartDude/BFS다. `RawDudeNotice.Move`의 현재 속도2배·기존 속도의 정수절삭 복원 의미를 유지하기 위해 **Dude만 metadata.speed를 `dudeSpeeds[kind].speed`에서 읽는다.** 다른 부모는 기존 타입 자산 속도를 유지한다. Session의 `Dudes/StartDudeMovement`는 같은 몸체를 공개하며 상태나 경로를 복사하지 않는다.

## 검증 범위와 남은 일

최종 전체 Release 빌드는 **경고0·오류0·종료0**이며 새 검사2개도 컴파일됐다. 빌드 로그는 `extracted/cpp-dude-path-20261021-build.log`다. 검사 실행/게임/CTest/전체 회귀는 수행하지 않았다.

새 검사2개를 작성했다. `DudePath_NativeConstructorAndSharedVtables`는 생성자6입력 전체와 두 정밀도/공유 슬롯을 대조하도록 작성했으며, 부착24행을 제품 Host 실행으로 표시하지 않는다. `CampaignDude_ActualPathNoticeSpeedAndArrival`는 실제 원래 지도에서 비점유 표면을 골라 명시 Dude를 생성하고 같은 경로 끝점 유지·로컬24·직선 구간2배 속도·부모 도착 삭제를 검사하도록 작성했다. 전체 Kernel은 실행하지 않고 도착 task5107만 명시 입력으로 준다. 자연 생성/통지 대피/완주 검사가 아니다. 두 검사는 **컴파일만 하며 실행하지 않는다.** 빌드의 실제 결과와 파일 해시·핀 역변환은 `cpppj/dude-path-session.json`에 기록한다.

다음 경계는 부모 Regular450fa0(task5101~5107)과 자연 생성/최대 population 초기화 정책이다. 빌드 대기 중 보호PE를 추가로 읽어 population의 객체 시작이 **55a4ec**임을 확인했다. static initializer **4fc9b0**은push1e(30)→ECX55a4ec→44fb20을 수행한다. 공통 생성자44fb20은 new(capacity×4)와ptr+4/cap+8/count+0xc·VT508c04·472880 registry 등록을 처리한다. 이번 단위는 이 초기화를 제품에 반영하지 않았으며, 다음 단위에서 독립 관측 후 현재 capacity0을 원래30과 등록 정책에 맞게 연결해야 한다. 기존 Notice의Regular5107이 실제 Kernel에 분배되기 전에는 이번 경로 연결만으로 자연 대피 완료를 주장할 수 없다. 지형 Devastation·포획/Drop·제단/희생·다른 지정 미션·F8 Guide/키·전략AI·원본 동일성과 실제 통합 플레이도 계속 남는다. 전체 goal은 **active/미완료**다.

빌드 말미 추가 읽기에서 **Dude foot0의 탐색 실패 경계**도 확인했다. 기존 `RawPathStart::Footprint`는134/158에 대한 foot≥1 가정이므로 Dude의 Count0에서 거부한다. 이번 단위는 성공 경로 연결이며 이 실패 처리는 다음 첫 작업이다. 원본4ade70→49ae80은 `(x,y)`에서 시작한 사각형에 `x−(footX−1),y−(footY−1)`를40e880/40e8f0로 min/max 확장하므로 foot0의 범위는x..x+1/y..y+1이다. 다음 단위에서 실패/겹침의 새 독립 관측 후 수정해야 한다. 기존 PathStart fixture의 부모134/158·foot1 입력은 덮어쓰지 않는다.

기존 보고서의 현재 소스 핀 **124보고서·362항목**만 후속 이력으로 갱신했다. 전체 JSON을 시작 커밋으로 역변환해 원본·fixture·과거 실행/바이너리·이력 보존을 확인했다. `tools/decomp_dude_path.py --verify`와 `tools/cpp_dude_path_audit.py --verify`는 읽기 전용 감사다. 인수인계 갱신 후 날짜/프로젝트 규칙에 맞춰 로컬 커밋하며 push하지 않는다.
