"""격리 사용자 폴더의 실제 cpppj에서 튜토리얼 안내를 관측한다. 원본·단계·승패 값은 쓰지 않는다."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import queue
import subprocess
import threading


class TutorialPeer:
    """자기 게임 자식 하나의 MCP 응답과 종료만 관리한다."""

    def __init__(self, args, output):
        """현재 제품과 별도 사용자 폴더로 시작한다. 고정 시각이나 단계 주입은 사용하지 않는다."""
        self.output = output  # 이 실행의 원시 관측을 보존할 새 디렉터리다.
        self.replies = queue.Queue()  # 읽기 스레드의 실제 JSON 응답이다.
        self.next_id = 0  # 요청을 구별하는 연결별 순번이다.
        self.transcript = []  # 이미지 본문을 제외한 실제 요청/응답 요약이다.
        self.stderr = (output / "stderr.log").open("wb")  # 게임 진단은 MCP stdout과 분리한다.
        settings = f"SCREENW={args.width};SCREENH={args.height};welcomeGiven=1;tellTips=0;autoDemo=0;intro=1;currentLanguage={args.language}"
        if args.fullscreen:
            settings += ";startInFullScreen=1"  # 제품의 기존 설정으로 시작 모드를 선택한다. 별도 제품 CLI 플래그는 만들지 않는다.
        command = [str(args.exe.resolve()), "--run", str(args.game.resolve()), "--mission", args.mission,
                   "--mcp", "--no-audio", "--set", settings]
        if not args.fullscreen:
            command.append("--window")  # 창 모드 검사 때만 기존 강제 창 옵션을 사용한다.
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=self.stderr,
                                        env=dict(os.environ, NETSTORM_USER_DATA=str(output / "user")),
                                        creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0)
        self.reader = threading.Thread(target=self.read, daemon=True)  # 큰 PNG 응답도 파이프를 막지 않게 계속 비운다.
        self.reader.start()
        (output / "run.json").write_text(json.dumps(command, ensure_ascii=False, indent=2), encoding="utf-8")

    def read(self):
        """실제 응답 한 줄마다 큐에 넣고 EOF도 알린다."""
        try:
            for line in self.process.stdout:  # 게임이 종료되면 파이프도 끝난다.
                self.replies.put(json.loads(line))
        except Exception as error:
            self.replies.put(error)
        finally:
            self.replies.put(None)

    def call(self, name, **arguments):
        """게임이 제공하는 MCP 도구에만 제한 시간45초로 요청한다."""
        self.next_id += 1
        request = dict(jsonrpc="2.0", id=self.next_id, method="tools/call", params=dict(name=name, arguments=arguments))
        self.process.stdin.write((json.dumps(request) + "\n").encode("utf-8"))
        self.process.stdin.flush()
        reply = self.replies.get(timeout=45)
        if isinstance(reply, Exception):
            raise reply
        if reply is None or reply.get("id") != self.next_id or "error" in reply:
            raise RuntimeError(reply)
        result = reply["result"]
        self.transcript.append(dict(request=request, result={k: v for k, v in result.items() if k != "content"}))
        if result.get("isError"):
            raise RuntimeError(result)
        return result

    def action(self, operation, argument, frames=10):
        """실제 입력 큐로 키·마우스 동작을 보내고 프레임 완료를 기다린다."""
        return self.call("netstorm_input", actions=[dict(op=operation, arg=argument)], wait_frames=frames, screenshot=False)

    def observe(self, name):
        """실제 보고와 논리 화면 PNG를 같은 이름으로 저장한다."""
        report = self.call("netstorm_report")["content"][0]["text"]
        (self.output / f"{name}.tsv").write_text(report, encoding="utf-8")
        for item in self.call("netstorm_screenshot")["content"]:  # 서버가 반환한 이미지 본문만 저장한다.
            if item.get("type") == "image":
                (self.output / f"{name}.png").write_bytes(base64.b64decode(item["data"]))
        return report

    def close(self):
        """자기 자식에 정상 종료를 요청하고 응답 불능일 때만 자기 자식을 종료한다."""
        try:
            if self.process.poll() is None:
                self.call("netstorm_quit")
                self.process.wait(timeout=15)
        finally:
            if self.process.poll() is None:
                self.process.kill()
                self.process.wait()
            self.stderr.close()
            for pipe in (self.process.stdin, self.process.stdout):  # 시작 실패의 끊어진 파이프도 자기 연결만 닫는다.
                try:
                    pipe.close()
                except OSError:
                    pass
            (self.output / "mcp.json").write_text(json.dumps(self.transcript, ensure_ascii=False, indent=2), encoding="utf-8")


def field(report, key):
    """보고서의 실제 첫 탭 행을 읽으며 누락과 빈 값을 구별한다."""
    return next((line.split("\t")[1:] for line in report.splitlines() if line.startswith(key + "\t")), None)


def dismiss(peer):
    """안내의 실제 다음/확인 버튼만 누른다. 없으면 입력은 생략된다."""
    for _ in range(5):  # 가장 긴 초기 안내의 실제 HTML 페이지 수보다 충분히 반복한다.
        for label in ("MORE", "Next", "OK", "Continue", "다음", "확인", "次へ"):  # 뒤로·승리·미션 전환 버튼은 누르지 않는다.
            peer.action("click-if", label, 4)
    peer.call("netstorm_wait_frames", frames=60)


def main():
    """요청된 한 화면 조건의 실제 표시/전환을 확인하고 실패도 원시 결과로 남긴다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("cpppj/build/bin/Release/Netstorm.exe"))
    parser.add_argument("--game", type=Path, default=Path("extracted/cpp-reset-all-1325/game"))
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--language", choices=("english", "korean", "japanese"), default="english")
    parser.add_argument("--mission", default="tutorial1")
    parser.add_argument("--width", type=int, default=1024)
    parser.add_argument("--height", type=int, default=768)
    parser.add_argument("--fullscreen", action="store_true")
    parser.add_argument("--progress", action="store_true", help="튜토리얼1에서 실제 F4 입력 후 A→B 안내를 확인한다")
    parser.add_argument("--lifecycle", action="store_true", help="F8 안내 재열기·미션 전환·이탈 때 현재 안내의 숨김/복원을 확인한다")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    result = dict(passed=False, exeSha256=hashlib.sha256(args.exe.read_bytes()).hexdigest())
    peer = None
    try:
        peer = TutorialPeer(args, output)
        peer.call("netstorm_wait_frames", frames=180)
        peer.observe("briefing")
        dismiss(peer)
        report = peer.observe("objective")
        result["initial"] = dict(objective=field(report, "tutorial_objective"), gate=field(report, "mission_gate"), size=field(report, "size"), screenMode=field(report, "screen_mode"))
        assert bool(int(field(report, "screen_mode")[0]) & 1) == args.fullscreen, "요청한 창/전체화면 모드와 실제 모드가 다릅니다"
        if args.mission.lower().startswith("tutorial"):
            assert field(report, "tutorial_objective")[0], "초기 미션 안내가 비어 있습니다"
        else:
            assert field(report, "tutorial_objective") == [""], "대상 밖 미션에 안내가 남았습니다"
        if args.progress or args.lifecycle:
            assert args.mission.lower() == "tutorial1"
            peer.action("key", "f4", 90)  # 실제 F4 키 입력으로 원본 Home 명령을 발생시킨다.
            dismiss(peer)
            report = peer.observe("after-f4")
            result["afterF4"] = dict(objective=field(report, "tutorial_objective"), gate=field(report, "mission_gate"))
            assert field(report, "mission_gate")[1] == "66", "실제 F4 뒤 B단계가 아닙니다"
            assert field(report, "tutorial_objective") != result["initial"]["objective"], "현재 단계 안내가 갱신되지 않았습니다"
            assert "8" in field(report, "tutorial_objective")[0], "B단계의 실제 다리8개 조건과 문구가 다릅니다"
        if args.lifecycle:
            previous = field(report, "tutorial_objective")  # 다시 열기 뒤 복원해야 하는 실제 B단계 문구다.
            peer.action("key", "f8", 30)
            assert field(peer.observe("reopened-dialog"), "tutorial_objective") == [""], "안내창과 목표 글자가 겹칩니다"
            dismiss(peer)
            assert field(peer.observe("resumed"), "tutorial_objective") == previous, "안내창을 닫은 뒤 목표가 복원되지 않았습니다"
            peer.action("script-command", "MissionRestart,0", 180)  # 원래 재시작 경로로 같은 미션의 초기 상태에 다시 진입한다.
            dismiss(peer)
            report = peer.observe("restarted")
            assert field(report, "mission_gate")[0:2] == ["1", "65"], "재시작 뒤 초기 A단계가 아닙니다"
            assert field(report, "tutorial_objective") == result["initial"]["objective"], "재시작 뒤 이전 B단계 안내가 남았습니다"
            result["restart"] = True
            peer.action("script-command", "MissionBegin,Tutorial2", 180)  # 기존 메뉴의 실제 미션 전환 명령을 입력 큐에 전달한다.
            dismiss(peer)
            report = peer.observe("next-mission")
            assert field(report, "mission_gate")[0:2] == ["2", "65"], "다음 미션의 실제 초기 단계가 아닙니다"
            assert field(report, "tutorial_objective")[0] and field(report, "tutorial_objective") != previous, "다음 미션에서 이전 안내가 남았습니다"
            peer.action("script-command", "LeaveBattle,1", 120)  # 완료나 승리를 주입하지 않고 정상 이탈 경로를 사용한다.
            report = peer.observe("left-mission")
            assert field(report, "tutorial_objective") == [""], "미션 이탈 뒤 안내가 남았습니다"
            result["lifecycle"] = True
        result["passed"] = True
    except Exception as error:
        result["error"] = repr(error)
    finally:
        if peer:
            try:
                peer.close()
            except Exception as error:
                result["closeError"] = repr(error)
                result["passed"] = False
            result["exitCode"] = peer.process.returncode
            result["passed"] = result["passed"] and peer.process.returncode == 0
        (output / "result.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False), flush=True)
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
