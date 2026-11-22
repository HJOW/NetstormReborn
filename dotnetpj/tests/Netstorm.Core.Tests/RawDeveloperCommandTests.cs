using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Core.Original;

// Source 패킷 계약과 제품 파서를 사용하지 않은 Windows CRT 관측을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>31h의 가변 길이·현재BYTE·scanf 하위와 보드 변 계산을 대조한다.</summary>
public sealed class RawDeveloperCommandTests
{
    /// <summary>독립 CRT434입력의 성공 개수/정수DWORD/float32 전체 비트를 전수 비교한다.</summary>
    [Fact]
    public void ScannerMatches434IndependentWindowsCrtInputs()
    {
        int rows = 0;
        // 기존 PE fixture와 구별한 OS CRT 관측만 읽으며 실패 입력의 stack 값을 소비하지 않는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "developer-scanf-windows.tsv")))
        {
            if (line.StartsWith('#') || line.Length == 0) continue; string[] row = line.Split('\t'); var input = new RawDeveloperCommandInput(Convert.FromHexString(row[1]));
            int count = int.Parse(row[2], CultureInfo.InvariantCulture); uint expected = uint.Parse(row[3], NumberStyles.HexNumber, CultureInfo.InvariantCulture);
            if (row[0] == "float")
            {
                if (count != 1) Assert.Throws<ArgumentException>(() => input.RequiredFloat());
                else { float value = input.RequiredFloat(); Assert.True(expected == BitConverter.SingleToUInt32Bits(value), line); Assert.Equal(Encoding.ASCII.GetString(Convert.FromHexString(row[5])), RawDeveloperCommandInput.FormatFloat(value)); }
            }
            else if (row[0] == "one")
            {
                bool converted = input.One(out int value); Assert.Equal(count == 1, converted); if (converted) Assert.Equal(expected, unchecked((uint)value));
            }
            else
            {
                int converted = input.Two(out int first, out int second); Assert.Equal(Math.Max(0, count), converted);
                if (converted > 0) Assert.Equal(expected, unchecked((uint)first));
                if (converted > 1) Assert.Equal(uint.Parse(row[4], NumberStyles.HexNumber, CultureInfo.InvariantCulture), unchecked((uint)second));
            }
            rows++;
        }
        Assert.Equal(434, rows);
    }
    /// <summary>255BYTE 키와 전체 %*s, C 공백, 첫NUL·ASCII 전용 소문자를 구별한다.</summary>
    [Fact]
    public void ScannerKeeps255ByteKeyButSkipsWholeCommandAndStopsAtFirstNul()
    {
        var input = new RawDeveloperCommandInput(Encoding.Latin1.GetBytes(new string('A', 260) + "\v-7 8\0 9"));
        Assert.Equal(new string('a', 255), input.Key); Assert.Equal(2, input.Two(out int first, out int second)); Assert.Equal(-7, first); Assert.Equal(8, second);
        Assert.Equal("Àz", new RawDeveloperCommandInput(Encoding.Latin1.GetBytes("ÀZ 1")).Key); Assert.Throws<ArgumentException>(() => new RawDeveloperCommandInput(" \t\r\n\v\f"u8));
    }
    /// <summary>수신 관찰은 빈 물리 입력에서도 무효과이며 송신 진단 뒤 현재 DWORD를 다시 읽는다.</summary>
    [Fact]
    public void PacketObserverReadsCurrentLengthAfterReturningAssertion()
    {
        RawDeveloperCommandPacket.Observe(Memory<byte>.Empty, 0, null); var packet = Enumerable.Repeat((byte)0xa5, 16).ToArray(); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), uint.MaxValue); int assertions = 0;
        RawDeveloperCommandPacket.Observe(packet, 1, (text, file, line) => { assertions++; Assert.Equal("len != (int)-1", text); Assert.Equal(@"\Ns\O\_p_packets.cpp", file); Assert.Equal(0x74bu, line); Assert.Equal(0xa5, packet[2]); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 65535); });
        Assert.Equal(1, assertions); Assert.Equal(7, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2))); Assert.All(packet[8..], value => Assert.Equal(0xa5, value));
    }
    /// <summary>수신은 선언DWORD를 무시하고 같은 입력의 첫NUL 전 구간만 실제 실행기에 빌린다.</summary>
    [Fact]
    public void PacketReceiverUsesFirstNulAndOriginalMemoryInsteadOfDeclaredLength()
    {
        byte[] packet = [0xff, 0x31, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, (byte)'q', (byte)'b', 0, (byte)'x']; ReadOnlyMemory<byte> captured = default;
        RawDeveloperCommandPacket.Receive(packet, body => captured = body); Assert.Equal("qb"u8.ToArray(), captured.ToArray()); packet[8] = (byte)'Q'; Assert.Equal((byte)'Q', captured.Span[0]);
        packet[8] = 0; RawDeveloperCommandPacket.Receive(packet, body => Assert.True(body.IsEmpty));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawDeveloperCommandPacket.Receive(new byte[8], null)); Assert.Throws<ArgumentOutOfRangeException>(() => RawDeveloperCommandPacket.Receive(Enumerable.Repeat((byte)1, 12).ToArray(), null));
        Assert.Throws<InvalidOperationException>(() => RawDeveloperCommandPacket.Receive(new byte[9], null));
    }
    /// <summary>네 변의 확장/중간점은 별도 지도 clamp 없이 Source 식을 사용한다.</summary>
    [Theory]
    [InlineData(0u, 5f, 1f)]
    [InlineData(2u, 9f, 5f)]
    [InlineData(4u, 5f, 9f)]
    [InlineData(6u, 1f, 5f)]
    public void BoardTargetExpandedSidesUseSourceGeometry(uint side, float x, float y)
    {
        RawPathPoint point = new RawBoardTarget(new()).Side(new(2, 2, 8, 8), side, 4); Assert.Equal(x, point.X); Assert.Equal(y, point.Y);
    }
    /// <summary>잘못된 변의 flags 재해석과 일반 접근 실패 뒤 현재 실패점 재조회를 보존한다.</summary>
    [Fact]
    public void BoardTargetKeepsQuietNanAndReadsCurrentFailurePointAfterAssertion()
    {
        var state = new BoardTargetState(); var lines = new List<uint>(); var board = new RawBoardTarget(state, line => { lines.Add(line); state.Invalid = new() { X = 17, Y = -21 }; });
        RawPathPoint point = board.Side(new(0, 0, 1, 1), 7, 0x7f800001); Assert.Equal(0x7fc00001u, BitConverter.SingleToUInt32Bits(point.X)); Assert.Equal(BitConverter.SingleToUInt32Bits(point.X), BitConverter.SingleToUInt32Bits(point.Y));
        point = board.Nearest(new(1, 1, 2, 2), new() { X = 4, Y = 8 }, 0); Assert.Equal(17, point.X); Assert.Equal(-21, point.Y); Assert.Equal(new uint[] { 0x1f9, 0x262 }, lines);
    }
}
