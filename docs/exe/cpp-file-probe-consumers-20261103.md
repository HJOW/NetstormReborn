# 765 — 실제 존재조회와 캡처·구름·배경 GIF 소비자

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일 HJOW-Athlon/2026-10-23 유지. 단위765 완료·전체 목표 미완료다.

## 존재조회와 실제 caller

41ab10은 내용을 읽는 부모와 다르다. 직접장치는5/0이며 실패시fallback하지 않는다. 상대이름은 기본TAFF2→기본disk1→보조disk3→보조TAFF4/0 순서다. 실제FileSpec은검색핸들을최종조회/예외까지소유하고Device가같은고리한칸을소비한다. 디스크에실제로도달할때만그시점의현재root를작성한다. metadata성공앞에불필요한root/CWD조회나fread를하지않는다.

실제VFS는가상게임CWD검색을제공하며호스트CWD는바꾸지않는다. 직접절대/UNC는root조회를하지않고같은drive의상대이름만현재게임root를쓴다. wildcard의선택이름은Device판정에만관여하며원래wildcard를fopen해실패하는원문동작을유지했다. 독립formatter-null값형API의literal판정은변경하지않았다.

SaveGifScreenshot(4db140),KencloudFile(470bb0),BackgroundGif(419ec0)의exists를내용TryRead대신같은Probe에연결했다. 캡처번호별Config재조회/계수와구름/배경의후속실제Read·decoder·그림반납은기존부모를쓴다. metadata존재는payload해독성공이아니다. 짧은GIF/해독예외를미발견으로숨기거나fallback으로바꾸지않는다. snapshot/optional/OS직접ReadFileBytes계약은유지했다.

## 검증과 최초 실패

원문640입력/두정밀도1280관측과root추가1280회가일치했다. 열가지이름·전체32출처조합·검색실패/성공·선행고리번호·첫Device칸/현재칸256BYTE·검색반납순서를고정했다. 실제Find/OS파일열기/TAFFmetadata는CPU의명시경계이며부모는payload/길이/키장치를호출할수없도록관측했다. 최초실제Device범위누락은fixture작성전에교정했고원시실패를보존했다.

첫C++빌드는새검사의BaseFile.h누락으로실패했다. 직접include로수정했으며최초build-v1.log를보존했다. 두번째는준비검사의비공개LoadFile접근으로실패해공개Build→실제LoadFile경로로고쳤다. 제품용검사getter/public확장은없다. 존재조회고리/실제payload/해독실패단언은유지하고미발견은기존원문Build의fallback할당까지범위를확장했다. 줄바꿈복원중캡처consumer삽입이사라진것도diff로찾아정확한SaveGifScreenshot행을재접합했다. 원문fixture와존재조회단언은유지했다. 최종build-v3는경고/오류0이다.

집중 **37개(새7개)/실패0**. 실제Unicode루트·TAFF/loose충돌·아카이브이동뒤metadata·보조root변경·wildcard와호스트CWD보존·현재공유고리·검색예외반납및기존파일/구름/GIF픽셀/캡처인코더회귀를확인했다. 실제Client캡처검사는두번째파일명Set에서명시중단하여화면잠금을하지않으며실제GUI캡처검증이아니다. 새실행대상0/기존66개·전체71대상이다.

기존보호 2683개불변·새fixture1 포함2684개/fixture864. 다음기준extracted/cpp-resume-765/protected-after.json,제품SHA-256 `d0ab5ee203a80d4abcc8589209a39469e6f9eb56654f1a51b912231e962c3d38`. [보고서](../../cpppj/file-probe-consumers-20261103.json)와같은extracted의최초/최종빌드·집중·native/native-verify를따른다. 실제세caller상위전체의CPU재현/32비트FILE/SEH전체·다른decoder부모동등성을주장하지않는다.

새게임/GUI/CLI/MP·물리/PCM·포획/희생·특정적공격·승리·자연완주·전체C++/34/ZIP검사는0이다. 다음은766 Help stream writer와각직접reader/남은일반명령이다.
