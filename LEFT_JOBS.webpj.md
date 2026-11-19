# LEFT_JOBS.webpj — 현행 상태와 남은 작업

현재 진행 상황, 남은 단계와 유효한 결정을 기록한다. 단위별 구현과 검증은 `docs/webpj-*.md` 및 [PORT_MAP.tsv](webpj/PORT_MAP.tsv), 지난 인계 원문은 [E801 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801)을 참조한다. 공통 사항과 cpppj는 [LEFT_JOBS.md](LEFT_JOBS.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)를 따른다.

## 0. 한눈에 보기

### 0.1 현재 값 (2026-11-20, 마지막 단위 WB04-E981)

| 항목 | 현행 상태 |
| --- | --- |
| 실행·플레이 | 브라우저에서 ‘클릭하여 시작’, 메인 메뉴·캠페인/데모 목록·도움말·옵션·Credits와 개발용 지형 미리보기가 동작한다. Node 통합 검사에서 실제 Fort로 Session을 적재하고 내부 Login·Start·일부 이동과 건설을 확인했다. 브라우저의 `MissionBegin`·`StartDemo`에서 실제 월드로 들어가 플레이하는 기능은 미연결이다. |
| 원본과의 동일성 | 영어 4:3 메뉴 M1 8장면은 cpppj 오라클과 알려진 구름 배경 등 차이 영역 밖에서 픽셀 차이 0이다. 월드 부품은 원본 10.78 x86 fixture 또는 독립 계산으로 대조했다. 실제 미션 L2·L3·L4, 자연 완주는 미검증이다. |
| 현재 작업 | **M2의 4-D 진행 중.** 앱의 공통 시계·난수·SP·Player·Template·월드 저장소와 Guide 생성/반납을 연결하고 독립 Session 적재를 확인했다. E893에서 내부 미션 전송의 실제 Session 수신 콜백 대여와 State 정리 관문을 연결했다. [내부 전송 콜백](docs/webpj-session-internal-callback-20261119.md) E892에서 Fort 이름의 원본 관측 876행 전부를 물리·콜백 변이·중단과 호출 순서로 대조했다. [전체 Fort 관측](docs/webpj-fort-name-observation-20261119.md) E891에서 manager 없는 편집 설정 경계 20행을 추가했다. [Fort 관측](docs/webpj-fort-name-editor-observation-20261119.md) E890에서 앱 프레임의 시각 고정 뒤 Guide를 호출해 실제 Tutorial1의 단계·지연 요청 관문을 확인했다. [앱 Guide 프레임](docs/webpj-app-guide-frame-20261119.md) E889에서 앱의 실제 Client 전역과 State7 부모·State8 내부 접속을 같은 Guide/World에 연결했다. [앱 State7](docs/webpj-app-state7-20261119.md) E888에서 앱이 같은 Guide의 Fort로 GameWorld를 소유하고 로컬 로그인·반납하는 경계를 검사했다. [앱 GameWorld](docs/webpj-app-game-world-20261119.md) E887에서 Guide 반복기를 실제 창·State 대기 조건과 Tutorial Init에 결합했다. E886은 Fort 요청을 Session 로컬 로그인까지 검사했다. [Guide 반복기](docs/webpj-app-guide-loop-20261119.md) E941에서 Chromium의 실제 갈무리11·확정 문자 큐·Gump 픽셀 출력을 검사했다. 실제 OS IME와 미션 앱 연결은 남았다. [갈무리11 편집 검증](docs/webpj-client-text-entry-galmuri-20261119.md) E940에서 실제 Gump 배경·픽셀에 혼합 글꼴과 선택 영역 출력을 연결했다. 앱 렌더러 연결은 남았다. [편집 글자 출력](docs/webpj-client-text-entry-draw-20261119.md) E939에서 원본 FontSet과 확장 문자 글꼴의 혼합 폭을 편집기에 연결했다. 앱 글꼴 인스턴스·실제 출력은 남았다. [편집 글꼴 측정](docs/webpj-client-text-entry-measure-20261119.md) E938에서 실제 TextEntry·Console의 버퍼·두 Slider·공통 입력/idle/수명을 연결했다. 앱 글꼴·출력과 미션 UI는 남았다. [Client TextEntry](docs/webpj-client-text-entry-controls-20261119.md) E937에서 실제 Slider·손잡이·돌 화살표를 같은 Client 트리와 입력·수명에 연결했다. TextEntry·출력은 남았다. [Client Slider](docs/webpj-client-scroll-controls-20261119.md) E936에서 같은 Client의 돌 버튼·배경 생성·입력·idle·실패 정리를 연결했다. Slider·출력은 남았다. [Client 돌 버튼](docs/webpj-client-stone-controls-20261119.md) E935에서 같은 공통 수명의 파생 idle 분배를 연결했다. TextEntry 공장·앱 프레임은 남았다. [Gump idle](docs/webpj-app-gump-idle-20261119.md) E934에서 직접 beforeinput과 조합 후속의 중복을 구분하고 영어·한국어 접근성 이름을 적용했다. OS IME·미션 연결은 남았다. [직접 텍스트](docs/webpj-browser-beforeinput-20261119.md) E933에서 실제 편집 가능한 IME 요소의 포커스·큐·앱 수명을 연결했다. 실제 OS IME와 미션 TextEntry는 미검증이다. [IME 요소](docs/webpj-browser-ime-element-20261119.md) E932에서 Canvas 조합 확정 사건을 원자적 문자 큐에 연결했다. 실제 OS IME용 편집 요소·포커스는 남았다. [조합 사건](docs/webpj-browser-composition-events-20261119.md) E931에서 영문·1252 및 기본 제어 키의 원본 문자 사건을 큐에 연결했다. IME·실제 입력 소비는 남았다. [문자 키](docs/webpj-browser-character-key-20261119.md) E930에서 한글 Console 자동완성이 확장 토큰을 한 문자로 탐색한다. 실제 Console 장치는 남았다. [Console 단어](docs/webpj-console-korean-word-20261119.md) E929에서 확장 줄의 화면 잘라내기·선택·캐럿 경계를 보정했다. 실제 글꼴·IME 연결은 남았다. [한글 출력 경계](docs/webpj-text-entry-draw-boundary-20261119.md) E928에서 한글 줄의 수평 스크롤을 완전한 문자 경계로 보정했다. 실제 출력 보정은 남았다. [한글 스크롤](docs/webpj-text-entry-scroll-20261119.md) E927에서 편집 버퍼의 확장 문자 붙여넣기·바이트 제한을 연결했다. 실제 클립보드·출력·IME는 남았다. [한글 붙여넣기](docs/webpj-text-entry-paste-20261119.md) E926에서 원본형 편집기의 한글 태그 삽입·선택·이동·삭제를 연결했다. Paste·출력·IME는 남았다. [한글 편집](docs/webpj-text-entry-korean-20261119.md) E925에서 확장 표시 문자의 앞·뒤·이전·다음 바이트 경계를 구현했다. 실제 편집기 연결은 남았다. [표시 문자 경계](docs/webpj-display-text-boundary-20261119.md) E924에서 한국어 확정 문자 태그·표시 바이트와 원자적 큐 사건을 구현했다. 편집기·IME 연결은 남았다. [확정 문자](docs/webpj-committed-text-input-20261119.md) E923에서 원본 입력 큐의 확정 본문 일괄 삽입과 여유 칸을 구현했다. 실제 문자·IME 연결은 남았다. [일괄 큐](docs/webpj-input-queue-batch-20261119.md) E922에서 방향·기능·보조 키의 가상 키 큐 예약을 연결했다. 문자·IME·실제 미션 소비는 남았다. [가상 키](docs/webpj-browser-virtual-key-20261119.md) E921에서 실제 Canvas 버튼 사건의 원본 큐 적재를 구현했다. 미션 소비자 전까지 비활성이다. [마우스 큐](docs/webpj-browser-mouse-queue-20261119.md) E920에서 실제 앱 폴링·40칸 큐의 포인터 재진입 뗌 보정을 연결했다. 일반 입력·앱 프레임은 남았다. [앱 포인터 경계](docs/webpj-app-pointer-boundary-20261119.md) E919에서 실제 앱의 마우스·보조 키 폴링과 Chromium 입력을 확인했다. 일반 큐·프레임 연결은 남았다. [브라우저 폴링](docs/webpj-browser-input-poll-20261119.md) E918에서 창 안팎과 대화창·포커스 조건에 따른 실제 overlay→방송 관문을 연결했다. 포인터 폴링·앱 프레임은 남았다. [포인터 통지](docs/webpj-app-gump-pointer-notify-20261119.md) E917에서 공통 GumpFrame 방송을 실제 구분선 입력에 분배했다. 브라우저 포인터·앱 프레임 호출은 남았다. [Client 방송](docs/webpj-app-gump-broadcast-20261119.md) E916에서 실제 Client 구분선 생성·부착·입력·자식 연동을 연결했다. 전체 HUD 공장과 자동 방송은 남았다. [Client 구분선](docs/webpj-app-hud-divider-20261119.md) E915에서 공통 트리의 직접 부착·Move를 파생 가상 부착으로 분배했다. 실제 구분선 본체 연결은 남았다. [가상 부착](docs/webpj-app-gump-virtual-attach-20261119.md) E914에서 실제 Client 등록표의 HUD serial·현재 완성 상태 조회를 구현했다. UI 공장·분배는 남았다. [HUD 등록 조회](docs/webpj-uber-root-lookup-20261119.md) E913에서 실제 Gump 등록0 교체와 Client 루트 별칭·파생 입력 분배를 연결했다. 실제 Uber UI 공장은 남았다. [앱 Gump 루트 교체](docs/webpj-app-gump-replace-20261119.md) E912에서 Uber 루트 생성·교체 전체 물리와 호출 순서를 원본 160행에 대조하고 재생성을 검사했다. 실제 하위 UI 공장·앱 연결은 남았다. [Uber 루트 생성](docs/webpj-uber-root-construct-20261119.md) E911에서 Uber 루트의 현재 HUD 자식 뷰와 실제 공통 트리의 표준 배치 등록을 확인했다. 전체 생성·교체와 UI 공장은 남았다. [Uber 루트 뷰](docs/webpj-uber-root-view-20261119.md) E910에서 실제 파생 Gump 노드의 공통 주소·입력·소멸을 배경 생성기로 확인했다. HUD·HTML·글자 저장소는 남았다. [앱 파생 Gump](docs/webpj-app-gump-derived-20261119.md) E909에서 같은 Gump 루트의 기본 자식 부착·입력·지연 삭제와 Client 상세 진단을 연결했다. 파생 UI·월드 투영은 남았다. [앱 Gump 트리](docs/webpj-app-gump-tree-20261119.md) E908에서 실제 Gump 루트·등록·수명·수집·입력을 Client 수명에 연결했다. HUD·진행 창·브라우저 입력은 남았다. [앱 Gump 루트](docs/webpj-app-gump-root-20261119.md) E907에서 State7 battlePal을 실제 구름·난수·파일·Screen·World 색표에 연결했다. 다른 UI 색상·날씨·Renderer·구름 표시는 남았다. [앱 전투 팔레트](docs/webpj-app-battle-palette-20261119.md) E906에서 최초 설정과 이후 쓰기를 같은 Client 구름 전역에 반영했다. 구름 생성·표시와 전체 설정 순서는 남았다. [앱 구름 설정](docs/webpj-app-cloud-options-20261119.md) E905에서 화면비 변경 시 현재 팔레트·loaded 상자·고정 색표를 유지해 Screen DIB를 재생성했다. Renderer·카메라 재배치는 남았다. [앱 Screen 크기](docs/webpj-app-screen-resize-20261119.md) E904에서 Screen DIB 부분 출력을 앱 표면과 실제 Canvas에 연결했다. [앱 Screen 출력](docs/webpj-app-screen-present-20261119.md) E903에서 앱 Screen의 최초 캡처 뒤 창 모드 DIB·전체 clip을 준비했다. 화면비에 따른 Renderer·카메라 재배치는 남았다. [앱 Screen DIB](docs/webpj-app-screen-dib-20261119.md) E902에서 Screen 적용 색상을 실제 브라우저 표시기에 연결하고 색 변경 뒤 재출력을 예약했다. 전체 Renderer와 다른 UI 색상·날씨 연결은 남았다. [앱 표시기 색상](docs/webpj-app-presenter-palette-20261119.md) E901에서 현재 Screen 팔레트와 고정 색표·소유자 표를 실제 Tutorial1 World·Session에 대여했다. 전투 팔레트 전환·표시 연결은 잔여다. [앱 Session 색상표](docs/webpj-app-session-colors-20261119.md) E900에서 실제 TYPE·고정 입력·공유 redraw로 소유자 색표와 9표 메모리 캐시를 초기화하고 Screen 명암 행을 연결했다. 전투 팔레트 및 영구 캐시는 남았다. [앱 색상 캐시](docs/webpj-app-color-cache-20261119.md) E899에서 앱의 실제 fortPal과 Screen 논리 색을 Client 장기 ColorSort primary에 최초 캡처했다. 표시 장치와 전투 팔레트 연결은 남았다. [앱 팔레트 캡처](docs/webpj-app-palette-capture-20261119.md) E898에서 앱 시작과 실제 Tutorial1 State7에 공유 카메라 Renderer 초기화를 연결했다. [앱 카메라 초기화](docs/webpj-app-renderer-init-20261119.md) E897에서 앱의 Login 설정·시계·Player 관문·화면 갱신을 실제 Tutorial1 연결에 제공했다. [앱 Login 장치](docs/webpj-app-login-services-20261119.md) E896에서 앱의 실제 번역·설정·제목을 Session 설치에 연결하고 Player 이름의 표시 바이트를 cpppj와 대조했다. [앱 설치·표시 바이트](docs/webpj-app-install-display-20261119.md) E895에서 기존 앱 World의 로컬 State Poll·패킷 Pump를 Guide 앞에 연결해 실제 Tutorial1 로그인 Ready를 앱 프레임으로 확인했다. [앱 연결 프레임](docs/webpj-app-connection-frame-20261119.md) 앱 메뉴의 지연 요청 소비·로그인 완료 후 화면 설치·Kernel 프레임, 실제 Fort Snapshot/Notify 및 Tutorial1 L2가 남았다. [Tutorial Init](docs/webpj-app-tutorial-init-20261119.md) [앱 소유자](docs/webpj-app-world-storage-20261119.md) · [Guide](docs/webpj-app-campaign-guide-20261119.md) · [Fort 이름](docs/webpj-client-fort-name-20261119.md) · [Tutorial1 프레임 차이](docs/webpj-campaign-ownership-theme-20261119.md) |
| 전체 회귀 | E941 Node v26.13.0 전체 6,512건, E939 Node v22.23.3 전체 6,511건 통과(실패·취소·건너뜀 0). E941 실제 갈무리11 편집 Chromium 1건 통과; E940 편집 출력·원본 2,292행 포함 연관 Node 검사 46건 통과; E939 글꼴 측정·원본 2,292행 포함 연관 Node 검사 72건 통과; E938 Client TextEntry·원본 2,292행 포함 연관 Node 검사 63건 통과; E937 Client Slider·연관 Node 검사 51건 통과; E936 Client 돌 버튼·연관 Node 검사 27건 통과; E935 Gump idle·연관 Node 검사 49건 통과; E934 직접 텍스트·연관 Node 검사 15건과 Chromium 1건 통과; E933 IME 요소·연관 Node 검사 22건과 Chromium 1건 통과; E932 조합 사건·연관 Node 검사 14건과 합성 Chromium 1건 통과; E931 브라우저 문자 키·연관 Node 검사 10건과 Chromium 5건 통과; E930 Console 한글 단어·원본 2,292행 포함 Node 검사 28건 통과; E929 한글 출력 경계·원본 2,292행 포함 Node 검사 31건 통과; E928 한글 스크롤·원본 2,292행 포함 Node 검사 27건 통과; E927 확장 붙여넣기·원본 2,862행 포함 Node 검사 30건 통과; E926 한글 편집·원본 2,292행 포함 Node 검사 27건 통과; E925 표시 문자 경계·연관 Node 검사 7건 통과; E924 확정 문자·연관 Node 검사 24건 통과; E923 입력 큐·연관 Node 검사 17건 통과; E922 가상 키·연관 Node 검사 12건과 Chromium 4건 통과; E921 마우스 큐·연관 Node 검사 11건과 Chromium 3건 통과; E920 앱 포인터 경계·연관 Node 검사 20건과 Chromium 2건 통과; E919 브라우저 폴링·연관 Node 검사 18건과 Chromium 1건 통과; E918 포인터 통지·연관 Node 검사 36건 통과; E917 공통 방송·연관 Node 검사 17건 통과; E916 실제 구분선·연관 Node 검사 30건 통과; E915 가상 부착·연관 Node 검사 29건 통과; E914 HUD 등록 조회·연관 Node 검사 22건 통과; E913 실제 루트 교체·연관 Node 검사 36건과 Chromium 1건 통과; E912 원본 Uber 전체 160행·연관 Node 검사 28건 통과; E911 Uber/HUD·Gump 연관 Node 검사 24건 통과; E910 파생 Gump 연관 Node 검사 40건·Chromium 1건 통과; E909 Gump 연관 Node 검사 39건·Chromium 입력 1건 통과; E908 Gump 및 앱 연관 Node 검사 60건·Chromium 수명 1건 통과; E907 구름·팔레트 Node 검사 17건·실제 로그인 2건 및 Chromium 전환 1건 통과; E906 집중·연관 Node 검사 53건 및 Chromium 설정 쓰기 1건 통과; E905 집중·연관 Node 검사 47건 및 Chromium 화면비 전환 1건 통과; E904 집중·연관 Node 검사 84건 및 Chromium 실제 Canvas 1건 통과; E903 집중·연관 Node 검사 83건 및 Chromium 1건 통과; E902 집중·연관 Node 검사 42건 및 Chromium 1건 통과; E901 실제 로그인 2건·연관 Node 검사 22건 및 Chromium 1건 통과; E900 집중·연관 Node 검사 38건과 Chromium 1건 통과; E899 집중·연관 Node 검사 42건, 수동 적재 관련 Node v22 31건, Guide 로그인 2건과 Chromium 종료·재시작 1건 통과. 전체 회귀 중 선행 레이아웃 누락 시험 5건을 교정한 뒤 전부 재실행했다. E898 Node 관련 검사 37건·실제 로그인 1건·Chromium 1건 통과; E897 집중 검사 6건·실제 World 2건 통과; E896 Node 25건·실제 World 2건·Chromium 2건 및 cpppj 표시 바이트 11,683행 일치; E895 앱 프레임·상태 관련 검사 23건과 실제 Tutorial1 로그인을 포함한 Session 검사 126건 통과; E894 favicon Chromium 창·헤드리스 각 1건 통과; E893 집중·연관 Node 검사 62건과 E892 원본 관측 전체 876행 및 연관 검사 통과. HJOW-Athlon의 작업 단위 주기: E962 v26 전체 6,539건 통과 이후 v26 4/4, E958 v22 전체 6,529건 통과 이후 E966 고정 v26·v22 전체 각각 6,547건 통과. E970 고정 v26 전체 회귀 실행 중이며 E970 고정 v26 전체 6,551건과 E974 고정 v22 전체 6,554건 통과·실패와 건너뜀 0. E974 고정 v26 실행 중이며 양쪽의 다음 주기는 E975~E981 7단위이며 E978 고정 v26 검증 트리를 준비했다. fixture 835개 해시·파일 불일치/누락 0; 대응표 1,160행 오류 0을 확인했고 기존 cpppj 글꼴 관련 변경 검토 항목은 유지한다. |
| 브라우저 검사 | 2026-11-17 Chromium 2/2. 갈무리11 일반·굵은 글꼴, 한국어 메뉴·도움말, 적재 실패 시 영어 대체 확인. 최초 실행·진행 잠금/체크 및 저장 재실행은 E616에서 별도 Chromium 2/2로 확인했다. [갈무리11](docs/webpj-korean-galmuri11-20261117.md) · [최초 실행](docs/webpj-first-run-20261116.md) |

