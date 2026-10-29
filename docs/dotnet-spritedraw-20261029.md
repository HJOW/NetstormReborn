# dotnetpj R2d-3 — 일반 객체 그리기와 실제 픽셀 출력

2026-10-29, HJOW-Athlon. 이식 기준은 `8ca081a26b4b`의 `cpppj/src/client/SpriteDraw.h`·`SpriteDraw.cpp`와 `src/o/RawRenderCollection.h`다. 마지막 전체 정밀 디컴파일 호스트도 HJOW-Athlon이다. 이 단위에서는 커밋된 cpppj 소스와 기존 fixture를 읽었으며 디컴파일을 다시 수행하거나 산출물을 변경하지 않았다.

## 1. 구현

`Netstorm.Client/Display/SpriteDraw.cs`는 토큰→SID 해석, 화면 WORD/월드 좌표 계약, 그림자 조기 반환과 SHP alias 쓰기, plain/owner/abstract/dead 분기, 가상 프레임·가상 Draw, 소유자 표식, 상태 그림과 선택선/막대를 옮겼다. 판본별 WORD·BYTE 필드, signed DWORD 감김, signed64 절삭의 하위 DWORD, 호출 순서를 유지한다.

가상 함수가 raw나 공유 전역을 바꿀 수 있으므로 그 뒤의 필터·소유자 색표·표식 옵션·선택 비트·Kernel 프로세스 목록은 현재 값을 읽는다. 공유 DWORD 포인터는 .NET 대리자로 빌리며 조회 시점은 C++와 같다. 독립 fixture용 입력도 같은 우선순위를 갖는다.

`SpritePixelTarget.cs`는 SHP 디코딩 캐시, 현재 소유자 색 문맥, 소스 그림자 색 변환, 투명 마스크 보존, 양 끝 포함 직교선과 끝 제외 fill을 옮겼다. 기존 `RendererDraw.DrawIndexedImage`와 같은 8비트 표면에 쓴다. `RenderCollectionEntry.cs`는 수집·정렬·출력 사이의 원본 16바이트 항목 의미를 보존한다. 수집기·분배기 본체는 이 단위에 포함하지 않는다.

`MakeSpriteSurfaceLookup`은 `+0.9999F`를 double로 승격한 뒤 절삭하며, 경계 밖은 SID 0을 반환한다. 현재 해시 선택 단계를 바꾸지 않고 0단계만 조회한다.

## 2. 검증

- `spritedraw-x86.tsv`를 사본 없이 Link로 연결하고 SHA-256 목록에 등록했다. **1,389행 = originals·originalCD·original1037 각 463행**이다. 전체 명령열·선택 SID·alias 쓰기·raw 불변성을 원본 기대값 그대로 대조한다. 이전 인계의 “실자료 인프라가 있어야 L1 가능”은 일반 Draw 분기에 대해서는 불필요했다. 기존 명령열 fixture는 독립 raw/SHP 헤더 입력만으로 재생할 수 있다.
- CD·추가판 관측도 Draw 필드/명령열 검증에 포함한다. 다만 현재 `RawSidPool`은 10.78의 50바이트 저장 간격이므로 CD 입력 필드는 그 저장소에 옮겨 적재한다. **CD 풀의 36바이트 배치나 원본 주소 일치 검증으로 세지 않는다.** 이번 목표는 10.78이며 CD 풀 이식은 별도다.
- cpppj의 독립 계약 10개를 xUnit으로 이식했다. raw→Draw→SHP→픽셀, 투명·불투명0/255, 소유자 문맥, 소스 그림자, 클립·선·막대, 조기 반환/잘못된 토큰/좌표, 가상 Draw 후 raw 재조회, alias 물리 헤더/논리 count 분리, 현재 필터·공유 진단/소유자표/표식, 표면 경계를 검사한다. 원본의 수집·정렬·공통 그룹 분배 검사는 해당 모듈이 없는 R5로 남긴다.
- 변이: 그림자 프레임에 1을 더하자 세 판본 모두 60번 입력에서 `S,70,36,...` 대 원본 `S,70,35,...`로 실패했다. 변이를 제거한 소스로 최종 전체 검사를 실행했다. 최초 빌드 오류 1개는 C# 저장소의 `Blocks()` 호출 괄호 누락이며 수정했다. 기대값 변경은 없다.
- 원시 로그는 Git 제외 `extracted/dotnet-spritedraw-mutation-20261029.log`·`dotnet-spritedraw-tests-20261029.log`에 있다. 게임·창 실행, 실제 사제 포획/희생·특정 적 공격·시나리오 승리 재현과 멀티플레이 검사는 수행하지 않았다.
- 최종 전체 검사는 **1,513개 통과(Assets363·Core1030·Client120), 실패/건너뜀0**이다. 원본 fixture 목록은 84개이며 포트 맵은138행(97 L1·41 port)이다. 기준 이후 변경 신호29쌍(27 L1·2 port)은 후속 재대조 대상으로 유지하고 전체 reflected-commit은 올리지 않았다.
- 최종 소유자 색 문맥은 C++ `ColorMap` 값 복사와 같게 배열을 복사한다. 반환한 표를 바꿔도 설정한 문맥이 유지되는 검사를 더했다. SID의 WORD 변환까지 재대조한 뒤 Client120개를 재실행해 모두 통과했다. 전체 Release 빌드는 경고/오류0이다.

## 3. 남은 연결

이 단위는 L1 명령열과 독립 실제 픽셀 계약 검증이다. 세션의 실제 그림자 alias/색표 전역, `GameAssets`에 대응하는 `MakeSpriteAssetViews`·`MakeSpriteAssetShapes`, `SquidRenderer::Shapes`, 수집/정렬/분배와 일반 Draw 바인딩은 R4·R5에서 잇는다. 전체 게임 화면의 픽셀 일치나 플레이 완료로 간주하지 않는다. 기존 근사 코드와 검사는 동결 상태로 유지했고 은퇴시킨 항목은 없다.

다음은 R3의 Gump 저장소/생성/트리/입력/그리기와 `.chfnt` 글꼴·GIF 어댑터다. 이후 세션·HUD·플레이 규칙·Edit·전체 미션·배포를 현행 계획 순서대로 연결한다.
