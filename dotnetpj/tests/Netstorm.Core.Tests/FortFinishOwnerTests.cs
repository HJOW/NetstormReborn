using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort owner 후처리·시각 seed·AddRandom 배치의 정상 반환과 local 난수를 두 판본 기록에 대조한다.
/// cpppj <c>FortFinishOwnerTests</c>의 전수 719·공유 Player·연결·경계 6종을 옮겼다. (F2c-8, 2026-11-03)
/// </summary>
public sealed class FortFinishOwnerTests
{
    /// <summary>고유 fixture 입력 순서다.</summary>
    private enum FinishField
    {
        /// <summary>소유자다.</summary>
        Owner = 0,
        /// <summary>모드다.</summary>
        Mode = 1,
        /// <summary>fort 모드다.</summary>
        Fort = 2,
        /// <summary>special 모드다.</summary>
        Special = 3,
        /// <summary>지역이다.</summary>
        Region = 4,
        /// <summary>목록 입력 번호다.</summary>
        List = 5,
        /// <summary>시각 하위 DWORD다.</summary>
        TimeLow = 6,
        /// <summary>시각 상위 DWORD다.</summary>
        TimeHigh = 7,
        /// <summary>사각형 입력 번호다.</summary>
        Rect = 8,
        /// <summary>성공 시작 시도다.</summary>
        SuccessAt = 9,
        /// <summary>성공 주기다.</summary>
        Period = 10,
        /// <summary>seed다.</summary>
        Seed = 11,
        /// <summary>타입이다.</summary>
        Type = 12,
        /// <summary>수량이다.</summary>
        Quantity = 13,
        /// <summary>강제다.</summary>
        Force = 14,
        /// <summary>외부 변형 번호다.</summary>
        Mutation = 15,
    }

    /// <summary>판본별 행 수다.</summary>
    private const int Cases = 719;

    /// <summary>장면 풀 용량이다.</summary>
    private const int Capacity = 24000;

    /// <summary>float 입력을 비트 그대로 비교한다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    private static uint Bits(float value)
    {
        return BitConverter.SingleToUInt32Bits(value);
    }

    /// <summary>쉼표 구분 입력 칸을 부호 없는 DWORD 목록으로 읽는다.</summary>
    /// <param name="text">입력 열이다.</param>
    private static List<uint> Inputs(string text)
    {
        List<uint> values = [];
        // 칸마다 부호 없는 수로 읽는다
        foreach (string field in text.Split(','))
        {
            values.Add(uint.Parse(field, CultureInfo.InvariantCulture));
        }
        return values;
    }

    /// <summary>실제 native의 지역 입력 사각형이다. 기대 배치 좌표는 계산하지 않는다.</summary>
    /// <param name="profile">입력 번호다.</param>
    private static FortRandomRectangle FinishRectangle(uint profile)
    {
        return profile switch
        {
            0 => new FortRandomRectangle(32f, 48f, 96f, 112f),
            1 => new FortRandomRectangle(15.25f, 21.5f, 16.1f, 23.9f),
            2 => new FortRandomRectangle(-15.25f, -3.5f, 4.5f, 9.5f),
            3 => new FortRandomRectangle(20f, 21f, 20f, 21f),
            4 => new FortRandomRectangle(20f, 21f, 19f, 19f),
            5 => new FortRandomRectangle(1f, 2f, 255f, 254f),
            _ => throw new ArgumentOutOfRangeException(nameof(profile), profile, "Finish 사각형 입력 범위 오류"),
        };
    }

    /// <summary>원본 residence 목록의 타입·상태·소유자·genus 자료만 공급한다.</summary>
    private sealed class FinishNode
    {
        /// <summary>원본 풀에서 객체를 찾는 SID 번호다.</summary>
        public ushort Sid;

        /// <summary>타입 표에서 객체 종류를 찾는 번호다.</summary>
        public byte Type;

        /// <summary>검사 입력과 실행 뒤 결과를 보관할 상태다.</summary>
        public byte State;

        /// <summary>객체나 지역을 소유한 플레이어 번호다.</summary>
        public byte Owner;

