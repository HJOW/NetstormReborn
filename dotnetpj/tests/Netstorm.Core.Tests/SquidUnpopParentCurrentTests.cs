using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원문 현재 Unpop 부모의 전체 물리 관측과 실제 옆면을 같은 입력으로 비교한다.
namespace Netstorm.Core.Tests;

/// <summary>원본 결과를 수정하지 않고 저장값/현재값·진단 복귀와 전체 공간 체크섬을 검사한다.</summary>
public sealed partial class SquidUnpopParentCurrentTests
{
    /// <summary>원문 장치와 같은 외부 입력/실제 Pool/Hash/지도/옆면 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>두 판본 중 PATCH인지다.</summary>
        private readonly bool _patch;
        /// <summary>원본 실제 raw prefix 폭이다. CD 관리 패딩은 따로 보존한다.</summary>
        private readonly int _width;
        /// <summary>판본별 추가 BYTE 위치다.</summary>
        private readonly int _extra;
        /// <summary>실제 관측 대상 SID다.</summary>
        private readonly int _source;
        /// <summary>원본의 전체 호출 flags다.</summary>
        private readonly uint _flags;
        /// <summary>명시 외부 장치의 현재 효과 입력이다.</summary>
        private readonly uint _action;
        /// <summary>같은 현재 checking DWORD다.</summary>
        private uint _checking;
        /// <summary>최초 옆면 외부 알림 효과를 한 번만 적용한다.</summary>
        private bool _notified;
        /// <summary>원본과 같은 실제 경계 사건 순서다.</summary>
        private readonly StringBuilder _events = new();
        /// <summary>같은 관리 Pool이며 CD는 원본36BYTE prefix를 조립한다.</summary>
        private readonly RawSidPool _pool;
        /// <summary>네 단계의 실제 해시다.</summary>
        private readonly RawSquidHash _hash = new();
        /// <summary>전체65536BYTE 점유 지도다.</summary>
        private readonly byte[] _spots = new byte[65536];
        /// <summary>원문 판본별 현재 TYPE 표이며 PATCH188칸/CD171칸을 실제 옆면과 공유한다.</summary>
        private readonly OriginalTypeRecord[] _types;
        /// <summary>같은 Pool/Hash의 실제 옆면 몸체다.</summary>
        private readonly RawBuildingSpace _building;
        /// <summary>명시 외부 장치와 실제 TYPE/옆면 경계다.</summary>
        private readonly SquidUnpopParentHooks _hooks;

