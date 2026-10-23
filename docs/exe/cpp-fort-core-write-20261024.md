# CoreData 저장 부모 004beb30

현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 원본 10.78의 364BYTE를 Unicorn 격리 메모리에서 실행했으며 게임/OS/GUI는 실행하지 않았다. 공유 기존 소스·CMake·LEFT·공용 빌드·라이브러리·git은 변경하지 않았다.

신규 [RawFortCoreWrite.h](../../cpppj/src/o/RawFortCoreWrite.h) / [본문](../../cpppj/src/o/RawFortCoreWrite.cpp)의 API는 `WriteFortCoreData(slot, globals, hooks)`다. [검사](../../cpppj/tests/FortCoreWriteTests.cpp)와 [격리 검증](../../extracted/cpp-fort-core-write-20261024-isolated/verification.json)을 별도 보존한다. Editor 외곽과 Battle Snapshot의 `coreData`에서 같은 부모를 호출할 수 있다.

1. 고정 this의 현재 manager0은 Template.cpp `0x45f` 진단이다. 이어 현재 Metamap00594fc8 비영은 `0x460` 진단이다.
2. 그 뒤 현재 missionPost00594fa4를 읽는다. 비영이면 editor도 읽지 않는다. post0일 때 현재 editor005c85a4 비영이면 자연 반환한다. 이 두 자연 경로에는 Create/Player/seed/RNG/직렬화/SP 장치가 필요 없다.
3. 둘0이면 고정 this의 현재 manager를 다시 읽어 실제 `Create("CoreData")`를 호출한다. 반환 stream은 끝까지 유지한다.
4. Create 뒤의 현재 Player base0059534c에서 this 슬롯1..8의 Player를 고정한다. 그 밖의 정렬 slot은 Player0이다. 현재 playerOwner00540c70 불일치는 `0x467` 진단이며 Template owner00540cac와 구별한다.
5. 실제 0044fc00을 호출한 뒤 앞서 고정한 Player+0x6c의 같은10BYTE를 일반 writer0044cce0에 넘긴다. callback이 현재 Player base를 바꿔도 기존 물리를 유지한다.
6. 그 뒤에만 `globals.seed()`로 실제005c89b4의 같은 DWORD 참조를 빌린다. low16이0이면 실제 prepare004559d0 뒤 Next004558c0(65536)을 반드시 한 번 이상 호출한다. 반환 low16이0이면 반복하고 각 반환 때 현재 상위16은 보존한다.
7. 현재 seed DWORD를 로컬에 보관해 낮은2BYTE를 같은 stream에 일반 쓰기 한다. 마지막 세 SP004be960 호출 직전마다 고정 Player의 byte+0x70, word+0x6c, word+0x6e를 각각 다시 읽고 unsigned→float32로 넘긴다.

누락된 선택 장치는 그 위치에서 예외를 던진다. 부분 파일/Player/seed/난수 효과를 rollback하지 않는다. 실제 Player 할당은 함수가 끝날 때까지 살아 있어야 하며 현재 span의 변경으로 옛 할당을 반납해서는 안 된다. 잘못된 host 물리는 원문 성공으로 가장하지 않고 선택 접근 위치에서 명시 거부한다.

최초 제품 작성 전에 [338관측](../../cpppj/tests/fixtures/fort-core-write-x86.tsv)과 [진단3/상위DWORD22관측](../../cpppj/tests/fixtures/fort-core-write-extra-x86.tsv)을 고정했다. 최초 도구에서 `create/refresh/prepare` 변경명은 실제1기준 경계명과 달라 변경되지 않았다. 그 자료를 고치지 않고 실제 `create1/refresh1/prepare1`을 선택한 [별도36관측](../../cpppj/tests/fixtures/fort-core-write-callbacks-x86.tsv)을 독립 PE로 추가했다. 총396관측은 정상372/명시중단24, x87 두 정밀도 차이0이다. 제품 기대를 원문 입력이나 결과로 역산하지 않았다.

전체 global/두 Player 할당의 CoreData/일반 쓰기 부분출력/SP float 비트/외부 호출순서를 대조했다. seed의 실제 CPU WORD/DWORD 읽기는 원시 fixture에 보존하며 C++ 직접 참조 load에 인위 callback을 끼우지 않았다. 제품 비교는 seed 최종DWORD·2BYTE 출력·난수 소비와 그 밖의 전체 trace를 사용한다.

격리 MSVC Release `/W4 /utf-8 /permissive-` 빌드와 신규 **5검사 실패0**이다. 첫 빌드/검사와 seed를 지연 getter로 좁힌 뒤 둘째 빌드/검사를 모두 보존했다. 마지막 빌드·검사 종료0/관측 경고·오류0이다. 공용 검사 등록·링크·전체 Editor 성공 검증은 수행하지 않았다.

후속 연결 경계는 다음과 같다.

- **005c89b4**는 PE `.data`의 raw 범위 밖 virtual zero이므로 mapped 최초DWORD=0이다. 현 Session/Client에는 같은 상태 owner가 없다. 004b2df0은 lowWORD만0, NewFort004b37c0은 새 비영 lowWORD 생성, CoreLoad004beca0은 저장WORD 복원이다. CRT/Subscriber init/GameMode와 alias하거나 설치용 스크래치로 대체하면 안 된다. 자연 Editor에는 getter를 비워 미선택으로 유지할 수 있다.
- **0044fc00**은 현재 Core+6 DWORD를0으로 쓰고 기술 부모의 자식 flags4/TYPE+0x1ec로 Ch 비트를 갱신한다. 이어 자기 Player의 살아 있는 객체 vfunc0x80 최댓값으로 현재 Core+5의 하한을 갱신한다. 현재 제품에 같은 본문이 없으므로 선택 시 필수 실제 경계다.
- **004be960**은 일반 SP 저장소가 아니다. float의 실제 `_ftol` 값32비트를 각16회 `Next(32)`의 잡음과 섞어 8BYTE/8WORD/16DWORD, 총88BYTE를 인라인 저장한다. 한 호출512난수이며 현재 제품에 같은 본문이 없다. `WriteMemStream`의 float4BYTE나 `ScrambledSpStore`로 대체하지 않는다.
- 실제 일반 writer는 기존 Storage.WriteSection, Create는 같은 Storage.CreateSection, RNG는 같은 Session의 `seed.Reseed()` / `random.Next()`를 재사용할 수 있다. refresh/SP/seed owner는 별도 실제 복원·접합 전까지 선택 누락을 유지한다.

CD10.72 대응은 정밀 자료의 `00429090`이다. 이번 PE 관측은 10.78만 대상으로 하며 CD 동일성을 주장하지 않는다.
