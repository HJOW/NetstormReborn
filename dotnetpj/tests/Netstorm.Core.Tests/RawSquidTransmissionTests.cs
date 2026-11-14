using System.Buffers.Binary;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 독립 C++ 모듈 관측과 실제 공통 버퍼·현재 대리자 경계를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>35BYTE writer·압축·선택자·반환 크기와 현재 대상 분배를 창/네트워크 없이 검사한다.</summary>
public sealed class RawSquidTransmissionTests
{
    /// <summary>writer가 물리 슬롯만 읽는 원문 server 번호 입력이다.</summary>
    private const int Target = 15000;

    /// <summary>원본 폭의 입력만 쓰고 이웃 바이트는 보존한다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 명시한 원문 폭만 기록한다.
        for (int i = 0; i < width; i++) raw[offset + i] = unchecked((byte)(value >> (i * 8)));
    }

    /// <summary>C++ 프로브와 동일한 입력만 만들고 실제 writer의 결과는 재계산하지 않는다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 PATCH 물리 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>같은 칸의 현재 필드를 빌리는 TYPE 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>전체 버퍼와 누적을 보존하는 현재 상태다.</summary>
        public readonly RawSquidTransmissionState State = new();
        /// <summary>실제 writer·분배기다.</summary>
        public readonly RawSquidTransmission Writer;
        /// <summary>실제 방송이 받은 양수 반환 크기다.</summary>
        public int Sent;
        /// <summary>TYPE 조회 횟수다.</summary>
        public int TypeReads;
        /// <summary>현재 기본 타입 조회 횟수다.</summary>
        public int DefaultReads;
        /// <summary>현재 최대 HP 조회 횟수다.</summary>
        public int HpReads;
        /// <summary>누적 경계의 실제 로그다.</summary>
        public readonly StringBuilder Logs = new();

        /// <summary>저장한 행 번호의 경계 입력을 현재 C++ 프로브와 같은 순서로 공급한다.</summary>
        public Scene(uint index)
        {
            Span<byte> raw = Pool.RawSlot(Target);
            // writer 입력50BYTE의 모든 stale 바이트도 같은 독립 패턴으로 준비한다.
            for (int i = 0; i < raw.Length; i++) raw[i] = unchecked((byte)((uint)i * 11 + index));
            Put(raw, 0, index % 19 == 0 ? 0x5092d0u : 0x501dd0u); raw[10] = 74;
            raw[11] = (byte)((index % 8) | ((index & 32) != 0 ? 16u : 0)); raw[34] = (byte)(index % 9); raw[40] = unchecked((byte)(index * 11));
            uint[] xs = [0x41f00000, 0x41f20000, 0x437f0000, 0x7fc01234], ys = [0x42200000, 0x42220000, 0, 0x7f800000];
            uint[] quantities = [0, 1, 255, 256, 65535], frames = [0, 1, 256, 0x12345678];
            Put(raw, 14, xs[index % 4]); Put(raw, 18, ys[index % 4]); Put(raw, 12, quantities[(index / 4) % 5], 2);
            Put(raw, 26, (index & 32) != 0 ? 65535u : 200u, 2); Put(raw, 36, frames[(index / 16) % 4]);
            raw[35] = unchecked((byte)((index * 7) ^ ((index & 4) != 0 ? 0x80u : 0)));
            Types[74].Flags1 = (index & 8) != 0 ? 16u : 0; Types[74].ZOrder = unchecked((sbyte)(byte)(index * 7));
            // 처음 전체 버퍼를 채워 실제 반환 크기 뒤의 보존도 검사한다.
            for (int i = 0; i < State.Buffer.Length; i++) State.Buffer[i] = unchecked((byte)((uint)i * 17 + index));
            State.Compress = index % 4 == 0 ? 0 : index % 4 == 1 ? 1 : -1; State.SameOwnerEnabled = index & 2;
            State.LastOwner = (index & 1) != 0 ? raw[34] : 0xffffff9d;
            if (index % 32 == 7) State.LastOwner = (uint)raw[34] + 256;
            State.CompressedCount = (index & 128) != 0 ? 254u : 0; State.CompressedBytes = (index & 128) != 0 ? 8880u : 0;
            if (index % 32 == 31) { State.CompressedCount = uint.MaxValue; State.CompressedBytes = 0xfffffffa; }
            if (index % 64 == 63) { State.CompressedCount = 0x800000fe; State.CompressedBytes = 0x80000000; }
            State.Update = new FormPacketHeader((byte)(0x20 + index % 16), (byte)(0x81 + index % 4), (ushort)((index & 32) != 0 ? 65534 : index % 32));
            State.Compressed = new SquidCompressedPacketHeader((byte)(0x70 + index % 16), (byte)(0x81 + index % 4), (index & 64) != 0 ? 0x10002u : index % 4);
            State.Deletion.Sid = (index & 1) != 0 ? Target : 15001; State.Deletion.Token = 0xa5a51234 + index;
            Writer = new RawSquidTransmission(Pool, State, new RawSquidTransmissionHooks
            {
                Type = number => { TypeReads++; return new SquidPopTypeReference(() => Types[number]); },
                DefaultType = () =>
                {
                    DefaultReads++;
                    if ((index & 128) != 0) { Types[74] = Types[74].Clone(); Types[74].Flags1 ^= 16; }
                    return ((index & 16) != 0 ? 74u : 82u) + ((index & 64) != 0 ? 256u : 0);
                },
                MaxHp = _ => { HpReads++; if ((index & 64) != 0) Types[74].ZOrder++; return (index & 64) != 0 ? -1 : 200; },
                Broadcast = packet => Sent = packet.Length,
                Trace = (level, text) => Logs.Append(level.ToString(CultureInfo.InvariantCulture)).Append(':').Append(text),
                Assertion = (_, _, _) => throw new InvalidOperationException("프로브 원문 진단"),
            });
        }
    }

    /// <summary>원본 PE 자료와 구별한 C++ 모듈256관측의 패킷·전체 버퍼 보존·누적·효과 조회를 전수 대조한다.</summary>
    [Fact]
    public void CppModule256MatchesPacketsStateAndEffectReadOrder()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "Fixtures", "squid-wire-cpp.tsv");
        string[][] rows = File.ReadLines(path).Where(line => line.Length > 0 && !line.StartsWith('#')).Select(line => line.Split('\t')).ToArray(); Assert.Equal(256, rows.Length);
        var seen = new HashSet<uint>();
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(10, row.Length); uint index = X86Fixture.UInt(row[0]); Assert.True(seen.Add(index)); var scene = new Scene(index);
            scene.Writer.Transmit(Target, -1, index % 17 == 0 ? -2 : -1, 0, 0x12345678u ^ index);
            Assert.Equal(X86Fixture.Int(row[1]), scene.Sent); Assert.Equal(X86Fixture.UInt(row[2]), scene.State.CompressedCount);
            Assert.Equal(X86Fixture.UInt(row[3]), scene.State.CompressedBytes); Assert.Equal(X86Fixture.UInt(row[4]), scene.State.LastOwner); Assert.Equal(X86Fixture.Int(row[5]), scene.State.Deletion.Sid);
            Assert.Equal(row[6], Convert.ToHexStringLower(scene.State.Buffer.AsSpan(0, 64))); Assert.Equal("1", row[7]);
            // C++가 보존한64..2047의 원래 패턴을 바이트마다 직접 확인한다.
            for (int i = 64; i < scene.State.Buffer.Length; i++) Assert.Equal(unchecked((byte)((uint)i * 17 + index)), scene.State.Buffer[i]);
            string log = scene.Logs.ToString(); Assert.Equal(row[8], log.Length == 0 ? "-" : Convert.ToHexStringLower(Encoding.UTF8.GetBytes(log)));
            Assert.Equal(row[9], $"{scene.TypeReads},{scene.DefaultReads},{scene.HpReads}"); return true;
        });
        Assert.Equal(256, seen.Count);
    }

    /// <summary>현재 대상 연결은 writer 반환 뒤 읽고 같은 버퍼와 원문 writer 인자를 보존한다.</summary>
    [Fact]
    public void ProcessWriterPrecedesCurrentTargetAndBorrowsSameBuffer()
    {
        var pool = new RawSidPool(24000, false); BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(Target), 0x512098);
        var state = new RawSquidTransmissionState(); var calls = new List<string>();
        var hooks = new RawSquidTransmissionHooks
        {
            ProcessPacket = (sid, buffer, argument) => { Assert.Equal(Target, sid); Assert.Equal(37u, argument); calls.Add("writer"); buffer[0] = 0xaa; return 5; },
            Connected = target => { Assert.Equal(4, target); calls.Add("connected"); state.Buffer[0] = 0xbb; return true; },
            Send = (target, packet) => { Assert.Equal(4, target); calls.Add("send"); Assert.Equal(5, packet.Length); Assert.Equal(0xbb, packet.Span[0]); Assert.True(MemoryMarshal.TryGetArray(packet, out ArraySegment<byte> segment)); Assert.Same(state.Buffer, segment.Array); },
        };
        var writer = new RawSquidTransmission(pool, state, hooks); hooks.ProcessPacket = (_, _, _) => throw new InvalidOperationException("외부 함수 객체 교체");
        writer.SendTo(Target, 4, 37); Assert.Equal(new[] { "writer", "connected", "send" }, calls);
        writer.SameOwnerMode(0); Assert.Equal(0u, state.SameOwnerEnabled); Assert.Equal(0xffffff9du, state.LastOwner);
    }

    /// <summary>음수/0은 분배를 생략하고2048 경계 진단 정상 복귀 뒤 전체 예약을 Native 분배에 전달한다.</summary>
    [Theory]
    [InlineData(-1)]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(2048)]
    [InlineData(2049)]
    public void SignedSizeAndMutableReservationFollowReachedBoundaries(int size)
    {
        var pool = new RawSidPool(24000, false); BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(Target), 0x512098);
        var state = new RawSquidTransmissionState(); int sends = 0; var lines = new List<uint>();
        var writer = new RawSquidTransmission(pool, state, new RawSquidTransmissionHooks
        {
            ProcessPacket = (_, buffer, argument) => { Assert.Equal(0u, argument); buffer[4] = 0xab; return size; },
            Assertion = (_, _, line) => lines.Add(line), BroadcastMutable = buffer => { Assert.Same(state.Buffer, buffer); sends++; },
            Broadcast = _ => throw new InvalidOperationException("Native 예약 대신 제한 버퍼 선택 금지"),
        });
        if (size > 2048) Assert.Throws<ArgumentOutOfRangeException>(() => writer.Pop(Target, 1)); else writer.Pop(Target, 1);
        Assert.Equal(size > 0 && size <= 2048 ? 1 : 0, sends); Assert.Equal(0xab, state.Buffer[4]);
        Assert.Equal(size >= 2048 ? new uint[] { 0x2e3 } : [], lines);
    }

    /// <summary>실제 삭제 writer는 헤더를 먼저 쓰고 정상 복귀한 진단이 바꾼 현재 표식만 소비한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void DeletionHeaderPrecedesDiagnosticAndCurrentStamp(bool abort)
    {
        var pool = new RawSidPool(24000, false); var headers = new FormPacketHeaders(); var stamp = new FormDeletionStamp { Sid = 50, Token = 1 };
        var writer = new RawFormPacket(pool, headers, stamp); byte[] buffer = Enumerable.Repeat((byte)0xcc, 16).ToArray(); int diagnoses = 0;
        Action<string, string, uint> assertion = (expression, file, line) =>
        {
            Assert.Equal("!isClientSid()", expression); Assert.Equal(@"\Ns\O\Squid.cpp", file); Assert.Equal(0x2f6u, line);
            Assert.Equal(new byte[] { 1, 0x2e, 8, 0 }, buffer[..4]); Assert.Equal(0xcc, buffer[4]); diagnoses++;
            if (abort) throw new InvalidOperationException("명시 삭제 진단 중단");
            stamp.Token = 0x1234;
        };
        if (abort) Assert.Throws<InvalidOperationException>(() => writer.WriteDeletion(50, 0x56780000, buffer, assertion));
        else { Assert.Equal((ushort)8, writer.WriteDeletion(50, 0x56780000, buffer, assertion)); Assert.Equal(0, stamp.Sid); Assert.Equal((ushort)(0x5670 | 0x1234), BinaryPrimitives.ReadUInt16LittleEndian(buffer.AsSpan(6))); }
        Assert.Equal(1, diagnoses); if (abort) Assert.Equal(50, stamp.Sid); Assert.All(buffer[8..], value => Assert.Equal(0xcc, value));
    }
}