**E981.** 실제 Tutorial1 World·구름·Renderer를 Screen DIB와 앱 IndexedSurface까지 독립 합성했다. 앱 자동 프레임·HUD/Canvas는 남았다. [미션 화면 합성](docs/webpj-campaign-renderer-drawing-20261120.md)

**E980.** 실제 Tutorial1 표시 목록의 원본 정렬과 지면·자산·입자·커서 출력기를 연결했다. 앱 배경·실제 UI·Screen/Canvas는 남았다. [미션 표시 제출](docs/webpj-campaign-render-submission-20261120.md)

**E979.** 실제 Tutorial1의 지면·객체·입자·커서를 원본 공통 표시 목록에 수집한다. 앱 UI 수집·정렬/출력·배경은 남았다. [미션 표시 수집](docs/webpj-campaign-render-collection-20261120.md)

**E978.** Renderer의 현재 카메라 기반 지형 수집 범위·행 한도를 원본 x86 90행에 대조했다. 실제 수집과 앱 출력은 남았다. [수집 범위](docs/webpj-renderer-collection-geometry-20261120.md)

**E977.** Renderer 프레임 구름 큐·스캔 전역을 Client 수명에 소유하고 화면 크기 변경에서 보존한다. 실제 구름 합성은 남았다. [Renderer 전역](docs/webpj-app-renderer-frame-owner-20261120.md)

**E976.** 앱 프레임의 현재 World 준비·공유 Kernel 단일 실행·객체 보기 후처리를 연결했다. 주변 원본 루프·월드 화면/입력은 남았다. [앱 Kernel](docs/webpj-app-mission-kernel-frame-20261120.md)

**E975.** Tutorial1의 Ready→Start→첫 안내·시계 정지/재개를 앱 프레임에 연결했다. 메뉴 시작 명령·세계 표시/입력과 Kernel은 남았다. [앱 Tutorial Start](docs/webpj-app-tutorial-start-frame-20261120.md)

**E974.** 실제 Tutorial1 Session·Start에 앱 안내와 시계 정지/재개 실행기를 준비했다. Guide 자동 호출·세계 입력/표시는 남았다. [앱 Tutorial](docs/webpj-app-tutorial-runtime-20261120.md)

**E973.** 앱 Session에 현재 미션 승패 안내·팝업·AI/생산 할당 장치를 연결했다. Tutorial 자동 프레임과 실제 승패는 남았다. [앱 승패 장치](docs/webpj-app-mission-outcome-20261120.md)

**E972.** 앱 Client Gump 루트의 직계 팝업 차단 조회와 지연 닫기를 연결했다. Tutorial 프레임 연결은 남았다. [팝업 관문](docs/webpj-app-gump-popup-lifetime-20261120.md)

**E971.** 현재 미션 대본의 Tell 우선 조회와 창 없는 init 첫 파서를 연결하고 실제 Tutorial1 안내를 확인했다. [미션 Tell](docs/webpj-app-mission-tell-20261120.md)

**E970.** 로그인 Ready인 실제 Tutorial1의 Guide·World에 앱 시계·표시·자원·명령 창을 사용하는 미션 Start를 준비하고 직접 실행했다. Guide 자동 호출과 안내·화면은 남았다. [앱 Start](docs/webpj-app-mission-start-20261120.md)

**E969.** 앱 Client의 채팅 창·로그·무시 목록 전역을 같은 Session에 대여한다. 실제 채팅 창 공장과 명령 Gump 알림은 남았다. [채팅 창 전역](docs/webpj-app-chat-window-owner-20261120.md)

**E968.** 기존 World가 없는 메인 메뉴의 Guide Fort 요청을 음향 준비 뒤 앱 프레임에서 State7·로컬 로그인으로 자동 전달했다. 메뉴 명령·World 교체와 Start/표시는 남았다. [앱 Fort 요청](docs/webpj-app-fort-request-frame-20261120.md)

**E967.** 앱의 실제 채팅 100기록과 AI 이름·번역·현재 난이도를 Session에 연결하고 시험 대역 없는 Tutorial1 로그인을 확인했다. [앱 AI·채팅](docs/webpj-app-campaign-ai-20261120.md)

**E966.** 앱 프레임의 State Poll 뒤·패킷 처리 전 효과음 재생 수를 재계산해 자연 종료 후 동시 재생 한도를 회복한다. [프레임 재계산](docs/webpj-app-sound-frame-20261119.md)

**E965.** 앱 소유 SoundPlayer·WebSoundDevice와 설정·Session 효과음을 연결하고 실제 Tutorial1 로그인을 확인했다. 미션 진입 자동 준비·프레임 재계산과 음악은 남았다. [앱 미션 음향](docs/webpj-app-campaign-audio-20261119.md)

**E964.** 현재 언어·기본 효과음의 제한된 동시 적재와 해독을 구현하고 Chromium에서 218개 전체를 확인했다. 미션 진입 자동 호출·앱 옵션·Session 재생은 남았다. [효과음 준비](docs/webpj-mission-sound-preload-20261119.md)

**E963.** SoundPlayer에 연결할 Web Audio 버퍼·복제·음량/좌우 장치를 구현하고 Chromium 오프라인 출력을 확인했다. 앱 옵션·Session 연결은 남았다. [웹 소리 장치](docs/webpj-web-sound-device-20261119.md)

**E962.** 원본 효과음의 언어별·감쇠 접미사 우선 파일 선택과 같은 파일의 중복 해독 방지를 연결했다. Web Audio 버퍼 장치·미션 재생은 남았다. [파일 선택](docs/webpj-sound-file-resolution-20261119.md)

**E961.** 원본 SoundPlayer의 재생·복제·위치·이름·음량 제어를 이식하고 10.78 관측 2,128행을 대조했다. 실제 Web Audio 장치·Session 연결은 남았다. [재생 계층](docs/webpj-sound-player-20261119.md)

**E960.** 원본 위치 음향의 화면 투영·가시·좌우·거리 음량을 이식해 10.78 관측 3,759행과 대조했다. 실제 재생 장치·위치 음향 연결은 남았다. [위치 음향](docs/webpj-sound-spatial-20261119.md)

**E959.** 현재 매니페스트와 Web Audio에 미션 효과음·음악의 개별 적재, 동시 요청 공유와 반납을 연결했다. 미션 재생·위치 음향·곡 선택은 남았다. [음향 적재](docs/webpj-mission-audio-loader-20261119.md)

**E958.** E954 전체 회귀에서 실패한 독립 World 로그인 검사를 실제 앱 메뉴·Gump·팔레트·Renderer에 연결해 교정했다. [회귀 교정](docs/webpj-app-world-login-regression-20261119.md)

**E957.** 공통 효과음·음악 전체를 웹 공개 자산의 미션 묶음에 포함했다. E959에서 선별 적재를 연결했으며 위치 음향·음악 재생 장치는 남았다. [미션 음향 자산](docs/webpj-mission-audio-assets-20261119.md)

**E956.** Session의 즉시 카메라 적용을 실제 앱 Screen·Renderer·Gump·World에 연결했다. 프레임 말미 적용·월드 합성·입력은 남았다. [카메라 적용](docs/webpj-app-camera-commit-20261119.md)

