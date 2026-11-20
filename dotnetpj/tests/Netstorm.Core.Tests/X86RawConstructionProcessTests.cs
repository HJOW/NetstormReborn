using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 세 원본PE의 건설 시작·진행·완료·Form 삭제를 불변 관측값과 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>시간·HP를 기대 계산 없이 원본2868행의 전체 raw와 효과 기록에 대조한다.</summary>
public sealed class X86RawConstructionProcessTests
{
    /// <summary>원문 실행기의 고정 진행음 항목이다.</summary>
    private const uint BuildSound = 0x15000040;
    /// <summary>소리 예약 경계에서 바꿀 현재 항목이다.</summary>
    private const uint ChangedSound = 0x15000200;
    /// <summary>완료음 경계에서 바꿀 현재 준비음이다.</summary>
    private const uint ChangedReady = 0x15000400;

    /// <summary>현재 입력·물리 슬롯·효과 응답만 공급하는 관측 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>판본별 HP·소유자·extra 입력을 받는 실제 풀이다.</summary>
        public readonly RawSidPool Pool;
        /// <summary>현재 부모genus의 명시 입력이다.</summary>
        public readonly OriginalTypeRecord[] Types;
        /// <summary>효과 경계가 변경할 수 있는 현재 전역이다.</summary>
        public readonly ConstructionProcessState State = new();
        /// <summary>Attach가 보관하는 실제 필드 저장소다.</summary>
        public ConstructionProcessFields Fields = new();
        /// <summary>행의 원본29개 DWORD 입력이다.</summary>
        public uint[] Input = [];
        /// <summary>호출 순서대로 기록한 실제 효과 인자다.</summary>
        public readonly List<string> Events = [];
        /// <summary>현재 준비음 항목이다.</summary>
        private uint _ready;
        /// <summary>최대HP 조회 횟수다.</summary>
        private uint _hpCalls;
        /// <summary>현재 판본의 raw 오프셋 선택이다.</summary>
        private bool Patch => Pool.Edition == OriginalEdition.Patch1078;
        /// <summary>현재 부모의 원본SID다.</summary>
        public int Parent => (int)Input[0];
        /// <summary>현재 소유자BYTE 위치다.</summary>
        private int Owner => Patch ? 34 : 32;
        /// <summary>현재abstract BYTE 위치다.</summary>
        private int Extra => Patch ? 40 : 35;

