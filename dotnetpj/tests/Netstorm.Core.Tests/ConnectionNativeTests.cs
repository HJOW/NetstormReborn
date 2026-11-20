using System.Buffers.Binary;
using Netstorm.Net;
using Netstorm.Tests;

// 원본 연결 등록/정리의 전체 표와 물리 노드 및 중단 prefix를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>기존 독립 원본868입력으로 같은 실제 등록/정리 본체를 검증한다.</summary>
public sealed class ConnectionNativeTests
{
    /// <summary>원본 관측의 첫 실제 격리 노드 주소다.</summary>
    private const uint Nodes = 0x12000100;
    /// <summary>명시 장치에서만 중단하고 정상 반환과 구별한다.</summary>
    private sealed class EffectStop : Exception { }
    /// <summary>전체 패턴 중 현재4바이트만 읽는다.</summary>
    private static uint Read(byte[] bytes, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(offset));
    /// <summary>표현한 현재DWORD만 최초 패턴에 반영한다.</summary>
    private static void Put(byte[] bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset), value);
    /// <summary>명시 입력의 같은 노드만 조회하며 주소 절삭/대체를 하지 않는다.</summary>
    private static PacketConnection Node(PacketConnection[] nodes, uint token)
    {
        if (token < Nodes || (token - Nodes) % 192 != 0 || (token - Nodes) / 192 >= nodes.Length) throw new ArgumentOutOfRangeException(nameof(token));
        return nodes[(token - Nodes) / 192];
    }
    /// <summary>미선택장치는요구하지않고선택된누락지점의flags/sync/칸prefix를보존한다.</summary>
    [Fact]
    public void MissingEffectsAreRequiredOnlyAtNativeSelectionPoint()
    {
        var node = new PacketConnection { Depth = 1 }; ConnectionCleanup.DeferConnectionClose(node, null, null); Assert.Equal(0x40u, node.Flags);
        node.Depth = 0; node.Flags = 0; Assert.Throws<InvalidOperationException>(() => ConnectionCleanup.DeferConnectionClose(node, null, null)); Assert.Equal(0x40u, node.Flags);
        var sync = new ConnectionSyncState { Synchronization = 71, PendingCount = 1 };
        var globals = new ConnectionCleanupGlobals(new uint[22], new uint[22], new uint[32], sync); globals.Transceivers[9] = 71; globals.Buffers[9] = 72;
        var hooks = new ConnectionCleanupHooks { Node = token => { Assert.Equal(71u, token); return node; } };
        Assert.Throws<InvalidOperationException>(() => ConnectionCleanup.DisconnectTransceiver(9, globals, hooks)); Assert.Equal(0u, sync.Synchronization); Assert.Equal(71u, globals.Transceivers[9]);
        int destroyed = 0; hooks.Destroy = _ => { destroyed++; node.Depth = 7; }; node.Depth = 0;
        Assert.Throws<InvalidOperationException>(() => ConnectionCleanup.DisconnectTransceiver(9, globals, hooks)); Assert.Equal(1, destroyed); Assert.Equal(7u, node.Depth); Assert.Equal(71u, globals.Transceivers[9]);
        node.Depth = 1; Assert.Throws<InvalidOperationException>(() => ConnectionCleanup.DisconnectTransceiver(9, globals, hooks));
        Assert.Equal(0u, globals.Transceivers[9]); Assert.Equal(72u, globals.Buffers[9]); Assert.Equal(1, destroyed);
    }
    /// <summary>없는대상/음수범위는장치없이반환하고양수범위진단뒤실제물리접근만거부한다.</summary>
    [Fact]
    public void EmptyAndNegativeIdsRequireNoDevices()
    {
        var sync = new ConnectionSyncState { Synchronization = 19, PendingCount = 7 };
        var globals = new ConnectionCleanupGlobals(new uint[22], new uint[22], new uint[32], sync);
        // 원문음수범위/빈칸의자연반환을보존한다.
        foreach (int id in new[] { int.MinValue, -33, -32, 0, 1, 21 }) ConnectionCleanup.DisconnectTransceiver(id, globals, new());
        ConnectionCleanup.CloseAllTransceivers(globals, new()); ConnectionCleanup.EndPlayerConnections(1, globals, new()); ConnectionCleanup.CloseAllPendingConnections(globals, new());
        Assert.Equal(19u, sync.Synchronization); Assert.Equal(7u, sync.PendingCount); uint line = 0;
        Assert.ThrowsAny<Exception>(() => ConnectionCleanup.DisconnectTransceiver(22, globals, new() { Assertion = value => line = value })); Assert.Equal(250u, line);
    }
    /// <summary>원본등록288행의전체800바이트·효과열·중단60입력을대조한다.</summary>
    [Fact]
    public void OriginalConnectionRegistration288()
    {
        string[][] rows = X86Fixture.Read("connection-registration"); Assert.Equal(288, rows.Length); int stopped = 0;
        // 각독립입력은원문표/노드/전역을새로구성한다.
        foreach (string[] row in rows)
        {
            byte[] bytes = X86Fixture.Bytes(row[7]); Assert.Equal(800, bytes.Length);
            var main = new uint[22]; var pending = new uint[32]; var nodes = new PacketConnection[3];
            // 현재 전체양수표를 실제입력에서만 읽는다.
            for (int i = 0; i < main.Length; i++) main[i] = Read(bytes, i * 4);
            // pending0을포함한현재32칸을보존한다.
            for (int i = 0; i < pending.Length; i++) pending[i] = Read(bytes, 88 + i * 4);
            // 원문등록이표현하는두필드만노드에연결한다.
            for (int i = 0; i < nodes.Length; i++) nodes[i] = new() { Flags = Read(bytes, 224 + i * 192 + 0x34), Sender = Read(bytes, 224 + i * 192 + 0x40) };
            var sync = new ConnectionSyncState { PendingCount = Read(bytes, 216) }; uint server = Read(bytes, 220);
            var globals = new ConnectionCleanupGlobals(main, new uint[22], pending, sync);
            int source = X86Fixture.Int(row[5]), target = X86Fixture.Int(row[6]); bool interrupted = false; var calls = new List<string>();
            /// <summary>지정된실제장치의효과/입력변경/중단만적용한다.</summary>
            void Effect(char kind, uint value)
            {
                calls.Add($"{kind}:{value}");
                if (kind == 'B') { Node(nodes, value).Flags |= 2; if (X86Fixture.UInt(row[2]) != 0) { sync.PendingCount = 0; main[target] = Nodes + 384; pending[-source] = Nodes + 384; } }
                if (row[3] == kind.ToString()) throw new EffectStop();
            }
            var hooks = new ConnectionRegistrationHooks { Node = token => Node(nodes, token), Buffered = token => Effect('B', token), Assertion = line => Effect('A', line) };
            try
            {
                switch (X86Fixture.UInt(row[0]))
                {
                    case 0: ConnectionRegistration.RegisterTransceiver(target, Nodes, globals, hooks); break;
                    case 1: ConnectionRegistration.RemovePendingConnection(source, globals); break;
                    case 2: Assert.Equal(target, ConnectionRegistration.AssignPendingConnection(source, target, server, globals, hooks)); break;
                    default: Assert.Fail("미등록원본등록연산"); break;
                }
            }
            catch (EffectStop) { interrupted = true; stopped++; }
            // 현재표/전역만독립적으로직렬화한다.
            for (int i = 0; i < main.Length; i++) Put(bytes, i * 4, main[i]);
            // 현재pending전체를기대값과비교한다.
            for (int i = 0; i < pending.Length; i++) Put(bytes, 88 + i * 4, pending[i]);
            Put(bytes, 216, sync.PendingCount); Put(bytes, 220, server);
            // 미사용표현필드는변경되지않아야한다.
            for (int i = 0; i < nodes.Length; i++) { Put(bytes, 224 + i * 192 + 0x34, nodes[i].Flags); Put(bytes, 224 + i * 192 + 0x40, nodes[i].Sender); Assert.Equal(0u, nodes[i].Received | nodes[i].Heartbeat | nodes[i].Depth); Assert.False(nodes[i].Delayed); }
            Assert.Equal(X86Fixture.Bytes(row[8]), bytes); Assert.Equal(row[9], calls.Count == 0 ? "-" : string.Join(';', calls)); Assert.Equal(row[10] == "1", interrupted); Assert.Equal(0u, sync.Synchronization);
        }
        Assert.Equal(60, stopped);
    }
    /// <summary>원본정리580행의전체892바이트·현재값재조회·중단268입력을대조한다.</summary>
    [Fact]
    public void OriginalConnectionCleanup580()
    {
        string[][] rows = X86Fixture.Read("connection-cleanup"); Assert.Equal(580, rows.Length); int stopped = 0;
        // 원본각입력은독립표/노드와외부효과를사용한다.
        foreach (string[] row in rows)
        {
            byte[] bytes = X86Fixture.Bytes(row[6]); Assert.Equal(892, bytes.Length); var main = new uint[22]; var buffers = new uint[22]; var pending = new uint[32]; var nodes = new PacketConnection[3];
            // 송수신/별도버퍼표의현재입력이다.
            for (int i = 0; i < main.Length; i++) { main[i] = Read(bytes, i * 4); buffers[i] = Read(bytes, 88 + i * 4); }
            // 전체정리가사용하는pending0도그대로입력한다.
            for (int i = 0; i < pending.Length; i++) pending[i] = Read(bytes, 176 + i * 4);
            // 원문정리노드의동작/보존필드를현재값으로연결한다.
            for (int i = 0; i < nodes.Length; i++) { int at = 316 + i * 192; nodes[i] = new() { Flags = Read(bytes, at + 0x34), Sender = Read(bytes, at + 0x40), Received = Read(bytes, at + 0x44), Heartbeat = Read(bytes, at + 0x48), Depth = Read(bytes, at + 0x8c) }; }
            var sync = new ConnectionSyncState { Synchronization = Read(bytes, 304), PendingCount = Read(bytes, 308) }; uint server = Read(bytes, 312);
            var globals = new ConnectionCleanupGlobals(main, buffers, pending, sync); bool changed = false, interrupted = false; var calls = new List<string>();
            /// <summary>최초지정close효과뒤의현재표/깊이변경과명시중단만적용한다.</summary>
            void Effect(char kind, uint token)
            {
                calls.Add($"{kind}:{token}");
                if (X86Fixture.UInt(row[2]) != 0 && !changed && kind == 'D') { changed = true; main[21] = Nodes + 384; pending[31] = Nodes + 384; sync.Synchronization = Nodes + 384; sync.PendingCount = uint.MaxValue; server = 0; Node(nodes, token).Depth = 7; }
                if (row[3] == kind.ToString()) throw new EffectStop();
            }
            var hooks = new ConnectionCleanupHooks { Node = token => Node(nodes, token), Destroy = token => Effect('D', token), Release = token => Effect('F', token), Assertion = line => Effect('A', line) };
            int argument = X86Fixture.Int(row[5]);
            try
            {
                switch (X86Fixture.UInt(row[0]))
                {
                    case 0: ConnectionCleanup.DeferConnectionClose(nodes[0], () => Effect('D', Nodes), () => Effect('F', Nodes)); break;
                    case 1: ConnectionCleanup.ClosePendingConnection(argument, globals, hooks); break;
                    case 2: ConnectionCleanup.DisconnectTransceiver(argument, globals, hooks); break;
                    case 3: ConnectionCleanup.CloseAllTransceivers(globals, hooks); break;
                    case 4: ConnectionCleanup.EndPlayerConnections(server, globals, hooks); break;
                    case 5: ConnectionCleanup.CloseAllPendingConnections(globals, hooks); break;
                    default: Assert.Fail("미등록원본정리연산"); break;
                }
            }
            catch (EffectStop) { interrupted = true; stopped++; }
            // 모든현재표를기대출력과독립적으로직렬화한다.
            for (int i = 0; i < main.Length; i++) { Put(bytes, i * 4, main[i]); Put(bytes, 88 + i * 4, buffers[i]); }
            // pending전체의자연보존/정리를비교한다.
            for (int i = 0; i < pending.Length; i++) Put(bytes, 176 + i * 4, pending[i]);
            Put(bytes, 304, sync.Synchronization); Put(bytes, 308, sync.PendingCount); Put(bytes, 312, server);
            // 실제노드필드이외의최초패턴은그대로유지한다.
            for (int i = 0; i < nodes.Length; i++) { int at = 316 + i * 192; Put(bytes, at + 0x34, nodes[i].Flags); Put(bytes, at + 0x40, nodes[i].Sender); Put(bytes, at + 0x44, nodes[i].Received); Put(bytes, at + 0x48, nodes[i].Heartbeat); Put(bytes, at + 0x8c, nodes[i].Depth); Assert.False(nodes[i].Delayed); }
            Assert.Equal(X86Fixture.Bytes(row[7]), bytes); Assert.Equal(row[8], calls.Count == 0 ? "-" : string.Join(';', calls)); Assert.Equal(row[9] == "1", interrupted);
        }
        Assert.Equal(268, stopped);
    }
}
