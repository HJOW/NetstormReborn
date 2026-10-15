# Tell 본문 실제 글꼴·다중 줄 버퍼 연결

현재/마지막 정밀 디컴파일 PC는 **VM-W11-Codex, 2026-10-15**다. 이 단위는 이미 복원한 기본 HTML/StyleText/다중 줄 버퍼/폰트 처리기를 연결한다. 새 원본 실행·GUI 실행·정밀 export는 없으며 보호 원본·AGENTS·dotnetpj를 변경하지 않는다. `DialogScript`, `UberGump`, `Renderer`는 직접 수정하지 않고 부모 작업에서 최종 Compose 연결을 수행한다.

## 소유 범위와 API

`client/DialogBodyLayout`은 페이지 설정과 반복 화면 배치를 분리한다. `SetBody()`가 HTML 효과를 한 번 실행하고, 같은 실제 StyleText/Markup 표와 버퍼에 본문을 소유한다. `Layout()`은 이미 만든 본문을 배치하므로 `$` 명령/조건/배경 효과를 반복하지 않는다. 같은 폭의 표시 복제도 재사용한다. 실제 Early1/2의 `<!...>` 선택을 위해 후속으로 원본 그림 helper와 기존 다중 줄 markup/실제 SHP 연결을 추가했다.

```cpp
DialogBodyLayout body(fonts,palette,gameAssets,edition); // 실제 자산은 body/결과보다 오래 산다.
HtmlTextParserState htmlState; // 같은 파서의 현재 shown/depth다.
HtmlTextParserGlobals htmlGlobals; // 같은 P 억제 전역이다.
std::array<std::uint32_t,8> context{0,0,palette.basic[7],0,0,0,1,1}; // 기본 TextGump 문맥이다.
HtmlTextParserHooks hooks;BindHtmlPictureText(hooks); // 기본 그림 문자열 helper를 연결한다.
body.SetBody(pageHtmlBody,context,htmlState,htmlGlobals,hooks); // 페이지가 바뀔 때 한 번 호출한다.
auto result=body.Layout(contentWidth,contentOrigin,contentClip); // Compose에서 반복할 수 있다.
```

결과의 `text`는 기존 `RenderText[]`다. 각 항목에 실제 `BitmapFont*`, 원본 바이트, 기준선에 맞춘 위치, 현재 색/그림자/외곽선, 전달받은 내부 clip을 보관한다. `sprites`는 실제 Markup 가상 출력이 고른 원본 SHP/물리 프레임/기준점/내부 clip의 `RenderSprite[]`다. `maximumWidth`는 실제 표시 줄의 signed 최대 폭, `height`는 `StyleTextBuffer::TotalHeight()`의 마지막 줄 y+높이, `lineCount`는 실제 표시 줄 수다. `height`는 `ScrollingTextGump::FitHeight`의 추가1을 포함하지 않는다. 전체 높이/최대 폭은 viewport clip과 독립이다. 기존 문자 전용 생성자도 유지하지만 실제 그림 자산을 만들어 주지는 않는다.

`SetBody()`는 출력 설정→입력 설정→기본 HTML 변환 뒤 새 소유 본문을 작성한다. 성공한 뒤에만 이전 본문과 교체하므로 새 HTML/외부 markup이 실패하면 이전 성공 본문을 배치할 수 있다. 이미 실행한 외부 callback 효과를 rollback한다고 주장하지 않는다. `Layout()` 폭은 양수여야 한다. 페이지 미설정 상태의 양수 폭 배치는 빈 결과다.

## 기존 원본 처리기 재사용

