# 섬 미리보기의 실제 Bitmap과 SP 비트 처리 — 단위474

섬 추가 창이 사용할 실제 지형 Bitmap을 원본 `0046e410→004c1cc0→004c1860`에 맞춰 복원하고 기존·신규 Fort의 같은 Session에 연결했다. 보호 SHP의 실제 RLE를 원본 CPU에서도 실행해 전체 픽셀을 비교한다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 마지막 수행일2026-10-23**이며 디컴파일 산출물은 변경하지 않았다.

## 실제 이미지 생성과 장기 소유자

`RawTerrainBitmap`은 X열 우선 문자 배열에서 아래쪽 fringe를 먼저 그린 뒤 행 우선 지면 텍스처를 채운다. fringe 길이는 로컬 GameRandom의 Next(3)+3이며 실제 texture 주기는 SHP0 헤더의 폭−1/높이−1이다. 임시 표면은 원본과 같은200×200/stride200/초기255이고 출력 버퍼는 별도0으로 시작한다. 빈 출력에서도 두 텍스처를 원문 순서로 조회한다.

최초 분류는 `005c8ab0`의255BYTE만 지우고 문자'x'에 현재 녹색 `005acd20`의 하위BYTE를 저장한다. 녹색0은 원문대로 false 분류다. `ClientGameplayGlobals`가 이 캐시와 현재 fringe TYPE를 장기 소유하며, Session 대여·반납이나 팔레트 교체가 캐시를 재생성하지 않는다. 실제 호스트의 최초 녹색 조회는 현재 `Screen::Colors().basic[2]`를 사용한다.

`EditorIslandBitmap`은 별도 실제 Context→일반 Growth→MarkEdges→Bitmap을 조립한다. bitmap의 난수는 owner×4323 로컬 seed이며 공유 게임 난수를 사용하지 않는다. 원문과 같이 출력 폭/높이는 bounds 차×16이고 문자 샘플의 원점은0이다. 시작 좌표가 다른 bounds도 Native 전체 부모의 픽셀로 확인한다.

Session의 `BuildEditorIslandMapImage`는 실제 현재 Territory 지도와 Bitmap을 함께 만든다. `BuildEditorIslandChoiceImage`는 원문 owner0/ignorePosition의 단일 후보 지도를 사용한다. 기존 loaded/Editor/Fort/비Battle 관문을 유지하며 반환 이미지는 호출자가 소유한다. manager가 없으면 기존 지도 준비 결과에 따라 nullopt로 구별한다. 전용 MetaImage·목록·창의 생성/해제는 다음 연결 단계다.

## 원본과 달랐던 SP 가산

기존 섬 배치 회귀 검사는 변경하지 않은 상태에서 다시 보상 정수 변환 오류를 보였다. 초기 객체 교체의 삭제 flags0은 수신자0의 보상을 호출한다. known1의 시작 금액을0으로 명시했어도 소유자0에는 실제 할당 입력 비트가 남을 수 있다.

원본 `0040eec0/0041df80`과 CRT ftol을 독립 실행해 큰 유한 값·무한대·NaN·서브노멀, 소유자0/1/3, 일반/두 고정 모드를 관측했다. 원본은 SP 비트를 계산·저장하고 비유한/64비트 초과 ftol에서 정수 불확정값의 하위DWORD0을 지갑에 전달한다. 기존 C++ 가산은 이 위치에서 예외를 던지고 있었다.

PATCH SP 가산의 해당 변환을 원문대로 복원했다. NaN 등의 저장 비트와 지갑 전달 비트도 새 Native504행과 정확히 비교한다. 기존 비용 계산·수신자 범위 검사와 CD 경로는 그대로 유지했다. 기존 섬 배치 검사, 기존 보상 검사 본문·입력·기대값은 변경하지 않았다. 10.72의 이 비정상 SP 입력 확대 관측은 이번 범위에 포함하지 않는다.

## 독립 증거와 검증

