using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>요새 요청의 독립 원본 저장소와 효과 순서를 대조한다.</summary>
public sealed class MissionFortRequestTests
{
    private static string Hex(byte[] bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void MatchesNativeRequests(bool bound)
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "mission-fort-request-x86.tsv");
        string[] rows = File.ReadAllLines(path).Where(x => x.Length != 0 && x[0] != '#').ToArray(); Assert.Equal(144, rows.Length);
        foreach (string row in rows)
        {
            string[] columns = row.Split('\t'); using JsonDocument doc = JsonDocument.Parse(columns[0]); JsonElement[] input = doc.RootElement.EnumerateArray().ToArray();
            var local = new ClientLocalStateGlobals(new(), new());
            byte[] address = Convert.FromHexString(input[1].GetString()!);
            uint port = input[2].GetUInt32();
            var globals = bound ? new MissionFortRequestGlobals(local.State, local.ZonePending, address, () => port) : new MissionFortRequestGlobals { Port = port };
            globals.Sequence = input[3].GetUInt32();
            if (!bound) { address.CopyTo(globals.LocalAddress, 0); address = globals.LocalAddress; }
            byte[] bytes = Convert.FromHexString(columns[1]);
            for (int i = 0; i < 36; i++) globals.WriteNext(i, BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(8 + i * 4)));
            uint variant = input[5].GetUInt32(); var image = new MissionFortLoadRequest(); string trace = "", storage = "";
            MissionFortRequest.Write(input[4].GetString()!, globals, new MissionFortRequestHooks
            {
                SelectFort = name => { trace = "Fort," + Hex(Encoding.Latin1.GetBytes(name)); if (variant == 1) { Encoding.ASCII.GetBytes("ABCDEFGHI").CopyTo(address, 0); port = 12345; if (!bound) globals.Port = port; globals.Sequence = 0xfffffffe; } },
                Duplicate = packet => { trace += ";Allocate,86"; if (variant == 2) { globals.Sequence = 0xffffffff; port = 9876; if (!bound) globals.Port = port; globals.WriteNext(0x60 / 4, 0x76543210); } Assert.Equal(1u, MissionFortRequest.CaptureZone(image, packet)); storage = Hex(packet); return 0x12200000; }
            });
            for (int i = 0; i < 36; i++) BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(8 + i * 4), globals.ReadNext(i));
            Assert.Equal(columns[2], Hex(bytes)); Assert.Equal(columns[3], globals.Sequence.ToString()); Assert.Equal(columns[4], storage); Assert.Equal(columns[5], Hex(image.Zone)); Assert.Equal(columns[6], trace);
            if (bound)
            {
                Assert.Equal(bytes.AsSpan(8, 144).ToArray(), local.State.Next.ToArray()); Assert.Equal(globals.Sequence, local.ZonePending.Value);
            }
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
