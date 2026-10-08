# cpppj 실제 게임 플레이 복원 계획

2026-10-08 최신(`VM-W11-CODEX`): **[SquidFrame 표시 갱신·실제 SHP 크기 공급](exe/cpp-frame-display-integration.md)**을 `MakeSquidFrameHooks`로 실제 SquidDisplay/Unpop/Pop·Renderer에 연결했다. Release 경고/오류 0·CTest 내부 **251개·실패 0**(57.52초), 관련 감사 **4종 모두 통과**다. 실제 10.78/CD 자산의 물리 프레임 **6,950개** 메타데이터와 범위 안 크기 공급 **6,946개**를 확인했고 두 판본 모두 부분 Draw/Present가 통과했다. 새 독립 x86 입력은 없고 누적 **131,542개 유지**다. **다음은 Graph 활성 상태의 끝 칸 생성/삭제/Pop·받침 생성 통합**이다. GUI raw GameWorld·Kernel↔게임 루프·애니메이션 프로세스·실제 낙하/파편/소리·건설/경제/전투/승패는 남았다. 원본 게임/복사본·클론 창 실행·커밋/푸시 없음.

2026-10-08 앞 단계(`VM-W11-CODEX`): **[섬 삭제→지연 낙하→끝 칸 변환→연결 객체 정리](exe/cpp-surface-lifecycle-integration.md)**를 `RawSurfaceLifecycle`과 실제 raw 모듈로 연결했다. 10.78/CD에서 정상 교체·고립 삭제·hard 유지·`0x1000` 보존·표면 삭제의 walker 분배를 확인했다. Release 경고/오류 0·CTest 내부 **246개·실패 0**(60.27초), 관련 근거 감사 **7종 모두 통과**다. 새 기계어 관찰은 없고 누적 제한 x86 **131,542개**는 그대로다. **다음은 SquidFrame→SquidDisplay 표시 갱신과 실제 SHP 크기 공급이다.** Graph 활성 통합·GUI raw GameWorld·Kernel↔게임 루프·낙하/파편/소리·건설/경제/전투/승패는 남았다. 이번 단계 원본 게임/복사본·클론 창 실행·커밋/푸시 없음.

2026-10-08 앞 단계(`VM-W11-CODEX`): **[noIsland 최초 등록·3×3 받침 소유자 전파](exe/cpp-islandpostpop-reconstruction.md)**를 복원해 `SquidPostPop`에 연결했다. F 칸에서 받침·종유석을 만들고 H 칸에서 9칸/받침/종유석의 소유자를 맞추는 실제 raw 모듈 통합이 두 판본에서 통과한다. 새 제한 x86 **1,656개**·Release 경고/오류 0·CTest 내부 **240개·실패 0**(100.04초), 누적 **131,542개**·감사 **31개 모두 통과**다. `regiongraph`는 재생성 후 기존 1,536개 fixture가 바이트 단위로 같고 감사도 통과한다. 클론 창 회귀 4종도 통과했다. **GUI raw GameWorld·Graph 활성 통합·Kernel↔게임 루프·낙하/파편/소리·건설/경제/전투/승패는 남았다.** 원본 게임/복사본 실행 없음.

> 2026-10-05 사용자 요청: **cpppj에서도 기존 게임이 온전히 동작하고 실제 게임 플레이가 가능하도록 복원한다. 화면비·한국어·Linux·추가 기능은 후순위다.** 이 문서의 1단계는 진행 상황 확인에서 제안한 "Renderer·원본 글꼴·커서 기반"이다. [기존 기반 로드맵](cpp-roadmap.md)의 완료된 "1. 설정·경로"와 번호를 구분한다.

## 복원 완료의 기준

실행 파일 빌드, 원본 데이터 읽기, 검사용 창 표시만으로 cpppj를 완료 처리하지 않는다. 먼저 **메인 메뉴 → 캠페인 선택 → 1-1 브리핑 → 실제 선택·이동·건설·경제·전투 → 승리/패배·결과 → 재시작/메인 메뉴 복귀**를 완성한다. 캠페인 1-1도 전체 복원의 중간 이정표다. 이후 전체 원본 유닛·단일 플레이 캠페인·튜토리얼·AI·메뉴·사운드·저장/불러오기·에디터를 실제로 구동하여 검사한다.

원본 패치판 10.78의 초기화·갱신·입력·화면 순서를 기준으로 삼고, CD 배포 자료/추가 10.37·복원한 10.62·10.82 본체/DevLog는 함수·자료형·판본 차이의 비교 자료로 쓴다. 사용자도 2026-10-07 **1차 기준 10.78 유지**를 확인했다. [추가 판본 비교](exe/cpp-reference-versions.md). 두 구버전 실행 파일의 버전 파일은 모두 10.37이며 InsertCD 분기 한 바이트 외에는 같다([비교](exe/original1037-comparison.md)). CD 배포판 설명 10.72와 구분한다. 팬게임의 커스텀 기능이나 다른 판본의 규칙을 섞지 않는다. 원본을 디컴파일해 C++로 최대한 복원하는 기존 목표를 실제 플레이 기준으로 구체화한다.