        /// <summary>genus다.</summary>
        public uint Genus;
    }

    /// <summary>원본과 동일한 중복·후행·틀린 소유자·flags·free·dead·void 입력이다.</summary>
    /// <param name="profile">입력 번호다.</param>
    /// <param name="owner">기준 소유자다.</param>
    private static List<FinishNode> FinishNodes(uint profile, byte owner)
    {
        byte other = (byte)((owner + 1) % 9);
        return profile switch
        {
            0 => [],
            1 => [new FinishNode { Sid = 50, Type = 89, State = 0, Owner = owner, Genus = 0x40000000u }],
            2 => [new FinishNode { Sid = 50, Type = 89, State = 0, Owner = other, Genus = 0x40000000u }],
            3 => [new FinishNode { Sid = 50, Type = 100, State = 0, Owner = owner, Genus = 0u }],
            4 => [
                new FinishNode { Sid = 50, Type = 100, State = 0, Owner = owner, Genus = 0u },
                new FinishNode { Sid = 51, Type = 89, State = 0, Owner = other, Genus = 0x40000000u },
                new FinishNode { Sid = 52, Type = 89, State = 0, Owner = owner, Genus = 0x40000000u },
                new FinishNode { Sid = 52, Type = 89, State = 0, Owner = owner, Genus = 0x40000000u },
            ],
            5 => [new FinishNode { Sid = 50, Type = 89, State = 15, Owner = owner, Genus = 0x40000000u }],
            _ => throw new ArgumentOutOfRangeException(nameof(profile), profile, "Finish 목록 입력 범위 오류"),
        };
    }

    /// <summary>현재 타입·목록·모드·호출별 시각·배치 정책 응답을 같은 actual pool에 공급하는 장면이다.</summary>
    private sealed class FortFinishScene
    {
        /// <summary>객체 슬롯 풀이다.</summary>
        public RawSidPool Pool;

        /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표다.</summary>
        public List<OriginalTypeRecord> Types;

        /// <summary>거주 건물이다.</summary>
        public RawPostPopList Residences = new();

        /// <summary>검사 입력과 실행 뒤 결과를 보관할 상태다.</summary>
        public FortFinishOwnerState State = new();

        /// <summary>값이다.</summary>
        public List<uint> Values;

        /// <summary>발생한 이벤트다.</summary>
        public List<string> Events = [];

        /// <summary>시도다.</summary>
        public uint Attempts;

        /// <summary>회차다.</summary>
        public uint Rounds;

        /// <summary>마지막 시드다.</summary>
        public uint LastSeed;

        /// <summary>추가된 여부다.</summary>
        public bool Added;

        /// <summary>10.78 패치판의 상태 또는 패치판 검사용 자료다.</summary>
        public bool Patch;

        /// <summary>
        /// 가상 배치 경계가 반환한 성공의 실제 목록 변경을 관찰할 번호를 미리 확보한다.
        /// </summary>
        /// <param name="edition">판본이다.</param>
        /// <param name="input">입력 16개 목록이다.</param>
        public FortFinishScene(OriginalEdition edition, List<uint> input)
        {
            Pool = new RawSidPool(Capacity, true, null, edition);
            Types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToList();
            Values = input;
            Patch = edition == OriginalEdition.Patch1078;
            // raw native 입력 SID를 실제로 할당한다
            for (int sid = 5; sid <= 100; sid++)
            {
                Assert.Equal(sid, Pool.Allocate((SidAllocation)2));
            }
            Types[89].Flags2 = 0x40000000u;
            Types[90].Flags2 = 0x40000000u;
            Residences.Entries = new uint[8];
            Residences.Count = 0;
            State.ChaffMode = Values[(int)FinishField.Mode] != 0;
            State.Fort = Values[(int)FinishField.Fort] != 0;
            State.Special = Values[(int)FinishField.Special] != 0;
            State.ResidenceType = Values[(int)FinishField.Type];
            // 각 목록 항목의 원본 raw 상태만 공급하며 같은 SID의 중복도 그대로 둔다
            for (int index = 0; index < State.PlayerRegions.Length; index++)
            {
                State.PlayerRegions[index] = unchecked((int)Values[(int)FinishField.Region]);
            }
            foreach (FinishNode node in FinishNodes(Values[(int)FinishField.List], (byte)Values[(int)FinishField.Owner]))
            {
                Span<byte> raw = Pool.AllocatedBytes(node.Sid);
                raw.Fill(0);
                raw[10] = node.Type;
                raw[Patch ? 34 : 32] = node.Owner;
                raw[11] = node.State;
                Types[node.Type].Flags2 = node.Genus;
                Residences.Entries[(int)Residences.Count++] = node.Sid;
            }
        }

