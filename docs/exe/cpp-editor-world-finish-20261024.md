# Editor 마지막 noIsland 정리와 Graph 재구성

현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. AGENTS·LEFT_JOBS·닷넷 인수인계를 다시 읽었다. 단위381 `fa7762f9` 이후의 독립 준비이며 기존 Client/Session/CMake/LEFT/공유 build/lib/git/원본·CD/디컴파일 소스는 변경하지 않았다. GUI·게임·OS 진입도 없다. 신규 모듈/검사/원문 관측과 문서만 작성했다.

**최종 범위:** 보호10.78의 `0046e980` 부모 전체와 실제 iterator/좌표 정수화/ftol을 제품 작성 전에 독립 실행했다. v2 **309입력×2=618관측·정상572·명시 부분중단46**, 신규 C++ 격리 **3검사/실패0·빌드/실행 종료0·경고오류0**이다. Finder/가상+48/Release/Graph 하위 몸체는 명시 효과 경계다. 실제 Session 연결·전체 Editor/GUI/사용자 편집 성공은 아직 검증하지 않았다.

## 먼저 바로잡은 실제 대상

이 함수의 `005412cc`와 `005411d0`는 **noIsland157/island94**다. 신전/사제 비교가 아니다. `RawBridgeConnect`/실제 TYPE 전역과 보호 PE 값이 일치한다. EditorPlayers `004904b0` 끝의 `004905bc JMP 0046e980`에 해당하며 [앞선 EditorPlayers](cpp-editor-players-20261024.md)는 이 tail을 필수 외부 경계로만 남겼다.

정밀 C는 `extracted/refined/originals/Netstorm.c:90773`이다. 실제 부모 범위는 `0046e980..0046ea76`와 `0046ea80..0046ebc1` 합계569BYTE이며 사이9BYTE의 건너뛴 정렬 명령은 별도다. 원문 실행에는 iterator `0040eb40..0040eb71` 50BYTE, 좌표 정수화 `0041d770..0041d79a` 43BYTE, 실제 `__ftol004e49c0..004e4a34` 117BYTE도 포함한다.

## 원문 순서와 캐시

1. iterator를 `{index=1,cachedEnd=현재capacity}`로 만든 뒤 실제0040eb40을 호출한다. 이 helper가 먼저 증가하므로 **SID2부터**다. free bit0만 cachedEnd까지 건너뛰며 dead/void 등의 새 필터를 추가하지 않는다.
2. 각 후보의 유효 관문은 현재 Pool 기준과 **현재 capacity**를 읽는다. 후보 form의 물리 주소를 고정하고 **다음 iterator를 먼저 진행한 뒤** 현재 form의 type BYTE와 noIsland 전체DWORD를 비교한다. 다음 free-skip은 최초 cachedEnd를 계속 사용한다. capacity가 늘면 cachedEnd 밖 free도 현재 유효 관문 안에 들어와 검사될 수 있다.
3. noIsland의 현재 float32 X/Y를 실제0041d770의 `ftol→signed32→float32`로 정수화하고 다시ftol하여 일반 Finder `Begin(x,y,x,y)`를 만든다. 기본 필터/flags0이며 후보마다 현재 Pool에서 type BYTE를 읽어 현재 island 전체DWORD와 비교한다. **island를 찾은 뒤에도 마지막 후보까지 Next**한다.
4. Finder가 끝난 뒤 캡처 form의 **현재 X/Y를 다시 읽어** island가 있고 `0<X<board` 및 `0<Y<board`이면 보존한다. board는 lower-X가 통과한 뒤 현재 signedDWORD를 float32로 한 번 읽어 두 축에 사용한다. unordered NaN도 삭제 관문으로 가며 `<=`/`>=`의 일반 비교로 누락시키지 않는다.
5. 삭제할 때 현재 캡처 form의 X/Y를 다시ftol하고 `cell=(y<<8)+x`, `wordOffset=cell<<1`을 DWORD 산술로 고정한다. **현재 Pool 기준**으로 signed SID를 계산하여 **현재 surface WORD표에 임시 SID**를 쓴다.
6. 캡처 form의 **가상+48(0)**→**현재 spot표의 고정 cell에0**→가상 효과 뒤 **현재 Pool 기준으로 SID를 다시 계산하여004abf60**→Release 뒤 **현재 surface표의 고정 wordOffset에0** 순서다. 가상/Release가 바꾼 표를 예전 span으로 대신하지 않는다.
7. 순회 종료 뒤 **00463110(0)**을 정확히 한 번 호출한다. 빈 Pool에서도 이 Graph 경계는 필수다.

