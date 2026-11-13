# webpj 실제 커서 전역의 수집·표시 연결 (WB04-E404, 2026-11-14)

5-A의 E386·E387 이월분인 `PreviewCollection.bindCursor`·`PreviewDraw.bindCursor`·`makeCursorPreviewAssetShapes`와 `CursorPreviewSource.h`를 이식했다. 기준선 `1f00a25a`의 Source20줄·Collection214줄·Draw363줄(각 본문/헤더 합계)이며 모두 기준선 뒤 변경이 없다. Source는 `render/cursor-preview-source.js`, 나머지는 기존 모듈을 확장했다. Collection/Draw는 partial→port다.

## 실제 참조와 수명

- Source는 같은 PATCH 풀만 받으며 연결 순간의 Drop/Lifecycle/Placement/표면 TYPE/실제 Canon 격자 참조를 고정한다. 전역 필드의 getter는 호출 중 바뀐 현재 값을 다시 읽는다. 별도 근사 월드나 커서 사본을 만들지 않는다.
- 수집은 Drop의 잠금/소유자/좌표, Lifecycle의 abstract TYPE/부모/현재 count/30DWORD, Placement의 **표시 배치 허용**을 읽는다. 최종 `drop.legal`과 구별한다. SP 표시 피드백과 설정/장치는 기존 state/hooks에 남는다.
- 표시는 같은 표시 판정/현재 부모/소유자와 격자·설명·경고/비용 치수·카메라 원점을 사용한다. 그림 종류·팔레트·옵션·문자 프레임·색표 존재 목록은 기존 Draw 상태의 현재 값을 읽는다. 원본 좁은 문자열은 문자 BYTE 번호로 기존 라벨 순회에 연결한다.
- 비용 그림 공급자는 GameAssets 생성자가 보장하는10.78 저장소를 받는다. 일반 TYPE는 기존 블록 연결을 사용하고 비용 그림은 매번 현재 Lifecycle의 block+1 토큰을 읽어0/상한 초과를 거부한다.
- 같은 원본 수집/Draw 본체를 그대로 사용한다. 출력 경계 뒤 현재 상태 재조회와 앞서 저장한 위치/깊이/화면 좌표의 구별을 유지한다.

## 검사

기존 두 검사 파일에 새5건을 추가하고 공통 픽셀 시험을 독립/공유 두 모드로 확장했다. 집중 **26/26 통과**:

1. `PreviewCollection_PatchNativeSharedCursor`: 원본 **520행**을 실제30DWORD/표시 전역에 공급하고 중복 독립 입력은 잠금1/무효 TYPE/빈 목록으로 바꿨다. 반환·작업64칸 전체 바이트·SP/강조·보호 항목과 풀/spot 불변이 모두 같다.
2. `PreviewCollection_SharedCursorCurrentReadsAndContracts`: SP 조회가 첫 SID5→6/count2→1·raw 위치·TYPE 깊이를 바꿔도 본체 토큰250/보조300과 저장한 위치20,25·깊이7,6을 유지한다. 최종 내려놓기 불가/표시 가능의 분리와 다른 풀/판본 거부를 확인했다.
3. `PreviewDraw_PatchNativeSharedCursor`: 원본 **477행**을 현재 커서에 공급하고 중복 독립 부모/허용/소유자는 반대로 둬도 모든 출력 사건과 raw 불변이 같다.
4. `PreviewDraw_SharedCursorCurrentReadsAndContracts`: 첫 격자 출력이 실제 격자를 지우고 부모를 바꾸면 사각형1개 뒤 문구/그림은 없다. 본체 윤곽 효과가 표시 허용·선택 TYPE·소유자를 바꾼 뒤 현재 파괴 색/소유자3을 읽고 색 선택 뒤 소유자4로 그림을 출력한다. 다른 풀/판본도 거부한다.
5. `PreviewDraw_CurrentCursorProtectedAssetBinding`: 실제 자산·Lifecycle 비용 이미지 어댑터로92→123 TYPE 비용 블록 변경·일반 TYPE·0/상한 초과를 대조했다.
6. `PreviewDraw_CommonCollectionDispatch`: 독립/공유 두 경로 모두 실제 수집→CRT 정렬→kind4 분배→VFX 픽셀을 거쳐 투명9·원색0/255를 같은 세 위치에 출력한다.

E403 전체 회귀를 기다리는 동안 초안은 무시된 임시 디렉토리에서만 만들었다. 격리 실행기 import 경로 재작성 도구의 첫 실행 문법 오류를 교정한 뒤 초안26/26, E403 커밋 후 실제 경로 적용 첫 집중도26/26 통과했다. 제품 코드의 원본 대조 실패나 기대값 변경은 없다. 첫 대응표 검사에서 복수 모듈 구분자를 쉼표로 쓴 등록 오류4건을 도구 규칙의 세미콜론으로 교정했다.

기존 fixture 두 항목에 공유 Source 모듈/근거 헤더와 현재 단위 문서만 연결했다. 원본 자료 바이트 변경·새 게임/브라우저 실행은 없다. 전체 Node v26.13.0·v22.23.3 각각 **3883/3883 통과**, 실패/취소/건너뜀0(317.8초/352.2초). 대응표 오류0, port654·partial59·todo183이다.

## 남은 경계

다음은 표시기가 직접 요구하는 `GumpDraw`(todo)·`GumpTextDraw`(partial)의 실제 클립/글자 출력 경계를 먼저 마무리한다. 그 뒤 이 부품을 실제 파일 팔레트·OwnerColor·SP·시간·번역/비트맵 글꼴·Renderer dirty 표면에 연결하는 `RawCampaignCursorDisplay`다. ClientRendererFrame·ClientGameplayGlobals·실제 Session/앱 수명 조립과 통합 시험은 아직 미완료다.
