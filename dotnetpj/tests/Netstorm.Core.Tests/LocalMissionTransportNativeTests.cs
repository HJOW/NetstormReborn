using System.Buffers.Binary;
using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Net;
using Netstorm.Tests;

// 로컬 작성/설치와 집합 분배를 기존 독립 원본 관측으로 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>원본66입력의 전체 바이트·현재값·trace와 실제CoreData 별칭을 검증한다.</summary>
public sealed class LocalMissionTransportNativeTests
{
    /// <summary>메타맵도캐시의같은10바이트를빌리며캐시주소교체가이미빌린주소를바꾸지않는다.</summary>
    [Fact]
    public void MetamapCoreDataBorrowsActualCachedAllocation()
    {
        var globals = new LocalMissionZoneGlobals { Metamap = true }; byte[] cached = globals.CachedCoreData;
        LocalMissionZone.Install(LocalMissionPackets.MakeLocalMissionZone().Storage, new byte[1620], new(), globals, new()
        {
            LoginTimeout = () => 30, Seconds = () => 0, StoreName = (_, _) => { }, Serve = port => Assert.Equal(6799u, port),
        }, 6799);
        Assert.True(globals.CurrentCoreData.Equals(cached.AsMemory())); globals.CurrentCoreData.Span[0] = 77; Assert.Equal(77, cached[0]);
        globals.CachedCoreData = new byte[10]; Assert.True(globals.CurrentCoreData.Equals(cached.AsMemory())); Assert.Equal(77, globals.CurrentCoreData.Span[0]);
    }
    /// <summary>명시 입력의 DWORD만 같은 물리에 쓴다.</summary>
    private static void Put(byte[] bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset), value);
    /// <summary>원문 단일바이트 문자열의 같은 비트를 trace로 표현한다.</summary>
    private static string Hex(string value) => Convert.ToHexString(Encoding.Latin1.GetBytes(value)).ToLowerInvariant();
    /// <summary>원본의2Zone·24Login·32Batch·8Install 고유 입력을 전부 대조한다.</summary>
    [Fact]
    public void OriginalLocalMissionTransport66()
    {
        string[][] rows = X86Fixture.Read("local-mission-transport").Where(row => row[0] != "kind").ToArray(); Assert.Equal(66, rows.Length); var seen = new HashSet<string>();
        // 최초 원본의 각 독립 입력을 새 실제 장치에 공급한다.
        foreach (string[] row in rows)
        {
            Assert.True(seen.Add(row[0] + ":" + row[1])); uint[] input = row[1].Split(',').Select(X86Fixture.UInt).ToArray(); uint seed = input[0]; int nameLength = (int)input[1], imageLength = (int)input[2];
            if (row[0] == "Zone")
            {
                LocalMissionTransport.LocalMissionPacket packet = LocalMissionPackets.MakeLocalMissionZone(); Assert.Equal(X86Fixture.Bytes(row[2]), packet.Storage); Assert.Equal(X86Fixture.Bytes(row[3]), packet.Bytes().ToArray());
            }
            else if (row[0] == "Login")
            {
                byte[] image = Enumerable.Range(0, imageLength).Select(index => unchecked((byte)(index * 17 + seed))).ToArray(); string name = new('N', nameLength);
                LocalMissionTransport.LocalMissionPacket packet = LocalMissionPackets.MakeLocalMissionLogin(1, name, image); Assert.Equal(X86Fixture.Bytes(row[2]), packet.Storage); Assert.Equal(X86Fixture.Bytes(row[3]), packet.Bytes().ToArray());
                LocalMissionTransport.LocalMissionLoginView view = LocalMissionPackets.ReadLocalMissionLogin(packet.BytesMemory()); Assert.Equal(1u, view.Subscriber); Assert.Equal(name, view.Handle); Assert.Equal(image, view.FortImage.ToArray());
                Assert.True(MemoryMarshal.TryGetArray(view.FortImage, out ArraySegment<byte> imageView)); Assert.Same(packet.Storage, imageView.Array); Assert.Equal(17 + packet.Storage[12], imageView.Offset);
            }
            else if (row[0] is "Batch" or "BatchClosed")
            {
                uint now = 104; bool mutated = false; var trace = new List<string>();
                var hooks = new PacketDispatchHooks
                {
                    Now = () => { now++; trace.Add("Now," + now); return now; },
                    Record = (connection, packet, length) => trace.Add($"Record,{packet.Span[1]},{length},{connection.Sender}"),
                    Terminate = connection => { trace.Add("Terminate"); connection.Flags |= 0x40; }, Close = _ => trace.Add("Close"), Release = _ => trace.Add("Free"),
                };
                using var wire = new LocalMissionTransport(new byte[1620], () => true, hooks);
                /// <summary>원본외부수신의현재sender변경입력만적용한다.</summary>
                void Receive(uint sender, PacketConnection connection, ReadOnlyMemory<byte> packet)
                {
                    trace.Add($"Receive,{sender},{packet.Span[1]},{connection.Depth}"); if (!mutated && imageLength != 0) { connection.Sender = 1; mutated = true; }
                }
                wire.RegisterLogin(Receive); wire.Dispatch.Version(0x27, 1, (byte)nameLength); wire.Dispatch.Version(0x24, 1, 0); wire.Dispatch.Register(0x24, Receive);
                PacketConnection node = wire.Server; node.Sender = uint.MaxValue; node.Flags = row[0] == "BatchClosed" ? 0x64u : 0x24; node.Received = 103; node.Heartbeat = 104;
                bool result = wire.DispatchFrame(node, X86Fixture.Bytes(row[2]));
                Assert.Equal(row[4], string.Join(',', new[] { result ? 1u : 0, node.Flags, node.Sender, node.Received, node.Heartbeat, node.Depth })); Assert.Equal(row[5], trace.Count == 0 ? "-" : string.Join(';', trace));
            }
            else
            {
                Assert.Contains(row[0], new[] { "Install", "InstallMutate" }); bool mutate = row[0] == "InstallMutate", mutated = false;
                byte[] zone = new byte[512]; Array.Fill(zone, (byte)0xff); zone[0] = 1; zone[1] = 0x1a; zone[2] = 86; zone[3] = 0; Put(zone, 4, 0x82000004); zone[8] = zone[9] = 0; Put(zone, 10, 0); zone[14] = 1; zone[15] = 0; Put(zone, 16, 1); Put(zone, 20, seed & 1);
                string name = new('N', nameLength); Encoding.Latin1.GetBytes(name).CopyTo(zone, 24); zone[24 + name.Length] = 0; zone[56] = 0;
                if (mutate) { zone[86] = 1; Put(zone, 87, 42); Put(zone, 91, 0); Encoding.Latin1.GetBytes("Second").CopyTo(zone, 95); zone[101] = zone[127] = 0; }
                byte[] players = new byte[1620]; Put(players, 180 + 0x50, seed << 12); var player = new PlayerWorldState { BattleOwner = 8, TemplateOwner = 8 }; var globals = new LocalMissionZoneGlobals { MySubscriber = 1, PendingStamp = seed + 3, MissionWorld = true }; var trace = new List<string>();
                LocalMissionZone.Install(zone, players, player, globals, new()
                {
                    LoginTimeout = () => { trace.Add("Timeout"); return 30; }, Seconds = () => { trace.Add("Time"); return 100.5; },
                    StoreName = (subscriber, current) => { trace.Add($"Name,{subscriber},{Hex(current)}"); if (mutate && !mutated) { Put(zone, 16, 9); zone[14] = 2; mutated = true; } },
                    Serve = port => trace.Add("Serve," + port), Assertion = text => throw new InvalidOperationException(text),
                }, 6799);
                Assert.Equal(X86Fixture.Bytes(row[2]), players); Assert.Equal(row[5], string.Join(';', trace));
                string state = string.Join(',', new[] { player.BattleOwner, player.TemplateOwner, player.PlayerKnown, globals.Server ? 1u : 0, globals.Boss ? 1u : 0, globals.ActiveStamp, globals.ActiveZoneWord, globals.ZoneFlags }) + "," + BitConverter.DoubleToUInt64Bits(globals.EarlyCaptureEnd);
                Assert.Equal(row[4], state); Assert.True(MemoryMarshal.TryGetArray((ReadOnlyMemory<byte>)globals.CurrentCoreData, out ArraySegment<byte> segment)); Assert.Same(players, segment.Array); Assert.Equal(180 + 0x6c, segment.Offset); Assert.Equal(10, segment.Count);
                globals.CurrentCoreData.Span[0] = 77; Assert.Equal(77, players[180 + 0x6c]);
            }
        }
    }
}