cpppj의 **Win32 직접 호출·Windows 전용** 범위는 유지한다. 기존 4:3 화면과 영어 UI, 원본 수준의 요소별 애니메이션을 먼저 맞춘다. 원본에 있는 전체화면·커서·가장자리 이동·음악·효과음도 복원 대상이다. 실제 구동에 필요한 Windows 10/11 호환성과 전체화면 종료 뒤 정상 재실행은 실행 단계에서 검사한다.

**16:9·16:10 확장, 한국어/D2Coding, Linux, 60/120프레임, MCP 및 새 기능은 플레이 복원 뒤**에 둔다. Linux는 프로젝트 후속 목표이며 cpppj의 현재 플랫폼에 추가하지 않는다. 멀티플레이는 AGENTS.md대로 후순위다. dotnetpj의 새 개발은 cpppj 완성 뒤 진행한다.

## 현재부터의 작업 순서

| 단계 | 작업 | 완료 기준 |
|---|---|---|
| **1. 화면·원본 글꼴·커서 기반** | 변경 표·깊이 정렬·프레임 캐시·색/그림자 합성·창 출력, `.chfnt`, 원본 커서 | 기계어·자산 대조, 정적 검사 장면과 글자 표시, 변화 없는 프레임 출력 없음 |
| **2. 메인 메뉴와 미션 진입** | Gump/UberGump·타이틀/구름·버튼, 캠페인 목록·옵션·상태 전환·브리핑 | 새 실행에서 메뉴→캠페인 선택→브리핑→미션 진입, 취소·종료 |
| **3. 실제 월드와 조작** | Squid·생성자·SID·지형·플레이어/동맹/시작 상태, 실제 객체 그리기, UserInput·이동·애니메이션 | TEST01/1-1 초기 월드, 선택·우클릭·화면 이동·사제 이동·일시정지 |
| **4. 캠페인 1-1 완주** | 다리·건설·에너지·워크샵/덱·채집·경제·전투·포획/희생·AI·미션 조건·소리 | 실제 플레이→승리/패배→결과→재시작/메뉴 복귀 |
| **5. 나머지 원본 기능** | 모든 단일 플레이 유닛·AI·미션·튜토리얼·UI·저장/불러오기·커스텀 맵·에디터·영상·원본 화면 모드 | 원본 기능/미션별 검사표와 실제 플레이 결과 |
| **6. Windows 실행 안정성과 배포** | 전체화면·포커스/최소화·종료/재실행·자원 수명·실행 패키지 | Windows 10/11 반복 구동과 새 환경의 플레이·저장·재실행 |
| **그 이후** | 화면비 확장·한국어·60/120프레임·MCP·후속 플랫폼 작업 | 원본 플레이 기준을 유지한 기능별 검사 |

### 1단계 — 표시 기반 구현·검증 완료

- [x] `Renderer`의 100항목 변경 표·면적 병합·무시/불투명 플래그·깊이/y/x 비교를 두 판본 기계어 **394개 사례**와 대조했다.
- [x] 프레임 해독 캐시·투명 합성·소스 색 변환·대상 색 변환 그림자·변경 영역 그리기/출력을 연결했다.
- [x] `.chfnt` 네 정수 표·256개 VFX 글리프·공백·전진 폭·스타일을 복원했다. 캐시가 없거나 손상됐으면 같은 GDI 글꼴로 메모리에서 생성하고 원본 캐시를 덮어쓰지 않는다.
- [x] 원본 18개 커서 리소스·핫스팟·창 클래스·`WM_SETCURSOR`·8비트 소프트웨어 프레임·커서 이동 흔적 복구를 연결했다.
- [x] `Client::draw`를 제거하고 `Client::Frame`을 Renderer에 연결했다. `WM_PAINT`는 창 영역을 검증하고 전체 다시 그리기를 요청한다.
- [x] 타입 표·미션·글꼴 검사 장면을 Renderer에 제출한다. 글꼴 **18개·4,608글리프·302,480픽셀**, 실제 글꼴 창 **786,432픽셀**이 독립 판독기와 일치한다. 변화 없는 뒤 4프레임에서 출력하지 않는다.

**완료 범위:** 원본 `004994b0` 전체 함수의 완전 복원은 아니다. 원본의 Squid·지형·Gump 목록 수집, 실제 깊이/프레임·소유자 변환표 선택, 객체별 `00498220`, 구름·카메라 복사는 2·3단계에서 연결한다. 원본 `qsort`의 동률 순서는 미확정이어서 안정 정렬로 둔다. 검사 장면은 기본 프레임을 제출한다. 실제 게임 월드/메뉴 복원 전까지 `InspectView`는 Renderer에 목록을 공급하는 어댑터로 남는다. [근거·검사·제한](exe/cpp-renderer-reconstruction.md).

### 2단계 — 메뉴 탐색·브리핑·정적 미션 진입 연결 완료 (2026-10-05)

