using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
using Netstorm.Client.Mission;

namespace Netstorm.Client.Tests;

/// <summary>요새 요청의 독립 원본 저장소와 효과 순서를 대조한다.</summary>
public sealed class MissionFortRequestTests
{
    private static string Hex(byte[] bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    [Fact]
    public void MatchesNativeRequests()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "mission-fort-request-x86.tsv");
        string[] rows = File.ReadAllLines(path).Where(x => x.Length != 0 && x[0] != '#').ToArray(); Assert.Equal(144, rows.Length);
        foreach (string row in rows)
        {
            string[] columns = row.Split('\t'); using JsonDocument doc = JsonDocument.Parse(columns[0]); JsonElement[] input = doc.RootElement.EnumerateArray().ToArray();
            var globals = new MissionFortRequestGlobals { Port = input[2].GetUInt32(), Sequence = input[3].GetUInt32() };
            Convert.FromHexString(input[1].GetString()!).CopyTo(globals.LocalAddress, 0);
            byte[] bytes = Convert.FromHexString(columns[1]);
            for (int i = 0; i < 36; i++) globals.Next[i] = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(8 + i * 4));
            uint variant = input[5].GetUInt32(); var image = new MissionFortLoadRequest(); string trace = "", storage = "";
            MissionFortRequest.Write(input[4].GetString()!, globals, new MissionFortRequestHooks
            {
                SelectFort = name => { trace = "Fort," + Hex(Encoding.Latin1.GetBytes(name)); if (variant == 1) { Encoding.ASCII.GetBytes("ABCDEFGHI").CopyTo(globals.LocalAddress, 0); globals.Port = 12345; globals.Sequence = 0xfffffffe; } },
                Duplicate = packet => { trace += ";Allocate,86"; if (variant == 2) { globals.Sequence = 0xffffffff; globals.Port = 9876; globals.Next[0x60 / 4] = 0x76543210; } Assert.Equal(1u, MissionFortRequest.CaptureZone(image, packet)); storage = Hex(packet); return 0x12200000; }
            });
            for (int i = 0; i < 36; i++) BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(8 + i * 4), globals.Next[i]);
            Assert.Equal(columns[2], Hex(bytes)); Assert.Equal(columns[3], globals.Sequence.ToString()); Assert.Equal(columns[4], storage); Assert.Equal(columns[5], Hex(image.Zone)); Assert.Equal(columns[6], trace);
        }
    }

    [Fact]
    public void CapturedZoneIsOwnedAndCannotBeCapturedTwice()
    {
        var image = new MissionFortLoadRequest(); byte[] packet = MissionFortRequest.MakeLocalMissionZone();
        Assert.Equal(1u, MissionFortRequest.CaptureZone(image, packet)); packet[0] = 99;
        Assert.Equal(1, image.Zone[0]); Assert.Equal(86, image.Zone.Length);
        Assert.Throws<InvalidOperationException>(() => MissionFortRequest.CaptureZone(image, packet));
    }
}
