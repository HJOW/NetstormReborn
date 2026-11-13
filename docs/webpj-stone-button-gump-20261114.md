# webpj 실제 돌 버튼 조립·출력 (WB04-E417, 2026-11-14)

RawMenuControls 선행 **StoneButtonGump 258줄(.cpp/.h, partial→port)**을 완료했다. 기준선 `1f00a25a` 전체 소스와 GumpTextTests의 실제 부모 측정/라벨/할당 실패/Draw/배경·수명 시험을 읽었다. 두 소스의 기준선 뒤 변경은 없다. CD/추가판은 웹10.78 범위 밖이다. [E414](webpj-gump-redraw-text-help-20261114.md)에서 이월한 실제 StoneButton 부모 시험도 이번 단위에서 완료했다.

## 같은 부모와 자식

- 같은 ButtonGump/GumpCreate의 실제 노드에64칸 라벨·세32BYTE 문맥·두 측정값을 붙였다. 생성자는 세 문맥의 앞 네DWORD만 초기화하고 extra·비활성 shadow 필드·라벨 마지막 바이트·측정값은 보존한다. 라벨 null은 첫 바이트만0, 나머지는 strncpy63 규칙이며 마지막 바이트를 임의 종료하지 않는다.
- 기본 측정 문맥과 정상/눌림/비활성 Draw 문맥을 구별한다. BindStoneButtonText는 실제 GumpText/FontSet과 현재 폭/높이 출력 별칭을 연결한다. SetLabel은 측정만 하고 부모/배경 크기·재그리기를 추가하지 않는다.
- FitToText는 측정 뒤 현재 출력/scale을 읽고 DWORD +12/+6 감김 뒤 각 float 저장 지점을 유지한다. Attach는 가상 배경 Push 뒤 현재 background/부모 기본 크기를 읽고 배경 크기-2/부착을 마친 뒤 실제 기본 GumpTree.Attach에 위임한다. 산술로 새로 쓴 크기는 원문 직접 복사의 이전 신호NaN 메타를 재사용하지 않는다.
- 배경 new0xb8·실제 GumpBackgroundCreate/Tree를 사용한다. 할당 실패는 이미 등록한 부모/라벨/필드를 되돌리지 않고 원본 null 접근에 대응하는 호스트 오류를 남긴다. 자식 배경은 실제 공통 Destroy에서 먼저 해제하며 scalar가 중복 free하지 않는다.
- DrawPlan은 선택 문맥을 값으로 복사한다. 실제 Draw는 부모 선→현재 Node/Position→현재 라벨과 선택 문맥으로 GumpTextDraw/PixelTarget에 출력한다. 자식 배경은 같은 수집/정렬의 별도 kind2 항목이다. 가상 Attach/소멸자/Draw의 다른 타입 fallback을 보존한다.

기존 웹 UTF16/UnicodeFontProvider 정책도 명시 저장소 경계에서 유지한다. 기본은 원문64BYTE이며 UTF16 라벨은 생성 전에64칸 Uint16Array를 공급해야 한다. 바이트 저장소로 한국어를 묵시 축소하지 않는다. 기존 앱의 간소화 StoneButton은 이번에 바꾸지 않았고 실제 RawMenuControls 조립 때 교체한다.

## 검사

새 **10/10 첫 통과**, 주변 Button/Gump/메인 메뉴 **103/103 통과**다. 최초7개 이후 현재 값/Unicode/도움말·산술 비트 검사를 더했으며 실패 교정은 없다. 부모의 별도 x86 생성 fixture가 있는 것은 아니다. 원문 측정 fixture의 Font 입력/Ab 관측과 CPP의 독립 SHP/글리프/부모 연결 기대값을 사용했다.

1. 원문 폭 표/캐시→실제 BitmapFont/FontSet→같은 StoneButton 측정→Fit→가상 자식 재부착→같은 GumpLifetime Kill/Cleanup을 연결했다. 문맥/라벨/측정값 sentinel 보존, 빈 글자 그림자 여백, 부모13×21·배경11×19·자식 깊이와 해제3→2/등록 루트 보존을 확인했다.
2. null/짧은/긴 라벨의63칸 복사와 끝 바이트 보존, 원문 Ab 폭, 측정의 NUL 범위 실패에서 앞선 출력값 보존을 확인했다. 배경 할당 실패도 등록된 부모·라벨/command/background0을 보존한다.
3. 실제 한 픽셀 캐시를 사용하는 수집→정렬/kind2→가상 Draw→clip/색/글리프를 연결했다. 정상 흰 잉크243·그림자9·선57, 눌림의(1,1) 이동·비활성 문맥·한 픽셀 dirty·guard와 현재 색 표가 CPP의 명시 기대값과 같다. 기본 문맥을 빈 슬롯2로 바꿔도 Draw의 선택 문맥0을 쓴다.
4. 라벨 Draw 범위 실패에서도 먼저 출력한 선과 원래 clip을 보존하고 색 표는 아직 변경하지 않는지 확인했다.
5. 실제 독립 SHP 자산→자식 배경 생성/부착→공통 수집/정렬→배경/글자 출력→실제 Button.SetPressed→함께 바뀌는 명암/글자 위치→자식 우선 해제를 연결했다. 일반 픽셀4·테두리14/22→24/12와 글리프/현재 공유 색, 해제 뒤 수집0·루트만 등록된 상태를 대조했다.
6. 현재 측정 callback이 바꾼 출력/배율·signed 크기 감김, 가상 fallback과 빠진/잘못된 장치·라벨 소스 범위를 확인했다.
7. UTF16 라벨을 실제 UnicodeFontProvider에 명시 래스터라이저 응답으로 공급하여 측정/픽셀·guard와 바이트 저장소 거부를 검사했다. 실제 브라우저 D2Coding 렌더링을 실행한 시험은 아니다.
8. 실제 Button→GumpHelp 몸체를 연결해 현재 주제/루트와 도움말 callback 순서·opengump.wav를 확인했다. 실제 메뉴 UI 공장은 후속이다. 산술 float의 이전 직접 복사 비트 무효화도 검사했다.
9. 가상 Push가 바꾼 현재 배경/부모 기본 크기·배율을 실제 트리에 적용하고 부모 선 callback 뒤 현재 라벨/문맥/Position과 값형 Draw 문맥 보존을 구별했다.

원본 파일 변경·게임/브라우저 실행은 없다. 전체 Node v26.13.0·v22.23.3 각각 **3983/3983 통과**, 실패/취소/건너뜀0(354.6초/384.5초). 대응표 오류0, port674·partial54·todo168이다.

## 다음

StyleText·TextGump·ScrollingText와 파생 메뉴 처리기를 같은 저장소에 이식한 뒤 RawMenuControls를 조립한다. ClientRendererFrame·GameWorld·ClientGameplayGlobals·Session/앱 연결도 남아 실제 미션은 아직 플레이할 수 없다.
