# Player 기술 링크와 동적 Deck 반납 부모

2026-10-24 **387 이후 독립 준비**다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. PATCH10.78 **0048fcd0/79BYTE**(`48fcd0-48fcd7;48fce0-48fd26`)를 새 제품 전에 독립 PE로 고정했다. SourceMap CD10.72 대응은 **0046dfb0**이며 CD 몸체는 실행하지 않았다. 공유 기존 소스·CMake·LEFT·git·공용 build/lib·GUI와 보호 원본/CD는 변경하거나 실행하지 않았다.

## 실제 필드와 원문 순서

이 부모는 현재 owner 전역이나 PlayerKnown을 읽지 않는다. **Player0..8 고정9회**를 순서대로 처리한다. 각 기록의 원문은 현재0059534c 재조회→**+9c DWORD0**, 다시 현재base 재조회→**+8 DWORD0**, 다시 현재base 재조회→**+68 DWORD 읽기**다. 마지막 값이0이면 free와 뒤의0쓰기 모두 생략한다. 비0이면 그 캡처 토큰을 **_free004e4354**에 넘기고 복귀 뒤 **현재base를 다시 읽어 +68 DWORD0**을 쓴다.

따라서 free가 base A→B로 바꾸면 A의 기술SID/+8은 이미0이고 **반납인자는A의토큰**, 뒤의+68=0은 B에 수행한다. A의+68을 편의상 같이 지우거나 B의 기술/+8까지 다시 지우지 않는다. 중단 시 앞선0쓰기는 남고 free뒤 pointer0과 다음 기록 처리는 아직 수행하지 않는다.0필드에서 누락 release를 선행 요구하지 않는다. 재진입마다 로컬 기록 위치는 별도이고 같은 현재base/free 장치를 공유한다.

기술SID+9c를0으로 쓸 뿐 기술 객체나 그 contained 목록을 가상삭제하지 않는다. +8은 Player+0의 동적 목록과 함께 쓰는 count 필드다. +0의 별도120BYTE 할당과 Player1620BYTE 전체는 반납하지 않는다. 그 반납/재생성은 뒤의 **004910a0**이고 이 부모와 다르다.

## 기존 하위의 정확한 재사용 범위

원문 **0044f700**을 확인해 +68은 **new0x160(352BYTE) 생산Deck**임을 확정했다. 없을 때만 할당하고 다섯DWORD를 초기화한 뒤 같은Player+68에 부착한다. 기존 `RawProductionDeck`는 이 정확한 getter/Player+68/토큰별352BYTE blocks_를 단독 소유한다. 따라서 실제 Session에서는 같은 productionDeck 소유자에게 캡처 토큰 하나를 반납해야 한다.

현재 그 클래스에는 **Release(token) 공개 API가 없다.** 소멸자는9현재 포인터를 검사하고 소유 map 전체를 반납하며, `ClearEntries`는 항목/카운터만 초기화한다. 둘을 이 부모의 선택free로 치환하면 시점/효과가 다르다. 후속 최소 API는 기존 blocks_에서 **그 토큰 한 개만 erase하고 Player 필드는 건드리지 않는** 반납이다.0/미등록 토큰 처리의 범위도 실제 선택 계약으로 정해야 하며 host 포인터를DWORD로 잘라 std::free에 넘기지 않는다. 이번에는 기존 클래스나 그 소멸자를 수정하지 않았다.

`RawPlayerWorld::CurrentTechnology/Technology`는 현재+9c를 읽거나 생성후 링크를 쓰는 몸체다. 이9기록 정리 전체를 대신하지 않는다. `RawPlayerTechnology::Clear`는 contained 기술을 찾아 가상삭제하는 **다른 부모**다. 지식 메뉴는 같은기술/TYPE/Deck을 조회하는 UI이며 이 동적 블록 반납 소유자가 아니다. 단순vector.clear/Player초기화/가짜포인터0으로 원문free를 생략하지 않는다.

## Native 선관측과 공개 API

