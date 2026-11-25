"""R4-5e2 대조기의 brain·SP·Player 행 추출과 종료 판정을 검사한다. 게임은 실행하지 않는다."""

import json
import struct
import tempfile
import unittest
from pathlib import Path

import ai_frame_gap


def player_hex(brain=0, region=127, name=b"You"):
    """180바이트 Player 기록을 만든다. brain은 +0xb0, 첫 지역은 +0x78이다."""
    raw = bytearray(ai_frame_gap.RECORD_BYTES)
    struct.pack_into("<I", raw, ai_frame_gap.BRAIN_OFFSET, brain)
    struct.pack_into("<i", raw, ai_frame_gap.REGION_OFFSET, region)
    raw[0x0C:0x0C + len(name)] = name
    return raw.hex()


def write_dump(path, players, sps, game_random="1", crt_random="2"):
    """최소 session-dump TSV를 쓴다. players는 owner→hex, sps는 owner→필드 목록이다."""
    lines = ["netstorm_session_dump\t1", f"random\t{game_random}\t{crt_random}", "world\t1"]
    for owner, hexed in players.items():
        lines.append(f"player\t{owner}\t{hexed}")
    for owner, fields in sps.items():
        lines.append("sp\t" + str(owner) + "\t" + "\t".join(fields))
    lines.append("end")
    Path(path).write_text("\n".join(lines) + "\n", encoding="utf-8")


class AiFrameGapTests(unittest.TestCase):
    """brain 토큰 위치·SP 대조·종료 판정의 혼동을 막는다."""

    def test_brain_token_offsets(self):
        """brain은 +0xb0, 첫 지역은 +0x78에서 읽는다."""
        raw = bytes.fromhex(player_hex(brain=0x20000000, region=3))
        self.assertEqual(0x20000000, ai_frame_gap.brain_token(raw))
        self.assertEqual(3, ai_frame_gap.first_region(raw))

    def test_short_player_record_is_rejected(self):
        """180바이트가 아니면 Player 기록으로 받지 않는다."""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "dump.tsv"
            write_dump(path, {1: "00" * 100}, {})
            with self.assertRaises(ValueError):
                ai_frame_gap.compare_route(path, path)

    def test_matching_route_has_no_gap(self):
        """같은 덤프끼리는 격차가 없고 종료 0이다."""
        with tempfile.TemporaryDirectory() as directory:
            dump = Path(directory) / "dump.tsv"
            write_dump(dump, {1: player_hex(0x20000000, 1), 2: player_hex()}, {1: ["50", "0"]})
            compared = ai_frame_gap.compare_route(dump, dump)
            self.assertEqual({}, compared["brain_diff"])
            self.assertEqual({}, compared["sp_diff"])
            self.assertEqual({}, compared["player_offsets"])

    def test_brain_and_sp_diffs_are_reported(self):
        """brain 토큰·SP 값 차이를 owner별로 보고한다."""
        with tempfile.TemporaryDirectory() as directory:
            cpp = Path(directory) / "cpp.tsv"
            dn = Path(directory) / "dn.tsv"
            write_dump(cpp, {2: player_hex(0x20000001, 1)}, {1: ["50", "7"]})
            write_dump(dn, {2: player_hex(0x20000000, 1)}, {1: ["50", "0"]})
            compared = ai_frame_gap.compare_route(cpp, dn)
            self.assertEqual({2: (0x20000001, 0x20000000)}, compared["brain_diff"])
            self.assertEqual({1: (["50", "7"], ["50", "0"])}, compared["sp_diff"])
            self.assertEqual({2: {"cpp": 0x20000001, "dn": 0x20000000}}, compared["brains"])

    def test_player_byte_histogram_points_at_fields(self):
        """바이트 대조는 brain(+0xb0) 위치를 가리킨다."""
        with tempfile.TemporaryDirectory() as directory:
            cpp = Path(directory) / "cpp.tsv"
            dn = Path(directory) / "dn.tsv"
            write_dump(cpp, {2: player_hex(0x20000001, 1)}, {})
            write_dump(dn, {2: player_hex(0x20000000, 1)}, {})
            compared = ai_frame_gap.compare_route(cpp, dn)
            self.assertIn(ai_frame_gap.BRAIN_OFFSET, compared["player_offsets"])
            self.assertEqual([2], compared["player_offsets"][ai_frame_gap.BRAIN_OFFSET])

    def test_summary_counts_and_exit_code(self):
        """수집 요약을 세고 격차가 남으면 종료 1, 없으면 종료 0이다."""
        with tempfile.TemporaryDirectory() as directory:
            collection = Path(directory)
            for route in ("same", "diff"):
                for engine in ("cpp", "dn"):
                    (collection / route / engine).mkdir(parents=True)
            write_dump(collection / "same" / "cpp" / "f090.tsv", {1: player_hex()}, {1: ["50", "0"]})
            write_dump(collection / "same" / "dn" / "f090.tsv", {1: player_hex()}, {1: ["50", "0"]})
            write_dump(collection / "diff" / "cpp" / "f090.tsv", {1: player_hex(1)}, {1: ["50", "0"]})
            write_dump(collection / "diff" / "dn" / "f090.tsv", {1: player_hex()}, {1: ["50", "0"]})
            summary = ai_frame_gap.summarize(collection)
            self.assertEqual(2, summary["totals"]["routes"])
            self.assertEqual(1, summary["totals"]["brain_diff_owners"])
            self.assertEqual(1, ai_frame_gap.main(["--collection", str(collection)]))
            alone = collection / "same"
            only = Path(directory) / "only"
            (only / "same" / "cpp").mkdir(parents=True)
            (only / "same" / "dn").mkdir(parents=True)
            (only / "same" / "cpp" / "f090.tsv").write_bytes((alone / "cpp" / "f090.tsv").read_bytes())
            (only / "same" / "dn" / "f090.tsv").write_bytes((alone / "dn" / "f090.tsv").read_bytes())
            self.assertEqual(0, ai_frame_gap.main(["--collection", str(only)]))

    def test_missing_route_is_marked(self):
        """한쪽 덤프가 없으면 누락으로 표시하고 세지 않는다."""
        with tempfile.TemporaryDirectory() as directory:
            collection = Path(directory)
            (collection / "half" / "cpp").mkdir(parents=True)
            (collection / "half" / "cpp" / "f090.tsv").write_text("netstorm_session_dump\t1\nend\n", encoding="utf-8")
            summary = ai_frame_gap.summarize(collection)
            self.assertTrue(summary["per_route"]["half"]["missing"])
            self.assertEqual(0, summary["totals"]["routes"])


if __name__ == "__main__":
    unittest.main()