## 신규 API와 실제 연결 계약

`o/RawEditorWorldFinish.h/.cpp`의 `FinishEditorWorld(const EditorWorldFinishHooks&)`가 위 반복/조건/직접쓰기를 소유한다. hooks는 다음 실제 경계만 받는다.

- `pool()`은 현재 `{raw span,address,capacity}`를 읽는다. `address`는 동일 Pool 기준의32비트 주소 표현이며 실제 raw 배열의 복제본이 아니다. captured form은 기준 주소가 바뀌어도 원래 span/주소를 유지한다. 실제 연결은 주소 표현과 span의 동일성을 보장하고, 재배치가 가능하면 이전 raw 빌림을 해당 반복 끝까지 살려야 한다. 임시 span/지역 복사 Pool을 공급하지 않는다.
- `noIslandType/islandType/boardSize`는 실제 현재 전역을 읽는 효과 없는 getter다. type은 DWORD와 BYTE의 원본 비교를 유지하고 bool/추정 타입으로 축약하지 않는다.
- `begin(form,x,y,x,y)`는 실제 `RawSquidFinder::Begin({x,y,x,y},0)`와 같은 Finder의 `current/next` 수명을 반환한다. 첫 `form`은 어댑터의 캡처 문맥이며 원본004b16d0의 추가 인자라는 뜻이 아니다. Finder 결과를 따로 만들어 성공으로 대신하지 않는다.
- `surfaces()/spots()`는 실제 현재 WORD/BYTE표의 **mutable BYTE뷰**다. 본체가 실제 지정 위치에 직접 쓴다. span getter 자체는 효과 없이 현재 저장소를 읽는다.
- `unpop(form,0)`은 실제 캡처 객체의 가상+48이다. 이 hook에서 Release를 중복 호출하지 않는다. `release(int32 sid)`는 원문004abf60이며 bool/uint16 축약 전에 원문 signed 결과를 받는다. `graph(0)`은 실제00463110이다. 미연결을 빈 callback으로 채우지 않는다.

원본의32비트 주소 산술 감김/ftol 하위DWORD를 보존하지만, span 밖 손상 물리 접근은 C++에서 명시 예외로 거부한다. 선행 surface/spot 쓰기와 하위 효과를 되돌리지 않는다. 실제 SID Pool·공간 해시·삭제 장부·Graph가 연결되지 않은 현재 준비만으로 전체 Editor 완료를 주장하지 않는다.

## 독립 원문과 최초 입력 교정 이력

`tools/decomp_editor_world_finish.py`는 보호 PE만 읽어 Unicorn에 전개한다. 제품 소스/바이너리를 읽지 않는다. 허용 코드779BYTE만 실제 실행하고, Finder Begin/Next·가상+48·Release004abf60·Graph00463110은 명시 하위 입력/효과/중단 경계다. 직접 CPU 쓰기는 지역 스택·두surface·두spot표로 제한한다. 정상 RET의 ESP/callee-save/x87 제어어·TOP와 실행 코드 불변, 두 정밀도 결과 동일을 확인한다. 후보 검사/범위/좌표 연산은 원문 CPU가 수행한다.

최초 `editor-world-finish-x86.tsv`/`editor-world-finish-native.json`은618관측을 먼저 고정했다. 그 뒤 일부 unpop/release mutation 입력이 island+유효좌표 때문에 해당 삭제 경계에 도달하지 않는 coverage 부족을 발견했다. 원래 자료/원래 도구 전체는 `extracted/editor-world-finish-20261024-first-capture`에도 보존했다. 이를 제품 기대에 맞춰 덮어쓰지 않았다. 삭제 mutation은 Finder 빈 결과로, 좌표 mutation은 실제 lower-X 실패로 교정하여 **별도 이름 v2 fixture/report**를 제품 작성 전에 생성했다. 최초 Unicorn API가 bytearray를 받지 않는 준비 오류는 bytes 전달로 고친 뒤 첫 fixture를 생성했으며 원문 계산을 바꾸지 않았다.

