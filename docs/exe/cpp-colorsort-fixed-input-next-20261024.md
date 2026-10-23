# 고정 ColorSort 입력과 현재 팔레트 소비자의 접합 제안

단위386 준비·**미적용 사본**이다. 현재/정밀 디컴파일 호스트는 HJOW-Athlon /2026-10-23이다. 공유 제품·CMake·LEFT·git·공용build/lib·GUI와 기존 Kencloud/Random 제안·새385 [생성 부모7파일](cpp-colorsort-startup-native-20261024.md)은 변경하지 않았다. [ClientColorSortInput](../../cpppj/src/client/ClientColorSortInput.h)은 실제 초기 논리RGB 입력과43c010의9표를 소유하는 의미 계층이다. **raw0x3800/0x5008전체객체의대체구현은 아니다.** 그 실제할당/439de0전체필드/43a8e0/qsort/기본색띠의필수합성은 RawColorSortStartup에 남는다.

## 제품 전에 고정한 수명 관측

[새 원문 도구](../../tools/decomp_colorsort_fixed_input.py)는 기존 `BackgroundOracle`의 실제439de0와전체43c010을 재사용한다. 초기 입력은 기존 실제GIFCLOUD,이후화면입력은같은기존도구의혼합RGB와중복회색이다. **같은00555d88주소/0x3800BYTE객체를 유지한 채 현재화면 논리RGB만 두번 바꾸고43c010을 다시 호출**했다. 현재빨강DWORD는189→13→255의명시하위입력이다.9표의기대값은제품을읽거나계산하지않고PE메모리에서만읽었다.

[최초fixture](../../cpppj/tests/fixtures/colorsort-fixed-input-x86.tsv)/[보고서](../../cpppj/colorsort-fixed-input-native.json)는 제품 작성 전에 고정한6관측(두control×세단계)이다. 준비단계의각439de0/첫maps를포함해2constructor와8maps를호출했고,기록하는6단계maps의명령수는50,045,682다. 각control내고정객체전체와현재화면입력은모든단계에서불변이었다.53/64비트의전체HLS객체hash는서로다르며,**9번째명도표만세단계모두다르고다른8표는같다.** 실제기본53비트결과와64비트별도결과를함께보존한다.

처음 도구는 서로다른control의전체객체까지같아야한다고과도하게가정해6단계실행뒤fixture쓰기전에중단했다. [당시도구사본](../../extracted/colorsort-20261024-isolated/decomp_colorsort_fixed_input-before_precision.py)과 [실패기록](../../extracted/colorsort-20261024-isolated/first-fixed-input-attempt.json)을보존했다. 같은control내불변검사는유지하고control간HLShash/9표차이를각각기록하도록고친뒤최초자료를고정했다. 이미존재하는원문fixture나제품기대값을변경하지않았다.

## 재사용한 계산과 실제 소유권

`ClientColorSortInput::Capture`는 명시생성위치에서 실제논리RGBX256DWORD를 자기고정배열에 복사한다. 복사/이동금지와고정std::array로입력주소를유지한다. 일반palette전환/화면RGB변경/해상도재생성은Capture를호출하지않는다. 명시새생성이요청된경우에만다시Capture할수있다. Captured/built는host입력준비계약이며원문paletteLoaded/primary DWORD를대신하지않는다.

`Rebuild(currentRed)`는 같은초기입력과현재명명빨강을사용한다. SpriteColorMaps/PreviewColorMaps의기존계산을그대로재사용하며,그결과로Production의동일disabled/underlay와PaletteShade의bright/dark도얻는다. 기존에노출되지않은1.6RGB표와명도표만추가계산하여전체원문9표와대조했다. 기본53비트계약이며64비트명도별도출력을기본기대에섞지않는다.

| 실제주소 | 결과 |
|---|---|
|00555e38|정규RGB1.600000023841858배의L1최근접.입력을1로clamp하지않고끝에서228..245/254/255를항등으로덮음|
|00555f38|현재빨강의lowBYTE채움:기존Preview.destroy|
|00556038|빨강2.1배/1제한의제곱거리:기존Preview.illegal/Production.disabled|
|00556138|항등256BYTE|
|00556238|기존Sprite.bright/PaletteShade.bright|
|00556338|0채움:기존Preview.labelShadow/Production.underlay|
|00556438|기존Sprite.dead/PaletteShade.dark|
|00557738|기존Sprite.shadow|
|00557838|HLS L에서 `(L+0.20000000298023224)/3`의최근접L,끝254/255항등.53/64비트별도차이|

