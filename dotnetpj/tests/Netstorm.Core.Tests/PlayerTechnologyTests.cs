using System.Buffers.Binary;
using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;

// 실제 기술 내용물·목록을 독립 원본 기계어에 대조하는 검사다.
namespace Netstorm.Tests;

/// <summary>cpppj/tests/PlayerTechnologyTests.cpp의158입력과 실제타입 연결·무진행 경계 검사다.</summary>
public sealed class PlayerTechnologyTests
{
    /// <summary>원본 격리 입력의 가상삭제·부착 경계 표식이다.</summary>
    private const uint TechnologyVtable = 0x12010000;
    /// <summary>선택한 폭의 원문 바이트만 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value, int width = 4)
    {
        // 원문 필드 폭의 낮은 바이트부터 쓴다.
        for (int index = 0; index < width; index++) raw[offset + index] = unchecked((byte)(value >> (index * 8)));
    }
    /// <summary>선택한 폭의 정렬되지 않은 원문 정수를 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint result = 0;
        // 낮은 바이트부터 결합한다.
        for (int index = 0; index < width; index++) result |= (uint)raw[offset + index] << (index * 8);
        return result;
    }
    /// <summary>실제 바이트를 원본과 같은 소문자16진수로 쓴다.</summary>
    private static string Hex(ReadOnlySpan<byte> raw) => Convert.ToHexString(raw).ToLowerInvariant();

    /// <summary>같은 풀·기술부모·내용물·생성기를 한 장면 수명으로 소유한다.</summary>
    private sealed class Scene
    {
        /// <summary>실제clientSID5부터 새Form을 할당하는 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768);
        /// <summary>실제188TYPE이며 검사 대상에는 앞48개만 빌린다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>모든 부모가 공유하는9Player원문이다.</summary>
        public readonly byte[] Records = new byte[1620];
        /// <summary>실제PlayerKnown진단과 기술 조회 부모다.</summary>
        public readonly RawPlayerWorld Player;
        /// <summary>실제contained커서·flags/kind조회다.</summary>
        public readonly RawCarrierPreDestroy Finder;
        /// <summary>기존Factory·Form필드 쓰기·부착 생성기다.</summary>
        public readonly RawContainedCreate Create;
        /// <summary>현재 기술 목록 처리기다.</summary>
        public readonly RawPlayerTechnology Technology;
        /// <summary>독립 입력의 소유자 번호다.</summary>
        public readonly int Owner;
        /// <summary>실제 조회·진단·생성·부착·삭제의 순서다.</summary>
        public readonly List<string> Trace = [];
        /// <summary>실제Factory가 만든 새번호의 순서다.</summary>
        public readonly List<int> Born = [];

        /// <summary>fixture의 입력만 읽어 원문 체인을 준비한다. 기대 출력은 읽지 않는다.</summary>
        public Scene(string input)
        {
            string[] fields = input.Split('|');
            long[] numbers = fields[0].Split(',').Select(X86Fixture.Long).ToArray();
            Owner = (int)numbers[0];
            uint known = (uint)numbers[2];
            bool pattern = numbers[3] != 0, mutate = numbers[4] != 0;
            Put(Records, Owner * 180 + 0x9c, 50);
            // 기대all목록을 계산하지 않고 실제TYPE이름과group입력만 구성한다.
            for (int index = 0; index < Types.Length; index++)
            {
                Types[index].Name = "T" + index;
                Types[index].Group = index % 7 == 0 ? 10 : index % 10;
            }
            // 내장Form의 실제 생성자 주소만 제공한다.
            foreach (int type in new[] { 1, 2, 3, 6 }) Types[type].ConstructorAddress = OriginalConstructors.Address(OriginalEdition.Patch1078, type);
            // 부모·네 내용물의 주변 바이트를 원본 하네스와 같게 초기화한다.
            foreach (int sid in new[] { 50, 51, 60, 61, 62, 63 }) Pool.RawSlot(sid).Clear();
            Put(Pool.RawSlot(50), 0, TechnologyVtable); Pool.RawSlot(50)[10] = 152;
            if (pattern)
            {
                Put(Pool.RawSlot(50), 6, 60, 2);
                uint[] kinds = [1, 2, 2, 7], flags = [4, 4, 1, 4];
                // 비contained타입·flags1·flags4를 같은 실제 체인에 섞는다.
                for (int index = 0; index < 4; index++)
                {
                    Span<byte> raw = Pool.RawSlot(60 + index);
                    Put(raw, 0, TechnologyVtable); raw[10] = index == 1 ? (byte)46 : (byte)6;
                    Put(raw, 4, index < 3 ? (uint)(61 + index) : 0, 2);
                    Put(raw, 18, kinds[index], 2); Put(raw, 20, flags[index], 2);
                }
            }
            var post = new RawPostPop(Pool, Types, new RawPostPopState());
            Player = new RawPlayerWorld(Pool, post, Records, new PlayerWorldState { PlayerKnown = known },
                new PlayerWorldHooks { Assertion = value => { Assert.Equal(PlayerWorldAssertion.PlayerIdKnown, value); Trace.Add("A,669"); } });
            Finder = new RawCarrierPreDestroy(Pool, new ContainedFinderState(), new CarrierPreDestroyHooks
            {
                ValidateDamageable = (_, _) => throw new InvalidOperationException("선택하지 않은 피해 검증"),
                DamageablePre = (_, _) => throw new InvalidOperationException("선택하지 않은 피해 후처리"),
                DependentFound = () => throw new InvalidOperationException("선택하지 않은 종속 통지"),
            });
            var factory = new RawSquidFactory(Pool, Types);
            Create = new RawContainedCreate(Pool, new ContainedCreateState(), new ContainedCreateHooks
            {
                Create = (type, flags) =>
                {
                    Trace.Add($"C,{type},{flags}"); Assert.Equal(6u, type);
                    int sid = factory.CreateContained(flags); Born.Add(sid); return sid;
                },
                NotifySurface = _ => throw new InvalidOperationException("선택하지 않은 표면 통지"),
                Attach = (sid, parent, flags) =>
                {
                    Trace.Add($"B,{sid},{parent},{flags}");
                    Put(Pool.AllocatedBytes(sid), 4, Read(Pool.Slot(parent), 6, 2), 2);
                    Put(Pool.RawSlot(parent), 6, (uint)sid, 2);
                },
            });
            Technology = new RawPlayerTechnology(Player, Finder, Create, Types.Take(48).ToArray(), new PlayerTechnologyHooks
            {
                FindType = name =>
                {
                    Trace.Add("F," + Hex(Encoding.Latin1.GetBytes(name)));
                    // 명시 입력 이름의 첫 일치를 조회하며 기대 목록을 계산하지 않는다.
                    for (int index = 0; index < Types.Length; index++)
                        if (string.Equals(name, Types[index].Name, StringComparison.OrdinalIgnoreCase)) return index;
                    return -1;
                },
                Assertion = where => Trace.Add(where == PlayerTechnologyAssertion.FindTypeRange ? "A,867" : "A,886"),
                Destroy = (sid, flags) =>
                {
                    Trace.Add($"D,{sid},{flags}");
                    int previous = 50, field = 6;
                    uint current = Read(Pool.Slot(previous), field, 2);
                    // 외부삭제 응답은 같은 현재 체인에서 해당SID만 제거한다.
                    while (current != 0 && current != sid)
                    { previous = (int)current; field = 4; current = Read(Pool.Slot(previous), field, 2); }
                    Assert.Equal((uint)sid, current);
                    Put(Pool.RawSlot(previous), field, Read(Pool.Slot(sid), 4, 2), 2);
                    if (mutate) Put(Records, Owner * 180 + 0x9c, 51);
                },
            });
        }

        /// <summary>관찰 대상 부모·네 내용물의 전체50바이트를 순서대로 쓴다.</summary>
        public string Raw() => string.Join(':', new[] { 50, 60, 61, 62, 63 }.Select(sid => Hex(Pool.Slot(sid))));
        /// <summary>실제 생성된 새Form전체를 할당 순서로 쓴다.</summary>
        public string BornHex() => Born.Count == 0 ? "-" : string.Join(':', Born.Select(sid => Hex(Pool.Slot(sid))));
    }

    /// <summary>원본158고유입력의 반환·목록·전체raw·새Form·모든 효과와 기술부모 변경을 전수 대조한다.</summary>
    [Fact]
    public void Native158()
    {
        string[][] rows = X86Fixture.Read("player-technology").Where(row => row[0] != "kind").ToArray();
        Assert.Equal(158, rows.Length);
        Assert.Equal(158, rows.Select(row => (row[0], row[1])).Distinct().Count());
        X86Fixture.CheckRows(rows, row =>
        {
            var scene = new Scene(row[1]);
            string[] fields = row[1].Split('|');
            int type = int.Parse(fields[0].Split(',')[1], System.Globalization.CultureInfo.InvariantCulture);
            string text = Encoding.Latin1.GetString(Convert.FromHexString(fields[1]));
            string result = "-", values = "-";
            switch (row[0])
            {
                case "find": result = scene.Technology.Find(scene.Owner, type).ToString(); break;
                case "clear": scene.Technology.Clear(scene.Owner); break;
                case "ensure": scene.Technology.Ensure(scene.Owner, type); break;
                case "add": scene.Technology.AddList(scene.Owner, text); break;
                case "parse":
                case "stalled":
                    var list = new PlayerTechnologyList();
                    // 정상 종료 또는 같은 미등록 항목의 두 무진행 반복까지만 관찰한다.
                    while (true)
                    {
                        PlayerTechnologyParseStep step = scene.Technology.ParseStep(text, list);
                        if (step == PlayerTechnologyParseStep.Complete) break;
                        if (step == PlayerTechnologyParseStep.Stalled)
                        {
                            Assert.Equal("stalled", row[0]);
                            Assert.Equal(PlayerTechnologyParseStep.Stalled, scene.Technology.ParseStep(text, list));
                            result = list.Index.ToString(); break;
                        }
                    }
                    if (list.Count > 0) values = string.Join(',', list.Types.Take((int)list.Count));
                    break;
                default: throw new InvalidOperationException("알 수 없는 원본 기술 검사 종류");
            }
            return result == row[2] && (scene.Trace.Count == 0 ? "-" : string.Join(';', scene.Trace)) == row[3]
                && values == row[4] && scene.Raw() == row[5] && scene.BornHex() == row[6]
                && Read(scene.Records, scene.Owner * 180 + 0x9c).ToString() == row[7];
        });
    }

    /// <summary>실제 보호TYPE이름 조회를 직접 연결하며 무진행 오류를 기술 생성 전에 보고한다.</summary>
    [Fact]
    public void ActualTypeBindingsAndStalledBoundary()
    {
        string data = Environment.GetEnvironmentVariable("NETSTORM_DATA") ?? throw new InvalidOperationException("NETSTORM_DATA실제 자료 루트 필요");
        var archive = TaffArchive.Open(Path.Combine(data, "netstorm.tarc"));
        var sources = new List<RiftTypeSource>();
        // 실제 원본 아카이브의116개 타입을 정해진 로딩 순서로 읽는다.
        foreach (string name in RiftTypeLoadOrder.Get(OriginalEdition.Patch1078))
        {
            Assert.True(archive.TryFind("d/" + name + ".type", out TaffEntry entry));
            sources.Add(new(name, RiftTypeDefinition.Parse(OriginalText.DecodeWindows1252(archive.Read(entry)))));
        }
        var actual = new RiftTypeTable(OriginalEdition.Patch1078, sources);
        var scene = new Scene("1,0,1,0,0|");
        var hooks = RawPlayerTechnology.BindPlayerTechnologyTypes(actual, new PlayerTechnologyHooks());
        var technology = new RawPlayerTechnology(scene.Player, scene.Finder, scene.Create, actual.Types, hooks);
        var list = new PlayerTechnologyList();
        // 실제표에서 서로 다른3개 기술 이름을 조회하는 원문 입력이다.
        const string names = "sunWalker;windBattery;sunCannon";
        Assert.Equal(PlayerTechnologyParseStep.Progress, technology.ParseStep(names, list));
        Assert.Equal(actual.Find("sunWalker"), list.Types[0]);
        Assert.Equal(PlayerTechnologyParseStep.Progress, technology.ParseStep(names, list));
        Assert.Equal(PlayerTechnologyParseStep.Progress, technology.ParseStep(names, list));
        Assert.Equal(PlayerTechnologyParseStep.Complete, technology.ParseStep(names, list));
        Assert.Equal(3u, list.Count);
        Assert.Throws<InvalidOperationException>(() => technology.AddList(1, "sunWalker;missing;sunCannon"));
        Assert.Empty(scene.Born);
    }
}
