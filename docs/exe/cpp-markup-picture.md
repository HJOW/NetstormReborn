# cpppj Markup 그림 조각·SHP·StyleText 체인 연결

**같은 날 후속:** [회전 없는 VFX 배율 출력](cpp-vfx-shape-scale.md)을 구현해 아래 단계에서 전달 경계였던 Markup scaled 요청을 실제 현재 표면에 연결했다. 이 문서의 빌드/근거는 그림 조각 구현 당시 기록이다.

작업일 **2026-10-12**, PC **HJOW-Athlon / 192.168.0.94**. AGENTS.md와 두 LEFT_JOBS를 읽고 같은 PC의 정밀 디컴파일 기록을 확인했다. 두 정밀 프로젝트에서 함수 **14/15개**를 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-12.**

## 원본 몸체와 구현

[Markup.h](../../cpppj/src/client/Markup.h)·[Markup.cpp](../../cpppj/src/client/Markup.cpp)를 추가했다. 원본 그림 조각의 타입/프레임·배율·색 치환 BYTE, 현재 SHP 헤더 조회·타입 기준점 보정·상자에 맞추기·기준선/크기·출력 분기·복제·소멸을 구현했다. 대괄호 그림 명령의 문자열 파싱은 이번 범위에 포함되지 않는다.

| 동작 | 10.78 | CD 10.72 |
| --- | --- | --- |
| 그림 생성 | `00475810` | `004506f0` |
| 현재 SHP 헤더/기준점 조회 | `00475840` | `00450770` |
| 색 치환 BYTE / 상자 맞춤 | `004758e0` / `004758f0` | `00450800` / `00450810` |
| 가상 기준선 / 측정 / 출력 | `00475960` / `004759e0` / `00475a60` | `004508a0` / `00450920` / `004509c0` |
| 그림 복제 / scalar 삭제 | `00475b80` / `00475bd0` | `00450b80` / `00450c00` |
| 기본 가상 표 전환 | `00475bf0` | `00450c59` → `00450b70` |
| CRT 절삭 | `004e49c0` | `004f161c` |

패치의 `00419850` 프레임 조회와 두 판본의 기본 빈 메트릭/측정 몸체도 내보냈다. 그림 가상 표는 패치 `0050c5d8`, CD `00501f88`이며 scalar 삭제·기준선·측정·출력·복제 순서다. CD 그림 scalar 삭제는 기본 조각 scalar 삭제 `00450b50`와 별도 함수다. 실제 가상 표와 명령에서 이를 구분했다.

## 공통 조각과 수명

`RegisterStyleTextFragment`를 공통 토큰 발급에 사용한다. 그림의 next/+8 DWORD는 같은 `StyleTextState`에 있고 type(+c)/frame(+10)/scale(+14)/mapped BYTE(+18)는 `MarkupState`에 있다. 저장소는 map으로 주소를 유지하며 호스트 토큰을 원본 포인터로 해석하지 않는다. 새 그림은 next=0, 배율=1, 색 치환=0으로 시작한다. 복제는 type/frame/+8을 복사하고 배율/색 치환은 1/0으로 다시 만든다.

소멸은 공통 분류를 `Base`로 전환하며 그림 next·타입·프레임·배율·색 BYTE를 보존한다. 실제 SHP는 전역 자산 소유이므로 그림 조각이 해제하지 않는다. `StyleText::Destroy`가 scalar 플래그를 처리하고 `BindMarkup`의 반납 분배가 파생 표를 정리한다. 기존 반납 통지를 먼저 호출해 소멸 후 필드를 관찰할 수 있게 한다.

문자열 소멸도 `Base` 분류로 전환하도록 보완했다. Base 조각의 기준선/측정은 0, 출력은 빈 동작이다. scalar 반납 전에는 동일한 공통 소유 체인에 남으며 다음 소멸/반납은 같은 표에서 수행한다. 다른 외부 조각의 기존 메트릭·측정·출력·삭제·반납 분배는 유지한다.

