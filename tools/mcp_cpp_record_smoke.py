#!/usr/bin/env python3
"""cpppj 로컬 MCP 녹화(영상+소리) 스모크 검사.

게임을 소리를 켠 채(`--no-audio` 없이) `--run <게임 디렉터리> --mcp --window`로 띄우고 netstorm_record_start/stop으로
몇 초 녹화한 뒤, 프레임 PNG 순번 파일·manifest·ffconcat·audio.wav를 검증한다.
메인 메뉴의 장면 음악이 재생 중이므로 오디오 장치가 정상이면 audio.wav에 0이 아닌 표본이 있어야 한다.
게임 창과 소리가 실제로 나오므로 AGENTS.md의 실행 확인 규칙(예외 호스트 또는 사용자 확인)을 따른다.

사용법: python tools/mcp_cpp_record_smoke.py <Netstorm.exe 절대 경로> <게임 디렉터리 절대 경로> [--mux]
"""
import json
import os
import shutil
import sys
import tempfile
import wave

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mcp_cpp_smoke import McpClient  # noqa: E402


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    exe, game = sys.argv[1], sys.argv[2]
    mux = "--mux" in sys.argv
    user_data = tempfile.mkdtemp(prefix="netstorm-rec-")
    env = dict(os.environ, NETSTORM_USER_DATA=user_data)
    client = McpClient([exe, "--run", game, "--mcp", "--window"], env)
    try:
        client.request("initialize", {"protocolVersion": "2024-11-05", "capabilities": {}})
        client.call("netstorm_wait_frames", frames=20)
        start = json.loads(client.call("netstorm_record_start", fps=10, name="smoke")["content"][0]["text"])
        print("start:", start)
        assert "error" not in start, start
        # 녹화 중 화면을 바꾼다(Options 대화창 열기). 장면 음악은 계속 재생된다.
        client.call("netstorm_input", actions=[{"op": "script-command", "arg": "Tell,Blank"}], wait_frames=10, screenshot=False)
        client.call("netstorm_wait_frames", frames=150)
        stop = json.loads(client.call("netstorm_record_stop", mux=mux)["content"][0]["text"])
        print("stop:", json.dumps(stop, ensure_ascii=False))
        assert stop["frames"] >= 5, stop
        directory = stop["directory"]
        assert os.path.dirname(directory).replace("\\", "/").startswith(user_data.replace("\\", "/")), directory  # 사용자 Captures 아래여야 한다.
        names = os.listdir(directory)
        assert "manifest.json" in names and "frames.ffconcat" in names, names
        frames = sorted(name for name in names if name.startswith("frame_"))
        assert len(frames) == stop["frames"], (len(frames), stop["frames"])
        manifest = json.load(open(os.path.join(directory, "manifest.json"), encoding="utf-8"))
        assert manifest["frames"] == stop["frames"] and len(manifest["timestamps_ms"]) == stop["frames"]
        if start.get("audio"):
            with wave.open(os.path.join(directory, "audio.wav"), "rb") as wav:
                print("wav:", wav.getnchannels(), "ch", wav.getframerate(), "Hz", wav.getsampwidth() * 8, "bit", wav.getnframes(), "frames")
                assert wav.getnchannels() == 2 and wav.getframerate() == 44100 and wav.getsampwidth() == 2
            audio = stop["audio"]
            print("audio stats:", audio)
            assert audio["frames"] > 0
            print("sound present:", audio["non_zero_frames"] > 0)
        else:
            print("audio 캡처를 시작하지 못함:", start.get("audio_error"))
        if mux:
            print("mp4:", stop.get("mp4"), stop.get("mux_error"))
        client.call("netstorm_quit")
        code = client.process.wait(timeout=60)
        assert code == 0, code
        print("OK")
        return 0
    except Exception:
        print("\n".join(client.stderr_lines[-20:]))
        client.process.kill()
        raise
    finally:
        if "--keep" not in sys.argv:
            shutil.rmtree(user_data, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