**E955.** 앱 논리 크기 변경을 실제 미션 World에도 전달해 카메라 위치를 유지하면서 한계와 표면·입자 보기를 갱신한다. E956에서 즉시 카메라 적용을 연결했으며 월드 합성은 남았다. [World 크기](docs/webpj-app-world-resize-20261119.md)

**E954.** 앱 논리 Screen과 실제 Renderer·SquidRenderer 변경 표를 연결하고 화면 크기 변경 시 함께 교체한다. 월드 합성·Present와 실제 카메라 적용은 남았다. [표시 장치](docs/webpj-app-renderer-display-20261119.md)

**E953.** 소유권 테마·생산 갱신과 자산·개발자 명령을 현재 앱 설정·Gump·입력 큐에 연결했다. 프레임의 명령 소비·실제 월드 표시와 HUD는 남았다. [미션 장치](docs/webpj-app-gameplay-services-20261119.md)

**E952.** 앱 로그인 진행 창을 실제 메뉴 TellWindow와 연결하고 Tutorial1 요청부터 로그인 Ready까지 실제 진행 창을 확인했다. 원본 Gump serial 재사용과 LeaveBattle 취소·자동 요청 소비·기존 World 반납은 남았다. [진행 창](docs/webpj-app-mission-loading-20261119.md)

**E951.** 로그인 State7·13의 HUD 모드를 실제 앱 Gump 입력 트리에 연결하고 Tutorial1에서 모드 2·3을 확인했다. E950 지연 Fort 요청 처리기의 앱 프레임 자동 호출·현재 World 반납·진행 창은 남았다. 4D-1c의 비미션 검증은 스커미시 후속이다. [로그인 HUD](docs/webpj-app-login-hud-mode-20261119.md) · [요청 프레임](docs/webpj-campaign-request-frame-20261119.md) · [E949 이름표](docs/webpj-app-sound-list-20261119.md)

**E981 검증.** 실제 Tutorial1 DIB 출력·구름/지도 clip, 원본 구름 관측과 Renderer 등 Node 50건 통과. E974 고정 v26 전체 회귀 실행 중이다. [검증](docs/webpj-campaign-renderer-drawing-20261120.md)

**E980 검증.** 실제 Tutorial1 픽셀 출력·묶음 숨김 및 원본 제출/지면과 Renderer 검사 등 Node 47건 통과. E974 고정 v26 전체 회귀 실행 중. [검증](docs/webpj-campaign-render-submission-20261120.md)

**E979 검증.** 실제 Tutorial1 목록과 원본 공통 제출 322행·지면 묶음 353행 등 Node 29건 통과. E974 고정 v26 전체 회귀 실행 중. [검증](docs/webpj-campaign-render-collection-20261120.md)

**E978 검증.** 원본 수집 범위·행 한도 90행과 fixture/대응표 포함 Node 12건 통과. E974 고정 v22 전체 6,554건 통과 후 같은 상태 v26 실행 중이다. [검증](docs/webpj-renderer-collection-geometry-20261120.md)

**E977 검증.** 실제 Tutorial1 화면비 변경의 Renderer 전역 보존과 원본 구름 695행 등 Node 14건 통과. E974 고정 v22 전체 회귀 실행 중. [검증](docs/webpj-app-renderer-frame-owner-20261120.md)

**E976 검증.** 실제 Tutorial1 정지/재개 프레임의 Kernel 단일 호출과 ProcessForm 원본 482행 등 Node 24건 통과. E970 고정 v26 전체 6,551건 통과·실패와 건너뜀 0, E974 고정 v22 전체 회귀 실행 중. [검증](docs/webpj-app-mission-kernel-frame-20261120.md)

**E975 검증.** 실제 Tutorial1 자동 Ready/Start/안내·시계 재개 및 원본 수명/게이트 등 Node 25건 통과. E970 고정 v26 실행 중이며 E974 고정 v22 검증 트리를 준비했다. [검증](docs/webpj-app-tutorial-start-frame-20261120.md)

**E974 검증.** 실제 Tutorial1 안내 10프레임과 종료 후 7프레임 재개, 공통 게이트 등 Node 30건 통과. E970 고정 v26 회귀 뒤 E974 고정 v22 정기 전체 회귀가 필요하다. [검증](docs/webpj-app-tutorial-runtime-20261120.md)

**E973 검증.** 실제 Tutorial1 로그인·Start 뒤 승패 조회, 원본 승패 364행과 Gump 등 Node 24건 통과. E970 고정 v26 전체 회귀 실행 중. [검증](docs/webpj-app-mission-outcome-20261120.md)

**E972 검증.** Client 실제 Gump 트리·기존 팝업 처리기 Node 32건 통과. E966 고정 v26·v22 전체 각각 6,547건 통과·실패와 건너뜀 0, E970 고정 v26 실행 중. [검증](docs/webpj-app-gump-popup-lifetime-20261120.md)

**E971 검증.** 실제 Tutorial1 안내·원본 Tell 조회 및 메뉴 스크린샷 등 Node 54건 통과. E966 고정 v22 전체 회귀 뒤 E970 고정 v26 회귀를 실행할 검증 트리를 준비했다. [검증](docs/webpj-app-mission-tell-20261120.md)

**E970 검증.** 실제 Tutorial1 로그인·Start의 자원·순번·명령 창 알림 및 Client 서비스 등 Node 14건 통과. E966 고정 v22 전체 회귀 실행 중. [검증](docs/webpj-app-mission-start-20261120.md)

**E969 검증.** 앱 채팅 창 전역·Session 대여·실제 Tutorial1 로그인 등 Node 26건 통과. E966 고정 v26 전체 6,547건 통과·실패와 건너뜀 0, 같은 상태의 v22 실행 중. [검증](docs/webpj-app-chat-window-owner-20261120.md)

**E968 검증.** 실제 Tutorial1 요청 자동 처리·내부 로그인, 음향 실패/취소 및 기존 World 보존·시계 등 Node 13건 통과. E966 고정 v26 전체 회귀 실행 중이며 v22는 이후다. [검증](docs/webpj-app-fort-request-frame-20261120.md)

**E967 검증.** 실제 Tutorial1 로그인·AI 설정/번역·원본 채팅 2,448행 등 Node 17건 통과. E966 고정 Node 26 전체 회귀 실행 중이다. [검증](docs/webpj-app-campaign-ai-20261120.md)

**E966 검증.** 원본 SoundPlayer 관측 2,128행·실제 Tutorial1 로그인·재생 수 순서 등 Node 14건 통과. E966 고정 Node 26 및 Node 22 정기 전체 회귀를 순차 실행한다. [검증](docs/webpj-app-sound-frame-20261119.md)

**E965 검증.** 앱 음향·실제 Tutorial1 로그인 등 Node 15건 통과. E962 고정 Node 26 전체 6,539건 통과·실패와 건너뜀 0. Node 26 주기 3/4, Node 22 주기 7/8. [검증](docs/webpj-app-campaign-audio-20261119.md)

**E964 검증.** 음향 연관 Node 12건과 Chromium 실제 효과음 218개 해독·오프라인 출력 2건 통과. E962 고정 v26 정기 전체 회귀 실행 중이다. [검증](docs/webpj-mission-sound-preload-20261119.md)

**E963 검증.** SoundPlayer·Web Audio 장치 연관 Node 10건과 Chromium 오프라인 출력 1건 통과. E958 고정 Node v26·v22 전체 회귀 각각 6,529건 모두 통과. E962 고정 v26 정기 전체 회귀 실행 중이다. [검증](docs/webpj-web-sound-device-20261119.md)

**E962 검증.** 실제 감쇠 파일 선택·별칭 공유·언어 우선 및 SoundPlayer 연관 8건 통과. E958 기준 Node v22 전체 6,529건 통과 뒤 E962 고정 v26 정기 전체 회귀(4/4)가 실행 중이다. [검증](docs/webpj-sound-file-resolution-20261119.md)

**E961 검증.** 원본 SoundPlayer 재생 1,065행과 제어 1,063행 및 실제 SoundProcess 훅을 포함한 연관 22건이 통과했다. E958 고정 Node v26 전체 6,529건 통과(실패·건너뜀 0), 같은 상태의 Node v22 전체 6,529건도 통과했다. E962 기준 후속 v26 정기 전체 회귀는 실행 중이다. [검증](docs/webpj-sound-player-20261119.md)

**E960 검증.** 원본 위치 음향 관측 3,759행과 SoundProcess·fixture 목록 연관 검사 18건 통과. E958 고정 v26 전체 회귀는 6,529건 통과했고 v22 정기 회귀가 진행 중이다. [검증](docs/webpj-sound-spatial-20261119.md)

**E959 검증.** 실제 파일 선별 적재·해독·동시 요청·실패·반납 등 연관 29건과 E958 Session Load 전체 127건 통과. E958 고정 v26 전체 회귀는 6,529건 통과했고 v22 정기 회귀가 진행 중이다. [검증](docs/webpj-mission-audio-loader-20261119.md)

**E958 검증.** 실패했던 실제 World 로그인과 앱 State7 로그인 2건, 진행 창·요청·앱 장치 11건 통과. Session Load 전체 파일 127건 통과. 교정 커밋의 v26 전체 6,529건 통과 후 같은 커밋의 v22 정기 전체 회귀가 진행 중이다. [교정](docs/webpj-app-world-login-regression-20261119.md)

**E957 검증.** 음향 원본 목록·해시·같은 출처와 메뉴 적재 관련 14건 및 자산 도구 --check 통과. E954 기준 격리 v26 전체 6,526건 중 1건 실패(기존 독립 World 검사에 새 로그인 Gump 장치 누락), 건너뜀 0. E958에서 해당 검사를 실제 장치로 교정했고 v26 재회귀 통과 후 v22 회귀가 진행 중이다. [검증](docs/webpj-mission-audio-assets-20261119.md)

**E956 검증.** 실제 앱 카메라·Gump·Tutorial1 요청 연관 34건과 추가 Tutorial1 확인 4건 통과. E954 기준 v26 전체 회귀 결과와 교정 대기는 E957 검증을 따른다. [검증](docs/webpj-app-camera-commit-20261119.md)

**E955 검증.** 실제 로그인 World 1건 및 앱 화면·요청 검사 14건 통과. E950 기준 격리 v26 전체 6,519건 중 6,518건 통과·실패 0·1건 건너뜀(dotnetpj 패턴 파일 누락). E954 기준 정기 v26 전체 회귀 결과와 교정 대기는 E957 검증을 따른다(`extracted/webpj-e954-node26-isolated.log`). v22 8/8 전체 회귀는 교정된 v26 회귀 뒤 실행해야 한다. [검증](docs/webpj-app-world-resize-20261119.md)

**E954 검증.** 실제 앱 표시·팔레트·Tutorial1 요청 집중 검사 14건 통과. v22 주기 7/8. E954 고정 상태의 정기 v26 격리 전체 회귀 결과는 E957 검증을 따른다(4/4). [단위 검증](docs/webpj-app-renderer-display-20261119.md)

**E953 검증.** 실제 앱 장치·Client 장치 및 Tutorial1 요청 집중 검사 14건 통과. E950 기준 격리 Node v26 전체 회귀는 스크린샷 누락과 글꼴 라이선스의 CRLF 변환으로 6,519건 중 3건 실패·2건 건너뜀. 격리 자산 보완 후 관련 38건이 통과했고 동일 커밋 전체는 6,518건 통과·1건 건너뜀·실패 0으로 끝났다. 건너뛴 dotnetpj 자료를 포함한 E954 회귀와 후속 교정 결과는 E958 검증을 따른다. [원인·검증](docs/webpj-app-gameplay-services-20261119.md)

**E952 검증.** 실제 메뉴·진행 창·Client 및 Tutorial1 요청 집중 검사 29건과 실제 앱 State7 로그인 1건 통과. HJOW-Athlon 주기는 v22 5/8이며, 진행 중인 E950 기준 v26 격리 회귀 성공 뒤 E951부터 2/4다. [단위 검증](docs/webpj-app-mission-loading-20261119.md)

**E951 검증.** 실제 Tutorial1 로그인 1건과 앱 로그인·Gump 연관 검사 22건 통과. E950 실제 Tutorial1 요청을 포함한 집중 검사 4건과 ClientState 검사 3건 통과. E947 Node v22.23.3 정기 전체 회귀 6,514건 통과. E949 Node v26 전체 회귀는 실행 중 E950 모듈 추가·대응표 갱신이 겹쳐 6,515건 중 대응표 검사 1건이 실패했으며 현행 대응표 집중 검사 7건은 통과했다. `b4ff359de` 격리 작업 트리의 첫 Node v26 회귀는 환경 문제 3건으로 실패했고 보완 뒤 결과는 E955 검증을 따른다.

**E941 미완료 범위.** 앱의 FontSet·Gump 출력과 TextEntry 수명, 웹 입력 큐의 앱 프레임 소비, 실제 OS IME를 확인해야 한다. 미션 앱의 진행 창·프레임, HUD, Tutorial1 플레이 역시 미확인이다.

**미션 식별.** 이전 공통 fixture의 `thewarbegins.fort`는 캠페인 1-1이다. 실제 `tutorial1`이 선택하는 Fort는 `BridgeTheGap`이며, 지형·섬을 E753에서 다시 대조했다. [미션명 정정](docs/webpj-session-login-shipped-forts-20261118.md) · [Tutorial1 지형 재검증](docs/webpj-tutorial1-terrain-recheck-20261118.md)

1. **4D-3 Login.** 남은 명령·전송 장치와 World 입력을 경유하는 통합 시험을 서로 구분해 추적한다. 실제 Client 진행 창·Poll은 4D-6, Native 수신기는 4D-8에서 연결한다.
2. **4D-1 후속.** Fort 모드 snapshot 실제 통합을 확인한다. 비미션 너겟 정상 생성은 12차 스커미시 Session에서 검증한다.
3. **4D-4~4D-10.** 프레임·게이트 → 종료/반납 → GameWorld·Client → Native 장기 저장소 → 덤프 → Tutorial1 L2 순으로 진행한다. GameWorld 프레임을 연결한 뒤 다음 입력 작업은 상위 메뉴·브라우저 사건 연결과 실제 다중 유닛 이동 검증이다.

