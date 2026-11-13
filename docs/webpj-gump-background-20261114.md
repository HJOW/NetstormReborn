# webpj Gump 배경 생성·출력 (WB04-E415, 2026-11-14)

RawMenuControls 선행 GumpBackground(114줄, partial→port)·GumpBackgroundCreate(96줄)·GumpBackgroundDraw(304줄), **세 단위514줄**을 완료했다. 기준선 `1f00a25a` .cpp/.h 전체와 GumpBackgroundTests·GumpBackgroundDrawTests·GumpBackgroundTestAssets·GumpWidePanelTests를 읽었다. 여섯 소스의 기준선 뒤 변경은 없다. CD/추가판은 웹10.78 범위 밖이다.

## 같은 저장소와 현재 출력

- 기존 gump-visual.js 계획기를 유지하며 signed WORD 메타 크기·정렬/감김·100만 호출 안전 경계와 단일 Texture 계획을 보완했다. 순수 IndexedImage 어댑터는 프레임 상대점·소스 색 치환·opacity와 원문 바탕→왼쪽→오른쪽→위→아래 순서를 보존한다.
- GumpBackgroundCreate는 공통 GumpCreate의 같은 파생 노드/등록표를 빌린다. 기본 생성 뒤 현재 signed 색 BYTE를 command에 부호 확장하고, type0이면 기존 shape/타일 필드를 보존한다. 10.78은 타입 조회 뒤 현재 frame을 읽고 프레임 콜백 뒤 현재 shape를 저장한다.
- Draw는 같은 Bounds 뒤 현재 debug/노드를 읽는다. 체크→단색→질감 우선순위와 평면 왼쪽→위→오른쪽→아래의 현재 선 색을 보존한다. debugColor는 fill 전에 DWORD 증가한다.
- Texture는 매 pass마다 현재 타입/프레임 메타와 조회 뒤 clip을 읽는다. 유효하지 않은 크기는 출력 장치 없이 반환한다. 초기 clip 설정/shape 예외에서도 half-open/VFX clip을 복구하며 이미 바뀐 색 표/픽셀은 보존한다. 기존 가상 처리기 fallback도 유지한다.
- GumpBackgroundAssets는 실제 GameAssets/ShapeDatabase의 물리 메타와 논리 클러스터·추가 레이어 비트를 구별한다. 현재 type/frame으로 실제 SHP를 다시 선택하므로 생성 노드에 남은 shape/타일 캐시를 출력에 쓰지 않는다. PixelTarget은 기존 GumpPixelTarget·공유256색 표에 실제 RLE 픽셀/선/fill을 연결한다.
- 명시한 fortGump A02에서만 실제 해독 그림의 연속 불투명 색0 선두 행을 센다. 첫 원문 타일774행은 보존하고 새 높이에서는 선두25행을 제외한749행 본문만 반복한다. 크기/행 수를 하드코딩하지 않으며 투명 검정과 다른 프레임에는 정책을 설치하지 않는다.

## 검사와 경계

새 **10/10 첫 통과**, 주변 Gump/메인 메뉴 **88/88 통과**다. 원본 대조/제품/검사의 실패 교정은 없다.

1. `gumpbackground-x86.tsv` originals **59행**의3개 명암 표 전체1536바이트와56개 전체 계획의 pass/clip/명암/여분 타일/복구 순서가 원문 관측과 같다.
2. `gumpbackgrounddraw-x86.tsv` originals **209행**의 가상/직접 분배, 모든 자산/clip/map/shape/fill/선 인수, 최종 debug/shade, **각768 실제 fill 픽셀**과 guard/clip 복구가 원문과 같다. 해당 fixture에서 선/VFX 그림은 호출 경계이며 픽셀을 썼다고 주장하지 않는다.
3. 실제 기본 생성/등록과 파생 생성의 공유 주소·색 부호 확장·타입/프레임 콜백의 현재 읽기·type0 보존, 안정된 메타 주소·논리/물리 범위/중복 바인딩 진단을 확인했다.
4. 독립 실제36BYTE Squid 메타/24BYTE VFX 헤더·RLE 두 레이어를 사용해 정상/눌림 테두리, 소스 명암, 이전 캐시 무시·현재 타입/프레임·투명 중앙·공유 색/clip/guard를 손계산 색과 대조했다. 순수 IndexedImage 경로도 상대점/opacity/겹침 순서를 대조했다.
5. 실제 평면 checker/solid/debug 픽셀·낮은 색 BYTE·현재 선 색·debug 증가, 조회 뒤 clip·부분 출력 예외·초기 clip 실패 복구·미선택/필수 경계와 가상 fallback을 검사했다.
6. 독립80×774 SHP의 첫774행 전체 보존, 명시한7개 본문 행쌍·작은 dirty 전체 픽셀·명암/clip/guard를 확인했다. 7×13/선두3행과 투명 선두로 메타 사용을 구별했다.
7. **실제10.78 GameAssets 전체 자산 어댑터**를 만들고 fortGump A02 선두25행/메타 높이775를 확인했다. 실제 패널1080행 전체를 독립 SHP 해독의 최초774행/반복 본문 원본 행과 대조했다. 전 타입 게임 객체를 생성/실행하는 검사는 아니다.

원본 파일 변경·게임/브라우저 실행은 없다. 전체 Node v26.13.0·v22.23.3 각각 **3968/3968 통과**, 실패/취소/건너뜀0(366.9초/391.8초). 대응표 오류0, port672·partial56·todo168이다.

## 다음

ButtonGump·StoneButtonGump를 같은 실제 생성/트리/수명·측정/재그리기/도움말·이번 배경에 연결한다. E414에서 이월한 GumpTextTests의 실제 StoneButton 부모 FitToText/라벨/배경/픽셀·해제 시험도 수행한다. StyleText/메뉴·RawMenuControls와 ClientRendererFrame/Session/앱 수명은 미연결이고 실제 미션은 아직 플레이할 수 없다.
