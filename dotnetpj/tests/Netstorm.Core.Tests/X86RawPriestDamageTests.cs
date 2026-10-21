using System.Globalization;
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 사제 피격(Priest Hit)·레벨 보정·burning 판정을 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/PriestDamageTests.cpp 의 DamageScene 과 같다: 실제 슬롯 풀·타입/프레임 표·HP setter 위에서
/// 회복 취소·프레임 복귀·HP 설정·소유자 통지·burning 을 실행한다.
/// 입력 0 은 레벨 보정만, 1 은 사제 Hit, 2 는 burning 판정만 실행한다.
/// 기준 판본은 10.78 이며 originalCD 행은 읽지 않는다 (W03 삭제·수명 아홉째 조각, 2026-10-21).
/// </summary>
public sealed class X86RawPriestDamageTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 10.72 다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 한 판본의 행 수(보정 336 + Hit 1,152 + burning 224).</summary>
    private const int RowsPerEdition = 1712;

    /// <summary>피격 대상 슬롯 번호.</summary>
    private const int Source = 50;

    /// <summary>공격자 슬롯 번호.</summary>
    private const int Attacker = 60;

    /// <summary>사제 가상 표 값(10.78).</summary>
    private const uint PatchPriestVtable = 0x0050F210;

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

    /// <summary>한 사제 피격 행의 풀·타입/프레임 표·전역을 소유하는 재생 장면.</summary>
    private sealed class Scene
    {
        /// <summary>피격 대상 슬롯 풀(24,000칸, 서버 아님).</summary>
        public readonly RawSidPool Pool;

        /// <summary>타입 표(10.78 188개).</summary>
        public readonly OriginalTypeRecord[] Types;

        /// <summary>타입별 프레임 코드 표(10.78 188개).</summary>
        public readonly FrameCode[][] Frames;

        /// <summary>칸별 점유 지도(65,536칸).</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>사제 HP 변경의 권한·회복 허용 상태.</summary>
        public readonly PriestHitPointMode Mode = new();

        /// <summary>현재 레벨·권한 전역.</summary>
        public readonly PriestDamageState DamageState = new();

        /// <summary>현재 burning 전역.</summary>
        public readonly DamageableBurningState BurningState = new();

        /// <summary>나간 외부 경계 순서.</summary>
        public readonly List<string> Events = [];

        /// <summary>고정 fixture 입력 23개.</summary>
        public string[] Row { get; private set; } = [];

        /// <summary>보정 결과 버퍼 4바이트.</summary>
        public byte[] Output { get; } = new byte[4];

        /// <summary>장면을 만든다. 클라이언트 번호 5~50 을 확보한다.</summary>
        public Scene()
        {
            Pool = new RawSidPool(24000, false);
            // 이후 독립 입력들이 같은 원본 번호를 사용한다
            for (int id = 5; id <= 50; id++)
            {
                Assert.Equal(id, Pool.Allocate(SidAllocation.Client));
            }
            Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 타입 번호 전체에 빈 타입 레코드를 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            Frames = new FrameCode[Types.Length][];
            for (int i = 0; i < Frames.Length; i++)
            {
                Frames[i] = [new FrameCode('A', default, 0, FrameCodeFlags.None)];
            }
        }

        /// <summary>슬롯 바이트를 직접 쓴다(검사 입력용).</summary>
        /// <param name="sid">대상 슬롯 번호.</param>
        /// <returns>쓰기 가능한 슬롯 바이트.</returns>
        private Span<byte> Slot(int sid) => Pool.RawSlot(sid);

        /// <summary>입력 숫자를 그대로 현재 타입·HP·rank·프레임에 반영한다.</summary>
        /// <param name="value">fixture 입력 23칸.</param>
        public void Prepare(string[] value)
        {
            Assert.Equal(23, value.Length);
            Row = value;
            Events.Clear();
            Array.Clear(Output);
            // 타입 표를 원본 기본 상태로 되돌린다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            int type = X86Fixture.Int(value[17]);
            Types[type].MaxHitPoints = X86Fixture.Int(value[18]);
            Types[type].Flags1 = 0x69012;
            Types[type].Flags2 = 0x210000;
            Types[type].DamageEffect = X86Fixture.Int(value[13]);
            Frames[type] = [new FrameCode((char)(65 + X86Fixture.Int(value[12])), default, 0, FrameCodeFlags.None)];
            Mode.Authority = X86Fixture.UInt(value[10]) != 0;
            Array.Fill(Mode.RecoveryAllowed, true);
            DamageState.Invulnerable = X86Fixture.UInt(value[9]) != 0;
            DamageState.Boss = Mode.Authority;
            DamageState.LevelCorrectionDisabled = X86Fixture.UInt(value[8]) != 0;
            Array.Clear(DamageState.Levels);
            DamageState.Levels[X86Fixture.Int(value[6])] = X86Fixture.Int(value[4]);
            DamageState.Levels[X86Fixture.Int(value[7])] = X86Fixture.Int(value[5]);
            BurningState.Checking = X86Fixture.UInt(value[16]) != 0;
            Span<byte> raw = Slot(Source);
            // raw padding 도 native 의 독립 입력 패턴과 동일하게 준비한다
            for (int i = 0; i < raw.Length; i++)
            {
                raw[i] = (byte)(i * 17 + 3);
            }
            Put(raw, 0, PatchPriestVtable);
            Put(raw, 4, 0, 2);
            Put(raw, 6, 0, 2);
            raw[RawSidPool.TypeOffset] = (byte)type;
            raw[RawSidPool.StateOffset] = 0;
            raw[40] = 0;
            raw[34] = (byte)X86Fixture.UInt(value[7]);
            Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f));
            Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f));
            Put(raw, 26, (uint)X86Fixture.Int(value[2]), 4);
            Put(raw, 36, 0, 4);
            Put(Output, 0, (uint)X86Fixture.Int(value[3]));
        }

        /// <summary>원본 가상 Unpop·Repop 관찰 시점의 현재 HP·좌표를 기록한다.</summary>
        /// <param name="marker">기록 표시 문자.</param>
        /// <param name="sid">대상 슬롯 번호.</param>
        /// <param name="flags">호출 플래그.</param>
        /// <param name="hpState">실제 사제 HP setter.</param>
        private void Space(char marker, int sid, uint flags, RawPriestState hpState)
        {
            ReadOnlySpan<byte> raw = Pool.Slot(sid);
            Events.Add($"{marker}:{sid}:{flags}:{hpState.CurrentHitPoints(sid)}:{Get(raw, 14)}:{Get(raw, 18)}");
        }

        /// <summary>nonzero burning 의 명시 조회·생성·삭제·소리 경계다.</summary>
        /// <returns>burning 훅.</returns>
        public DamageableBurningHooks BurningHooks()
        {
            return new DamageableBurningHooks
            {
                Find = (sid, type, index) =>
                {
                    int result = X86Fixture.UInt(Row[14]) != 0 && index == 0 ? 90 : 0;
                    Events.Add($"B:{sid}:{type}:{index}:{result}");
                    return result;
                },
                Create = sid =>
                {
                    Events.Add($"A:{sid}");
                    return X86Fixture.UInt(Row[15]) != 0 ? 90 : 0;
                },
                Destroy = (sid, flags) => Events.Add($"X:{sid}:{flags}"),
                BulfSound = (sid, x, y) =>
                    Events.Add($"S:{sid}:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}"),
            };
        }

        /// <summary>native 경계와 동일한 회복·프레임·소유자 관찰 및 실제 HP setter·burning 을 연결한다.</summary>
        /// <param name="burning">실제 burning 판정 몸체.</param>
        /// <param name="hpState">실제 사제 HP setter.</param>
        /// <returns>사제 피격 훅.</returns>
        public PriestDamageHooks PriestHooks(RawDamageableBurning burning, RawPriestState hpState)
        {
            return new PriestDamageHooks
            {
                HasRegenEvent = sid => X86Fixture.UInt(Row[11]) != 0,
                CancelEvent = (sid, @event) => Events.Add($"C:{sid}:{@event}"),
                DefaultFrame = sid => 0,
                SetFrame = (sid, frame, flags) =>
                {
                    Events.Add($"F:{sid}:{frame}:{flags}");
                    Put(Slot(sid), 36, (uint)frame, 4);
                },
                SetHitPoints = (sid, value) => hpState.SetHitPoints(sid, value, Mode, new PriestHitPointHooks
                {
                    Unpop = (target, flags) => Space('U', target, flags, hpState),
                    Repop = (target, flags) => Space('R', target, flags, hpState),
                }),
                OwnerDamageNotice = (owner, victim, attacker) => Events.Add($"N:{owner}:{victim}:{attacker}"),
                DamageableRefresh = sid => burning.Refresh(sid),
            };
        }

        /// <summary>반환·raw 전체·관찰 순서·보정 buffer 를 원본과 같은 형식으로 출력한다.</summary>
        /// <returns>결과 4칸.</returns>
        public string[] Run()
        {
            var hp = new RawHitPoints(Pool, Types, new RawHitPointState());
            var hpState = new RawPriestState(Pool, hp, Types, Spots);
            var burning = new RawDamageableBurning(Pool, Types, hp, BurningState, BurningHooks());
            var priest = new RawPriestDamage(Pool, Types, Frames, DamageState, PriestHooks(burning, hpState));
            string result = "-";
            switch (X86Fixture.UInt(Row[1]))
            {
                case 0:
                    Put(Output, 0, (uint)RawPriestDamage.Adjust(X86Fixture.Int(Row[3]), X86Fixture.UInt(Row[6]),
                        X86Fixture.UInt(Row[7]), DamageState));
                    break;
                case 1:
                    result = priest.Hit(Source, X86Fixture.Int(Row[3]), X86Fixture.UInt(Row[6]), Attacker) ? "1" : "0";
                    break;
                case 2:
                    result = burning.Refresh(Source).ToString(CultureInfo.InvariantCulture);
                    break;
                default:
                    throw new InvalidOperationException("사제 피격 fixture 몸체 오류");
            }
            string actions = string.Join(';', Events);
            return
            [
                result,
                actions,
                Convert.ToHexStringLower(Pool.Slot(Source).ToArray()),
                Convert.ToHexStringLower(Output),
            ];
        }
    }

    /// <summary>
    /// 사제 피격 fixture 의 10.78 몫을 재생한다.
    /// </summary>
    [Fact]
    public void PriestDamageMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("priestdamage");
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
            // 원본이 관찰한 경계 순서만 비교한다
            Assert.Equal(row[19], actual[0]);
            Assert.Equal(row[20], actual[1]);
            Assert.Equal(row[21], actual[2]);
            Assert.Equal(row[22], actual[3]);
            count++;
            return true;
        });
        Assert.Equal(RowsPerEdition, count);
    }
}