1. `HtmlTextParser::Parse`의 정확한 제어문자열을 받는다. `<i>`는 `~N~j` 색이며 `<b>`는 `~N~B`다. 인라인 태그에서 임의 개행하지 않는다.
2. 기존 `ScrollingTextGumpMarkup`이 같은 실제 파생/기본 노드와 문맥/metadata/callback/버퍼를 사용한다. 이 노드의 Redraw는 페이지 내부 변경 요청에 기록한다. 문자·그림 구간을 별도 임시 체인에 모았다가 재해석하지 않는다.
3. 문자열은 실제 `ScrollingTextGump::AppendText`→`StyleTextBuffer::AppendText`, 그림은 `StyleTextMarkup::ApplyScrolling`→`ScrollingTextGump::AppendPicture`→버퍼의 실제 그림 추가/맞춤으로 전달한다. 단일 줄 Picture의 `#`/`&` 규칙을 대신 사용하지 않는다. 기존 LF 분리와 미완료 끝 칸 계약을 유지한다. 줄 칸은 변환 결과의 LF 수+1개를 확보해 페이지 앞줄이 용량 때문에 버려지지 않게 한다.
4. `BindStyleTextFonts`/`BindStyleTextBufferFonts`가 같은 `GumpText`를 통해 실제 `FontStore`의 첫 폭 표, 별도 논리 슬롯 높이, 캐시 상승/하강을 읽는다. `BindStyleTextPalette`는 호출자가 제공한 실제 팔레트의 원본 색 번호를 읽는다.
5. `StyleTextBuffer::Layout`이 원본의 strict 폭 비교, 단어/공백 복사, 명시/자동 줄 구분을 처리한다. 임의 문자열 길이나 평균 글자 폭으로 wrap하지 않는다.
6. 그림의 실제 `MarkupAssets`와 `BindStyleTextMarkupAssets`는 같은 GameAssets에서 현재 타입 번호/이름/논리 프레임 선택/기본·Gump 프레임·글자별 시작과 물리 SHP 메타 헤더를 공급한다. 버퍼의 외부 복제는 `BindStyleTextBufferMarkup`을 사용하며 원본 `ClonePicture`의 배율1/색치환0 초기화 규칙을 그대로 적용한다.
7. 각 표시 줄은 기존 `StyleText::Draw`의 두 번 기준선 순회로 출력한다. 각 줄의 최대 ascent를 구하고 조각별 ascent를 뺀 y에서 출력하며 실제 측정 폭으로 x를 전진한다. 이는 `ScrollingTextGump::Draw`의 기준선 방식과 같다. 외부 그림은 `BindMarkup`의 실제 Baseline/Measure/Draw 분배를 사용한다.

출력 폰트 선택은 `GumpTextDraw`와 같은 원본 슬롯 검사·불법 스타일의 font0/normal fallback·비어 있는 조합을 사용한다. CD의 slot6/bold도 빈 조합으로 유지한다. 실제 빈 폰트 조합을 다른 GDI 글꼴로 대체하지 않는다. 모든 글자는 요청 clip을 가진 `RenderText`로 제출되며 최종 픽셀 잘라내기는 기존 Renderer가 한다. 전체 줄의 측정은 가시성 때문에 생략하지 않는다.

버퍼 입력 조각과 표시 복제는 별도 소유다. 페이지 교체/소멸은 완료 입력 줄·표시 복제를 원본 scalar 반납 경계로 해제한다. 그림의 파생 표도 같은 `BindMarkup` 반납 분배에서 정리한다. 원본 `StyleTextBuffer::Destroy`가 남기는 미완료 끝 칸은 이 어댑터의 소유이므로 먼저 명시 해제한다.

## 효과와 실제 통합 경계

부모가 공급할 `ParseDialogPage::htmlBody`는 `$Button` 등 페이지 메타, `<$...>` 명령, 배경 태그를 분리한 본문이다. helper 자체는 메타/Config parser를 중복 구현하지 않는다. 기본 `HtmlTextParserHooks`는 호출자가 공급하며 선택된 실제 효과가 없으면 기존 예외 경계가 유지된다.

`BindHtmlPictureText`는 PATCH0046bf40/CD00495310의 기본 문자열 helper다. 먼저 `~[I`를 쓰고, `>`/NUL 앞의 첫 따옴표를 찾아 안쪽을 다음 따옴표/`>`/NUL 앞까지 그대로 복사한 뒤 `]`를 쓴다. 인용이 없으면 `~[I]`다. 이름/프레임/대소문자/entity를 정규화하지 않는다. 원본 tutorial2.english의 실제 `<!"mana.8>`/9/10의 닫는 따옴표 누락도 이 규칙으로 처리한다.

그림은 GameAssets 지원 생성자에서 실제 자산/헤더를 사용한다. 문자 전용 생성자에 그림 리소스 경계가 없으면 성립하는 I 형식의 이름 조회/타입 조회에서 예외를 낸다. 원본이 이미 무효 타입(-1)이나 성립하지 않는 정규식을 소비하는 경우는 원본의 무효 동작이며 임의 그림 제거 fallback을 추가한 것이 아니다. 링크 저장소/상호작용과 소리 장치는 여전히 호출자 경계이며 빈 성공 훅을 공급하지 않는다.