        /// <summary>실제 효과·인자의 원시 DWORD를 기록한다.</summary>
        /// <param name="letter">사건 글자다.</param>
        /// <param name="args">값이다.</param>
        public void Event(char letter, params uint[] args)
        {
            StringBuilder text = new();
            text.Append(letter);
            // native 관찰의 값 순서·폭을 유지한다
            foreach (uint value in args)
            {
                text.Append(':');
                text.Append(value.ToString(CultureInfo.InvariantCulture));
            }
            Events.Add(text.ToString());
        }

        /// <summary>매 시각 호출마다 전달했던 독립 double 표본을 반환한다. callback 횟수도 검증한다.</summary>
        public double Clock()
        {
            double initial = BitConverter.UInt64BitsToDouble(((ulong)Values[(int)FinishField.TimeHigh] << 32) | Values[(int)FinishField.TimeLow]);
            double time = initial + Rounds++ * 0.125;
            ulong bits = BitConverter.DoubleToUInt64Bits(time);
            Event('T', (uint)bits, (uint)(bits >> 32));
            return time;
        }

        /// <summary>native의 실제 외부 경계만 공급한다. 생성 성공은 같은 residence 목록에 등록하므로 Finish가 다시 읽는다.</summary>
        /// <param name="finishMode">Finish 진입이면 참, 배치 단독이면 거짓이다.</param>
        public FortFinishOwnerHooks Hooks(bool finishMode)
        {
            return new FortFinishOwnerHooks
            {
                RegionBounds = region =>
                {
                    FortRandomRectangle rect = FinishRectangle(Values[(int)FinishField.Rect]);
                    Event('R', unchecked((uint)region), Bits(rect.Left), Bits(rect.Top), Bits(rect.Right), Bits(rect.Bottom));
                    if (Values[(int)FinishField.Mutation] == 1)
                    {
                        State.ResidenceType = 90;
                    }
                    return rect;
                },
                WallSeconds = () => Clock(),
                Place = (random, request) =>
                {
                    Attempts++;
                    Event('P', request.Type, Bits(request.X), Bits(request.Y), unchecked((uint)request.Region), request.Mode, request.Force);
                    if (Values[(int)FinishField.Mutation] == 2 && Attempts == 1)
                    {
                        random.SetState(0x98765432u);
                    }
                    LastSeed = random.State;
                    bool success = Attempts >= Values[(int)FinishField.SuccessAt]
                        && (Attempts - Values[(int)FinishField.SuccessAt]) % Values[(int)FinishField.Period] == 0;
                    if (success && finishMode && !Added)
                    {
                        Added = true;
                        Span<byte> raw = Pool.AllocatedBytes(80);
                        raw.Fill(0);
                        raw[10] = (byte)request.Type;
                        raw[Patch ? 34 : 32] = (byte)Values[(int)FinishField.Owner];
                        Residences.Entries[(int)Residences.Count++] = 80;
                    }
                    return success ? 7u : 0u;
                },
                InvalidType = () => Event('D', 29),
            };
        }

        /// <summary>관찰 문자열을 fixture와 같은 형식으로 묶는다.</summary>
        public string EventText()
        {
            return Events.Count == 0 ? "-" : string.Join(";", Events);
        }

