using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 사제 회복(RawPriestRegen.Handle)을 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/PriestRegenTests.cpp 의 패치 재생과 같다: 같은 풀·보상·사제·모드·난수·상태·추적으로 Handle 한 번을 실행한다.
/// 반환 비트·사건·난수·추적·슬롯 전체를 대조한다. 기준 판본은 10.78 이며 CD·10.37 행은 C# 패치 범위가 아니라 건너뛴다 (R0-4 재대조, 2026-10-28).
/// </summary>
public sealed class X86RawPriestRegenTests
{
    /// <summary>사제 회복 이벤트 번호.</summary>
    private const uint PriestRegenEvent = 0x25a;

    /// <summary>원본 관찰의 client 슬롯 번호.</summary>
    private const int SourceSid = 50;

    /// <summary>사제 타입 번호.</summary>
    private const int PriestType = 158;

    /// <summary>사제 패치 가상 표 값.</summary>
    private const uint PatchPriestVtable = 0x0050F210u;

    /// <summary>원본 전환의 공간 효과 경계와 HP 조회 순서를 재생하는 검사 장면이다.</summary>
    private sealed class RegenScene
    {
        /// <summary>객체 슬롯 풀. 각 관찰이 같은 SID와 stale 슬롯 내용으로 시작한다.</summary>
        public RawSidPool Pool { get; } = new(24000, false);

        /// <summary>원본 전역 난수.</summary>
        public NetstormRandom Random { get; } = new(0);

        /// <summary>패치판 SP 저장소.</summary>
        public RawSpStore Store { get; }

        /// <summary>등록 목록과 개수의 장부.</summary>
        public RawPostPopState Book { get; } = new();

        /// <summary>보상 계산이 읽는 판본별 전역.</summary>
        public SquidRewardState HpMode { get; } = new();

        /// <summary>타입 번호 순서의 타입 표 188개.</summary>
        public OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, OriginalConstructors.TypeCount)
            .Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>칸별 점유·통행 비트 지도.</summary>
        public byte[] Spots { get; } = new byte[65536];

        /// <summary>HP 조회·변경 처리기.</summary>
        public RawSquidReward Hp { get; }

        /// <summary>HP 조회 몸체. 사제 상태와 같은 풀·타입을 쓴다.</summary>
        public RawHitPoints HitPoints { get; }

        /// <summary>HP 조회 전역.</summary>
        public RawHitPointState HitState { get; } = new();

        /// <summary>사제 상태 처리기.</summary>
        public RawPriestState Priest { get; }

        /// <summary>동작 분기를 선택하는 모드.</summary>
        public PriestHitPointMode Mode { get; } = new();

        /// <summary>처리 중 조회·갱신하는 상태.</summary>
        public PriestRegenState State { get; } = new();

        /// <summary>발생한 사건 기록.</summary>
        public string Events { get; private set; } = string.Empty;

        /// <summary>후속 V 쓰기 값("-"이면 쓰지 않는다).</summary>
        public string After { get; set; } = "-";

        /// <summary>후속 V 쓰기 소유자.</summary>
        public byte AfterOwner { get; set; }

        /// <summary>감사 변경 허용 여부.</summary>
        public bool AuditChange { get; set; }

        /// <summary>점유 결과.</summary>
        public bool Occupied { get; set; }

        /// <summary>측정 결과.</summary>
        public double Measurement { get; set; }

        /// <summary>같은 풀·난수·저장소로 장치를 조립한다.</summary>
        public RegenScene()
        {
            Store = new RawSpStore(Random, () => 1u);
            Hp = new RawSquidReward(Pool, Types, Book, HpMode, Store);
            HitPoints = new RawHitPoints(Pool, Types, HitState);
            Priest = new RawPriestState(Pool, HitPoints, Types, Spots);
            // 각 관찰이 같은 SID와 stale 슬롯 내용으로 시작한다
            for (int id = 5; id <= SourceSid; id++)
            {
                Assert.Equal(id, Pool.Allocate(SidAllocation.Client));
            }
        }

