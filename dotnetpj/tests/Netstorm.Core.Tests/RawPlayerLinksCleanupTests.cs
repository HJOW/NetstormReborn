using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원본 Player 링크 정리162입력의 두 전체 기록·현재 base·free 인자와 재진입 순서를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>명시 외부 효과만 공급하며 정리 결과는 원본 TSV로 비교한다.</summary>
public sealed class RawPlayerLinksCleanupTests
{
    /// <summary>선택된 외부 free 효과의 명시 중단이다.</summary>
    private sealed class Stop : Exception { }
    /// <summary>초기 두 물리·현재 base·원본과 같은 외부 변이 장치다.</summary>
    private sealed class Device
    {
        /// <summary>전체1620바이트 Player 두 벌이다.</summary>
        public readonly byte[][] Blocks = [new byte[1620], new byte[1620]];
        /// <summary>현재 물리 base 번호다.2는 NULL 입력이다.</summary>
        public uint Current;
        /// <summary>free 효과가 바꾸는 외부 owner다. 정리 본체가 읽으면 안 된다.</summary>
        public uint Owner = 7;
        /// <summary>선택된 외부 중단 호출 번호다.</summary>
        private readonly uint _stop;
        /// <summary>현재 free 호출 번호다.</summary>
        private uint _calls;
        /// <summary>선택된 외부 변이 명세다.</summary>
        private readonly string _mutation;
        /// <summary>실제 전달받은 free 토큰 순서다.</summary>
        public readonly List<uint> Tokens = [];
        /// <summary>각 base 조회/free/재진입 순서다.</summary>
        public readonly List<string> Trace = [];
        /// <summary>Native 실행기에 공급했던 물리와 명시 변이를 그대로 준비한다.</summary>
        public Device(string input)
        {
            string[] values = input.Split(','); uint seed = X86Fixture.UInt(values[0]);
            uint[] masks = [X86Fixture.UInt(values[1]), X86Fixture.UInt(values[2])];
            Current = X86Fixture.UInt(values[3]); _stop = X86Fixture.UInt(values[4]); _mutation = values[5];
            // 변경하지 않을 인접 필드도 모두 원본 입력 패턴으로 채운다.
            for (int block = 0; block < 2; block++)
            {
                // 두 전체 배열의 각 물리 바이트 입력이다.
                for (int index = 0; index < 1620; index++) Blocks[block][index] = unchecked((byte)(seed + block * 53 + index * 17));
                // 실제 토큰만 각 기록의 Deck DWORD에 입력한다.
                for (int owner = 0; owner < 9; owner++)
                    Put(Blocks[block], owner * 180 + 104, (masks[block] & (1u << owner)) != 0 ? 0x21000000u + (uint)block * 0x1000u + (uint)owner * 0x10u : 0);
            }
        }
        /// <summary>매 위치의 현재 base를 관측하고 살아 있는 실제 물리를 돌려준다.</summary>
        private Memory<byte> Players() { Trace.Add("R:" + Current); return Current == 2 ? Memory<byte>.Empty : Blocks[(int)Current]; }
        /// <summary>Native와 같은 free 인자·중단·명시 base/owner/포인터 변이와 재진입만 수행한다.</summary>
        private void Release(uint token)
        {
            Trace.Add("F:" + token); Tokens.Add(token); _calls++;
            if (_calls == _stop) throw new Stop();
            string[] parts = _mutation.Split('-', 2);
            if (parts[0] != "f" + _calls) return;
            switch (parts[1])
            {
                case "baseB": Current = 1; break;
                case "baseA": Current = 0; break;
                case "null": Current = 2; break;
                case "owner8": Owner = 8; break;
                case "pointerA": Put(Blocks[0], 104, 0x21234567); break;
                case "nextA": Put(Blocks[0], 180 + 104, 0); break;
                case "reenter": Trace.Add("RE:begin"); RawPlayerLinksCleanup.Run(Hooks()); Trace.Add("RE:end"); break;
                default: throw new InvalidOperationException("Player 링크 명시 효과 해석 누락");
            }
        }
        /// <summary>같은 현재 물리와 외부 free 장치를 본체에 연결한다.</summary>
        public PlayerLinksCleanupHooks Hooks() => new() { Players = Players, Release = Release };
    }
    /// <summary>입력 DWORD만 little endian으로 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>두 x87 정밀도 원문162행의 전체 물리와 정상/중단/NULL 부분 쓰기를 대조한다.</summary>
    [Fact]
    public void OriginalCurrentBaseAndFreeOrder162()
    {
        string[][] rows = X86Fixture.Read("player-links-cleanup"); Assert.Equal(162, rows.Length);
        var statuses = new Dictionary<string, int>();
        // 원본 각 입력을 새 물리/외부 장치로 실행한다.
        foreach (string[] row in rows)
        {
            var device = new Device(row[1]); string status = "normal";
            try { RawPlayerLinksCleanup.Run(device.Hooks()); }
            catch (Stop) { status = "stop"; }
            catch (ArgumentOutOfRangeException) when (device.Current == 2) { status = "fault-null"; }
            statuses[status] = statuses.GetValueOrDefault(status) + 1;
            Assert.Equal(row[2], status); Assert.Equal(row[3], $"{device.Current},{device.Owner}");
            Assert.Equal(row[4], (Convert.ToHexString(device.Blocks[0]) + Convert.ToHexString(device.Blocks[1])).ToLowerInvariant());
            Assert.Equal(row[5], device.Tokens.Count == 0 ? "-" : string.Join(',', device.Tokens));
            Assert.Equal(row[6], string.Join(';', device.Trace));
        }
        Assert.Equal(138, statuses["normal"]); Assert.Equal(18, statuses["stop"]); Assert.Equal(6, statuses["fault-null"]);
    }
}
