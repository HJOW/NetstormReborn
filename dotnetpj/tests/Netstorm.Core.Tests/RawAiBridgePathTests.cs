// AI 다리의 실제 단계 함수 출력과 배치/내용물/timeout 경계를 검사한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// AI 다리 경로를 검사하는 클래스다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.
// 근거: cpppj tests/AiBridgePathTests.cpp의 15건 전부다.
public sealed class RawAiBridgePathTests
{
    /// <summary>원본 fixture와 같은 소문자 16진 표현이다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="raw">바꿀 바이트다.</param>
    /// <returns>소문자 16진이다.</returns>
    private static string Hex(byte[] raw) => Convert.ToHexString(raw).ToLowerInvariant();

    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>같은 실제 본체/풀과 하위 호출의 순서·현재 값 변경을 조립한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    private sealed class BridgePathScene
    {
        /// <summary>원본 본체의 실제 소유 메모리다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x55000000u, bytes => bytes.Fill(0));

        /// <summary>현재 살아 있는 본체 핸들이다.</summary>
        public readonly uint Token;

        /// <summary>내용물 체인의 실제 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768);

        /// <summary>현재 내용물 TYPE/권한이다.</summary>
        public readonly ContainedFinderState Finder = new();

        /// <summary>QWORD 기한의 정밀도를 확인하는 현재 시각이다.</summary>
        public double Now = 100.00000001;

        /// <summary>원본 지도 상한이다.</summary>
        public int Limit = 256;

        /// <summary>각 원본 전역 TYPE/로그 값이다.</summary>
        public uint Bridge = 82;

        /// <summary>각 원본 전역 TYPE/로그 값이다.</summary>
        public uint Kind = 1;

        /// <summary>각 원본 전역 TYPE/로그 값이다.</summary>
        public uint StartMarker = 93;

        /// <summary>각 원본 전역 TYPE/로그 값이다.</summary>
        public uint EndMarker = 94;

        /// <summary>각 원본 전역 TYPE/로그 값이다.</summary>
        public uint TargetMarker = 95;

        /// <summary>실제 하위 경계의 명시 입력이다.</summary>
        public AiBridgePathHooks Hooks = new();

        /// <summary>하위 진입 순서다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>소비한 실제 내용물 번호다.</summary>
        public readonly List<Sid> Destroyed = [];

        /// <summary>native fixture와 같은 false/true/현재 상태 변경 입력이다.</summary>
        public uint Mode;

        /// <summary>시작점 하위가 반환하는 명시 좌표다.</summary>
        public RawPathPoint First = new(20, 21);

        /// <summary>효과 뒤 현재 본체/전역 변경이다.</summary>
        public Action? DuringDeploy;

        /// <summary>효과 뒤 현재 본체/전역 변경이다.</summary>
        public Action? DuringTechnology;

        /// <summary>효과 뒤 현재 본체/전역 변경이다.</summary>
        public Action? DuringDestroy;

        /// <summary>효과 뒤 현재 본체/전역 변경이다.</summary>
        public Action? DuringRefresh;

        /// <summary>효과 뒤 현재 본체/전역 변경이다.</summary>
        public Action? DuringMarker;

        /// <summary>다음에 할당할 SID 번호다.</summary>
        private ushort _nextSid = 5;

        /// <summary>실제 부모/내용물 다섯 슬롯을 생성하고 원문 단계 하위만 연결한다.</summary>
        public BridgePathScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            // 연속된 실제 SID의 링크/필드 초기값을 명확히 하는 반복문이다.
            for (ushort number = 5; number <= 9; number++)
            {
                ushort sid = _nextSid++;
                Assert.Equal(number, sid);
                Span<byte> raw = Pool.RawSlot(sid);
                raw.Fill(0);
                raw[RawSidPool.TypeOffset] = number == 5 ? (byte)1 : (byte)6;
            }

            Field(0x260, 1);
            Field(0x3d8, 7);
            Point(0x6bc, new RawPathPoint(101, 202));
            Point(0x6ac, new RawPathPoint(11, 12));
            Field(0x6b4, 3);
            Field(0x6b8, 2);
            Field(0x600, BitConverter.SingleToUInt32Bits(0.25f));
            Hooks.Initialize = (brain, source, target) =>
            {
                Assert.Equal(Token, brain);
                Assert.Equal(10f, source.X);
                Assert.Equal(20f, source.Y);
                Assert.Equal(30f, target.X);
                Assert.Equal(40f, target.Y);
                Trace.Add("initialize");
                if (Mode == 2)
                {
                    Field(0x5f0, 0xffffffffu);
                }
            };
            Hooks.Prepare = brain =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("prepare");
                if (Mode == 2)
                {
                    Field(0x5f0, 0xffffffffu);
                }

                return Mode != 0;
            };
            Hooks.Search = brain =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("search");
                if (Mode == 2)
                {
                    Field(0x5f0, 0xffffffffu);
                }