### 0.2 단계 진행판

`n/m`은 상위 체크 수이며 보관한 완료 항목을 포함한다. 상위 항목 안의 완료 부분은 중복 가산하지 않는다. 남은 단위와 줄 수는 `PORT_MAP.tsv`의 partial·todo 기준이며 partial에는 파일 전체 줄 수를 센다.

| 순서 | 단계 | 체크·상태 | 남은 단위 / 줄 |
| --- | --- | --- | --- |
| 1~3 | M1·월드 선행 부품 | 완료 | — |
| 4 | 4-A 자료·4-B 적재·4-C 전송·4-R 재기준선 | 완료. 4-C 통합 잔여는 4D-3 | — |
| 4 | **4-D Session 조립·Tutorial1 Load/Start** | **56/85, L2 미완료** | 76 / 17,771 |
| 5 | 5-A 렌더러·Draw | 44/46 | 3 / 533 |
| 5 | 5-B 카메라·HUD·미니맵·커서 | 9/13 | 9 / 1,446 |
| 5 | 5-C 화면 허브·미션 진입 | 0/6 → M2 | 10 / 6,033 |
| 6 | 입력·게임 메뉴·경제·건설 | 1/8 → M3 | 7 / 913 |
| 7 | 전투·의식·AI | 0/6 | 3 / 97 |
| 8 | 미션 수명·대본·오디오·도움말 | 0/9 → M4 | 30 / 5,979 |
| 9 | 표시 품질·브라우저/OS·배포 | E945 하위 경로·설정 복원 검증, M5 미완료 | 1 / 41 |
| 10 | AGENTS 후순위 웹 요구 | 일부 선행, 3.7절 참조 | 6 / 749 |
| **합계** | | | **145 / 33,562** |

### 0.3 이식량

기준선 `225553df`의 이식 대상 1,022단위는 **port 877(85.8%)·partial 64·todo 81**이다. 제외 138단위를 합하면 대응표 1,160행이다. 앱 수명 연결을 뜻하는 `session`과 오라클 대조를 뜻하는 `oracle`은 0이다.

| 범위 | port / 대상 | partial | todo |
| --- | --- | --- | --- |
| `o/` 월드·규칙 | 564 / 574 | 5 | 5 |
| `client/` 세션·UI·표시 | 312 / 447 | 58 | 77 |
| `zacket/` 내부 미션 전송 | 1 / 1 | 0 | 0 |

남은 145단위 중 기존 기준선은 76단위/28,124줄, 재기준선 추가분은 69단위/5,438줄이다. E854에서 `UberReconnect`의 단독 감시를, E884에서 단독 미션의 Fort 이름 선택 두 단위를 이식 대상으로 재분류했다. 재기준선 분류·영역별 상세는 [E801 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801)과 `python -X utf8 tools/port_inventory.py --families wb`에서 확인한다.

### 0.4 기준선과 자료

- **이식 기준선**은 cpppj 제품 1313(11.01) `225553df`다. 이후 변경 판정은 3.6절을 따른다. 별도 오라클 빌드는 HJOW-Athlon의 `extracted/webpj-oracle/225553df/Netstorm.exe`에 있다(SHA-256 `5b52d96d…b77eb46`, Git 제외).
- **자료 추적**은 [PORT_MAP.tsv](webpj/PORT_MAP.tsv)와 [fixture manifest](webpj/tests/fixtures/manifest.json) 834항목을 기준으로 한다. 이식 자료는 cpppj 소스·fixture·문서를 사용한다.

### 0.5 단위 착수와 종료

1. AGENTS.md와 공통·dotnetpj·웹 인계를 읽고 Git 상태·원격 변경을 확인한다. 같은 작업 트리의 다른 세션 변경을 고려해 파일명을 지정해 stage한다.
2. 대응표 검사와 기준선 뒤 변경을 확인한다. 3.1절의 단계 순서와 선행 조건에 맞춰 기준선 cpppj 코드, 기존 시험, fixture, 단위 문서를 읽는다. 큰 단위는 공개 API 경계로 나눈다.
3. 원본/클론·오라클 수집·별도 창 실행에는 AGENTS.md의 확인·예외 규칙을 적용한다. 웹 이식에는 디컴파일 소스 분석이 필요 없다.
4. 단위 문서에 근거 커밋·fixture/오라클·검사·실패와 교정·미연결 경계를 기록한다. 대응표의 web·base(8자리)·status·메모와 해당 manifest를 갱신한다. 현행 인계에는 현재 값·다음 작업·검증과 해당 체크만 갱신하고 완료 항목 상세는 정리 때 이관한다.
5. 각 작업 단위마다 해당 변경의 집중 검사와 연관 검사를 수행한다. HJOW-Athlon에서는 완료된 작업 단위 4회마다 Node v26 전체 회귀를, 8회마다 Node v22 전체 회귀를 수행한다. 두 버전의 전체 회귀가 함께 필요한 경우 순차 실행하며 동시에 실행하지 않는다. 다른 PC에서는 사용 가능한 최신 Node 버전으로 완료된 작업 단위 4회마다 전체 회귀를 수행한다. 필요하면 변경의 위험이나 통합 범위에 맞춰 추가 검사를 수행한다. 신호 NaN 비트 대조 시험은 맨 위에 `import './nan-exact.mjs'`를 둔다. (2026-11-19 사용자 지시)
6. 완료 후 당일 최대 번호를 확인하고 `YYYYMMDD-NN WB 요약`으로 로컬 커밋한다. 기존 커밋의 Codex 공동 작성자 표기와 AGENTS.md의 push 금지 규칙을 따른다.

### 0.6 제품 공통 요구

11.01 표기·Credits 세 번째 항목과 HJOW/AI 문구·`Copyright 2026 HJOW`, 시작 클릭 화면, 공개 루트 내 실행 자산, 외부 접속 문서화·유효성 검사 비활성화는 반영했다. Credits AI 문구는 사용자가 다시 지시할 때 수정한다. **15차 Reset All은 E863에서 구현·검증했다.** [제품 11.01](docs/webpj-product-1101-20261105.md) · [외부 접속](docs/webpj-external-access-20261105.md) · [초기화](docs/webpj-reset-all-20261119.md)

<a id="favicon"></a>

