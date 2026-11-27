using System.Buffers.Binary;
using Netstorm.Client.Gump;

namespace Netstorm.Client.Tests; // 예산 창의 원본 관측과 현재 실제 트리를 대조한다.

/// <summary>실제 등록·가상 호출 순서와 원본 독립 관측을 확인한다.</summary>
public sealed class StormPowerRefreshTests
{
    /// <summary>원본 가상 장치가 지정한 위치에서 중단한 상태다.</summary>
    private sealed class Stop : Exception;
    /// <summary>독립 관측의 첫 노드 토큰과 고정 간격이다.</summary>
    private const uint NodeBase = 0x12000100;
    /// <summary>비정렬 DWORD를 원본 little endian 순서로 읽는다.</summary>
    private static uint Read(byte[] bytes, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(offset, 4));
    /// <summary>변경한 DWORD만 원본 입력 사본에 다시 쓴다.</summary>
    private static void Write(byte[] bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset, 4), value);

    /// <summary>120개 원본 관측에서 전체 노드·등록표·호출 순서와 부분 실패를 대조한다.</summary>
    [Fact]
    public void MatchesNativeObservationsAndPartialEffects()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "budget-refresh-x86.tsv");
        int count = 0, stopped = 0;
        // 원본의 각 독립 입력을 새로운 실제 노드와 고정 등록표에 복원한다.
        foreach (string line in File.ReadLines(path))
        {
            if (line.Length == 0 || line[0] == '#') continue;
            string[] row = line.Split('\t'); Assert.Equal(12, row.Length);
            byte[] bytes = Convert.FromHexString(row[5]), table = Convert.FromHexString(row[6]);
            uint[] entries = new uint[GumpCreateState.RegistryCapacity];
            var nodes = new GumpCreateNode[3];
            // 입력 전체 등록표를 순서대로 재현한다.
            for (int i = 0; i < entries.Length; i++) entries[i] = Read(table, i * 4);
            // 같은 노드 객체의 원본 접근 필드만 복원한다.
            for (int i = 0; i < nodes.Length; i++)
            {
                int at = i * 192;
                nodes[i] = new() { Flags = Read(bytes, at + 4), Serial = Read(bytes, at + 0x2c), Depth = Read(bytes, at + 0x30), X = BitConverter.UInt32BitsToSingle(Read(bytes, at + 0x4c)), Y = BitConverter.UInt32BitsToSingle(Read(bytes, at + 0x50)) };
            }
            uint serial = uint.Parse(row[4]); var calls = new List<string>();
            GumpCreateNode Node(uint token)
            {
                if (token < NodeBase || (token - NodeBase) % 192 != 0 || (token - NodeBase) / 192 >= nodes.Length) throw new ArgumentOutOfRangeException(nameof(token));
                return nodes[(token - NodeBase) / 192];
            }
            void Effect(bool attach, uint token, uint x = 0, uint y = 0, uint parent = 0, uint mode = 0)
            {
                calls.Add(attach ? $"A:{token}:{x}:{y}:{parent}:{mode}" : $"P:{token}");
                var node = Node(token);
                if (row[1] != "0")
                {
                    node.Depth = attach ? 0xfedcba98u : 0x89abcdefu;
                    if (!attach) { node.X = BitConverter.UInt32BitsToSingle(0xc2300000); node.Y = BitConverter.UInt32BitsToSingle(0x42b00000); serial = 0xfedcba98; }
                }
                if (row[2] == (attach ? "attach" : "push")) throw new Stop();
            }
            var hooks = new StormPowerRefreshHooks
            {
                Node = Node, Push = token => Effect(false, token),
                Attach = (token, x, y, parent, mode) => Effect(true, token, BitConverter.SingleToUInt32Bits(x), BitConverter.SingleToUInt32Bits(y), parent, mode),
            };
            bool interrupted = false;
            try { StormPowerRefresh.Refresh(entries, serial, hooks); }
            catch (Stop) { interrupted = true; stopped++; }
            // 실제로 쓸 수 있는 세 필드만 전체 입력 바이트에 되돌린다.
            for (int i = 0; i < nodes.Length; i++)
            {
                int at = i * 192; Write(bytes, at + 0x30, nodes[i].Depth); Write(bytes, at + 0x4c, BitConverter.SingleToUInt32Bits(nodes[i].X)); Write(bytes, at + 0x50, BitConverter.SingleToUInt32Bits(nodes[i].Y));
            }
            Assert.Equal(Convert.FromHexString(row[7]), bytes);
            Assert.Equal(uint.Parse(row[8]), serial);
            Assert.Equal(calls.Count == 0 ? "-" : string.Join(';', calls), row[9]);
            Assert.Equal(row[10] == "1", interrupted);
            // 등록표는 조회만 하며 어떤 칸도 바꾸지 않는다.
            for (int i = 0; i < entries.Length; i++) Assert.Equal(Read(table, i * 4), entries[i]);
            count++;
        }
        Assert.Equal(120, count); Assert.Equal(48, stopped);
    }

    /// <summary>대상이 없으면 가상 장치를 요구하지 않고 늦은 실패는 이전 효과를 보존한다.</summary>
    [Fact]
    public void MissingTargetAndLateFailurePreserveEffects()
    {
        uint[] entries = new uint[GumpCreateState.RegistryCapacity];
        StormPowerRefresh.Refresh(entries, 0, new());
        Assert.Throws<ArgumentException>(() => StormPowerRefresh.Find(entries.AsSpan(0, 999), 0, null));
        var node = new GumpCreateNode { Serial = 71, Depth = 19, X = 3, Y = -5 }; entries[1] = 37;
        var missingPush = new StormPowerRefreshHooks { Node = token => token == 37 ? node : null };
        Assert.Throws<InvalidOperationException>(() => StormPowerRefresh.Refresh(entries, 71, missingPush));
        var missingAttach = new StormPowerRefreshHooks { Node = _ => node, Push = _ => { node.Depth = 29; node.X = 7; } };
        Assert.Throws<InvalidOperationException>(() => StormPowerRefresh.Refresh(entries, 71, missingAttach));
        Assert.Equal(29u, node.Depth); Assert.Equal(7, node.X);
        var complete = new StormPowerRefreshHooks { Node = _ => node, Push = _ => { node.Depth = 29; node.X = 7; }, Attach = (_, x, y, parent, mode) => { Assert.Equal((7f, -5f, 0u, 0u), (x, y, parent, mode)); node.Depth = 31; } };
        node.Depth = 19; StormPowerRefresh.Refresh(entries, 71, complete); Assert.Equal(19u, node.Depth);
    }
}
