# 마나 공급원 등록의 중복 진단과 현재 생산 카운터

단위826을 완료했다. 제품은 **826 / 표시11.01**이다. 이전 완료는 `a9157275 / 20261104-07 CP`다. 현재 PC와 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이며 작업 전 AGENTS.md·LEFT_JOBS.md·dotnet 인계를 다시 읽었다. 최종 빌드와 집중 검사16개가 통과했으며 전체 목표는 미완료다.

원문 등록 함수는10.78 `00472f70`,CD10.72 `0048a360`이다. 유일한 직접 caller는 각 판본의 공통 postPop `004b0d30`/`004ae180`이다. 최초 등록 flags bit0·abstract/buried 아닌 원소에서 비용 누적 뒤 TYPE.flags1의0x10000000을 읽고 호출한다. 이 함수는 원문 이름이 Mana.cpp인 공급원 등록이며 커서 미리보기 마나103의 별도 목록이 아니다.

원문 인자는 전체 DWORD이며16비트 SID로 줄이지 않는다. 활성 목록 앞에서부터 첫 중복을 찾는다. 패치는 최초 count/포인터를 레지스터에 저장하고 CD는 첫 원소를 검사한 뒤 반복 조건에서 현재 count를 다시 읽는다. 패치만 검색 뒤 현재 checking 전체DWORD를 읽고 중복이면 `!alreadyThere`, `\Ns\O\Mana.cpp`,0x44(68)줄 진단을 선택한다. 진단이 목록 포인터/개수/원소를 바꾸더라도 저장한 중복 판정으로 재추가를 생략한다. 중복이 아니면 signed count/capacity 비교 뒤 여유가 있을 때 store→현재 count 증가 순서를 수행한다. 마지막에는 중복/가득 참 여부와 무관하게 현재 생산 갱신 카운터를 한 번 감김 증가한다. 진단 중단은 증가까지 진행하지 않으며 앞선 외부 효과를 되돌리지 않는다.

새 RawManaRegistration과 SquidPostPop의 BindManaDiagnostics를 연결했다. 기본 World는 장기 RawSurfaceStorage의 현재 checking/진단 장치를 빌리고 Session은 같은 ClientGameplayGlobals의 AiDebug DWORD/현재 AI 진단 서비스로 연결한다. Session 진단 함수는 서비스 함수를 소유하며 짧은 Session this를 외부 World에 보관하지 않는다. 공급원은 기존 RawStateSidLists.providers의 실제256DWORD 배열/현재 count이며 별도 사본을 만들지 않았다. 일반/파생 Pop은 기존 공통 postPop을 통해 같은 함수를 사용한다. 기존 공급/배치/에너지/Dude/삭제 소비자는 같은 Providers 뷰를 유지한다.

독립 CPU 관측기는 읽기 전용 원본 PE를 자신의 전용 payload로 복사해 위 작은 두 함수만 실제 x86 CPU에서 호출한다. 진단 장치만 관측/입력 변경 경계로 바꾸며 원문 검색/판단/배열 쓰기/카운터는 계산해 대체하지 않는다. **1,084행(판본별542),중복 진단135행**이며 재관측이 정확한 바이트로 일치했다. 빈/여유/가득 참/축소·signed 음수 용량,중복 없음/앞/뒤/양쪽,0/50/65536/ffffffff 인자,checking0/상위비트/ffffffff·카운터 감김과 진단 중 현재 pointer/count/capacity/원소/카운터 변경을 포함한다. 두16DWORD 물리 배열의 비활성 꼬리까지 보존한다. fixture SHA-256은 `4711f8b9998cd3e60f2cb5538dca4f84e95b8f24bf7e5477b5d00e0b3f7c532b`다.

호스트 배열 밖 검색/쓰기·음수 저장 인덱스·DWORD를 초과한 capacity는 별도 오류다. 원문 손상 포인터/배치 자체를 임의의 큰 배열로 정상화하지 않았다. 기존 공통 postPop의 ValidateBase가 보존하는 손상 목록 사전 거부와 하위 등록 함수의 실제 물리 범위/읽기 시점을 구별한다. 현재 count/capacity 뷰는 실제 장기 저장소를 가리킨다.

최종 집중 **16개(새6) 실패0**이다. 새 CPU2·최초 읽기 순서1·선택장치/부분효과1·실제 Client/두 World대여1·실제 Session1과 기존 postPop6/연소4다. 실제 World 진단에서 목록 개수를0으로 바꾸고 현재 카운터를ffffffff로 만든 뒤0 감김을 확인했다. 동시에 raw TYPE/owner를 바꿔 저장된 Factory flags2로 추가를 생략하고 현재 TYPE/owner 통계까지 이어가는 것을 대조했다. 두 World의 같은256DWORD 배열 주소와 실제 TheWarBegins 보호 자료의 공급원/Session 기본 진단도 확인했다. 게임/Kernel·AI·공격 프레임은 실행하지 않았다.

첫 집중15개도 실패0이었다. 정적 검토에서 CD0048a362의 양수 분기 뒤0048a374의 첫 배열 비교/중복 종료가0048a37c의 다음 현재 개수 읽기보다 앞선다는 것을 확인했다. 첫 C++ for조건은 그 전에 현재 개수를 다시 읽었으므로 do본문/후방 조건으로 보완했다. 첫 count가1을 반환한 직후 현재 count가0으로 바뀌는 독립 읽기 장치 검사를 추가했다. 원문 순서대로 두 판본 모두 첫 중복을 찾아 한 번의 count 읽기·재추가 없음·현재 카운터1 증가를 유지한다. CPU fixture는 정적 메모리 입력이므로 이 동적 장치 경계는 기계어와 별도로 대조했다. 최초/최종 빌드와 집중 로그를 보존했으며 Native/기존 기대값을 바꾸지 않았다. 이번 단위 컴파일/검사 실패는0이다.

Release2회와 x86 프로브1회는 종료0·경고/오류0이다. EXE SHA-256은 `10f8c29fb973ab33dcac6e644554cfade76e6b759370cb657b56ad8dd87a6abe`다. 기존2753개/fixture933개는 변경0이며 새 CPU fixture를 포함해 보호2754개/fixture934개다. 상세 결과/소스·로그 해시는 `cpppj/mana-registration-checking-20261104.json`과 LEFT_JOBS.md를 따른다. checking은 **20=부모12+부분5+MP분류3**,완료부모30이다. 다음827은 Terrainbuilder004c0e20의 첫 유효 청크 영역 조회/실패 진단·실제 생성/편집 소비자다.

정적 검토 중004a7510은 Reconnecting/참가자 Subscriber 재연결·팀 재편의 멀티플레이 부모로 확인했다. 779의 부모 분류 집계에는 미완료로 남기고 작업을8번MP 사전 구현으로 이월한다. 해당 함수의 신규 검사는0이며 이번 공급원 완료로 합산하지 않는다. 실제 게임/GUI·포획/희생/특정 적 공격/승리·사용자 직접 확인·MP/공중3종 새 전용·물리 화면/PCM·전체34/71/CTest/ZIP 검사는 실행하지 않는다. 원시는 Git 제외 `extracted/cpp-resume-826/`이며 기존2753개/fixture933개를 보호 기준으로 고정했다.