- [x] Gump의 입력·UberGump·UI State와 HTML 본문/조건 부분을 Renderer·원본 글꼴에 연결했다. 타이틀·정적 구름 타일·돌 질감·캡처/뗌·목록 누름을 구현했다.
- [x] 원본 메뉴 파일에서 캠페인·제목·잠금/완료 표시를 읽고 선택·Back·종료를 연결했다. 원본 옵션 키 저장, 실제 창 해상도 변경, 재실행 유지도 검사했다.
- [x] 스크립트/요새 로드→원본 A./A1. 브리핑·정지→확인→정적 미션 표시→취소/메뉴 복귀를 연결했다. 1-1과 튜토리얼 1의 MORE/BACK을 실제 클론 창에서 검사했다.
- [x] `Client::ClearFullScreenState`를 UberGump의 원본 메인 버튼 활성 사건(`004cebf0`)에 연결했다. 오른쪽 클릭/일반 종료에서는 지우지 않는다.
- [x] **3단계 기본 연동:** 저장 Tutorial 미션의 실제 지형/객체·시작 상태·선택/이동·카메라·정지로 교체했다. 전체 생성자/SID/미션 종류 복원과 게임 플레이 완주는 후속이다.
- [ ] **원본 UI 전체:** 정확한 패널/하위 메뉴 배치·구름 애니메이션·전체 StyleText·이미지/링크/스크롤·Help·기술·팁·자동 데모·음향 등. 이번 연결은 Gump/State/Mission 전체 복원을 의미하지 않는다.

근거: [메인 메뉴](screens/main-menu.md), [메뉴 버튼](videos/menu-buttons-20261003.md), [미션 스크립트](formats/mission-script.md). dotnetpj의 UI/캠페인 기록은 분석 참고이며 cpppj 완료 근거로 세지 않는다.

새 실행은 `cpppj/build/bin/Release/NetstormCpp.exe --run originals --window`다. **기본 메뉴 탐색과 3단계 기본 지형/선택·이동 경로를 완료했다.** [메뉴의 미완료 UI](exe/cpp-menu-reconstruction.md), [월드의 근거·검사·남은 복원](exe/cpp-world-reconstruction.md).

### 3단계 — 저장 미션의 기본 월드·조작 연결 완료

- [x] TerrainBuilder의 연결 통로·청크별 시드 성장·빈틈 보정, 실제 본섬/절벽·완전한 저장 받침 표시와 공유 지지/점유 격자.
- [x] 미션의 시작 SP·지식 입력/허용 표·동맹·색, 저장 객체의 좌표·프레임·수량·내용물/작업장 상태 보존. 1-1은 저장 Money 100000 대신 미션 3000 SP, TEST01은 50000 SP를 적용한다. 생산 덱은 아직 재구성하지 않는다.
- [x] Kernel 소유 월드·해제 순서, 실제 객체 목록 제출. 몸통 좌클릭 선택→땅 좌클릭 이동→선택 해제, 우클릭 메뉴·허공 거부·화면 이동·홈 보기·일시정지/재개·메뉴 복귀/재진입.
- [x] 사제 타입 속도 1.8칸/초와 방향 A~H의 8프레임 그림. 8방향 경로·대각선 모서리 제한·부드러운 이동 어댑터. 원본 경로 알고리즘/정확한 애니메이션 주기 전체 복원은 아니다.
- [x] 69개 단위 검사, 초기 자료 6개 사례·393216마스크 바이트·2593객체의 독립 대조, 1-1/TEST01 클론 창 20개 조작 상태 검사. 원본 게임 실행 없이 자료 해시/설정 복구를 확인한다.
- [ ] **3단계 전체 복원:** 원본 생성자/가상 함수·SID/세대/좌표 해시·파생 프로세스·내용물 객체화·Battle/Normal 재배치·동적 지형·덱·그림자·정지 객체 애니메이션·상황별 커서·경로 재탐색/충돌/낙하 등.
- [ ] **다음 구현:** 4단계 다리/배치·건설→생산 덱·에너지·채집→경제·전투·AI·포획/희생·승패/결과·소리로 연결해 1-1 완주. 현재 미션은 완주할 수 없다.

### 4단계 첫 부분 — 다리 계산 복원·검증 완료, 배치/붕괴 연결 남음

