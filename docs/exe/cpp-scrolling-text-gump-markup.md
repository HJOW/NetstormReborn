# 다중 줄 TextGump 스타일 파서·버퍼 교체·설정

2026-10-13, **마지막 디컴파일 PC: HJOW-Athlon**. 현재 호스트/IP는 **HJOW-Athlon / 192.168.0.94**이며 앞선 정밀 프로젝트의 호스트와 일치한다. AGENTS.md와 두 LEFT_JOBS를 읽고 같은 PC의 두 PE 프로젝트에서 함수를 읽기 전용으로 내보냈다. 게임/창 실행은 사용자 허용 상태지만 이번 단계에서는 정적 분석과 빌드만 진행했다.

앞선 [다중 줄 생성/수명/입력](cpp-scrolling-text-gump-widgets.md), [표시/스크롤/선택](cpp-text-gump-display.md), [입력/표시 버퍼](cpp-style-text-buffer.md), [한 줄 대괄호 파서](cpp-style-text-markup.md)에 실제 다중 줄 입력과 추가 설정을 연결한다. **Client/UberGump의 기존 메뉴 어댑터와 raw 자동 GUI·건설/미션 완주는 미완료다.**

## 원본 근거

계획은 `tools/ghidra/scrollmarkup-functions.json`, 내보내기는 `extracted/scrollmarkup/{originals,originalCD}/creation.c`, 범위는 `functions.tsv`, 실제 PE 명령은 `assembly.txt`다. 정적 기록 도구는 `tools/analyze_scrollmarkup.py`, 결과는 `cpppj/recovery-scrollmarkup-evidence.json`이다. 원본 **10개**, CD **11개** 함수를 내보냈다. CD의 문자열 교체 보조 함수가 추가 한 개다.

| 기능 | 10.78 | CD 10.72 |
| --- | --- | --- |
| 다중 줄 파서 | 004ba8f0 | 004b6c80 |
| 기본 문맥/명시 문맥 문자열 추가 | 004ba090 / 004ba0f0 | 004b60a0 / 004b6100 |
| 그림 추가 | 004ba130 | 004b6140 |
| 선택/입력 허용 | 004ba180 | 004b6180 |
| 스크롤바 표시 | 004ba1b0 | 004b61a0 |
| 줄바꿈 | 004ba210 | 004b6200 |
| 스크롤 허용 | 004ba230 | 004b6220 |
| 일반 문자열 교체 | 004bc250 | 004b6050 → 004b5390 |
| 연결 버퍼/소유 교체 | 004bc3b0 | 004b64f0 |

이전 인수인계의 **W 명령은 CD 다중 줄에서 쓰는 소리 형식**이다. CD ROM `0053c408`에는 `W` 뒤 두 캡처를 요구하는 형식이 있다. CD 파서는 첫 캡처만 소리 이름으로 전달하고 두 번째는 사용하지 않는다. 목표 버전 10.78은 `S` 소리 형식과 zone 허용 옵션을 사용한다. W를 줄바꿈 명령으로 구현하지 않는다.

제한 그림의 높이 검사도 기계어로 확인했다. 패치는 `004babde`에서 헤더를 stack `+b0`에 복사하고 `004babe7`에서 `+ca` WORD를 16과 signed 비교한다. CD는 `+b8`에 복사하고 `004b6fd4`에서 `+d2` WORD를 12와 비교한다. 두 차이는 모두 **헤더 +1a, 원본 signed 높이**다. 보정 기준점이나 마지막 추가 WORD가 아니다.

## 구현과 연결

`ScrollingTextGumpMarkup`는 기존 `ScrollingTextGump`, `StyleText`, `Markup`, `StyleTextMarkup`과 생성/표시/입력의 같은 노드 저장소를 참조한다. 별도 조각 목록을 만들지 않는다. `StyleText::ApplyShortCommand`로 기존 한 줄 파서와 색/글꼴/스타일 명령 몸체를 공유하고, 대괄호 정규식/옵션도 `StyleTextMarkup`의 같은 처리기를 사용한다.

- 입력 시작의 기본 32바이트 문맥을 복사한다. `~.`와 `~n`은 명령 시점의 현재 기본 문맥을 다시 읽는다. 문자열 추가 통지가 기본 문맥을 바꾼 경우에도 이전 복사본으로 되돌리지 않는다.
- T 태그는 노드 **+c8**에 보관한다. 다음 Parse에도 유지되고 `~.`로 지워지지 않는다. 각 일반 구간/리터럴/그림 추가 직전에 현재 태그를 읽는다. 한 줄 StyleText의 파서 지역 태그와 구분한다.
- 명령 앞 문자열을 먼저 실제 버퍼에 추가하고 dirty/가상 Redraw(false)를 수행한다. `~~`도 길이 1의 같은 버퍼 입력이며 마지막 빈 입력도 onAppend/dirty/Redraw를 수행한다. 빈 입력은 새 문자열 조각을 만들지 않는 기존 버퍼 규칙을 따른다.
- LF/미완료 끝 줄/줄 수 제한/공백 줄바꿈은 기존 `StyleTextBuffer`가 처리한다. 그림도 같은 버퍼의 현재 끝 줄에 들어간다.
- 10.78은 `I→S→T→C→B→E`, CD는 `I→W→T` 우선순위다. CD W는 소리 허용 zone 옵션을 검사하지 않는다. 옵션 비활성/무효 그림도 해당 형식이 성립하면 다음 명령 형식을 검사하지 않는다.
- 10.78은 현재 +d4가 비0일 때 `~0/~1/~3/~4`를 소비만 한다. `~5/~6`은 변경하며 CD는 여섯 글꼴 명령을 모두 변경한다.

