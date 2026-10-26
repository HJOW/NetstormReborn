# 섬 추가의 실제 Image 변환과 수명 — 단위475

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 일치한다. 정밀 C와 원본/CD 파일은 수정하지 않는다. 앞 단위474의 실제 Context/Growth/Bitmap을 원본 `0046d790→004dad20→004042cd→0040463b/004047ab`에 맞춰 동적 VFX Image로 변환하고 같은 Editor Session에 연결한다.

## 원본 Image와 실제 소비 경로

`RawImage::Convert`는 행 우선 bitmap의 소유권을 받아 shape를 만든 뒤 입력 픽셀 버퍼를 반납한다. 현재 지도는 crop0, 목록 후보는 crop1이다. crop1은 마지막 nonzero의 X/Y만 사용하므로 왼쪽과 위쪽 여백을 유지한다. 팔레트255도 nonzero이며 투명 색은0이다. 전부0인 후보는 최소1×1 크기를 남긴다.

출력은 별도 TYPE/Squid 추가 헤더가 없는 VFX 1.10 단일 프레임이다. 상자와 기준점은 crop 폭−1/높이−1이고 실제 nonzero 사각형은 이 기준점에 상대적인 좌표다. 완전히 빈 그림의 sentinel은 원문32비트 감김까지 유지한다. 현재 그림을 호출자가 정한 일반 UI 크기로 바꾸거나 기존 자산 ImageGump로 대신하지 않는다.

행 시작 pair는 길이2부터 repeat, literal 안의 같은 색은 triple부터 repeat다. literal/repeat는127, skip은255 단위로 나눈다. 행 끝의 투명 run은 생략하고0 terminator를 쓴다. `CopyFrom`은 먼저 현재 Image를 파괴한 뒤 깊게 복사한다. `Destroy`는 두 버퍼와 폭/높이를 비우되 마지막 shape byte 수는 보존하므로 두 번 호출할 수 있다. 호스트의 vector는 각 버퍼를 독립 소유한다.

`RawCampaignSession::BuildEditorIslandMapShape`와 `BuildEditorIslandChoiceShape`는 기존 실제 bitmap 준비 API를 사용한다. 기존 Editor 설치 완료/manager/Fort/비Battle 관문을 유지하며 manager 부재는 nullopt다. 지도 crop0과 후보 crop1이 실제 세션의 같은 장기 분류 캐시/보호 SHP를 사용한다. 전용 MetaImage와 목록/창 수명은 다음 단계다.

## 독립 원본 관측

새 `tools/decomp_editor_image.py`는 10.78 원본의 변환/전체 인코더, `0046d900` 깊은 복사, `0046d8a0` 이중 소멸을 실행한다. malloc/free 주소만 명시 장치이며 원문 CODE는 보존한다. 총785입력×두 x87 모드=1,570행이다. 빈/단일/offset/경계 그림, 긴 repeat/literal/skip, 여섯 픽셀의 모든 삼색729조합, 앞 단위의 실제 전체 Bitmap12개를 포함한다. 각 행에 입력 픽셀·폭/높이·crop, 전체 shape 바이트·복사 바이트·소멸 후20BYTE·heap 호출 순서를 보관한다.

최초 격리 실행은 원문 malloc thunk `0043fa80` 경계가 없어 중단됐다. 실제 heap 경계를 식별해 허용하고 원문 변환/인코더를 유지했다. 첫 실패와 후속 실행 로그는 `extracted/cpp-editor-image-475/`에 보존한다. 10.72 대응 변환 주소는 정밀 C의 `004bc480`이며 이번 신규 확대 관측은 10.78만 수행한다. raw imageBits가 남아 있는 별도 상태의 복사와 MetaImage 가상 함수/실제 GUI는 이 fixture의 증거 범위에 포함하지 않는다.

## 최종 검증

- Release build1은 종료0·경고/오류0이다. 이후 제품/검사 소스2,815파일을 동결하고 빌드 시점 manifest와 마지막 SHA의 일치를 확인했다.
- 신규 Image2개를 포함한 집중99고유/100관측·실패0이다. EditorImage2 + EditorIsland4 + IslandBitmap6 + ExistingEditor58 + NewFort13 + reward9 + Reward8이며, NewFort/Bitmap 사례1개가 두 필터에서 관측됐다.
- 같은 제품의 전체 C++3,248개·실패0이다. 기존3,246등록 이름을 유지했다. CTest wrapper가 아닌 실제 검사 실행 파일 전체를 직접 실행했다.
- Native785입력/1,570행을 생성한 뒤 실제 CPU로 다시 실행해 fixture 전체 바이트가 일치했다. C++은 Native shape 전체 바이트·크기·깊은 복사·이중 소멸을 비교하고 기존 RLE renderer로 원래 crop 픽셀을 다시 그렸다.
- 원본/CD/AGENTS·기존 검사/fixture3,172파일은 바이트동일이다. 기존 Checkpoint116,911BYTE 전체 접두부를 보존하고 새 include만 덧붙였다. 기존 검사 입력·기대값을 변경하지 않았다.
- 첫 집중 명령에서 `--filter`를 빠뜨려 실행기가 거부한 로그도 보존했다. 올바른 인자로 같은 빌드를 실행했으며 제품/검사 소스 수정은 없었다.
- 게임 SHA256은 `69edeaccc1ba105fc9c3a92110ea80d4416cf06f50076dfa31e7bfd8dfc06f14`, 검사 실행 파일은 `55c51a0c73e2de3980c31a9455078ab98e440f231ebf2447b760cb07e39491d9`다.

원시 로그와 최종 소스 manifest는 `extracted/cpp-editor-image-475/`, 실제 명령·종료값·SHA는 [실행 보고서](../../cpppj/editor-image-runtime-20261027.json), 원문 범위는 [Native 보고서](../../cpppj/editor-image-native-20261027.json)에 둔다. 실제 게임/GUI/음향 장치는0회다.

다음은 실제 `0047f2a0` MetaImage의 생성, `0047cbd0`의 image 크기+1, `0047cc00`의 Attach 후 배율, `0047cae0`의 plain/mapped/배율 Draw다. Draw의 기준점은 실제 image 폭/높이를 더하므로 crop 크기와 VFX 기준점의 차이를 유지해야 한다. 이후21개 목록과 Add Island 창의 생성/Refresh/Idle/소멸·단일 창 수명을 같은 공통 트리에 연결한다. 전체 Edit/시나리오와 모든 미션의 자연 완주/원본 전체 동일성 목표는 계속 미완료다.
