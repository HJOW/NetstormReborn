# webpj HTML 전체 창 생성 (WB04-E459, 2026-11-14)

기준선 `1f00a25a`의 `HtmlWindowFactory.cpp/.h`를 `www/ui/html-window-factory.js`로 온전히 이식했다. 기존 `html-text.js`/`tell-window.js`의 부분 대응은 새 공장 모듈로 교체했다. 장기 HtmlWindowGumpState와 실제 생성/현재 노드·Config/파서/메타·본문/가상부착·모달/수명을 빌린다.

생성 또는 suppress/할당0, 현재 createdSerial, Config 치환, 첫8192BYTE 변환, ASCII 내용 관문, 빈 본문 xxx→부착→Kill, 벽시각→현재 메타 버퍼→필요한 기본 버튼, 현재 본문의 LF→BR, 같은 파서의 두 번째 변환과 마지막LF, 현재 화면/부모/크기, 부착→모달 뒤 현재serial 순서를 유지했다. 생성 때의 serial 캐시를 반환값으로 재사용하지 않는다. 두 파서는 같은 조건 상태를 공유하며 부모 링크 저장소를 별도로 초기화하지 않는다.

## 기준선 뒤 판정

변경은 `1d5b1f437` 한 건이다. ASCII가 없는 경우 선택하는 `localizedContent`는 **② 공통 플랫폼 관문으로 반영**했다(base `1d5b1f43`). 빈 callback인 원본 모드는 ASCII 관문을 유지한다. 현행 C++의 `RawMenuHtml.inc:28`은 `extendedText`와 사설4BYTE 판별기를 공급한다. 그 표시 부호화 자체는 E269 정책대로 **① 웹 제외**이며 웹 UTF16/D2Coding 장치의 연결은 후속이다. 원문 BYTE 계약에 인코딩되지 않은 한글을 낮은BYTE로 잘라 넣지 않고 명시 실패한다. 이 선택 관문 이식으로 한국어 HTML 전체 연결을 완료했다고 표시하지 않는다.

## 검증

집중 **6건 첫 실행 통과**. `cpppj/tests/fixtures/html-window-factory-x86.tsv` **원본10.78 106행**의 전역·현재부모serial·8192BYTE 링크 저장소·첫/둘째 출력·반환/중단·효과 순서가 모두 일치했다. 원본 상위 전체 생성 관측이며 생성/파서/메타 하위는 명시 입력 장치다. manifest `html-window-factory`로 연결했고 기존 fixture를 수정하거나 다시 실행하지 않았다.

독립 입력은 suppress/할당0의 첫 파서 효과, 두 파서의 같은 조건 상태, 첫 메타가 바꾼 현재 본문, 링크 NUL 뒤 꼬리, 모달 뒤 현재serial과 생성serial 구별, localizedContent의 자연 단락, UTF16 경계, 첫 지역8192BYTE와 마지막LF/NUL 한계다. 현재 `parseHtml`·공유 파서 전역과 실제 HtmlWindowMetadata를 연결해 기본 OK 버튼과 본문 변환도 확인했다. 이 검사에서 버튼 생성/본문/부착은 명시 하위 장치이므로 실제 Gump 공장/원본 글꼴 픽셀까지 검증한 것은 아니다. Gump/Decoration/수명과 Fit의 실제 조립 검사는 E456/E457에 있고 전체 소비자 결선은 허브 몫이다.

전체 회귀는 Node v26.13.0·v22.23.3에서 각각 **4,745건 통과, 실패0**, 대응표984행 오류0이다. Node v24는 이 PC에 없어 실행하지 못했다. 앱은 기존 TellWindow를 사용한다. 실제 Client/RawMenuControls의 파서·Config·부모·기본버튼·모달/전체Fit과 이 공장을 같은 수명에 연결해야 한다. 보호 원본/디컴파일/cpppj 파일을 수정하거나 게임/브라우저를 실행하지 않았다. 실제 미션 플레이는 아직 불가다.

다음은 HtmlTextParser의 현재 필드/공유80BYTE 배경·가상 callback과 TellSection의 실제256BYTE 현재이름/설정 등록부를 마무리한 뒤 RawMenuControls 허브로 이어간다.

## 사용자 요청에 따른 일시 중지와 재개 인계

사용자가 2026-11-14에 현재 단위 마무리 뒤 문서·커밋과 목표 일시 중지를 요청했다. **E459까지 마감하고 다음 단위는 재개 지시 전 진행하지 않는다.** 전체 목표와 실제 미션 플레이는 미완료다.

다음 HtmlTextParser는 소스 사전 대조만 진행했다. 기존 `html-text.js`의 parseHtml은 실제 효과 누락을 조용히 넘기고, callback 뒤 cursor/출력 물리 검증과 공유80BYTE 배경 꼬리 보존이 빠져 있다. 기준선 C++의 `HtmlTextParserTests` 전체를 기준으로 새 원문 BYTE 처리기를 먼저 검증해야 한다. 기본 fixture는 `htmltext-x86.tsv`의 10.78 행, 그림은 `htmlpicture-x86.tsv`의 10.78 행, 배경은 `html-background-x86.tsv` 648행이다. CD10.72 행은 웹 범위 밖이다. 같은 StyleText 조각으로 전달하는 C++ 통합 검사도 포함한다.

ClientHtmlWindowGlobals의 `text`는 현재 기존 `createParserGlobals()` 객체다. 새 원문 처리기를 옮길 때 실제80BYTE 배경 저장소와 표시 이름 별칭을 공유하도록 바꾸고 관련 owner 검사를 함께 갱신한다. 앱의 기존 Unicode 경로는 RawMenuControls 결선 전까지 유지한다. 기준선 뒤 `1d5b1f437`의 사설4BYTE extendedText는 E269 정책에 따라 ① 웹 제외로 판정할 대상이다.

**재개(2026-11-14):** 사용자의 `/goal` 재개 지시로 [E460 HtmlTextParser](webpj-html-text-parser-20261114.md)를 이어서 마쳤다(예비 초안은 쓰지 않고 기준선과 현재 계약을 다시 대조해 새로 작성).

이 컴퓨터(HJOW-Athlon)의 `%TEMP%/netstorm-e460-html-text-parser.js`에 **미검증 예비 초안**만 있다. 저장소에 반영하거나 검사·커밋하지 않았다. 재개 때 초안의 존재를 전제로 삼지 말고 기준선과 현재 계약을 다시 대조한 뒤 사용한다. 인수인계의 기준은 이 문서와 커밋된 소스다.
