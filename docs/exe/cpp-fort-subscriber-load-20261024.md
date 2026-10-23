# Fort Subscriber 파일 꼬리004bd450

2026-10-24 신규 독립 복원이다. 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 기존 공유 소스·CMake·LEFT·공유 build·GUI·git은 수정하지 않았다. 최초 독립 PE fixture를 고정한 뒤 신규 모듈을 작성했다.

| 실제 몸체 | 바이트 | CD10.72 대응 | 역할 |
|---|---:|---|---|
|004bd450|122|004276f0|고정Template 진단/선택→ID 직접쓰기→문자열→등록마감|
|004bcb60|144|00426890|signed WORD 길이→직접복사→현재pos/end 진단→NUL|
|004b38d0|78|00430290|현재ID==-1일 때 설정/새ID/설정식 처리|

## 원문 순서

004bd450은 고정this의현재manager0 진단 `Template.cpp:0x1d8` 뒤 **현재manager를 다시 읽어** `Select("Subscriber",0)`한다. 반환한 같은stream의현재data+pos에서ID DWORD를 읽어 **실제00540c74에 먼저 쓴다**. 그 뒤 현재pos와end를 다시 읽고pos+=4를 써서signed pos>end면 `memstream:0x2e`를 보고한다. ID/pos 쓰기를 진단 뒤로 미루지 않는다.

004bcb60은 현재data/pos의signed WORD를 먼저 캡처하고pos+=2/signedend진단한다. 진단이data/pos를 바꾸어도길이는 캡처한값을 유지한다. **진단복귀 뒤의현재data+pos**를복사주소로잡고DWORD단위앞방향복사→잔여BYTE복사한다. 내부NUL도저장길이만큼복사한다. 그뒤현재pos와end를다시읽어pos+=signExtendedLength/signed진단→출력주소+저장길이에NUL순서다. 미리논리끝/256자제한으로복사를생략하지않는다. 음수길이의거대한직접복사는명시물리범위끝에서부분효과를보존한실패로구별한다.

004b38d0은 그때 **현재ID가ffffffff일때만** `ReadInt("registeredSubId",현재ID참조)`한다. 반환0이면004bc950의반환DWORD를현재ID에쓰고,CRT `%u`의unsigned십진수로 `registerdSubId = "값"`을 만들어00441580에넘긴다. **쓰기키의오탈자registerdSubId와읽기키registeredSubId가서로다르다.** ReadInt는키존재가아닌값변경여부이므로설정값도−1이면새ID생성이필요하다. callback이ID를바꾸면그다음원문위치의현재값을따르며생성callback의임시ID변경은반환값대입으로덮인다.

정상파일부모004b5ca0의공통꼬리004b5e80은 이부모복귀뒤에현재handle로 `fort.mySubHandle`설정→00460730창제목갱신한다. Subscriber본문은Config의사용자이름설정/제목/World/TYPE/loadingDepth를추가하지않는다. 같은이름의미리읽기나World뒤실행으로옮기지않는다.

## API와 Storage 재사용

[RawFortSubscriberLoad.h](../../cpppj/src/o/RawFortSubscriberLoad.h)·[본문](../../cpppj/src/o/RawFortSubscriberLoad.cpp)은 `LoadFortSubscriber(slot,handleSpan,{currentSubscriber참조},hooks)`와독립 `FinishFortSubscriberRegistration`을제공한다. handle은호출진입때전달된출력주소이며이본문이00540ca8을다시읽지않는다. 출력의stale꼬리/내부NUL/부분복사를유지한다. 물리범위/실제장치누락은앞선효과를보존한예외다.

Session바인딩은같은 `Impl::globals.mySubscriber`와 `Impl::handle`을사용한다. hooks.manager는고정slot의현재 `Storage.Headers()[slot*20]`,select는실제 `SelectSection(slot,"Subscriber",0)`의생성수명registry,access는그반환참조의 `AccessSection`으로연결한다. 원시fields/버퍼/장치는동기operation밖에저장하지않는다. 등록마감은access반납뒤실행하므로Configcallback이파일/선택을바꾸어도끝난stream빌림을계속붙잡지않는다. Session의현재64BYTEhandle을넘는파일이름은그실제물리끝에서앞선복사를보존한실패로남긴다.

현재 `CampaignTemplateStorage::ReadSubscriberName`은004bd4d0의목록용동작이다. ID를읽거나쓰지않고4BYTE건너뛰며등록마감도없다. 추가로길이/end/256BYTE를복사전에검사하므로이정상파일꼬리의대체로사용할수없다. 기존목록검사/계약은바꾸지않고실제SelectSection/AccessSection과같은manager/노드/버퍼만재사용한다.

`readRegistered`는선택시에만현재Client Config.ReadInt에연결한다. Config의signed int32출력은DWORD비트를보존해같은ID에동기반영해야한다. `configure`는원문00441580의키/값해석과변경callback을갖는실제설정경계다. 이고정식은키registerdSubId와unsigned값으로Config.Set에연결할수있지만 signed SetInt로값을바꾸거나callback을생략하지않는다.

004bc950은아직이모듈의외부필수장치다. 최초005c89a4==0에서time32 XOR clock으로CRT srand하고플래그를증가한뒤100회버림→32회의rand조건으로27비트ID를만들어최대16회재시도한다. 기존게임GameRandom과별개이며독립고정ID/게임난수/매Session새시드로대체하지않는다. 이생성기와장기Client ID/handle소유자의후속연결을완료했다고하지않는다. PE초기현재handle포인터00540ca8은00540c78을가리키지만새모듈은호스트포인터를DWORD로자르지않는다.

## 최초 원문과 검증

[관측 도구](../../tools/decomp_fort_subscriber_load.py)는세실제몸체 **344BYTE**를실행한다. Select/진단/ConfigRead/새ID/CRT서식/Config식만명시경계다. [fixture](../../cpppj/tests/fixtures/fort-subscriber-load-x86.tsv)는347입력×2=**694행**,정상658/물리실패22/명시중단14,68,452명령/정밀도차이0이다. 최초SHA256은 **`b411318ef787f7401779816b568104c8283e3046a24aab311e7c1eadaa6bc638`**이며 [최초보고서](../../cpppj/fort-subscriber-load-native.json)는제품작성전상태로보존한다. 도구 `--verify`의모든근거SHA검사는종료0이다.

manager0/진단뒤manager변경·반환stream교체·data0·pos0/64·end0/1/6/512/ffffffff/80000000·파일ID0/1/ffffffff/80000000·길이0/1/3/4/7/31/63/64/127/255/256/8000/ffff·진단뒤data/pos/end/ID변경·등록Read/생성/Config뒤ID변경·각장치중단을포함한다. [FortSubscriberLoadTests.cpp](../../cpppj/tests/FortSubscriberLoadTests.cpp)는694행의현재ID/manager/두stream전필드/출력512BYTE전체/trace/정상·중단·물리실패를비교한다. 별도검사는이미등록된ID의미선택장치허용,부족한실제64BYTE출력의prefix보존,실제새ID장치누락직전의완성된파일이름/ID/pos를확인한다.

[격리 CMake](../../extracted/cpp-fort-subscriber-load-20261024-isolated/CMakeLists.txt)만으로 **Release 경고·오류0,2검사/check실패0**이며공용라이브러리도링크하지않았다. SHA/로그는 [검증기록](../../extracted/cpp-fort-subscriber-load-20261024-isolated/verification.json)에보존한다. 기존공유소스·원본게임·GUI·OS함수·전체SEH분배기는실행/수정하지않았다. 전체Editor파일/외곽완료는별도root통합검사대상이다.