다중 줄 I 그림 식은 한 줄 식과 다른 규칙을 유지한다.

| 항목 | 다중 줄 동작 |
| --- | --- |
| 타입 식 | 정수 0이면 이름 검색, -1이면 생략. 패치 숫자 타입은 현재 타입 수 이상이면 생략 |
| 프레임 `*`/`#` | gumpFrame, -1이면 defaultFrame을 선택하고 첫 문자를 소비 |
| 글자 프레임 | 유효한 A~P 구간의 첫 인덱스에 suffix 숫자를 DWORD 덧셈 |
| 맞춤 | 소비한 뒤 suffix 첫 문자가 `#`일 때 기존 fitWidth/fitHeight 적용. `#` 하나는 맞춤하지 않으며 `##`/`*#`/유효 글자 뒤 `#`는 맞춤 |
| `&` | 색 치환 특수 접두로 처리하지 않음. 일반 숫자 해석/기본 mapped=0 |
| 프레임 상한 | count-1이 아닌 count까지 원본 클램프 |
| 현재 +d4 제한 | 타입/자산/프레임을 확인하고 실제 SHP signed 높이 ≤16(패치)/≤12(CD)일 때만 추가. 맞춤 99999×12 적용 |

`BindStyleTextMarkupAssets`는 실제 GameAssets의 전체 타입 수와 SHP `+1a` 높이를 공급한다. 기존 이름/논리 프레임/글자 표·팔레트·SoundPlayer·설정 연결을 재사용한다. +d4는 여기서 호출하는 함수 포인터가 아니라 원본처럼 0/비0 제한 조건으로 읽는 저장 필드다.

## 문자열·버퍼 교체와 설정

`ScrollingTextGump::ReplaceText`는 현재 기본 문맥을 먼저 복사하고 **완료 입력 줄만** 제거한 뒤 일반 문자열을 추가한다. 기존 미완료 끝 줄은 남는다. 스타일 명령 해석과 T 태그/스크롤 요청 초기화는 하지 않으며 이전 표시 복제는 다음 Layout까지 유지한다. CD는 같은 순서의 `004b5390`을 호출하고 패치는 해당 반복을 몸체에서 수행한다.

`ScrollingTextGumpWidgets::ReplaceBuffer`는 새 null을 거부한 뒤 현재 버퍼가 소유 상태이면 Destroy/free/포인터 0 처리하고 새 버퍼 토큰/소유 DWORD만 저장한다. 빌린 버퍼는 소멸하지 않는다. dirty/Redraw/태그/스크롤은 자동 갱신하지 않는다. 같은 소유 버퍼를 새 버퍼로 지정하면 원본처럼 먼저 소멸한 토큰을 다시 저장한다. 호스트에서는 이후 실제 접근 시 등록 없음 오류가 발생한다.

| 설정 API | 동작 |
| --- | --- |
| SetSelectable | 전체 값 비0이면 기본 입력 제외 bit 0x200 해제, 0이면 설정 |
| SetSlider | 최하위 비트로 textFlags bit 2 설정. 켜면서 scroll bit 4가 꺼져 있으면 bit 4도 켬. 기본 void bit 8이 없으면 가상 Push 후 현재 x/y로 자신을 Attach(parent 0, mode 0) |
| SetWrap | 현재 버퍼 noWrap=(value==0) 저장만 수행. dirty/Redraw 없음 |
| SetScrollable | 최하위 비트로 textFlags bit 4 설정. 끄면서 slider가 켜져 있으면 SetSlider(0) 호출 |

각 처리기는 실제 가상 분배/버퍼 저장소와 연결해 호출해야 한다. 상위 Client의 전체 UI 조립과 자동 파서 호출을 완료했다는 의미는 아니다.

## 빌드와 남은 작업

라이브러리 `netstorm`과 실행 파일 `NetstormCpp`의 **Release 최종 빌드 경고 0·오류 0**. SOURCE_MAP을 생성했으며 내용 차이는 없다. **새 테스트 추가/실행·전체 CTest·독립 원본/C++ 파서/콜백/소유/설정/픽셀 대조·게임/GUI 실행은 미수행**이다. 이번 원본 실행 관찰과 C++ 비교 추가는 각각 **0개**다. 기존 관찰 **523,634개**, C++ 대조 **523,374개**, CTest **854개 통과**는 앞 단계 기록이다.

남은 작업은 독립 원본/C++ 동작 대조, 그림 색 표/회전 VFX/flag 2 재사용, Form/메뉴 생성/장식/제목/수명/Attach, OS 입력 큐/폴링/overlay와 raw Client UI 조립이다. 실제 사건/타입 전역·allocator·커서 공급, 생성 EH 정리와 극단 x87 대조도 남았다. 입력 원본 포인터의 바이트를 콜백에서 바꾸는 alias, 80바이트 이상 프레임 지역 배열 쓰기, 끝 `~`의 버퍼 밖 읽기는 호스트에서 재현하지 않는다.

보호 두 PE SHA-256은 불변이다. AGENTS.md/보호 원본/dotnetpj/테스트 파일을 수정하지 않았고 커밋/푸시는 수행하지 않았다.
