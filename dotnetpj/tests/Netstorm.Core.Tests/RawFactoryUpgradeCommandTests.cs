using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 단일 프로세스에서 시설 업그레이드의 순서·SharedRegular 인자·메시지 원문을 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj FactoryUpgradeCommandTests의 요청·수신 계약을 검사한다. 통신 장치는 실행하지 않는다.</summary>
public sealed class RawFactoryUpgradeCommandTests
{
    /// <summary>실제 풀과 변경 가능한 현재 전역·하위 효과를 준비한다.</summary>
    private sealed class Scene
    {
        /// <summary>시설 원문을 보유한 PATCH 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768, true, null, OriginalEdition.Patch1078);
        /// <summary>현재 시설 genus 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>선택된 실제 하위 호출 순서다.</summary>
        public readonly List<string> Trace = [];
        /// <summary>SharedRegular 부모·사건·지연 인자다.</summary>
        public readonly List<(int Parent, uint Event, float Delay)> Timers = [];
        /// <summary>실제 SP 가산 대상과 값이다.</summary>
        public readonly List<(uint Owner, float Delta)> Money = [];
        /// <summary>장치에 전달한 요청 원문이다.</summary>
        public readonly List<byte[]> Sent = [];
        /// <summary>장치에 전달한 방송 원문이다.</summary>
        public readonly List<byte[]> Broadcasts = [];
        /// <summary>현재 시설 SID다.</summary>
        public readonly int Factory;
        /// <summary>부작용을 바꿀 원본 하위 장치다.</summary>
        public readonly FactoryUpgradeCommandHooks Hooks;
        /// <summary>효과 뒤에도 현재 전역을 조회하는 장치다.</summary>
        public readonly FactoryUpgradeCommandState State;
        /// <summary>현재 서버 권한이다.</summary>
        public bool Boss = true;
        /// <summary>현재 원본 진단 선택값이다.</summary>
        public bool Checking;
        /// <summary>할당 성공 또는 실패 입력이다.</summary>
        public bool Reserve = true;
        /// <summary>float 축소 여부를 확인할 현재 QWORD 시각이다.</summary>
        public double Now = 16777216.25;

        /// <summary>시설 TYPE·owner와 관찰 장치를 같은 풀에 준비한다.</summary>
        public Scene()
        {
            Factory = Pool.Allocate(SidAllocation.Client);
            Pool.AllocatedBytes(Factory).Clear();
            Pool.AllocatedBytes(Factory)[10] = 85;
            Pool.AllocatedBytes(Factory)[34] = 2;
            Types[85].Flags2 = 0x4000;
            State = new() { Boss = () => Boss, Now = () => Now };
            State.Checking.Bind(() => Checking);
            Hooks = new()
            {
                Cost = type => { Assert.Equal(85u, type); Trace.Add("cost"); return 500; },
                AddSp = (owner, delta) => { Trace.Add("money"); Money.Add((owner, delta)); },
                ReserveShared = () => { Trace.Add("reserve"); return Reserve; },
                AddShared = (parent, ev, delay) => { Trace.Add("timer"); Timers.Add((parent, ev, delay)); },
                Send = (target, bytes) => { Assert.Equal(9u, target); Trace.Add("send"); Sent.Add(bytes.ToArray()); },
                Broadcast = bytes => { Trace.Add("broadcast"); Broadcasts.Add(bytes.ToArray()); },
                Log = (kind, text) => { Assert.Equal(4u, kind); Assert.Equal("new ftUPGRADE sharedRegPro\n", text); Trace.Add("log"); },
                Assertion = text => { Assert.Equal("factory.getGenus() & gFACTORY", text); Trace.Add("assert"); },
            };
        }
        /// <summary>제품 writer를 호출하지 않고 원문16바이트 입력과 기대값을 직접 만든다.</summary>
        public byte[] Packet(double due)
        {
            byte[] raw = [1, 0x58, 16, 0, (byte)Factory, (byte)(Factory >> 8), 0, 1, 0, 0, 0, 0, 0, 0, 0, 0];
            BinaryPrimitives.WriteUInt64LittleEndian(raw.AsSpan(8), BitConverter.DoubleToUInt64Bits(due));
            return raw;
        }
        /// <summary>같은 현재 입력을 빌리는 본체를 만든다.</summary>
        public RawFactoryUpgradeCommand Make() => new(Pool, Types, State, Hooks);
    }

