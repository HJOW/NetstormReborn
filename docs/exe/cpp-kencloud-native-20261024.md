# Kencloud 파일·생성·행 갱신의 독립 복원

단위384 후속 준비·미등록이다. 새 [RawKencloud.h](../../cpppj/src/o/RawKencloud.h)/[cpp](../../cpppj/src/o/RawKencloud.cpp)는 원본10.78의 `00470bb0`, `00471040`, `00470b20`을 복원한다. 이미 있는 `SpStore`의 실제 `GameRandom` 계산을 재사용하며 같은 객체 참조를 받는다. 공유 제품·CMake·LEFT·git·공용 build/lib·GUI는 수정하거나 실행하지 않았다. 현재 호스트와 마지막 정밀 디컴파일 호스트는 모두 **HJOW-Athlon**, 디컴파일 일자는 **2026-10-23**이다.

## 먼저 고정한 원문 관측

[v2 도구](../../tools/decomp_kencloud_v2.py)는 제품 본문 작성 전에 [v2 fixture](../../cpppj/tests/fixtures/kencloud-v2-x86.tsv)와 [native 보고서](../../cpppj/kencloud-v2-native.json)를 고정했다. 보호 PE/정밀 C/함수표/기존 GameRandom 소스를 읽으며 실제 게임·OS 파일·GIF·GUI를 실행하지 않았다. 원문3함수 및 실제 `004558c0` GameRandom40BYTE를 Unicorn x86에서 실행하고, 파일 장치·할당/free·CRT memmove만 명시 하위 경계로 제공했다. memmove는 원문이 넘긴 실제 겹치는 주소에서 읽고 쓴다.

123개 독립 입력을 x87 `0x27f`/`0x37f`에서 각각 실행한 **246관측**, 총 **4,847,462명령**, 정밀도 차이 **0**이다. 정상 반환208과 중단38을 구별한다. 중단은 divide-zero8, fill-physical2, step-tail8, step-read2, file-path-write2, path/exists/decode/free/alloc/random1/move1/move65 각2다. 물리 오류나 명시 중단을 정상 성공으로 세지 않는다. speed `0xffffffff`의 사례는 move65에서 명시 중단하며 그 거대 반복 전체가 완료했다고 주장하지 않는다.

**최초 v1은 삭제·수정하지 않는다.** [v1 도구](../../tools/decomp_kencloud.py)/[fixture](../../cpppj/tests/fixtures/kencloud-x86.tsv)/[보고서](../../cpppj/kencloud-native.json)는 먼저123입력/246관측을 기록했다. 이후 제품 작성 전 계측 검토에서 Unicorn의 `rep stosd` hook이 **ECX=0인 종료 진입에도 호출**되는 점을 찾았다. v1은 이때 one-past EDI를 실제 쓰기로 오판해 유효 fallback의 완전 fill 뒤를 `fill-physical`로 중단했다. v1 원문 관측을 제품에 맞춰 바꾸지 않고 별도 v2에서 `ECX!=0`일 때만 실제 store 물리를 검사했다. v2는 제품을 읽거나 기대값을 역산하지 않았다. v1은 계측 오류 이력이며 최종 C++ 대조 자료는 v2다.

## 선택한 실제 범위

| 원본10.78 | 선택 범위 | 의미 |
|---|---|---|
| `00470bb0..00470c24` |117BYTE| CloudFileSpec 파일 부모. CD10.72 대응 `004b0930` |
| `00471040..0047118f` |실행 본체329BYTE, 중간 정렬 간격 제외|512×512BYTE 캐시·seed·fallback warm-up |
| `00470b20..00470bac` |141BYTE| 현재 위상과 현재 주소를 사용하는 행 이동 |
| `004558c0..004558e7` |40BYTE, 기존 SpStore 재사용|00532710 GameRandom. CD 대응 `0048cbd0` |

`KencloudGlobals`는 현재 cache/end/nextRow/lastRow/phase/dirty/spacer DWORD와 설정·행표·seed 물리를 참조/`span`으로 빌린다. 구조체가 별도 캐시·난수·옵션 사본을 소유하지 않는다. memory hook은 **순수 주소 조회**이며 반환된 실제 할당 뷰는 해당 접근 중 안정되어야 한다. 포인터 토큰은 `uint32_t`이고 host 포인터를 잘라 저장하지 않는다. 원문에 없는 의미상 count 상한을 추가하지 않으며 caller가 실제 물리 크기를 공급한다. fixture의 seed 물리는1000DWORD이고 정상 원본 설정 count400 및 현재값 변경을 관측했다. 임의 손상 행표의 음수 주소가 전역 앞을 덮는 동작이나 전체 주소공간의 무한 alias를 검증했다고 확대하지 않는다.