현재 제품 검사 대상은 `tests/fixtures/editor-world-finish-v2-x86.tsv`와 `editor-world-finish-v2-native.json`이다. `--verify`는 최초 근거 SHA를 확인하며 `--dry-run`은618관측을 다시 실행해 기존 TSV와 전부 대조하고 파일을 쓰지 않는다. 둘 다 종료0이다. 명령 진입 hook은 **386028회**이고 그중 명시 하위 진입2552회가 포함된다. 원문 허용 범위의 명령 진입은383476회다. 이 수치를 명시 하위 몸체 실행 수로 합산하지 않는다.

v2는 비0 전체 raw/표, SID1 제외·free skip·다중 noIsland, BYTE와413DWORD 타입 비교, 빈/다중 후보와 island 발견 뒤 Next, 0/소수/경계/음수/NaN/Inf, Finder 뒤 좌표·현재 타입/board/Pool 변경, 가상 뒤 Pool/map/spot·capacity 변경, Release 뒤 map/Pool 변경, 물리범위 및 각 하위 중단을 포함한다. 정상572와 별개인46중단은 surface 물리범위36 및 Begin/Next/Unpop/Release/Graph 각각2다. 전체 두Pool raw와 네표의Adler32, 모든 하위/직접쓰기trace를 보존한다.

## C++ 검사와 한계

`tests/EditorWorldFinishTests.cpp`의 독립 원문618 대조는 입력열만 신규 제품에 공급한다. 직접 표 쓰기는 원문이 관측한 주소에서 대여/반납 사이 **실제 바이트를 읽고 다른 전체 바이트 보존**을 확인한다. 관측 주소를 제품 메모리에 쓰거나 기대 상태를 주입하지 않는다. 빈 Pool/필수Graph 검사에 더해, 세번째 검사는 **fixture 주소도 사용하지 않고** 실제 unpop/release/graph callback에서 임시surfaceSID→spot0→변경된Pool의releaseSID→Release 뒤 현재surface0 순서를 직접 확인한다.

자체 경로 `extracted/cpp-editor-world-finish-20261024-isolated`에서 MSVC C++20/UTF8/W4/WX/O2로 신규 두 제품 파일과 검사만 링크했다. 최종 **3검사/실패0**, 빌드/실행 종료0·경고오류0이며 공유 object/lib를 사용하지 않았다. PE 독립 읽기 검토도 캐시/순서/열린 좌표 관문에 명백한 오류를 찾지 못했다. 공용 CMake 등록/전체 CTest/실제 Session 연결·GUI는 root 후속이다. 실제 Finder/Unpop/Release/Graph 하위 전체가 새 모듈의 fixture에서 실행됐다는 주장은 하지 않는다.

## 10.72 정적 기록

자동 함수 대응표는 이 함수의 CD 대응을 표시하지 않지만, CD EditorPlayers `004077c0`의 **실제 PE00407937 CALL0048fcc0**와 `NETSTORM.c:125289`에서 동일 역할의 외곽을 확인했다. CD는 stride36, noIsland0051cbb8/island0051cabc, Finder004eae20/004eafe0, 좌표00440a50/__ftol004f161c, surface0052d590/spot0052fe48, Release004ab250, 마지막Graph0045bd60(0)이다. CD 하위/좌표 분기 전체는 이번에 실행하거나 제품으로 복원하지 않았으므로10.78 결과를 CD 통과로 세지 않는다.

주요 SHA-256은 보호10.78 PE `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`, 정밀 C `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`, v2 fixture `b9a5d0789941aa18c4903113ec335858fa02574e5b115acd44d7dd872f1c34f5`다. CD PE는 `613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233`이고 읽기만 했다. source/tool/실행파일/최종 로그는 같은 격리 경로의 최종manifest에 고정한다.
