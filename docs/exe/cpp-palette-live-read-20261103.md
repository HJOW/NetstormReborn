# 현재 팔레트 File과 직접 저장 칸 기록

단위786은785(`e4b1b472`) 뒤의 활성 비멀티 파일 부모 복원이다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 현재 호스트와 같다. AGENTS.md·보호 원본·기존 fixture와 추출물은 바꾸지 않는다.

## 원문과 기존 입구 차이

4a4850은41b4c0(mode0/create1)로 File을 열고776바이트이면Seek8 뒤 RGB각1BYTE를256회 반복해 saved_의 BGRX칸에 직접 쓴다. 예약BYTE는 유지한다. 그 외에는 길이를 다시 조회해1024바이트일 때 한 번에 직접 읽고, 다른 길이는Screen.cpp940진단이다. 진단이 복귀하면 기존 saved_로 후단을 계속한다. loaded1/clear/Apply/색 목록·58색 검색·weather복사 뒤에만 File을 닫는다. 읽기/후단 실패 때4fb3f0 funclet이 같은 File만 정리하고 앞선 팔레트/화면 효과는 남긴다.

기존 Client 입구는 Files.Read snapshot→GamePalette 전체 파싱→Screen.LoadPalette 순서였다. 전체 파싱 전에는 saved_가 바뀌지 않고 파일 수명도 후단을 포함하지 않았다. 새10.78 입구는 현재 FILE/키와 직접 saved_ 기록을 연결한다. 기존 GamePalette/값형 LoadPalette/CD는 유지하며 기존 Screen 후단은 ApplySavedPalette로 추출해 그대로 공유한다. 일반 whole read의 최소4/추가NUL/0fill을 넣거나 RGB를768BYTE 묶음으로 바꾸지 않는다.

기존 Screen 대응표의CD10.72 함수는00424760이다. 이번 새 CPU 관측/FILE 입구의 대상은10.78의004a4850이며 CD 새 관측·파일 부모 전체 복원으로 합산하지 않는다.

## 최초 원문 관측

독립 CPU92입력×두x87정밀도=184회와 새 프로세스의184회가 바이트 일치했다. 최초fixture SHA256은 `b9a765cbba5b7757bd74b5cf84fc637ad3dd17106088a22e6763cb6055004913`이다. 실제4a4850·41aa80·41a700·41a7a0·41b410·4fb3f0을 실행한다. Open/CRT·OS표면/표시·색검색·목록할당은 명시 장치다. 후단의58색검색까지 File이 열린 조건을 확인하지만 실제 색검색 픽셀/OS출력과 전체32비트SEH dispatcher를 관측한 것은 아니다.

정상/부분읽기·키/부분파일·clear/깊이·Open/읽기/적용/닫기 실패·잘못된길이의진단복귀를 구별한다. 같은 암호 작업 물리는 명시0으로 한 번만 시드하며 후속1BYTE Read에서는 이전BYTE를 보존한다. 이는 원문의 미정 stack 자체와 같다는 주장이 아니다.

최초 도구의 `file==PLAYERS && count==1` 조건이 가상FILE0에서 중단됐다. count 제한을 풀어도 같은 FILE0 조건에서 중단됐고, 진단 프로브에서 `(file0,size1,count1,length776,key0,sub0,scenario2)`를 확인했다. 해당 스트레스 입력은 가상CRT의 nullRead 명시 실패로 기록한다. 실제 fread(NULL) OS 동작이나 성공을 합성하지 않는다. 최초두실패·진단·최종관측로그는 `extracted/cpp-resume-786/`에 보존하며 제품/C++검사실패와 구별한다.

**원인 설명 정정:** 최초에 암호1024읽기를 size1/count1024로 추정했던 설명은 잘못됐다. 보존된92행의 정상 fread 관측은1/1과1024/1뿐이며, 암호1024행도 `R:1024:1:1024`다. 이 정정은 도구 실패 원인 설명에만 해당한다. 최초fixture·C++제품·12개 통과 결과와 해시는 바꾸지 않았다.

## C++ 연결과 검증 범위

ReadPaletteFile은 같은1024BYTE와 RawBaseFileState를 빌린다. 완료된 Open 이후만 원문식 File 정리를 수행하며 정상 소멸 자체의 실패에서 다시 소멸하지 않는다. 실제 reader는 Unicode CRT와 VFS 현재 metadata/backing FILE을 사용한다. 미완성 Open의 OS 자원 추가 회수는 현대RAII 경계다.

Screen의 직접 입구는 현재 saved_ 바이트를 제공하고 같은 기존 clear/SetPalette/BuildPaletteColorTable/shade 후단을 File 안에서 완료한다. Client는Patch1078에만 이를 선택하고CD의값형계약은유지한다. 새 단위 검사3개와 기존 회귀는 아래 최종 확인값을 따른다.

Native92행 C++대조는 File여섯필드·saved1024BYTE·reader효과열/실패접두를검사한다. fixture의별도Native화면전역/색검색횟수열을reader검증으로합산하지않는다. 실제VFS검사는등록뒤변경한backing1024/기존snapshot보존·looseRGB의예약BYTE·필수missing/길이실패와후속재읽기를확인한다. 기존clear집중2개를현재파일대상에재사용하며새실행대상0이다.

현재 단계에서는 새게임·GUI·PCM/실제CD·공중3종·공격/포획/희생/승리·자연완주·멀티플레이검사를 실행하지 않았다. 전체목표와checking잔여50은미완료다. 완료값과 다음 보호기준은 아래 최종 확인값을 따른다.

## 최종 확인값

게임/파일집중대상Release빌드는경고/오류0이다. 새Native92·현재TAFF/looseRGB·missing/길이실패3개와기존clear2/GIF4/title3, 총12개실패0이다. reader검사와화면전역/실제색검색·OS출력을구별한다. 기존Screen후단은줄끝정규화외동일함을대조했고CD/값형API도유지한다.

EXE SHA256은 `55400bc43a9bd5d08f070901226f1f92c918c4491670525738bb8d0c260baba2`이다. 기존보호2720개불변·새fixture포함2721개/fixture901이며다음기준은 `extracted/cpp-resume-786/protected-after.json`이다. [보고서](../../cpppj/palette-live-read-20261103.json)와해당원시폴더를따른다. 새게임/MP/전체71대상/전체C++/34경로/ZIP·지정플레이/물리/PCM검사는추가하지않았다. 다음787은Xlat현재File·버퍼설치·CR정규화뒤닫기/파서분리이며전체목표는진행중이다.
