@echo off
setlocal
rem 이 실행기는 저장된 전체화면 시작값을 창 모드로 바꾼 뒤 원본 게임을 시작합니다.
rem 전체화면 재시작 때 발생하는 DirectDraw 초기화 오류를 피하기 위한 실행 파일입니다.
powershell -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0RunNetstormWindowed.ps1"
exit /b %ERRORLEVEL%
