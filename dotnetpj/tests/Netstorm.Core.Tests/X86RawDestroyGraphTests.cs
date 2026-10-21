using System.Globalization;
using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 공통 삭제 그래프 재생(<see cref="RawGraph"/> + <see cref="RawSquidDestroyLifecycle"/>)을 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/DestroyGraphTests.cpp 의 Scene 과 같다: 실제 슬롯 풀·4단계 해시·점유 지도·그래프 표 위에서
/// 사전 파괴(표면 분할·Free·특수 H 치환)와 사후 파괴(발자국→일반 탐색→순차 Add)를 실행한다.
/// 호출마다 효과 문자열·슬롯 바이트·풀/해시/점유 체크섬·장부·좌표·그래프 표 255행·flood 스택 체크섬까지 비교한다.
/// 기준 판본은 10.78 이며 originalCD·original1037 행은 읽지 않는다 (W03 삭제·수명 네째 조각, 2026-10-21).
/// </summary>
public sealed class X86RawDestroyGraphTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 10.72·원본 10.37 다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 한 절의 행 수(Pre·Post·Destroy 각각 96 행 × 판본 빌드 2종).</summary>
    private const int RowsPerKind = 192;

    /// <summary>섬 H 프레임 치환 절의 10.78 행 수(Pre 12·Destroy 12).</summary>
    private const int IslandRows = 24;

    /// <summary>장면이 확보하는 클라이언트 슬롯 7개.</summary>
    private static readonly int[] SlotIds = [5, 6, 7, 8, 9, 10, 11];

    /// <summary>슬롯의 기본 가상 표 값(10.78 기본 Squid).</summary>
    private const uint BaseVtable = 0x00501DD0;

    /// <summary>슬롯 +8 에 넣는 값(원본 헤더 크기).</summary>
    private const uint HeaderSize = 23;

    /// <summary>특수 표면 타입 번호(원본 157).</summary>
    private const int SpecialSurfaceType = 157;

    /// <summary>특수 표면의 치환 타입 번호(원본 162).</summary>
    private const int SpecialSurfaceReplacementType = 162;

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>DWORD 를 물리 메모리와 같은 낮은 바이트 순서로 늘어놓는다.</summary>
    /// <param name="bytes">늘어놓을 바이트 목록.</param>
    /// <param name="value">쓸 값.</param>
    private static void Append(List<byte> bytes, uint value)
    {
        // 네 바이트를 낮은 자리부터 붙인다
        bytes.Add((byte)value);
        bytes.Add((byte)(value >> 8));
        bytes.Add((byte)(value >> 16));
        bytes.Add((byte)(value >> 24));
    }

    /// <summary>네 단계 해시 머리를 0단계부터 16비트 리틀 엔디언으로 늘어놓는다.</summary>
    /// <param name="hash">공간 해시.</param>
    private static byte[] HeadBytes(RawSquidHash hash)
    {
        var bytes = new List<byte>();
        // 단계 0~3 순서로 모든 머리를 쓴다
        for (int level = 0; level < RawSquidHash.Levels; level++)
        {
            // 머리마다 두 바이트
            foreach (ushort value in hash.Entries(level))
            {
                bytes.Add((byte)value);
                bytes.Add((byte)(value >> 8));
            }
        }
        return bytes.ToArray();
    }

    /// <summary>삭제 기록(클라이언트→서버)을 DWORD 리틀 엔디언으로 늘어놓는다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    private static byte[] LogBytes(RawSidPool pool)
    {
        var bytes = new List<byte>();
        // 클라이언트 기록부터 서버 기록 순서로 쓴다
        foreach (bool client in new[] { true, false })
        {
            // 기록마다 번호·타입을 DWORD 로 쓴다
            foreach (SidDeletion record in pool.Deletions(client))
            {
                // 번호·타입을 네 바이트씩 쓴다
                foreach (uint value in new[] { record.Sid, record.Type })
                {
                    Append(bytes, value);
                }
            }
        }
        return bytes.ToArray();
    }

    /// <summary>
    /// 원본 결과를 계산하지 않는 중복/미사용 꼬리 포함 목록 입력이다.
    /// 여섯 패턴의 활성 항목 수와 겹침 위치를 원본 시험과 같은 순서로 만든다.
    /// </summary>
    /// <param name="list">채울 목록.</param>
    /// <param name="pattern">패턴 번호(0~5).</param>
    /// <param name="baseValue">목록 항목의 기준 값.</param>
    private static void ListInput(RawPostPopList list, int pattern, uint baseValue)
    {
        // 여섯 패턴의 활성 항목 수다
        uint[] counts = [0, 1, 5, 6, 4, 3];
        list.Count = counts[pattern];
        list.Entries = new uint[6];
        // 각 목록의 기준 값을 순서대로 넣는다
        for (int i = 0; i < 6; i++)
        {
            list.Entries[i] = baseValue + (uint)i;
        }
        // 패턴마다 중복 위치가 다르다
        if (pattern == 1)
        {
            list.Entries[0] = 5;
        }
        if (pattern == 3)
        {
            list.Entries[0] = 5;
            list.Entries[2] = 5;
            list.Entries[5] = 5;
        }
        if (pattern == 4)
        {
            // 앞 네 칸을 같은 번호로 채운다
            for (int i = 0; i < 4; i++)
            {
                list.Entries[i] = 5;
            }
        }
        if (pattern == 5)
        {
            list.Entries[1] = 5;
        }
    }

    /// <summary>쉼표로 나눈 정수 목록을 읽는다. 빈 칸과 "-" 는 빈 목록이다.</summary>
    /// <param name="text">쉼표로 나눈 칸.</param>
    /// <returns>각 항목의 정수 목록.</returns>
    private static List<int[]> Nodes(string text)
    {
        var result = new List<int[]>();
        if (text == "-")
        {
            return result;
        }
        // 각 객체/spot 의 필드 순서를 유지한다
        foreach (string entry in text.Split(';'))
        {
            result.Add(entry.Split(',').Select(v => int.Parse(v, CultureInfo.InvariantCulture)).ToArray());
        }
        return result;
    }

    /// <summary>little endian hex 에서 부호 있는 세 WORD 를 패딩 없이 해독한다.</summary>
    /// <param name="hex">그래프 표의 hex 문자열.</param>
    /// <returns>255개 레코드.</returns>
    private static GraphRecord[] Records(string hex)
    {
        Assert.Equal(RawGraph.TableSize * 12, hex.Length);
        var result = new GraphRecord[RawGraph.TableSize];
        // sentinel/reserved 를 포함한 모든 항목을 읽는다
        for (int i = 0; i < result.Length; i++)
        {
            var words = new short[3];
            // 두 hex byte 의 부호 비트를 보존한다
            for (int j = 0; j < 3; j++)
            {
                int pos = i * 12 + j * 4;
                ushort low = ushort.Parse(hex.Substring(pos, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture);
                ushort high = ushort.Parse(hex.Substring(pos + 2, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture);
                words[j] = unchecked((short)(low | (high << 8)));
            }
            result[i] = new GraphRecord { Surfaces = words[0], InUse = words[1], Reserved = words[2] };
        }
        return result;
    }

    /// <summary>한 삭제 그래프 행의 풀·해시·점유 지도·그래프를 소유하는 재생 장면.</summary>
    private sealed class Scene
    {
        /// <summary>삭제 대상 슬롯 풀(32,768칸, 서버).</summary>
        public readonly RawSidPool Pool;

        /// <summary>4단계 공간 해시.</summary>
        public readonly RawSquidHash Hash = new();

        /// <summary>점유 지도(65,536칸).</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>타입 표(10.78 188개).</summary>
        public readonly OriginalTypeRecord[] Types;

        /// <summary>타입별 프레임 코드 표(10.78 188개).</summary>
        public readonly FrameCode[][] Frames;

        /// <summary>자산 뿌리의 공간 해제기.</summary>
        public readonly RawUnpop Unpop;

        /// <summary>삭제 본체.</summary>
        public readonly RawSquidDestroy Destroy;

        /// <summary>공유 장부.</summary>
        public readonly RawPostPopState Bookkeeping = new();

        /// <summary>삭제 상태.</summary>
        public readonly SquidDeletionState State = new();

        /// <summary>표면 그래프.</summary>
        public readonly RawGraph Graph;

        /// <summary>삭제 수명주기.</summary>
        public readonly RawSquidDestroyLifecycle Lifecycle;

        /// <summary>나간 효과 문자열.</summary>
        public readonly List<string> Events = [];

        /// <summary>삭제에 적용되는 플래그 비트.</summary>
        public uint Flags;

        /// <summary>행 입력(25칸)·노드 7개·점유 지도·그래프 표로 장면을 준비한다.</summary>
        /// <param name="input">쉼표로 나눈 25개 입력값.</param>
        /// <param name="nodes">각 슬롯의 13개 필드.</param>
        /// <param name="initialSpots">점유 지도 초기 값(칸 x·칸 y·값).</param>
        /// <param name="records">그래프 표의 hex 문자열.</param>
        /// <param name="islandFrame">참이면 섬 H 프레임(9→1) 치환용 프레임 표를 쓴다.</param>
        public Scene(string[] input, List<int[]> nodes, List<int[]> initialSpots, string records, bool islandFrame)
        {
            Assert.Equal(25, input.Length);
            Assert.Equal(7, nodes.Count);
            Pool = new RawSidPool(32768, true);
            Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 타입 번호 전체에 빈 타입 레코드를 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            Frames = new FrameCode[Types.Length][];
            // 비어 있는 타입의 프레임 표는 빈 목록이다
            for (int i = 0; i < Frames.Length; i++)
            {
                Frames[i] = [];
            }
            Unpop = new RawUnpop(Pool, Hash, Spots);
            Destroy = new RawSquidDestroy(Pool, Unpop, Types);
            // 부모 실제 Create와 같은 클라이언트 할당만 준비한다
            for (int i = 0; i < SlotIds.Length; i++)
            {
                Assert.Equal(SlotIds[i], Pool.Allocate(SidAllocation.Client));
            }
            // 뿌리와 주변 여섯 슬롯의 타입·프레임·좌표·공간 체인을 원본 시험과 같은 순서로 넣는다
            for (int i = 0; i < nodes.Count; i++)
            {
                int[] n = nodes[i];
                Assert.Equal(13, n.Length);
                int number = i == 0 ? (int)X86Fixture.UInt(input[0]) : 74 + i;
                OriginalTypeRecord type = Types[number];
                type.Flags1 = (uint)n[4];
                type.Flags2 = (uint)n[5];
                type.FootX = n[2];
                type.FootY = n[3];
                type.Cost = 1.25f;
                Frames[number] =
                [
                    new FrameCode((char)n[6], (char)n[7], 1, FrameCodeFlags.None),
                    new FrameCode('A', 'P', 2, FrameCodeFlags.None),
                ];
                Span<byte> raw = Pool.AllocatedBytes(SlotIds[i]);
                raw.Clear();
                Put(raw, 0, BaseVtable);
                Put(raw, 8, HeaderSize, 2);
                raw[10] = (byte)number;
                raw[11] = (byte)n[9];
                Put(raw, 14, BitConverter.SingleToUInt32Bits((float)n[0]));
                Put(raw, 18, BitConverter.SingleToUInt32Bits((float)n[1]));
                raw[30] = (byte)n[11];
                raw[33] = (byte)n[12];
                raw[34] = 1;
                Put(raw, 36, (uint)n[8], 4);
                raw[40] = (byte)n[10];
                // 공간에서 빠진 슬롯은 해시 체인에 넣지 않는다
                if ((n[9] & 4) == 0)
                {
                    ref ushort head = ref Hash.Bucket(n[12], (float)n[0], (float)n[1]);
                    Put(raw, 4, head, 2);
                    head = (ushort)SlotIds[i];
                }
            }
            // 뿌리의 타입 번호는 입력 첫 칸이고 그룹 값도 따로 들어 있다
            int rootNumber = (int)X86Fixture.UInt(input[0]);
            Types[rootNumber].Group = X86Fixture.Int(input[4]);
            // 특수 표면의 치환 타입(162) 준비는 대상 타입이 162 가 아닐 때만 한다
            if (rootNumber != SpecialSurfaceReplacementType)
            {
                OriginalTypeRecord replacement = Types[SpecialSurfaceReplacementType];
                replacement.Flags1 = OriginalTypeRecord.Surface;
                replacement.Flags2 = X86Fixture.UInt(input[24]);
                replacement.FootX = X86Fixture.Int(input[23]);
                replacement.FootY = X86Fixture.Int(input[23]);
                replacement.Cost = 1.25f;
                Frames[SpecialSurfaceReplacementType] =
                [
                    new FrameCode('A', 'P', 1, FrameCodeFlags.None),
                    new FrameCode('A', 'P', 2, FrameCodeFlags.None),
                ];
            }
            // interior 바이트는 해시/타입 입력과 별개다
            foreach (int[] spot in initialSpots)
            {
                Assert.Equal(3, spot.Length);
                Spots[spot[1] * RawSquidHash.WorldCells + spot[0]] = (byte)spot[2];
            }
            Flags = X86Fixture.UInt(input[8]);
            State.Selected = input[9] == "1" ? SlotIds[0] : 0;
            Bookkeeping.LocalOwner = (byte)X86Fixture.Int(input[6]);
            Bookkeeping.Suppressed = input[10] == "1";
            State.UnitLostSuppressed1 = input[11] == "1";
            State.UnitLostSuppressed2 = input[12] == "1";
            State.SilentType = X86Fixture.UInt(input[13]);
            int pattern = X86Fixture.Int(input[14]);
            Bookkeeping.TotalCost = X86Fixture.Int(input[15]);
            Bookkeeping.ProductionDirty = X86Fixture.UInt(input[16]);
            Bookkeeping.Depth = 3;
            ListInput(Bookkeeping.Providers, pattern, 0x60000000);
            ListInput(Bookkeeping.Factories, pattern, 0x61000000);
            // 원본 Player 0의 미사용 목록까지 같은 입력을 넣는다
            for (int i = 0; i < RawPostPopState.OwnerCount; i++)
            {
                ListInput(Bookkeeping.OwnerFactories[i], pattern, 0x70000000 + (uint)i * 100);
            }
            uint seed = X86Fixture.UInt(input[18]);
            // 통계 표 전체의 비대상 타입 보존과 DWORD 감김도 비교한다
            for (int i = 0; i < RawPostPopState.TableSize; i++)
            {
                Bookkeeping.LocalCounts[i] = unchecked(seed + (uint)i * 2654435761U);
                Bookkeeping.LocalSecondaryCounts[i] = unchecked(Bookkeeping.LocalCounts[i] + 17U);
                Bookkeeping.GlobalCounts[i] = unchecked(Bookkeeping.LocalCounts[i] + 34U);
            }
            State.PlacementX = -7.25f;
            State.PlacementY = 8.5f;
            State.LostX = 90.25f;
            State.LostY = -91.5f;
            Bookkeeping.GraphsEnabled = input[19] == "1";
            State.RebuildGraph = input[20] == "1";
            State.RegionMask = X86Fixture.UInt(input[21]);
            var stack = new uint[RawGraph.FloodSize];
            uint graphSeed = X86Fixture.UInt(input[22]);
            // 전체 스택의 오래된 DWORD를 native 입력과 맞춘다
            for (int i = 0; i < stack.Length; i++)
            {
                stack[i] = graphSeed + (uint)i;
            }
            // 실제 157의 물리 프레임 H=8 과 162의 단일 프레임을 기존 합성 입력과 구별하여 재현한다
            if (islandFrame)
            {
                var island = new FrameCode[9];
                // 여덟 칸은 같은 A/P 프레임이고 마지막만 H 다
                for (int i = 0; i < island.Length; i++)
                {
                    island[i] = new FrameCode('A', 'P', 1, FrameCodeFlags.None);
                }
                island[8] = new FrameCode('H', 'P', 9, FrameCodeFlags.None);
                Frames[SpecialSurfaceType] = island;
                Frames[SpecialSurfaceReplacementType] = [new FrameCode('A', 'P', 1, FrameCodeFlags.None)];
            }
            Graph = new RawGraph(Pool, Hash, Spots, Types, Records(records), stack, Frames);
            Lifecycle = new RawSquidDestroyLifecycle(Pool, Types, Bookkeeping, State, Destroy, new SquidDeletionHooks
            {
                Emit = Emit,
                Forward = Forward,
            }, Graph);
        }

        /// <summary>사전·사후 함수 진입 때 raw 상태를 별도로 관찰한다.</summary>
        /// <param name="tag">기록 표시 문자.</param>
        /// <param name="sid">대상 슬롯 번호.</param>
        /// <param name="value">호출 플래그.</param>
        private void Entry(char tag, int sid, uint value)
        {
            Events.Add($"{tag}:{sid}:{value}:{Pool.Slot(sid)[11]}");
        }

        /// <summary>삭제 수명주기가 낸 외부 효과를 기록한다.</summary>
        /// <param name="effect">나온 효과.</param>
        private void Emit(SquidDeletionEvent effect)
        {
            switch (effect.Effect)
            {
                case SquidDeletionEffect.ClearSelection:
                    Events.Add($"S:{State.Selected}");
                    break;
                case SquidDeletionEffect.Refund:
                    Events.Add($"Q:{effect.Sid}:{effect.Recipient}");
                    break;
                case SquidDeletionEffect.UnitLostSound:
                    Events.Add($"L:{effect.Sid}");
                    break;
                default:
                    throw new InvalidOperationException("알 수 없는 삭제 수명주기 효과");
            }
        }

        /// <summary>공통 몸체가 다루지 않는 종속·전파 효과를 기록한다.</summary>
        /// <param name="effect">나온 파생 효과.</param>
        private void Forward(SquidDestroyEvent effect)
        {
            if (effect.Effect != SquidDestroyEffect.Transmit)
            {
                throw new InvalidOperationException("장면에 없는 종속 가상 효과");
            }
            Events.Add($"T:{effect.Sid}:{effect.Flags}");
        }

        /// <summary>
        /// 직접 전/후 훅의 깊이 underflow와 통합 삭제의 균형 깊이를 서로 구별해 실행한다.
        /// </summary>
        /// <param name="kind">실행할 절("Pre"·"Post"·"Destroy").</param>
        public void Run(string kind)
        {
            if (kind == "Pre")
            {
                Entry('P', SlotIds[0], Flags);
                Lifecycle.PreDestroy(SlotIds[0], Flags);
            }
            else if (kind == "Post")
            {
                Entry('O', SlotIds[0], Flags);
                Lifecycle.PostDestroy(SlotIds[0], Flags);
            }
            else
            {
                SquidDestroyHooks hooks = Lifecycle.Hooks();
                Action<SquidDestroyEvent> emit = hooks.Emit!;
                // 전체 삭제 안에서도 공통 훅 몸체 자체를 호출한다
                hooks.Emit = (SquidDestroyEvent effect) =>
                {
                    if (effect.Effect == SquidDestroyEffect.PreDestroy)
                    {
                        Entry('P', effect.Sid, effect.Flags);
                    }
                    if (effect.Effect == SquidDestroyEffect.PostDestroy)
                    {
                        Entry('O', effect.Sid, effect.Flags);
                    }
                    emit(effect);
                };
                Destroy.Destroy(SlotIds[0], Flags, hooks);
            }
        }

        /// <summary>모든 목록의 inactive DWORD와 누적 생산 표를 포함한 장부 결과다.</summary>
        public long[] BookkeepingValues()
        {
            var values = new List<long> { Bookkeeping.TotalCost, Bookkeeping.ProductionDirty, Bookkeeping.Depth };
            // 서로 다른 세 타입 표를 정해진 순서로 해시한다
            foreach (uint[] table in new[] { Bookkeeping.LocalCounts, Bookkeeping.LocalSecondaryCounts, Bookkeeping.GlobalCounts })
            {
                var bytes = new byte[table.Length * 4];
                // 표 전체를 DWORD 리틀 엔디언으로 늘어놓는다
                for (int i = 0; i < table.Length; i++)
                {
                    Put(bytes, i * 4, table[i]);
                }
                values.Add(X86Fixture.Adler32(bytes));
            }
            var data = new List<byte>();
            // 공급원/작업장 다음에 Player 0~8 전체 목록을 관찰한다
            foreach (RawPostPopList list in new[] { Bookkeeping.Providers, Bookkeeping.Factories })
            {
                values.Add(list.Count);
                // 목록의 inactive 칸까지 모두 늘어놓는다
                foreach (uint value in list.Entries)
                {
                    Append(data, value);
                }
            }
            // 소유자별 작업장 목록을 같은 순서로 기록한다
            foreach (RawPostPopList list in Bookkeeping.OwnerFactories)
            {
                values.Add(list.Count);
                // 목록의 inactive 칸까지 모두 늘어놓는다
                foreach (uint value in list.Entries)
                {
                    Append(data, value);
                }
            }
            values.Add(X86Fixture.Adler32(data.ToArray()));
            return values.ToArray();
        }

        /// <summary>그래프 표의 flood 스택 체크섬을 구한다. 활성 범위 밖의 오래된 값도 포함한다.</summary>
        public uint StackAdler()
        {
            var bytes = new byte[RawGraph.FloodSize * 4];
            // 스택 전체를 DWORD 리틀 엔디언으로 늘어놓는다
            for (int i = 0; i < RawGraph.FloodSize; i++)
            {
                Put(bytes, i * 4, Graph.FloodStack[i]);
            }
            return X86Fixture.Adler32(bytes);
        }

        /// <summary>외부 효과와 raw 슬롯 바이트·전체 풀/공간·장부·선택 좌표·그래프 표를 모두 대조한다.</summary>
        /// <param name="row">fixture 행(14칸).</param>
        public void Check(string[] row)
        {
            Assert.Equal(14, row.Length);
            string text = Events.Count == 0 ? "-" : string.Join(';', Events);
            Assert.Equal(row[4], text);
            // 뿌리와 주변 일곱 슬롯은 체크섬 대신 직접 바이트도 확인한다
            foreach (string entry in row[5].Split(';'))
            {
                string[] fields = entry.Split(':');
                int sid = X86Fixture.Int(fields[0]);
                Assert.Equal(fields[1], Convert.ToHexStringLower(Pool.Slot(sid).ToArray()));
            }
            uint[] expected = row[6].Split(',').Select(X86Fixture.UInt).ToArray();
            uint[] actual =
            [
                Pool.FreeCount,
                Pool.PredictableCursor,
                (uint)Pool.FirstFree(true),
                (uint)Pool.FirstFree(false),
                (uint)Pool.Tail(true),
                (uint)Pool.Tail(false),
                X86Fixture.Adler32(Pool.Bytes),
                X86Fixture.Adler32(HeadBytes(Hash)),
                X86Fixture.Adler32(Spots),
                X86Fixture.Adler32(LogBytes(Pool)),
                (uint)State.Selected,
                Destroy.PreDepth,
                Destroy.PostDepth,
            ];
            Assert.Equal(expected, actual);
            long[] book = BookkeepingValues();
            long[] bookExpected = row[7].Split(',').Select(X86Fixture.Long).ToArray();
            Assert.Equal(bookExpected, book);
            uint[] context = row[8].Split(',').Select(X86Fixture.UInt).ToArray();
            uint[] coordinates =
            [
                State.PlacementType,
                BitConverter.SingleToUInt32Bits(State.PlacementX),
                BitConverter.SingleToUInt32Bits(State.PlacementY),
                BitConverter.SingleToUInt32Bits(State.LostX),
                BitConverter.SingleToUInt32Bits(State.LostY),
            ];
            Assert.Equal(context, coordinates);
            Assert.Equal(State.PlacementType != 0, Bookkeeping.PendingPlacement);
            // 그래프 표 255행은 세 WORD(표면 수·사용 여부·예약)를 직접 비교한다
            GraphRecord[] expectedRecords = Records(row[12]);
            Assert.Equal(RawGraph.TableSize, Graph.Records.Count);
            for (int i = 0; i < expectedRecords.Length; i++)
            {
                Assert.Equal(expectedRecords[i].Surfaces, Graph.Records[i].Surfaces);
                Assert.Equal(expectedRecords[i].InUse, Graph.Records[i].InUse);
                Assert.Equal(expectedRecords[i].Reserved, Graph.Records[i].Reserved);
            }
            Assert.Equal(X86Fixture.UInt(row[13]), StackAdler());
        }
    }

    /// <summary>한 절의 10.78 행만 골라 장면으로 만들어 실행하고 기대값과 대조한다.</summary>
    /// <param name="rows">fixture 전체 행.</param>
    /// <param name="kind">실행할 절 이름.</param>
    /// <param name="count">기대 행 수.</param>
    private static void Replay(string[][] rows, string kind, int count)
    {
        int played = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != kind || row[1] != PatchEdition)
            {
                return true;
            }
            var scene = new Scene(row[3].Split(','), Nodes(row[9]), Nodes(row[10]), row[11], false);
            scene.Run(kind);
            scene.Check(row);
            played++;
            return true;
        });
        Assert.Equal(count, played);
    }

    /// <summary>
    /// 원본 pre 의 표면 분할·Free·특수 H 치환·선택/장부 순서를 비교한다.
    /// </summary>
    [Fact]
    public void PreMatchOriginalX86()
    {
        Replay(X86Fixture.Read("destroygraph"), "Pre", RowsPerKind);
    }

    /// <summary>
    /// 발자국 경계·네 해시 단계·중복 머리·inactive 표면과 순차 Add를 비교한다.
    /// </summary>
    [Fact]
    public void PostMatchOriginalX86()
    {
        Replay(X86Fixture.Read("destroygraph"), "Post", RowsPerKind);
    }

    /// <summary>
    /// actual destroy→pre Graph/장부→Unpop→post Graph/비용→Release 전체를 비교한다.
    /// </summary>
    [Fact]
    public void DestroyMatchOriginalX86()
    {
        Replay(X86Fixture.Read("destroygraph"), "Destroy", RowsPerKind);
    }

    /// <summary>
    /// 실제 9→1 프레임 치환에서 원본 pre/Destroy의 풀·장부·그래프 전체를 비교한다.
    /// </summary>
    [Fact]
    public void IslandH8AndSingleFrameReplacementMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("island-graph-frame");
        int played = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[1] != PatchEdition)
            {
                return true;
            }
            var scene = new Scene(row[3].Split(','), Nodes(row[9]), Nodes(row[10]), row[11], true);
            scene.Run(row[0]);
            scene.Check(row);
            played++;
            return true;
        });
        Assert.Equal(IslandRows, played);
    }
}
