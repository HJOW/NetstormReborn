# ColorSort 생성 부모의 독립 복원

단위385 독립 준비·미등록이다. AGENTS와 최신 공통/닷넷 인계를 확인했으며 현재/마지막 정밀 디컴파일 호스트는 HJOW-Athlon /2026-10-23이다. 신규 [RawColorSortStartup.h](../../cpppj/src/o/RawColorSortStartup.h)/[cpp](../../cpppj/src/o/RawColorSortStartup.cpp)는 원문10.78의 **0043b430 265BYTE와0043b300 304BYTE** 두 생성 부모만 복원한다. 전자는 CD10.72의00459c50,후자는00459b50에 대응한다. 원본/CD/정밀산출물·기존제품·CMake·LEFT·git·공용build/lib·GUI를 변경하거나 실행하지 않았다.

## 제품보다 먼저 고정한 자료

[원문 도구](../../tools/decomp_colorsort_startup.py)는 실제 PE의 두 부모만 Unicorn x86으로 실행해 [fixture](../../cpppj/tests/fixtures/colorsort-startup-x86.tsv)/[native report](../../cpppj/colorsort-startup-native.json)를 제품 작성 전에 고정했다. **58독립입력×2정밀도=116관측**, **12,246명령**, 양정밀도 부모 결과 차이0이다. 정상100과 assert/free1/alloc1/sort/search/qsort/alloc3/gradient의 명시 중단 각2,총16을 구별한다.

할당·free·진단·439de0·43a8e0·qsort·43ad20는 **명시 하위 장치**다. fixture의 SORT/BASE/GRAD marker와 입력 앞8BYTE는 부모 호출·직접vtable store·반환값/현재전역의 부분효과를 대조하기 위한 장치 입력이며 실제 하위 생성자 전체를 복원했다는 뜻이 아니다. SEH 현재연결 DWORD를 격리메모리에 제공했지만 OS/SEH 예외전파 전체를 재현한 것도 아니다. 고정 RGB/HLS의53/64비트 계산 차이는 별도 다음 단위 자료에 보존하며 이 부모정밀도0과 혼합하지 않는다.

## 정확한 두 부모 순서

`InitializeColorSort(logicalToken,globals,hooks)`는 실제 입력 포인터 값만 캡처한다. 입력 RGB의 내용은 뒤439de0 진입 시점에 읽는다. loaded0이면 Colorsort.cpp451의 paletteLoaded 진단을 호출한 뒤 계속한다. 현재 primary가 비영이면 free→**복귀뒤 같은현재슬롯0**, 그 뒤 현재 search에도 같은 과정을 수행한다. 하위가 슬롯을 바꾸어도 부모의0쓰기는 남고 다음슬롯은 현재값을 다시 읽는다.

new(0x3800)가 비영이면439de0(this,같은logicalToken)의 반환주소를 primary에 쓰며 NULL이면0이다. 그 다음new(0x5008)의 반환주소를 **캡처**하고 비영이면43a8e0→그같은주소첫DWORD에00506658→qsort(search+8,256,80,0043a1e0)다. 그 뒤 **search슬롯대입→count10쓰기** 순서가 실제PE0043b514/0043b51a에 고정되어 있다. callback이 search/count를 바꾸었어도 원문 위치의 대입이 실행된다. 마지막에는 같은globals로 기본색띠 부모를 호출한다.

`InitializeColorSortGradient`는 현재gradient 조건부free/복귀뒤0→new(0x5020)이며 비영일 때 **할당callback뒤현재count**를 캡처한다. from은 PE00506698의 정확한double0.21568627450980393 세성분,to=(1,0,0),width는00503678의2.4다. 실제43ad20(this,from,to,count,width)의 반환주소를gradient에 대입한다. NULL할당은생성자를생략하고0대입한다. 생성자진입에서현재primary를읽는책임은실제하위구현에남긴다.

모든 도달한 하위는 필수 std::function이며 빈성공을 넣지 않는다. 자연 생략되는 장치는 선행검사하지 않는다. 직접vtable store만 실제같은memory span에 little endian DWORD로 쓰며 물리부족이면 그위치에서 예외다. 하위 예외/중단은 삼키거나 되돌리지 않아 앞선free/0/대입/직접쓰기 효과가 남는다.

## 현재 실제 Client와의 경계

기존 PaletteShade/Preview/Sprite/Production/OwnerColorMaps와 ParticleGradient의 계산은 재사용한다. 다만 ParticleGradient는 색선택에 필요한 의미필드/CRT정렬을 복원한 것으로,**0x3800 ColorSort/0x5008 검색객체 전체 물리 소유자가 아니다**.43a8e0/qsort를 빈성공으로 만들거나256RGB사본 하나가 두raw객체 전체라고 표시하지 않는다. 같은 Client 수명의 실제 primary/search/gradient/count/loaded와 같은할당소유자·실제하위생성/정렬이 필수접합이다.

원문 시작00439537은 초기 fortPal→GamePalSpec→LoadPalette 뒤 실제논리RGB로 이부모를 한 번 호출한다. 이후004b6100은 같은 ColorSort로43c010을 호출하며43b430을 다시 호출하지 않는다. 따라서 현재ScreenRGB로매번초기입력을치환하면안된다. [ColorSort 입력수명근거](cpp-palette-colorsort-owner-20261024.md)와 별도 진행중인 고정RGB소유자/43c010전체9표제안은 이 부모동결과 구별한다. 이 두 모듈을 등록한 것만으로 실제 Client startup/전체Editor 설치완료라고 판정하지 않는다.

## 격리 검증과 인계

[ColorSortStartupTests](../../cpppj/tests/ColorSortStartupTests.cpp)는 최초116행의 정상/중단이유·5현재전역·3할당prefix/source8BYTE·전체trace를 대조한다. 진단뒤슬롯변경,free뒤무조건0,할당뒤입력변경,constructor반환0,search/qsort뒤현재값변경,색띠할당뒤count7,부분중단을 포함한다. 다른검사는3개NULL할당의자연생략과누락진단의이전상태보존을확인한다.

기존공유lib없이 신규2본문/검사/Runner만 VC14.51.36231 x64·SDK10.0.26100.0 `/std:c++20 /EHsc /utf-8 /W4`로 직접 빌드했다. **build1 종료0·관측경고오류0**, 같은결과의 **2검사0실패**다. 최초원문 SHA/UTF8/AST/JSON과 최종원시 SHA는 [startup-freeze.json](../../extracted/colorsort-20261024-isolated/startup-freeze.json)에 기록한다. 공용등록/실제하위합성과사용자검증은root의후속범위다.
