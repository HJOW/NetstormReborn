using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;

// 미사일 기존 장면에 실제 반사탄/플로터의 독립 입력을 추가한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 반사탄1302와 플로터481의 전체 비행/피해/난수 대조다.</summary>
public sealed partial class RawMissileGeometryTests
{
    /// <summary>반사1의 유효 포구/target0/기존 상자/직접 피해/0기간 종료를 비교한다.</summary>
    [Theory]
    [InlineData(0, 144)] [InlineData(1, 1158)]
    public void MatchesAllNativeReflectedMissiles(int kind, int expected)
    {
        var scene = new Scene(3); int count = 0;
        // 실제 네 결과를 작성한 뒤에만 기대열을 읽는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/ice-reflected-x86.tsv"))) { if (line.Length == 0 || line.StartsWith('#')) continue; string[] row = line.Split('\t'); if (N(row[0]) != kind) continue; scene.Prepare(row); Assert.Equal(row.Skip(34), scene.Run()); count++; }
        Assert.Equal(expected, count);
    }
    /// <summary>플로터만 공유 PRNG/현재 전역을 추가하며 실제 Frame/geometry는 기존 본문이다.</summary>
    private sealed class FloaterScene
    {
        /// <summary>원문 썬더 입력을 재사용하는 같은 실제 월드다.</summary>
        internal Scene Current { get; } = new(1);
        /// <summary>실제 두 Frame이 공유하는 난수다.</summary>
        internal GameRandom Random { get; } = new();
        /// <summary>보충 타입의 C++값대입을 재현하는 자체 복사다.</summary>
        private static MissileTypeData Copy(MissileTypeData data) => new() { Range = data.Range, FrameCount = data.FrameCount, FiringSound = data.FiringSound, ImpactSound = data.ImpactSound, DamagePerSecond = data.DamagePerSecond, SpecialFlyerDamage = data.SpecialFlyerDamage, FlyerDamagePerSecond = data.FlyerDamagePerSecond };
        /// <summary>추가 여섯 입력만 현재 raw/TYPE/전역에 공급한다.</summary>
        internal void Prepare(string[] row)
        {
            Assert.Equal(34, row.Length); Current.Prepare(row.Take(23).Concat(row.Skip(29).Take(4)).ToArray()); int source = (int)N(row[27]), display = (int)N(row[28]); Current.Types[source] = Current.Types[131].Clone(); Current.Types[source].Group = source is 71 or 140 ? 0 : 1; Current.Data[source] = Copy(Current.Data[131]); Current.Data[display] = Copy(Current.Data[132]); Current.Pool.RawSlot(50)[10] = (byte)source; Current.Pool.RawSlot(60)[10] = (byte)N(row[23]); Current.Pool.RawSlot(90)[10] = (byte)display; Current.State.FloaterType = N(row[24]); Current.State.AvoidanceCutoff = int.Parse(row[25], CultureInfo.InvariantCulture); Random.SetState(N(row[26]));
        }
        /// <summary>선택 난수는 원문455930의double 반환 순서로 같은 PRNG를 한 번 소비한다.</summary>
        internal void Frame(bool connectRandom = true)
        {
            var mode = new RawGunMode(Current.Pool, Current.Frames, new() { Value = 140 }); var geometry = new RawProjectileGeometry(Current.Pool, Current.Types, Current.GeometryData, gunMode: mode); MissileHooks hooks = Current.Hooks(geometry); hooks.QueueCommand = null; hooks.RandomInteger = null; hooks.ImpactName = null; hooks.Ricochet = _ => Current.Events.Add("R:90:60"); hooks.RandomFloat = connectRandom ? (low, high) => { Current.Events.Add($"N:{B(low)}:{B(high)}"); return (double)low + Random.Next(65536) * (1.0 / 65536.0) * Math.Abs((double)high - low); } : null; new RawMissile(Current.Pool, Current.Types, Current.Data, Current.State, hooks).Frame(Current.Fields);
        }
        /// <summary>기대값을 사용하지 않고 실제 네 이미지/사건과 최종 난수 상태를 반환한다.</summary>
        internal string[] Run(string[] row, bool connectRandom = true)
        {
            Prepare(row); Frame(connectRandom); return [string.Join(';', Current.Events), Hex(Current.Fields.Bytes), string.Concat(new[] { 50, 60, 90 }.Select(sid => Hex(Current.Pool.Slot(sid)))), "0000000000000000", Random.State().ToString(CultureInfo.InvariantCulture)];
        }
    }
    /// <summary>다섯 실제 발사자의 회피/충돌/코너/사거리와 전체 PRNG 상태481행을 비교한다.</summary>
    [Fact]
    public void MatchesAllNativeFloaterMissiles()
    {
        var scene = new FloaterScene(); int count = 0;
        // 예상 출력 다섯 열은 실제 본문이 끝난 뒤에만 비교한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/missile-floater-x86.tsv"))) { if (line.Length == 0 || line.StartsWith('#')) continue; string[] row = line.Split('\t'); Assert.Equal(row.Skip(29), scene.Run(row)); count++; }
        Assert.Equal(481, count);
    }
    /// <summary>회피 뒤의 두 번째 Frame은 passed로 난수를 생략하며 누락 장치는 선행 이동을 보존한다.</summary>
    [Fact]
    public void FloaterConsumesRandomOnceAndMissingRandomRetainsMovement()
    {
        string[] row = File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/missile-floater-x86.tsv")).Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t')).First(row => N(row[23]) == 146 && N(row[24]) == 146 && row[25] == "800" && N(row[26]) == 0xfed && N(row[27]) == 131 && N(row[9]) == 4 && N(row[10]) == 0 && N(row[12]) == 0 && N(row[20]) == 0); var scene = new FloaterScene(); scene.Run(row); Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Current.Fields.Bytes.AsSpan(16))); uint saved = scene.Random.State(); scene.Current.State.Now += .01; scene.Frame(); Assert.Equal(saved, scene.Random.State()); Assert.Single(scene.Current.Events, value => value == "N:0:1148846080"); Assert.Throws<InvalidOperationException>(() => scene.Run(row, false)); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Current.Fields.Bytes.AsSpan(16))); Assert.Equal(0xfedu, scene.Random.State()); Assert.Contains("U:90:0", scene.Current.Events);
    }
}