- 래스터 **14입력×두 x87 모드=28관측**은 실제 `004c1860`, 문맥 저장/복구, 보호97/102 SHP0의 RLE와 로컬 난수를 실행한다. TYPE 포인터 조회와 bitmap heap만 명시 장치다.
- 전체 부모 **6입력×두 모드=12관측**은 `004c1cc0→실제 Context/Growth/MarkEdges→보호 RLE/Bitmap`을 실행한다. 빈/단일/연결 청크·owner0/1/8·시작 좌표가 다른 bounds·14×14 크기를 포함한다. 최초 분류 캐시는 녹색43으로 사전 준비하며 heap/free와 TYPE 포인터만 명시한다.
- SP 가산은 **252입력×두 모드=504관측**이다. float 연산과 실제 CRT ftol은 원문이며 SP Get/Set·지갑·표시만 관측 장치다. 저장→지갑→표시 순서와 정상 스택/x87/CODE 보존을 확인한다.
- 세 자료의 총 **544관측**을 생성 후 `--verify`로 다시 실행해 바이트 일치를 확인했다. 새 fixture와 Native 보고서는 원본 파일·기존 fixture와 별개다.

원본 래스터 격리 초기 실행에서는 FS:[0] 입력과 VFX의 별도 fast 함수 허용 범위가 빠져 실패했다. 격리 입력/허용 표를 보완하고 원문 CODE를 유지했다. 첫 빌드 도중 전체 부모 검사를 추가해 기존 CMake 구성에 경로 macro가 없었던 실패도 보존했다. 최종 구성 재생성 후 빌드와 검사를 수행한다.

Windows에서는 `reward`/`Reward` 로그 이름이 같은 파일이므로 검사기가 덮어쓰기를 거부했다. 소문자 결과는 유지하고 대문자 결과만 별도 label로 실행했다. 최종 보고서는 실행한 실제 사례 이름으로 고유/관측 수를 구분한다.

## 최종 결과

- 최종 Release build4는 종료0·경고/오류0이다. build2/3도 경고/오류0이며, 첫 실패와 원시 진단은 보존했다. build4 이후 제품/검사 소스는 동결했다.
- 집중 **117고유/118관측·실패0**이다. Bitmap6 + EditorIsland4 + ExistingEditor58 + TerrainContext2 + TerrainGrowth2 + NormalTerrain3 + ClientStateWorld13 + NewFort13 + reward9 + Reward8이며, 신규 Bitmap의 NewFort 사례1개는 두 필터에서 관측했다.
- 같은 최종 제품의 전체 C++ **3,246개·실패0**이다. CTest wrapper 대신 `netstorm_tests.exe` 전체를 직접 실행했다. 기존3,239등록 이름을 유지하고 신규7개를 추가했다.
- 게임 SHA256은 `3c26ebe9fdfcc841754674b7a9204bc5726f05d1f5581f69207bd8baf1c64add`, 검사 실행 파일은 `f960367f44e679b5c034e31f7064a7fdd4e0816bbc5ce7fe56ea52ef1a41a4bf`다.
- 원본/CD/AGENTS·기존 검사/fixture3,166파일은 바이트동일이다. 기존 Checkpoint 검사 파일은116,776BYTE 전체 접두부를 보존하고 새 include만 추가했으며 검사 CMake에는 새 파일/자료 경로만 등록했다. 기존 섬 배치 검사와 보상 검사는 바꾸지 않았다.
- 최종2,810소스/검사 파일의 manifest는 `extracted/cpp-island-bitmap-474/source-final.json`에 있다. 보고서에는 최초 실패·원본 관측·최종 실행 로그의 SHA를 기록했다.

원시 로그는 `extracted/cpp-island-bitmap-474/`, 최종 실행 결과와 SHA는 [보고서](../../cpppj/island-bitmap-runtime-20261027.json)에 둔다. 실제 게임·GUI·음향 장치는 실행하지 않았다. 다음은 MetaImage의 크기·출력·소멸,21개 전용 목록과 Add Island 창의 생성/Refresh/Idle/단일 창 수명·공통 트리 등록이다. 전체 Edit/시나리오·자연 미션 완주·화면/음향 동일성 목표는 계속 미완료다.
