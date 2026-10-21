using System.Globalization;
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 일반 피해(Damageable Hit·BaseHit)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/DamageableHitTests.cpp 의 HitScene 과 같다: 실제 슬롯 풀·타입 표·레벨/권한/시계 전역 위에서
/// 방어 판정·레벨 피해 보정·HP 쓰기·소유자 통지·섬 경고·burning 을 실행한다.
/// 호출마다 생존 여부·외부 경계 순서·피해자 슬롯 50바이트·사망 기록 8바이트·경고 시각 8바이트를 비교한다.
/// 기준 판본은 10.78 이며 originalCD 행은 읽지 않는다 (W03 삭제·수명 여덟째 조각, 2026-10-21).
/// </summary>
public sealed class X86RawDamageableHitTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 10.72 다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 한 판본의 행 수.</summary>
    private const int RowsPerEdition = 2923;

    /// <summary>피격 대상 슬롯 번호.</summary>
    private const int Source = 50;

    /// <summary>공격자 슬롯 번호.</summary>
    private const int Attacker = 60;

    /// <summary>지면 자산 슬롯 번호.</summary>
    private const int Island = 70;

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

    /// <summary>슬롯 안의 값을 읽는다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽는 바이트 수.</param>
    private static uint Get(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        uint value = 0;
        // 필요한 폭만 낮은 바이트부터 조합한다
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (i * 8);
        }
        return value;
    }

    /// <summary>한 일반 피해 행의 풀·타입 표·전역을 소유하는 재생 장면.</summary>
    private sealed class Scene
    {
        /// <summary>피격 대상 슬롯 풀(24,000칸, 서버 아님).</summary>
        public readonly RawSidPool Pool;

        /// <summary>4단계 공간 해시. CD 판의 지면 조회 보정에만 쓴다.</summary>
        public readonly RawSquidHash Hash = new();

        /// <summary>타입 표(10.78 188개).</summary>
        public readonly OriginalTypeRecord[] Types;

        /// <summary>현재 레벨·권한 전역.</summary>
        public readonly PriestDamageState Levels = new();

        /// <summary>현재 일반 피해 전역.</summary>
        public readonly DamageableHitState State = new();

        /// <summary>현재 burning 전역.</summary>
        public readonly DamageableBurningState BurningState = new();

        /// <summary>나간 외부 경계 순서.</summary>
        public readonly List<string> Events = [];

        /// <summary>고정 fixture 입력 32개.</summary>
        public string[] Row { get; private set; } = [];

        /// <summary>장면을 만든다. 클라이언트 번호 5~70 을 확보한다.</summary>
        public Scene()
        {
            Pool = new RawSidPool(24000, false);
            // 각 독립 입력은 같은 원본 번호를 사용한다
            for (int id = 5; id <= 70; id++)
            {
                Assert.Equal(id, Pool.Allocate(SidAllocation.Client));
            }
            Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 타입 번호 전체에 빈 타입 레코드를 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            State.Levels = Levels;
        }

        /// <summary>슬롯 바이트를 직접 쓴다(검사 입력용).</summary>
        /// <param name="sid">대상 슬롯 번호.</param>
        /// <returns>쓰기 가능한 슬롯 바이트.</returns>
        private Span<byte> Slot(int sid) => Pool.RawSlot(sid);

        /// <summary>현재 double 비트를 판단 없이 입력으로 읽는다.</summary>
        /// <param name="index">입력 칸 번호.</param>
        private double Double(int index) => BitConverter.Int64BitsToDouble(long.Parse(Row[index], CultureInfo.InvariantCulture));

        /// <summary>현재 독립 raw·타입·상태를 native 와 같은 저장 폭으로 초기화한다.</summary>
        /// <param name="value">fixture 입력 32칸.</param>
        public void Prepare(string[] value)
        {
            Assert.Equal(32, value.Length);
            Row = value;
            Events.Clear();
            Hash.Reset();
            Hash.Cell(0, 20, 21) = Island;
            // 타입 표를 원본 기본 상태로 되돌린다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            Types[71].MaxHitPoints = 300;
            Types[71].Flags1 = X86Fixture.UInt(value[14]) != 0 ? 0x10u : 0u;
            Types[71].Flags2 = X86Fixture.UInt(value[15]) != 0 ? 0x400000u : 0u;
            Types[71].DamageEffect = (int)X86Fixture.UInt(value[17]);
            Types[128].Flags2 = X86Fixture.UInt(value[16]);
            Types[90].Flags2 = X86Fixture.UInt(value[22]);
            Levels.Invulnerable = X86Fixture.UInt(value[9]) != 0;
            Levels.LevelCorrectionDisabled = X86Fixture.UInt(value[8]) != 0;
            Array.Clear(Levels.Levels);
            Levels.Levels[X86Fixture.Int(value[6])] = (int)X86Fixture.UInt(value[4]);
            Levels.Levels[X86Fixture.Int(value[7])] = (int)X86Fixture.UInt(value[5]);
            State.Server = X86Fixture.UInt(value[10]) != 0;
            State.Checking = X86Fixture.UInt(value[20]) != 0;
            State.LocalOwner = (byte)X86Fixture.UInt(value[21]);
            State.PrimaryIslands[X86Fixture.Int(value[7])] = (int)X86Fixture.UInt(value[24]);
            State.WarningTime = Double(26);
            State.KilledVictim = 0;
            State.KillerOwner = 0;
            BurningState.Checking = State.Checking;
            Span<byte> raw = Slot(Source);
            // 주변 padding 및 HP 옆 필드도 같은 native 독립 패턴을 유지한다
            for (int i = 0; i < raw.Length; i++)
            {
                raw[i] = (byte)(i * 17 + 3);
            }
            Put(raw, 0, 0x11319800);
            Put(raw, 4, 0, 2);
            Put(raw, 6, 0, 2);
            raw[RawSidPool.TypeOffset] = 71;
            raw[RawSidPool.StateOffset] = (byte)X86Fixture.UInt(value[11]);
            raw[40] = (byte)X86Fixture.UInt(value[12]);
            raw[34] = (byte)X86Fixture.UInt(value[7]);
            Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f));
            Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f));
            Put(raw, 26, X86Fixture.UInt(value[2]), 4);
            Put(raw, 36, 0, 4);
            Span<byte> attacker = Slot(Attacker);
            attacker.Clear();
            attacker[RawSidPool.TypeOffset] = 128;
            Span<byte> island = Slot(Island);
            island.Clear();
            island[RawSidPool.TypeOffset] = 90;
            Put(island, 8, X86Fixture.UInt(value[23]), 2);
        }

        /// <summary>기존 burning 숫자 몸체에 실제 원본과 같은 관찰 find/create/destroy 를 제공한다.</summary>
        /// <returns>burning 훅.</returns>
        public DamageableBurningHooks BurningHooks()
        {
            return new DamageableBurningHooks
            {
                Find = (sid, type, index) =>
                {
                    int found = X86Fixture.UInt(Row[18]) != 0 && index == 0 ? 90 : 0;
                    Events.Add($"B:{sid}:{type}:{index}:{found}");
                    return found;
                },
                Create = sid =>
                {
                    Events.Add($"A:{sid}");
                    return X86Fixture.UInt(Row[19]) != 0 ? 90 : 0;
                },
                Destroy = (sid, flags) => Events.Add($"X:{sid}:{flags}"),
            };
        }

        /// <summary>명시 하위 경계 인자·순서를 관찰하며 상위 일반 피해 몸체는 실제 구현을 실행한다.</summary>
        /// <param name="burning">실제 burning 판정 몸체.</param>
        /// <returns>일반 피해 훅.</returns>
        public DamageableHitHooks Hooks(RawDamageableBurning burning)
        {
            DamageableHitHooks hooks = new()
            {
                RejectDamage = (sid, attacker) =>
                {
                    Events.Add($"R:{sid}:{attacker}");
                    return X86Fixture.UInt(Row[13]) != 0;
                },
                Destroy = (sid, flags) => Events.Add($"D:{sid}:{flags}"),
                OwnerDamageNotice = (owner, victim, attacker) => Events.Add($"N:{owner}:{victim}:{attacker}"),
                Unrealize = sid => Events.Add($"U:{sid}"),
                IslandAt = (x, y) =>
                {
                    Events.Add($"I:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}");
                    return Island;
                },
                RealTime = () =>
                {
                    Events.Add($"W:{Row[25]}");
                    return Double(25);
                },
                WarningEvent = () => Events.Add("V:12"),
                WarningSound = () => Events.Add("S:islandUnderAttack.wav"),
                WarningText = () =>
                {
                    Events.Add("F:islandUnderAttack");
                    Events.Add("L:islandUnderAttack");
                },
            };
            // 같은 풀의 실제 burning 숫자 몸체를 일반 피격 소비자에 연결한다
            hooks.Refresh = sid => burning.Refresh(sid);
            return hooks;
        }

        /// <summary>실제 일반 피해와 공통 HP·burning 을 실행한 뒤 원본과 같은 최종 필드를 직렬화한다.</summary>
        /// <returns>결과 5칸.</returns>
        public string[] Run()
        {
            var hp = new RawHitPoints(Pool, Types, new RawHitPointState());
            var burning = new RawDamageableBurning(Pool, Types, hp, BurningState, BurningHooks());
            var hit = new RawDamageableHit(Pool, Types, State, Hooks(burning));
            bool result = X86Fixture.UInt(Row[1]) != 0
                ? hit.Hit(Source, X86Fixture.Int(Row[3]), X86Fixture.UInt(Row[6]), Attacker)
                : hit.BaseHit(Source, X86Fixture.Int(Row[3]), X86Fixture.UInt(Row[6]), Attacker);
            string actions = string.Join(';', Events);
            var death = new byte[8];
            Put(death, 0, (uint)State.KilledVictim);
            Put(death, 4, State.KillerOwner);
            ulong deadline = (ulong)BitConverter.DoubleToInt64Bits(State.WarningTime);
            var warning = new byte[8];
            Put(warning, 0, (uint)deadline);
            Put(warning, 4, (uint)(deadline >> 32));
            return
            [
                result ? "1" : "0",
                actions,
                Convert.ToHexStringLower(Pool.Slot(Source).ToArray()),
                Convert.ToHexStringLower(death),
                Convert.ToHexStringLower(warning),
            ];
        }
    }

    /// <summary>
    /// 일반 피해 fixture 의 10.78 몫을 재생한다.
    /// 입력 1 이 0 이면 BaseHit, 1 이면 Hit 경로로 실행한다.
    /// </summary>
    [Fact]
    public void HitMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("damageablehit");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != PatchEdition)
            {
                return true;
            }
            var scene = new Scene();
            scene.Prepare(row);
            string[] actual = scene.Run();
            // 원본이 관찰한 순서만 비교한다
            Assert.Equal(row[27], actual[0]);
            Assert.Equal(row[28], actual[1]);
            Assert.Equal(row[29], actual[2]);
            Assert.Equal(row[30], actual[3]);
            Assert.Equal(row[31], actual[4]);
            count++;
            return true;
        });
        Assert.Equal(RowsPerEdition, count);
    }
}
