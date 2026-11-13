# webpj Markup 그림 조각·자산·픽셀 (WB04-E420, 2026-11-14)

RawMenuControls 선행 **Markup 465줄(.cpp/.h, todo→port)**을 완료했다. 기준선 `1f00a25a` 전체 소스와 [CPP 정적 복원 문서](exe/cpp-markup-picture.md)·현재 메뉴/HTML 그림 전달 사용처를 읽었다. 두 소스의 기준선 뒤 변경은 없다. **전용 Markup PE fixture/독립 원본 직접 대조는 없다.** CPP 문서도 당시 테스트/직접 대조 미수행을 기록한다. 아래는 기준선 계약·독립 현재 응답·실제 자산/글꼴/픽셀 연결 검사이며 원본 직접 관측 수치로 세지 않는다. CD는 웹10.78 범위 밖이다.

## 같은 그림과 조각 수명

- StyleTextState의 같은 자동 토큰/공통 next·metadata와 MarkupState의 안정된 type/frame/float scale/mapped BYTE를 사용한다. 두 번째 표 등록 실패는 새 공통 조각만 지우고 증가한 토큰은 유지한다. Clone은 type/frame/metadata만 복사하고 배율/치환을1/0으로 새로 만든다.
- Destruct는 Base로만 전환하여 next/그림 필드를 남긴다. Contains는 실제 반납 전까지 true다. BindMarkup은 문자열과 같은 가상 baseline/측정/출력·소멸/반납에 연결하고 기존 다른 외부 조각 fallback을 보존한다. 기존 release 관찰 callback을 먼저 실행한 뒤 파생 그림 표를 지운다.
- Lookup은 signed 타입 범위→현재 타입/SHP→조회 뒤 현재 frame→논리 헤더→헤더 조회 뒤 현재 타입 기준점/SHP 순서다. 타입 기준점의 하위WORD를 빼 signed WORD로 감기며 무자산이면 여섯 WORD0이다.
- Fit은 세로 비율을 먼저 float32로 저장하고 선택 결과가1 미만일 때만 기존 배율을 쓴다. 큰 상자에 맞춘다고 이전 축소를1로 되돌리지 않는다. unordered 비교·배율1의 WORD 직독과 그 외 CRT signed64 절삭 하위DWORD를 보존한다.
- Draw는 현재 기준점/배율·SHP/mapped를 계산해 저장한 뒤, 색 표 설치 callback 이후의 현재 frame만 다시 읽는다. 일반은 좌표 DWORD 합이며 배율은 기준점×float+좌표를 합한 뒤 절삭하고16.16 배율을 만든다. 텍스트 override는 그림에 적용하지 않는다.
- MarkupAssets는 실제188타입/116자산·타입 기준점과 여섯 signed SHP WORD를 제공한다. Lookup/Frame은 논리 클러스터 수, VFX Shape는 실제 물리 프레임 범위를 사용한다. MarkupPixelTarget은 같은 GumpPixelTarget/현재256색 표에 원색·소스 치환·angle0 직교 배율을 출력하고 외부 회전 분배는 보존한다. 특별 빈 프레임은 해독/출력하지 않는다.

## 검사

새 **9/9 첫 통과**, 기존 스타일/대화상자/그림·Button/Stone/Gump/메뉴와 함께 **165/165 통과**다. 제품/검사의 실패 교정은 없다.

1. 공통/파생 토큰·metadata·mapped 하위BYTE, 배율/치환을 복사하지 않는 clone, Base 전환/Contains/직접 파생 거부·반납을 확인했다. 파생 토큰 중복에서 공통 등록만 제거하고 기존 파생 항목/다음 토큰을 보존한다.
2. 타입 callback이 바꾼 현재 frame과 헤더 callback 뒤 현재 타입 기준점/SHP를 명시 응답으로 공급하여 순서와 signed WORD 감김·헤더 값형 복사를 대조했다.
3. 무자산/음수·타입 수 경계·없는 SHP/프레임 범위에서는0 WORD/무출력이며 선택한 헤더/출력 장치만 요구한다.
4. baseline/폭·높이, 축소 유지·세로 float 저장·NaN/무한대 분기와 CRT 무효 결과를 검사했다. Draw의 저장한 위치/배율/SHP/mapped와 색 설치 뒤 바뀐 현재 frame·signed 좌표 감김을 구별했다.
5. 실제 FontSet/GumpText/글리프→같은 StyleText→그림 가상 기준선/측정/출력을 연결했다. 본문 글리프와 그림 기준선 배치를 대조하고 공통 Clear의 문자열→그림 Base 전환·기존 release 관찰→두 표 반납 순서를 확인했다.
6. 다른 외부 조각의 네 가상/fallback과 release를 각각 보존한다. release 실패는 Base/공통·그림 필드를 남겨 원인 추적 가능한 부분 상태를 유지한다.
7. 실제188타입·fortGump92/논리81·메타200×146/기준점0·여섯 헤더 WORD를 대조했다. dude70의 논리65/물리130에서 논리65는 거부하고 물리65는 허용하여 레이어 경계를 구별했다. 이들은 자산 메타 조회이며 게임 객체를 생성한 시험이 아니다.
8. 실제 StyleText→Markup→GameAssets/원색·치환·16.16 출력의 전체 픽셀을 직접 SHP 해독/기존 VFX 커널의 명시 입력과 대조했다. 현재 색 표 주소/내용·특별 빈 프레임·회전 fallback·clip/guard와 공통 Clear 뒤 두 표의 빈 상태를 확인했다. 별도의 원본 Markup 픽셀 fixture를 뜻하지 않는다.

원본 파일 변경·게임/브라우저 실행은 없다. 전체 Node v26.13.0·v22.23.3 각각 **4007/4007 통과**, 실패/취소/건너뜀0(395.8초/453.9초). 대응표 오류0, port677·partial52·todo167이다.

## 다음

StyleTextMarkup의 대괄호 type/frame/소리/색/태그·StyleTextBuffer/줄 배치·TextGump/ScrollingText와 파생 메뉴를 같은 저장소에 연결한다. RawMenuControls·ClientRendererFrame·GameWorld·ClientGameplayGlobals/Session·기존 앱 메뉴 교체가 남아 실제 미션은 아직 플레이할 수 없다.