- [x] 패치/CD 각각 26개 다리 표와 판본별 가중치(합계 287/306), CanonDecoder 회전·프레임 반복, 원본 난수의 다음 상태/결과를 대조했다.
- [x] Bridge 한 방향/전체 열린 끝과 수명 비트/제거 플래그의 접두 부분을 복원했다. 원본 표면 조회의 `0.9999f` 편향과 x87 중간 정밀도를 경계 입력으로 확인했다.
- [x] 새 x86 18,053개 입력 사례, 두 판본 합계 936셀 자산 비교, 원본 1,819파일 보존. 함수 오대응 수정과 패치 9개/CD 8개 함수의 자료형 디컴파일. [상세 범위·한계](exe/cpp-bridge-reconstruction.md).
- [x] 표면 연결 계산·정수 발자국/flag 8 이웃·`004218b0` 붕괴 방문 목록을 복원하고 새 x86 3,450개 사례로 검증했다. 실제 월드 표면/SID·spot 연결은 남았다. [범위·검사](exe/cpp-surface-reconstruction.md).
- [x] SquidHash 4단계 배열 초기화/주소·객체 단계와 Squid의 발자국/지붕 유효 genus를 복원했다. 새 x86 4,632개 입력, float 경로의 x87 53/64비트 결과를 대조했다. [표면 배열의 실제 관계·남은 등록/해제](exe/cpp-hash-reconstruction.md).
- [x] Pop/Unpop의 next 체인·spot OR/AND·좌표 캐시·비전투 상태·CD판 충돌 차이와 실제 등록 지도→정수 SurfaceFinder 전달을 복원했다. 새 878개 전이는 가상/영역 효과를 계약 대체한 조건부 기계어 검증이다. [범위·검사·후속](exe/cpp-spatial-reconstruction.md).
- [x] SID raw 풀·판본별 번호 경계·FIFO/예측 할당·void 반납·타입 보존/삭제 기록·서버 목록 재구성. 두 판본 합계 3,168회 기계어 대조이며 기존 메모리/소진 전 경로만 검증했다. 이 경로에는 세대 비트가 없다. [범위·제한](exe/cpp-sid-reconstruction.md).
- [x] 생성자 주소 표·constructor=0 base 생성·postCreate/postTake·free/void Take를 SID 풀에 연결했다. 새 964회 x86 대조와 별도 359타입 주소 표 대조. [원본 판본 차이·검증 제한](exe/cpp-creation-reconstruction.md).
- [x] 자산 파생 생성자 153개·타입 연결 179개를 추가했다. 새 6,028회는 실제 ctor/공통 초기화/free·void Take 대조다. [남은 가상/공간 수명](exe/cpp-derived-reconstruction.md).
- [x] raw SID 풀·해시·spot의 표시 비활성 일반 Unpop과 non-void Take·공통 firstPop 플래그·void 반납을 연결했다. 새 3,618회는 대체 함수 없는 기계어 대조이며 실제 Pop/파생 삭제·월드는 미연결이다. [CD 미발견 차이·151행 vtable 표·남은 효과](exe/cpp-unpop-reconstruction.md).
- [x] raw SID의 일반 Pop→실제 firstPop·비전투 Activate·억제된 공통 postPop→Unpop·반납을 연결했다. 표시/postPop 효과 억제·합성 타입/SHP 범위이며 GameWorld는 미연결이다. [원본 기계어·디컴파일 비트 비교 정정·이번 GUI 검사](exe/cpp-pop-reconstruction.md).
- [x] raw 공통 update88/update8c의 프레임 경계·선택·그림자 갱신을 표시 활성 상태로 복원하고 기존 Renderer의 dirty 표에 연결했다. 새 x86 1,728회와 raw→Renderer 부분 픽셀 검사, 실제 SHP 헤더 6,950개 읽기를 확인했다. [판본 차이·근거·남은 연결](exe/cpp-display-reconstruction.md).
- [x] 공통 postPop 비용 집계·공급/소유자별 작업장 목록·타입 통계·noGraph 리셋을 raw Pop에 선택 연결했다. 새 x86 984회와 비유한 비용/손상 목록·미지원 그래프/AI/배치 선택의 변경 전 거부를 확인했다. [판본 비용/리셋 차이·범위](exe/cpp-postpop-reconstruction.md).
- [x] Graph의 정수 표면 연결·LIFO flood·최대 무리 병합·할당/반납/감소를 복원했다. 새 x86 2,560회와 다중 칸 객체 수 1·WORD 감김·손상/소진 보호를 확인했다. [근거](exe/cpp-graph-reconstruction.md).
- [x] raw SID·실제 FrameCode·0단계 해시·spot 스냅샷과 graph byte 반영을 공통 postPop/지원 Pop에 연결했다. 새 x86 1,536회와 Pop 최종 상태의 사전 검증·기존 스택 유지·다른 풀/지도/타입 연결 거부를 확인했다. 정상 다리/섬의 영역 효과·raw 월드는 남았다. [범위·재현](exe/cpp-rawgraph-reconstruction.md).
- [x] 주변 영역 무효화→Add와 일반 다리/섬 비전투 Pop을 연결했다. 새 10.37을 전체 디컴파일하고 세 실제 PE×두 x87 정밀도의 새 1,536회, 체인 순환/영역 이후 소진의 변경 전 거부를 검사했다. CD/10.37은 동일 코드 배치다. [범위·재현](exe/cpp-regiongraph-reconstruction.md).
- [x] 일반 다리/섬 비전투 Unpop과 삭제 준비의 일반/rebuild Graph 분할을 복원했다. 새 세 PE x86 1,158회, x64 Release·CTest 내부 137개·실패 0. [0단계 위치 조회·경계 접촉 해석·검증 범위](exe/cpp-graphremove-reconstruction.md).
- [x] 같은 위치의 다른 정상 표면 SID에서 조회한 graph와 삭제 원천의 타입/프레임을 분리했다. 10.62/10.82·DevLog를 정적으로 비교했고 10.78 기준을 유지했다. 새 x86 384회·Release/콘솔 CTest 내부 139개·실패 0. [근거/재현](exe/cpp-reference-versions.md).
- [x] 전역 Graph의 무효 수집/전체 재구성과 소진 할당 API를 전체 raw 풀에 연결했다. 새 세 PE×두 x87 정밀도 288회·Release/콘솔 CTest 내부 143개·실패 0. Add/Detach/Pop 자동 연결·SID 소진 복구는 후속이다. [원본 순서·제약·재현](exe/cpp-graphrebuild-reconstruction.md).
- [x] 전체 자산 풀 계약 `GraphRecovery::FullPool`에서 Add/Detach/Pop·공통 postPop의 내부 번호 소진에 전역 재구성을 연결했다. 실제 상위 x86 192회·Release/콘솔 CTest 내부 145개·실패 0. 새 좌표로 Pop 전 재구성을 예측하고 다른 void 표면/내부 타입의 손상을 공간 변경 전에 거부한다. [원본 순서·계약·재현](exe/cpp-graphrecovery-reconstruction.md).
- [x] 다리 붕괴 스캔 `00422bc0`(프레임당 `trunc(delta / 10 × 범위)`개, 주기 끝 일괄 처리)·한 칸 처리 `004227e0`·수명 감소 전체 `00421c30`·금 간/보통/약화/복구 프레임 전환·다리 destroy 재정의 `004220f0`을 복원했다. 세 실제 PE×두 x87 정밀도의 새 x86 14,524개·Release/콘솔 CTest 내부 152개·실패 0. 실제 삭제·낙하·소리는 사건으로만 돌려준다. [식·순서·대체 범위·재현](exe/cpp-bridgedecay-reconstruction.md).
- [x] raw 다리 preDestroy/postDestroy의 주변 연결 참조 검사·좌표 보정·효과 순서와 지연 낙하 payload 포장을 복원했다. 일반 탐색과 실제 destroy/fall/base/소리는 호출자가 공급하는 계약이다. 새 x86 6,170개·Release/CTest 내부 157개·실패 0, 이번에 허용된 클론 창의 캠페인/TEST01 회귀 20개 상태 통과. [기계어에서 찾은 좌표 보정·제한·재현](exe/cpp-bridgeeffects-reconstruction.md).
- [x] 일반 Begin/Next의 네 단계 해시/next·기본 true 필터·소수 좌표 발자국/높이 확장·전체 커서·동적 필드 조회를 복원하여 raw 다리 삭제 훅에 연결했다. 새 x86 1,776개·Release/콘솔 CTest 내부 163개·실패 0. 이번 `DESKTOP-HJOW` 작업은 창 실행 없음. [근거·합성 등록/외부 효과의 대체 경계](exe/cpp-rawfinder-reconstruction.md).
- [x] 공통 destroy의 dead·훅 깊이·종속 release 전 next 저장·전파/선택·실제 Unpop/SID 반납과 다리 훅 내부의 링크 삭제를 복원했다. 새 x86 1,992개·Release/콘솔 CTest 내부 169개·실패 0. 공통 pre/post 전체·종속 파생 메서드·파편/소리/낙하는 대체 범위다. [원본 flags 정정·순서·대체 경계](exe/cpp-destroy-reconstruction.md).
- [x] 공통 pre/postDestroy의 선택/abstract 배치 복구·공급/작업장/소유자 목록 제거·현재 타입 수 감소·누적 생산 수 유지·unitLost 사건/좌표·판본별 비용 차감을 공통 destroy의 실제 Unpop/SID 반납에 연결했다. 새 x86 1,728개·Release/콘솔 CTest 내부 175개·실패 0. Graph 비활성·AI null·보상/SP·소리/UI 대체 범위다. [장부 순서·비용 차이·보상 함수 정정·재현](exe/cpp-destroylifecycle-reconstruction.md).
- [x] 공통 삭제의 Graph 분할/직접 Free와 주변 표면 Add를 실제 destroy/Unpop/SID 반납에 연결했다. 새 x86 1,728개·CTest 내부 182개·실패 0. [근거](exe/cpp-destroygraph-reconstruction.md).
- [x] 삭제/해체 보상(`0044c2f0`/CD `004626d0`)의 비용 곡선·수신자 지급·남은 HP 비례 환불·AI 지갑·샘 풀과 패치판 SP 저장소·전역 난수 소비를 복원해 삭제 장부에 연결했다. 새 x86 7,200개·Release/콘솔 CTest 내부 191개·실패 0. SP 표시·시각 재시드·힙 할당·assert 보고·파생 vtable +0x80 개수만 대체한 범위다. [판본 차이·검증 범위](exe/cpp-reward-reconstruction.md).
- [x] 객체 부착 프로세스: BaseProcess 생성자의 Kernel 등록·ProcessForm 생성/부착·양방향 종속 체인, form의 공통 destroy와 부모 삭제 때의 종속 정리, RegularProcess의 실행/재예약/종료와 이벤트 조회를 복원하고 다리 지연 낙하(`0x2692`) 예약/조회를 올렸다. 새 x86 시나리오 360개·연산 5,893개(보강 재생성)·Release/콘솔 CTest 내부 205개·실패 0. 부모의 가상 pre/post/이벤트 처리기·new/free·로그·전파만 대체한 범위다. [구조·판본 차이·검증 범위](exe/cpp-process-reconstruction.md).
- [x] 다리 이벤트 처리기(vtable +0x5c)와 끝 칸 변환: `0x2691` 자기 destroy, `0x2692` 지연 낙하 payload(`_ftol` 하위 DWORD의 x/y)를 풀어 현재 프레임 J/K의 방향 비교로 끝 칸 L·M·N·O를 고르고 임시 프레임 이웃 탐색→수명 4→단단한 옛 프레임 검사→새 객체 교체를 복원했다. 방향 글자별 첫 프레임 구간 표(`0049b060`/CD `00444da0`)도 복원했다. 새 x86 4,653개·Release/콘솔 CTest 내부 205개·실패 0. 이웃 탐색·표면 알림·생성·destroy·소유자·Pop은 훅 대체이며 raw GameWorld에 연결하지 않았다. [근거·한계](exe/cpp-bridgeevent-reconstruction.md).
- [x] 소유자 지정(base Squid vtable +0x74, `004adf00`/CD `004aefa0`): 소유자 바이트 쓰기와 요새/전투 모드에서 vortex·factory 타입의 소유자별 작업장 목록 이동(이전 소유자에서 모두 제거, void·buried가 아니면 새 소유자에 추가)을 복원했다. 새 x86 3,000개(대체 함수 없음)·Release/콘솔 CTest 내부 211개·실패 0. 다리 이벤트 처리기는 가상 표 기록값으로 분배해 Regular 프로세스에 이었다(단위 검사). [근거](exe/cpp-owner-reconstruction.md), [처리기 연결](exe/cpp-bridgeevent-reconstruction.md).
- [x] 2026-10-08 후속: [flag 0 첫 연결 이웃·다리 postPop 접두·끝 칸 raw 통합](exe/cpp-neighbor-reconstruction.md)을 완료했다. 새 제한 x86 **1,968개**(이웃/이벤트 1,728 + 접두 240)·Release 경고/오류 0·CTest 내부 **218개·실패 0**(CTest 전체 86.80초), 누적 **111,754개**·감사 **27개 모두 통과**다. 실제 생성/소유자/삭제/Pop과 실행 중 Regular 정리를 콘솔에서 연결했다. **다리 연결 객체/소유자 전파 함수는 콜백 경계이며 GUI raw GameWorld·Kernel↔게임 루프·Graph 활성 통합·실제 낙하/파편/소리·건설/경제/전투/승패는 남았다.** 게임/복사본·클론 창 실행 없음.
- [x] 2026-10-08(`VM-W11-CODEX`): [다리/섬 연결·연결 객체 생성·소유자 전파](exe/cpp-bridgeconnect-reconstruction.md) — 연결 순회 `004213b0`, 연결 객체 생성 `004210f0`, 소유자 전파 `00421240`, 한 칸 타입 조회, 소유자 색 프레임, 섬 받침·종유석의 소유자 지정과 섬 받침 postPop 재정의, 탐색기 flags 0~7과 Next 순회. 새 x86 10,800개. 다리 postPop의 연결 콜백 경계를 실제 생성/소유자 지정/Pop으로 이었고 섬 받침 Pop(종유석 생성)도 지원한다. 앞 단계 이웃 필터의 spot 칸 보정(+0.9999)을 정정했다.
- [x] 2026-10-08: [Squid 프레임 지정](exe/cpp-setframe-reconstruction.md) `004acee0` — 현재 프레임의 해시 단계가 저장값과 같으면 표시 갱신 두 번, 다르면 Unpop → Pop(flags | 0x50). 새 x86 5,772개. 표시 갱신은 훅이다.
- [x] 2026-10-08: [섬 받침 preDestroy·noIsland postDestroy](exe/cpp-islandlifecycle-reconstruction.md) `00442240`·`00442640` — 섬이 사라질 때 닿아 있던 다리에 지연 낙하 예약, 표면 칸이 사라질 때 그 칸의 walker 낙하. 새 x86 1,560개. 세 단계 합쳐 CTest 내부 234개·실패 0, 누적 129,886개. 이 PC에서 클론 창 회귀 4종도 통과했다.
- [x] noIsland postPop 재정의 `004423b0`/CD `004d2790`: 3×3 프레임 선택·F 받침/종유석 생성·H 받침 소유자 전파·연결 순회와 공통 postPop 연결. 새 x86 1,656개·두 판본 실제 raw 모듈 통합·CTest 240개 통과. [근거와 범위](exe/cpp-islandpostpop-reconstruction.md).
- [ ] 섬 삭제 → 지연 낙하 → 다리 끝 칸 변환 → 연결 객체 정리의 한 흐름 통합.
- [ ] AI 부착 통지·SID 소진 복구·상위 Take 목록/카운터·참조 수명·SharedRegular와 다른 파생 프로세스·DependForm/GumpForm/ContentForm의 가상 메서드·건물 부착 효과·파생 삭제/의존 객체/반납 수명, form/process를 포함한 실제 전체 풀 재구성 계약·AI/배치 선택·생산 덱/자원/SP 차감·dirty/grid·표면 부착 프레임/변경 알림을 복원한다. 일반 raw Pop/공통 표시/비용·목록·통계/표면 Graph/영역은 연결했으며 섬/지면/noIsland/받침 생성·등록과 소수 좌표/일반 공간 탐색을 GameWorld에 연결한다.
- [ ] 실제 배치 함수 `0049b510`의 다리 경로·법적 위치 마스크, Construction 생성과 표면 지도/spot을 연결한다(`00421240`/`004213b0` 소유자 전파는 위 2026-10-08 항목에서 복원했다. 그 미리보기 경로의 호출자 `00445d87`은 배치 쪽이다). 앞서 배치로 적은 `004215d0`는 폭발 뒤 끝 칸 변환으로 정정했다.
- [ ] 붕괴 사건을 raw GameWorld에서 실제 효과로 소비한다. 일반 탐색·공통 destroy의 실제 Unpop/반납·공통 장부/Graph·보상 계산·이벤트 프로세스/끝 칸 변환은 raw 계층에서 복원했다. flag 0 첫 연결 이웃과 실제 생성/삭제/Pop의 콘솔 통합도 [후속 작업](exe/cpp-neighbor-reconstruction.md)에서 이었다. GUI 월드 연결, 다리 postPop의 연결 객체/소유자 전파, AI 통지·종속 파생 효과·파편 생성(기존 NotifyRemoval)·walker 가상 낙하·소리 출력은 남았다. 다른 파생 탐색 필터·다리 칸 공급/커서·회전·배치 UI도 남았다.
- [ ] Construct→템플/워크샵·생산 덱·에너지→채집/SP→경제·전투·AI·포획/희생·승패/결과·소리. 현재 다리는 계산 계층이며 플레이 기능 연결 완료가 아니다.

