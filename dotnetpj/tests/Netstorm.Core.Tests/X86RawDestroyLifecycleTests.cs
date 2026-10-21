using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 공통 삭제 수명주기(<see cref="RawSquidDestroyLifecycle"/>)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/DestroyLifecycleTests.cpp 의 Scene 과 같다: 실제 슬롯 풀·해시·점유 지도 위에서
/// 선택 해제·보상·소리 효과와 공급원/작업장 목록 압축·타입별 통계 감소·비용 차감을 실행하고,
/// Pair 절은 사전·사후를 직접, Destroy 절은 공통 삭제 본체(004af780)를 통해 실행한다.
/// 호출마다 효과 문자열·슬롯 바이트·풀/해시/점유 체크섬·장부·좌표를 비교한다.
/// 기준 판본은 10.78 이며 원본1037·CD 행은 읽지 않는다 (W03 삭제·수명 셋째 조각, 2026-10-21).
/// </summary>
public sealed class X86RawDestroyLifecycleTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 원본1037·CD 다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 Pair 행 수(사전·사후 직접 호출, 빌드 2종).</summary>
    private const int PairRowsPerEdition = 384;

    /// <summary>10.78 Destroy 행 수(공통 삭제 본체 경유, 빌드 2종).</summary>
    private const int DestroyRowsPerEdition = 192;

    /// <summary>장면이 확보하는 클라이언트 슬롯 7개.</summary>
    private static readonly int[] SlotIds = [5, 6, 7, 8, 9, 10, 11];

    /// <summary>슬롯의 기본 가상 표 값(10.78 기본 Squid).</summary>
    private const uint BaseVtable = 0x00501DD0;

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
                    bytes.Add((byte)value);
                    bytes.Add((byte)(value >> 8));
                    bytes.Add((byte)(value >> 16));
                    bytes.Add((byte)(value >> 24));
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

    /// <summary>한 삭제 수명주기 행의 풀·해시·점유 지도·장면을 소유하는 재생 장면.</summary>
    private sealed class Scene
    {
        /// <summary>삭제 대상 슬롯 풀(32,768칸, 서버).</summary>
        public readonly RawSidPool Pool;

        /// <summary>공간 해시.</summary>
        public readonly RawSquidHash Hash = new();

        /// <summary>점유 지도(65,536칸, 항상 0).</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>타입 표(10.78 188개).</summary>
        public readonly OriginalTypeRecord[] Types;

        /// <summary>자산 뿌리의 공간 해제기.</summary>
        public readonly RawUnpop Unpop;

        /// <summary>삭제 본체.</summary>
        public readonly RawSquidDestroy Destroy;

        /// <summary>공유 장부.</summary>
        public readonly RawPostPopState Bookkeeping = new();

        /// <summary>삭제 상태.</summary>
        public readonly SquidDeletionState State = new();

        /// <summary>삭제 수명주기.</summary>
        public readonly RawSquidDestroyLifecycle Lifecycle;

        /// <summary>나간 효과 문자열.</summary>
        public readonly List<string> Events = [];

        /// <summary>삭제에 적용되는 플래그 비트.</summary>
        public uint Flags;

        /// <summary>행 입력(19칸)으로 장면을 준비한다.</summary>
        /// <param name="input">쉼표로 나눈 19개 입력값.</param>
        public Scene(string[] input)
        {
            Assert.Equal(19, input.Length);
            Pool = new RawSidPool(32768, true);
            Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 타입 번호 전체에 빈 타입 레코드를 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            Unpop = new RawUnpop(Pool, Hash, Spots);
            Destroy = new RawSquidDestroy(Pool, Unpop, Types);
            // 부모 실제 Create와 같은 클라이언트 할당만 준비한다
            for (int i = 0; i < SlotIds.Length; i++)
            {
                Assert.Equal(SlotIds[i], Pool.Allocate(SidAllocation.Client));
            }
            // 같은 위치의 0단계(뿌리)·1단계(주변) 버킷 체인을 넣는다
            for (int i = 0; i < SlotIds.Length; i++)
            {
                Span<byte> raw = Pool.AllocatedBytes(SlotIds[i]);
                raw.Clear();
                Put(raw, 0, BaseVtable);
                raw[10] = (byte)(74 + i);
                Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f));
                Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f));
                Types[74 + i].FootX = 1;
                Types[74 + i].FootY = 1;
                ref ushort head = ref Hash.Bucket(i == 0 ? 0 : 1, 20.75f, 21.9f);
                Put(raw, 4, head, 2);
                head = (ushort)SlotIds[i];
            }
            int number = X86Fixture.Int(input[0]);
            OriginalTypeRecord type = Types[number];
            type.Flags1 = X86Fixture.UInt(input[1]);
            type.Flags2 = X86Fixture.UInt(input[2]);
            type.Cost = BitConverter.UInt32BitsToSingle(X86Fixture.UInt(input[3]));
            type.Group = X86Fixture.Int(input[4]);
            type.FootX = 1;
            type.FootY = 1;
            Span<byte> root = Pool.AllocatedBytes(SlotIds[0]);
            root[10] = (byte)number;
            root[11] = input[17] == "1" ? (byte)4 : (byte)0;
            root[34] = (byte)X86Fixture.Int(input[5]);
            root[40] = (byte)X86Fixture.Int(input[7]);
            // 뿌리가 공간에서 빠졌으면 0단계 버킷 머리를 비운다
            if ((root[11] & 4) != 0)
            {
                Hash.Bucket(0, 20.75f, 21.9f) = 0;
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
            Lifecycle = new RawSquidDestroyLifecycle(Pool, Types, Bookkeeping, State, Destroy, new SquidDeletionHooks
            {
                Emit = Emit,
                Forward = Forward,
            });
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
        /// 직접 훅 쌍의 깊이 underflow와 통합 삭제의 균형 깊이를 서로 구별해 실행한다.
        /// </summary>
        /// <param name="pair">true 면 사전·사후를 직접 부르고, false 면 공통 삭제 본체를 탄다.</param>
        public void Run(bool pair)
        {
            if (pair)
            {
                Entry('P', SlotIds[0], Flags);
                Lifecycle.PreDestroy(SlotIds[0], Flags);
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
            // 공급원/작업장 다음에 Player 0~8 전체 목록을 관찰한다
            var data = new List<byte>();
            // 공급원과 작업장 목록의 개수와 항목을 기록한다
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

        /// <summary>외부 효과와 raw 슬롯 바이트·전체 풀/공간·장부·선택 좌표를 모두 대조한다.</summary>
        /// <param name="row">fixture 행(9칸).</param>
        public void Check(string[] row)
        {
            Assert.Equal(9, row.Length);
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
        }
    }

    /// <summary>
    /// 삭제 수명주기 fixture 의 10.78 Pair 몫 384회를 재생한다.
    /// 호출마다 효과 순서·슬롯 바이트·풀/해시/점유 체크섬·장부·좌표를 대조한다.
    /// </summary>
    [Fact]
    public void PairMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("destroylifecycle");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != "Pair" || row[1] != PatchEdition)
            {
                return true;
            }
            var scene = new Scene(row[3].Split(','));
            scene.Run(true);
            scene.Check(row);
            count++;
            return true;
        });
        Assert.Equal(PairRowsPerEdition, count);
    }

    /// <summary>
    /// 삭제 수명주기 fixture 의 10.78 Destroy 몫 192회를 공통 삭제 본체 경유로 재생한다.
    /// 삭제 순서(죽음 표시→종속→전송·선택→공간 해제→사후)와 장부 결과를 대조한다.
    /// </summary>
    [Fact]
    public void DestroyMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("destroylifecycle");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != "Destroy" || row[1] != PatchEdition)
            {
                return true;
            }
            var scene = new Scene(row[3].Split(','));
            scene.Run(false);
            scene.Check(row);
            count++;
            return true;
        });
        Assert.Equal(DestroyRowsPerEdition, count);
    }
}
