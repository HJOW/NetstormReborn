# buried·Rune의 현재 후처리와 선택 장치

단위857 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치이며 산출물 변경0이다. 직전856은 `42e8f1bc`이고 현재제품은857/11.01이다. 전체 목표는 미완료다.

PATCH00462170은 현재 boss/로딩 관문이 허용한 최초 보상 추첨 뒤004ac220의 물리 WORD 저장과 현재 Bridge DWORD 조건부004214a0 통지를 실행한다. 이후 최초 두40BYTE 확보를 독립 시도하고 성공한 생성자만 요구한다. 항상004acd10으로 현재 QA WORD를 읽고0049a840으로 사용하지 않는 주소를 계산한 뒤 공통004b0d30으로 간다. 실제 몸체 앞 전체 Validate/AllocatedBytes 선행 거부를 제거했다. 효과 뒤 free/contained/TYPE/VT를 되돌리거나 재검사하지 않는다. WORD 통지는 같은 World/Session의 현재 Bridge DWORD와 실제 NotifySurface를 빌린다. QA65535도 unused 주소 계산 때문에 TYPE 표를 역참조하지 않는다. 명시 Validate는 별도 무효과 사전 계약이며 CD buried는 기존 미지원 경계를 유지한다.

PATCH00448930은 최초40BYTE 확보와 성공 때304/0.01f 생성 뒤 flags2에서 현재 Dais finder00448a20과 시간 helper00448340을 실행한다. 발견한 Dais가 시간 helper의 this이며 Rune SID를 건설에 전달한다. 시간 계산은 건설 장치 확인보다 앞선다. PATCH 생성자의 전수 장치 검사와 실제 몸체 앞 Validate를 제거하고 실제 VT로 분배한다. 명시 Validate는 기존 TYPE/가상표/공통 조건을 확인하고 CD00484520/00482990의 기존 구성/타입 사전 계약은 유지한다. Session 시간 helper는 PrepareRitual 때 사본만 사용하던 차이를 고쳐 같은 현재 missionPost와 Player+50 BYTE를 도달 위치에서 읽는다. Player의 실제9기록 밖 접근은 호스트 물리 경계로 거부한다. 독립 구성의 state fallback은 유지한다.

새19개(순수18+Session1) 포함52개/10묶음을 확인했다. 기존 buried580행과 양판본 Rune1920행의 전체 raw/효과를 재생했다. 새 검사는 도달 장치/실패 앞 부분 효과·현재 TYPE/수명/소유자/Bridge·시간 전역·QA 물리 경계와 CD 차이를 확인한다. 기존 buried의 잘못된 VT 거부 단언1개는 실제 몸체 대신 명시 Validate를 호출하도록 API 계약을 교정했고 fixture 입력/기대값은 바꾸지 않았다. 실제 Session은 TYPE74를 유지한 buried flags0와 Rune 최초304 Regular의 지연/카운트/같은 시계만 확인한다. 시간 장치 연결은 순수 현재 저장소 계약 검사/코드 대조 범위이며 실제 Session 건설 시간/진행의 동작 관측으로 합산하지 않는다. Frame/Process 틱·의식/포획/희생/공격/승리는 실행하지 않는다. Release 제품/두 집중 대상1회는 종료0·컴파일 경고/오류0이고 집중52개(새19)/10묶음은 첫 실행 실패0이다. 핸들98445 종료/자식 해제와 125.7380182초·첫10소스 보존/제품 소스9개 불변을 확인했다. 빌드 중 소스 변경/교정 재실행0이다.

첫10소스·정밀 원문8개와 보호2761개/fixture941개 기준은 Git 제외 `extracted/cpp-resume-857/`에 보존했다. 새 Native 관측/fixture0이다. 기존 양판본 기대값을 바꾸지 않으며 checking12·완료부모36/부분3은 유지한다. Player/Dude·섬/표면/Teleport와 다른 현재 가상20/접두·전체 Pop 및 후속 단계는 미완료다. 사용자 직접 확인·공중3종 전용·신규MP/통신·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복을 제외한다.

기존 보호2761개/fixture941개는 모두 불변이다. 현재 EXE SHA-256은 `4d09e165c97795a2e625ba377f4c6043665fb076e6024d2d0e53696d68333aea`다. [검증 JSON](../../cpppj/buried-rune-current-20261104.json)에 첫 소스/원문/기존 fixture SHA·실제 검사 개수/종료값·검증 한계를 보존했다. 단위 인계/로컬 커밋 뒤858로 이어가며 목표는 active다.

빌드/검사 뒤 CMakeLists.txt는 기존 혼합 줄바꿈을 보존해 무관한 줄바꿈 변경만 제거했다. LF 정규화한 첫 빌드 내용과 동일하며 다른9개 제품/검사 소스 바이트는 불변이다.