## 현재 메타데이터와 축소 계산

`Lookup`은 현재 signed 타입이 `0 < type < typeCount`이고 SHP가 있으며 현재 signed frame이 `0 <= frame < frameCount`일 때만 헤더를 읽는다. 그 외에는 shape=0·여섯 WORD=0이다. 새 `ShapeDatabase::SquidHeader`는 VFX 앞의 **실제 36바이트**를 보존한다. 폭/높이·x/y 기준점·마지막 두 WORD를 signed 비트로 읽는다. 추가 헤더가 없는 글꼴/순수 VFX 파일은 범위 오류로 거부한다.

타입의 x/y 기준점은 SHP 기준점에서 **하위 WORD 감김**으로 뺀다. 타입 기준점은 헤더 조회 뒤 현재 값을 다시 읽는다. 반환 SHP도 조회 뒤의 현재 타입 필드다. 조각에 과거 헤더나 잘린 기준점을 캐시하지 않는다.

상자 맞춤은 `폭/메타 폭`, `높이/메타 높이`의 비율을 원본 순서로 비교한다. 현재 배율보다 작은 값만 선택하는 함수가 아니며 선택한 비율이 원본의 1 비교를 통과할 때 배율을 쓴다.

- **10.78:** 세로 비율을 float32로 저장한 다음 x87의 가로 비율과 비교한다. 가로 비율이 ordered 미만이면 가로를, 그 외에는 저장한 세로를 선택한다. 최종 ordered `< 1`일 때만 저장하므로 NaN은 저장하지 않는다.
- **CD:** 두 비율의 비교에서 가로가 크면 세로를, 같거나 unordered이면 가로를 선택한다. 최종 unordered도 저장 분기로 들어가므로 NaN 배율이 남을 수 있다. 동률의 signed zero 선택도 패치와 다르다.

가상 기준선은 `높이, 보정된 y 기준점`, 가상 크기는 `폭, 높이`다. 배율의 **비트가 정확히 1.0**이면 signed WORD를 그대로 반환한다. 그 외에는 해당 값×배율에 CRT 절삭을 적용해 signed 하위 DWORD를 사용한다. CD의 추가 폭 절삭 재확인도 현재 배율로 수행한다. 임의 포인터/출력이 조각의 float 메모리와 겹치는 raw ABI는 이 의미별 호스트 API의 계약 밖이다.

## 출력과 실제 자산 연결

그림이 없으면 출력/색 설치를 생략한다. 배율 1이면 보정된 기준점을 입력 위치에 DWORD 감김으로 더한다. 그 외에는 `절삭(입력 위치 + 보정 기준점×배율)`을 사용하고 **절삭(배율×65536)**을 x/y 16.16 배율로 전달한다. 회전 인자는 0이다. 원본 두 PE의 ROM에서 비교 상수 **1.0**과 고정 배율 **65536.0**을 읽었다.

색 BYTE는 bool로 정규화하지 않고 하위 BYTE를 보관한다. 비트가 0이 아니면 원본 그림 변환 표를 먼저 설치한다. 그 설치 후 **현재 조각 frame**을 다시 읽어 출력 요청에 넣는다. 이미 계산한 위치/16.16 배율과 헤더 조회가 반환한 SHP는 유지한다. 그림은 전달된 텍스트 override 문맥을 사용하지 않는다.

`MarkupAssets`/`BindMarkupAssets`는 실제 GameAssets의 원본 타입 수·논리 프레임 수·타입 기준점·SHP 블록을 공급한다. 출력에서는 실제 물리 VFX 프레임을 검사해 색 설치 후 바뀐 프레임도 다시 조회한다. 무자산/내장 타입은 null 자산을 공급한다.