- [ ] **favicon 탭 시각 확인.** E894에서 공통 ICO를 복사하고 상대 경로 링크를 추가했다. 원본 두 이미지의 바이트 대조와 Chromium의 HTTP 200·해독을 확인했다. 기존 창 소유자 오류·Edge 시간 초과 뒤 재시도에서는 인앱 화면에 탭 영역이 없었고 Chrome은 URL 판별 실패로 컴퓨터 사용이 중단되어 탭 표시는 시각 확인이 남았다. [작업·검증](docs/webpj-favicon-20261119.md) · [공통 기준](LEFT_JOBS.md#exe-icon)

<a id="korean-font-change"></a>

### 0.7 한국어 갈무리11

E896에서 실제 미션 설치의 You→당신 번역과 Player 표시 바이트를 연결했다. [설치](docs/webpj-app-install-display-20261119.md) 제공된 일반·굵은 WOFF2와 라이선스를 공개 데이터에 포함했고 로딩·Canvas·자료 준비에서 같은 글꼴 설정을 공유한다. 11픽셀 작은 글씨와 영어 대체를 Chromium 메뉴·도움말에서 확인했다. [구현·검증](docs/webpj-korean-galmuri11-20261117.md)

- [ ] **실제 미션 화면** — `MissionBegin` 연결 뒤 한국어 브리핑과 혼합 제목의 줄바꿈·잘림 및 영어 대체를 확인한다.

## 1. 범위와 유지되는 결정

- **범위.** 일반 31경로와 데모 3경로를 대상으로 한다. 단독 미션의 같은 프로세스 안 내부 전송·패킷·Login·Zone·서버/클라이언트는 포함한다. LAN TCP·멀티플레이 UI·재접속/메타, 공식 검증 서버, Edit·커스텀 맵·User Made Campaigns, 설치/CD·Quit, 옵션 해상도/전체화면은 웹 범위에서 제외한다. 스커미시의 MP 시작 규칙과 Outpost는 12차에 포함한다.
- **이식.** cpppj의 파일과 API 경계, 부작용과 호출 순서, 정수 폭과 저장 시 정밀도, 난수 소비를 보존한다. 단독에서 도달하는지 확인해 웹 제외 분기에는 명시 오류를 유지한다. 현재 단계가 직접 요구하는 부품만 앞당긴다.
- **웹 구조.** `netstorm.js`가 앱 수명을 맡는다. `index.html`은 최소 변경하고 스타일은 JS에서 관리한다. 기존 CommonJS 서버와 `www/` ES 모듈 경계를 유지한다. 코어를 DOM·Canvas·Web Audio에서 분리하고 8비트 논리 화면과 최종 표시를 구분한다.
- **시간·입력·수명.** 시작 입력에서 오디오를 활성화한다. 탭이 숨으면 진행을 멈추고 복귀 시 시계를 다시 잡는다. `requestAnimationFrame`은 표시 기회다. 4:3부터 확인하고 16:9·16:10, 입력 역변환·DPI를 비교한다. 가장자리 스크롤은 활성 미션과 게임 영역 안의 포인터에서 동작한다.
- **자산·설정·언어.** `assets/game-data`·`fonts`·`webpj/translations`에서 `webpj/www/data`를 준비하며 느슨한 파일→TAFF 조회 순서를 유지한다. 버전 있는 `netstorm.settings`·`netstorm.progress` JSON은 localStorage에 두며 오류 시에도 계속할 수 있게 한다. 한국어 누락 파일·섹션·주제는 영어로 대체하고 미션 문구는 cpppj 확정본을 기본으로 삼는다. 사용자 검수와 언어별 글꼴은 3.5·3.7절을 따른다.
- **외부 링크.** Tell/도움말에서 치환된 `http:`·`https:`·`mailto:` 링크는 새 탭/창으로 열고 다른 주소의 제외 사유는 console에 기록한다. 개발 서버는 모든 인터페이스에서 수신한다. [외부 접속 7절](docs/webpj-external-access-20261105.md)
- **사용자 확인 범위.** 자연 완주·전체 외관/음향 동일성·실제 포획/희생·특정 적 공격·승리 재현은 사용자 확인 또는 별도 요청 대상으로 둔다. 공중 3종(훨리기그·맨오워·더스트 데빌)은 정적 이식 범위다.
- **보호·분담.** AGENTS.md·원본/CD·기존 fixture·공통 인계·dotnetpj 파일을 보존한다. cpppj 동시 작업은 사용자의 목표 지시를 기준으로 하며 별도 언급이 없으면 필요한 cpppj 수정이 가능하다. README 공통 상태는 일괄 갱신 때 반영한다. 새 문서와 코드는 UTF-8, 새 코드의 한국어 주석은 설명·사용법을 먼저 쓰고 추가·수정 시점과 이유를 뒤에 쓴다.

## 2. 완료 범위 요약

| 범위 | 확인한 결과 |
| --- | --- |
| WB00~WB04-D5c | 시작·메뉴·설정/진행·도움말·언어/제품 표기, 자산/TAFF/Fort 조회, SID·난수·Kernel·Form/Process 공통층. [기반과 단위 링크](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801) |
| E1~E328 | 월드 규칙 부품, 자료·타입 표, Fort·지형·섬과 단독 미션 전송 앞단. [순서 3 및 4-A~4-C](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801) |
| E329~E632 | 이동·비행·전투·의식 선행 부품, 렌더러·Gump·카메라/HUD, 제품 1313으로 재기준선, typed Session 생성자·공유 snapshot writer 선행. 최초 실행(E616)과 내부 미션 전환 flush/종료 수신(E620) 반영. [선행 부품과 재기준선](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801) |
| E633~E702 | 외곽 생성자·snapshot/콜백과 typed `Load()` 완료분. 실제 1-1 Fort 적재, Battle snapshot 재적재, 기술·로컬/세계 간헐천 및 단일 미션 너겟 관문 확인. [typed Load](docs/webpj-session-load-20261118.md) · [Battle snapshot](docs/webpj-session-battle-snapshot-20261118.md) |
| E703~E774 | 실제 typed Login/Ready·Start, 출하 스크립트 32종의 Fort 로그인, 같은 Session의 사제·골렘·벌룬 이동과 워크샾 배치·격리 완공. [Login](docs/webpj-session-login-ready-20261118.md) · [Start](docs/webpj-session-mission-start-20261118.md) · [워크샾](docs/webpj-workshop-isolated-completion-20261118.md) |
| E775~E801 | Session 카메라·입자·튜토리얼 게이트, AI/Player/Process 반납 경계, 지연 GameWorld·팔레트·자산 보기·선택. World 입력 부품과 UserInput 폴링·단축키·우클릭까지 연결. [공통 게이트](docs/webpj-session-tutorial-gate-20261118.md) · [지연 GameWorld](docs/webpj-campaign-game-world-front-20261118.md) · [입력](docs/webpj-campaign-selection-keys-20261118.md) |

## 3. 남은 작업과 실행 순서

### 3.0 개발 방식

Tutorial1 하나를 Load→Start→표시→입력→진행까지 먼저 확인하고, Early Missions·캠페인 1-1/1-2와 전체 34경로로 넓힌다. cpppj 클래스 단위로 기존 시험을 옮기고 원본 x86 fixture를 직접 읽는다. 플랫폼 경계는 대응표의 `adapt`로 판정한다. 생성 표는 생성 도구의 `--check`로 대조한다.

### 3.1 단계별 항목 목록

단계 안에서 선행 조건을 따라 위에서부터 진행한다. 막힌 사유를 기록하고 해당 단계의 다음 항목으로 간다. 4-D 전체가 선행 대기면 가능한 5-A·5-B를 병행한다. 완료 체크에 단위 링크를 붙이고 0.2절과 단계 머리글을 함께 갱신한다.

#### 4-D Session 조립·Tutorial1 Load/Start (56/85)

**완료 기준은 L2다.** Tutorial1 Load/Start의 보고서·Session 덤프·첫 프레임 논리 화면을 cpppj 오라클과 대조한다. typed `Load()`와 실제 Fort Login/Ready, 일부 Start는 Node 검사에서 확인했다. Client·Native 장치와 브라우저 앱 수명은 이어서 연결한다. 기준선 구간 지도와 E633~E801의 완료 체크 및 단위 링크는 [E801 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801)에 있다.

아래 cpppj 줄 번호는 `225553df`의 [RawCampaignSession.cpp](cpppj/src/client/RawCampaignSession.cpp) 기준이다. E946에서 4D-1b를 완료해 56개다. 단계 중간의 완료 검증은 해당 상위 범위의 근거로 둔다.

**4D-1 Session 외곽·snapshot.** 저장 writer, 현재 Core, 부모 callback, 생성자/Publish와 외곽 반납 앞단은 완료했다.

- [x] **4D-1b 전체 snapshot** — Battle 모드 재적재와 기술 획득 콜백의 Fort 모드 snapshot·재적재를 확인했다. `loaded` 이전의 준비된 Editor 설치·호출 수명은 Edit 제외 범위다(SnapshotFort/WriteFortCoreDataBody 4488..4509, 5903..5982). [Battle snapshot](docs/webpj-session-battle-snapshot-20261118.md) · [Fort 경계](docs/webpj-session-fort-snapshot-boundary-20261119.md)
- [ ] **4D-1c 외곽 callback** — 기술 획득, 로컬·세계 간헐천, 단일 미션 너겟 관문은 실자료로 확인했다. `PlaceBattleNuggets`의 비미션 정상 생성은 12차 실제 스커미시 Session에서 검증한다(5761..5772).

**4D-2 typed Load 완료.** 건설·이동·AI·의식·파괴/주문·운반 투하/기도·배치/커서를 Session에 설치했다. 캠페인 1-1의 저장 객체 적재와 `loaded` 관문을 확인했다. [E697](docs/webpj-session-load-20261118.md)

**4D-3 Login·로컬 연결·4-C 통합 시험.** 내부 Login → Load·명령 등록 → Ready → Start 순서를 실제 Session에서 확인했다.

- **E950 선행:** 지연 요청의 Poll 전 수명과 실제 Guide→State7→Ready를 검사했다. E968에서 기존 World가 없는 경우 앱 프레임 자동 호출을 연결했다. 기존 World 교체는 남았다. [E968](docs/webpj-app-fort-request-frame-20261120.md) [요청 프레임](docs/webpj-campaign-request-frame-20261119.md)

- [ ] **4D-3a RawCampaignSessionLogin** — typed 미션 Begin/Pump·내부 Ready 왕복과 행 base 뒤 32커밋 판정은 완료했다. E851은 진행 안내 장치를 실제 내부 로그인에 연결했고 E852는 Pump와 분리한 타이머 실행을 공개했다. 앱 메뉴의 최초 지연 요청 소비·로그인 완료 및 Tutorial Start는 E968·E975에서 연결했다. 로그인 완료 후 세계 화면 설치·분리 타이머는 4D-6, 고정 수신기는 4D-8과 연계한다. E884~E892에서 Fort 이름 선택과 원본 관측 876행을 대조했고 E948에서 실제 앱 Session의 Snapshot·Notify·State를 연결했다. Guide Init은 E886에서, 앱 프레임의 Guide 실행은 E890에서 연결했고 E968에서 기존 World가 없는 경우 앱 프레임의 요청 소비를 연결했으며 기존 World 교체는 남았다. [E968](docs/webpj-app-fort-request-frame-20261120.md) 나머지 31h 명령은 4D-3e에서 잇는다. `BeginGeneral`·`InitializeGeneral`·`LocalFort`의 필요 범위는 스커미시에서 판단한다. [E765](docs/webpj-session-login-commit-audit-20261118.md) · [E851](docs/webpj-client-login-progress-20261119.md) · [E852](docs/webpj-session-login-frame-timer-20261119.md) · [E948](docs/webpj-app-fort-name-session-20261119.md)
  - [x] **출하 Fort 로그인** — 일반 31종과 별도 데모 1종의 스크립트가 선택한 Fort를 실제 내부 Login/Ready까지 적재했다. 데모 모드 Start는 별도 확인 범위다. [E752](docs/webpj-session-login-shipped-forts-20261118.md)
- [ ] **4D-3b 로컬 공개 API·ClientCampaignTransport** — 시작/Poll/Pump·상태8·전송 기록·로컬/mutable 송신·Tell과 typed 기본 수신기(경로/압축 관찰·기술·Chat·연결 종료)는 확인했다. E893에서 내부 전송 등록 세대가 실제 Session을 대여하고 수신 중 State 정리를 막는다. 실제 Client의 장기 소유·Native 전송 수명은 남았다(4146..4200, 4292..4317). [E893](docs/webpj-session-internal-callback-20261119.md) [E772](docs/webpj-session-state-connect-record-20261118.md)
- [ ] **4D-3c 전송 앞단** — E820에서 공통 `RawPacketFrame`을 로컬 전송에 연결하고 E947에서 실제 앱의 시간 표본·Session Pump·공유 GameClock·State 정리를 확인했다. `RawClockSynchronization`의 월드 교체를 포함한 외곽 수명과 Native baseline, 소켓 지연 FIFO의 웹 제외 경계 및 `ClientPacketSources`가 남았다. [E820](docs/webpj-packet-frame-20261119.md) · [E947](docs/webpj-app-clock-synchronization-20261119.md)
- [ ] **4D-3d Squid 수신** — E822~E823에서 `RawPeerWorldSync` 본체와 World·Login 서버 콜백을 연결했다. 실제 원격 로그인은 웹 범위 밖이며 ReceiveSquid·Dais·Gun·Aviary·Flyer의 미연결/Native 경계(5148..5261)가 남았다. 공통 객체·종속 부착·변경/삭제·검증·공유 Regular·압축의 typed 수신은 반영했다. [E823](docs/webpj-peer-world-session-20261119.md) · [압축](docs/webpj-compressed-squid-receive-20261118.md)
- [ ] **4D-3e Login 콜백·명령 등록** — Carrier·건설·연구 typed 등록을 확인했다. RegisterCarrierCommands 5262..5436의 Native 분기와 미연결 명령, 31h 개발자 패널 명령의 실제 입력 고리 예약을 확인했다. 나머지 실제 Client 장치·단독 도달과 앱의 큐 소비를 확인한다. [E750](docs/webpj-session-login-ready-20261118.md) · [E766](docs/webpj-developer-devices-20261118.md)
- [ ] **4D-3f 통합 시험** — 같은 Session의 Login/옵션/사제 자연 도착, 일반·데모 Start와 AI 동맹, 세 운반 유닛의 World 선택→호버→클릭→전송/Session Kernel 자연 도착, 1-1 워크샾의 전체 Kernel 자연 완공과 연구 58h·생산 기술 동기화·플레어 자연 수명 및 중복 연구, GameWorld 프레임의 세 유닛 자연 도착을 확인했다. 실제 Client/앱 프레임과 유닛 생산은 입력/생산 장치 연결과 함께 확인한다. [E826](docs/webpj-world-carrier-input-20261119.md) · [E827](docs/webpj-workshop-kernel-completion-20261119.md) · [E828](docs/webpj-workshop-research-sync-20261119.md) · [E830](docs/webpj-workshop-research-flare-20261119.md) · [E836](docs/webpj-game-world-frame-20261119.md)

**4D-4 프레임·카메라·Tutorial 게이트**

- [ ] **4D-4a 카메라·프레임** — Session의 PrepareFrame/RunFrame, 공유 카메라 요청·적용, 초기/Home/사제 시점, 입자 뷰·커서 경계와 GameWorld 프레임·단일 Kernel 준비/후처리를 확인했다. 실제 Client·전체 Renderer에 연결하고 프레임 순서를 대조한다(3982..3999, 4523..4551, 4564..4587). [E775](docs/webpj-session-camera-request-20261118.md) · [E777](docs/webpj-session-particle-view-20261118.md)
- [ ] **4D-4b Tutorial 게이트·표시 장치** — Tutorial 1~6 판정·공통 게이트와 신전의 현재 슬롯 재조회는 연결했다. 실제 UI·앱, 다리 수명 숫자·Renderer, 승패 판정 소비 경계가 남았다(3659..3667, 6033..6075, 6115..6125). [E781](docs/webpj-session-temple-draw-20261118.md) · [E782](docs/webpj-session-tutorial-gate-20261118.md)

**4D-5 승패·종료·State 반납**

- [ ] **4D-5a 종료 수명** — 승패 조회 경계와 AI·Player/Deck·Process 자식 반납 증빙, 일반 종료 순서·오류 진단을 확인했다. 실제 Client/앱 State 반납, Native 건설·지속 실패와 승패 수명을 확인한다(4000..4057, 4588, 4816..4833). [E786](docs/webpj-session-state-withdraw-20261118.md) · [E789](docs/webpj-session-process-shutdown-error-20261118.md)
- [ ] **4D-5b Client State 정리** — ClientStateWorldGlobals의 실제 팝업·음향 소유자와 ClientStateWorldCleanup·ClientStateStorageCleanup·ClientStateRuntimeRetirement·ClientUserSelectionCleanup의 실제 Client/State 연결. E818에서 Fort 적재 Session, E819에서 내부 로그인·AI 미션 시작 뒤 정리를 검사했다. 실제 Client 전송·RawMenuControls·ClientAudio와 승패 후 수명이 남았다. [E818](docs/webpj-client-state-loaded-cleanup-20261119.md) · [E819](docs/webpj-client-state-started-cleanup-20261119.md)
- [ ] **4D-5c 이탈·전투 판정** — RawBattleCompletion의 팀·기권·미접속 관문을 기준선 C++ 합성 상태 4,096건과 대조했다. 실제 Session의 Player·동맹·연결·사제 종료 조회도 확인했다. 후반 Frame도 C++ 합성 상태 4,096건과 대조했다. Client 장기 소유자와 입력 고리, 무승부 거절의 Session 6F 처리·방송도 연결했다. 전투 복귀 State6의 결과·배경·음악도 이식했다. 실제 앱 루프·스커미시 결과·저장 장치와 4209..4279의 단독 도달이 남았다. [E842](docs/webpj-battle-completion-gates-20261119.md) · [E843](docs/webpj-session-battle-completion-20261119.md) · [E844](docs/webpj-battle-completion-frame-20261119.md) · [E845](docs/webpj-client-battle-frame-20261119.md) · [E846](docs/webpj-session-reject-draw-20261119.md) · [E847](docs/webpj-battle-return-state-20261119.md)

**4D-6 GameWorld·Client 서비스**

- **E951 선행:** State7·13 HUD 모드를 실제 앱 Gump 입력에 연결했다. E952에서 메뉴 진행 창을 추가했으며 원본 serial과 전체 HUD는 별도다. [로그인 HUD](docs/webpj-app-login-hud-mode-20261119.md)

- [ ] **4D-6a GameWorld** — 지연 Session 소유·Login 시작/완료·현재 자산·팔레트·선택 효과까지 확인했다. GameWorld 프레임 및 단일 Kernel 준비·후처리와 State7 부모의 생성·State8 내부 로그인은 확인했다. E855에서 실제 GameWorld의 로그인 이후 연결 감시·타이머 프레임까지 검사했다. E895에서 앱 프레임의 로컬 State Poll·패킷 Pump를 실제 Tutorial1 로그인에 연결했다. E913에서 공통 Gump 등록0과 Client 루트 별칭 교체 및 파생 입력을 연결했다. [E913](docs/webpj-app-gump-replace-20261119.md) E912에서 Uber 전체 생성·교체를 원본 160행과 대조했다. [E912](docs/webpj-uber-root-construct-20261119.md) E911에서 Uber의 현재 HUD 자식 뷰와 공통 트리 배치를 확인했다. [E911](docs/webpj-uber-root-view-20261119.md) E910에서 같은 등록표의 파생 노드·입력·소멸 경계를 연결했다. [E910](docs/webpj-app-gump-derived-20261119.md) E909에서 공통 Gump 기본 자식 부착·입력·삭제와 실제 Client 진단을 연결했다. [E909](docs/webpj-app-gump-tree-20261119.md) E908에서 공통 Gump 루트·등록·수명·입력 Client 소유자를 연결했다. [E908](docs/webpj-app-gump-root-20261119.md) E907에서 State7 팔레트를 같은 구름·파일·Screen·World에 연결했다. [E907](docs/webpj-app-battle-palette-20261119.md) E906에서 설정 적재·변경을 같은 구름 전역에 반영했다. [E906](docs/webpj-app-cloud-options-20261119.md) E905에서 화면비 변경 시 Screen을 현재 색상표와 함께 재생성했다. [E905](docs/webpj-app-screen-resize-20261119.md) E904에서 Screen DIB 출력을 앱 표면과 Canvas에 연결했다. [E904](docs/webpj-app-screen-present-20261119.md) E903에서 Screen DIB와 전체 clip을 준비하고 clear 픽셀을 확인했다. [E903](docs/webpj-app-screen-dib-20261119.md) E902에서 Screen 색상 적용을 실제 표시기 팔레트와 재출력 요청에 연결했다. [E902](docs/webpj-app-presenter-palette-20261119.md) E901에서 앱의 같은 고정 색표와 소유자 표를 실제 Tutorial1 Session과 World에 대여했다. 다른 UI 색상·날씨와 실제 Renderer 연결은 잔여다. [E901](docs/webpj-app-session-colors-20261119.md) E900에서 고정 입력의 소유자 표·9색표 캐시와 Screen 명암 조회를 연결했다. [E900](docs/webpj-app-color-cache-20261119.md) E899에서 실제 fortPal의 최초 ColorSort 입력을 앱 수명에 연결했다. [E899](docs/webpj-app-palette-capture-20261119.md) E898에서 앱 시작과 State7에 공유 논리 카메라 초기화를 연결했다. [E898](docs/webpj-app-renderer-init-20261119.md) 지연 요청 소비·완료 후 화면 설치·분리 타이머·진행 안내·종료·전체 삭제·Renderer를 연결한다. [E895](docs/webpj-app-connection-frame-20261119.md) 입력은 6단계에서 이어간다. [E790](docs/webpj-campaign-game-world-front-20261118.md) · [E793](docs/webpj-campaign-game-world-selection-20261118.md) · [E855](docs/webpj-game-world-login-frame-20261119.md)
- [ ] **4D-6b Client 서비스** — CampaignClientServices의 단독 미션 Start 효과, AI 이름·채팅·난이도, State7 요청 준비·캡처 및 부모 Poll의 State8 연결을 실제 Session에 연결했다. 개발자 입력 예약은 실제 입력 고리에 연결했다. E851에서 로그인 진행 안내·타이머 창 장치를 같은 GameWorld 로그인에 연결했고 E856~E858에서 사용자 입력과 커서 Gump·Client 표시 장치를 연결했다. E875에서 신전 소유권·PostPop 생산 갱신 사건을 실제 Gump 입력에 연결했다. E896에서 앱의 현재 번역·설정·제목을 실제 Session 설치와 연결했다. E897에서 Login 공통 설정·시계·Player 관문·화면 갱신을 앱 소유자로 연결했다. [E897](docs/webpj-app-login-services-20261119.md) [E896](docs/webpj-app-install-display-20261119.md) E952에서 현재 메뉴의 로그인 진행 창을, E953에서 현재 설정·Gump·입력 큐의 미션 장치를, E954~E955에서 화면 크기와 실제 Renderer 변경 표·World 크기 변경을, E956에서 즉시 카메라 적용을, E965에서 앱 미션 효과음·설정과 Session 재생을 연결했다. E966에서 실제 프레임 재생 수 재계산을, E967에서 앱의 실제 AI 이름·난이도·채팅 서비스를, E969에서 같은 Client의 채팅 창 전역 대여를, E970에서 로그인 Ready Session의 실제 앱 Start 준비를, E971에서 현재 미션 Tell 조회와 숨긴 init 파서를, E972에서 실제 Gump 루트의 팝업 차단·닫기를, E973에서 앱 Session 승패 안내와 AI/생산 할당 장치를, E974에서 실제 앱 Tutorial 안내·정지 실행기를, E975에서 앱 프레임 Ready→Start→첫 안내를, E976에서 World 준비·공유 Kernel·객체 보기 갱신을 연결했다. [E976](docs/webpj-app-mission-kernel-frame-20261120.md) [E975](docs/webpj-app-tutorial-start-frame-20261120.md) [E974](docs/webpj-app-tutorial-runtime-20261120.md) [E973](docs/webpj-app-mission-outcome-20261120.md) [E972](docs/webpj-app-gump-popup-lifetime-20261120.md) [E971](docs/webpj-app-mission-tell-20261120.md) [E970](docs/webpj-app-mission-start-20261120.md) [E969](docs/webpj-app-chat-window-owner-20261120.md) [E967](docs/webpj-app-campaign-ai-20261120.md) E968에서 최초 Fort 요청의 음향 자동 준비를 연결했다. [E968](docs/webpj-app-fort-request-frame-20261120.md) [E966](docs/webpj-app-sound-frame-20261119.md) [E965](docs/webpj-app-campaign-audio-20261119.md) 원본 Gump serial·취소 명령과 나머지 실제 앱 메뉴·Gump 소유자·프레임 타이머·입력 장치, 나머지 Poll, 스커미시 준비와 건설 기록 장치가 남았다. [E837](docs/webpj-campaign-client-start-effects-20261119.md) · [E838](docs/webpj-campaign-client-ai-services-20261119.md) · [E839](docs/webpj-client-mission-starting-20261119.md) · [E840](docs/webpj-client-mission-parent-20261119.md) · [E841](docs/webpj-client-developer-input-20261119.md) · [E851](docs/webpj-client-login-progress-20261119.md) · [E856](docs/webpj-campaign-user-input-services-20261119.md) · [E857](docs/webpj-campaign-cursor-gumps-20261119.md) · [E858](docs/webpj-campaign-cursor-client-20261119.md) · [E875](docs/webpj-campaign-production-refresh-20261119.md)
- [ ] **4D-6c Session 공개 조회·장치** — Start의 조회·기술·사제, 건설 운반, 생산 소유자, 커서·에너지·배치까지 확인했다. E848에서 운반체·Dude 이동 시작을 같은 경로/Kernel에 공개했고 E849~E850에서 Dude 부모·통지 조회와 실제 인구/경로, 속도별 시간표와 도착 삭제를 확인했다. 남은 조회/장치를 연결한다(4590..4619, 4643..4741, 6029..6032, 6076..6079, 6100..6114, 6316..6399). [E848](docs/webpj-session-public-movement-20261119.md) · [E849](docs/webpj-session-dude-public-20261119.md) · [E850](docs/webpj-session-dude-path-20261119.md)

**4D-7 ClientMain·Client 전역**

- **E949 선행:** Client 효과음 이름표를 실제 Session에 공유한다. 재생 장치와 위치 음향은 8단계에서 연결한다. [이름표 수명](docs/webpj-app-sound-list-20261119.md)

- [ ] **4D-7a ClientMain 단독 조립** — E878에서 앱의 공통 난수·SP 초기화와 실제 Session 대여를 검증했다. E879에서 앱의 Player·Core·Template·Subscriber 장기 소유자와 Session 교체를 확인했다. E880에서 실제 자산의 장기 Storage·Kernel과 Session 적재를 확인했고 E881에서 앱의 미션 자료 준비를 검사했고 E883에서 앱 Guide 생성·교체·반납과 제한 표 공유를 연결했다. 장기 Session과 나머지 Client 전역, RawSurfaceStorage 장치(기준선 ClientMain 442..533), ConfigInterface의 미션 연결. Native는 4D-8과 함께 적용한다. [E879](docs/webpj-app-shared-players-20261119.md) · [E880](docs/webpj-app-world-storage-20261119.md) · [E881](docs/webpj-app-mission-assets-20261119.md) · [E883](docs/webpj-app-campaign-guide-20261119.md)
- [ ] **4D-7b 메인 루프 단독 단계** — E852에서 로그인 타이머 프레임 호출, E853에서 연결 활동 패킷의 실제 전송·수신을 공개하고 E854에서 Client 연결 감시 뒤 타이머 순서를 실제 Session에 연결했다. E859~E860에서 Client 임시 문자열 고리·정수 서식과 별칭을 이식했고 E861에서 Client trace·console/Chat 경계를 실제 Session에 연결했다. E864는 Kernel 소진 요약·상세·단언 바인더와 실제 Session 프로세스 요약을 검증했다. E876에서 공유 Client 프레임 시계와 보정을 실제 GameWorld Kernel 준비에 연결했다. E877에서 앱 프레임과 탭 숨김 정지에 같은 시계를 결합했다. 실제 앱 미션·Kernel 프레임, 전투 자원 프레임·재접속 저장의 단독 도달, ClientDiagnostics 잔여와 고리·진단의 실제 Client 수명, 나머지 오류/예외 로깅을 확인한다. [E852](docs/webpj-session-login-frame-timer-20261119.md) · [E853](docs/webpj-session-heartbeat-20261119.md) · [E854](docs/webpj-client-connection-health-20261119.md) · [E859](docs/webpj-client-static-strings-20261119.md) · [E860](docs/webpj-client-static-integer-20261119.md) · [E861](docs/webpj-client-diagnostics-20261119.md) · [E864](docs/webpj-client-kernel-diagnostics-20261119.md) · [E876](docs/webpj-client-frame-clock-20261119.md) · [E877](docs/webpj-app-frame-clock-20261119.md)
- [ ] **4D-7c 메타 단독 앞단** — ClientMetaPacketReceivers·ClientChallengeGlobals·NativeMetaConnectedState, MetaMenuType/NotifyPlayerResults(4620..4642)의 도달을 판정한다. ClientMetaSpriteRefresh·NativeMetaSpriteRefresh는 5-C와 확인한다.
- [ ] **4D-7d Client 렌더러 프레임 전역** — 채팅·HTML·점 명령 전역은 이식했다. E977에서 ClientRendererFrameGlobals의 실제 앱 소유·화면 크기 변경 수명을 연결했다. 수집/출력 사용은 5-A에서 연결한다. [E977](docs/webpj-app-renderer-frame-owner-20261120.md) [E496](docs/webpj-renderer-cloud-queue-20261115.md)

**4D-8 Native(10.78 Client) 분기.** 실제 cpppj 오라클은 시작 때 RawSurfaceStorage에 Native 장치를 결합한다. typed Session과 같은 상태를 비교하려면 실제 Client 소유자 아래 Native 경계를 함께 맞춰야 한다.

- [ ] **4D-8a RawSurface 삭제·수명·공통 11단위** — AssetDeletion·AssetNotice·Attack·Content·Decoration·Deletion·Depend·FrameRegular·Lifetime·PlayerRegular·RegistryDeletion.
- [ ] **4D-8b RawSurface 자원·생산·사제 9단위** — Carrier·DudeRegular·Priest·PriestDeletion·FactoryRegular·Mana·ResidenceRegular·Reward·Gump.
- [ ] **4D-8c RawSurface 전투·의식 9단위** — Balloon·Bomb·Damageable·Fence·Flare·Ice·DaisDeletion·Ritual·Vortex. 소비자 연결은 7단계와 연계한다.
- [ ] **4D-8d Native 건설·Vortex** — ClientConstructionGlobals·Regions·RandomPlacement·Placement·Devices·Gem, AssetDeletionVtables와 RawCampaignVortex. 입력·건설 나머지는 6단계다.
- [ ] **4D-8e Native 고정 수신기** — NativeLogin/Gameplay/Carrier/ConstructionPacketReceivers·ClientChatPacketReceiver·FortCoreDataLoad와 Session의 고정 수신(4063..4099, 5437..5582). canonical 기준선·대여를 함께 확인한다(3.6절).
- [ ] **4D-8f 공통 후처리·Fort 가상** — SquidPostPop의 실제 World 등록·기본 장치·자원/AI 통지와 NativeFortSaveVirtual.
- [ ] **4D-8g Native 분기 재대조** — 대응표와 단위 문서에서 남긴 `UsesNativeTransmissionDevices()` 분기, 장기 Storage owner, 보상/삭제 공유 장부, ProcessHostStorage·Kernel procSumm을 함께 확인한다.

**4D-9 웹 session-dump·오라클 시나리오**

- [ ] **4D-9a SessionStateDump** — E865에서 판번호 1 순수 TSV 서식과 분산 SP 복호를 구현했고 E866~E867에서 실제 Session의 시계·난수·Kernel/Regular·풀·Player/동맹·SP·Graph·TotalMade를 수집했다. 실제 Client 수명과 덤프 호출 시점을 연결한다. [덤프 형식](docs/exe/cpp-session-state-dump-20261109.md) · [E865](docs/webpj-session-dump-format-20261119.md) · [E866](docs/webpj-session-dump-base-20261119.md) · [E867](docs/webpj-session-dump-world-20261119.md)
- [x] **4D-9b 오라클 시나리오** — E868에서 `tools/cpp-oracle.mjs`의 `session-dump` 단계·고정 프레임 인자와 결과 수집을 연결했다. E869에서 고정 시각 Tutorial1의 실제 Load/Start 덤프·화면·보고서를 수집했다. 웹 비교는 4D-10에 남는다. [E868](docs/webpj-cpp-oracle-session-dump-20261119.md) · [E869](docs/webpj-tutorial1-cpp-oracle-20261119.md)

**4D-10 재대조와 L2**

- [ ] **4D-10a 실제 소유자 재대조** — E882에서 CampaignGuideRelease의 실제 Client 금지값 참조와 Session 보유·무World 해제를 확인했다. 남은 typed 선행 바인딩과 ClientGameplayGlobals, GameAux·TypeNames·RawBombBlastBindings를 실제 Client/Session 및 Native 장기 경로로 확인한다. Archer·Motion/Path·Cursor·SoundProcess 소비자 검증은 6~8단계와 연계한다. [E882](docs/webpj-client-guide-release-20261119.md)
- [ ] **4D-10b L2 판정** — E870에서 판번호 1 덤프의 행별 비교와 명시적 SP 소유자 0 제외를 구현했다. E873에서 원본 테마 기본값을 맞추자 Tutorial1 안쪽 613개는 프레임까지 일치하고 경계 45개가 남았다. E874에서 이 차이는 모두 같은 방향·변형의 해당 원소 후보 구간 내에 있음을 확인했다. 같은 시점·난수에서 Tutorial1 Load/Start의 보고서·덤프·첫 프레임 논리 화면을 맞추고 실제 결선 단위를 대응표에서 `session`으로 올린다. [E870](docs/webpj-session-dump-compare-20261119.md) · [E873](docs/webpj-campaign-ownership-theme-20261119.md) · [E874](docs/webpj-tutorial1-border-candidates-20261119.md)


#### 5-A 렌더러·Draw·시각 시스템 (44/46)

- [ ] **ClientRendererFrame** — E978에서 원본 지형 수집 범위·행 한도 90행을 복원하고 E979에서 실제 미션 지면·객체·입자·커서 수집을, E980에서 원본 정렬·출력을, E981에서 실제 Screen DIB·구름·Renderer 독립 합성을 연결했다. [E981](docs/webpj-campaign-renderer-drawing-20261120.md) [E980](docs/webpj-campaign-render-submission-20261120.md) [E979](docs/webpj-campaign-render-collection-20261120.md) [E978](docs/webpj-renderer-collection-geometry-20261120.md) RawMenuControls(5-C)·GameWorld(4D-6a)를 선행하고 ClientRendererFrameGlobals·GumpVisual을 함께 확인한다. Kencloud를 메뉴에 연결하고 구름 배경의 알려진 차이를 해소한다. [화면 문서](docs/webpj-screen-20261113.md)
- [ ] **E197·E198·E201 재대조** — 웹 조립으로만 확인한 시각 시스템을 실제 Session의 cpppj 시험과 대조한다. 7단계와 중복되는 시험은 한 번만 한다.

#### 5-B 카메라·HUD·미니맵 (9/13)

- [ ] **UberHud** — HudDock·HudDividerInput을 Uber에 연결하고 HudNsInfoMeta·NativePlayerInfoInput·NativeNsInfoInput·HudPlayerButtonDraw의 단독 경로를 확인한다.
- [ ] **RawMinimapControls** — 초안에 MinimapGeometry·cpppj 전체 계약·원본 fixture와 메뉴/월드 허브를 연결한다.
- [ ] **FloatingSpGump·RawProductionControls** — 실제 표시 소비자를 연결하고 InventoryItemGump 분리 뒤 잔여를 확인한다.
- [ ] **Cursor·RawCursor 표시** — 선행 커서 부품을 실 화면에 연결한다. 실제 카메라 투영·가장자리 스크롤·마나 볼트 정기 표시를 함께 대조한다.

#### 5-C 게임 화면 허브·미션 진입 (0/6)

- [ ] **Uber 허브** — E917에서 공통 프레임 방송을 실제 구분선에 분배했다. [E917](docs/webpj-app-gump-broadcast-20261119.md) E916에서 실제 Client 공통 구분선 생성·입력·부착을 연결했다. [E916](docs/webpj-app-hud-divider-20261119.md) E914에서 실제 Client 등록표의 serial·현재 HUD 판정을 구현했다. [E914](docs/webpj-uber-root-lookup-20261119.md) UberRootGump의 전체 생성·교체는 원본 160행으로 대조했다. [E912](docs/webpj-uber-root-construct-20261119.md) UberGump·RawUberRootControls와 실제 하위 공장을 연결해 HUD·덱·미니맵·채팅을 소유하게 한다. Meta Sprite 두 단위는 4D-7c와 확인한다.
- [ ] **메뉴·버튼 컨트롤** — RawMenuControls·RawButtonControls·RawMenuHtml·RawMenuLoading·RawTextEntryControls·RawMenuChat. RawMenuControls는 5-A의 선행이다.
- [ ] **메인 메뉴·타이틀 상태** — NativeIntroState·TitleBitmapLifetime·NativeMainMenuState는 이식했다. ClientTitleBitmaps가 남았다.
- [ ] **미션 진입 명령** — MissionBegin·StartDemo·환영 창 명령을 실제 Fort 요청·State 전환·Session 생성에 연결한다.
- [ ] **허브 fixture** — 3.8절의 채팅·HTML 항목을 연결한다.
- [ ] **M2 / L4a** — 4-D·5-A·5-B 조립 뒤 Tutorial1 진입 직후와 N프레임 뒤의 논리 화면 차이 0을 확인한다. 영어 4:3부터 16:9/16:10과 한국어로 넓힌다.

#### 6 입력·게임 메뉴·사제/운반·경제·건설 (1/8)

- [ ] **입력 본체** — UserInput의 상위 메뉴·브라우저 이벤트와 특수 진단의 실제 로그 장치를 연결한다. 폴링·우클릭·단축키·실제 후보 선택 및 일반·이동 모드 좌클릭을 확인했다. NB20의 실제 조각 취득→P 취소는 Session에서 검증했다. [E802](docs/webpj-campaign-user-input-movement-click-20261118.md) · [E803](docs/webpj-campaign-user-input-primary-click-20261118.md) · [E804](docs/webpj-nb20-real-cursor-cancel-20261118.md)
  - [x] **GameWorldInput 코드 이식** — 선택/강조·미니맵 이동과 객체 메뉴·이동 커서까지 완료했다. 대응표 `GameWorldInput.inc` 상태는 `port`다. [E795](docs/webpj-campaign-game-world-input-20261118.md) · [E796](docs/webpj-campaign-game-world-menu-cursor-20261118.md)
- [ ] **게임 메뉴** — RawGameMenus의 ESC 가로/하위 메뉴·명령·Player 메뉴의 실제 수명.
- [ ] **객체 우클릭 메뉴** — CampaignObjectMenuClient의 건물·운반체·특수 대상 메뉴와 소유자 명령 연결.
- [ ] **사제·운반·경로 바인딩** — 캠페인 Session 운반 입력과 일반 사제 이동은 E794에서 확인했다. MetaMap·CampaignUserInputServices·앱과 저장 미션 thewarbegins/masterofwhirligigs의 전체 초기화·경로 루프를 대조한다. [E794](docs/webpj-session-carrier-input-20261118.md)
- [ ] **건설·생산·경제** — RawProductionMaintenance 본체는 원본 86행으로 대조했고 같은 Session의 기술·SP·시계·내용물에 연결해 실제 다리 공급을 확인했다. ClientProductionSync는 무월드와 실제 Session 양쪽의 같은 Deck 경로를 연결했다. ProductionControlContext는 같은 Client 장기 소유자와 옵션을 공유한다. ClientBridgeDecay는 장기 스캔과 현재 Session 칸 전달을 연결했다. HUD의 주기 호출, 실제 Client 메인 루프 및 4-D에 남은 건설 장치의 입력 소비자가 남았다. [E831](docs/webpj-production-maintenance-20261119.md) · [E832](docs/webpj-session-production-maintenance-20261119.md) · [E833](docs/webpj-client-production-sync-20261119.md) · [E834](docs/webpj-production-context-20261119.md) · [E835](docs/webpj-client-bridge-decay-20261119.md) 실제 wire 배치/확정, SP·덱/생산, 다리 붕괴, RawBuildReplay·ClientBuildReplay의 FILE/최신 rec·Host·F7/Ctrl+F6를 확인한다. [건설 기록 재생](docs/webpj-build-replay-runtime-20261116.md)
- [ ] **공장·지식 메뉴와 기술** — ThemeTechnologyCommand에서 연구·업그레이드·지식 메뉴와 기술 명령을 연결한다.
- [ ] **M3 / L3** — Tutorial1 이동·수확·건설·생산의 입력 단계표로 cpppj 상태·화면을 대조한다. Tutorial1·2와 캠페인 1-1·1-2의 브라우저 검증까지 진행한다.

#### 7 전투·의식·AI (0/6)

- [ ] **전투 바인딩** — Hit·GunQueries·Archer·Motion·BombBlast·Damageable 표와 실제 저장 미션 Session/Kernel을 대조한다. Archer의 초기 배치·비행 및 factory/clock loop는 1절의 검증 범위를 적용한다.
- [ ] **파괴·주문 효과** — DetonateInput 소비자와 RawObjectConversion의 기폭 입력 fixture를 확인한다. 주문과 Meltdown typed Load는 완료했다.
- [ ] **의식** — RitualChoiceInput과 보상 UI·제단·World 연결. 의식 typed Load는 완료했다.
- [ ] **AI 잔여·전투 이탈** — AiTypeRequiredEnergy에 실제 타입 자산을 연결한다. RawBattleLeave 부품은 이식했다.
- [ ] **AI 세션 결선** — Start의 일반/데모 AI·동맹과 typed 프레임은 확인했다. 현재 Player·SP·명령·난수 및 장기 RawSurface 소비자를 연결해 AI 미션의 N프레임 L3을 비교한다.
- [ ] **전용 시험이 없었던 시스템** — E197·E198·E201 등을 실제 cpppj Session 시험과 대조한다(5-A와 연계).

#### 8 미션 수명·안내·대본·소리·채팅·도움말 (0/9)

- [ ] **대본·대화창** — DialogScript·DialogBodyLayout, 현재 미션의 안내·채팅·대화 소비자.
- [ ] **소리 본체** — Sound·SoundDevice·ClientAudio를 Web Audio로 적응한다. E957에서 원본 효과음 전체를 공개 루트에 배치하고 E959에서 파일별 선별 적재, E960에서 원본 위치 음향 계산, E961에서 장치 독립 SoundPlayer, E962에서 감쇠 파일 선택, E963에서 Web Audio 버퍼 장치, E964에서 효과음 전체 사전 해독, E965에서 앱 설정·Session 재생 장치를 연결했다. E966에서 프레임별 재생 수 계산을 연결했다. E968에서 최초 Fort 요청의 음향 자동 준비를 연결했다. 실제 청취와 Wind Noise 및 음악을 확인한다.
- [ ] **음악** — E957에서 원본 음악 전체를 공개 루트에 배치하고 E959에서 파일별 선별 적재를 연결했다. SoundMusic·SoundMusicRuntime·SoundMusicFile·SoundMusicSelection·SoundSceneMusic의 선별 적재·재생 수명·곡 선택은 남았다.
- [ ] **동영상** — 인트로 제공 범위를 판정하고 RawMovieControls·MoviePlayback·MoviePcmOutput·MovieLifecycle·SmackerDecoder를 브라우저 디코더에 맞춘다.
- [ ] **도움말·Tell** — QuickHelpHtml/Control/Window/Request, HtmlConditionEffect·HtmlLinkText·TellFileMenus·HelpLinkPolicy·ExternalHelpClient·RawNsInfoContext의 남은 계약.
- [ ] **글자·설정·그림** — BitmapFont·KoreanResources·ClientOptionsDisk·GifImage·GifScreenshot의 웹 적응·남은 부분과 1322 글꼴 변경 영향.
- [ ] **미션 수명** — 튜토리얼/정지·Retry/Next/Abort·메뉴 복귀와 실제 진행 저장, 데모 360초 반복·Auto-Demo를 확인한다.
- [ ] **적응 fixture** — 3.8절의 오디오·글꼴·렌더러·음악 관측을 판정한다.
- [ ] **M4** — 일반 31 + 데모 3경로의 진입·진행·저장과 한국어 미션 문구를 확인한다. 자연 완주는 1절의 사용자 확인 범위다.

### 3.2 실행 순서와 남은 마일스톤

M1은 메뉴 관문을 통과했다. 4-D의 Tutorial1 L2 → 5-A/5-B·5-C의 Tutorial1 화면(M2) → 6단계 입력과 플레이(M3) → 7/8단계 전체 범위(M4) → 9단계 배포(M5) 순으로 진행한다. 9단계는 원본 시계를 유지하며 60/120 표시, 성능·메모리, Chrome/Edge/Firefox·GUI Linux와 하위 경로 정적 배포·새로고침·캐시·저장 호환을 검증한다. AGENTS 후순위는 3.7절을 따른다.

- [x] **E945 하위 경로 배포.** Windows의 Chromium·Chrome·Edge·Firefox에서 하위 경로의 정적 모듈과 자산을 로드하고 같은 URL을 새로고침하여 음량 설정이 복원되는 것을 확인했다. HTTP 캐시, GUI Linux와 실제 미션 진행 저장은 별도 검증한다. [E945](docs/webpj-static-deployment-20261119.md)

### 3.3 cpppj 오라클 사용법

- **같은 조건.** 자료 해시·언어·화면 크기·옵션·시드·프레임과 입력을 맞춘다. 기준선 실행 파일은 0.4절의 별도 빌드를 쓴다. 독립 `NETSTORM_USER_DATA`와 `--mute-audio`로 소리 장치 상태를 보존한다.
- **수집·비교.** `tools/cpp-oracle.mjs`의 시나리오에서 `--ui-script`의 `snapshot`·`report`를 수집하고 `tools/screen-compare.mjs`로 논리 화면을 비교한다. 알려진 차이는 영역·픽셀 수·원인을 기록한다. 기준선 cpppj에는 `session-dump`·`--fixed-frame-ms`가 있으며 웹 시나리오·덤프 포맷 지원은 4D-9에 남았다. L4b는 cpppj MCP 녹화를 쓴다. [오라클 비교](docs/webpj-cpp-oracle-20261107.md) · [덤프·고정 시각](docs/exe/cpp-session-state-dump-20261109.md)
- **실행·추적.** 창을 띄우는 수집은 AGENTS.md의 확인과 호스트 예외를 적용한다. 결과·cpppj 커밋·EXE SHA-256·명령줄을 manifest에 남긴다. Node 검사는 저장된 결과를 읽는다.

### 3.4 cpppj 현황과 기준선 관계

공통 인계상 최신 제품은 1322(갈무리11)이며 웹 비교 기준선은 제품 1313이다. 1314 최초 실행과 1320 미션 전환을 웹에 반영했다(E616·E620). 1315의 한국어 `Type` 번역 교정본은 8단계 공식 한국어 자료에 적용하고, cpppj 시험만 바뀐 항목은 해당 시험을 옮길 때 현행 기대값을 확인한다. 1322 소스 영향 판정은 3.6절에 있다. [공통 인계](LEFT_JOBS.md)

### 3.5 사용자 확인과 결정 대기

- 자연 완주·실제 사제 포획/희생·특정 적 공격·승리·전체 화면/음향은 1절 범위에 따라 확인한다.
- 미션 한국어는 cpppj K1~K5 확정본을 기본으로 한다. 자료 분리 여부와 실제 미션 줄바꿈/혼합 제목·대체 표시를 8단계에서 확인한다.
- [x] **웹 링크 브라우저 검증.** Chromium에서 외부 출처 탭의 `opener`와 `referrer`가 비어 있고, 브라우저가 팝업을 차단해도 현재 문서가 유지된다. [E942](docs/webpj-external-access-20261105.md#71-브라우저-검증)
- 와이드 전투 시야와 카메라 재배치를 연결할 때 16:9의 논리 크기 전환도 검증한다. WebMCP 대상 브라우저·조작/캡처/녹화 범위는 착수 시 확정한다.

### 3.6 cpppj 변경 재반영

대응표 검사에서 행 base 이후 변경을 확인한다. ① 웹 영향 없음, ② 웹 재반영, ③ 해당 소비 단계에서 처리로 판정한다. ②는 별도 단위로 반영하고 ③은 대상 단계에 남긴다. 판정 근거를 대응표와 단위 문서에 기록한다. 현재 보고는 1322(`635906357`)의 BitmapFont·ClientMain·KoreanResources·UberGump 4행이며 새 EmbeddedBitmapFont·KoreanFontSettings 파일의 웹 영향도 함께 판정한다.

| 처리 단계 | 남은 판정·연결 |
| --- | --- |
| 4D-3·4D-6 | Client 진행 창/분리 Poll, GameClock의 실제 Client 바인딩·RawClockSynchronization 외곽, ClientConnectionGlobals의 PacketSources·PacketDelays, RawPacketCopies 현재 State/Zone의 복제·인수, RawMonitorTime의 실제 Config/Chat 부모, RawIslandRuntime의 실제 Client 및 NoWorld 소비자 결선. 아비어리 ReadProcess·WritePacket 및 공용 Form writer의 실제 Storage/World 경계 확인. |
| 4D-8 | PacketRegistrationTable·RawPacketDispatch의 canonical 승격/등록/스냅샷, RawConnectionActivity·RawMissionLogin·CampaignTransportServices·ClientCampaignTransport의 고정 수신기 대여. 장기 RawSurfaceWorld/Storage·ProcessHostStorage, Native mutable 송신·기본 Form writer, 공통 Form/Process·자산 삭제/후처리·보상과 Native 조회기. SidPool 소진 회수·진단과 GameAuxState의 새 전역 칸은 소비자와 연결한다. |
| 5-A~5-C | 프레임 카메라·Renderer, UberGump/메뉴 허브·‘Creating Islands.’ 로그인 진행 안내와 메타 Sprite 소비 경계. |
| 6~7 | 건설 ActualConfirm/ActualPlace, F7/Ctrl+F6 건설 기록·자원 갱신, 동맹 협상 입력/단독 도달, RawPathRuntime/RawPathData 관계, RawGunHit Rain override. NB20 실제 취소는 E804에서 검증했다. |
| 8~10 | TellSection 대체 파일·한국어/일본어 자원 우선순위·입력, Frame Rate·Difficulty·NB10, 스커미시 일반 시작·중앙섬·자기 Fort의 실제 소비 경계. |

커밋·함수별 이력은 [E801 보관본 3.6절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801), [대응표](webpj/PORT_MAP.tsv), [Login 변경 판정](docs/webpj-session-login-commit-audit-20261118.md)에 있다. typed 공유 Regular·압축 수신은 E734·E735, 일반 Storage Squid 송신은 E594, 기술·Chat·연결 종료를 포함한 typed 기본 Client 수신기는 E767~E770에서 반영했다. Native 대여는 4D-8에 남는다.

### 3.7 AGENTS 후순위 요구(4~15차)의 웹 처리

| 차수 | 요구와 남은 일 |
| --- | --- |
| 4 | E943에서 16:9 논리 화면을 최대 1920×1080까지 지원하고 창 크기 변경과 입력 좌표를 확인했다. 전투 카메라·HUD와 WebMCP 조작·화면 캡처·소리 포함 녹화가 남았다. |
| 5 | localStorage 설정·진행 저장과 오류 대응을 구현했다. 실제 미션에서 진행값이 발생하는 8단계와 15차 초기화에서 저장 수명을 확인한다. |
| 6 | 메뉴·Tell·도움말과 갈무리11까지 구현했다. 8단계 현재 미션 Tell과 한국어 공식 미션 문구가 남았다. |
| 7 | E862의 초기화·적재·프레임·이벤트와 E944의 화면 관찰·한국어 적재·오디오는 단계·상태·시각과 원본 예외를 console에 보고한다. 실제 미션 수명에서의 오류 진단이 남았다. [E862·E944](docs/webpj-browser-error-reporting-20261119.md) |
| 8·9 | 웹 범위 제외(AA/FSR, Edit 시나리오 편집). |
| 10 | 10.78 알려진 버그 중 웹 적용분을 cpppj 판정과 대조한다. NB04/29 간헐천 계산·callback, NB18 요청/수신 관문과 NB20 실제 Session 선택/취소는 선행했다. 실제 스커미시와 브라우저 조작 검증은 남았다. |
| 11 | 쉬움/보통/어려움(기본 보통). 쉬움은 적 AI 다리/생산 주기 4배와 충분한 SP에서 채취 유닛 절반(최소 1), 어려움은 주기 절반. E554·E555 계산과 AiCreate 연결 뒤 Options·Session hook·저장 확인이 남았다. |
| 12 | Campaign의 Skirmish와 AI 수·개별 난이도/동맹·금지 유닛 최대 5(Outpost 포함)·시작/취소 UI, 무작위 섬·간헐천을 연결한다. Outpost 목록 수명과 분류까지 완료했다. 비미션 너겟 정상 생성과 간헐천을 실제 Session에서 검증한다. |
| 13 | 일본어 등 추가 언어. WOFF2가 없는 글꼴은 Google Fonts CDN을 사용하고 외부 접속 문서와 소스 감사를 함께 갱신한다. 일본어 Patch Credits에도 카피라이트를 넣는다. |
| 14 | 웹 자체 JS 사용으로 QuickJS 탑재 불필요. |

- [x] **15차 Reset All** — Options 최하단에서 확인 후 현재 게임 소유 설정·진행 키만 지우고 앱 정리 후 다시 로딩한다. 취소·확정·오류·최초 실행과 다른 앱 저장값 보존을 검사했고 Chromium 실제 메뉴에서도 확인했다. 영구 저장 키가 추가되면 초기화 목록도 갱신한다. [E863](docs/webpj-reset-all-20261119.md)
- **12차 Outpost 잔여.** 배치/거리 후보 목록의 실제 소유 관계 확인, 지역 소유 통지·Factory/연구/운반·AI·전투·Client 연결, 건설·금지 목록·삭제를 확인한다. 일반 캠페인/데모에서의 제외도 확인한다. [Outpost 부품](docs/webpj-raw-outpost-lifecycle-20261115.md) · [이관 전 범위](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801)

### 3.8 원본 관측 연결 이월

- [ ] **채팅 허브** — `chat-callbacks`·`chat-input-recipients`를 실제 Client/RawMenuControls·RawChatRuntime과 연결한다.
- [ ] **점 명령/HTML** — `html-input-dialog` 생성/공장 관측을 SaveFortNameDialog 소비자와 확인한다.
- [ ] **입력·장치** — `detonate-input`은 RawObjectConversion, `audio-reinterpret`·`font-face-enum`·`font-face-lookup`은 웹 적응 여부와 비교할 관측을 판정한다.
- [ ] **렌더러·음악** — `renderer-projection`·`scenemusic`의 소비자·적응 경계를 확인한다.

[원본 fixture 감사](docs/webpj-fixture-audit-20261115.md)

## 4. 남은 차이와 검증 한계

| 영역 | 후속 확인 |
| --- | --- |
| 지도·지형 | 중립색·가운데 카메라·고정 구름은 개발 미리보기 정책이다. 실제 지도에서 owner·원소 투표/themeMode·도장/flood·카메라 제한/스크롤·공유 난수·색 번호·팔레트/ColorSort와 `useBanners`를 확인한다. 절벽 외 0x10/0x18 지형 선택 및 회전2/3의 실제 저장 사례도 남아 있다. |
| edgeFarm·보상/SP | edgeFarm의 spots/finder·MayPlace·Retype를 실 월드와 맞추고, 보상/SP의 재시드·실제 지갑·알림/소리·보상/후처리 장부 공유는 4D-8·6단계에 연결한다. [지형 추가 관측](docs/webpj-terrain-extras-20261102.md) · [보상과 Content 수명](docs/webpj-content-lifetime-20261107.md) |
| 메뉴·입력 | 구름 비교 차이는 4:3의 37,054픽셀이다. Kencloud 연결 뒤 M1 시나리오를 갱신한다. Help/Options 위치·와이드 배치, Overlay 드래그/캡처·미션 커서, Edge Scroll과 Auto-Demo의 실제 동작은 미확인이다. |
| 도움말·Tell | `<info>` 머리글/능력치, `#`/`&` 맞춤·색 치환, mana.8~11의 실제 원본 F1 모습, `<background>`/nsisles 사용, `{@미션.키}`·스크립트 도움말 Back 및 필요 시 두 열 메뉴를 확인한다. 도움말 그림은 레이어0/그림자 없음 경계를 유지한다. |
| 소리·환경 | 실제 청취·위치 음향, 다양한 브라우저·GUI Linux·고DPI/모니터 이동은 미확인이다. |
| Credits | 영어 AI 문구 `OpenAI Codex, ChatGPT and Anthropic Claude Code`와 한국어 문구는 사용자 수정 지시까지 유지한다. 수정 시 `www/assets/product.js`와 `tests/product.test.mjs`를 함께 갱신한다. |

## 5. 검증과 완료 판정

| 단계 | 판정 |
| --- | --- |
| L1 모듈 | 원본 10.78 x86 fixture와 cpppj 검사로 반환값·슬롯·정수/float/double 비트·부작용·난수 순서를 대조한다. 체크섬 비교와 바이트 비교를 구분한다. |
| L2 시작 상태 | 같은 Fort·언어·옵션·시드 및 Load/Start 시점의 Session 덤프와 논리 화면을 비교한다. |
| L3 입력 진행 | 같은 프레임·입력 단계표의 상태·보고서·화면을 비교한다. 표시 주사율·비동기 자산 로딩과 게임 시계/난수를 분리한다. |
| L4 화면·소리 | L4a는 논리 화면과 cpppj snapshot, L4b는 소리 시점·곡 선택과 MCP 녹화를 비교한다. 전체 외관과 청취는 사용자 확인 범위다. |
| L5 자연 플레이 | 사용자가 자연 승패 → 새 완료값 저장 → 새로고침/재실행 → 다음 미션 해제를 확인한다. |

실제 브라우저·OS·논리/물리 해상도·DPI·음향/저장 환경과 검사 범위를 기록한다. 원본 fixture를 덮어쓰지 않고 미연결 장치, 웹 적응과 누락 fixture를 단위 문서에 남긴다.

## 6. 작업 명령과 참고

```powershell
npm --prefix webpj start
npm --prefix webpj test
node webpj/tools/port-map.mjs
node webpj/tools/fixture-manifest.mjs
node webpj/tools/headless.mjs --scenarios webpj/tests/fixtures/cpppj/m1-scenarios.json --out <폴더>
node webpj/tools/screen-compare.mjs --diff <폴더>
npm --prefix webpj run check-assets
npm --prefix webpj run test:e2e
```

- 개발 서버는 `http://localhost:9803/`이다. `?map=savetheisland&mute&lang=ko&aspect=4:3`는 규칙 없는 개발용 미리보기다.
- 필요한 항목만 `node webpj/tools/fixture-manifest.mjs --write --only <아이디,아이디>`로 갱신한다. 자산 변경 시 `npm --prefix webpj run prepare-assets`를 쓴다.
- Node v22는 HJOW-Athlon의 webpj 폴더에서 `& "C:\Program Files\nodejs22\node.exe" --test "tests/*.test.mjs"`로 순차 실행한다.
- [이식 대응표 사용법](docs/webpj-port-map-20261106.md) · [실행 자산](docs/runtime-resources.md) · [형식 목록](docs/formats/README.md) · [공유 명칭](assets/unit-names.korean.json) · [cpppj 소스 지도](cpppj/SOURCE_MAP.md) · [외부 접속 정책](docs/webpj-external-access-20261105.md)

## 7. 정리 기록

**2026-11-18.** E801 직후 문서 전체를 보관본에 이관했다. 오래된 재개·중단 안내, E632 계획 견적, 단위별 진행 기록 및 이미 끝난 체크의 상세는 보관본에서 확인한다. 출하 Fort 로그인과 GameWorldInput 코드 이식을 완료 하위 항목으로 명시했다. 출하 Fort 로그인·월드 선택/미니맵 이동·세 운반 유닛의 Session 이동 집중 검사 3건이 통과했다. 상위 4-D와 입력 본체는 남은 검증을 마친 뒤 완료한다. [E801 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801)

현재 AGENTS.md 기준은 `faecb9cb`(15차 설정 초기화), `2c8ca3bf`(갈무리11·다른 언어 글꼴), `d047bf9a`(cpppj·dotnetpj 실행 파일 아이콘, 웹 해당 없음)와 `11b16b77`(webpj favicon, [0.6절](#favicon))까지 반영했다. 이전 이관 기록은 보관본의 날짜별 원문에서 확인한다.