이작은소유자는**유효한256색초기입력과자기256BYTE표**를요구한다. 미캡처/미생성조회는명시오류다. 원문NULLprimary/손상된raw객체로43c010이일부zero/fill을쓴뒤fault하는동작까지복원했다고표시하지않는다. 그검증과actualraw객체주소/전체필드는별도필수경계다. 같은RGB입력의성공9표동일성은부모부분효과116관측과구별한다.

## 미적용 Client 사본

[15파일patch](../../extracted/cpp-colorsort-input-20261024-proposed.patch), SHA **68ccbf770a9fab9e364804c82a1c57276db03b539a67d59e9f06d23787262c7c**은당시공유파일원시기준/새사본을함께보존한다. 신규Input의2제품파일과검사/fixture는별도추가등록이필요하다. 공용CMake는수정하지않았다.

1. Client는 다른소유자/Screen/메뉴/월드보다오래사는shared Input을한번구성한다. 시작첫GamePalSpec→LoadPalette가끝난뒤,current코드의fullTime다음/DIB이전자리에서 `Screen::LogicalPalette()`의실제005c7954역할뷰를Capture한다. 현재생략된원문initPlayers/전체43b430을성공시킨것은아니다. 원문00439537의실제하위생성전체를연결할때Capture위치를그439de0진입으로귀속해야한다.
2. Screen은현재논리팔레트getter와고정입력조회기를명시적으로받는다. 두Screen생성경로가같은owner를빌리고해상도재생성은다시Capture하지않는다. 기존기본/명명색검색은현재logical을사용하며기존ShadeMaps계산의입력만초기ColorSort로맞춘다. Screen의기존조기2표계산이전체43c010부모의실행을대신한다는뜻은아니다.
3. `Client::RebuildColorSortMaps()`는 현재Screen의명명빨강과고정입력으로9표만작성한다. `RefreshPaletteConsumers()`는별도다. 후속004b6100의buildMaps경계는실제Rebuild→현재소비자갱신으로합성해야하며,이patch가아직없는전체Editorpalettehook을빈성공으로만들지않는다.
4. UberGump→World→Factory/Cursor 및생산패널의SetPalette는 **현재표시RGB/명명색**과 **같은초기ColorSort입력span**을별도전달한다. currentRGB를저장하는월드/커서흰색·빨강/생산hint/소리tint는현재값을유지하고Sprite/Preview/생산/전용OwnerColorMaps의검색입력만초기값이다. World의지형전용명도캐시도그같은초기입력을쓴다.
5. Source없는독립표시검사만기존현재논리입력을명시fallback으로사용한다. 실제Client는항상고정owner의비어있지않은256색span을준다. 255색등잘못된입력은현재RGB로조용히대체하지않는다. SceneMusic의기존LoadScenePalette(clear0)/Refresh호출순서는그대로다. 그현재어댑터전체를원문파일cache/모든부모실행의동일성으로확대하지않는다.

Input은원문ColorSort의필요RGB의미를고정하는한소유자다. 43b430의primary/search/gradient 실제슬롯을복제해만들거나43a8e0/qsort를빈성공으로연결하지않았다. 원문초기004396e9→0043c4e0의 `!color.dat`/소유자표/조건부43c010생성부모도별도필수경계다. 기존ParticleGradient는같은초기RGB계산에재사용할수있지만두raw객체의전체물리소유자는아니다. 이사본을전체ColorSort startup완료로표시하지않는다.

## 검사와 이후 확인

[새검사](../../cpppj/tests/ColorSortFixedInputTests.cpp)는6행모두를읽고53비트9표전체,64비트앞8표일치/마지막표별도차이를확인한다. 세호출동안초기입력의같은주소·원시값과원문객체hash불변,두control간HLS차이를확인한다. 미캡처/미생성오류,명시Capture만의교체,같은입력span/독립fallback/잘못된물리폭거부도포함한다.

신규Input/Sprite/Preview/PaletteShade/검사/Runner만직접연결한첫격리build1과2검사는종료0/관측경고0/실패0이다. 이후사본8cpp문법검사도종료0이며기존ClientMain의getenv/fopen C4996네개만관측했고새경고는없다. 같은최종Input/selector의독립2검사도실패0이다. [격리제안디렉토리](../../extracted/cpp-colorsort-input-20261024-proposed)의로그와[verification.json](../../extracted/cpp-colorsort-input-20261024-proposed/verification.json)에그범위를기록한다. 공유lib/실제Client·Screen·Session 객체/게임/GUI는실행하지않았다. 실제팔레트두번변경·해상도재생성·미션교체의소비자픽셀/초기입력주소·전체초기화부모검증은root의후속단계다.
