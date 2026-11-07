# 1096 — 메타 원근 투영·클리핑과 Client 수명

제품1096 / 표시·파일·제품버전11.01이다.1095(`4619ffaa / 20261107-184 CP`)의같은 실제 송신 장치에 이어 **메타 표시의 기울기 카메라·투영·기준 배율·raw 객체 저장 본체를 컴파일하고 같은 ClientChallengeGlobals에 장기 소유시켰다.** 전체0047dd60/479760/47cab0 표시/Poll·CS 수신·실제 오브젝트/HTML 연결과1dh/28~33 등록은 미적용이다.

00419570의 보호double각도0.7853981852531433과sin/cos의F32저장,004189f0 높이만 저장,00418a00 높이20미만 `(20-height)*sin+1`/FTOL 하위DWORD와그밖1을복원했다. 이 공식은C 디컴파일의FPU생략을PE로보완했다. Camera/기준플래그005930a0/기준높이00540adc/기준배율00540ae0를같은Client에두어CS Reset/Session 교체에서임의재생성하지않는다. 기준높이/배율의PE 초기값22/1과첫보정후60/계산값을구별한다.

00497e50의viewport 경계합/2와00497ec0의화면크기/2는서로다르다. signed32 덧셈감김→signed /2의0방향절삭→현재Renderer 픽셀더하기→현재float 칸크기나누기→F32저장을보존했다. 전역snapshot을현재Renderer로대신쓰지않는다.

00418a30은단정도입력/저장과중간double을구별하고,깊이의ordered `<= -1`에서만clipped1·scale0/좌표를쓴다. NaN은원문분기처럼나눗셈으로진행한다.00479760의최초보정분기는임시카메라생성/높이0→0,0,4투영→필요진단 `!clipped`(Metadisplay.cpp:1081)→기준배율→기준높이60→보정flag1이다. 상위PlayerKnown 관문과이후전체표시루프는후속이다.

00479680은현재카메라투영→normalize/화면flags10h일때만기준배율나누기→scale*65536/FTOL→raw+8 WORD→raw+1a DWORD→반환용UI clipped추가순서다. +1a는공통HP필드를메타표시의상위14비트배율/4000h숨김으로재사용한다. **후행0.9/1.1범위판정은이미쓴raw4000h를다시바꾸지않는다.** 실제raw의해당폭만쓰며다른필드/기본게임HP동작은수정하지않는다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0이다. 원문8함수+00479760 첫보정분기/관련PE 전체범위·상수9개·같은Client owner/선행저장순서를정적대조했다. 보호2800파일불변·새fixture0이며신규C++/MP/게임/통신/새R·원본R/전체검사/34미션/ZIP실행0이다. **std::sin/cos·double로이식했으며Native x87초월함수/모든중간비트·픽셀의실행대조는없다.** 컴파일/정적대조를MP동작검증완료로쓰지않는다. Netstorm.exe 12,397,568바이트/SHA-256 `7f48c789d2bad8dad52e1cc520737cc09df83abbe61bd3ef5e6b2af2f79ed64d`,새R.exe 669,696바이트/SHA-256 `aeee6385673a5b1d9ab373b33cf22cf54d3560d98f99988732f89f120b0ac146`다.

다음은전체0047dd60/479760/47cab0·CS수신/실제오브젝트·가상표/카메라·HTML장치→1095송신/1096투영과1dh/28~33등록이다. 세.server 콘텐츠/배포·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어후속→Edit시나리오UI도남아전체목표는미완료다. 사용자검사제외·폐기검증/공식서버미접속·보호원본규칙을유지한다.

보고서`cpppj/meta-projection-20261108.json`,원문/기계어·빌드/정적감사`extracted/cpp-resume-1096/`(Git제외)다. 마지막전체정밀디컴파일 **2026-11-03/HJOW-Athlon**,R전용 **2026-11-07/HJOW-Athlon**이며재디컴파일/생성소스변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는수정하지않았다.