        /// <summary>예약/free 상태도 같은 물리 슬롯으로 읽고 쓴다.</summary>
        private Span<byte> Raw(int sid) => _pool.RawSlot(sid);
        /// <summary>같은 raw의 DWORD를 읽는다.</summary>
        private uint Read(int sid, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(Raw(sid)[offset..]);
        /// <summary>지정한 실제 DWORD 하나만 입력으로 쓴다.</summary>
        private void Put(int sid, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Raw(sid)[offset..], value);
        /// <summary>현재 대상 x 좌표다.</summary>
        private float X => BitConverter.UInt32BitsToSingle(Read(_source, 14));
        /// <summary>현재 대상 y 좌표다.</summary>
        private float Y => BitConverter.UInt32BitsToSingle(Read(_source, 18));
        /// <summary>원문 TYPE의 명시 필드 입력만 채운다.</summary>
        private void Type(int number, uint flags, uint genus, uint width, uint height)
        {
            _types[number].Flags1 = flags; _types[number].Flags2 = genus; _types[number].FootX = (int)width; _types[number].FootY = (int)height;
        }
        /// <summary>진단이 복귀할 때 원본 관측과 같은 입력 효과만 실행한다.</summary>
        private void Assertion(string expression, string file, uint line)
        {
            _events.Append($"A,{line}:{expression}:{file};");
            if (_action == 1 && line == 0x617) { _checking = 0; Array.Fill(_spots, (byte)0xf0); }
            if (_action == 2) { _checking = 0x80000000; Raw(_source)[10] = 71; }
            if (_action == 3) { _checking = 0; Raw(_source)[10] = 70; Raw(_source)[11] = 0; }
            if (_action == 8) { _checking = 0; Raw(_source)[10] = 73; }
            if (_action == 10 && line == 0x65d) _hash.Bucket(0, X, Y) = (ushort)_source;
            if (_action == 11) { _checking = 0x80000000; Raw(_source)[10] = 70; Raw(_source)[11] &= 0xf7; }
        }

        /// <summary>원본 관측의 입력17칸을 같은 장치/물리에 조립한다.</summary>
        public Scene(string[] row, bool patch)
        {
            _patch = patch; _width = patch ? 50 : 36; _extra = patch ? 40 : 35;
            uint[] input = row.Take(17).Select(X86Fixture.UInt).ToArray(); _checking = input[1]; _source = (int)input[2]; _flags = input[13]; _action = input[14];
            _pool = new RawSidPool(24000, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            _types = Enumerable.Range(0, patch ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
            _building = new(_pool, _hash, _types, new() { IslandType = 83, BridgeType = 82 }); Array.Fill(_spots, (byte)input[15]);
            // CD 비균일 입력 프로필도 원본과 같은 셀값으로 공급한다.
            if (!patch && (input[15] & 0x100) != 0)
                for (int cell = 0; cell < _spots.Length; cell++) _spots[cell] = unchecked((byte)(cell * 37 + input[15]));
            // 관측 구간129개 raw와 CD 관리 패딩은 원문 입력0으로 만든다.
            for (int sid = 0; sid <= 128; sid++) Raw(sid).Clear();
            Type(70, input[11], input[10], input[8], input[9]); Type(6, input[11], input[10], input[8], input[9]); Type(71, 0x800, 1, 3, 3); Type(72, 0, 6, 2, 2); Type(73, 0, 0, 1, 1);
            Put(_source, 0, 0x61f000); Raw(_source)[10] = (byte)input[3]; Raw(_source)[11] = (byte)input[4]; Raw(_source)[_extra] = (byte)input[5]; Put(_source, 14, input[6]); Put(_source, 18, input[7]);
            BinaryPrimitives.WriteUInt16LittleEndian(Raw(_source)[4..], 51); BinaryPrimitives.WriteUInt16LittleEndian(Raw(49)[4..], (ushort)_source); Raw(70)[10] = 83;
            if (patch) Raw(70)[12] = 0xff; else BinaryPrimitives.WriteUInt16LittleEndian(Raw(70)[12..], 0xa5ff);
            // 주변 표면만 명시 입력으로 배치한다.
            if (input[16] != 0)
                for (int y = 17; y <= 22; y++)
                    // 원본 x 순서의 실제 표면 WORD다.
                    for (int x = 13; x <= 18; x++) _hash.Cell(0, x, y) = 70;
            uint layout = input[12];
            if (layout is 1 or 4) _hash.Bucket(0, X, Y) = (ushort)_source;
            if (layout == 2 || !patch && layout == 5) _hash.Bucket(1, X, Y) = 49;
            if (layout is 3 or 4) _hash.Bucket(3, X, Y) = (ushort)_source;
            if (!patch && layout == 5) BinaryPrimitives.WriteUInt16LittleEndian(Raw(_source)[4..], 49);
            if (!patch && layout is 6 or 7) { _hash.Bucket(1, X, Y) = (ushort)_source; BinaryPrimitives.WriteUInt16LittleEndian(Raw(_source)[4..], layout == 6 ? (ushort)_source : (ushort)65535); }
            _hooks = new()
            {
                Type = number => new(() => _types[number]), Assertion = Assertion,
                MiniMap = (sid, genus) =>
                {
                    _events.Append($"M,{sid},{Read(_source, 14)},{Read(_source, 18)},{genus};");
                    if (_action == 4) { _checking = 0x80000000; Raw(_source)[10] = 71; Put(_source, 14, 0x41880000); Put(_source, 18, 0x41a80000); _types[70].FootX = _types[70].FootY = 2; }
                    if (_action == 5) Raw(_source)[_extra] |= 8; if (_action == 6) Raw(_source)[11] |= 8; if (_action == 12) _checking = 0;
                },
                Side = (x, y) =>
                {
                    int sid = _building.Surface(x, y); _building.UnpopSide(x, y); _events.Append($"N,{sid};");
                    if (_notified) return; _notified = true;
                    if (_action == 7) { _checking = 0x80000000; Raw(_source)[10] = 71; Put(_source, 14, 0x41880000); Put(_source, 18, 0x41a80000); }
                    if (_action == 9) Raw(_source)[11] |= 8;
                },
                Contained = sid => _events.Append($"D,{sid};Q,{sid};"),
                Update = (sid, flags) => _events.Append($"U,{((flags & 0x2000) != 0 ? 0x8c : 0x88)},{sid},{Raw(sid)[10]},{Raw(sid)[11]};"),
            };
        }

        /// <summary>실제 부모 실행 뒤 원본과 같은 raw prefix/네해시/전체점유 및 사건을 반환한다.</summary>
        public string[] Run()
        {
            RawSquidUnpopParent.Run(_pool, _hash, _spots, _source, _flags, new RawCheckingFlag(() => _checking), _hooks);
            byte[] rawBytes = new byte[129 * _width];
            // 두 판본의 실제 stride만 조밀하게 관측하며 관리 패딩은 원문에 섞지 않는다.
            for (int sid = 0; sid <= 128; sid++)
            {
                Raw(sid)[.._width].CopyTo(rawBytes.AsSpan(sid * _width));
                if (!_patch) Assert.Equal(-1, Raw(sid)[_width..].IndexOfAnyExcept((byte)0));
            }
            byte[] hashBytes = new byte[Enumerable.Range(0, 4).Sum(level => _hash.Entries(level).Length) * 2]; int offset = 0;
            // 원본의 단계/WORD little endian 순서로 전체 해시를 관측한다.
            for (int level = 0; level < 4; level++)
                // 해시 원소 자체를 재정렬하거나 생략하지 않는다.
                foreach (ushort value in _hash.Entries(level)) { BinaryPrimitives.WriteUInt16LittleEndian(hashBytes.AsSpan(offset), value); offset += 2; }
            uint[] values = [_checking, Raw(_source)[10], Raw(_source)[11], Raw(_source)[_extra], Read(_source, 14), Read(_source, 18), (uint)_types[70].FootX, (uint)_types[70].FootY, X86Fixture.Adler32(rawBytes), X86Fixture.Adler32(hashBytes), X86Fixture.Adler32(_spots)];
            return [.. values.Select(value => value.ToString(CultureInfo.InvariantCulture)), _events.Length == 0 ? "-" : _events.ToString()];
        }
    }

    /// <summary>PATCH1300/CD1402개의 최초 관측을 같은 실제 옆면과 전체 물리 체크섬으로 대조한다.</summary>
    [Theory]
    [InlineData(true, "squid-unpop-parent-hardware", 1300)]
    [InlineData(false, "cd-squid-unpop-current-hardware", 1402)]
    public void ParentAndActualSideMatchIndependentPe(bool patch, string name, int count)
    {
        string[][] rows = X86Fixture.Read(name); Assert.Equal(count, rows.Length); int executed = 0;
        X86Fixture.CheckRows(rows, row => { Assert.Equal(29, row.Length); var scene = new Scene(row, patch); Assert.Equal(row[17..], scene.Run()); executed++; return true; });
        Assert.Equal(count, executed);
    }
}