    /// <summary>요청의 사건4/20초와 원문 완료시각+15초를 구분한다.</summary>
    [Fact]
    public void RequestWaitAndPacketDeadline()
    {
        var scene = new Scene();
        scene.Make().Request(scene.Factory);
        Assert.Equal(["reserve", "timer", "send"], scene.Trace);
        Assert.Equal((scene.Factory, 4u, 20f), Assert.Single(scene.Timers));
        Assert.Equal(scene.Packet(scene.Now + 15), Assert.Single(scene.Sent));
        Assert.Empty(scene.Money);
    }

    /// <summary>할당 실패에도 송신하고 부착 효과가 바꾼 현재 시각을 요청에 사용한다.</summary>
    [Fact]
    public void RequestAllocationFailureAndCurrentClock()
    {
        var scene = new Scene { Reserve = false };
        scene.Hooks.AddShared = null;
        scene.Make().Request(scene.Factory);
        Assert.Equal(["reserve", "send"], scene.Trace);
        Assert.Empty(scene.Timers);
        var changed = new Scene();
        changed.Hooks.AddShared = (_, _, _) => changed.Now += 3.25;
        changed.Make().Request(changed.Factory);
        Assert.Equal(changed.Packet(changed.Now + 15), Assert.Single(changed.Sent));
    }

    /// <summary>비용→로그→할당→사건3→정확히16바이트 방송 순서를 대조한다.</summary>
    [Fact]
    public void ReceiveCostTimerAndExactBroadcast()
    {
        var scene = new Scene();
        byte[] first = scene.Packet(scene.Now + 15);
        byte[] buffer = first.Concat(Enumerable.Repeat((byte)0xa7, 8)).ToArray();
        scene.Make().Receive(buffer);
        Assert.Equal(["cost", "money", "log", "reserve", "timer", "broadcast"], scene.Trace);
        Assert.Equal((2u, -500f), Assert.Single(scene.Money));
        Assert.Equal((scene.Factory, 3u, 15f), Assert.Single(scene.Timers));
        Assert.Equal(first, Assert.Single(scene.Broadcasts));
    }

    /// <summary>유한 기한의 최소1초와 미래 지연을 구분한다.</summary>
    [Theory]
    [InlineData(-1.0, 1f)]
    [InlineData(0.0, 1f)]
    [InlineData(1.0, 1f)]
    [InlineData(2.25, 2.25f)]
    public void ReceiveDelay(double delta, float expected)
    {
        var scene = new Scene();
        scene.Make().Receive(scene.Packet(scene.Now + delta));
        Assert.Equal(expected, Assert.Single(scene.Timers).Delay);
    }

    /// <summary>unordered 차이는 최소1초로 바꾸지 않고 NaN을 보존한다.</summary>
    [Fact]
    public void ReceiveUnorderedDelay()
    {
        var scene = new Scene();
        scene.Make().Receive(scene.Packet(double.NaN));
        Assert.True(float.IsNaN(Assert.Single(scene.Timers).Delay));
    }

