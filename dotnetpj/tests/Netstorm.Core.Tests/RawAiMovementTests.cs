// AI 이동의 실제 기하/원문과 권한·경로·이름 조회 순서를 검사한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AI 이동을 검사하는 클래스다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.
// 근거: cpppj tests/AiMovementTests.cpp의 8건 전부다.
public sealed class RawAiMovementTests
{
    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>같은 실제 raw 풀/본체와 명시 경로·전송·이름 장치를 구성한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    private sealed class AiMoveScene
    {
        /// <summary>실제 부모/대상/본체의 소유 저장소다.</summary>
        public readonly RawSidPool Pool = new(32768);

        /// <summary>실제 본체 저장소다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x4a000000u, bytes => bytes.Fill(0));

        /// <summary>현재 본체 토큰이다.</summary>
        public readonly uint Token;

        /// <summary>현재 실제 슬롯이다.</summary>
        public Sid Actor;

        /// <summary>현재 실제 슬롯이다.</summary>
        public Sid Target;

        /// <summary>현재 실제 슬롯이다.</summary>
        public Sid Other;

        /// <summary>현재 권한이다.</summary>
        public bool Boss = true;

        /// <summary>명시 경로 반환이다.</summary>
        public ulong Path = 1;

        /// <summary>명시 거리 반환이다.</summary>
        public double Distance = 2;

        /// <summary>같은 헤더다.</summary>
        public readonly CarrierMoveState Packet = new();

        /// <summary>같은 기하다.</summary>
        public readonly BoardTargetState BoardState = new();

        /// <summary>현재 일반 접근점 기하다.</summary>
        public readonly RawBoardTarget Board;

        /// <summary>실제 하위 장치다.</summary>
        public AiMovementHooks Hooks = new();

        /// <summary>선택 효과/실제 순서다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>송신한 원문 패킷이다.</summary>
        public readonly List<byte[]> Packets = [];

        /// <summary>받은 경로 인자다.</summary>
        public RawPathStartRequest Started;

        /// <summary>실제 대상이다.</summary>
        public Sid Moved;

        /// <summary>절삭 좌표 로그 인자다.</summary>
        public int LogX;

        /// <summary>절삭 좌표 로그 인자다.</summary>
        public int LogY;

        /// <summary>이름 로그 인자다.</summary>
        public string SourceName = "";

        /// <summary>이름 로그 인자다.</summary>
        public string TargetName = "";

        /// <summary>다음에 할당할 SID 번호다.</summary>
        private ushort _nextSid = 5;

        /// <summary>원본 입력과 관찰할 하위만 준비한다.</summary>
        public AiMoveScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            Board = new RawBoardTarget(BoardState);
            Actor = New();
            Target = New();
            Other = New();
            Span<byte> raw = Pool.RawSlot(Actor.Value);
            raw[RawSidPool.TypeOffset] = 158;
            Put(raw, 14, BitConverter.SingleToUInt32Bits(12f));
            Put(raw, 18, BitConverter.SingleToUInt32Bits(5f));
            Field(0x260, 2);
            Field(0xb00, 1);
            Field(0x294, Target.Value);
            Hooks.Movement = type =>
            {
                Assert.Equal((byte)158, type);
                Trace.Add("movement");
                return 54u;
            };
            Hooks.Lookup = (parent, type, index, remove) =>
            {
                Assert.Equal(Actor, parent);
                Assert.Equal(54u, type);
                Assert.Equal(0u, index);
                Trace.Add(remove ? "remove" : "lookup");
                return new Sid(50);
            };
            Hooks.Footprint = sid =>
            {
                Assert.Equal(Target, sid);
                Trace.Add("footprint");
                return new SquidFootprint(10, 10, 14, 14);
            };
            Hooks.Send = (channel, bytes) =>
            {
                Assert.Equal(9, channel);
                Trace.Add("send");
                Packets.Add(bytes.ToArray());
            };
            Hooks.Sacrifice = (parent, sid) =>
            {
                Assert.Equal(Actor, parent);
                Assert.Equal(Target, sid);
                Trace.Add("sacrifice");
            };
            Hooks.Target = (parent, sid, replace, helper) =>
            {
                Assert.Equal(Actor, parent);
                Assert.Equal(0u, replace);
                Assert.Equal(0u, helper);
                Moved = sid;
                Trace.Add("target");
                return Path;
            };
            Hooks.Start = request =>
            {
                Started = request;
                Trace.Add("start");
                return Path;
            };
            Hooks.Name = sid =>
            {
                Trace.Add("name:" + sid.Value);
                return sid == Actor ? "actor" : "target" + sid.Value;
            };
            Hooks.LogTarget = (brain, source, destination) =>
            {
                Assert.Equal(Token, brain);
                SourceName = source;
                TargetName = destination;
                Trace.Add("logTarget");
            };
            Hooks.LogPoint = (brain, name, x, y) =>
            {
                Assert.Equal(Token, brain);
                SourceName = name;
                LogX = x;
                LogY = y;
                Trace.Add("logPoint");
            };
            Hooks.Distance = (point, sid) =>
            {
                Assert.Equal(12f, point.X);
                Assert.Equal(5f, point.Y);
                Assert.Equal(Target, sid);
                Trace.Add("distance");
                return Distance;
            };
        }

