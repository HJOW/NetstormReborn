using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 피해가능객체(Damageable) 종속 해방을 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/DamageableReleaseTests.cpp 의 ReleaseScene 과 같다: 실제 슬롯 풀·spot 지도·타입 표 위에서
/// 사제 생성과 일반 자산 생성·WORD 복사·소유자 지정·Pop 을 실행하고, "D" 절은 Damageable 를 직접,
/// "C" 절은 Carrier 경유로 부른다.
/// 호출마다 외부 호출 순서·부모/후보 슬롯 50바이트 ×5·새 슬롯 50바이트 ×4·두 종류의 현재 전역을 비교한다.
/// 기준 판본은 10.78 이며 originalCD·original1037 행은 읽지 않는다 (W03 삭제·수명 여섯째 조각, 2026-10-21).
/// </summary>
public sealed class X86RawDamageableReleaseTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 10.72·원본 10.37 다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 한 판본의 행 수(D 752 + C 752).</summary>
    private const int RowsPerEdition = 1504;

    /// <summary>장면이 확보하는 슬롯의 마지막 번호.</summary>
    private const int LastSlot = 103;

    /// <summary>해방 부모 슬롯 번호.</summary>
    private const int Source = 50;

    /// <summary>장면이 비교하는 다섯 슬롯 번호(부모와 후보 네 개).</summary>
    private static readonly int[] SlotIds = [50, 60, 61, 62, 63];

    /// <summary>생성 경계가 돌려주는 새 슬롯 번호.</summary>
    private static readonly int[] NewIds = [100, 101, 102, 103];

    /// <summary>실행기 안의 합성 가상 표 기록값. 호스트 주소로 역참조하지 않는다.</summary>
    private const uint ReleaseVtable = 0x20000100;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

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

    /// <summary>한 해방 행의 풀·spot 지도·타입 표·전역을 소유하는 재생 장면.</summary>
    private sealed class Scene
    {
        /// <summary>해방 대상 슬롯 풀(24,000칸, 서버 아님).</summary>
        public readonly RawSidPool Pool;

        /// <summary>칸별 점유 지도(65,536칸).</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>타입 표(10.78 188개).</summary>
        public readonly OriginalTypeRecord[] Types;

        /// <summary>내용물 검색기의 권한·타입 전역.</summary>
        public readonly ContainedFinderState FinderState = new();

        /// <summary>사제 해방 소리 판정 전역.</summary>
        public readonly DamageablePreDestroyState PreState = new();

        /// <summary>전투·허용 genus·다리 타입 전역.</summary>
        public readonly DamageableReleaseState State = new();

        /// <summary>나간 외부 호출 순서.</summary>
        public readonly List<string> Events = [];

        /// <summary>생성 경계가 돌려준 새 슬롯의 개수.</summary>
        public int Born { get; private set; }

        /// <summary>입력 변경이 이미 들어갔는지.</summary>
        public bool Changed { get; private set; }

        /// <summary>입력 변경 구분(0 이면 없음).</summary>
        public uint Change { get; private set; }

        /// <summary>장면을 만든다. 클라이언트 번호 5~103 을 먼저 확보한다.</summary>
        public Scene()
        {
            Pool = new RawSidPool(24000, false);
            // 실행기와 같은 번호 범위만 확보한다
            for (int id = 5; id <= LastSlot; id++)
            {
                Assert.Equal(id, Pool.Allocate(SidAllocation.Client));
            }
            Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 타입 번호 전체에 빈 타입 레코드를 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            FinderState.CheckingDead = true;
        }

        /// <summary>슬롯 바이트를 직접 쓴다(장면 준비용).</summary>
        /// <param name="sid">대상 슬롯 번호.</param>
        /// <returns>쓰기 가능한 슬롯 바이트.</returns>
        private Span<byte> Slot(int sid) => Pool.RawSlot(sid);

        /// <summary>좌표는 raw float 비트를 그대로 쓴다.</summary>
        /// <param name="x">x 좌표.</param>
        /// <param name="y">y 좌표.</param>
        private void Coordinates(float x, float y)
        {
            Span<byte> raw = Slot(Source);
            Put(raw, 14, BitConverter.SingleToUInt32Bits(x));
            Put(raw, 18, BitConverter.SingleToUInt32Bits(y));
        }

        /// <summary>첫 생성 경계에서 독립 실행기와 같은 입력 변경만 공급한다.</summary>
        private void Mutation()
        {
            if (Change == 0 || Changed)
            {
                return;
            }
            Changed = true;
            if (Change == 1)
            {
                Put(Slot(60), 12, 0xfedc, 2);
                Put(Slot(60), 4, 0, 2);
                Coordinates(23.25f, 24.75f);
                FinderState.Boss = false;
                Put(Slot(61), 18, 158, 2);
                Put(Slot(61), 20, 32, 2);
                State.AllowedGenus = 0xffffffff;
            }
            else
            {
                State.Battle = false;
                State.AllowedGenus = 0;
                FinderState.ContainedType = 46;
                Slot(62)[RawSidPool.TypeOffset] = 46;
                State.BridgeType = 90;
                Spots[21 * RawSquidHash.WorldCells + 20] = 16;
            }
        }

        /// <summary>외부 효과를 독립 관찰 형식으로 기록한다.</summary>
        /// <param name="event">저장할 호출 표시.</param>
        private void Record(string @event) => Events.Add(@event);

        /// <summary>실제 하위 함수 대체만 제공한다. 좌표/타입 분기와 WORD 복사는 구현에 맡긴다.</summary>
        /// <returns>해방 훅.</returns>
        public DamageableReleaseHooks Hooks()
        {
            return new DamageableReleaseHooks
            {
                SpawnPriest = (x, y, kind, word) =>
                {
                    Record($"P:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}:{kind}:{word}");
                    Mutation();
                },
                Create = (kind, flags) =>
                {
                    Assert.Equal(0u, flags);
                    Assert.True(Born < NewIds.Length);
                    int index = Born++;
                    Span<byte> raw = Slot(NewIds[index]);
                    raw.Fill(0xcd);
                    Put(raw, 0, ReleaseVtable);
                    raw[RawSidPool.TypeOffset] = (byte)kind;
                    raw[RawSidPool.StateOffset] = 0;
                    int sid = NewIds[index];
                    Record($"N:{kind}:{flags}:{sid}");
                    Mutation();
                    return sid;
                },
                SetOwner = (sid, owner) => Record($"O:{sid}:{owner}"),
                Pop = (sid, x, y, flags) =>
                    Record($"L:{sid}:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}:{flags}"),
                NotifySurface = sid => Record($"S:{sid}"),
            };
        }

        /// <summary>이미 대조한 효과/소리/공통 경계이며 해방은 실제 몸체에 연결한다.</summary>
        /// <param name="release">실제 해방 몸체.</param>
        /// <returns>피해가능객체 사전 파괴 훅.</returns>
        public DamageablePreDestroyHooks PreHooks(RawDamageableRelease release)
        {
            DamageablePreDestroyHooks hooks = new()
            {
                Collapse = sid => Record($"C:{sid}"),
                Explosion = sid => Record($"X:{sid}"),
                Sound = effect =>
                {
                    if (effect.Sound == DamageableSound.PriestFree)
                    {
                        Record("F");
                    }
                    else
                    {
                        string marker = effect.Sound == DamageableSound.Collapse ? "A0" : "A1";
                        Record($"{marker}:{BitConverter.SingleToUInt32Bits(effect.X)}:{BitConverter.SingleToUInt32Bits(effect.Y)}");
                    }
                },
                ValidateBase = (sid, flags) => { },
                Base = (sid, flags) => Record($"B:{sid}:{flags}"),
            };
            return RawDamageableRelease.MakeDamageableReleaseHooks(Pool, release, hooks);
        }

        /// <summary>입력 칸만 사용하여 현재 타입 표·공간·raw 를 준비한다.</summary>
        /// <param name="row">fixture 행(25칸).</param>
        public void Prepare(string[] row)
        {
            Assert.Equal(25, row.Length);
            Events.Clear();
            Born = 0;
            Changed = false;
            Change = X86Fixture.UInt(row[19]);
            FinderState.Boss = X86Fixture.UInt(row[4]) != 0;
            FinderState.ContainedType = X86Fixture.UInt(row[5]);
            PreState.PriestType = X86Fixture.UInt(row[6]);
            State.Battle = X86Fixture.UInt(row[17]) != 0;
            State.AllowedGenus = X86Fixture.UInt(row[18]);
            State.BridgeType = 82;
            Array.Clear(Spots);
            // 각 map 표시를 실행기의 셀에 기록한다
            foreach (string mark in row[10].Split(';'))
            {
                string[] values = mark.Split(',');
                Spots[X86Fixture.Int(values[1]) * RawSquidHash.WorldCells + X86Fixture.Int(values[0])] = (byte)X86Fixture.UInt(values[2]);
            }
            // 미지정 raw 바이트도 독립 실행기의 채움 값과 일치시킨다
            foreach (int sid in SlotIds)
            {
                Span<byte> raw = Slot(sid);
                raw.Fill(0xab);
                raw[RawSidPool.TypeOffset] = 158;
                raw[RawSidPool.StateOffset] = 0;
                raw[ExtraOffset] = 0;
                Put(raw, RawSidPool.NextOffset, 0);
            }
            Span<byte> source = Slot(Source);
            source[RawSidPool.TypeOffset] = 160;
            source[RawSidPool.StateOffset] = (byte)X86Fixture.UInt(row[2]);
            source[ExtraOffset] = (byte)X86Fixture.UInt(row[3]);
            Put(source, 14, X86Fixture.UInt(row[8]));
            Put(source, 18, X86Fixture.UInt(row[9]));
            Put(source, 6, X86Fixture.UInt(row[11]), 2);
            // next/kind/flags와 후보 수명 비트를 추가 필터 없이 쓴다
            foreach (string node in row[12].Split(';'))
            {
                if (node.Length == 0)
                {
                    continue;
                }
                string[] values = node.Split(',');
                int sid = X86Fixture.Int(values[0]);
                Span<byte> raw = Slot(sid);
                raw[RawSidPool.TypeOffset] = (byte)X86Fixture.UInt(values[1]);
                raw[RawSidPool.StateOffset] = (byte)X86Fixture.UInt(values[2]);
                Put(raw, RawSidPool.NextOffset, X86Fixture.UInt(values[3]), 2);
                Put(raw, 18, X86Fixture.UInt(values[4]), 2);
                Put(raw, 20, X86Fixture.UInt(values[5]), 2);
            }
            // 생성 대상의 WORD 복사에서 상위 비트가 남는지 관찰할 입력이다
            for (int i = 1; i < SlotIds.Length; i++)
            {
                Put(Slot(SlotIds[i]), 12, 0x8100 + (uint)SlotIds[i], 2);
            }
            // 실제 생성 여부와 관계없이 네 개의 슬롯을 모두 비교한다
            foreach (int sid in NewIds)
            {
                Slot(sid).Fill(0xcd);
            }
            Types[160].Flags2 = X86Fixture.UInt(row[16]);
            Types[160].FootX = X86Fixture.Int(row[14]);
            Types[160].FootY = X86Fixture.Int(row[15]);
            Types[158].Flags2 = 0x200000;
            Types[82].Flags2 = 2;
            Types[90].Flags2 = 0x40000000;
        }

        /// <summary>부모와 후보 전체 바이트를 관찰한다.</summary>
        /// <returns>슬롯 hex 문자열.</returns>
        public string Slots() => string.Join('|', SlotIds.Select(id => Convert.ToHexStringLower(Pool.Slot(id).ToArray())));

        /// <summary>생성 전의 미지정 슬롯을 포함한 새 슬롯 전체 바이트를 관찰한다.</summary>
        /// <returns>새 슬롯 hex 문자열.</returns>
        public string NewSlots() => string.Join('|', NewIds.Select(id => Convert.ToHexStringLower(Pool.Slot(id).ToArray())));

        /// <summary>현재 boss/contained/priest 전역이다.</summary>
        /// <returns>전역 문자열.</returns>
        public string Globals() => $"{(FinderState.Boss ? 1 : 0)},{FinderState.ContainedType},{PreState.PriestType}";

        /// <summary>해방 중 바뀔 수 있는 현재 전투/허용 genus/다리 타입 전역이다.</summary>
        /// <returns>해방 전역 문자열.</returns>
        public string ReleaseGlobals() => $"{(State.Battle ? 1 : 0)},{State.AllowedGenus},{State.BridgeType}";
    }

    /// <summary>
    /// 피해가능객체 해방 fixture 의 10.78 몫을 재생한다.
    /// "D" 절은 Damageable 를 직접, "C" 절은 Carrier 경유로 부른다.
    /// </summary>
    [Fact]
    public void ReleaseMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("damageablerelease");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != PatchEdition)
            {
                return true;
            }
            var scene = new Scene();
            scene.Prepare(row);
            var release = new RawDamageableRelease(scene.Pool, scene.Types, scene.Spots, scene.FinderState, scene.State, scene.Hooks());
            var damageable = new RawDamageablePreDestroy(scene.Pool, scene.Spots, scene.FinderState, scene.PreState, scene.PreHooks(release));
            uint flags = X86Fixture.UInt(row[7]);
            if (row[1] == "D")
            {
                damageable.PreDestroy(Source, flags);
            }
            else
            {
                var carrier = new RawCarrierPreDestroy(scene.Pool, scene.FinderState, new CarrierPreDestroyHooks
                {
                    ValidateDamageable = (sid, value) => damageable.Validate(sid, value),
                    DamageablePre = (sid, value) => damageable.PreDestroy(sid, value),
                    DependentFound = () => scene.Events.Add("T"),
                });
                carrier.PreDestroy(Source, flags);
            }
            // 구현에서 기대 분기를 만들지 않고 각 원본 관찰과 비교한다
            Assert.Equal(row[20], scene.Globals());
            Assert.Equal(row[21], string.Join(';', scene.Events));
            Assert.Equal(row[22], scene.Slots());
            Assert.Equal(row[23], scene.ReleaseGlobals());
            Assert.Equal(row[24], scene.NewSlots());
            count++;
            return true;
        });
        Assert.Equal(RowsPerEdition, count);
    }
}