### 3·4단계의 주의점

읽은 `.fort` 레코드의 기본 월드 생성과 소유자·동맹·시작 SP는 연결했다. 기술 입력/허용 표와 저장 Deck을 보존하지만 생산 목록/워크샵·동적 기술 갱신은 아직 없다. 타입의 숫자 속성을 읽는 것만으로 생성자·전투 수치·요구 에너지까지 복원됐다고 판단하지 않는다.

SID·파생 프로세스·객체 수명·좌표 해시·이동 경로/충돌, 섬 지면/절벽/받침과 실제 목록/깊이, 원본 난수 소비·요소별 프레임/타이머·일시정지·갱신 순서를 복원한다. Renderer에 공급할 실제 객체의 그림자/소유자 표 선택도 이 범위다.

1-1 완주는 다리·생산·건설·경제뿐 아니라 적 AI·사제 포획/운반/희생·승패·결과·재시작·메뉴 복귀·소리까지 포함한다. 임시 방어 AI를 원본 전략 복원 완료로 처리하지 않는다. 입력·난수·시각을 고정한 자동 검사와 실제 GUI 시나리오에서 위치·SP·HP·사건 시각을 비교한다. 원본에 필요한 조작과 게임 화면이 모두 연결되기 전에는 플레이 가능으로 표시하지 않는다.