        /// <summary>같은 실제 목록의 중복·최종 순서를 그대로 비교한다.</summary>
        public string List()
        {
            if (Residences.Count == 0)
            {
                return "-";
            }
            List<string> parts = [];
            // 활성 count까지만 직렬화하며 native와 같은 후행 저장소는 읽지 않는다
            for (uint index = 0; index < Residences.Count; index++)
            {
                parts.Add(Residences.Entries[(int)index].ToString(CultureInfo.InvariantCulture));
            }
            return string.Join(",", parts);
        }
    }

    /// <summary>지정 폭의 원시 필드만 변경한다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(byte[] bytes, uint at, uint value)
    {
        // 주변 필드는 그대로 둔다
        for (int index = 0; index < 4; index++)
        {
            bytes[(int)(at + (uint)index)] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>entry·standalone batch·실제 seed 생성 관찰을 동일 입력의 C# 몸체와 대조한다.</summary>
    /// <param name="editionName">판본 이름이다.</param>
    /// <param name="sharedPlayers">공유 Player 기록으로 재생하면 참이다.</param>
    private static void ReplayFinish(string editionName, bool sharedPlayers = false)
    {
        OriginalEdition edition = editionName == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        string[][] rows = X86Fixture.Read("fortfinishowner", editionName);
        Assert.Equal(Cases, rows.Length);
        int count = 0;
        // 한 행마다 다른 actual pool·목록을 구성해 이전 callback 상태가 섞이지 않게 한다
        foreach (string[] row in rows)
        {
            Assert.Equal(8, row.Length);
            FortFinishScene scene = new(edition, Inputs(row[2]));
            RawFortFinishOwner finish = new(scene.Pool, scene.Types, scene.Residences, scene.State, scene.Hooks(row[1] == "F"));
            uint stride = scene.Patch ? 0xb4u : 0xacu;
            uint field = scene.Patch ? 0x78u : 0x70u;
            byte[] records = new byte[9 * stride];
            Array.Fill(records, (byte)0xa5);
            if (sharedPlayers)
            {
                // 기존 독립 입력의 지역만 원문에 넣고 state 사본은 다른 값으로 분리한다
                for (int owner = 0; owner < 9; owner++)
                {
                    Put(records, (uint)owner * stride + field, scene.Values[(int)FinishField.Region]);
                }
                finish.BindPlayerRecords(records);
                for (int index = 0; index < scene.State.PlayerRegions.Length; index++)
                {
                    scene.State.PlayerRegions[index] = 999;
                }
            }
            byte[] before = (byte[])records.Clone();
            uint result = 0;
            if (row[1] == "F")
            {
                finish.Finish(scene.Values[(int)FinishField.Owner]);
            }
            else if (row[1] == "B")
            {
                NetstormRandom random = new(scene.Values[(int)FinishField.Seed]);
                result = finish.PlaceRandom(
                    random,
                    FinishRectangle(scene.Values[(int)FinishField.Rect]),
                    unchecked((int)scene.Values[(int)FinishField.Region]),
                    scene.Values[(int)FinishField.Quantity],
                    scene.Values[(int)FinishField.Type],
                    scene.Values[(int)FinishField.Force]);
                scene.LastSeed = random.State;
            }
            else
            {
                result = RawFortFinishOwner.TimeSeed(scene.Clock());
                scene.LastSeed = result;
            }
            bool same = scene.EventText() == row[3]
                && result.ToString(CultureInfo.InvariantCulture) == row[4]
                && scene.LastSeed.ToString(CultureInfo.InvariantCulture) == row[5]
                && scene.List() == row[6]
                && scene.State.ResidenceType.ToString(CultureInfo.InvariantCulture) == row[7];
            if (sharedPlayers)
            {
                Assert.True(records.AsSpan().SequenceEqual(before));
                foreach (int value in scene.State.PlayerRegions)
                {
                    Assert.Equal(999, value);
                }
            }
            Assert.True(same, $"Fort finish {editionName} {row[1]} 행{count} 불일치: {scene.EventText()} vs {row[3]}");
            count++;
        }
        Assert.Equal(Cases, count);
    }

    /// <summary>원본 전체 entry·seed·batch의 관찰을 재생한다.</summary>
    [Fact]
    public void ReplaysPatch()
    {
        ReplayFinish("originals");
    }

    /// <summary>CD의 사각형 점 helper·local 난수 및 정수 HP와 별도의 후처리 관찰이다.</summary>
    [Fact]
    public void ReplaysCd()
    {
        ReplayFinish("originalCD");
    }

    /// <summary>기존 패치 719입력을 현재 Player 지역 DWORD로 재생하며 별도 상태를 읽지 않는다.</summary>
    [Fact]
    public void PatchSharedPlayerRecords()
    {
        ReplayFinish("originals", true);
    }

    /// <summary>CD의 다른 물리 위치와 지역 snapshot·반복 흐름을 기존 719입력과 대조한다.</summary>
    [Fact]
    public void CdSharedPlayerRecords()
    {
        ReplayFinish("originalCD", true);
    }

    /// <summary>loader 후처리는 실제 같은 월드 모듈을 호출하고 residence가 있으면 지역·시각·배치를 하지 않는다.</summary>
    [Fact]
    public void ConnectsTheSameResidenceWorld()
    {
        List<uint> input = Inputs(X86Fixture.Read("fortfinishowner", "originals")[0][2]);
        input[(int)FinishField.List] = 1;
        input[(int)FinishField.Region] = 3;
        input[(int)FinishField.Mode] = 0;
        input[(int)FinishField.Fort] = 0;
        input[(int)FinishField.Special] = 0;
        input[(int)FinishField.Owner] = 1;
        FortFinishScene scene = new(OriginalEdition.Patch1078, input);
        RawFortFinishOwner finish = new(scene.Pool, scene.Types, scene.Residences, scene.State, scene.Hooks(true));
        FortObjectLoadHooks hooks = RawFortFinishOwner.MakeFortFinishOwnerHooks(scene.Pool, finish, new FortObjectLoadHooks());
        hooks.FinishOwner!(1);
        Assert.Empty(scene.Events);
        Assert.Equal(0u, scene.Rounds);
        Assert.Equal(0u, scene.Attempts);
        RawSidPool foreign = new(Capacity, true, null, OriginalEdition.Patch1078);
        Assert.Throws<ArgumentException>(() => RawFortFinishOwner.MakeFortFinishOwnerHooks(foreign, finish, new FortObjectLoadHooks()));
        scene.State.ChaffMode = true;
        finish.Finish(0xffffffffu);
        Assert.Empty(scene.Events);
    }

    /// <summary>by-value rectangle snapshot·quantity·실제 callback과 손상된 현재 목록·누락 효과 거부를 검사한다.</summary>
    [Fact]
    public void PreservesRectangleAndRequiresRealEffects()
    {
        List<uint> input = Inputs(X86Fixture.Read("fortfinishowner", "originals")[0][2]);
        FortFinishScene scene = new(OriginalEdition.Patch1078, input);
        FortFinishOwnerHooks missing = scene.Hooks(false);
        missing.Place = null;
        Assert.Throws<ArgumentException>(() => new RawFortFinishOwner(scene.Pool, scene.Types, scene.Residences, scene.State, missing));
        FortRandomRectangle rect = new(1f, 2f, 1f, 2f);
        uint attempts = 0;
        FortFinishOwnerHooks hooks = scene.Hooks(false);
        hooks.Place = (random, request) =>
        {
            Assert.Equal(1f, request.X);
            Assert.Equal(2f, request.Y);
            attempts++;
            rect = new FortRandomRectangle(100f, 101f, 150f, 151f);
            return 7u;
        };
        RawFortFinishOwner finish = new(scene.Pool, scene.Types, scene.Residences, scene.State, hooks);
        NetstormRandom random = new(0);
        Assert.Equal(7u, finish.PlaceRandom(random, rect, 3, 2, 89, 0));
        Assert.Equal(2u, attempts);
        Assert.Equal(0u, random.State);
        scene.State.PlayerRegions[1] = 3;
        scene.State.ChaffMode = false;
        scene.State.Special = false;
        scene.State.Fort = false;
        scene.Residences.Count = 9;
        Assert.Throws<ArgumentOutOfRangeException>(() => finish.Finish(1));
    }
}