## 파일 부모00470bb0

1. `PathSpec("CloudFileSpec",0,0,0)` 뒤 반환된 같은 NUL 물리를 캡처한다. caller가 실제264BYTE scratch의 초기 height/width/path를 모두 명시한다. 임의0 초기화로 원문 stack의 미작성 값을 가장하지 않는다.
2. 원문처럼 **source BYTE 읽기→target BYTE 쓰기**로 NUL까지 순방향 복사한다. 길이 선계산·문자열 별도 사본·256자 절단 성공을 추가하지 않는다. source/target alias도 같은 실제 뷰로 관측한다. 물리 경계를 넘으면 그때까지의 쓰기를 남기고 예외다.
3. exists는 같은 복사 버퍼를 받고 비영일 때만 decode한다. decode의 인자는 같은 path와 실제 `width`/`height` 참조다. callback이 이름·치수·현재 캐시를 바꾸면 그 효과가 남는다.
4. 반환 뒤 **현재 width부터**, 다음 현재 height를512와 비교한다. 모두512면 반환주소0도 그대로 반환한다. 치수가 다르면 **주소0도 release에 전달**한 뒤0을 반환한다. exists0이면 decode/release는 자연 생략한다.

## 생성00471040

처음 cache가 비영이면 하위를 호출하거나 난수를 쓰지 않고 partialDirty0→allDirty1로 복귀한다. cache0이면 파일 부모 반환주소를 같은 cache에 기록한다. 반환0일 때만 `allocate(0x40000)`를 호출한다. **할당 복귀→현재 lowColor 읽기→반환주소를 cache에 대입→전체 BYTE fill** 순서다. 할당 실패를 빈 성공으로 바꾸지 않으며 이미 발생한 cache 대입은 물리 예외 뒤에도 남는다.

현재 base로 nextRow=base+512를 쓰고 초기 signed seedCount를 캡처한 다음 end=base+262144, lastRow=end-512를 쓴다. 처음 count가 양수면 각 seed마다 같은 `GameRandom.Next(510)+1`, `Next(512)`를 순서대로 소비한다. 첫 반환값은 두 번째 호출 동안 캡처하고, 그 뒤 **현재 cache**로 주소를 계산해 현재 seed 슬롯에 쓴다. **저장 뒤 현재 count 재조회→index 증가→signed 비교**다. 현재 count가 첫 난수 callback에서0이 되어도 현재 seed의 두 난수와 쓰기는 먼저 끝낸다. 그 다음 같은 난수 `Next(256)+5`를 spacer에 쓴다. **유효 GIF도 seed/spacer 난수를 소비**한다.

fallback만 현재 signed fury로 `10000/fury`를 계산하여 양수 횟수의 warm-up을 한다. 각 반복은 cache0이면 같은 함수·같은 GameRandom으로 재귀 생성하고, **재귀 복귀 뒤 현재 speed를 한 번 캡처**한다. 현재 batch는 캡처 횟수만큼 StepKencloud를 실행한다. step callback이 speed를 바꿔도 현재 남은 횟수는 바뀌지 않는다. batch 뒤 현재 fury의 나눗셈을 다시 수행해 다음 반복을 판단한다. fury0은 앞선 효과를 남기고 domain_error이며 정상 dirty reset까지 도달하지 않는다. 모든 정상 반환에서만 partialDirty0→allDirty1을 쓴다.

## 행 갱신00470b20

현재 phase를 읽고 **현재 cache 첫 BYTE를 먼저 읽은 뒤** end를 캡처한다. phase=(old+1)&15 쓰기→캡처 end[-1]에 첫 BYTE 쓰기→현재 phaseRows[phase] 읽기다. 따라서 cache 물리 읽기 실패는 phase를 바꾸지 않고, end 물리 쓰기 실패는 이미 phase를 바꾼 상태다. 시작행 signed>=512이면 그 뒤 행 이동은 생략한다.

각 행은 현재 cache/end를 다시 읽어 `cache+row*0x3dd`를 계산하고 unsigned 주소 비교로 end-1까지 clamp한다. 같은 방식으로 길이를 정해 실제 겹침 memmove(dst,dst+1,length)를 실행한다. **이동 완료→선택 관측 callback→dirtyRows[row]=1** 순서다. 그 뒤 row+=16, offset+=0x3dd0이며 signed row<512를 검사한다. callback이 cache/end/난수를 바꾸거나 중단하는 효과와 다음 행의 재조회가 fixture에 포함된다. 함수 자체는 난수를 소비하지 않는다.

## 실제 난수·파일·Client 수명 접합

