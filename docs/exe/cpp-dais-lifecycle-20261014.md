# 다이스·알터 최초Pop와 삭제 전후 복원 준비 — 2026-10-14

마지막 디컴파일 PC/날짜는 **HJOW-Athlon, 2026-10-14**다. `daislifecycle`의 패치10.78/CD10.72 각5개 실제몸체를 Refined readOnly로 내보냈고 종료0이다. 단위11의 좁은Dais/Altar관찰과 구분해 **후속단위12**에등록한다.

| 실제몸체 | 패치10.78 | CD10.72 |
| --- | --- | --- |
| 알터 preDestroy |0044a4d0|004836b0|
| 알터 postPop |00448ad0|004835c0|
| 알터 postDestroy 직접위임 |00448b60|00483660|
| 다이스 postPop |00449320|00481360|
| 다이스 postDestroy |00449580|00481620|

## 현재 새몸체

`RawDaisLifecycle`는 동일한Pool/Hash/기존발자국/Temple count·Stage/Altar typed조회/Carrier DamageablePostPop/DamageablePreDestroy를 공유한다. 최초다이스Pop은 저장한플레이어개수로첫프레임을보정하고현재개수BYTE를갱신한다. flags1/2의서로다른Player SID쌍을등록한다. 로딩중boss이면 현재단계를초기화하는가상Unpop→plain frame쓰기→Repop 뒤현재알터타입을생성해owner와전체Pop를요청한다. 현재로컬/비로딩/홈섬불일치는설정과억제전역을다시읽어경고한다.

알터 최초Pop은 **로컬21 Regular**를현재저항주기3초전역으로예약한뒤실제Damageable후처리에위임한다. SharedRegular로대체하지않는다. 알터preDestroy는flags400000이없고같은현재다이스가dead가아니면동일flags의가상삭제를먼저요청한다. 현재발자국의abstractRune를서버또는client번호권한에따라flags0으로지우고실제DamageablePreDestroy를호출한다.

다이스postDestroy는현재owner의등록된SID두쌍중현재다이스와일치하는쌍만0으로지운다. 다른SID와그mirror는유지한다. 마지막은 **전체DamageablePostDestroy**이며알터postDestroy도그전체몸체로직접위임한다.

## 독립 관찰과 새검사 초안

`daislifecycle-x86.tsv`는 **1928정상/assert0**이다. 판본당964입력을각각두x87정밀도에서실행해1928정상반환/판본을확인했다. 전체새5/5본문의분기/Player BYTE·SID쌍/플레이어일치·비일치/로딩·권한·억제/알터예약/추상Rune조건을원본PE에서읽었다. 초기smoke20과같은입력재관찰/기존Dais668·Altar2866/helper는1928에합산하지않는다.

최초도구 `decomp_daislifecycle_oracle.py`와1928보고서는 당시production/전수test초안/전체import/export/PE/fixture의전체SHA,중복없는입력키,정상x87/SEH/스택복구를기록했다. `DaisLifecycleTests.cpp`는현재1928입력의동일풀·실제typed/finder·Player상태를대조한다. 마지막전체Damageable 진입에서원본의명시효과경계에맞춰관찰하고전용예외로중단한다. 단위12에1검사로등록해첫공유빌드를진행중이며 **그예외검사를전체파괴완료로표시하지않는다.**

Frame/가상공간/Factory/음향/UI/표면조회와마지막전체Damageable는명시효과경계다. Native에서CD의표면조회는직접WORD지도를읽고패치는표면helper를요청하므로C++관찰도판본차이를구분한다. 현재전체게임/GUI실행은0이다.

첫 C++ 검사에서1928중효과비교12개가실패했고raw/player검사는전수일치했다. CD의`0048153f`/`0048154e`는각float좌표에`00504030`의0.9999를더한뒤__ftol한다. Ghidra C에서이fadd가생략돼이전oracle이표면SID9를(40,41)에썼지만원본은(41,42)를읽었다. C++생산/검사는수정하지않고새 `decomp_daislifecycle_current_oracle.py`의입력셀만교정했다.

새 `daislifecycle-current-x86.tsv`/`daislifecycle-current.json`은같은1928을두x87에서다시관찰했다. CD profile1~4×saved0/1/2의12행효과만변경되고나머지1916행전체·전수raw/player는그대로다. `analyze_daislifecycle_current_successor.py`/`daislifecycle-current-successor.json`은새tool의출력경로/입력셀만역변환하면전체옛tool과같음을증명한다. 최초1928/fixture/report/tool/현재생산4/Test/lineage/exports/실패로그의전체16파일은 `hist/daislifecycl-4443d`에보존했다. 기존보고서·fixture·Test·생산본문·11자료를덮어쓰지않으며새입력합산0이다. 단위12 3차 Release는 경고/오류0으로 완료됐고 현재1928 C++ 전수1검사는 실패0으로 통과했다. 로그는 `extracted/run12-rawDaisLifecycle_fixed.log`다.

## 후속 실제소비자

단위11 `5bdb5be` 뒤 SquidPostPop의동일Pool등록/Handles/Validate/Activate와RawDaisLifecycle의새prefix Validate를추가했다. `analyze_dais_postpop_lineage.py`와 `dais-postpop-lineage.json`은 기준11 네파일의Git LF/작업CRLF 전체원문을보존하고새등록을역제거하면전체기존본문/토큰이동일함을확인한다. 기존11JSON과수명1928당시보고서는덮어쓰지않고 `dais-lifecyc-dd023`의당시소스/검사/도구/fixture/spec로새현재SHA를연결한다. 추가원본입력은0이다.

실제다이스최초Pop→AltarFactory→owner→전체Pop→로컬21과사제해방의전체Pop를같은Pool·Host/Kernel에연결하는검사는별도후속이다. 포획Attach의Dais가상+24도 **00449660/CD004816c0**의필수몸체다. reason1은typedAltar→가상Unpop0→현재타입O첫프레임plain쓰기→포획원래ownerWORD&127→가상Repop0 및로컬cue정지를수행한다. reason2만자연반환하며처음부터빈부모알림으로처리하지않는다. 새RawDaisContentNotice와1/1읽기전용spec를후속준비했다.

마지막DamageablePostDestroy **0044b9e0/CD00461b40**과그첫드롭helper **0044b830/CD00461970**은asset담당이새 `RawDamageablePostDestroy`로별도복원중이다. 첫드롭을기존RawDamageableRelease로대체하지않는다. 섬정리 **0046e6c0/CD0048f960**만기존RawConstructionEffects.DropIsland를재사용한다. 현재전수body가새API `PostDestroy(Sid,std::uint32_t) const`로준비되면같은풀의실제파생삭제에연결한다.

현재이자료는알터→다이스전체삭제완료/캠페인1-1의식완주/승패·UI동일성을완료로표시하지않는다. 이전완료소스/관찰/보호원본을유지한다.
