using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 피해가능객체(Damageable) 사전 파괴와 캐리어 경계를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/DamageablePreDestroyTests.cpp 의 DamageableScene 과 같다: 실제 슬롯 풀·spot 지도 위에서
/// 붕괴/폭발 효과와 소리 접두·사제 해방 소리·해방 몸체·공통 preDestroy 호출을 실행하고,
/// "D" 절은 Damageable 를 직접, "C" 절은 Carrier 경유로 부른다.
/// 호출마다 외부 호출 순서·슬롯 50바이트 ×5·현재 권한/타입/사제 kind 전역을 비교한다.
/// 기준 판본은 10.78 이며 originalCD·original1037 행은 읽지 않는다 (W03 삭제·수명 다섯째 조각, 2026-10-21).
/// </summary>
public sealed class X86RawDamageablePreDestroyTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 10.72·원본 10.37 다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 한 판본의 행 수(D 816 + C 816).</summary>
    private const int RowsPerEdition = 1632;

    /// <summary>장면이 확보하는 슬롯 수(5~63).</summary>
    private const int LastSlot = 63;

    /// <summary>장면이 비교하는 다섯 슬롯 번호.</summary>
    private static readonly int[] SlotIds = [50, 60, 61, 62, 63];

    /// <summary>삭제 대상 슬롯 번호.</summary>
    private const int Source = 50;

    /// <summary>슬롯의 기본 타입 번호(원본 사제 타입).</summary>
    private const byte DefaultType = 158;

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

    /// <summary>한 Damageable 사전 파괴 행의 풀·spot 지도·전역을 소유하는 재생 장면.</summary>
    private sealed class Scene
    {
        /// <summary>삭제 대상 슬롯 풀(24,000칸, 서버 아님).</summary>
        public readonly RawSidPool Pool;

        /// <summary>칸별 점유 지도(65,536칸).</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>내용물 검색기의 권한·타입 전역.</summary>
        public readonly ContainedFinderState FinderState = new();

        /// <summary>사제 해방 판정 전역.</summary>
        public readonly DamageablePreDestroyState State = new();

        /// <summary>나간 외부 호출 순서.</summary>
        public readonly List<string> Events = [];

        /// <summary>효과를 고르는 장면 구분(0~3).</summary>
        public int Style { get; private set; }

        /// <summary>처음 주어진 권한 전역.</summary>
        public uint InitialBoss { get; private set; }

        /// <summary>지금까지 나온 사제 해방 소리 횟수.</summary>
        public int FreeCount { get; private set; }

        /// <summary>공통 사전 검사 호출 횟수.</summary>
        public int Validations { get; private set; }

        /// <summary>장면을 만든다. 클라이언트 번호 5~63 을 먼저 확보한다.</summary>
        public Scene()
        {
            Pool = new RawSidPool(24000, false);
            // 부모 실제 Create와 같은 클라이언트 할당만 준비한다
            for (int id = 5; id <= LastSlot; id++)
            {
                Assert.Equal(id, Pool.Allocate(SidAllocation.Client));
            }
            FinderState.CheckingDead = true;
        }

        /// <summary>슬롯 바이트를 직접 쓴다(장면 준비용).</summary>
        /// <param name="sid">대상 슬롯 번호.</param>
        /// <returns>쓰기 가능한 슬롯 바이트.</returns>
        private Span<byte> Slot(int sid) => Pool.RawSlot(sid);

        /// <summary>효과 뒤의 좌표를 비트 그대로 입력한다.</summary>
        /// <param name="x">x 좌표.</param>
        /// <param name="y">y 좌표.</param>
        private void Coordinates(float x, float y)
        {
            Span<byte> raw = Slot(Source);
            Put(raw, 14, BitConverter.SingleToUInt32Bits(x));
            Put(raw, 18, BitConverter.SingleToUInt32Bits(y));
        }

        /// <summary>
        /// 실행기의 경계별 raw/전역 변경만 공급한다. 기대 결과는 계산하지 않는다.
        /// 원본 PE 의 하위 효과가 메모리를 바꾸는 것을 그대로 흉내 낸다.
        /// </summary>
        /// <param name="marker">바뀌는 경계 표시.</param>
        private void Mutation(string marker)
        {
            if (Style == 1)
            {
                if (marker == "C")
                {
                    Coordinates(21.00001f, 22.00001f);
                    Slot(Source)[ExtraOffset] = 9;
                }
                if (marker == "A0")
                {
                    Coordinates(22.75f, 23.9f);
                    FinderState.Boss = InitialBoss == 0;
                }
                if (marker == "X")
                {
                    Coordinates(23, 24);
                }
                if (marker == "A1")
                {
                    Coordinates(24.00001f, 25.00001f);
                }
            }
            if (Style == 2 && marker == "F" && FreeCount == 1)
            {
                FinderState.Boss = InitialBoss == 0;
                FinderState.ContainedType = 46;
                State.PriestType = 159;
                Put(Slot(Source), 6, 0, 2);
                Put(Slot(60), 4, 0, 2);
                Slot(61)[RawSidPool.TypeOffset] = 46;
                Put(Slot(61), 18, 159, 2);
            }
            if (Style == 3 && marker == "B")
            {
                FinderState.Boss = true;
                FinderState.ContainedType = 6;
                Put(Slot(Source), 6, 63, 2);
                Slot(63)[RawSidPool.TypeOffset] = 6;
                Put(Slot(63), 20, 1, 2);
            }
        }

        /// <summary>외부 호출 순서를 원본 관찰과 같은 문자열로 저장한다.</summary>
        /// <param name="event">저장할 호출 표시.</param>
        private void Record(string @event)
        {
            Events.Add(@event);
        }

        /// <summary>원본 PE에서 대체한 함수/구간만 C#에도 동일한 외부 효과로 공급한다.</summary>
        /// <returns>피해가능객체 사전 파괴 훅.</returns>
        public DamageablePreDestroyHooks Hooks()
        {
            return new DamageablePreDestroyHooks
            {
                Collapse = sid =>
                {
                    Record($"C:{sid}");
                    Mutation("C");
                },
                Explosion = sid =>
                {
                    Record($"X:{sid}");
                    Mutation("X");
                },
                Sound = effect =>
                {
                    if (effect.Sound == DamageableSound.PriestFree)
                    {
                        Record("F");
                        FreeCount++;
                        Mutation("F");
                    }
                    else
                    {
                        string marker = effect.Sound == DamageableSound.Collapse ? "A0" : "A1";
                        Record($"{marker}:{BitConverter.SingleToUInt32Bits(effect.X)}:{BitConverter.SingleToUInt32Bits(effect.Y)}");
                        Mutation(marker);
                    }
                },
                ReleaseContained = sid => Record($"R:{sid}"),
                ValidateBase = (sid, flags) => Validations++,
                Base = (sid, flags) =>
                {
                    Record($"B:{sid}:{flags}");
                    Mutation("B");
                },
            };
        }

        /// <summary>입력 슬롯/spot/전역은 행의 입력 칸에서만 구성한다.</summary>
        /// <param name="row">fixture 행(17칸).</param>
        public void Prepare(string[] row)
        {
            Assert.Equal(17, row.Length);
            Events.Clear();
            Validations = 0;
            FreeCount = 0;
            Style = X86Fixture.Int(row[13]);
            InitialBoss = X86Fixture.UInt(row[4]);
            FinderState.Boss = InitialBoss != 0;
            FinderState.ContainedType = X86Fixture.UInt(row[5]);
            State.PriestType = X86Fixture.UInt(row[6]);
            Array.Clear(Spots);
            // 지정 map 셀만 표시하므로 좌표 경계의 선택이 실제 출력에 영향을 준다
            foreach (string mark in row[10].Split(';'))
            {
                string[] values = mark.Split(',');
                Spots[X86Fixture.Int(values[1]) * RawSquidHash.WorldCells + X86Fixture.Int(values[0])] = (byte)X86Fixture.UInt(values[2]);
            }
            // 주변/미지정 raw도 전체 비교한다
            foreach (int sid in SlotIds)
            {
                Span<byte> raw = Slot(sid);
                raw.Fill(0xab);
                raw[RawSidPool.TypeOffset] = DefaultType;
                raw[RawSidPool.StateOffset] = 0;
                raw[ExtraOffset] = 0;
                Put(raw, RawSidPool.NextOffset, 0);
            }
            Span<byte> source = Slot(Source);
            source[RawSidPool.StateOffset] = (byte)X86Fixture.UInt(row[2]);
            source[ExtraOffset] = (byte)X86Fixture.UInt(row[3]);
            Put(source, 14, X86Fixture.UInt(row[8]));
            Put(source, 18, X86Fixture.UInt(row[9]));
            Put(source, 6, X86Fixture.UInt(row[11]), 2);
            // 번호·타입·상태·next·kind·mask를 입력 그대로 쓴다
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
        }

        /// <summary>입력 5개 raw를 실행기의 순서로 직렬화한다.</summary>
        /// <returns>슬롯 바이트 hex 문자열.</returns>
        public string Slots()
        {
            // 후보의 미지정 바이트까지 모두 비교한다
            return string.Join('|', SlotIds.Select(id => Convert.ToHexStringLower(Pool.Slot(id).ToArray())));
        }

        /// <summary>현재 권한/타입/사제 kind 전역의 전체 관찰이다.</summary>
        /// <returns>전역 문자열.</returns>
        public string Globals() => $"{(FinderState.Boss ? 1 : 0)},{FinderState.ContainedType},{State.PriestType}";

        /// <summary>나간 외부 호출 순서를 원본과 같은 문자열로 만든다.</summary>
        /// <returns>이벤트 문자열.</returns>
        public string EventText() => Events.Count == 0 ? "-" : string.Join(';', Events);
    }

    /// <summary>
    /// Damageable 사전 파괴 fixture 의 10.78 몫을 재생한다.
    /// "D" 절은 Damageable 를 직접, "C" 절은 Carrier 경유로 부른다.
    /// </summary>
    [Fact]
    public void PreDestroyMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("damageablepredestroy");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != PatchEdition)
            {
                return true;
            }
            var scene = new Scene();
            scene.Prepare(row);
            var damageable = new RawDamageablePreDestroy(scene.Pool, scene.Spots, scene.FinderState, scene.State, scene.Hooks());
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
            // 후보 상태/바뀐 next/현재 전역을 구현에서 추정하지 않고 독립 행을 재생한다
            Assert.Equal(row[14], scene.Globals());
            Assert.Equal(row[15], scene.EventText());
            Assert.Equal(row[16], scene.Slots());
            Assert.Equal(row[1] == "D" ? 1 : 2, scene.Validations);
            count++;
            return true;
        });
        Assert.Equal(RowsPerEdition, count);
    }
}