[도구](../../tools/decomp_player_links_cleanup.py)·[최초TSV](../../cpppj/tests/fixtures/player-links-cleanup-x86.tsv)·[보고서](../../extracted/cpp-player-links-cleanup-20261024-native.json)는 신규 h/cpp/test 세 파일이 없는 상태에서 **81입력×양x87모드=162관측·19842명령**을 고정했다. **정상138/명시중단18/실제NULL미매핑6**, 선택free644회이며 양 정밀도 출력 차이는0이다.

두Player각1620BYTE 전체, 현재base와 외부장치가 바꾼owner 값, 매실제현재base load와 free 인자 순서를 기록했다. 전/후base 변경, pointer 재쓰기/다음pointer0, 외부owner8 변경, free1/2/5/9/10 중단, 실제 부모 재진입을 포함한다. fixture의 releasedOpaqueTokens 칸은 **free 호출에 전달한 인자/시도**이며 해당free가 중단됐을 때 실제 allocator 반납이 완료됐다는 증거가 아니다. 재진입의 같은토큰 중복free도 그대로 관측하지만 실제CRT에서 안전한 동작이라고 주장하지 않는다.

[신규 API](../../cpppj/src/o/RawPlayerLinksCleanup.h)는 `CleanupPlayerLinks(PlayerLinksCleanupHooks)`이며 **players() 순수현재span provider와 release(nonzero token)**만 빌린다. 각 원문접근 직전 provider를 다시 부른다. 배열길이/owner를 선행 검사하지 않고 선택한 DWORD 접근에서 host 물리를 확인해 앞선 효과를 보존한다. 현재base가NULL이면 원문은 미매핑 오류이고 C++는 선택span 범위오류로 구별한다. 잘못된 host 포인터 접근을 실제fault로 실행하지 않는다.

이 모듈은 Player/Deck/Core/9×16 장부를 소유하거나 초기화하지 않는다. State정리 원문의 **004b30ff ClearPlayerLinks**는 Kernel/Gump/Uber 정리보다 앞이고, 이어 별도색표43b540 뒤 **004b312d SelectCore(0,1)**, **004b31ad PlayerInitialize4910a0** 순서다. 새 Core9기록 CRT나 SelectCore0의 캐시복사를 이 자식 안으로 옮기지 않는다. 실제 순서는 [State 부모](cpp-state-world-cleanup-20261024.md), [장기수명 설계](cpp-client-long-lifetime-next-20261024.md), [Core9기록](cpp-player-core-records-20261024.md)과 함께 연결해야 한다.

## 격리 검증

신규 부모/검사만 독립 콘솔에 컴파일했다. **첫 build1 종료0·/W4 관측 경고오류0, 실제 test2 종료0·3검사/실패check0**이다.162행의전체물리/현재값/trace가모두일치했다. 최종 빌드/검사 상태와 SHA는 [격리 manifest](../../extracted/cpp-player-links-cleanup-20261024-isolated/verification.json)에 기록한다. 실제 Session/RawProductionDeck 반납 API/Native State 외곽/게임·GUI 성공은 이번 범위가 아니다.

검사는162관측의 전체두Player/현재전역/free인자/현재base조회trace 대조, 실제352BYTE 할당을 소유한 registry의 선택 반납과0필드 미선택, 누락free/짧은span의 prefix효과를 구분한다. 실제 소유 registry 검사와 Native의 명시 CRT 경계는 서로 다른 증거다. 최초 실행물 생성 전 테스트 호출은 프로그램이 없어 실패했으며 C++검사를 실행한 실패로 세지 않고 별도 기록에 보존한다.

다른 작업자가 h/cpp/3검사를정밀C와읽기대조해현재base재조회/선택free/중단/재진입및실제Deck소유자미연결구별에명백한오류가없음을확인했다.그검토는추가Native/빌드/실행을수행하지않았다.

공용 등록은 신규 `src/o/RawPlayerLinksCleanup.cpp`, `tests/PlayerLinksCleanupTests.cpp`, **NETSTORM_PLAYER_LINKS_CLEANUP_FIXTURE**에 최초TSV 절대경로를 공급한다. 기존 제품/공용 등록/빌드 변경과 실제 release 자동바인딩은 root 후속 범위다. 최초 도구/fixture/report와 기존 사용중인 Core/SavedReader/Cleanup 자료는 수정하지 않았다.