        /// <summary>같은 실제 슬롯의 payload만 명시 0으로 준비한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
        /// <returns>확보한 SID다.</returns>
        public Sid New()
        {
            ushort sid = _nextSid++;
            Span<byte> raw = Pool.RawSlot(sid);
            raw.Fill(0);
            return new Sid(sid);
        }

        /// <summary>선택된 현재 본체 DWORD만 쓴다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), at, value);

        /// <summary>같은 현재 권한/기하/헤더를 빌린다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
        /// <returns>검사할 이동 몸체다.</returns>
        public RawAiMovement Make() => new(Memory, Pool, () => Boss, Board, Packet, Hooks);
    }

    /// <summary>현재 TYPE→순번 0/remove를 전달하며 정지 0은 하위가 없어도 무동작이다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentPathLookupAndRemovalArguments()
    {
        var scene = new AiMoveScene();
        Assert.Equal(new Sid(50), scene.Make().MovementForm(scene.Actor));
        scene.Make().Stop(scene.Actor);
        Assert.Equal(["movement", "lookup", "movement", "remove"], scene.Trace);
        scene.Hooks = new AiMovementHooks();
        scene.Make().Stop(new Sid(0));
        Assert.Equal(4, scene.Trace.Count);
    }

    /// <summary>서버 실패/희생은 이름을 읽지 않고 성공은 대상→출발 이름을 디버그 조회 전에 읽는다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    [Fact]
    public void ServerTargetBranchesAndNameOrder()
    {
        var scene = new AiMoveScene();
        scene.Path = 0;
        scene.Make().ToTarget(scene.Token, scene.Actor, scene.Target, 0);
        Assert.Equal(["target"], scene.Trace);
        scene.Trace.Clear();
        scene.Make().ToTarget(scene.Token, scene.Actor, scene.Target, 1);
        Assert.Equal(["sacrifice"], scene.Trace);
        scene.Trace.Clear();
        scene.Path = 1;
        scene.Make().ToTarget(scene.Token, scene.Actor, scene.Target, 0);
        Assert.Equal(["target", "name:6", "name:5", "logTarget"], scene.Trace);
        Assert.Equal("actor", scene.SourceName);
        Assert.Equal("target6", scene.TargetName);
        scene.Trace.Clear();
        scene.Hooks.Name = sid =>
        {
            scene.Trace.Add("name:" + sid.Value);
            scene.Field(0xb00, 0);
            return "current";
        };
        scene.Hooks.LogTarget = null;
        scene.Make().ToTarget(scene.Token, scene.Actor, scene.Target, 0);
        Assert.Equal(["target", "name:6", "name:5"], scene.Trace);
    }

    /// <summary>비서버는 실제 접근점/현재 헤더의 18BYTE를 보내며 사용자 선택/중복 목적지 관문은 없다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    [Fact]
    public void ClientTargetUsesActualBoardAndPacket()
    {
        var scene = new AiMoveScene();
        scene.Boss = false;
        scene.Packet.Request = new FormPacketHeader(7, 0xf3, 2);
        scene.Make().ToTarget(scene.Token, scene.Actor, scene.Target, 0xdeadbeefu);
        Assert.Equal(["footprint", "send", "name:6", "name:5", "logTarget"], scene.Trace);
        Assert.Single(scene.Packets);
        byte[] bytes = scene.Packets[0];
        Assert.Equal(18, bytes.Length);
        Assert.Equal(0x73, bytes[0]);
        Assert.Equal(7, bytes[1]);
        Assert.Equal(2u, BinaryPrimitives.ReadUInt16LittleEndian(bytes.AsSpan(2, 2)));
        Assert.Equal(scene.Actor.Value, BinaryPrimitives.ReadUInt16LittleEndian(bytes.AsSpan(4, 2)));
        Assert.Equal(12f, BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(6, 4))));
        Assert.Equal(10f, BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(10, 4))));
        Assert.Equal(0xdeadbeefu, BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(14, 4)));
    }

    /// <summary>송신 효과 뒤 권한이 바뀌면 희생 우회 없이 현재 대상 이동/좌표 경로를 시작한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    [Fact]
    public void AuthorityRereadAfterSend()
    {
        var target = new AiMoveScene();
        target.Boss = false;
        target.Hooks.Send = (channel, _) =>
        {
            Assert.Equal(9, channel);
            target.Trace.Add("send");
            target.Boss = true;
        };
        target.Make().ToTarget(target.Token, target.Actor, target.Target, 1);
        Assert.Equal(["footprint", "send", "target", "name:6", "name:5", "logTarget"], target.Trace);
        var point = new AiMoveScene();
        point.Boss = false;
        point.Hooks.Send = (_, _) =>
        {
            point.Trace.Add("send");
            point.Boss = true;
        };
        point.Path = 0;
        point.Make().ToPoint(point.Token, point.Actor, new RawPathPoint(4, 5), 1);
        Assert.Equal(["send", "start"], point.Trace);
    }

    /// <summary>실제 시작 인자는 나머지 0이고 로그 좌표만 원본 signed 절삭을 적용한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    [Fact]
    public void PointStartAndFtolLogging()
    {
        var scene = new AiMoveScene();
        scene.Make().ToPoint(scene.Token, scene.Actor, new RawPathPoint(1.9f, -2.9f), 0xdeadbeefu);
        Assert.Equal((int)scene.Actor.Value, scene.Started.Parent);
        Assert.Equal(1.9f, scene.Started.Destination.X);
        Assert.Equal(-2.9f, scene.Started.Destination.Y);
        Assert.Equal(0u, scene.Started.ReplaceStart);
        Assert.Equal(0u, scene.Started.Helper);
        Assert.Equal(0u, scene.Started.TailWord);
        Assert.Equal(0u, scene.Started.TailByte);
        Assert.Equal(0u, scene.Started.Reserved);
        Assert.Equal(["start", "name:5", "logPoint"], scene.Trace);
        Assert.Equal(1, scene.LogX);
        Assert.Equal(-2, scene.LogY);
        scene.Make().ToPoint(scene.Token, scene.Actor, new RawPathPoint(float.NaN, float.PositiveInfinity), 0);
        Assert.Equal(0, scene.LogX);
        Assert.Equal(0, scene.LogY);
    }

    /// <summary>거리 2 이상과 unordered는 복귀하며 거리 효과 뒤 현재 신전 SID를 다시 읽는다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    [Fact]
    public void HomeBoundaryUnorderedAndCurrentTemple()
    {
        var scene = new AiMoveScene();
        scene.Distance = 1.999;
        scene.Make().Home(scene.Token, scene.Actor);
        Assert.Equal(["distance"], scene.Trace);
        scene.Trace.Clear();
        scene.Distance = 2;
        scene.Make().Home(scene.Token, scene.Actor);
        Assert.Equal(scene.Target, scene.Moved);
        Assert.Equal("distance", scene.Trace[0]);
        scene.Trace.Clear();
        scene.Hooks.Distance = (point, temple) =>
        {
            Assert.Equal(12f, point.X);
            Assert.Equal(5f, point.Y);
            Assert.Equal(scene.Target, temple);
            scene.Trace.Add("distance");
            scene.Field(0x294, scene.Other.Value);
            Put(scene.Pool.RawSlot(scene.Actor.Value), 14, BitConverter.SingleToUInt32Bits(99f));
            return double.NaN;
        };
        scene.Make().Home(scene.Token, scene.Actor);
        Assert.Equal(scene.Other, scene.Moved);
        Assert.Equal("distance", scene.Trace[0]);
        scene.Trace.Clear();
        scene.Field(0x294, 0);
        scene.Hooks = new AiMovementHooks();
        scene.Make().Home(scene.Token, scene.Actor);
        Assert.Empty(scene.Trace);
    }

    /// <summary>원본 거리의 음수 내부값/x float 저장/unordered x 선택을 확인한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    [Fact]
    public void FootprintDistanceSignedAndUnordered()
    {
        var rectangle = new SquidFootprint(10, 10, 14, 14);
        Assert.Equal(-2, RawAiMovement.AiDistanceToFootprint(new RawPathPoint(12, 12), rectangle));
        Assert.Equal(2, RawAiMovement.AiDistanceToFootprint(new RawPathPoint(8, 12), rectangle));
        Assert.Equal(2, RawAiMovement.AiDistanceToFootprint(new RawPathPoint(12, 8), rectangle));
        Assert.True(double.IsNaN(RawAiMovement.AiDistanceToFootprint(new RawPathPoint(float.NaN, 8), rectangle)));
        Assert.Equal(2, RawAiMovement.AiDistanceToFootprint(new RawPathPoint(8, float.NaN), rectangle));
    }

    /// <summary>누락된 이름/경로를 가짜 성공으로 바꾸지 않고 먼저 발생한 송신/경로 효과를 보존한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    [Fact]
    public void SelectedMissingEffectsKeepPrefix()
    {
        var scene = new AiMoveScene();
        scene.Hooks.Name = null;
        Assert.ThrowsAny<Exception>(() => scene.Make().ToTarget(scene.Token, scene.Actor, scene.Target, 0));
        Assert.Equal(["target"], scene.Trace);
        var client = new AiMoveScene();
        client.Boss = false;
        client.Hooks.Send = (_, bytes) =>
        {
            client.Packets.Add(bytes.ToArray());
            client.Boss = true;
        };
        client.Hooks.Start = null;
        Assert.ThrowsAny<Exception>(() => client.Make().ToPoint(client.Token, client.Actor, new RawPathPoint(4, 5), 0));
        Assert.Single(client.Packets);
    }
}