## 검사와 기록

**2026-10-08 `VM-W11-CODEX`:** 사용자가 이 PC에서는 창을 띄우는 작업을 허용했다(AGENTS.md의 게임 구동 확인 면제 시스템이기도 하다). 아래 window/renderer/menu/world 스모크 4종을 실행했고 모두 통과했다(각 8~26초, 원본 설정 파일 복구 확인). 원본 게임은 실행하지 않았다. 이 허용은 이 PC에만 해당한다.

**2026-10-07 사용자 지시(`DESKTOP-HJOW`):** 그 PC에서 원본/복사본 게임 실행과 창 검사를 금지한다. raw 월드 연결 후 GUI 검사는 다른 PC에 인계한다.

**2026-10-06 당시 기록:** 당시 PC에서는 창 검사를 허용하여 아래 window/renderer/menu/world 스모크를 실행했다. 원본/복사본 게임 프로세스는 실행하지 않았다. 기존 GUI 월드와 새 raw SID 경로는 아직 별개다. SID 3,168회는 malloc/소진 복구·파생 수명·월드 연결을, 일반 해제 3,618회는 당시의 실제 Pop/표시 활성/파생 삭제를 증명하지 않는다. [raw Pop 후속·이번 실행 검사](exe/cpp-pop-reconstruction.md)의 범위와 구분한다.

