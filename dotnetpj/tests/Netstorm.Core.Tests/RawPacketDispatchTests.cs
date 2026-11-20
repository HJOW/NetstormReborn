using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 원문 분배의 외부 효과 순서와 실제 공유 표/세션 대여 경계를 창 없이 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>잘못된 패킷 무시·현재 수신기 선택·집합/단일 차이와 반납 재진입을 구별한다.</summary>
public sealed class RawPacketDispatchTests
{
    /// <summary>계산을 대신하지 않고 시계/기록/종료의 실제 호출 인자만 관측한다.</summary>
    private sealed class Scene
    {
        /// <summary>표를 함께 빌리는 backend 판본이다.</summary>
        public OriginalEdition Edition { get; }
        /// <summary>현재 공유 표다.</summary>
        public PacketRegistrationTable Table { get; }
        /// <summary>콜백 순서와 현재 깊이를 저장하는 관측이다.</summary>
        public List<string> Trace { get; } = [];
        /// <summary>실제 세션 전체 owner를 대신하는 독립 검사 객체다.</summary>
        public object Owner { get; } = new();
        /// <summary>실제 연결의 현재 필드다.</summary>
        public PacketConnection Connection { get; } = new() { Sender = 7, Received = 11, Heartbeat = 12 };
        /// <summary>독립 장치 입력이며 분배기는 생성 시 대리자 값을 복사한다.</summary>
        public PacketDispatchHooks Hooks { get; }
        /// <summary>같은 실제 표와 현재 장치 사본을 사용하는 본체다.</summary>
        public RawPacketDispatch Dispatch { get; set; }
        /// <summary>검사 장치와 실제 표를 한 번 만들며 패킷 수신은 실행하지 않는다.</summary>
        public Scene(OriginalEdition edition = OriginalEdition.Patch1078)
        {
            Edition = edition; Table = new(edition);
            Hooks = new()
            {
                Now = () => { Trace.Add("now"); return 42; },
                Record = (connection, packet, length) => Trace.Add($"record:{packet.Span[1]}:{length}:{connection.Depth}"),
                Terminate = _ => Trace.Add("terminate"), Close = _ => Trace.Add("close"), Release = _ => Trace.Add("release"),
            };
            Dispatch = Rebuild();
        }
        /// <summary>재연결한 외부 장치를 같은 표에 연결하되 기존 backend 장치는 바꾸지 않는다.</summary>
        public RawPacketDispatch Rebuild() => new(Edition, Hooks, Table);
        /// <summary>원문의 검사 수신기를 한 칸에 등록하고 실제 인자/깊이만 관측한다.</summary>
        public void Register(byte type, byte version = 1, byte mismatch = 0, ushort length = 16)
        {
            Dispatch.Register(type, (sender, connection, packet) => Trace.Add($"receive:{sender}:{packet.Span[1]}:{connection.Depth}"));
            Dispatch.Version(type, version, mismatch); Dispatch.RequireLength(type, length);
        }
    }
    /// <summary>선언 WORD와 실제 예약 크기를 독립 입력으로 제공한다.</summary>
    private static byte[] Packet(byte type = 0x58, byte version = 1, ushort declared = 16, int size = 16)
    {
        var packet = new byte[size]; packet[0] = version; packet[1] = type;
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(2), declared); return packet;
    }
    /// <summary>완성된 실제 하위 버퍼를 순서 그대로 한 집합 예약에 넣는다.</summary>
    private static byte[] Aggregate(params byte[][] packets)
    {
        int length = 4 + packets.Sum(packet => packet.Length); byte[] result = Packet(4, 0, checked((ushort)length), length); int cursor = 4;
        // 각 완성 하위 버퍼를 그대로 복사하며 수신 처리기를 미리 실행하지 않는다.
        foreach (byte[] packet in packets) { packet.CopyTo(result, cursor); cursor += packet.Length; }
        return result;
    }
    /// <summary>보호 PE에서 복원한 판본별 범위·58h와 차이가 있는 원문 고정 길이를 확인한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 137u, 35, 9, 10, 8)]
    [InlineData(OriginalEdition.Cd1072, 126u, 24, 8, 8, 6)]
    public void OriginalRangesAndSelectedLengthWords(OriginalEdition edition, uint last, int message44, int message50, int message60, int message6d)
    {
        var scene = new Scene(edition); Assert.Equal(last, scene.Table.LastType());
        Assert.Equal(ushort.MaxValue, scene.Table.Registration(0).Length); Assert.Equal(16, scene.Dispatch.Registration(0x58).Length);
        Assert.Equal(message44, scene.Dispatch.Registration(0x2c).Length); Assert.Equal(message50, scene.Dispatch.Registration(0x50).Length);
        Assert.Equal(message60, scene.Dispatch.Registration(0x60).Length); Assert.Equal(message6d, scene.Dispatch.Registration(0x6d).Length);
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Table.Registration(last + 1)); Assert.Throws<ArgumentOutOfRangeException>(() => scene.Dispatch.Registration(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Dispatch.Register(0, null));
        Assert.Throws<ArgumentException>(() => new RawPacketDispatch(edition == OriginalEdition.Patch1078 ? OriginalEdition.Cd1072 : OriginalEdition.Patch1078, scene.Hooks, scene.Table));
    }
    /// <summary>단일은 관찰기를 부르지 않고 상위 비트를 가린 버전·선언 길이·깊이 순서만 적용한다.</summary>
    [Fact]
    public void SingleUsesDeclarationAndDoesNotInventObserverOrPhysicalLengthGate()
    {
        var scene = new Scene(); scene.Register(0x58); scene.Dispatch.Observe(0x58, _ => throw new InvalidOperationException("단일은 관찰기를 직접 호출하지 않는다"));
        Assert.True(scene.Dispatch.Dispatch(scene.Connection, Packet(version: 0x81, size: 4)));
        Assert.Equal(new[] { "now", "record:88:16:0", "receive:7:88:1" }, scene.Trace);
        Assert.Equal(42u, scene.Connection.Received); Assert.Equal(12u, scene.Connection.Heartbeat); Assert.Equal(0u, scene.Connection.Depth);
        scene.Trace.Clear(); Assert.True(scene.Dispatch.Dispatch(scene.Connection, Packet(declared: 15)));
        Assert.Empty(scene.Trace); Assert.False(scene.Dispatch.Validate(Packet(0))); Assert.False(scene.Dispatch.Validate(Packet(138)));
        Assert.False(new Scene().Dispatch.Validate(Packet())); Assert.Throws<ArgumentException>(() => scene.Dispatch.Validate(new byte[3]));
        scene.Connection.Flags = 0x40; Assert.False(scene.Dispatch.Dispatch(scene.Connection, new byte[0])); Assert.Empty(scene.Trace);
    }
    /// <summary>종료/무시 정책은 단일의 시계 전에 적용하며0은 버전 불일치를 그대로 수신한다.</summary>
    [Theory]
    [InlineData(0, 8, "terminate", false)]
    [InlineData(2, 4, "terminate", false)]
    [InlineData(0, 2, "", true)]
    [InlineData(2, 1, "", true)]
    [InlineData(0, 4, "receive", true)]
    [InlineData(2, 8, "receive", true)]
    [InlineData(1, 15, "receive", true)]
    [InlineData(2, 0, "receive", true)]
    public void SingleVersionPolicies(byte incoming, byte mismatch, string expected, bool result)
    {
        var scene = new Scene(); scene.Register(0x58, mismatch: mismatch);
        Assert.Equal(result, scene.Dispatch.Dispatch(scene.Connection, Packet(version: incoming)));
        if (expected == "receive") Assert.Equal(new[] { "now", "record:88:16:0", "receive:7:88:1" }, scene.Trace);
        else { Assert.Equal(expected.Length == 0 ? Array.Empty<string>() : [expected], scene.Trace); Assert.Equal(11u, scene.Connection.Received); }
    }
    /// <summary>6번만 heartbeat를 기록하고 system 타입의 record는 호출하지 않는다.</summary>
    [Fact]
    public void HeartbeatAndNestedCloseUseCurrentDepth()
    {
        var scene = new Scene(); scene.Register(6, length: 4); scene.Connection.Depth = 3; uint expectedDepth = 4;
        scene.Dispatch.Register(6, (_, connection, _) => { Assert.Equal(expectedDepth, connection.Depth); connection.Flags |= 0x40; });
        Assert.True(scene.Dispatch.Dispatch(scene.Connection, Packet(6, declared: 4, size: 4)));
        Assert.Equal(new[] { "now" }, scene.Trace); Assert.Equal(42u, scene.Connection.Heartbeat); Assert.Equal(11u, scene.Connection.Received);
        Assert.Equal(3u, scene.Connection.Depth); scene.Connection.Flags = 0; scene.Connection.Depth = 0; expectedDepth = 1; scene.Trace.Clear();
        Assert.False(scene.Dispatch.Dispatch(scene.Connection, Packet(6, declared: 4, size: 4)));
        Assert.Equal(new[] { "now", "close", "release" }, scene.Trace); Assert.Equal(0u, scene.Connection.Depth);
    }
    /// <summary>record가 실제 헤더와 표를 바꿔도 저장 type의 새 현재 F만 선택한다.</summary>
    [Fact]
    public void RecordReregistrationKeepsStoredTypeAndCurrentReceiver()
    {
        var scene = new Scene(); scene.Register(0x58); scene.Register(0x59); byte[] packet = Packet();
        scene.Hooks.Record = (_, _, _) =>
        {
            packet[1] = 0x59; scene.Dispatch.Register(0x58, (_, _, value) => scene.Trace.Add($"new:{value.Span[1]}"));
        };
        scene.Dispatch = scene.Rebuild(); Assert.True(scene.Dispatch.Dispatch(scene.Connection, packet));
        Assert.Equal(new[] { "now", "new:89" }, scene.Trace);
    }
    /// <summary>수신 예외는 깊이/pin을 복구하고 닫힘 정리 성공으로 바꾸지 않는다.</summary>
    [Fact]
    public void ReceiverExceptionRestoresDepthAndCallbackPin()
    {
        var scene = new Scene(); using PacketRegistrationLease lease = scene.Table.BindSession(scene.Owner); scene.Register(0x58);
        scene.Connection.Depth = 2;
        scene.Dispatch.Register(0x58, (_, connection, _) => { Assert.True(scene.Table.ConnectionCallbackActive()); connection.Flags |= 0x40; throw new FormatException("실제 수신 실패"); });
        Assert.Throws<FormatException>(() => scene.Dispatch.Dispatch(scene.Connection, Packet()));
        Assert.Equal(2u, scene.Connection.Depth); Assert.False(scene.Table.ConnectionCallbackActive()); Assert.DoesNotContain("close", scene.Trace);
    }
    /// <summary>반납 콜백은 전체 복귀까지 새 세대/전역 수정을 막고 자기 touched만 복원한다.</summary>
    [Fact]
    public void DetachedCallbackCannotMutateTableOrBindNextSessionUntilReturn()
    {
        var scene = new Scene(); scene.Register(0x58); PacketRegistration stable = scene.Dispatch.Registration(0x58);
        Action<uint>? baselineObserver = _ => { }; scene.Dispatch.Observe(0x59, baselineObserver);
        using PacketRegistrationLease lease = scene.Table.BindSession(scene.Owner);
        scene.Dispatch.Register(0x58, (_, _, _) =>
        {
            lease.Detach(); Assert.False(lease.Active); Assert.True(scene.Table.ConnectionCallbackActive());
            Assert.Same(stable, scene.Dispatch.Registration(0x58)); Assert.Same(baselineObserver, scene.Dispatch.Registration(0x59).Observer);
            Assert.Throws<InvalidOperationException>(() => scene.Table.BindSession(scene.Owner));
            Assert.Throws<InvalidOperationException>(() => scene.Dispatch.RequireLength(0x59, 4)); Assert.Throws<InvalidOperationException>(scene.Table.ResetCallbacks);
        });
        Assert.True(scene.Dispatch.Dispatch(scene.Connection, Packet())); Assert.False(scene.Table.ConnectionCallbackActive());
        scene.Trace.Clear(); Assert.True(scene.Dispatch.Dispatch(scene.Connection, Packet())); Assert.Contains("receive:7:88:1", scene.Trace);
        using PacketRegistrationLease next = scene.Table.BindSession(scene.Owner); next.Detach(); lease.Detach();
    }
    /// <summary>직접 입구는 NULL과 비정상 헤더를 그대로 전달하고 시계/깊이·observer를 추가하지 않는다.</summary>
    [Fact]
    public void DirectNullableEntryAndCanonicalSnapshotsPreserveExactBody()
    {
        var scene = new Scene(); int first = 0, second = 0;
        scene.Dispatch.RegisterCanonicalDirect(0x58, 0x453d20, (sender, connection, packet) => { Assert.Null(connection); Assert.Equal(99u, sender); Assert.Empty(packet.ToArray()); first++; });
        CanonicalPacketReceiver saved = Assert.IsType<CanonicalPacketReceiver>(scene.Dispatch.SnapshotReceiver(0x58));
        scene.Dispatch.InvokeDirect(0x58, 99, null, ReadOnlyMemory<byte>.Empty); Assert.Equal(1, first); Assert.Empty(scene.Trace);
        scene.Dispatch.RegisterCanonicalDirect(0x58, 0x453d21, (_, _, _) => second++); saved.Invoke(99, null, ReadOnlyMemory<byte>.Empty);
        Assert.Equal(2, first); Assert.Equal(0, second); Assert.Equal(0x453d20u, saved.NativeEntry);
        scene.Register(0x58); Assert.Throws<InvalidOperationException>(() => scene.Dispatch.SnapshotReceiver(0x58));
        Assert.Throws<ArgumentException>(() => scene.Dispatch.InvokeDirect(0x58, 99, null, ReadOnlyMemory<byte>.Empty));
        scene.Dispatch.Register(0x58, null); Assert.Null(scene.Dispatch.SnapshotReceiver(0x58));
        scene.Connection.Delayed = true; Assert.Equal(0, scene.Dispatch.DeliverReceiver(0x58, scene.Connection, Packet()));
        scene.Register(0x58); Assert.Throws<InvalidOperationException>(() => scene.Dispatch.DeliverReceiver(0x58, scene.Connection, Packet()));
    }
    /// <summary>승격된 장기 F/관찰/길이 정책은 대여를 나중에 다시 바꿔도 반납 뒤 보존한다.</summary>
    [Fact]
    public void CanonicalBaselineSurvivesSessionDetachAndCallbackResetKeepsLengths()
    {
        var scene = new Scene(); PacketRegistration stable = scene.Dispatch.Registration(0x58);
        using PacketRegistrationLease lease = scene.Table.BindSession(scene.Owner);
        int calls = 0; scene.Dispatch.RegisterCanonicalDirect(0x58, 0x453d20, (_, _, _) => calls++);
        scene.Dispatch.Version(0x58, 7, 3); scene.Dispatch.RequireLength(0x58, 19); scene.Dispatch.PersistCanonicalBaseline(0x58);
        scene.Dispatch.Register(0x58, (_, _, _) => throw new InvalidOperationException("반납 후 세션 F가 남음")); lease.Detach();
        scene.Dispatch.InvokeDirect(0x58, 0, null, ReadOnlyMemory<byte>.Empty); Assert.Equal(1, calls);
        Assert.Same(stable, scene.Dispatch.Registration(0x58)); Assert.Equal(7, stable.Version); Assert.Equal(3, stable.Mismatch); Assert.Equal(19, stable.Length);
        scene.Table.ResetCallbacks(); Assert.Null(stable.CanonicalReceive); Assert.Equal(0, stable.Version); Assert.Equal(19, stable.Length);
    }
    /// <summary>같은 표의 수신기와 backend 고유 시계/기록을 분리한다.</summary>
    [Fact]
    public void BackendsShareRegistrationAndOwnTheirDevices()
    {
        var scene = new Scene(); scene.Register(0x58); int records = 0;
        var other = new RawPacketDispatch(scene.Edition, new()
        {
            Now = () => 700, Record = (_, _, _) => records++, Terminate = _ => throw new InvalidOperationException("예상 밖 종료"),
            Close = _ => throw new InvalidOperationException("예상 밖 닫힘"), Release = _ => throw new InvalidOperationException("예상 밖 반납"),
        }, scene.Table);
        Assert.True(other.Dispatch(scene.Connection, Packet())); Assert.Equal(700u, scene.Connection.Received); Assert.Equal(1, records);
        Assert.Equal(new[] { "receive:7:88:1" }, scene.Trace);
    }
    /// <summary>집합은 시각→버전·기록→현재 observer0→저장 type의 현재 F 순서를 사용한다.</summary>
    [Fact]
    public void AggregateReadsCurrentObserverAndReceiverAfterRecordWithoutChangingStoredType()
    {
        var scene = new Scene(); scene.Register(0x58); scene.Register(0x59); byte[] frame = Aggregate(Packet());
        scene.Hooks.Record = (_, _, _) =>
        {
            scene.Trace.Add("record"); scene.Dispatch.ObservePacket(0x58, (packet, direction) =>
            {
                Assert.Equal(0u, direction); scene.Trace.Add("observer"); packet.Span[1] = 0x59;
                scene.Dispatch.Register(0x58, (_, connection, value) => scene.Trace.Add($"new:{value.Span[1]}:{connection.Depth}"));
            });
        };
        scene.Dispatch = scene.Rebuild(); Assert.True(scene.Dispatch.DispatchFrame(scene.Connection, frame));
        Assert.Equal(new[] { "now", "record", "observer", "new:89:1" }, scene.Trace); Assert.Equal(0x59, frame[5]);
    }
    /// <summary>집합 무시는 수신 시각을 이미 기록한 뒤 결정하며 단일과 구별한다.</summary>
    [Fact]
    public void AggregateVersionIgnoreOccursAfterClockAndRetiredValidationNeverRuns()
    {
        var scene = new Scene(); scene.Register(0x58, mismatch: 1);
        Assert.True(scene.Dispatch.DispatchFrame(scene.Connection, Aggregate(Packet(version: 2))));
        Assert.Equal(new[] { "now" }, scene.Trace); Assert.Equal(42u, scene.Connection.Received); scene.Trace.Clear();
        scene.Register(0x7f); scene.Register(0x80);
        Assert.True(scene.Dispatch.DispatchFrame(scene.Connection, Packet(0x7f))); Assert.Empty(scene.Trace);
        Assert.True(scene.Dispatch.DispatchFrame(scene.Connection, Aggregate(Packet(0x7f), Packet(0x80)))); Assert.Empty(scene.Trace);
    }
    /// <summary>손상 집합은 부분 성공으로 바꾸지 않고 실제 종료를 호출한다.</summary>
    [Fact]
    public void AggregateMalformedHeadersAndNestedFramesRemainDistinct()
    {
        var scene = new Scene(); Assert.False(scene.Dispatch.DispatchFrame(scene.Connection, Packet(4, declared: 8, size: 4)));
        Assert.Equal(new[] { "terminate" }, scene.Trace); scene.Trace.Clear();
        Assert.False(scene.Dispatch.DispatchFrame(scene.Connection, Packet(4, declared: 3, size: 4))); Assert.Equal(new[] { "terminate" }, scene.Trace); scene.Trace.Clear();
        Assert.False(scene.Dispatch.DispatchFrame(scene.Connection, Aggregate(Packet(declared: 0)))); Assert.Equal(new[] { "terminate" }, scene.Trace);
        Assert.Throws<InvalidOperationException>(() => scene.Dispatch.DispatchFrame(scene.Connection, Aggregate(Packet(4))));
    }
    /// <summary>현재 Player base를 한 번 조회하고 active 낮은 BYTE만 검사한다.</summary>
    [Fact]
    public void PlayerActiveUsesCurrentBaseAndOnlyNativeLowByte()
    {
        byte[] fallback = new byte[9 * 180], current = new byte[9 * 180]; fallback[180 + 0x50] = 1; current[180 + 0x51] = 1;
        Assert.True(RawPacketDispatch.PlayerActive(fallback, null, 1)); Assert.False(RawPacketDispatch.PlayerActive(fallback, () => current, 1));
        current[180 + 0x50] = 0x81; Assert.True(RawPacketDispatch.PlayerActive(fallback, () => current, 1));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawPacketDispatch.PlayerActive(current, null, 9));
    }
    /// <summary>C# 결과로 만들지 않은 보호 PE 정적 자료265 WORD를 두 실제 기본 표와 전수 대조한다.</summary>
    [Fact]
    public void BothCompleteLengthTablesMatchIndependentProtectedPeWords()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "packet-lengths-pe.tsv");
        var tables = new Dictionary<OriginalEdition, PacketRegistrationTable>
        { [OriginalEdition.Patch1078] = new(OriginalEdition.Patch1078), [OriginalEdition.Cd1072] = new(OriginalEdition.Cd1072) };
        var seen = new HashSet<(OriginalEdition Edition, uint Type)>();
        // 원문 PE에서 읽은 값만 기대값으로 사용하고 구현 표에서 기대값을 유도하지 않는다.
        foreach (string line in File.ReadLines(path))
        {
            if (line.StartsWith('#') || line.StartsWith("edition\t", StringComparison.Ordinal)) continue;
            string[] fields = line.Split('\t'); Assert.Equal(3, fields.Length);
            OriginalEdition edition = Enum.Parse<OriginalEdition>(fields[0]); uint type = uint.Parse(fields[1], System.Globalization.CultureInfo.InvariantCulture);
            Assert.True(seen.Add((edition, type))); Assert.Equal(ushort.Parse(fields[2], System.Globalization.CultureInfo.InvariantCulture), tables[edition].Registration(type).Length);
        }
        Assert.Equal(265, seen.Count); Assert.Equal(138, seen.Count(row => row.Edition == OriginalEdition.Patch1078)); Assert.Equal(127, seen.Count(row => row.Edition == OriginalEdition.Cd1072));
    }
    /// <summary>로컬 Poll과 집합 observer가 같은 운반 소유 배열의 부분뷰를 쓰고 송신 입력은 보존한다.</summary>
    [Fact]
    public void LocalPollPassesOwnedMutableFrameToRealAggregateObserver()
    {
        var scene = new Scene(); byte[] input = Packet(); byte[]? delivered = null;
        using var wire = new LocalMissionTransport(new byte[1620], () => true, scene.Hooks, registrations: scene.Table);
        wire.RegisterLogin((_, _, _) => throw new InvalidOperationException("프레임검사에없는로그인")); wire.Open("127.0.0.1", 6799); Assert.Equal(0, wire.Poll()); wire.AssignConnection(-1, 1);
        scene.Register(0x58); wire.Dispatch.ObservePacket(0x58, (packet, direction) =>
        {
            Assert.Equal(0u, direction); Assert.True(System.Runtime.InteropServices.MemoryMarshal.TryGetArray((ReadOnlyMemory<byte>)packet, out ArraySegment<byte> segment));
            Assert.Equal(4, segment.Offset); Assert.Equal(16, segment.Count); delivered = segment.Array; Assert.NotNull(delivered); Assert.Equal(4, delivered[1]); packet.Span[4] = 0x7b;
        });
        Assert.True(wire.Send(9, input)); Assert.Equal(0, wire.QueuedFrames()); wire.Flush(); Assert.Equal(1, wire.QueuedFrames()); Assert.Equal(1, wire.Poll()); Assert.Equal(0, wire.QueuedFrames());
        Assert.NotNull(delivered); Assert.Equal(0x7b, delivered[8]); Assert.Equal(0, input[4]); Assert.Equal(0u, wire.Server.Depth);
        wire.Dispatch.Register(0x58, null); Assert.True(wire.Send(9, input)); wire.Flush(); Assert.Equal(1, wire.Poll());
    }
}
