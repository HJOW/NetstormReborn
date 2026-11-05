# 언어 변경·폰트 재생성 경로 재대조 (단위931)

2026-11-05, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 PC의 2026-11-03 자료이므로 재디컴파일 없이 `extracted/refined/originals/Netstorm.c`와 `raw.c`를 읽었다. **정적 재대조만 수행했다.** 제품은 수정하지 않았고 새 빌드·검사·게임 실행도 없다. 제품은 927/11.01 그대로다.

## 원문 참조 조사

- `languageChanged` 토큰 `0054298c`는 `Netstorm.c`와 `raw.c` 모두 정확히 2곳에서만 참조된다. 송신은 `004dea70`(언어 요청), 수신은 Chat 처리기 `0042fab0`의 한 분기뿐이다. 다른 Gump나 메인 디스패처는 이 토큰을 소비하지 않는다.
- `004dea70`은 선택 번호가 `005435b4`와 다를 때만 `currentLanguage` 설정을 쓰고 `00452fe0`으로 `languageChanged`를 큐에 넣은 뒤 `005c85d8`(MetaRefresh)을 1 올린다. 같은 번호면 아무것도 하지 않는다. C++ `RequestOriginalLanguage`가 같은 순서다(단위710/712).
- Chat 수신 분기는 가상 `+0x18` 호출 뒤 `+0x20(this+0x4c, this+0x50, 0, 0)`을 호출한다. C++ `UberGump`의 `DispatchLanguageChanged`가 같은 Chat Push/Attach를 한 번만 실행한다(단위787).
- 폰트 재생성은 언어 변경에 묶여 있지 않다. 메인 디스패처 `004d62b0`의 `ChangeFontFace`(`00542764`)만 `004a2df0`(재생성)→`004a39f0`→`00499370`(전체 무효화, 폭/높이 순서 읽기)→`004a2e30`→`fontFaceName` 쓰기를 수행한다. C++ `DispatchPresentationCommandInput`/`RebuildPresentationFonts`가 같은 순서이며 호스트 표시 캐시 반납만 추가한 호환 처리다. 따라서 언어를 바꿔도 이미 생성된 글꼴/UI는 다시 만들지 않는 것이 원문 동작이다.
- 시작 용어표 `004dea00`의 German/English/Portuguese 세 장부, `004dea40`/`00441db0`의 번호→`LanguageSpec` 재적재, `004deab0`의 초기 `-1` 전환 순서는 단위710에서 이미 구현했다.

## 결론과 남는 범위

Edit 2번의 "원문 언어 UI/폰트 재생성" 항목은 새 구현 없이 닫는다. 남는 것은 6차 한국어(문자열/글꼴 추가), `004dea00`이 다루지 않는 French 등 추가 용어표의 의도적 부재 유지, 실제 OS 메모리 고갈 같은 호스트 경계다. 이 재대조를 새 실행 검증이나 전체 언어 UI 동일성 완료로 확대하지 않는다.