현재 다중 줄 표시 복제는 원본 ClonePicture에 따라 배율1/색치환0이다. 다른 경로가 배율/회전/색치환 요청을 실제 만들면 본 어댑터는 명시 오류를 낸다. Renderer의 확대/축소 경로를 구현했다고 주장하지 않는다. 모든 그림/글자는 기존 Renderer의 sprite→text 계층으로 제출되므로 서로 겹치는 그림/문자 사이의 완전한 호출 순서 합성은 별도 검증 대상이다.

현재 부모의 `FilterDialogConditions` 선처리와 원본 조건 평가의 기존 편차는 유지한다. 이 모듈을 연결했다는 이유만으로 원본 조건 파서/Config 치환의 동일성이 완료되지는 않는다. 실제 패널480폭/본문 영역 계산·클립·페이지 전환·Compose 연결과 GUI 확인은 부모 작업 범위다. 스크롤바/입력/Gump 전체 수명·가시 행 생략 최적화를 새로 구현한 것으로 세지 않는다.

## 검증 기록

검사 파일은 `tests/DialogBodyLayoutTests.cpp`다. 원본 `originals/netstorm.tarc`, `d/gifcloud.col`, Arial `chfnt`를 읽는 실제 VFS/FontStore를 사용하며 폰트 파일을 쓰지 않는다. 특히 normal14/bold14 캐시 존재를 검사하여 합성 폭 표의 결과로 세지 않는다. 화면 장치나 창은 만들지 않는다.

집중 검사는 실제 inline 색/normal·bold 폰트 포인터·전진 폭·같은 행 기준선, 좁은 폭의 원본 버퍼 wrap/높이 증가·clip 독립 측정, H1/P/BR의 실제 폰트 슬롯·그림자·빈 줄 높이, HTML 효과1회·미공급 효과와 실패 뒤 기존 본문 유지, 메모리 Renderer의 실제 폰트 픽셀/내부 clip을 확인한다.

- 최종 Release 게임/검사 빌드 **경고·오류0**, `--filter DialogBodyLayout` **집중5개/실패0**이다. 로그는 `extracted/dialog-body-build1.log`, `extracted/dialog-body-tests1.log`다.
- 이전 HTML 단위의 두 PE **816 native 입력**은 이번 어댑터의 새 native 입력으로 중복 계산하지 않는다.
- 기존 모듈의 native fixture 전체 반복이나 실제 GUI 검사는 이번 단위에서 실행하지 않는다.
- 새 소스/문서 UTF-8 해석과 공유 CMake 공백 검사를 확인했다. 본 단위의 코드/문서/검사 파일은 부모 통합 커밋 전 고정하며 LEFT/커밋/실제 Compose 연결은 부모가 수행한다.

### 후속 그림 복원 검증

`tools/decomp_htmlpicture_oracle.py`는 기존 기본 파서 VM의 허용 범위에 실제 그림 helper 몸체만 추가했다. **두 PE 276 고유 입력/정상 반환**(판본별138)의 전체 출력·커서·상태를 별도 `htmlpicture-x86.tsv`/`cpppj/htmlpicture.json`에 보관한다. 원본 tutorial1/2 그림11형태와 인용/미완성/긴 이름/high byte/NUL/닫힘·숨김 상태 입력이다. CRT signed 입력/ASCII 소문자화만 기존 ABI 경계이며 게임·GUI·OS 호출은0이다. 이전816개와 중복 합산하거나 전체 서식 fixture를 다시 실행한 자료로 세지 않는다. 생성기/원본/fixture SHA·사전 입력계획·집계를 `--verify`로 감사한다.

추가 본문 검사는 실제 Early1/2 그림11형태의 타입/기본·Gump 프레임/물리 SHP/기준점/폭/높이와 닫는 인용 누락, 원본 다중 줄 # 맞춤→외부 Clone, 폭 변경·페이지 교체·실제 그림+글꼴 픽셀 clip을 확인한다. 기존 그림 미연결 검사 문자열은 원본 I 정규식이 성립하도록 `~[Ipicture.0]`으로 명시했다.