        /// <summary>외부 호출 순서에 구분자를 붙인다.</summary>
        /// <param name="value">추가할 기록.</param>
        public void Event(string value)
        {
            Events += Events.Length == 0 ? value : ";" + value;
        }

        /// <summary>기록 대체는 호출 시점의 현재 raw 값을 관찰한다.</summary>
        /// <param name="marker">기록 구분자.</param>
        /// <param name="sid">대상 슬롯 번호.</param>
        public void Record(char marker, int sid)
        {
            ReadOnlySpan<byte> raw = Pool.Slot(sid);
            Event(marker + ":" + sid + ':' + Priest.CurrentHitPoints(sid) + ':' + raw[34] + ':' + SpGet(raw, 14) + ':' + SpGet(raw, 18));
            if (marker == 'V' && After != "-")
            {
                Span<byte> target = Pool.AllocatedBytes(sid);
                SpPut(target, 26, unchecked((uint)int.Parse(After)));
                target[34] = AfterOwner;
            }
        }

        /// <summary>StateOracle과 같은 U/R 호출 기록이며 여기서 외부 raw 변화는 V에만 공급한다.</summary>
        public PriestHitPointHooks Space()
        {
            void RecordSpace(char marker, int sid, uint flags)
            {
                ReadOnlySpan<byte> raw = Pool.Slot(sid);
                Event(marker + ":" + sid + ':' + flags + ':' + Priest.CurrentHitPoints(sid) + ':' + SpGet(raw, 14) + ':' + SpGet(raw, 18));
            }
            return new PriestHitPointHooks
            {
                Unpop = (sid, flags) => RecordSpace('U', sid, flags),
                Repop = (sid, flags) => RecordSpace('R', sid, flags),
            };
        }

        /// <summary>실제 공간·표시가 아직 없는 경계를 독립 실행기의 입력과 같은 방식으로 공급한다.</summary>
        public PriestRegenHooks Hooks()
        {
            return new PriestRegenHooks
            {
                Refresh = sid => Record('V', sid),
                Damageable = sid => Record('D', sid),
                Shape = type =>
                {
                    Event("F:" + type);
                    return 0x12345678u;
                },
                Occupied = (sid, shape) =>
                {
                    Event("O:" + sid + ':' + shape + ":0:0");
                    return Occupied;
                },
                NeutralAction = sid => Record('N', sid),
                PatchAudit = () =>
                {
                    Event("A");
                    if (AuditChange)
                    {
                        State.Patch.ObservedSequence = State.Patch.Sequence;
                        State.Patch.Remaining = 1;
                    }
                },
                Measurement = () =>
                {
                    Event("M");
                    return Measurement;
                },
            };
        }

        /// <summary>fixture는 입력만 공급한다. HP 계산·분기 기대값은 준비 코드에 없다.</summary>
        /// <param name="row">입력 행 31칸.</param>
        public void Prepare(string[] row)
        {
            Assert.Equal(31, row.Length);
            Events = string.Empty;
            OriginalTypeRecord type = Types[PriestType];
            type.MaxHitPoints = int.Parse(row[1]);
            type.Flags1 = 0x69012u;
            type.Flags2 = 0x210000u;
            HpMode.PriestQuarterHp = row[2] != "0";
            HitState.PriestQuarterHp = row[2] != "0";
            Span<byte> raw = Pool.AllocatedBytes(SourceSid);
            raw.Fill(0xab);
            SpPut(raw, 0, PatchPriestVtable);
            raw[10] = PriestType;
            raw[11] = 0;
            SpPut(raw, 26, X86Fixture.UInt(row[3]));
            raw[34] = (byte)X86Fixture.UInt(row[6]);
            raw[40] = (byte)X86Fixture.UInt(row[7]);
            SpPut(raw, 14, X86Fixture.UInt(row[8]));
            SpPut(raw, 18, X86Fixture.UInt(row[9]));
            Mode.Authority = row[4] != "0";
            Mode.RecoveryAllowed = new bool[10];
            Mode.RecoveryAllowed[X86Fixture.UInt(row[6])] = row[5] != "0";
            State.NeutralBlocked = row[10] != "0";
            State.ModeBlocked = row[11] != "0";
            Occupied = row[12] != "0";
            Random.SetState(X86Fixture.UInt(row[15]));
            State.Patch.Sample = 0;
            State.Patch.Sequence = int.Parse(row[16]);
            State.Patch.ObservedSequence = int.Parse(row[17]);
            State.Patch.ChangedSample = X86Fixture.UInt(row[18]);
            State.Patch.Remaining = X86Fixture.UInt(row[19]);
            State.Patch.Gate = X86Fixture.UInt(row[20]);
            State.Patch.Sentinel = short.Parse(row[21]);
            Measurement = double.Parse(row[22], CultureInfo.InvariantCulture);
            After = row[23];
            AfterOwner = (byte)X86Fixture.UInt(row[24]);
            AuditChange = row[25] != "0";
        }