        /// <summary>한 판본의 독립 실행 자료를 준비한다.</summary>
        public Scene(OriginalEdition edition)
        {
            Pool = new(24000, false, null, edition);
            Types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
        }
        /// <summary>이전 행을 지우고 원본 실행기에 공급한 보존 패턴·필드·전역을 넣는다.</summary>
        public void Prepare(string text, bool start)
        {
            Input = text.Split(',').Select(X86Fixture.UInt).ToArray(); Assert.Equal(29, Input.Length);
            Events.Clear(); _hpCalls = 0; _ready = Input[14];
            State.Now = Time(Input[2], Input[3]); State.ProcessType = Input[26]; State.SoundProcessType = Input[27];
            State.LocalPlayer = Input[20]; State.Boss = Input[21] != 0; State.PushGenus = Input[22]; State.DaisType = Input[23];
            State.QuickBuild.Set(Input[24] != 0); State.QuickVortex.Set(Input[25] != 0);
            Types[Input[6]].Flags2 = Input[13];
            Span<byte> raw = Pool.RawSlot(Parent);
            // 관측한 실제 판본 폭의 모든 주변 바이트도 비교 대상이다.
            for (int index = 0; index < raw.Length; index++) raw[index] = unchecked((byte)(index * 29 + 7));
            Put(0, 0x11010000); BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(6, 2), 90);
            raw[10] = (byte)Input[6]; raw[11] = (byte)Input[7]; Put(14, Input[11]); Put(18, Input[12]); PutHp(Input[9]);
            raw[Owner] = (byte)Input[10]; raw[Extra] = (byte)Input[8];
            Fields = start ? new() : new() { Dirty = 0, Start = Time(Input[16], Input[17]), End = Time(Input[18], Input[19]), Sound = BuildSound };
        }
        /// <summary>입력DWORD만 물리 슬롯에 쓴다.</summary>
        private void Put(int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Pool.RawSlot(Parent).Slice(offset, 4), value);
        /// <summary>원본 판본의HP 폭만 바꾸고 옆 바이트는 남긴다.</summary>
        private void PutHp(uint value)
        {
            if (Patch) Put(26, value);
            else BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(Parent).Slice(26, 2), (ushort)value);
        }
        /// <summary>실제 호출 인자만 원문 십진수 표기로 기록한다.</summary>
        private void Event(string label, params uint[] values) => Events.Add(label + string.Concat(values.Select(value => ":" + value.ToString(CultureInfo.InvariantCulture))));

        /// <summary>조건·시각·HP 계산을 대체하지 않고 독립 효과와 경계 변동만 공급한다.</summary>
        public ConstructionProcessHooks Hooks() => new()
        {
            Reserve = () => { Event("A", 56); return (Input[4] & 1) != 0; },
            Effect = (parent, instant) =>
            {
                Event(instant ? "I" : "E", (uint)parent);
                if (Input[5] == 1) State.Now = 7.75;
                if (Input[5] == 2) State.ProcessType = 44;
            },
            Attach = (initial, type, parent, flags) =>
            {
                Event("J", type, (uint)parent, 0, flags); Fields = initial.Copy();
                if (Input[5] == 5) State.Now = 99.5;
                return Fields;
            },
            LoadSound = name => { Events.Add("S:" + name); return Input[5] == 4 ? 0 : BuildSound; },
            ReserveSound = () => { Event("A", 40); if (Input[5] == 3) Fields.Sound = ChangedSound; return (Input[4] & 2) != 0; },
            AttachSound = (parent, sound, alternate, flags) => Event("P", (uint)parent, sound, alternate, flags),
            Refresh = () => { Events.Add("R"); if (Input[5] == 10) State.LocalPlayer = 2; },
            MaxHitPoints = parent =>
            {
                Event("H", (uint)parent); _hpCalls++; uint result = Input[15];
                if (Input[5] == 6) State.Now = 6;
                if (Input[5] == 9) PutHp(Input[15]);
                if (Input[5] == 14 && _hpCalls > 1) result += 7;
                return unchecked((int)result);
            },
            UpdateDisplay = parent => { Event("D", (uint)parent); if (Input[5] == 7) Pool.RawSlot(Parent)[Extra] ^= 1; },
            RemoveContained = (parent, type) => { Event("X", (uint)parent, type, 0, 1); if (Input[5] == 15) Pool.RawSlot(Parent)[Owner] = 2; },
            PlayAt = (x, y, name, loop) =>
            {
                Events.Add($"W:{Bits(x)}:{Bits(y)}:{name}:{loop}");
                if (Input[5] == 11) State.Boss = !State.Boss;
                if (Input[5] == 16) _ready = ChangedReady;
            },
            ReadySound = type => { Assert.Equal(Input[6], type); return _ready; },
            Play = sound => Event("G", sound, 0, 0, 0, 0, 0),
            Unpop = (parent, flags) =>
            {
                Event("U", (uint)parent, flags); Pool.RawSlot(Parent)[11] |= 4;
                if (Input[5] == 13) Pool.RawSlot(Parent)[Extra] |= 0x80;
            },
            Repop = (parent, flags) =>
            {
                Event("Q", (uint)parent, flags); Pool.RawSlot(Parent)[11] &= 0xfb;
                if (Input[5] == 12) { Put(14, Bits(40.5f)); Put(18, Bits(41.25f)); State.PushGenus = 0x200; }
            },
            Push = (x, y, type, owner) =>
            {
                if (Patch) Event("B", Bits(x), Bits(y), type, owner);
                else Event("B", Bits(x), Bits(y), type);
            },
        };
        /// <summary>원본 실행기의Kill은 프로세스를 남기며 현재 권한을 바꿀 수 있다.</summary>
        public bool Kill(uint flags) { Event("K", flags); if (Input[5] == 8) State.Boss = !State.Boss; return true; }
        /// <summary>현재 프로세스 필드의dirty·두double·소리 비트를 직렬화한다.</summary>
        public string FieldText()
        {
            ulong start = BitConverter.DoubleToUInt64Bits(Fields.Start), end = BitConverter.DoubleToUInt64Bits(Fields.End);
            return string.Join(':', new[] { (uint)Fields.Dirty, (uint)start, (uint)(start >> 32), (uint)end, (uint)(end >> 32), Fields.Sound });
        }
        /// <summary>원본 관측 폭만 출력한다. 관리CD 풀의 추가14바이트는 원문 관측 대상이 아니다.</summary>
        public string Raw() => Convert.ToHexString(Pool.Slot(Parent)[..(Patch ? 50 : 36)]).ToLowerInvariant();
    }

    /// <summary>원본double 입력을 두DWORD에서 손실 없이 복원한다.</summary>
    private static double Time(uint low, uint high) => BitConverter.UInt64BitsToDouble(low | ((ulong)high << 32));
    /// <summary>좌표·지속시간의float 비트다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);

    /// <summary>각PE956행의 순서·필드·부모 전체 raw를 같은 본체로 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void Original956(string edition)
    {
        string[][] all = X86Fixture.Read("constructionprocess"); Assert.Equal(2868, all.Length);
        string[][] rows = X86Fixture.Section(all, edition); Assert.Equal(956, rows.Length);
        var scene = new Scene(edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        var process = new RawConstructionProcess(scene.Pool, scene.Types, scene.State, scene.Hooks());
        X86Fixture.CheckRows(rows, row =>
        {
            bool start = row[1] == "Start"; scene.Prepare(row[2], start);
            if (start) process.Start(scene.Parent, BitConverter.UInt32BitsToSingle(scene.Input[1]));
            else if (row[1] == "Run") process.Run(scene.Fields, scene.Parent, scene.Kill);
            else if (row[1] == "Complete") process.Complete(scene.Parent);
            else { Assert.Equal("Destroy", row[1]); process.FormDestroyed(scene.Parent); }
            Assert.Equal(row[3], scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events));
            Assert.Equal(row[4], start && (scene.Input[4] & 1) == 0 ? "-" : scene.FieldText());
            Assert.Equal(row[5], scene.Raw()); return true;
        });
    }
}