    /// <summary>dead/mode 관문과 진단 후 현재 genus 재조회를 확인한다.</summary>
    [Fact]
    public void StateAndCheckingGates()
    {
        var dead = new Scene();
        dead.Pool.AllocatedBytes(dead.Factory)[11] = 3;
        dead.Make().Receive(dead.Packet(0).AsMemory(0, 6));
        Assert.Empty(dead.Trace);
        var mode = new Scene();
        byte[] packet = mode.Packet(0);
        packet[6] = 1;
        mode.Make().Receive(packet);
        Assert.Empty(mode.Trace);
        var changed = new Scene { Checking = true };
        changed.Types[85].Flags2 = 0;
        changed.Hooks.Assertion = _ => { changed.Trace.Add("assert"); changed.Types[85].Flags2 = 0x4000; };
        changed.Make().Receive(changed.Packet(changed.Now + 15));
        Assert.Equal("assert", changed.Trace[0]);
        Assert.Single(changed.Money);
    }

    /// <summary>비용·예약·부착 효과 뒤 현재 owner·시각·권한을 다시 읽는다.</summary>
    [Fact]
    public void CurrentOwnerClockAndBossAfterChildren()
    {
        var scene = new Scene();
        scene.Hooks.Cost = _ => { scene.Pool.AllocatedBytes(scene.Factory)[34] = 3; return int.MinValue; };
        scene.Hooks.ReserveShared = () => { scene.Now += 1; return true; };
        scene.Hooks.AddShared = (parent, ev, delay) => { scene.Timers.Add((parent, ev, delay)); scene.Boss = false; };
        scene.Make().Receive(scene.Packet(scene.Now + 15));
        Assert.Equal((3u, 2147483648f), Assert.Single(scene.Money));
        Assert.Equal(14f, Assert.Single(scene.Timers).Delay);
        Assert.Empty(scene.Broadcasts);
    }

    /// <summary>할당 실패는 비용·방송을 되돌리지 않고 누락은 앞선 실제 효과를 보존한다.</summary>
    [Fact]
    public void AllocationFailureAndSelectedMissingEffects()
    {
        var failed = new Scene { Reserve = false };
        failed.Hooks.AddShared = null;
        failed.Make().Receive(failed.Packet(failed.Now + 15));
        Assert.Single(failed.Money);
        Assert.Empty(failed.Timers);
        Assert.Single(failed.Broadcasts);
        var missing = new Scene();
        missing.Hooks.ReserveShared = null;
        Assert.Throws<InvalidOperationException>(() => missing.Make().Receive(missing.Packet(missing.Now + 15)));
        Assert.Equal(["cost", "money", "log"], missing.Trace);
        var shortPacket = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => shortPacket.Make().Receive(shortPacket.Packet(shortPacket.Now).AsMemory(0, 8)));
        Assert.Single(shortPacket.Money);
    }

    /// <summary>예약 실패·비권한이면 사용하지 않는 시각 칸을 읽지 않는다.</summary>
    [Fact]
    public void UnselectedTimestampIsNotRead()
    {
        var scene = new Scene { Reserve = false, Boss = false };
        scene.Make().Receive(scene.Packet(scene.Now).AsMemory(0, 8));
        Assert.Single(scene.Money);
        Assert.Empty(scene.Timers);
        Assert.Empty(scene.Broadcasts);
    }

    /// <summary>공통 연구 writer의 BYTE 절삭·QWORD 비트와 버퍼 꼬리 보존을 확인한다.</summary>
    [Fact]
    public void SharedWriterPreservesTailAndFieldWidths()
    {
        byte[] buffer = Enumerable.Repeat((byte)0xa7, 24).ToArray();
        Assert.Equal(16, FactoryResearchCommand.Write(0x1234, 0x1ff, -0.0, buffer));
        Assert.Equal(new byte[] { 1, 0x58, 16, 0, 0x34, 0x12, 255, 0 }, buffer[..8]);
        Assert.Equal(BitConverter.DoubleToUInt64Bits(-0.0), BinaryPrimitives.ReadUInt64LittleEndian(buffer.AsSpan(8)));
        Assert.All(buffer[16..], value => Assert.Equal((byte)0xa7, value));
        Assert.Throws<ArgumentOutOfRangeException>(() => FactoryResearchCommand.Write(1, 0, 0, new byte[15]));
    }
}