        /// <summary>추적 sequence는 signed 판단 뒤 raw DWORD의 출력 비트로 비교한다.</summary>
        public string Tracking()
        {
            PriestRegenPatchState t = State.Patch;
            return t.Sample + "," + unchecked((uint)t.Sequence) + "," + unchecked((uint)t.ObservedSequence) + ',' +
                t.ChangedSample + ',' + t.Remaining + ',' + t.Gate + ',' + t.Sentinel;
        }
    }

    /// <summary>명시 32비트 값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="bytes">쓸 물리.</param>
    /// <param name="at">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void SpPut(Span<byte> bytes, int at, uint value)
    {
        // 인접 바이트는 보존한다
        for (int b = 0; b < 4; b++)
        {
            bytes[at + b] = (byte)(value >> (8 * b));
        }
    }

    /// <summary>실제 현재 출력 폭을 검사 관측용 DWORD로 읽는다.</summary>
    /// <param name="bytes">읽을 물리.</param>
    /// <param name="at">읽기 시작 위치.</param>
    private static uint SpGet(ReadOnlySpan<byte> bytes, int at)
    {
        uint value = 0;
        // 관측 폭 밖의 바이트는 보지 않는다
        for (int b = 0; b < 4; b++)
        {
            value |= (uint)bytes[at + b] << (b * 8);
        }
        return value;
    }

    /// <summary>슬롯 바이트를 Native의 소문자 16진 표로 바꾼다.</summary>
    /// <param name="bytes">바꿀 물리 전체.</param>
    private static string SpHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        var text = new System.Text.StringBuilder(bytes.Length * 2);
        // 원문 순서와 0을 생략하지 않는다
        foreach (byte value in bytes)
        {
            text.Append(digits[value >> 4]);
            text.Append(digits[value & 15]);
        }
        return text.ToString();
    }

    /// <summary>
    /// 해당 판본의 반환 float 비트·사건·전역 난수·추적·raw 슬롯 전체를 재생한다.
    /// </summary>
    [Fact]
    public void PriestRegenPatchX86()
    {
        string[][] rows = X86Fixture.ReadFile("priestregen-x86.tsv");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != "originals")
            {
                return true;
            }
            var scene = new RegenScene();
            scene.Prepare(row);
            var regen = new RawPriestRegen(scene.Pool, scene.Hp, scene.Priest, scene.Mode, scene.Space(), scene.Random, scene.State, scene.Hooks());
            float result = regen.Handle(SourceSid, PriestRegenEvent, X86Fixture.UInt(row[13]), BitConverter.UInt32BitsToSingle(X86Fixture.UInt(row[14])));
            Assert.Equal(X86Fixture.UInt(row[26]), BitConverter.SingleToUInt32Bits(result));
            Assert.Equal(row[27], scene.Events.Length == 0 ? "-" : scene.Events);
            Assert.Equal(row[28], scene.Tracking());
            Assert.Equal(X86Fixture.UInt(row[29]), scene.Random.State);
            Assert.Equal(row[30], SpHex(scene.Pool.Slot(SourceSid)));
            count++;
            return true;
        });
        Assert.Equal(1513, count);
    }
}