`MarkupPixelTarget`은 일반 원색/색 치환 그림을 기존 `GumpPixelTarget`의 **현재 클립·8비트 표면·공유 VFX 색 표**로 합성한다. SHP의 실제 상대 픽셀 영역과 투명도를 보존하며 해제 이미지를 캐시한다. 특별 빈 프레임은 픽셀을 쓰지 않지만 앞서 설치한 색 표는 남는다. 그림 변환 표의 정확한 256바이트 값은 호출자가 공급해야 한다.

**확대/축소 VFX 픽셀 커널은 명시 전달 경계다.** 위치·회전 0·16.16 배율·원색/색 치환을 포함한 원본 요청을 기존 shape 경계로 넘긴다. 그 경계가 없으면 실제 scaled 출력에 도달했을 때 예외로 거부한다. 이번 구현은 임의 보간으로 원본 픽셀 선택을 대체하지 않는다.

처리기 구성 순서는 아래와 같다. 훅이 참조하는 상태/자산/표면/처리기는 StyleText보다 오래 살아야 한다.

```cpp
// 그림 자산과 일반 픽셀 출력에 같은 타입/SHP와 현재 VFX 표를 공급한다.
MarkupState pictureState;
MarkupHooks pictureHooks;
BindMarkupAssets(pictureState, pictureHooks, pictureAssets);
pictureHooks = picturePixels.Hooks(std::move(pictureHooks));
Markup pictures(styleState, pictureState, std::move(pictureHooks), edition);
// 기존 글꼴/팔레트 훅 뒤에 그림 가상 분배를 연결하고 전체 텍스트 처리기를 만든다.
BindMarkup(styleHooks, pictures);
StyleText styleText(styleState, std::move(styleHooks), edition);
// 문자열과 같은 소유 head에 그림을 추가하며 Clear가 두 저장소를 함께 반납한다.
styleText.Append(head, pictures.NewPicture(type, frame));
```

이 구성은 현재 메뉴의 StyleText 가상 측정/출력에서도 같은 그림을 처리할 수 있게 한다. **대괄호 문자열에서 type/frame을 골라 NewPicture를 호출하는 파서 연결은 후속**이다.

## 빌드·분석 자료·후속

게임 라이브러리/`NetstormCpp` **Release 빌드 경고 0·오류 0**. SOURCE_MAP은 생성 도구로 갱신해 Markup 파일 존재를 표시했다. **새 테스트 추가/실행·전체 CTest·독립 원본 동작 대조·GUI 실행은 미수행**이다. 원본 관찰 **523,634개**, C++ 대조 **523,374개**, CTest **854개 통과**는 앞 단계의 수치이며 이번 변경을 검증한 수치가 아니다.

자료: `cpppj/markup.json`, `tools/analyze_markup.py`, `tools/ghidra/markup-functions.json`, `extracted/markup/`, `extracted/markup-*-export.log`, `extracted/markup-build.log`. 정적 기록에는 실제 가상 표/상수/함수 범위/명령과 현재 구현 SHA가 있다. 앞 단계 StyleText 근거는 당시 구현 기록이며 이번 공통 조각 변경의 해시는 새 Markup 근거를 따른다.

```powershell
python -X utf8 tools/analyze_markup.py
cmake --build cpppj/build --config Release --target netstorm NetstormCpp --parallel 2
```

다음은 이번 그림/소멸/기준점/축소 분기와 StyleText/메뉴의 독립 동작 대조, 원본 그림 색 표 공급·확대/축소 VFX 커널·대괄호 옵션/그림 파서·다중 줄 버퍼/줄바꿈/스크롤·메뉴/Form 생성·Attach 보정·입력/통지·OS 큐/폴링·자동 UI 프레임이다. **Client/UberGump는 기존 메뉴 어댑터이며 raw 메뉴 자동 GUI 연결과 건설/미션 완주는 미완료다.**

원본 바이너리 SHA가 기존 기록과 같다. AGENTS.md·보호 원본·dotnetpj 변경, 커밋/푸시 없음.