                return Mode != 0;
            };
            Hooks.Start = (brain, _) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("start");
                return First;
            };
            Hooks.Deploy = (brain, type, point, pattern, direction) =>
            {
                Assert.Equal(Token, brain);
                Assert.Equal(Bridge, type);
                Assert.Equal(11f, point.X);
                Assert.Equal(12f, point.Y);
                Assert.Equal(3u, pattern);
                Assert.Equal(2u, direction);
                Trace.Add("deploy");
                DuringDeploy?.Invoke();
            };
            Hooks.Technology = owner =>
            {
                Trace.Add("technology" + owner);
                DuringTechnology?.Invoke();
                return new Sid(5);
            };
            Hooks.Destroy = (sid, flags) =>
            {
                Assert.Equal(0u, flags);
                Trace.Add("destroy");
                Destroyed.Add(sid);
                DuringDestroy?.Invoke();
            };
            Hooks.Refresh = (brain, parent) =>
            {
                Assert.Equal(Token, brain);
                Assert.Equal(new Sid(5), parent);
                Trace.Add("refresh");
                DuringRefresh?.Invoke();
            };
            Hooks.Marker = (brain, type, _) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("marker" + type);
                DuringMarker?.Invoke();
            };
            Hooks.Log = (brain, logKind, text) =>
            {
                Assert.Equal(Token, brain);
                Assert.Equal(Kind, logKind);
                Assert.Equal("Bridge timeout\n", text);
                Trace.Add("timeout");
            };
        }

        /// <summary>지정한 실제 본체 DWORD를 읽는다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
        /// <param name="at">읽을 위치다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Field(int at) => Get(Memory.Bytes(Token), at);

        /// <summary>주변 BYTE를 보존하며 실제 본체 DWORD를 쓴다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), at, value);

        /// <summary>두 float의 원문 비트를 기록한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="point">쓸 좌표다.</param>
        public void Point(int at, RawPathPoint point)
        {
            Field(at, BitConverter.SingleToUInt32Bits(point.X));
            Field(at + 4, BitConverter.SingleToUInt32Bits(point.Y));
        }

        /// <summary>다음 링크/패턴/현재 flags를 지정한 내용물 슬롯에 기록한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
        /// <param name="number">쓸 슬롯 번호다.</param>
        /// <param name="next">쓸 다음 번호다.</param>
        /// <param name="pattern">쓸 패턴 WORD다.</param>
        /// <param name="flags">쓸 flags WORD다.</param>
        public void Content(ushort number, ushort next, uint pattern, uint flags)
        {
            Span<byte> raw = Pool.RawSlot(number);
            BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(4, 2), next);
            BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(12, 2), (ushort)pattern);
            BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(20, 2), (ushort)flags);
        }

        /// <summary>같은 부모의 첫 링크를 기록한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
        /// <param name="number">기록할 첫 번호다.</param>
        public void Head(ushort number) => BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(5).Slice(6, 2), number);

        /// <summary>현재 전역 참조와 같은 풀을 사용하는 복원 모듈을 만든다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
        /// <returns>검사할 다리 경로 몸체다.</returns>
        public RawAiBridgePath Path() => new(Memory, Pool, Finder, new AiBridgePathGlobals
        {
            Now = () => Now,
            MapLimit = () => Limit,
            BridgeType = () => Bridge,
            LogKind = () => Kind,
            StartMarker = () => StartMarker,
            EndMarker = () => EndMarker,
            TargetMarker = () => TargetMarker,
        }, Hooks);
    }

    /// <summary>초기화/실제 CRT의 144입력·두 x87 정밀도 288반환에서 동결한 전체 0xb08 바이트를 대조한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void NativeInitialize144FullBodies()
    {
        string[][] rows = X86Fixture.Read("ai-bridge-initialize");
        int count = 0;
        // source/target/count/pattern은 caller 입력이며 전체 기대 본체를 재계산하지 않는 반복문이다.
        foreach (string[] row in rows)
        {
            Assert.Equal(7, row.Length);
            var scene = new BridgePathScene();
            byte[] raw = scene.Memory.Bytes(scene.Token);
            raw.AsSpan().Fill((byte)X86Fixture.UInt(row[5]));
            scene.Field(0x628, X86Fixture.UInt(row[4]));
            var source = new RawPathPoint(X86Fixture.FloatBits(row[0]), X86Fixture.FloatBits(row[1]));
            var target = new RawPathPoint(X86Fixture.FloatBits(row[2]), X86Fixture.FloatBits(row[3]));
            Assert.True(scene.Path().Initialize(scene.Token, source, target));
            Assert.Equal(row[6], Hex(raw));
            count++;
        }

        Assert.Equal(144, count);
    }

    /// <summary>실제 초기화 콜백을 연결했을 때 Step은 본체 준비 뒤 현재 상태를 한 단계만 증가시킨다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void StepUsesActualInitializationBody()
    {
        var scene = new BridgePathScene();
        scene.Field(0x5f0, 1);
        scene.Field(0x628, 0);
        scene.Hooks.Initialize = (token, source, target) =>
        {
            Assert.True(scene.Path().Initialize(token, source, target));
        };
        var source = new RawPathPoint(20.75f, -21.75f);
        Assert.False(scene.Path().Step(scene.Token, ref source, new RawPathPoint(30.5f, 40.5f)));
        Assert.Equal(2u, scene.Field(0x5f0));
        Assert.Equal(20u, scene.Field(0x638));
        Assert.Equal(unchecked((uint)-21), scene.Field(0x63c));
        Assert.Equal(30u, scene.Field(0x640));
        Assert.Equal(40u, scene.Field(0x644));
        Assert.Equal(0xffffffffu, scene.Field(0x6c8));
        Assert.Equal(20.75f, source.X);
        Assert.Equal(-21.75f, source.Y);
        Assert.Equal(0xc61c4000u, scene.Field(0x6bc));
        Assert.Equal(0xc61c4000u, scene.Field(0x6c0));
    }

    /// <summary>실제 원본 명령의 600개 입력/정상 반환과 하위 진입 순서를 재생한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void NativeStep600Inputs()
    {
        string[][] rows = X86Fixture.Read("ai-bridge-step");
        int count = 0;
        // 각 행은 독립 본체/원본 입력이며 기대 상태를 C#에서 재계산하지 않는 반복문이다.
        foreach (string[] row in rows)
        {
            Assert.Equal(9, row.Length);
            var scene = new BridgePathScene();
            scene.Field(0x5f0, X86Fixture.UInt(row[0]));
            scene.Field(0x604, X86Fixture.UInt(row[1]));
            scene.Field(0x628, X86Fixture.UInt(row[2]));
            scene.Mode = X86Fixture.UInt(row[3]);
            var source = new RawPathPoint(10, 20);
            bool done = scene.Path().Step(scene.Token, ref source, new RawPathPoint(30, 40));
            Assert.Equal(X86Fixture.UInt(row[4]), scene.Field(0x5f0));
            Assert.Equal(row[5] != "0", done);
            Assert.Equal(X86Fixture.UInt(row[6]), BitConverter.SingleToUInt32Bits(source.X));
            Assert.Equal(X86Fixture.UInt(row[7]), BitConverter.SingleToUInt32Bits(source.Y));
            Assert.Equal(row[8], scene.Trace.Count == 0 ? "-" : scene.Trace[0]);
            count++;
        }

        Assert.Equal(600, count);
    }

    /// <summary>pending/다른 TYPE/다른 패턴을 건너뛰며 첫 사용 가능 조각만 삭제한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void ConsumeFiltersAndFirstMatch()
    {
        var scene = new BridgePathScene();
        scene.Head(6);
        scene.Content(6, 7, 3, 0x140);
        scene.Content(7, 8, 4, 0x40);
        scene.Content(8, 9, 3, 0x40);
        scene.Content(9, 0, 3, 0x40);
        scene.Path().Consume(scene.Token, new Sid(5), 3);
        Assert.Equal([new Sid(8)], scene.Destroyed);
        double due = BitConverter.UInt64BitsToDouble(((ulong)scene.Field(0x634) << 32) | scene.Field(0x630));
        Assert.Equal(scene.Now + 0.25, due);
    }

    /// <summary>패턴 인자는 DWORD 전체이므로 65536을 WORD 0으로 잘라 소비하지 않는다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void ConsumeWholeDwordAndNoMatchPreservesDeadline()
    {
        var scene = new BridgePathScene();
        scene.Head(6);
        scene.Content(6, 0, 0, 0x40);
        scene.Field(0x630, 123);
        scene.Field(0x634, 456);
        scene.Path().Consume(scene.Token, new Sid(5), 65536);
        Assert.Empty(scene.Trace);
        Assert.Equal(123u, scene.Field(0x630));
        Assert.Equal(456u, scene.Field(0x634));
    }

    /// <summary>삭제 효과 뒤 현재 rate와 double now를 사용한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void ConsumeReadsCurrentDeadlineAfterDeletion()
    {
        var scene = new BridgePathScene();
        scene.Head(6);
        scene.Content(6, 0, 3, 0x40);
        scene.DuringDestroy = () =>
        {
            scene.Now = 123.00000001;
            scene.Field(0x600, BitConverter.SingleToUInt32Bits(0.125f));
        };
        scene.Path().Consume(scene.Token, new Sid(5), 3);
        Assert.Equal(123.12500001, BitConverter.UInt64BitsToDouble(((ulong)scene.Field(0x634) << 32) | scene.Field(0x630)));
    }

    /// <summary>상태가 완료되지 않았으면 입력 시작점과 모든 배치/소비 효과를 보존한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void ExtendWaitDoesNotDeploy()
    {
        var scene = new BridgePathScene();
        scene.Field(0x5f0, 4);
        scene.Field(0x604, 8);
        scene.Field(0x628, 7);
        RawPathPoint result = scene.Path().Extend(scene.Token, new RawPathPoint(10, 20), new RawPathPoint(30, 40));
        Assert.Equal(10f, result.X);
        Assert.Equal(20f, result.Y);
        Assert.Empty(scene.Trace);
        Assert.Equal(4u, scene.Field(0x5f0));
    }

    /// <summary>배치 뒤 현재 owner/패턴을 소비하고 다시 현재 owner로 재고를 갱신한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void ExtendUsesCurrentFieldsAfterEffects()
    {
        var scene = new BridgePathScene();
        scene.Field(0x5f0, 5);
        scene.Head(6);
        scene.Content(6, 0, 7, 0x40);
        scene.DuringDeploy = () =>
        {
            scene.Field(0x260, 2);
            scene.Field(0x6b4, 7);
            scene.Point(0x6bc, new RawPathPoint(220, 221));
        };
        scene.DuringDestroy = () => scene.Field(0x260, 3);
        RawPathPoint result = scene.Path().Extend(scene.Token, new RawPathPoint(10, 20), new RawPathPoint(30, 40));
        Assert.Equal(101f, result.X);
        Assert.Equal(202f, result.Y);
        Assert.Equal(["deploy", "technology2", "destroy", "technology3", "refresh"], scene.Trace);
        Assert.Equal(0u, scene.Field(0x5f0));
    }

    /// <summary>무효 결과/경계/NaN은 상태 0 복귀만 유지하고 배치·기술 조회를 생략한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void InvalidCompletionSkipsAllEffects()
    {
        // 각 좌표는 원본 strict 0..상한 검사에서 독립적으로 거부되는 반복문이다.
        foreach (RawPathPoint point in new RawPathPoint[]
        {
            new(0, 20), new(256, 20), new(10, 0), new(10, 256), new(-10000, -10000),
            new(float.NaN, 20),
        })
        {
            var scene = new BridgePathScene();
            scene.Field(0x5f0, 5);
            scene.Point(0x6bc, point);
            _ = scene.Path().Extend(scene.Token, new RawPathPoint(10, 20), new RawPathPoint(30, 40));
            Assert.Empty(scene.Trace);
            Assert.Equal(0u, scene.Field(0x5f0));
        }
    }

    /// <summary>디버그 표식은 배치/소비/갱신 뒤 현재 종류를 읽으며 둘째 TYPE도 다시 읽는다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void DebugMarkersFollowRefresh()
    {
        var scene = new BridgePathScene();
        scene.Field(0x5f0, 5);
        scene.DuringRefresh = () => scene.Kind = 29;
        scene.DuringMarker = () => scene.TargetMarker = 99;
        _ = scene.Path().Extend(scene.Token, new RawPathPoint(10, 20), new RawPathPoint(30, 40));
        Assert.Equal(["deploy", "technology1", "technology1", "refresh", "marker94", "marker99"], scene.Trace);
    }

    /// <summary>시작점 없음은 성공 반환을 유지하고 timeout을 감소하지 않는다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void AdvanceMissingStartKeepsTimeout()
    {
        var scene = new BridgePathScene();
        scene.Point(0x3d0, new RawPathPoint(-10000, -10000));
        scene.First = new RawPathPoint(-10000, -10000);
        Assert.True(scene.Path().Advance(scene.Token, new RawPathPoint(30, 40)));
        Assert.Equal(7u, scene.Field(0x3d8));
        Assert.Equal(["start"], scene.Trace);
    }

    /// <summary>유효한 첫 시작점은 저장한 뒤 표식을 놓고 기존 timeout을 유지한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void AdvanceFirstPointAndMarker()
    {
        var scene = new BridgePathScene();
        scene.Kind = 29;
        Assert.True(scene.Path().Advance(scene.Token, new RawPathPoint(30, 40)));
        Assert.Equal(BitConverter.SingleToUInt32Bits(20f), scene.Field(0x3d0));
        Assert.Equal(7u, scene.Field(0x3d8));
        Assert.Equal(["start", "marker93"], scene.Trace);
    }

    /// <summary>아직 기다리는 유효 연결점도 성공으로 취급해 timeout 1을 쓴다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void AdvanceWaitingPointResetsTimeout()
    {
        var scene = new BridgePathScene();
        scene.Point(0x3d0, new RawPathPoint(10, 20));
        scene.Field(0x5f0, 4);
        scene.Field(0x604, 8);
        scene.Field(0x628, 7);
        Assert.True(scene.Path().Advance(scene.Token, new RawPathPoint(30, 40)));
        Assert.Equal(1u, scene.Field(0x3d8));
        Assert.Empty(scene.Trace);
    }

    /// <summary>무효 결과에서만 signed 감소/실패/원래 출력 관문을 적용한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void AdvanceTimeoutAndOutputGate()
    {
        // 양수 2·1·0·부호 경계의 감김과 signed 판정을 확인하는 반복문이다.
        foreach (uint timeout in new uint[] { 2u, 1u, 0u, 0x80000000u })
        {
            var scene = new BridgePathScene();
            scene.Point(0x3d0, new RawPathPoint(10, 20));
            scene.Field(0x5f0, 5);
            scene.Point(0x6bc, new RawPathPoint(-10000, -10000));
            scene.Field(0x3d8, timeout);
            scene.Field(0xb00, 1);
            bool result = scene.Path().Advance(scene.Token, new RawPathPoint(30, 40));
            bool success = timeout == 2 || timeout == 0x80000000u;
            Assert.Equal(success, result);
            Assert.Equal(timeout == 0x80000000u ? 0x7fffffffu : 1u, scene.Field(0x3d8));
            Assert.Equal(success ? [] : ["timeout"], scene.Trace);
        }

        var quiet = new BridgePathScene();
        quiet.Point(0x3d0, new RawPathPoint(10, 20));
        quiet.Field(0x5f0, 5);
        quiet.Point(0x6bc, new RawPathPoint(-10000, -10000));
        quiet.Field(0x3d8, 1);
        quiet.Hooks.Log = null;
        Assert.False(quiet.Path().Advance(quiet.Token, new RawPathPoint(30, 40)));
        Assert.Equal(1u, quiet.Field(0x3d8));
    }

    /// <summary>미연결 하위에서 앞선 상태 0 복귀/배치 효과를 취소하지 않는다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    [Fact]
    public void MissingBoundaryPreservesPriorEffects()
    {
        var scene = new BridgePathScene();
        scene.Field(0x5f0, 5);
        scene.Hooks.Technology = null;
        bool rejected = false;
        try
        {
            _ = scene.Path().Extend(scene.Token, new RawPathPoint(10, 20), new RawPathPoint(30, 40));
        }
        catch (InvalidOperationException)
        {
            rejected = true;
        }

        Assert.True(rejected);
        Assert.Equal(0u, scene.Field(0x5f0));
        Assert.Equal(["deploy"], scene.Trace);
    }
}
