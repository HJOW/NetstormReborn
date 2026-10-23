# 단위384 Subscriber Session·Client 접합 제안

2026-10-24,현재/마지막 전체 정밀 디컴파일 **HJOW-Athlon / 마지막 수행2026-10-23**이다. root의단위383커밋뒤검토용 [제안 patch](../../extracted/cpp-editor-subscriber-session-20261024-proposed.patch)이며공유기존소스·CMake·LEFT·공유build·git·GUI를수정/실행하지않았다. 공용lib링크/검사실행도없다.

patch SHA256은 **`99ee09dc8b8427b7a51cca6b1404128b8213cf1af596ad2cb80dddd328e3a323`**이다. [manifest](../../extracted/cpp-editor-subscriber-session-20261024-work/manifest.json)에네기존파일의정확한기준/제안SHA와신규검사SHA를둔다. 기존공유문맥에서 `git apply --check --whitespace=error-all` 종료0을확인했다. 전체파일사본을공유제품에덮지않고필요hunk만후속병합한다.

## 독립 단계와 같은 Storage

제안 `RawCampaignSession::LoadExistingEditorSubscriber()`는명시ExistingEditor/World시작전/미완료상태에서만실행한다. 호출시점의 **현재playerState.templateOwner를한번읽어고정this slot**으로넘긴다. 전체File부모·World·Config이름/창제목·최종loaded는설정하지않는다. 이미시작한World뒤의늦은Subscriber읽기는거부하며반복파일로드전단계자체에임의일회성성공플래그를추가하지않는다.

Impl의 `ReadExistingEditorSubscriber(slot)`는실제headers[slot×20]의현재manager를원문위치에서조회한다. `SelectSection(slot,"Subscriber",0)`의반환 `TemplateWriteStream` 생성수명을지역registry에보존하고그참조의 `AccessSection`으로원시ID/문자열본문전체를동기빌린다. 타입사본·현재선택캐시·parsedSubscriber·목록용ReadSubscriberName을사용하지않는다.

같은 `globals.mySubscriber`와실제 `handle`에 [RawFortSubscriberLoad](cpp-fort-subscriber-load-20261024.md)를연결한다. 원시ID/부분이름/cursor는즉시쓰며등록장치실패에도복원하지않는다. 등록마감은Storage빌림반납뒤이므로Configcallback이manager/현재선택을바꾸어도끝난stream의뷰를붙잡지않는다.

## Client 경계와 실제 ReadInt 재사용

신규 `CampaignSubscriberServices`는readRegistered/configure/clock/assertion만공급한다. 새 `MakeCampaignSubscriberClientServices(Client&)` helper는화면/음향초기화없이같은실제Client를빌리며기존MakeCampaignClientServices가그결과를주입한다. 소진로그9는새로복제하지않고기존 `services.player.log`의같은실제로그경계에넘긴다.

등록읽기는 **기존Configuration().ReadInt를그대로호출한다.** 현재Subscriber의uint32참조를대응signed int32참조로접근하며,해당C++alias가허용되는정확한대응형인지make_signed/make_unsigned양방향·4BYTE·동일alignof를static_assert한다. 별도byte배열/포인터정렬추측/임시값동기화가없다. 실제ReadInt가Config.Get복귀뒤그같은ID객체를비교하므로Get동안의현재ID변경도보존한다. Get/Parse나ReadInt계약을중복구현하지않는다.

configure는실제 `ApplyScriptConfig(expression,client.Configuration())`→기존토큰/Config.Set/변경callback을사용한다. 원문등록쓰기오탈자registerdSubId와unsigned십진문장을유지한다. clock은기존로그인과같이실제 `std::clock()`의DWORD이며GameClock/벽시각으로바꾸지않는다. 원문진단은같은실제로그스트림에위치값을남긴다.

## 같은 Client 난수 소유자 의존

이patch는fury의별도ClientRandomGlobals제안에의존한다. 합의API는 `client.RandomGlobals()`의shared_ptr,`services.randomGlobals`,소유자의 `Crt()` 실제CrtRandom lvalue참조와 `SubscriberIdInitialized()` 실제DWORD참조다. 해당신규헤더·Client소유·서비스필드·MakeCampaignClientServices의randomOwner주입·Session.cppinclude를이patch에중복추가하지않았다.

파일ID가정상이면등록설정/clock/로그/새난수소유자를검사하지않는다. 파일ID−1에서도ReadInt가값을바꾸면생성기를선택하지않는다. 새ID가실제로필요한generate callback안에서만현재shared_ptr를지역으로유지하고 `BindSubscriberIdCrtRandom(owner.Crt(),devices)`→`GenerateSubscriberId({owner.SubscriberIdInitialized()},bound)`를실행한다. 이미있는services.unixSeconds를time32장치로재사용하며새clock·실패로그도도달할때만요구한다. 생성기의플래그/CRT를초기화하거나독립GameRandom/CRT상태를만들지않는다.

현재unixSeconds의지원날짜계약은기존Client서비스그대로이며이제안에서바꾸지않는다. 생성기자체의signed time32/clock상위비트·seed뒤현재플래그재조회/감김·소진반환은 [독립원문모듈](cpp-subscriber-id-generate-20261024.md)의검증범위다. Client/CRT전체소유권이실제로같아지는지는fury의별도patch와함께root가검증해야한다.

## 새 검사 초안과 남은 검증

patch안의 [신규4검사초안](../../extracted/cpp-editor-subscriber-session-20261024-work/source/cpppj/tests/CampaignExistingEditorSubscriberTests.cpp)은실제Client(Run없음)/Session/Storage와소유자를사용한다. 정상파일ID에서미선택장치누락허용,실제Config.ReadInt가−1을77로교체하여난수를선택하지않음,독립native입력init1/CRT1의ID87625992/최종CRT75413397과실제Config오탈자쓰기,등록장치누락시ID/부분이름/pos보존을확인하도록작성했다.

이네검사는 **아직컴파일·링크·실행하지않았다.** 공용CMake에도등록하지않았다. 기존독립Subscriber694행/생성기1024행의통과를새Session접합통과로대신하지않는다. 두제안병합후root가실제ABI/Client callback/Session수명/신규4검사와기존회귀를검증해야한다.

전체파일부모의정상State|1/Notify→Subscriber→fort.mySubHandle/제목→World순서,전체설치외곽/최종HUD·State·loaded·메뉴와실제Editor저장/재로드는계속미완료다. 이단계API를추가한것만으로최초Editor전체성공을표시하지않는다.