기본 검사는 [C++ 빌드 규칙](cpp-build.md)과 [기존 로드맵의 복원 절차](cpp-roadmap.md#디컴파일-신뢰도를-높이는-작업-방식)를 따른다. 현재 **240개 내부 검사**, 제한 x86 **131,542개 기대값 입력 사례**(2026-10-08의 다리/섬 연결 10,800＋프레임 지정 5,772＋섬 삭제 훅 1,560＋noIsland 최초 등록/받침 소유자 1,656을 더한 값이다. 그 앞까지는 111,754개: 기존 5,365＋다리 18,053＋표면/재귀 3,450＋해시/점유 4,632＋공간 등록 878＋SID 3,168＋base 생성/Take 964＋자산 파생 생성/Take 6,028＋raw 일반 공간 해제/수명 3,618＋raw 일반 Pop/수명 4,600＋공통 표시 활성 1,728＋공통 postPop 일부 효과 984＋Graph 2,560＋raw Graph 연결 1,536＋영역/일반 다리·섬 Pop 1,536＋삭제 준비/일반 Unpop 1,158＋같은 위치 조회 384＋전역 재구성 288＋상위 소진 복구 192＋다리 붕괴 스캔/한 칸 처리 14,524＋다리 삭제 훅/낙하 좌표 6,170＋일반 탐색/삭제 훅 연결 1,776＋공통 삭제/다리 중첩 1,992＋공통 삭제 장부 1,728＋공통 삭제 Graph 1,728＋삭제 보상(SP)·SP 저장소 7,200＋객체 부착 프로세스 5,893＋다리 이벤트 처리기/끝 칸 변환/글자 표 4,653＋소유자 지정 3,000＋flag 0 첫 이웃/이벤트 1,728＋다리 postPop 접두 240)다. 공간 등록 878개는 가상/영역 효과를 계약 대체한 조건부 검증이다. 앞선 Pop은 표시/postPop 효과 억제, 표시는 공통 표시 활성/postPop 효과 억제, postPop은 공통 표시/비용·목록·통계·noGraph 리셋 활성·비전투·AI 미부착/배치 선택 없음·합성 타입/SHP 범위다. 최신 삭제 준비/Unpop은 dead 원천·0단계 위치 조회·일반/rebuild 분할 정책·반복 해제/반납을 검사했다. 영역 Graph는 실제 SID·프레임·4단계 해시·spot·공통 postPop/지원 Pop 연결이며 정수 좌표·기존 표/스택·소진 전·비전투 dirty 큐 null·합성 타입/FrameCode/SHP 범위다. 새 전역 Graph는 전체 32768 raw 풀의 재구성/소진 할당을 검사했고 전체 자산 풀 계약의 상위 자동 복구 192회를 추가했으며 form/process·실제 raw 월드 계약은 남았다. 세 PE 중 CD/10.37은 같은 코드 배치다. 비용 인코딩은 접두 구간이며 생성자 359행/vtable 151행·준비 Reset/Create·내부 도달은 상위 호출 수와 별도다. 수명 480개는 접두 구간, 해시 초기화 4개는 할당 없는 경로이며 전체 열린 끝은 패치판만 검증했다. 메뉴/월드는 기존 임시 월드의 부분 동작·독립 자료 비교·클론 창 조작 기록이며 raw GameWorld 연결·원본 메뉴/월드 전체 기계어 대조는 아니다. 이 수치를 게임 전체 완성도로 해석하지 않는다. 일반 탐색은 실제 Begin/Next와 커서 전체를 대조했으며 합성 등록 상태·기본 true 필터·외부 삭제 효과 대체 범위다. 파생 필터와 raw GameWorld는 후속이다. 이전 공통 삭제 1,992개는 실제 destroy/다리 훅/finder/Unpop/반납·삭제 기록이며 공통 pre/post·종속 파생 효과·전파/선택·파편/소리/낙하는 명시적 대체다. 표시 억제·합성 등록/타입/SHP 범위이며 당시 공통 훅은 대체였다. 최신 공통 장부 1,728개는 실제 pre/post·목록·통계·비용과 destroy/Unpop/Release를 대조했으며 Graph 비활성·AI null·보상/SP·실제 소리/UI 대체·raw GUI 월드 미연결 범위다. 이후 공통 삭제 Graph 1,728개(실제 Graph 분할/Free/주변 표면 Add)와 삭제 보상 7,200개(실제 SP 지급/AI 지갑/샘 풀, 패치 SP 저장소 포함)를 더했으며 raw GUI 월드는 여전히 미연결이다. 그 뒤 객체 부착 프로세스 5,893개(실제 BaseProcess/Regular 생성·Kernel·form 부착/해제·공통 destroy)와 다리 이벤트 4,653개(실제 이벤트 처리기·끝 칸 변환·글자 표, 이웃 탐색·표면 알림·생성·destroy·소유자·Pop은 계약 대체)를 더했다. 이어 소유자 지정 3,000개(대체 함수 없음)를 더했다. 후속의 첫 이웃/이벤트 1,728개는 실제 일반 탐색·파생 연결 필터를 사용하지만 생성/삭제/소유자/Pop은 대체다. 다리 postPop 240개는 접두만 실행하며 연결 생성/소유자 전파와 공통 함수는 호출 기록 대체다. 실제 raw 모듈 통합은 별도 콘솔 검사이고 GUI 연결은 남았다.

```powershell
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
python tools/cpp_assets_smoke.py
python tools/cpp_window_smoke.py
python tools/cpp_renderer_smoke.py
python tools/cpp_menu_smoke.py
python tools/cpp_world_smoke.py
python tools/decomp_renderer_oracle.py
python tools/cpp_bridge_smoke.py
python tools/decomp_bridge_oracle.py
python tools/decomp_surface_oracle.py
python tools/decomp_hash_oracle.py
```

현재 PC에서는 창 검사를 실행하지 않는다. 아래 창 명령은 다른 PC 인수인계용이며 새 `NetstormCpp.exe`만 실행한다. 원본 게임 실행·허용 예외 파일·원본 보호 규칙은 AGENTS.md대로 따른다. 스모크는 두 설정 파일을 보관하고 성공/실패 모두 복구한다. 결과와 추출물은 `extracted/`에 둔다. 새 코드의 상수·함수·반복문은 한국어 주석, 새 문서/코드는 UTF-8을 사용한다.

단계가 끝나면 LEFT_JOBS.md의 해당 작업을 완료 표시한다. 중단/인수인계 때는 변경 파일·검사·남은 연결점·다음 항목을 남긴다. AGENTS.md는 수정하지 않는다.
