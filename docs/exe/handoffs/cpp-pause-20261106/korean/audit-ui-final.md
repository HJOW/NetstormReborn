# 한국어 UI 준비본 최종 읽기 검토

2026-11-06 / HJOW-Athlon. 기존 `cpp-korean-ui-preparation/pending`을 읽었으며 제품/기존 pending/final-pending/CMake는 수정하지 않았다. 빌드/실행0이다. 현재 주작업은 루트가 맡긴 8차 Scenario 통합이며 이 문서는 한국어 통합 담당에게 알려 주는 읽기 전용 검토다.

## 적용 전에 고칠 실제 경계

1. `pending/cpppj/src/client/TextEntryBuffer.cpp:137`은 incoming에 한글이 있을 때만 새 문자 경계/상한을 적용한다. 기존 본문이 한글인데 긴 ASCII clipboard를 붙이면 원래 Paste 분기로 들어가 `node.maxLength`를 넘길 수 있다. Chat은 80BYTE UI 상한과 128BYTE CopyInput 지역이 있으므로 적어도 다음처럼 분기를 넓혀야 한다.

```cpp
if (HasExtendedDisplayText(incoming) || HasExtendedDisplayText(Text())) {
    // 기존 준비본의 경계/선택 차감/최종 BYTE상한 코드를 그대로 사용한다.
}
```

새 검사는 한글 본문에 ASCII255BYTE 붙여넣기와 선택 교체/0남은상한을 추가한다. 순수 ASCII 본문+순수 ASCII 입력의 원문 Native fixture는 그대로 유지할 수 있다.

2. `TextEntryGump.cpp:187~202`는 Buffer 선택만 보정하고 `node.shiftAnchor/shiftEnd`는 그대로 둔다. 외부 선택이 비어 있지 않고 anchor가 한글 중간이면 Shift+Left가 다시 부분 선택을 만들 수 있다. 예를 들어 본문 `한A`, 선택0..4, anchor2/end2 상태에서 Shift+Left는 0..2를 만든다. extended 분기에서 현재 본문 경계로 두 필드도 보정하거나 Shift 선택 직후 Buffer 선택을 다시 floor/ceil로 확정해야 한다. 기존 순수 BYTE는 그대로 분기한다. 새 검사는 비정상 외부 선택+ShiftLeft/Right/Home/End 뒤 모든 선택 끝이 0/4/5 같은 유효 경계인지 확인한다.

3. LanguageChanged에서 메인 메뉴 `RebuildControls()`는 기존 초기 준비본에는 없다. 본문 담당의 Help/파일메뉴 캐시 반납 hunk 뒤 `if (state_.phase==ClientPhase::MainMenu) RebuildControls();`를 병합해야 기존Main 부모의 SetLabel/재측정 hunk가 실제 실행된다. 메인 부모/45초 deadline 재생성은 피한다.

## English UI에서 한글 입력 정책

`MakeCommittedTextEvents`/`QueueCommittedText`는 UI언어에 제한을 두지 않고, Client의 D2Coding 확장 face 설정도 언어와 무관하다. 따라서 English 선택 상태에서도 확정 한글을 TextEntry/Chat에서 표시할 수 있다. 반면 HTML Parse의 `extendedText=(언어7)`이면 Name 입력 후 Tell 제목/본문으로 돌아올 때 같은 토큰이 삭제된다. 번역 선택과 유효한 표시 문자 보존이 서로 다른 기능이다.

권장안은 **실제 클론 Client가 생성하는 HTML 표시 경계에서는 유효4BYTE 토큰 보존을 항상 켜고**, 원문/독립 parser fixture의 기본false는 유지하는 것이다. Client의 `HtmlTextParserGlobals.extendedText=true`를 한 번/매 해석에 설정하고 HasContent도 같은 flag를 읽으면 English 원문 문자열/번역 선택은 그대로이며 IME/UTF8 사용자 문자열만 자연스럽게 보존한다. 또는 명시 per-source `EncodeDisplayText` 결과 허용을 전달해도 되지만 이미 많은 HTML/메타 재생성 경계를 지나는 Name에는 전역 UI언어 조건만으로 부족하다. 어느 정책이든 English+한글 Name/HTML input, Korean+한글, 언어 전환 후 기존 한글 로그/도움말 제목의 표시를 검사해야 한다.

기존 `o::DecodeExtendedSequence`만 유효성을 판정한다. 다른 ValidToken 함수를 만들 필요가 없다. `HasExtendedDisplayText`는 정확한 토큰 유무, parser의 offset+3은 같은 함수가 성공했을 때만 적용한다. ASCII HasContent가 한글만 있는 본문을 비었다고 판정하는 경계도 함께 켜야 한다.

## 남은 정적·실제 확인

- 현재 새 C++8검사는 컴파일 전이다. 이 읽기에서 새로운 확정 C++ 구문 오류는 발견하지 않았으며 컴파일 성공 주장이 아니다.
- 새 helper/헤더의 public API는 본문 담당의 HasContent callback과 충돌하지 않는다. core에 DisplayTextInput.cpp, Windows imm32 링크, 검사 대상의 실제 의존성을 등록해야 한다.
- IME의 조합 종료 Enter/Backspace는 GetMessageTime+scan/repeat로 동일 WM_CHAR를 판정하는 정적 설계다. 실제 TSF/IME 지연 메시지 순서와 기본 후보 창 위치는 루트가 확인해야 한다.
- Unicode clipboard는 실제 장치만 확장하고 Native CF_TEXT 기본 장치는 그대로다. unsupported Unicode를 빈 결과로 거부하는 검사 외에 ASCII fallback 및 OS실패 cleanup은 추가 확인 대상이다.
- Save as Fort는 원문 ASCII/공백 파일명 정책을 유지한다. 한글 프로필/Chat/HTML Name 지원과 파일명 Unicode 지원을 합산하지 않는다.
- 새 raw 문자열 기대값을 CHECK 매크로 인자로 직접 넣지 않는다. 983의 전통 MSVC 전처리기 문제를 반복하지 않도록 지역 상수에 둔다.
