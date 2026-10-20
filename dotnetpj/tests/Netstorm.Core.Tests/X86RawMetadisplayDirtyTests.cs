using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 미니맵 갱신 큐(<see cref="RawMetadisplayDirty"/>)를 원본 x86 기대값에 대조한다.
/// 기대값은 metadisplaydirty-x86.tsv(두 PE 의 큐 등록·Refresh·view 비교·전체/부분 그리기 1,040행) 가운데 10.78 몫 520행이다.
/// 각 행은 입력 13개 DWORD(연산·표시 상태·큐·bitmap·초기화·프레임·카메라/사각형 경우·callback 변이·좌표 비트)를 재생하고,
/// 결과 15개 DWORD(큐 count·포화·bitmap 폭·초기화·프레임·캐시 카메라 2·캐시 사각형 4·큐/bitmap/관찰 사건 FNV)를 비교한다.
/// 검사 장면은 cpppj/tests/MetadisplayDirtyTests.cpp 의 MetadisplayScene 과 같은 입력을 쓴다 (W02 열두째 조각, 2026-10-20).
/// </summary>
public sealed class X86RawMetadisplayDirtyTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>원본 보고서의 순서 보존 FNV-1a 32비트. 사건·큐·bitmap 바이트를 같은 순서로 넣는다.</summary>
    /// <param name="bytes">해시할 바이트.</param>
    private static uint Fnv(ReadOnlySpan<byte> bytes)
    {
        uint value = 2166136261U;
        // 모든 바이트를 순서대로 섞는다
        foreach (byte item in bytes)
        {
            value = unchecked((value ^ item) * 16777619U);
        }
        return value;
    }

    /// <summary>리틀 엔디언 32비트를 바이트 목록 끝에 붙인다.</summary>
    /// <param name="bytes">붙일 목록.</param>
    /// <param name="value">붙일 값.</param>
    private static void Append(List<byte> bytes, uint value)
    {
        // 호스트 엔디언과 무관하게 BYTE 마다 저장한다
        for (int i = 0; i < 4; i++)
        {
            bytes.Add((byte)(value >> (8 * i)));
        }
    }

    /// <summary>fixture 한 행을 재생하는 장면. 같은 풀·상태·view·사건 순서를 원본과 같이 만든다.</summary>
    private sealed class Scene
    {
        /// <summary>호출 SID 의 현재 좌표를 읽을 실제 풀(클라이언트 번호 5 확보).</summary>
        public readonly RawSidPool Pool;

        /// <summary>native 와 같은 큐·캐시·bitmap·프레임 입력.</summary>
        public MetadisplayDirtyState State = new();

        /// <summary>Refresh 에 넘길 현재 카메라/사각형.</summary>
        public MetadisplayDirtyView Current;

        /// <summary>rebuild/셀 그리기 효과의 호출 순서를 담은 BYTE. DWORD 3개씩 쌓는다.</summary>
        public readonly List<byte> Events = [];

        /// <summary>효과 안에서 현재 count/full 을 바꾸는 명시 입력.</summary>
        public uint Mutation;

        /// <summary>첫 셀에서만 새 고유 셀을 등록했는지.</summary>
        public bool Mutated;

        /// <summary>클라이언트 번호 5 를 확보한다. 원본 게임/초기 로딩을 대신했다고 주장하지 않는다.</summary>
        public Scene()
        {
            Pool = new RawSidPool(24000, false);
            Assert.Equal(RawSidPool.FirstOrdinary, Pool.Allocate(SidAllocation.Client));
        }

        /// <summary>fixture 의 13개 입력 DWORD 를 상태·view·사건 입력으로 옮긴다. 의미 없는 옛 큐 칸도 모두 채운다.</summary>
        /// <param name="input">입력 DWORD 13개.</param>
        public void Prepare(uint[] input)
        {
            Assert.Equal(13, input.Length);
            State = new MetadisplayDirtyState();
            Current = default;
            Events.Clear();
            Mutated = false;
            Mutation = input[10];
            State.DisplayPresent = input[1] != 0;
            State.DisplayFlags = (byte)input[2];
            State.Count = (int)input[3];
            State.FullRefresh = input[4];
            State.BitmapWidth = input[5] != 0 ? MetadisplayDirtyState.BitmapSide : 0;
            if (input[5] != 0)
            {
                State.Bitmap = new byte[MetadisplayDirtyState.BitmapSide * MetadisplayDirtyState.BitmapSide];
                Array.Fill(State.Bitmap, (byte)0xA5);
            }
            State.Initialized = input[6];
            State.Frames = input[7];
            // 원본 첫 두 후보는 중복 검사 입력에 대응하며 count 밖 칸을 지우지 않는다
            for (int i = 0; i < MetadisplayDirtyState.CellCapacity; i++)
            {
                State.Cells[i] = new MetadisplayDirtyCell(i, i + 10, 0xA5A5A5A5U);
            }
            switch (input[8])
            {
                case 1:
                    Current.CameraX = 0x80000000;
                    break;
                case 2:
                    State.Cached.CameraX = Current.CameraX = 0x7FC00001;
                    break;
                case 3:
                    Current.CameraX = BitConverter.SingleToUInt32Bits(12.5f);
                    Current.CameraY = BitConverter.SingleToUInt32Bits(22.75f);
                    break;
                case 4:
                    State.Cached.CameraX = 0x7FC00001;
                    Current.CameraX = 0x7FC00002;
                    break;
                case 5:
                    State.Cached.CameraX = Current.CameraX = BitConverter.SingleToUInt32Bits(12.5f);
                    State.Cached.CameraY = Current.CameraY = BitConverter.SingleToUInt32Bits(22.75f);
                    break;
            }
            if (input[9] == 2)
            {
                SetCachedRectangle(ref State.Cached);
            }
            if (input[9] != 0)
            {
                SetRectangle(ref Current);
            }
        }

        /// <summary>캐시 view 의 사각형을 원본 관찰 값으로 채운다.</summary>
        /// <param name="view">채울 view.</param>
        private static void SetCachedRectangle(ref MetadisplayDirtyView view)
        {
            view.Left = 11;
            view.Top = 22;
            view.Right = 33;
            view.Bottom = 44;
        }

        /// <summary>현재 view 의 사각형을 원본 관찰 값으로 채운다.</summary>
        /// <param name="view">채울 view.</param>
        private static void SetRectangle(ref MetadisplayDirtyView view) => SetCachedRectangle(ref view);

        /// <summary>사건을 기록하고 명시 변이를 같은 시점에 적용하는 효과를 만든다.</summary>
        public MetadisplayDirtyHooks Hooks() => new()
        {
            Rebuild = () =>
            {
                Append(Events, 1);
                Append(Events, 0);
                Append(Events, 0);
                if (Mutation == 1)
                {
                    State.FullRefresh = 0;
                }
                else if (Mutation == 2)
                {
                    State.FullRefresh = 1;
                }
                else if (Mutation == 3)
                {
                    State.Count = 2;
                }
            },
            RedrawCell = (x, y) =>
            {
                Append(Events, 2);
                Append(Events, unchecked((uint)x));
                Append(Events, unchecked((uint)y));
                if (Mutation == 4 && !Mutated)
                {
                    Mutated = true;
                    State.Cells[State.Count] = new MetadisplayDirtyCell(40, 41);
                    State.Count++;
                }
            },
        };

        /// <summary>원본 snapshot 필드 순서로 결과 문자열을 만든다. 큐 밖 칸·bitmap·사건 순서까지 비교한다.</summary>
        public string Snapshot()
        {
            var output = new List<uint>
            {
                (uint)State.Count, State.FullRefresh, (uint)State.BitmapWidth, State.Initialized, State.Frames,
                State.Cached.CameraX, State.Cached.CameraY,
                unchecked((uint)State.Cached.Left), unchecked((uint)State.Cached.Top),
                unchecked((uint)State.Cached.Right), unchecked((uint)State.Cached.Bottom),
            };
            var cells = new List<byte>();
            // count 밖의 큐도 원본이 초기화하지 않는 바이트로 비교한다
            foreach (MetadisplayDirtyCell cell in State.Cells)
            {
                Append(cells, unchecked((uint)cell.X));
                Append(cells, unchecked((uint)cell.Y));
                Append(cells, cell.Unused);
            }
            output.Add(Fnv(cells.ToArray()));
            output.Add(State.Bitmap.Length == 0 ? 0 : Fnv(State.Bitmap));
            output.Add((uint)(Events.Count / 12));
            output.Add(Fnv(Events.ToArray()));
            return string.Join(',', output);
        }
    }

    /// <summary>10.78 몫 520행을 재생한다. CD 10.72 행은 기준 판본이 아니므로 읽지 않는다.</summary>
    [Fact]
    public void EnqueueRefreshAndHooksMatchOriginalX86()
    {
        string[][] rows = X86Fixture.ReadFile("metadisplaydirty-x86.tsv").Where(row => row[0] == PatchEdition).ToArray();
        Assert.Equal(520, rows.Length);
        int calls = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(3, row.Length);
            uint[] input = row[1].Split(',').Select(X86Fixture.UInt).ToArray();
            var scene = new Scene();
            scene.Prepare(input);
            var dirty = new RawMetadisplayDirty(scene.Pool, scene.State, scene.Hooks());
            if (input[0] == 0)
            {
                dirty.Enqueue(BitConverter.UInt32BitsToSingle(input[11]), BitConverter.UInt32BitsToSingle(input[12]));
            }
            else
            {
                dirty.Refresh(scene.Current);
            }
            calls++;
            return scene.Snapshot() == row[2];
        });
        Assert.Equal(520, calls);
    }
}