`BuildKencloud(globals,GameRandom&,hooks)`는 실제 난수 객체를 필수 인자로 받는다. 새 GameRandom·CRT 대체·다른 시드·사후 state 동기화가 없다. `afterRandom`은 같은 객체의 실제 Next **뒤** 선택 관측이며 실제 계산을 대신하는 hook이 아니다. `afterMove`도 실제 메모리 이동 뒤의 선택 관측이다. 도달한 파일/할당/물리 hook은 필수이고 미연결이면 예외를 그대로 전파한다. 자연 생략될 hook을 진입 전에 검사하지 않는다.

현재 [최소 Session getter 제안](cpp-kencloud-random-binding-20261024.md)과 [별도 patch](../../extracted/cpp-kencloud-game-random-20261024-proposed.patch)는 `Impl.random`을 직접 참조로 빌리는 미적용 사본이다. loaded=false인 실제 설치 Session에서 동기적으로 빌려야 한다. menu.world_나 public loaded 관문에 우회하지 않는다. Client의 월드 없는 구름 프레임/Session 교체 후00532710 전체 수명은 별도 필수 경계이며 장기 callback에 파괴된 Session 참조를 저장하면 안 된다.

기존 GifImage 디코더·실제 PathSpec/파일 존재 검사·같은 할당 소유자를 필수 hooks에 합성하면 파일 부모를 재사용할 수 있다. 이번 모듈은 OS/GIF 하위를 새로 쓴 것이 아니며 실제 Client 장기 cache 소유자·해제·draw·프레임 호출·전체 Kencloud를 연결한 것도 아니다. 기존 UberGump의 정적 GIF Tile 배경은 이 계산/난수/행 이동을 대신하지 않는다. 같은 palette 전환 부모의 buildCloud/stepCloud에 이 본문을 연결할 때에도 실제 전역 수명과 현재 Session 난수를 먼저 정해야 한다.

**실제 ColorSort의 고정 초기 논리RGB 입력 수명은 별개 필수 경계**다. [ColorSort 근거](cpp-palette-colorsort-owner-20261024.md)를 유지하며 이 Kencloud나 현재 화면RGB로 대체하지 않는다. Screen arg2=1 GDI 장치 patch의 검증도 별도 기록이다.

## 격리 검증과 인계

[KencloudTests](../../cpppj/tests/KencloudTests.cpp)는 v2의246행마다 정상/중단 이유, 현재12필드, 인접전역0x2018BYTE 전체, 세 할당 각262144BYTE 전체, 마지막 파일 scratch264BYTE 전체의 SHA256, 모든 하위 trace를 대조했다. C++ SHA256은 빈 입력과abc 표준 벡터도 확인하며 최초 Python hashlib 자료를 기대값으로 사용한다. callback에 의한 count/cache/end/color/fury/같은 난수 state 변경, 파일 이름 변화, 중단과 재귀 생성도 포함된다. 자연 생략·빈 필수 hook·파일 exists0의 미작성 scratch 보존을 별도 검사한다.

새 [격리 스크립트](../../extracted/kencloud-20261024-isolated/build.ps1)는 기존 공유 lib 없이 RawKencloud/읽기 전용 SpStore/새 검사/Runner만 직접 컴파일·링크한다. VC14.51.36231 x64와 SDK10.0.26100.0의 INCLUDE/LIB를 해당 자식 프로세스에만 지정했다. 첫 build1은 PowerShell native `/D` 문자열 따옴표 오류로 실패한 로그를 보존했다. literal `\"` 인자만 교정한 **build2 종료0, 관측 경고/오류0**이며 그 결과 실행한 **2검사/0실패**다. 게임·GUI·공용 제품을 실행하지 않았다. first 자료 재생성 없이 두 도구의 `--verify`, UTF8/AST/JSON/문서 링크/최종 SHA 검사는 [verification](../../extracted/kencloud-20261024-isolated/verification.json)에 기록한다.

root 공용 등록은 `RawKencloud.cpp`, `KencloudTests.cpp`, `NETSTORM_KENCLOUD_FIXTURE=".../tests/fixtures/kencloud-v2-x86.tsv"`가 필요하다. 실제 설치/GameRandom getter patch의 컴파일·접합 검사, Client/무월드 장기 소유권 및 사용자 화면 검증은 수행 전이다. 이번 독립 계산 일치를 전체 Editor loaded/플레이/저장 완료로 판정하지 않는다.

동료 읽기 검토에서도 PE00471040/00470b20 및 정밀C00470bb0과 대조해 명백한 순서·참조수명 오류를 찾지 못했다. 이는 새 원문 실행·fixture 재생성·전체 Client 수명 검증을 뜻하지 않는다.
