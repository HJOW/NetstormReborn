# Subscriber ID 생성004bc950

2026-10-24 다음 단위용 신규 독립 복원이다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 마지막 수행2026-10-23**으로 같다. 단위383의 동결 Subscriber/Contents 자료,기존 공유 소스·CMake·LEFT·공유 build·GUI·git은 변경하지 않았다. 보호 PE의 최초 fixture를 먼저 고정하고 신규 본문과 대조했다.

## 실제 순서와 전역

004bc950은 현재DWORD005c89a4가0일때만 `time32(0)`→현재CRT `clock()`→두32비트값XOR→`srand(seed)`를 실행한다. time32의signed32비트와clock의상위비트를그대로유지한다. srand복귀뒤 **005c89a4를다시읽고현재값+1**을쓴다. 시간/seed장치가이를바꾸어도진입시캐시0에1을쓰지않으며,ffffffff에서0으로감기는것도원문대로다.

초기화분기와무관하게 **매함수호출100회rand를버린다**. 그뒤최대16회시도하며,각시도는새후보0에서32회rand를소비한다. bit31→bit0순서로 **signed rand%100의나머지<50**이면1을넣는다. 원래CRT는0..32767을반환하지만이부모자체에새15비트clamp를추가하지않는다. 음수명시RAND장치입력도signed idiv100과같이처리한다.

후보를 `0x07ffffff`로마스킹한뒤 **unsigned 후보>64**면원문EBP에저장하고ffffffff가아니면반환한다. 이ffffffff거부명령은그대로유지하지만27비트mask때문에정상입력의생성후보로는도달불가다. 0/1/64는재시도하고65부터허용한다. 이미생성한후보의상위5비트도32회난수소비가끝난뒤버리므로27회난수로줄이지않는다.

16회소진또는명시ffffffff거부는level9의정확한본문 `Expired retry count in generateSubId()\n`을출력하고실패DWORDffffffff로 **정상반환**한다. 로그callback이중단하면예외/명시중단이며실패반환과구분한다. 원문등록부모004b38d0은반환후그DWORD를현재Subscriber에쓰고설정식을계속만들기때문에이하위가실패를0/고정ID로치환하거나무조건throw하면호출자순서도달라진다.

## API와 같은 CRT 재사용

[RawSubscriberIdGenerate.h](../../cpppj/src/o/RawSubscriberIdGenerate.h)·[본문](../../cpppj/src/o/RawSubscriberIdGenerate.cpp)의 `GenerateSubscriberId({initialized참조},hooks)`는현재005c89a4만참조로빌린다. hooks의time32/clock/seed/next/expired는실제도달할때만필수다. 초기화플래그가비영이면time32/clock/seed가없어도선택하지않으며,성공후보면실패로그를요구하지않는다. 장치중단시앞선플래그/CRT소비를복원하지않는다.

`BindSubscriberIdCrtRandom(CrtRandom& crt,hooks)`는 **기존실제CRT객체의참조**를캡처해seed→SetState,next→Next만연결하고나머지시간/로그장치를보존한다. 기존 [CrtRandom](../../cpppj/src/o/CrtRandom.cpp)의 `state=state*214013+2531011` DWORD감김/상위15비트와SetState몸체를재사용한다. 새기본CRT·독립시드·std::rand·GameRandom·임의ThreadCrtRandom생성은이모듈에없다. 반환hooks보다그실제CrtRandom객체가오래살아야하며임시Borrow객체를넘길수없도록lvalue참조를요구한다.

**005c89a4는Client/프로세스전역수명,CRT는같은스레드의기존소비자와같은상태수명**이어야한다. 현재Session별 `crt{}`에새연결해세션마다재초기화하면첫1회/이후공유시퀀스가원문과달라진다. 기존소유상태를 `CrtRandom::Borrow(currentState)`로빌릴경우그빌림객체자체도hooks가끝날때까지유지한다. root가후속Client/전체CRT소유권을결정하며이독립모듈이그이행을완료했다고하지않는다.

이생성기는현재Subscriber ID나handle에직접쓰지않는다. [Subscriber 등록부모](cpp-fort-subscriber-load-20261024.md)의generate hook에위반환값을연결하며,등록부모의현재ID쓰기/Config ReadInt/오탈자registerdSubId설정순서는별도소유다.

## 최초 원문과 검증

[관측 도구](../../tools/decomp_subscriber_id_generate.py)는실제 **004bc950부모161BYTE+srand004e649e13BYTE+rand004e64ab34BYTE=208BYTE**를실행한다. PTD getter는같은명시TLS블록주소만반환하고그블록+20의현재holdrand쓰기/소비는실제CRT명령이다. time32/clock/실패로그는명시OS/출력경계다. rawMode1만별도의signedRAND응답장치를사용해자연CRT에서희귀한64이하/최대16회소진경계를강제로공급한다. 이경우를실제CRT출력으로가장하지않는다.

[최초fixture](../../cpppj/tests/fixtures/subscriber-id-generate-x86.tsv)는512입력×2정밀도=**1024행**,정상994/명시중단30,연속두번호출을포함한정상함수반환1458회,3,807,618명령/정밀도차이0이다. 현재플래그0/1/ffffffff/80000000·CRT0/1/7/ffffffff/80000000·time32/clock상위비트·같은상태연속두번·현재플래그/CRT의시간/seed/rand뒤변경·warmup1/100·후보첫rand101/마지막132중단·signed RAND−1/−100/INT_MIN/INT_MAX·나머지49/50/99/100·후보0/64/65/상위비트·15회실패후16회성공/16회전체실패·로그중단을포함한다.

[최초보고서](../../cpppj/subscriber-id-generate-native.json)의원본/정밀decompile/도구/fixture SHA를 `--verify`로검사했고종료0이다. 최초원문은제품결과로재생성하지않는다. CD10.72의부모대응은정밀분석의004bbb60이며이번실제PE관측은10.78만대상이다.

[SubscriberIdGenerateTests.cpp](../../cpppj/tests/SubscriberIdGenerateTests.cpp)는1024행의현재initialized/CRT/호출별반환/매난수직전전역·CRT를포함한전체trace와명시중단을비교한다. 추가검사는비영초기화플래그의미선택시각장치허용,clock누락직전시간효과보존,실제seed뒤현재플래그감김/warmup장치누락,612회(100+16×32)소비뒤실패로그누락의효과를확인한다. 별도읽기검토에서도본문과참조수명의명백오류는발견되지않았다.

[격리 CMake](../../extracted/cpp-subscriber-id-generate-20261024-isolated/CMakeLists.txt)는기존CrtRandom.cpp를읽어컴파일하고신규모듈/검사만링크한다. **Release 경고·오류0,2검사/check실패0·종료0**이며공용라이브러리도링크하지않았다. 로그/제품·fixture SHA는 [검증기록](../../extracted/cpp-subscriber-id-generate-20261024-isolated/verification.json)에보존한다. 게임·GUI·실제OS함수·전체Client/Session설치·공유전체CTest는실행하지않았다. 공용등록/실제적용은root의단위383커밋이후별도작업이다.