첫 집중 검사에서 **11형태 모두 유효 그림일 것이라는 검사 가정 5체크**가 실패했다. 실제 보호 archive의 `d/mana.type`은 A01~A08 **8개 클러스터**, 보호 `d/_shapes.shp`의 mana블록33도 **8개 물리 프레임**이다. 원본 스크립트가 참조하는 mana17/11/8/9/10은 이 자산 범위를 벗어난다. 원본 `004ba8f0`의 그림 선택은 상한을 `count-1`이 아닌 **count(8)**로 clamp하며, 뒤의 `Markup::Lookup` 원본 `00475840`은 `frame>=count`를 거부한다. 따라서 이 5형태는 실제 sprite가 없고 폭0/마지막 빈 문자줄 논리 높이14가 맞다. 별도 임의 그림을 대신 넣거나 데이터 범위를 확대하지 않았다. 검사만 이 두 단계 원본 경계에 맞게 교정했고 유효6형태는 실제 SHP/기준점/폭/높이를 비교한다.

이 읽기 전용 정적 자산 근거의 SHA-256은 다음과 같다. 원본 Type 파서는 frame 목록 수를 타입+114에 쓰므로 그림 선택의 count와 같은 단위다(`0049c3b0` 본문 `piVar9[0x45]=iVar3`).

| 입력 | 크기 | SHA-256 |
|---|---:|---|
| originals/netstorm.tarc | 1,102,126 | `f2cafc1a0242ffe59b9b2fbfd6555907a03585cd6053d4bb4b0599a2445ea2e6` |
| originals/d/_shapes.shp | 5,618,120 | `c878353c8054208deef832e34c514b30fe670e175505b129333a5728ba70f68e` |
| archive에서 읽은 d/mana.type 평문 | 565 | `e6682f314e4f65aa7089bb624b4f55034bbbec059818f2d2c689ed463609e14d` |

최초 그림 빌드는 경고/오류0, HTML 집중6개는 통과했으나 위 **검사 가정** 때문에 본문7개 중1개/5체크가 실패했다. 그림 제품 코드에 가짜 프레임/대체 이미지를 추가한 것이 아니며 실제 코드 변경 없이 기대 범위만 고쳤다. 이 로그는 `extracted/dialog-picture-build1..2.log`/`dialog-picture-html-tests1.log`/`dialog-picture-body-tests1..2.log`에 보관한다.

교정 뒤 부모의 최종 공용 Release 게임/검사 빌드는 **경고·오류0**, HTML 집중 **6개/실패0**, 본문 집중 **7개/실패0**이다. 최종 로그는 `extracted/run47-50-build.log`, `extracted/html-picture-root-focused.log`, `extracted/dialog-body-picture-root-focused.log`다. 부모의 실제 GUI 재검사는 별도 통합 작업이며 여기서 먼저 완료했다고 세지 않는다. 다른 병행 제품 모듈의 교정과 본문의 검사 가정 교정은 구분한다.

### 실제 Tell 화면 연결 결과

UberGump는 실제 GameAssets/FontStore/Screen팔레트로본문을소유한다. TryTell의페이지설정때만HTML효과를실행하고 반복Compose에서는같은Buffer표시복제를배치한다. 이전문단별임의Wrap/italic글꼴변경을제거하고제목·빈줄·inline색/굵기·실제SHP그림을현재본문clip에제출한다. 배경태그는현재페이지의기존VFS배경표시로연결하고inline명령은앞단위의실제입력큐를유지한다.

실제클론검사는기존메뉴/1-1대화상자·캡처 **97상태**, Early1/2 A./B./C.·MORE/BACK/OK·Kernel현재섹션/다음Input Warning **23상태**, 두완료섹션의Done설정/다음Tutorial/실제옵션저장 **9상태**에서모두통과했다. originals전체파일명/SHA와허용설정은원상복구했다. 원본게임은실행하지않았다. `tutorial1-b.bmp`에서강조구간이같은행에서이어지고`i`의원본tan색을사용함을육안확인했다. 전체픽셀동일성이나실제플레이완주로세지않는다.

현재자료는`extracted/dialog-body-ui-{dialog,progress}-final.log`, `extracted/dialog-body-current-sections.log`, `extracted/dialog-body-mission-tell/`이다. 앞선그림미연결진입실패는`extracted/dialog-body-ui-progress.log`에보존하고최종성공과구분한다. 새원본관찰은그림276개뿐이며독립어댑터/GUI검사를기존816개에중복합산하지않는다. 조건은기존FilterDialogConditions선처리가남아있고원본조건효과는후속작업중이다. 링크/파생help·소리/비표준그림배율·스크롤바전체수명및완주동일성은미완료다.
